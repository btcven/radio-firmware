
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

#ifndef CFG_IO_H_
#define CFG_IO_H_

/* forward declaration */
struct cfg_io;

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/common_types.h"

#include "config/cfg.h"

/* Represents a single IO-Handler */
struct cfg_io {
  /* node for global tree in cfg_io.c */
  struct avl_node node;

  /* name of io handler */
  const char *name;

  /* true if this is the default handler */
  bool def;

  /* callback to load a configuration */
  struct cfg_db *(*load)(struct cfg_instance *,
      const char *param, const char *parser, struct autobuf *log);

  /* callback to save a configuration */
  int (*save)(struct cfg_instance *,
      const char *param, const char *parser, struct cfg_db *src, struct autobuf *log);
};

#define CFG_FOR_ALL_IO(instance, io, iterator) avl_for_each_element_safe(&(instance)->io_tree, io, node, iterator)

EXPORT void cfg_io_add(struct cfg_instance *, struct cfg_io *);
EXPORT void cfg_io_remove(struct cfg_instance *, struct cfg_io *);

EXPORT struct cfg_db *cfg_io_load_parser(struct cfg_instance *,
    const char *url, const char *parser, struct autobuf *log);
EXPORT int cfg_io_save_parser(struct cfg_instance *,
    const char *url, const char *parser, struct cfg_db *src, struct autobuf *log);

/**
 * Load a configuration database from an external source.
 * This call will always use autodetection for choosing the parser.
 * @param instance pointer to cfg_instance
 * @param url URL specifying the external source
 *   might contain io-handler specification with <iohandler>://
 *   syntax.
 * @param log pointer to autobuffer to contain logging output
 *   by loader.
 * @return pointer to configuration database, NULL if an error happened
 */
static INLINE struct cfg_db *
cfg_io_load(struct cfg_instance *instance, const char *url, struct autobuf *log) {
  return cfg_io_load_parser(instance, url, NULL, log);
}

/**
 * Store a configuration database into an external destination.
 * This call will always use autodetection for choosing the parser.
 * @param instance pointer to cfg_instance
 * @param url URL specifying the external source
 *   might contain io-handler specification with <iohandler>://
 *   syntax.
 * @param src configuration database to be stored
 * @param log pointer to autobuffer to contain logging output
 *   by storage.
 * @return 0 if data was stored, -1 if an error happened
 */
static INLINE int
cfg_io_save(struct cfg_instance *instance,
    const char *url, struct cfg_db *src, struct autobuf *log) {
  return cfg_io_save_parser(instance, url, NULL, src, log);
}

#endif /* CFG_IO_H_ */
