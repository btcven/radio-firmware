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

#include "common/autobuf.h"
#include "common/common_types.h"
#include "common/netaddr.h"
#include "common/netaddr_acl.h"

#include "config/cfg_validate.h"
#include "config/cfg.h"

/**
 * Validate if a value against a specific printable string
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param len maximum length of printable string
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_printable(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value, size_t len) {
  if (cfg_validate_strlen(out, section_name, entry_name, value, len)) {
    return -1;
  }
  if (!str_is_printable(value)) {
    /* not a printable ascii character */
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s has non-printable characters",
        value, entry_name, section_name);
    return -1;
  }
  return 0;
}

/**
 * Validate if a value against a specific string
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param len maximum length of string
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_strlen(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value, size_t len) {
  if (strlen(value) > len) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s is longer than %"PRINTF_SIZE_T_SPECIFIER" characters",
        value, entry_name, section_name, len);
    return -1;
  }
  return 0;
}

/**
 * Validate if a value against a specific list of strings
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param choices pointer to list of strings
 * @param choices_count number of strings in list
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_choice(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value, const char **choices,
    size_t choices_count) {
  int i;
  i = cfg_get_choice_index(value, choices, choices_count);
  if (i >= 0) {
    return 0;
  }

  cfg_append_printable_line(out, "Unknown value '%s'"
      " for entry '%s' in section %s",
      value, entry_name, section_name);
  return -1;
}

/**
 * Validate if a value against a specific integer
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param min minimum value of number including fractional digits
 * @param max maximum value of number including fractional digits
 * @param bytelen number of bytes available for target number
 * @param fraction number of fractional digits of target number
 * @param base2 true if number shall use binary prefixes instead of
 *    iso prefixes (1024 instead of 1000)
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_int(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value, int64_t min, int64_t max,
    uint16_t bytelen, uint16_t fraction, bool base2) {
  int64_t i, min64, max64;
  struct isonumber_str hbuf;

  if (str_from_isonumber_s64(&i, value, fraction, base2)) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s is not a fractional %d-byte integer"
        " with a maximum of %d fractional digits",
        value, entry_name, section_name,
        bytelen, fraction);
    return -1;
  }

  min64 = INT64_MIN >> (8 * (8 - bytelen));
  max64 = INT64_MAX >> (8 * (8 - bytelen));

  if (i < min64 || i > max64) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s' in section %s is "
        "too %s for a %d-hyte integer with %d fractional digits",
        value, entry_name, section_name,
        i < min ? "small" : "large", bytelen, fraction);
  }

  if (i < min) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s' in section %s is "
        "smaller than %s",
        value, entry_name, section_name,
        str_to_isonumber_s64(&hbuf, min, "", fraction, base2, true));
    return -1;
  }
  if (i > max) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s' in section %s is "
        "larger than %s",
        value, entry_name, section_name,
        str_to_isonumber_s64(&hbuf, min, "", fraction, base2, true));
    return -1;
  }
  return 0;
}

/**
 * Validate if a value against a specific network address
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param prefix true if the address might be a network prefix
 * @param af_types list of address families the address might be
 * @param af_types_count number of address families specified
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_netaddr(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value,
    bool prefix, const int8_t *af_types, size_t af_types_count) {
  struct netaddr addr;
  uint8_t max_prefix;
  size_t i;

  if (netaddr_from_string(&addr, value)) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s is no valid network address",
        value, entry_name, section_name);
    return -1;
  }

  max_prefix = netaddr_get_maxprefix(&addr);

  /* check prefix length */
  if (netaddr_get_prefix_length(&addr) > max_prefix) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s has an illegal prefix length",
        value, entry_name, section_name);
    return -1;
  }
  if (!prefix && netaddr_get_prefix_length(&addr) != max_prefix) {
    cfg_append_printable_line(out, "Value '%s' for entry '%s'"
        " in section %s must be a single address, not a prefix",
        value, entry_name, section_name);
    return -1;
  }

  for (i=0; i<af_types_count; i++) {
    if (af_types[i] == netaddr_get_address_family(&addr)) {
      return 0;
    }
  }

  /* at least one condition was set, but no one matched */
  cfg_append_printable_line(out, "Value '%s' for entry '%s'"
      " in section '%s' is wrong address type",
      value, entry_name, section_name);
  return -1;
}

/**
 * Validate if a value against a specific network access control list
 * @param out output buffer for error messages
 * @param section_name name of configuration section
 * @param entry_name name of configuration entry
 * @param value value that needs to be validated
 * @param prefix true if the address might be a network prefix
 * @param af_types list of address families the address might be
 * @param af_types_count number of address families specified
 * @return 0 if value is valid, -1 otherwise
 */
int
cfg_validate_acl(struct autobuf *out, const char *section_name,
    const char *entry_name, const char *value,
    bool prefix, const int8_t *af_types, size_t af_types_count) {
  struct netaddr_acl dummy;

  if (netaddr_acl_handle_keywords(&dummy, value) == 0) {
    return 0;
  }

  if (*value == '+' || *value == '-') {
    return cfg_validate_netaddr(out, section_name, entry_name, &value[1],
        prefix, af_types, af_types_count);
  }

  return cfg_validate_netaddr(out, section_name, entry_name, value,
      prefix, af_types, af_types_count);
}
