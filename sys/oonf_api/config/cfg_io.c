
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

#ifndef WIN32
#include <alloca.h>
#else
#include <malloc.h>
#endif
#include <assert.h>
#include <string.h>

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/avl_comp.h"
#include "common/common_types.h"
#include "common/string.h"

#include "config/cfg.h"
#include "config/cfg_io.h"

static struct cfg_io *_find_io(struct cfg_instance *instance,
    const char *url, const char **io_param, struct autobuf *log);

/**
 * Add a new io-handler to the registry. Name of io handler
 * must be already initialized.
 * @param instance pointer to cfg_instance
 * @param io pointer to io handler object
 */
void
cfg_io_add(struct cfg_instance *instance, struct cfg_io *io) {
  assert (io->name);
  io->node.key = io->name;
  avl_insert(&instance->io_tree, &io->node);

  if (io->def || instance->io_tree.count == 1) {
    instance->default_io = io;
  }
}

/**
 * Unregister an io-handler.
 * @param instance pointer to cfg_instance
 * @param io pointer to io handler
 */
void
cfg_io_remove(struct cfg_instance *instance, struct cfg_io *io) {
  struct cfg_io *ioh, *iter;
  if (io->node.key) {
    avl_remove(&instance->io_tree, &io->node);
    io->node.key = NULL;
  }

  if (instance->default_io == io) {
    /* get new default io handler */
    instance->default_io = avl_first_element_safe(&instance->io_tree, io, node);
    CFG_FOR_ALL_IO(instance, ioh, iter) {
      if (ioh->def) {
        instance->default_io = ioh;
        break;
      }
    }
  }
}

/**
 * Load a configuration database from an external source
 * @param instance pointer to cfg_instance
 * @param url URL specifying the external source
 *   might contain io-handler specification with {iohandler}://
 *   syntax.
 * @param parser name of parser to be used by io-handler (if necessary),
 *   NULL if parser autodetection should be used
 * @param log pointer to autobuffer to contain logging output
 *   by loader.
 * @return pointer to configuration database, NULL if an error happened
 */
struct cfg_db *
cfg_io_load_parser(struct cfg_instance *instance,
    const char *url, const char *parser, struct autobuf *log) {
  struct cfg_io *io;
  const char *io_param = NULL;

  io = _find_io(instance, url, &io_param, log);
  if (io == NULL) {
    cfg_append_printable_line(log, "Error, unknown config io '%s'.", url);
    return NULL;
  }

  if (io->load == NULL) {
    cfg_append_printable_line(log, "Error, config io '%s' does not support loading.", io->name);
    return NULL;
  }
  return io->load(instance, io_param, parser, log);
}

/**
 * Store a configuration database into an external destination.
 * @param instance pointer to cfg_instance
 * @param url URL specifying the external source
 *   might contain io-handler specification with {iohandler}://
 *   syntax.
 * @param parser name of parser to be used by io-handler (if necessary),
 *   NULL if parser autodetection should be used
 * @param src configuration database to be stored
 * @param log pointer to autobuffer to contain logging output
 *   by storage.
 * @return 0 if data was stored, -1 if an error happened
 */
int
cfg_io_save_parser(struct cfg_instance *instance,
    const char *url, const char *parser, struct cfg_db *src, struct autobuf *log) {
  struct cfg_io *io;
  const char *io_param = NULL;

  io = _find_io(instance, url, &io_param, log);
  if (io == NULL) {
    cfg_append_printable_line(log, "Error, unknown config io '%s'.", io->name);
    return -1;
  }

  if (io->save == NULL) {
    cfg_append_printable_line(log, "Error, config io '%s' does not support saving.", io->name);
    return -1;
  }
  return io->save(instance, io_param, parser, src, log);
}

/**
 * Decode the URL string for load/storage
 * @param instance pointer to cfg_instance
 * @param url url string
 * @param io_param pointer to a charpointer, will be used as a second
 *   return parameter for URL postfix
 * @param log pointer to autobuffer to contain logging output
 *   by storage.
 * @return pointer to io handler, NULL if none found or an error
 *   happened
 */
static struct cfg_io *
_find_io(struct cfg_instance *instance,
    const char *url, const char **io_param, struct autobuf *log) {
  struct cfg_io *io;
  const char *ptr1;

  ptr1 = strstr(url, "://");
  if (ptr1 == url) {
    cfg_append_printable_line(log, "Illegal URL '%s' as parameter for io selection", url);
    return NULL;
  }
  if (ptr1 == NULL) {
    /* get default io handler */
    io = instance->default_io;
    ptr1 = url;
  }
  else {
    char *buffer;

    buffer = strdup(url);
    if (!buffer)
      return NULL;

    buffer[ptr1 - url] = 0;

    io = avl_find_element(&instance->io_tree, buffer, io, node);
    free (buffer);
    ptr1 += 3;
  }

  if (io == NULL) {
    cfg_append_printable_line(log, "Cannot find loader for parameter '%s'", url);
    return NULL;
  }

  *io_param = ptr1;
  return io;
}
