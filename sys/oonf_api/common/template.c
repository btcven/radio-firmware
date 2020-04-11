
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

#include <stdio.h>
#include <stdlib.h>

#include "common/common_types.h"
#include "common/autobuf.h"
#include "common/string.h"
#include "common/template.h"

static struct abuf_template_data *_find_template(
    struct abuf_template_data *data, size_t tmplLength, const char *txt, size_t txtLength);
static int _json_printvalue(struct autobuf *out, const char *txt, bool string);

/**
 * Initialize an index table for a template engine.
 * Each usage of a key in the format has to be %key%.
 * The existing keys (start, end, key-number) will be recorded
 * in the integer array the user provided, so the template
 * engine can replace them with the values later.
 *
 * @param data array of key/value pairs for the template engine
 * @param data_count number of keys
 * @param format format string of the template
 * @return allocated template storage object, NULL if an error happened
 */
struct abuf_template_storage *
abuf_template_init (
    struct abuf_template_data *data, size_t data_count, const char *format) {
  struct abuf_template_storage *storage = NULL, *new_storage = NULL;
  struct abuf_template_data *d;
  bool no_open_format = true;
  bool escape = false;
  size_t start = 0;
  size_t pos = 0;

  storage = calloc(1, sizeof(struct abuf_template_storage));
  if (storage == NULL) {
    return NULL;
  }

  while (format[pos]) {
    if (!escape && format[pos] == '%') {
      if (no_open_format) {
        start = pos++;
        no_open_format = false;
        continue;
      }
      if (pos - start > 1) {
        d = _find_template(data, data_count, &format[start+1], pos-start-1);
        if (d) {
          new_storage = realloc(storage, sizeof(struct abuf_template_storage)
              + sizeof(storage->indices[0]) * (storage->count+1));
          if (new_storage == NULL) {
            free(storage);
            return NULL;
          }
          storage = new_storage;

          storage->indices[storage->count].start = start;
          storage->indices[storage->count].end = pos+1;
          storage->indices[storage->count].data = d;

          storage->count++;
        }
      }
      no_open_format = true;
    }
    else if (format[pos] == '\\') {
      /* handle "\\" and "\%" in text */
      escape = !escape;
    }
    else {
      escape = false;
    }

    pos++;
  }

  return storage;
}

/**
 * Append the result of a template engine into an autobuffer.
 * Each usage of a key will be replaced with the corresponding
 * value.
 * @param out pointer to autobuf object
 * @param format format string (as supplied to abuf_template_init()
 * @param storage pointer to template storage object, which will be filled by
 *   this function
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
abuf_add_template(struct autobuf *out, const char *format,
    struct abuf_template_storage *storage) {
  struct abuf_template_storage_entry *entry;
  size_t i, last = 0;

  if (out == NULL) return 0;

  for (i=0; i<storage->count; i++) {
    entry = &storage->indices[i];

    /* copy prefix text */
    if (last < entry->start) {
      if (abuf_memcpy(out, &format[last], entry->start - last) < 0) {
        return -1;
      }
    }

    if (entry->data->value) {
      if (abuf_puts(out, entry->data->value) < 0) {
        return -1;
      }
    }
    last = entry->end;
  }

  if (last < strlen(format)) {
    if (abuf_puts(out, &format[last]) < 0) {
      return -1;
    }
  }
  return 0;
}

/**
 * Converts a key/value list for the template engine into
 * JSON compatible output.
 * @param out output buffer
 * @param prefix string prefix for all lines
 * @param data array of template data
 * @param data_count number of template data entries
 * @return -1 if an error happened, 0 otherwise
 */
int
abuf_add_json(struct autobuf *out, const char *prefix,
    struct abuf_template_data *data, size_t data_count) {
  bool first;
  size_t i;

  if (abuf_appendf(out, "%s{\n", prefix) < 0) {
    return -1;
  }

  first = true;
  for (i=0; i<data_count; i++) {
    if (data[i].value == NULL) {
      continue;
    }

    if (!first) {
      if (abuf_puts(out, ",\n") < 0) {
        return -1;
      }
    }
    else {
      first = false;
    }

    if (abuf_appendf(out, "%s    \"%s\" : ",
        prefix, data[i].key) < 0) {
      return -1;
    }
    if (_json_printvalue(out, data[i].value, data[i].string)) {
      return -1;
    }
  }

  if (!first) {
    if (abuf_puts(out, "\n") < 0) {
      return -1;
    }
  }

  if (abuf_appendf(out, "%s}\n", prefix) < 0) {
    return -1;
  }
  return 0;
}

/**
 * Find the template data corresponding to a key
 * @param keys pointer to template data array
 * @param tmplLength number of template data entries in array
 * @param txt pointer to text to search in
 * @param txtLength length of text to search in
 * @return pointer to corresponding template data, NULL if not found
 */
static struct abuf_template_data *
_find_template(struct abuf_template_data *data, size_t tmplLength, const char *txt, size_t txtLength) {
  size_t i;

  for (i=0; i<tmplLength; i++) {
    const char *key;

    key = data[i].key;
    if (strncmp(key, txt, txtLength) == 0 && key[txtLength] == 0) {
      return &data[i];
    }
  }
  return NULL;
}

/**
 * Prints a string to an autobuffer, using JSON escape rules
 * @param out pointer to output buffer
 * @param txt string to print
 * @param delimiter true if string must be enclosed in quotation marks
 * @return -1 if an error happened, 0 otherwise
 */
static int
_json_printvalue(struct autobuf *out, const char *txt, bool delimiter) {
  const char *ptr;
  bool unprintable;

  if (delimiter) {
    if (abuf_puts(out, "\"") < 0) {
      return -1;
    }
  }
  else if (*txt == 0) {
    abuf_puts(out, "0");
  }

  ptr = txt;
  while (*ptr) {
    unprintable = !str_char_is_printable(*ptr);
    if (unprintable || *ptr == '\\' || *ptr == '\"') {
      if (ptr != txt) {
        if (abuf_memcpy(out, txt, ptr - txt) < 0) {
          return -1;
        }
      }

      if (unprintable) {
        if (abuf_appendf(out, "\\u00%02x", (unsigned char)(*ptr++)) < 0) {
          return -1;
        }
      }
      else {
        if (abuf_appendf(out, "\\%c", *ptr++) < 0) {
          return -1;
        }
      }
      txt = ptr;
    }
    else {
      ptr++;
    }
  }

  if (abuf_puts(out, txt) < 0) {
    return -1;
  }
  if (delimiter) {
    if (abuf_puts(out, "\"") < 0) {
      return -1;
    }
  }

  return 0;
}

