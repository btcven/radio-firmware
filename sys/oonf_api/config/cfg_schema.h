
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

#ifndef CFG_SCHEMA_H_
#define CFG_SCHEMA_H_

struct cfg_schema;
struct cfg_schema_section;
struct cfg_schema_entry;

#ifndef _WIN32
#include <arpa/inet.h>
#ifndef RIOT_VERSION
#include <netinet/if_ether.h>
#include <netinet/ip.h>
#endif
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "common/autobuf.h"
#include "common/avl.h"
#include "common/netaddr.h"
#include "common/string.h"

#include "config/cfg_db.h"

/* macros for creating schema entries */
#if !defined(REMOVE_HELPTEXT)
#define _CFG_VALIDATE(p_name, p_def, p_help,args...)                         { .key.entry = (p_name), .def = { .value = (p_def), .length = sizeof(p_def)}, .help = (p_help), ##args }
#else
#define _CFG_VALIDATE(p_name, p_def, p_help,args...)                         { .key.entry = (p_name), .def = { .value = (p_def), .length = sizeof(p_def)}, ##args }
#endif

/*
 * Example of a section schema definition.
 *
 * All CFG_VALIDATE_xxx macros follow a similar pattern.
 * - the first parameter is the name of the key in the configuration file
 * - the second parameter is the default value (as a string!)
 * - the third parameter is the help text
 *
 * static struct cfg_schema_section section =
 * {
 *     .type = "testsection",
 *     .mode = CFG_SSMODE_NAMED
 * };
 *
 * static struct cfg_schema_entry entries[] = {
 *     CFG_VALIDATE_PRINTABLE("text", "defaulttext", "help for text parameter"),
 *     CFG_VALIDATE_INT32_MINMAX("number", "0", "help for number parameter", 0, 10),
 * };
 */
#define _CFG_VALIDATE_INT(p_name, p_def, p_help, size, fraction, base2, min, max, args...)   _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_int, .cb_valhelp = cfg_schema_help_int, .validate_param = {{.i64 = (min)}, {.i64 = (max)}, {.i16 = {size, fraction, !!(base2) ? 2 : 10}}}, ##args )

#define CFG_VALIDATE_STRING(p_name, p_def, p_help, args...)                                  _CFG_VALIDATE(p_name, p_def, p_help, ##args)
#define CFG_VALIDATE_STRING_LEN(p_name, p_def, p_help, maxlen, args...)                      _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_strlen, .cb_valhelp = cfg_schema_help_strlen, .validate_param = {{.s = (maxlen) }}, ##args )
#define CFG_VALIDATE_PRINTABLE(p_name, p_def, p_help, args...)                               _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_printable, .cb_valhelp = cfg_schema_help_printable, .validate_param = {{.s = INT32_MAX }}, ##args )
#define CFG_VALIDATE_PRINTABLE_LEN(p_name, p_def, p_help, maxlen, args...)                   _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_printable, .cb_valhelp = cfg_schema_help_printable, .validate_param = {{.s = (maxlen) }}, ##args )
#define CFG_VALIDATE_CHOICE(p_name, p_def, p_help, p_list, args...)                          _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_choice, .cb_valhelp = cfg_schema_help_choice, .validate_param = {{.ptr = (p_list)}, { .s = ARRAYSIZE(p_list)}}, ##args )
#define CFG_VALIDATE_INT32(p_name, p_def, p_help, fraction, base2, args...)                  _CFG_VALIDATE_INT(p_name, p_def, p_help, 4, fraction, base2, INT32_MIN, INT32_MAX, ##args)
#define CFG_VALIDATE_INT64(p_name, p_def, p_help, fraction, base2, args...)                  _CFG_VALIDATE_INT(p_name, p_def, p_help, 8, fraction, base2, INT64_MIN, INT64_MAX, ##args)
#define CFG_VALIDATE_INT32_MINMAX(p_name, p_def, p_help, fraction, base2, min, max, args...) _CFG_VALIDATE_INT(p_name, p_def, p_help, 4, fraction, base2, min, max, ##args)
#define CFG_VALIDATE_INT64_MINMAX(p_name, p_def, p_help, fraction, base2, min, max, args...) _CFG_VALIDATE_INT(p_name, p_def, p_help, 8, fraction, base2, min, max, ##args)
#define CFG_VALIDATE_NETADDR(p_name, p_def, p_help, prefix, unspec, args...)                 _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_MAC48, AF_EUI64, AF_INET, AF_INET6, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_HWADDR(p_name, p_def, p_help, prefix, unspec, args...)          _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_MAC48, AF_EUI64, -1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_MAC48(p_name, p_def, p_help, prefix, unspec, args...)           _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_MAC48, -1,-1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_EUI64(p_name, p_def, p_help, prefix, unspec, args...)           _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_EUI64, -1,-1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_V4(p_name, p_def, p_help, prefix, unspec, args...)              _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_INET, -1,-1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_V6(p_name, p_def, p_help, prefix, unspec, args...)              _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_INET6, -1,-1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_NETADDR_V46(p_name, p_def, p_help, prefix, unspec, args...)             _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_netaddr, .cb_valhelp = cfg_schema_help_netaddr, .validate_param = {{.i8 = {AF_INET, AF_INET6, -1,-1, !!(unspec) ? AF_UNSPEC : -1}}, {.b = !!(prefix)}}, ##args )
#define CFG_VALIDATE_ACL(p_name, p_def, p_help, args...)                                     _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_MAC48, AF_EUI64, AF_INET, AF_INET6, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_HWADDR(p_name, p_def, p_help, args...)                              _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_MAC48, AF_EUI64, -1, -1, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_MAC48(p_name, p_def, p_help, args...)                               _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_MAC48, -1, -1, -1, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_EUI64(p_name, p_def, p_help, args...)                               _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_EUI64, -1, -1, -1, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_V4(p_name, p_def, p_help, args...)                                  _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_INET, -1, -1, -1, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_V6(p_name, p_def, p_help, args...)                                  _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_INET6, -1, -1, -1, -1}}, {.b = true}}, ##args )
#define CFG_VALIDATE_ACL_V46(p_name, p_def, p_help, args...)                                 _CFG_VALIDATE(p_name, p_def, p_help, .cb_validate = cfg_schema_validate_acl, .cb_valhelp = cfg_schema_help_acl, .list = true, .validate_param = {{.i8 = {AF_INET, AF_INET6, -1, -1, -1}}, {.b = true}}, ##args )

#define CFG_VALIDATE_BOOL(p_name, p_def, p_help, args...)                                    CFG_VALIDATE_CHOICE(p_name, p_def, p_help, CFGLIST_BOOL, ##args)

/*
 * Example of a section schema definition with binary mapping
 *
 * All CFG_MAP_xxx macros follow a similar pattern.
 * - the first parameter is the name of the struct the data will be mapped into
 * - the second parameter is the name of the field the data will be mapped into
 * - the third parameter is the name of the key in the configuration file
 * - the fourth parameter is the default value (as a string!)
 * - the fifth parameter is the help text
 *
 * struct bin_data {
 *   char *string;
 *   int int_value;
 * };
 *
 * static struct cfg_schema_section section =
 * {
 *     .type = "testsection",
 *     .mode = CFG_SSMODE_NAMED
 * };
 *
 * static struct cfg_schema_entry entries[] = {
 *     CFG_MAP_PRINTABLE(bin_data, string, "text", "defaulttext", "help for text parameter"),
 *     CFG_MAP_INT_MINMAX(bin_data, int_value, "number", "0", "help for number parameter", 0, 10),
 * };
 */
#define _CFG_MAP_INT(p_reference, p_field, p_name, p_def, p_help, size, fraction, base2, min, max, args...)   _CFG_VALIDATE_INT(p_name, p_def, p_help, size, fraction, base2, min, max, .cb_to_binary = cfg_schema_tobin_int, .bin_offset = offsetof(struct p_reference, p_field), ##args )

#define CFG_MAP_STRING(p_reference, p_field, p_name, p_def, p_help, args...)                                  _CFG_VALIDATE(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_strptr, .bin_offset = offsetof(struct p_reference, p_field), ##args )
#define CFG_MAP_STRING_LEN(p_reference, p_field, p_name, p_def, p_help, maxlen, args...)                      CFG_VALIDATE_STRING_LEN(p_name, p_def, p_help, maxlen, .cb_to_binary = cfg_schema_tobin_strptr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_STRING_ARRAY(p_reference, p_field, p_name, p_def, p_help, maxlen, args...)                    CFG_VALIDATE_STRING_LEN(p_name, p_def, p_help, maxlen, .cb_to_binary = cfg_schema_tobin_strarray, .bin_offset = offsetof(struct p_reference, p_field), ##args )
#define CFG_MAP_PRINTABLE(p_reference, p_field, p_name, p_def, p_help, args...)                               CFG_VALIDATE_PRINTABLE(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_strptr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_PRINTABLE_LEN(p_reference, p_field, p_name, p_def, p_help, args...)                           CFG_VALIDATE_PRINTABLE_LEN(p_name, p_def, p_help, maxlen, .cb_to_binary = cfg_schema_tobin_strptr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_PRINTABLE_ARRAY(p_reference, p_field, p_name, p_def, p_help, maxlen, args...)                 CFG_VALIDATE_PRINTABLE_LEN(p_name, p_def, p_help, maxlen, .cb_to_binary = cfg_schema_tobin_strarray, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_CHOICE(p_reference, p_field, p_name, p_def, p_help, p_list, args...)                          CFG_VALIDATE_CHOICE(p_name, p_def, p_help, p_list, .cb_to_binary = cfg_schema_tobin_choice, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_INT32(p_reference, p_field, p_name, p_def, p_help, fraction, base2, args...)                  _CFG_MAP_INT(p_reference, p_field, p_name, p_def, p_help, 4, fraction, base2, INT32_MIN, INT32_MAX, ##args)
#define CFG_MAP_INT64(p_reference, p_field, p_name, p_def, p_help, fraction, base2, args...)                  _CFG_MAP_INT(p_reference, p_field, p_name, p_def, p_help, 8, fraction, base2, INT64_MIN, INT64_MAX, ##args)
#define CFG_MAP_INT32_MINMAX(p_reference, p_field, p_name, p_def, p_help, fraction, base2, min, max, args...) _CFG_MAP_INT(p_reference, p_field, p_name, p_def, p_help, 4, fraction, base2, min, max, ##args)
#define CFG_MAP_INT64_MINMAX(p_reference, p_field, p_name, p_def, p_help, fraction, base2, min, max, args...) _CFG_MAP_INT(p_reference, p_field, p_name, p_def, p_help, 8, fraction, base2, min, max, ##args)
#define CFG_MAP_NETADDR(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)                 CFG_VALIDATE_NETADDR(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_HWADDR(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)          CFG_VALIDATE_NETADDR_HWADDR(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_MAC48(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)           CFG_VALIDATE_NETADDR_MAC48(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_EUI64(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)           CFG_VALIDATE_NETADDR_EUI64(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_V4(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)              CFG_VALIDATE_NETADDR_V4(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_V6(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)              CFG_VALIDATE_NETADDR_V6(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_NETADDR_V46(p_reference, p_field, p_name, p_def, p_help, prefix, unspec, args...)             CFG_VALIDATE_NETADDR_V46(p_name, p_def, p_help, prefix, unspec, .cb_to_binary = cfg_schema_tobin_netaddr, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL(p_reference, p_field, p_name, p_def, p_help, args...)                                     CFG_VALIDATE_ACL(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_HWADDR(p_reference, p_field, p_name, p_def, p_help, args...)                              CFG_VALIDATE_ACL_HWADDR(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_MAC48(p_reference, p_field, p_name, p_def, p_help, args...)                               CFG_VALIDATE_ACL_MAC48(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_EUI64(p_reference, p_field, p_name, p_def, p_help, args...)                               CFG_VALIDATE_ACL_EUI64(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_V4(p_reference, p_field, p_name, p_def, p_help, args...)                                  CFG_VALIDATE_ACL_V4(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_V6(p_reference, p_field, p_name, p_def, p_help, args...)                                  CFG_VALIDATE_ACL_V6(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_ACL_V46(p_reference, p_field, p_name, p_def, p_help, args...)                                 CFG_VALIDATE_ACL_V46(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_acl, .bin_offset = offsetof(struct p_reference, p_field), ##args)

#define CFG_MAP_BOOL(p_reference, p_field, p_name, p_def, p_help, args...)                                    CFG_VALIDATE_BOOL(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_bool, .bin_offset = offsetof(struct p_reference, p_field), ##args)
#define CFG_MAP_STRINGLIST(p_reference, p_field, p_name, p_def, p_help, args...)                              _CFG_VALIDATE(p_name, p_def, p_help, .cb_to_binary = cfg_schema_tobin_stringlist, .bin_offset = offsetof(struct p_reference, p_field), .list = true, ##args )


struct cfg_schema {
  /* tree of sections of this schema */
  struct avl_tree sections;

  /* tree of schema entries of this schema */
  struct avl_tree entries;

  /* tree of delta handlers of this schema */
  struct avl_tree handlers;
};

enum cfg_schema_section_mode {
  /*
   * normal unnamed section, delta handlers will be triggered at
   * startup even it does not exist in the configuration file
   *
   * default setting
   */
  CFG_SSMODE_UNNAMED = 0,

  /*
   * unnamed section, delta handler will only trigger if one value
   * is set to a non-default value
   */
  CFG_SSMODE_UNNAMED_OPTIONAL_STARTUP_TRIGGER,

  /*
   * named section, delta handlers will always trigger for this
   */
  CFG_SSMODE_NAMED,

  /*
   * named section, configuration demands at least one existing
   * section of this type to be valid.
   */
  CFG_SSMODE_NAMED_MANDATORY,

  /*
   * named section, if none exists the configuration will create
   * a temporary (and empty) section with the defined default name.
   */
  CFG_SSMODE_NAMED_WITH_DEFAULT,

  /* always last */
  CFG_SSMODE_MAX,
};

/*
 * Represents the schema of all named sections within
 * a certain type
 */
struct cfg_schema_section {

  /* node for global section tree, initialized by schema_add() */
  struct avl_node _section_node;

  /* name of section type, key for section_node */
  const char *type;

  /* name of default section if mode is CFG_SSMODE_NAMED_WITH_DEFAULT */
  const char *def_name;

  /* mode of this section, see above */
  enum cfg_schema_section_mode mode;

  /* help text for section */
  const char *help;

  /* callback for checking configuration of section */
  int (*cb_validate)(const char *section_name,
      struct cfg_named_section *, struct autobuf *);

  /* node for global delta handler tree, initialized by delta_add() */
  struct avl_node _delta_node;

  /* priority for delta handling, key for delta_node */
  uint32_t delta_priority;

  /* callback for delta handling, NULL if not interested */
  void (*cb_delta_handler)(void);

  /*
   * pointer to former and later version of changed section, only valid
   * during call of cb_delta_handler
   */
  struct cfg_named_section *pre, *post;

  /*
   * Name of configured section (or NULL if unnamed section), only valid
   * during call of cb_delta_handler
   */
  const char *section_name;

  /* list of entries in section */
  struct cfg_schema_entry *entries;
  size_t entry_count;

  /* pointer to next section for subsystem initialization */
  struct cfg_schema_section *next_section;
};

struct cfg_schema_entry_key {
  const char *type, *entry;
};

/* Represents the schema of a configuration entry */
struct cfg_schema_entry {
  /* node for global section tree */
  struct avl_node _node;
  struct cfg_schema_section *_parent;

  /* name of entry */
  struct cfg_schema_entry_key key;

  /* default value */
  struct const_strarray def;

  /* help text for entry */
  const char *help;

  /* value is a list of parameters instead of a single one */
  bool list;

  /* callback for checking value and giving help for an entry */
  int (*cb_validate)(const struct cfg_schema_entry *entry,
      const char *section_name, const char *value, struct autobuf *out);

  void (*cb_valhelp)(const struct cfg_schema_entry *entry, struct autobuf *out);

  /* parameters for validator functions */
  union {
    int8_t i8[8];
    uint8_t u8[8];
    int16_t i16[4];
    uint16_t u16[4];
    int32_t i32[2];
    uint32_t u32[2];
    int64_t i64;
    uint64_t u64;
    bool b;
    size_t s;
    void *ptr;
  } validate_param[3];

  /* callback for converting string into binary */
  int (*cb_to_binary)(const struct cfg_schema_entry *s_entry,
      const struct const_strarray *value, void *ptr);

  /* offset of current binary data compared to reference pointer */
  size_t bin_offset;

  /* return values for delta comparision */
  const struct const_strarray *pre, *post;
  bool delta_changed;
};

#define CFGLIST_BOOL_VALUES "true", "1", "on", "yes", "false", "0", "off", "no"
#define CFGLIST_BOOL_TRUE_VALUES "true", "1", "on", "yes"

EXPORT extern const char *CFGLIST_BOOL_TRUE[4];
EXPORT extern const char *CFGLIST_BOOL[8];
EXPORT extern const char *CFG_SCHEMA_SECTIONMODE[CFG_SSMODE_MAX];

EXPORT void cfg_schema_add(struct cfg_schema *schema);

EXPORT void cfg_schema_add_section(struct cfg_schema *schema, struct cfg_schema_section *section);
EXPORT void cfg_schema_remove_section(struct cfg_schema *schema, struct cfg_schema_section *section);

EXPORT int cfg_schema_validate(struct cfg_db *db,
    bool cleanup, bool ignore_unknown_sections, struct autobuf *out);

EXPORT int cfg_schema_tobin(void *target, struct cfg_named_section *named,
    const struct cfg_schema_entry *entries, size_t count);
EXPORT const struct const_strarray *cfg_schema_tovalue(struct cfg_named_section *named,
    const struct cfg_schema_entry *entry);

EXPORT int cfg_schema_handle_db_changes(struct cfg_db *pre_change, struct cfg_db *post_change);
EXPORT int cfg_schema_handle_db_startup_changes(struct cfg_db *db);

EXPORT int cfg_avlcmp_schemaentries(const void *p1, const void *p2);

EXPORT int cfg_schema_validate_printable(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);
EXPORT int cfg_schema_validate_strlen(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);
EXPORT int cfg_schema_validate_choice(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);
EXPORT int cfg_schema_validate_int(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);
EXPORT int cfg_schema_validate_netaddr(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);
EXPORT int cfg_schema_validate_acl(const struct cfg_schema_entry *entry,
    const char *section_name, const char *value, struct autobuf *out);

EXPORT void cfg_schema_help_printable(
    const struct cfg_schema_entry *entry, struct autobuf *out);
EXPORT void cfg_schema_help_strlen(
    const struct cfg_schema_entry *entry, struct autobuf *out);
EXPORT void cfg_schema_help_choice(
    const struct cfg_schema_entry *entry, struct autobuf *out);
EXPORT void cfg_schema_help_int(
    const struct cfg_schema_entry *entry, struct autobuf *out);
EXPORT void cfg_schema_help_netaddr(
    const struct cfg_schema_entry *entry, struct autobuf *out);
EXPORT void cfg_schema_help_acl(
    const struct cfg_schema_entry *entry, struct autobuf *out);

EXPORT int cfg_schema_tobin_strptr(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_strarray(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_choice(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_int(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_netaddr(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_bool(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_stringlist(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);
EXPORT int cfg_schema_tobin_acl(const struct cfg_schema_entry *s_entry,
    const struct const_strarray *value, void *reference);


/**
 * Finds a section in a schema
 * @param schema pointer to schema
 * @param type type of section
 * @return pointer to section, NULL if not found
 */
static INLINE struct cfg_schema_section *
cfg_schema_find_section(struct cfg_schema *schema, const char *type) {
  struct cfg_schema_section *section;

  return avl_find_element(&schema->sections, type, section, _section_node);
}

/**
 * Finds an entry in a schema section
 * @param section pointer to section
 * @param name name of entry
 * @return pointer of entry, NULL if not found
 */
static INLINE struct cfg_schema_entry *
cfg_schema_find_section_entry(struct cfg_schema_section *section, const char *name) {
  size_t i;

  for (i=0; i<section->entry_count; i++) {
    if (strcmp(section->entries[i].key.entry, name) == 0) {
      return &section->entries[i];
    }
  }
  return NULL;
}

#endif /* CFG_SCHEMA_H_ */
