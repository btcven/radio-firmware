
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
#include <strings.h>

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/common_types.h"
#include "config/cfg_io.h"
#include "config/cfg_parser.h"
#include "config/cfg.h"

/**
 * Initialize a configuration instance
 * @param instance pointer to cfg_instance
 */
void
cfg_add(struct cfg_instance *instance) {
  memset(instance, 0, sizeof(*instance));

  avl_init(&instance->io_tree, avl_comp_strcasecmp, false);
  avl_init(&instance->parser_tree, avl_comp_strcasecmp, false);
}

/**
 * Cleanup a configuration instance
 * @param instance pointer to cfg_instance
 */
void
cfg_remove(struct cfg_instance *instance) {
  struct cfg_io *io, *iit;
  struct cfg_parser *parser, *pit;

  CFG_FOR_ALL_IO(instance, io, iit) {
    cfg_io_remove(instance, io);
  }

  CFG_FOR_ALL_PARSER(instance, parser, pit) {
    cfg_parser_remove(instance, parser);
  }

  cfg_cmd_clear_state(instance);
}

/**
 * Appends a single line to an autobuffer.
 * The function replaces all non-printable characters with '.'
 * and will append a newline at the end
 * @param autobuf pointer to autobuf object
 * @param fmt printf format string
 * @return -1 if an out-of-memory error happened, 0 otherwise
 */
int
cfg_append_printable_line(struct autobuf *autobuf, const char *fmt, ...) {
  unsigned char *_value;
  size_t len;
  int rv;
  va_list ap;

  if (autobuf == NULL) return 0;

  _value = (unsigned char *)abuf_getptr(autobuf) + abuf_getlen(autobuf);
  len = abuf_getlen(autobuf);

  va_start(ap, fmt);
  rv = abuf_vappendf(autobuf, fmt, ap);
  va_end(ap);

  if (rv < 0) {
    return rv;
  }

  /* convert everything non-printable to '.' */
  while (*_value && len++ < abuf_getlen(autobuf)) {
    if (*_value < 32 || *_value == 127 || *_value == 255) {
      *_value = '.';
    }
    _value++;
  }
  abuf_append_uint8(autobuf, '\n');
  return 0;
}

/**
 * Tests on the pattern [a-zA-Z_][a-zA-Z0-9_]*
 * @param key section_type/name or entry name
 * @param section_name true if testing a section name, false if testing
 *   a section type or a key
 * @return true if input string is valid for this parser,
 *   false otherwise
 */
bool
cfg_is_allowed_key(const char *key, bool section_name) {
  static const char *valid = "_0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  /* test for [a-zA-Z_][a-zA-Z0-9_]* */
  if (!section_name && *key >= '0' && *key <= '9') {
    return false;
  }

  return key[strspn(key, valid)] == 0;
}

/**
 * Null-pointer safe avl compare function for keys implementation.
 * NULL is considered a string smaller than all normal strings.
 * @param p1 pointer to key 1
 * @param p2 pointer to key 2
 * @return similar to strcmp()
 */
int
cfg_avlcmp_keys(const void *p1, const void *p2) {
  const char *str1 = p1;
  const char *str2 = p2;

  if (str1 == NULL) {
    return str2 == NULL ? 0 : -1;
  }
  if (str2 == NULL) {
    return 1;
  }

  return strcasecmp(str1, str2);
}

/**
 * Looks up the index of a string within a string array
 * @param key pointer to string to be looked up in the array
 * @param array pointer to string pointer array
 * @param array_size number of strings in array
 * @return index of the string inside the array, -1 if not found
 */
int
cfg_get_choice_index(const char *key, const char **array, size_t array_size) {
  size_t i;

  for (i=0; i<array_size; i++) {
    if (strcasecmp(key, array[i]) == 0) {
      return (int) i;
    }
  }
  return -1;
}
