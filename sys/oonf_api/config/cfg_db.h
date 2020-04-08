
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

#ifndef CFG_DB_H_
#define CFG_DB_H_

/* forward declaration */
struct cfg_db;
struct cfg_section_type;
struct cfg_named_section;
struct cfg_entry;

#include "common/avl.h"
#include "common/common_types.h"
#include "common/string.h"

#include "config/cfg_schema.h"

/* Represents a single database with configuration entries */
struct cfg_db {
  /* tree of all sections of this db */
  struct avl_tree sectiontypes;

  /* linked schema of db */
  struct cfg_schema *schema;
};

/* Represents a section type in a configuration database */
struct cfg_section_type {
  /* node for tree in database */
  struct avl_node node;

  /* name of type */
  const char *type;

  /* backpointer to database */
  struct cfg_db *db;

  /* tree of named sections */
  struct avl_tree names;
};

/* Represents a named section in a configuration database */
struct cfg_named_section {
  /* node for tree in section type */
  struct avl_node node;

  /* name of named section */
  const char *name;

  /* backpointer to section type */
  struct cfg_section_type *section_type;

  /* tree of entries */
  struct avl_tree entries;
};

/* Represents a configuration entry */
struct cfg_entry {
  /* node for tree in named section */
  struct avl_node node;

  /* name of entry */
  char *name;

  /* value of entry, might contain multiple strings */
  struct strarray val;

  /* backpointer to named section */
  struct cfg_named_section *named_section;
};

#define CFG_FOR_ALL_SECTION_TYPES(db, s_type, iterator) avl_for_each_element_safe(&db->sectiontypes, s_type, node, iterator)
#define CFG_FOR_ALL_SECTION_NAMES(s_type, s_name, iterator) avl_for_each_element_safe(&s_type->names, s_name, node, iterator)
#define CFG_FOR_ALL_ENTRIES(s_name, entry, iterator) avl_for_each_element_safe(&s_name->entries, entry, node, iterator)

EXPORT struct cfg_db *cfg_db_add(void);
EXPORT void cfg_db_remove(struct cfg_db *);
EXPORT int _cfg_db_append(struct cfg_db *dst, struct cfg_db *src,
    const char *section_type, const char *section_name, const char *entry_name);

EXPORT struct cfg_named_section *_cfg_db_add_section(
    struct cfg_db *, const char *section_type, const char *section_name,
    bool *new_section);

EXPORT int cfg_db_remove_sectiontype(struct cfg_db *, const char *section_type);

EXPORT struct cfg_named_section *cfg_db_find_namedsection(
    struct cfg_db *, const char *section_type, const char *section_name);
EXPORT int cfg_db_remove_namedsection(struct cfg_db *db, const char *section_type,
    const char *section_name);

EXPORT struct cfg_entry *cfg_db_set_entry_ext(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value,
    bool append, bool front);

EXPORT struct cfg_entry *cfg_db_find_entry(struct cfg_db *db,
    const char *section_type, const char *section_name, const char *entry_name);
EXPORT int cfg_db_remove_entry(struct cfg_db *, const char *section_type,
    const char *section_name, const char *entry_name);
EXPORT const struct const_strarray *cfg_db_get_entry_value(struct cfg_db *db,
    const char *section_type, const char *section_name, const char *entry_name);

EXPORT int cfg_db_remove_element(struct cfg_db *, const char *section_type,
    const char *section_name, const char *entry_name, const char *value);

/**
 * Link a configuration schema to a database
 * @param db pointer to database
 * @param schema pointer to schema
 */
static INLINE void
cfg_db_link_schema(struct cfg_db *db, struct cfg_schema *schema) {
  db->schema = schema;
}

/**
 * Creates a copy of a configuration database
 * @param src original database
 * @return pointer to the copied database, NULL if out of memory
 */
static INLINE struct cfg_db *
cfg_db_duplicate(struct cfg_db *src) {
  struct cfg_db *dst;

  dst = cfg_db_add();
  if (dst) {
    if (_cfg_db_append(dst, src, NULL, NULL, NULL)) {
      cfg_db_remove(dst);
      return NULL;
    }
  }
  return dst;
}

/**
 * Copy all settings from one configuration database to
 * a second one.
 * @param dst destination database which will hold the values of
 *   both databases after the copy
 * @param src source of the append process
 * @return 0 if copy was successful, -1 if an error happened.
 *   In case of an error, the destination might contain a partial
 *   copy.
 */
static INLINE int
cfg_db_copy(struct cfg_db *dst, struct cfg_db *src) {
  return _cfg_db_append(dst, src, NULL, NULL, NULL);
}

/**
 * Copy a section_type from one configuration database to
 * a second one.
 * @param dst destination database which will hold the values of
 *   both databases after the copy
 * @param src source of the append process
 * @param section_type type of section to be copied
 * @return 0 if copy was successful, -1 if an error happened.
 *   In case of an error, the destination might contain a partial
 *   copy.
 */
static INLINE int
cfg_db_copy_sectiontype(struct cfg_db *dst, struct cfg_db *src,
    const char *section_type) {
  return _cfg_db_append(dst, src, section_type, NULL, NULL);
}

/**
 * Copy a named section from one configuration database to
 * a second one.
 * @param dst destination database which will hold the values of
 *   both databases after the copy
 * @param src source of the append process
 * @param section_type type of section to be copied
 * @param section_name name of section to be copied
 * @return 0 if copy was successful, -1 if an error happened.
 *   In case of an error, the destination might contain a partial
 *   copy.
 */
static INLINE int
cfg_db_copy_namedsection(struct cfg_db *dst, struct cfg_db *src,
    const char *section_type, const char *section_name) {
  return _cfg_db_append(dst, src, section_type, section_name, NULL);
}

/**
 * Copy a named section from one configuration database to
 * a second one.
 * @param dst destination database which will hold the values of
 *   both databases after the copy
 * @param src source of the append process
 * @param section_type type of section to be copied
 * @param section_name name of section to be copied
 * @return 0 if copy was successful, -1 if an error happened.
 *   In case of an error, the destination might contain a partial
 *   copy.
 */
static INLINE int
cfg_db_copy_entry(struct cfg_db *dst, struct cfg_db *src,
    const char *section_type, const char *section_name, const char *entry_name) {
  return _cfg_db_append(dst, src, section_type, section_name, entry_name);
}

/**
 * Finds a section object inside a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @return pointer to section type , NULL if not found
 */
static INLINE struct cfg_section_type *
cfg_db_get_sectiontype(struct cfg_db *db, const char *section_type) {
  struct cfg_section_type *section;
  return avl_find_element(&db->sectiontypes, section_type, section, node);
}

/**
 * Finds a (named) section inside a section type
 * @param type pointer to section type
 * @param name name of section
 * @return pointer to section, NULL if not found
 */
static INLINE struct cfg_named_section *
cfg_db_get_named_section(struct cfg_section_type *type, const char *name) {
  struct cfg_named_section *named;
  return avl_find_element(&type->names, name, named, node);
}

/**
 * Finds an entry object inside a (named) section.
 * @param named pointer to section
 * @param key name of entry
 * @return pointer to entry, NULL if not found
 */
static INLINE struct cfg_entry *
cfg_db_get_entry(struct cfg_named_section *named, const char *key) {
  struct cfg_entry *entry;
  return avl_find_element(&named->entries, key, entry, node);
}

/**
 * Alias for cfg_db_get_sectiontype
 * @param db pointer to configuration database
 * @param section_type type of section
 * @return pointer to section type , NULL if not found
 */
static INLINE struct cfg_section_type *
cfg_db_find_sectiontype(struct cfg_db *db, const char *section_type) {
  return cfg_db_get_sectiontype(db, section_type);
}

/**
 * Finds an unnamed section inside a section type
 * @param type pointer to section type
 * @return pointer to section, NULL if not found
 */
static INLINE struct cfg_named_section *
cfg_db_find_unnamedsection(struct cfg_db *db, const char *section_type) {
  return cfg_db_find_namedsection(db, section_type, NULL);
}

/**
 * @param named pointer to named section
 * @return true if named sections has a name, false if its an 'unnamed' one.
 */
static INLINE bool
cfg_db_is_named_section(struct cfg_named_section *named) {
  return named->name != NULL;
}

/**
 * @param pointer to section type
 * @return pointer pointer to 'unnamed' named_section element,
 *   NULL no unnamed section.
 */
static INLINE struct cfg_named_section *
cfg_db_get_unnamed_section(struct cfg_section_type *stype) {
  struct cfg_named_section *named;
  return avl_find_element(&stype->names, NULL, named, node);
}

/**
 * Adds a named section to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section
 * @return pointer to named section, NULL if an error happened
 */
static INLINE struct cfg_named_section *
cfg_db_add_namedsection(
    struct cfg_db *db, const char *section_type, const char *section_name) {
  bool dummy;
  return _cfg_db_add_section(db, section_type, section_name, &dummy);
}

/**
 * Adds an unnamed section to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @return pointer to named section, NULL if an error happened
 */
static INLINE struct cfg_named_section *
cfg_db_add_unnamedsection(
    struct cfg_db *db, const char *section_type) {
  bool dummy;
  return _cfg_db_add_section(db, section_type, NULL, &dummy);
}

/**
 * Sets an entry to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @param value entry value
 * @param append true if the value should be put in front of a list,
 *   false if it should overwrite all old values
 * @return pointer to cfg_entry, NULL if an error happened
 */
static INLINE struct cfg_entry *
cfg_db_set_entry(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value,
    bool append) {
  return cfg_db_set_entry_ext(db, section_type, section_name, entry_name,
      value, append, true);
}

/**
 * Adds an entry to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @param value entry value
 */
static INLINE int
cfg_db_overwrite_entry(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value) {
  if (cfg_db_set_entry(db, section_type, section_name, entry_name, value, false)) {
    return 0;
  }
  return -1;
}

/**
 * Appends an entry to a configuration database
 * @param db pointer to configuration database
 * @param section_type type of section
 * @param section_name name of section, NULL if an unnamed one
 * @param entry_name entry name
 * @param value entry value
 */
static INLINE int
cfg_db_add_entry(struct cfg_db *db, const char *section_type,
    const char *section_name, const char *entry_name, const char *value) {
  if (cfg_db_set_entry(db, section_type, section_name, entry_name, value, true)) {
    return 0;
  }
  return -1;
}

/**
 * @param entry pointer to configuration entry
 * @return true if entry has multiple values, false otherwise
 */
static INLINE bool
cfg_db_is_multipart_entry(struct cfg_entry *entry) {
  return strarray_get(&entry->val, 1) != NULL;
}


/**
 * Counts the number of list items of a configuration entry
 * @param entry pointer to cfg entry
 * @return number of items in the entries value
 */
static INLINE size_t
cfg_db_entry_get_listsize(struct cfg_entry *entry) {
  return strarray_get_count(&entry->val);
}

#endif /* CFG_DB_H_ */
