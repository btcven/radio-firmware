
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

#ifndef CFG_PARSER_H_
#define CFG_PARSER_H_

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/common_types.h"

#include "config/cfg.h"
#include "config/cfg_db.h"

/* Represents a config parser */
struct cfg_parser {
  /* node for global tree in cfg_parser.c */
  struct avl_node node;

  /* name of parser */
  const char *name;

  /* true if this is the default parser */
  bool def;

  /* callback for checking if the parser supports a certain input */
  bool (*check_hints)(struct autobuf *abuf, const char *path, const char *mimetype);

  /* callback for parsing a buffer into a configuration database */
  struct cfg_db *(*parse)(char *src, size_t len, struct autobuf *log);

  /* callback for serializing a database into a buffer */
  int (*serialize)(struct autobuf *dst, struct cfg_db *src, struct autobuf *log);
};

#define CFG_FOR_ALL_PARSER(instance, parser, iterator) avl_for_each_element_safe(&instance->parser_tree, parser, node, iterator)

EXPORT void cfg_parser_add(struct cfg_instance *, struct cfg_parser *);
EXPORT void cfg_parser_remove(struct cfg_instance *, struct cfg_parser *);

EXPORT const char *cfg_parser_find(struct cfg_instance *,
    struct autobuf *abuf, const char *path, const char *mimetype);

EXPORT struct cfg_db *cfg_parser_parse_buffer(struct cfg_instance *,
    const char *parser, void *src, size_t len, struct autobuf *log);
EXPORT int cfg_parser_serialize_to_buffer(struct cfg_instance *, const char *parser,
    struct autobuf *dst, struct cfg_db *src, struct autobuf *log);

#endif /* CFG_PARSER_H_ */
