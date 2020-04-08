
/*
 * The olsr.org Optimized Link-State Routing daemon version 2 (olsrd2)
 * Copyright (c) 2004-2013, the olsr.org team - see HISTORY file
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "common/common_types.h"
#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/string.h"

#include "config/cfg.h"
#include "config/cfg_schema.h"
#include "config/cfg_db.h"

static struct cfg_section_type *_alloc_section(struct cfg_db *, const char *);
static void _free_sectiontype(struct cfg_section_type *);

static struct cfg_named_section *_alloc_namedsection(
    struct cfg_section_type *, const char *);
static void _free_namedsection(struct cfg_named_section *);

static struct cfg_entry *_alloc_entry(
    struct cfg_named_section *, const char *);
static void _free_entry(struct cfg_entry *);

/**
 * @return new configuration database without entries,
 *   NULL if no memory left
 */
struct cfg_db *
cfg_db_add(void) {
  struct cfg_db *db;

  db = calloc(1, sizeof(*db));
  if (db) {
    avl_init(&db->sectiontypes, cfg_avlcmp_keys, false);
  }
  return db;
}

/**
 * Removes a configuration database including all data from memory
 * @param db pointer to configuration database
 */
void
cfg_db_remove(struct cfg_db *db) {
  struct cfg_section_type *section, *section_it;

  CFG_FOR_ALL_SECTION_TYPES(db, section, section_it) {
    _free_sectiontype(section);
  }
  free(db);
}

/**
 * Copy parts of a configuration database into a new db
 * @param dst pointer to target db
 * @param src pointer to source database
 * @param section_type section type to copy, NULL for all types
 * @param section_name section name to copy, NULL for all names
 * @param entry_name entry name to copy, NULL for all entries
 * @return 0 if copy was successful, -1 if an error happened.
 *   In case of an error, the destination might contain a partial
 *   copy.
 */
int
_cfg_db_append(struct cfg_db *dst, struct cfg_db *src,
    const char *section_type, const char *section_name, const char *entry_name) {
  struct cfg_section_type *section, *section_it;
  struct cfg_named_section *named, *named_it;
  struct cfg_entry *entry, *entry_it;
  char *ptr;
  bool dummy;

  CFG_FOR_ALL_SECTION_TYPES(src, section, section_it) {
    if (section_type != NULL && cfg_cmp_keys(section->type, section_type) != 0) {
      continue;
    }

    CFG_FOR_ALL_SECTION_NAMES(section, named, named_it) {
      if (section_name != NULL && cfg_cmp_keys(named->name, section_name) != 0) {
        continue;
      }

      if (_cfg_db_add_section(dst, section->type, named->name, &dummy) == NULL) {
        return -1;
      }

      CFG_FOR_ALL_ENTRIES(named, entry, entry_it) {
        if (entry_name != NULL && cfg_cmp_keys(entry->name, entry_name) != 0) {
          continue;
        }

        strarray_for_each_element(&entry->val, ptr) {
          if (cfg_db_set_entry_ext(
              dst, section->type, named->name, entry->name, ptr, true, false) == NULL) {
            return -1;
          }
        }
      }
    }
  }
  return 0;
}

/**
 * Adds a named section to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param new_section pointer to bool, will be set to true if a new
 *   section was created, false otherwise
 * @return pointer to named section, NULL if an error happened
 */
struct cfg_named_section *
_cfg_db_add_section(struct cfg_db *db, const char *section_type,
    const char *section_name, bool *new_section) {
  struct cfg_section_type *section;
  struct cfg_named_section *named = NULL;

  /* consistency check */
  assert (section_type);

  *new_section = false;

  /* get section */
  section = avl_find_element(&db->sectiontypes, section_type, section, node);
  if (section == NULL) {
    section = _alloc_section(db, section_type);
    if (section == NULL) {
      return NULL;
    }
    *new_section = true;
  }
  else {
    /* get named section */
    named = avl_find_element(&section->names, section_name, named, node);
  }

  if (named == NULL) {
    named = _alloc_namedsection(section, section_name);
    *new_section = true;
  }

  return named;
}

/**
 * Removes a section type (including all namedsections and entries of it)
 * from a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @return -1 if section type did not exist, 0 otherwise
 */
int
cfg_db_remove_sectiontype(struct cfg_db *db, const char *section_type) {
  struct cfg_section_type *section;

  /* find section */
  section = cfg_db_find_sectiontype(db, section_type);
  if (section == NULL) {
    return -1;
  }

  _free_sectiontype(section);
  return 0;
}

/**
 * Finds a named section object inside a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed section
 * @return pointer to named section, NULL if not found
 */
struct cfg_named_section *
cfg_db_find_namedsection(
    struct cfg_db *db, const char *section_type, const char *section_name) {
  struct cfg_section_type *section;
  struct cfg_named_section *named = NULL;

  section = cfg_db_find_sectiontype(db, section_type);
  if (section != NULL) {
    named = avl_find_element(&section->names, section_name, named, node);
  }
  return named;
}

/**
 * Removes a named section (including all entries of it)
 * from a configuration database.
 * If the section_type below is empty afterwards, you might
 * want to delete it too.
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed section
 * @return -1 if section type did not exist, 0 otherwise
 */
int
cfg_db_remove_namedsection(struct cfg_db *db, const char *section_type,
    const char *section_name) {
  struct cfg_named_section *named;

  named = cfg_db_find_namedsection(db, section_type, section_name);
  if (named == NULL) {
    return -1;
  }

  /* only free named section */
  _free_namedsection(named);
  return 0;
}

/**
 * Sets an entry to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @param value entry value
 * @param append true if the value should be appended to a list,
 *   false if it should overwrite all old values
 * @param front true if the value should be put in front of existing
 *   values, false if it should be put at the end
 * @return pointer to cfg_entry, NULL if an error happened
 */
struct cfg_entry *
cfg_db_set_entry_ext(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value,
    bool append, bool front) {
  struct cfg_entry *entry;
  struct cfg_named_section *named;
  bool new_section = false, new_entry = NULL;

  /* create section */
  named = _cfg_db_add_section(db, section_type, section_name, &new_section);
  if (!named) {
    return NULL;
  }

  /* get entry */
  entry = avl_find_element(&named->entries, entry_name, entry, node);
  if (!entry) {
    entry = _alloc_entry(named, entry_name);
    if (!entry) {
      goto set_entry_error;
    }
    new_entry = true;
  }

  if (!append) {
    strarray_free(&entry->val);
  }
  /* prepend new entry */
  if (front) {
    if (strarray_prepend(&entry->val, value)) {
      goto set_entry_error;
    }
  }
  else {
    if (strarray_append(&entry->val, value)) {
      goto set_entry_error;
    }
  }

  return entry;
set_entry_error:
  if (new_entry) {
    _free_entry(entry);
  }
  if (new_section) {
    _free_namedsection(named);
  }
  return NULL;
}

/**
 * Finds a specific entry inside a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed section
 * @param entry_name name of the entry
 * @return pointer to configuration entry, NULL if not found
 */
struct cfg_entry *
cfg_db_find_entry(struct cfg_db *db,
    const char *section_type, const char *section_name, const char *entry_name) {
  struct cfg_named_section *named;
  struct cfg_entry *entry = NULL;

  /* get named section */
  named = cfg_db_find_namedsection(db, section_type, section_name);
  if (named != NULL) {
    entry = avl_find_element(&named->entries, entry_name, entry, node);
  }
  return entry;
}

/**
 * Removes an entry from a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @return -1 if the entry did not exist, 0 if it was removed
 */
int
cfg_db_remove_entry(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name) {
  struct cfg_entry *entry;

  /* get entry */
  entry = cfg_db_find_entry(db, section_type, section_name, entry_name);
  if (entry == NULL) {
    /* entry not there */
    return -1;
  }

  _free_entry(entry);
  return 0;
}

/**
 * Accessor function to read the string value of a single entry
 * from a configuration database.
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed section
 * @param entry_name name of the entry
 * @return string value, NULL if not found or list of values
 */
const struct const_strarray *
cfg_db_get_entry_value(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name) {
  struct cfg_schema_entry_key key;
  struct cfg_schema_entry *s_entry;
  struct cfg_entry *entry;

  entry = cfg_db_find_entry(db, section_type, section_name, entry_name);
  if (entry != NULL) {
    return (const struct const_strarray *)&entry->val;
  }

  /* look for value of default section */
  if (section_name != NULL) {
    entry = cfg_db_find_entry(db, section_type, NULL, entry_name);
    if (entry != NULL) {
      return (const struct const_strarray *)&entry->val;
    }
  }

  /* look for schema default value */
  if (db->schema == NULL) {
    return NULL;
  }

  key.type = section_type;
  key.entry = entry_name;

  s_entry = avl_find_element(&db->schema->entries, &key, s_entry, _node);
  if (s_entry != NULL && s_entry->def.value != NULL) {
    return &s_entry->def;
  }
  return NULL;
}

/**
 * Removes an element from a configuration entry list
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @param value value to be removed from the list
 * @return 0 if the element was removed, -1 if it wasn't there
 */
int
cfg_db_remove_element(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value) {
  struct cfg_entry *entry;
  char *ptr;

  /* find entry */
  entry = cfg_db_find_entry(db, section_type, section_name, entry_name);
  if (entry == NULL) {
    return -1;
  }

  if (!cfg_db_is_multipart_entry(entry)) {
    /* only a single element in list */
    if (strcmp(value, entry->val.value) == 0) {
      _free_entry(entry);
      return 0;
    }
    return -1;
  }

  strarray_for_each_element(&entry->val, ptr) {
    if (strcmp(ptr, value) == 0) {
      strarray_remove(&entry->val, ptr);
      return 0;
    }
  }

  /* element not in list */
  return -1;
}

/**
 * Creates a section type in a configuration database
 * @param db pointer to configuration database
 * @param type type of section
 * @return pointer to section type, NULL if allocation failed
 */
static struct cfg_section_type *
_alloc_section(struct cfg_db *db, const char *type) {
  struct cfg_section_type *section;

  assert(type);

  section = calloc(1, sizeof(*section));
  if (!section) {
    return NULL;
  }

  section->type = strdup(type);
  if (!section->type) {
    free (section);
    return NULL;
  }

  section->node.key = section->type;
  avl_insert(&db->sectiontypes, &section->node);

  section->db = db;

  avl_init(&section->names, cfg_avlcmp_keys, false);
  return section;
}

/**
 * Removes a section type from a configuration database
 * including its named section and entries.
 * @param section pointer to section
 */
static void
_free_sectiontype(struct cfg_section_type *section) {
  struct cfg_named_section *named, *named_it;

  /* remove all named sections */
  CFG_FOR_ALL_SECTION_NAMES(section, named, named_it) {
    _free_namedsection(named);
  }

  avl_remove(&section->db->sectiontypes, &section->node);
  free((void *)section->type);
  free(section);
}

/**
 * Creates a named section in a configuration database.
 * @param section pointer to section type
 * @param name name of section (might be NULL)
 * @return pointer to named section, NULL if allocation failed
 */
static struct cfg_named_section *
_alloc_namedsection(struct cfg_section_type *section,
    const char *name) {
  struct cfg_named_section *named;

  named = calloc(1, sizeof(*section));
  if (!named) {
    return NULL;
  }

  named->name = (name == NULL) ? NULL : strdup(name);
  if (!named->name && name != NULL) {
    free (named);
    return NULL;
  }

  named->node.key = named->name;
  avl_insert(&section->names, &named->node);

  named->section_type = section;
  avl_init(&named->entries, cfg_avlcmp_keys, false);
  return named;
}

/**
 * Removes a named section from a database including entries.
 * @param named pointer to named section.
 */
static void
_free_namedsection(struct cfg_named_section *named) {
  struct cfg_entry *entry, *entry_it;

  /* remove all entries first */
  CFG_FOR_ALL_ENTRIES(named, entry, entry_it) {
    _free_entry(entry);
  }

  avl_remove(&named->section_type->names, &named->node);
  free ((void *)named->name);
  free (named);
}

/**
 * Creates an entry in a configuration database.
 * It will not initialize the value.
 * @param named pointer to named section
 * @param name name of entry
 * @return pointer to configuration entry, NULL if allocation failed
 */
static struct cfg_entry *
_alloc_entry(struct cfg_named_section *named,
    const char *name) {
  struct cfg_entry *entry;

  assert(name);

  entry = calloc (1, sizeof(*entry));
  if (!entry) {
    return NULL;
  }

  entry->name = strdup(name);
  if (!entry->name) {
    free (entry);
    return NULL;
  }

  entry->node.key = entry->name;
  avl_insert(&named->entries, &entry->node);

  entry->named_section = named;
  return entry;
}

/**
 * Removes a configuration entry from a database
 * @param entry pointer to configuration entry
 */
static void
_free_entry(struct cfg_entry *entry) {
  avl_remove(&entry->named_section->entries, &entry->node);

  strarray_free(&entry->val);
  free(entry->name);
  free(entry);
}
