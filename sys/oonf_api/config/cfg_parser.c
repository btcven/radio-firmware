
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

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/avl_comp.h"

#include "config/cfg.h"
#include "config/cfg_db.h"
#include "config/cfg_io.h"
#include "config/cfg_parser.h"

static struct cfg_parser *_find_parser(struct cfg_instance *,
    const char *name);

/**
 * Adds a parser to the registry. Parser name field must already
 * be initialized.
 * @param instance pointer to cfg_instance
 * @param parser pointer to parser description
 */
void
cfg_parser_add(struct cfg_instance *instance, struct cfg_parser *parser) {
  assert(parser->name);

  parser->node.key = parser->name;
  avl_insert(&instance->parser_tree, &parser->node);

  if (parser->def || instance->parser_tree.count == 1) {
    instance->default_parser = parser;
  }
}

/**
 * Removes a parser from the registry.
 * @param instance pointer to cfg_instance
 * @param parser pointer to initialized parser description
 */
void
cfg_parser_remove(struct cfg_instance *instance, struct cfg_parser *parser) {
  struct cfg_parser *ph, *iter;
  if (parser->node.key == NULL) {
    return;
  }

  avl_remove(&instance->parser_tree, &parser->node);
  parser->node.key = NULL;
  if (instance->default_parser == parser) {
    instance->default_parser =
        avl_first_element_safe(&instance->parser_tree, parser, node);
    CFG_FOR_ALL_PARSER(instance, ph, iter) {
      if (ph->def) {
        instance->default_parser = ph;
        break;
      }
    }
  }
}

/**
 * Looks for a parser that can understand a certain buffer content.
 * A path name and a mimetype can be used to give the parser additional
 * hints about the buffers content.
 * @param instance pointer to cfg_instance
 * @param abuf pointer to buffer to be parsed
 * @param path path where the buffers content was read from (might be NULL)
 * @param mimetype mimetype of buffer (might be NULL)
 * @return name of parser that can read the buffer, NULL if none fitting
 *   parser was found
 */
const char *
cfg_parser_find(struct cfg_instance *instance,
    struct autobuf *abuf, const char *path, const char *mimetype) {
  struct cfg_parser *parser, *iterator;

  CFG_FOR_ALL_PARSER(instance, parser, iterator) {
    if (parser->check_hints != NULL) {
      if (parser->check_hints(abuf, path, mimetype)) {
        return parser->name;
      }
    }
  }
  return NULL;
}

/**
 * Parse the content of a buffer into a configuration database
 * @param instance pointer to cfg_instance
 * @param parser parser name
 * @param src pointer to input buffer
 * @param len length of input buffer
 * @param log autobuffer for logging output
 * @return pointer to configuration database, NULL if an error happened
 */
struct cfg_db *
cfg_parser_parse_buffer(struct cfg_instance *instance,
    const char *parser, void *src, size_t len, struct autobuf *log) {
  struct cfg_parser *f;

  f = _find_parser(instance, parser);
  if (f == NULL) {
    cfg_append_printable_line(log, "Cannot find parser '%s'", parser);
    return NULL;
  }

  if (f->parse == NULL) {
    cfg_append_printable_line(log, "Configuration parser '%s'"
        " does not support parsing", parser);
    return NULL;
  }

  return f->parse(src, len, log);
}

/**
 * Serialize a configuration database into a buffer
 * @param instance pointer to cfg_instance
 * @param parser parser name
 * @param dst autobuffer to write into
 * @param src configuration database to serialize
 * @param log autobuffer for logging output
 * @return 0 if database was serialized, -1 if an error happened
 */
int
cfg_parser_serialize_to_buffer(struct cfg_instance *instance,
    const char *parser, struct autobuf *dst,
    struct cfg_db *src, struct autobuf *log) {
  struct cfg_parser *f;

  f = _find_parser(instance, parser);
  if (f == NULL) {
    cfg_append_printable_line(log, "Cannot find parser '%s'", parser);
    return -1;
  }

  if (f->serialize == NULL) {
    cfg_append_printable_line(log, "Configuration parser '%s' does not"
        " support db storage into buffer", parser);
    return -1;
  }

  return f->serialize(dst, src, log);
}

/**
 * Lookup a parser in the registry
 * @param instance pointer to cfg_instance
 * @param name name of parser, NULL for default parser
 * @return pointer to parser, NULL if not found
 */
static struct cfg_parser *
_find_parser(struct cfg_instance *instance, const char *name) {
  struct cfg_parser *parser;

  if (name == NULL) {
    return instance->default_parser;
  }

  return avl_find_element(&instance->parser_tree, name, parser, node);
}
