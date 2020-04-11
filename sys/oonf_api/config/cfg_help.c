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

#include "config/cfg_help.h"
#include "config/cfg.h"

#define _PREFIX "    "

void
cfg_help_strlen(struct autobuf *out, size_t len) {
  cfg_append_printable_line(out, _PREFIX "Parameter must have a maximum"
      " length of %"PRINTF_SIZE_T_SPECIFIER" characters", len);
}

void
cfg_help_printable(struct autobuf *out, size_t len) {
  cfg_help_printable(out, len);
  cfg_append_printable_line(out, _PREFIX "Parameter must only contain printable characters.");
}

void
cfg_help_choice(struct autobuf *out, bool preamble,
    const char **choices, size_t choice_count) {
  size_t i;

  if (preamble) {
    cfg_append_printable_line(out, _PREFIX "Parameter must be on of the following list:");
  }

  abuf_puts(out, "    ");
  for (i=0; i < choice_count; i++) {
    abuf_appendf(out, "%s'%s'",
        i==0 ? "" : ", ", choices[i]);
  }
  abuf_puts(out, "\n");
}

void
cfg_help_int(struct autobuf *out,
    int64_t min, int64_t max, uint16_t bytelen, uint16_t fraction, bool base2) {
  struct isonumber_str hbuf1, hbuf2;
  int64_t min64, max64;

  min64 = INT64_MIN >> (8 * (8 - bytelen));
  max64 = INT64_MAX >> (8 * (8 - bytelen));

  /* get string representation of min/max */
  str_to_isonumber_s64(&hbuf1, min, "", fraction, base2, true);
  str_to_isonumber_s64(&hbuf2, max, "", fraction, base2, true);

  if (min > min64) {
    if (max < max64) {
      cfg_append_printable_line(out, _PREFIX "Parameter must be a %d-byte fractional integer"
          " between %s and %s with a maximum of %d digits",
          bytelen, hbuf1.buf, hbuf2.buf, fraction);
    }
    else {
      cfg_append_printable_line(out, _PREFIX "Parameter must be a %d-byte fractional integer"
          " larger or equal than %s with a maximum of %d digits",
          bytelen, hbuf1.buf, fraction);
    }
  }
  else {
    if (max < max64) {
      cfg_append_printable_line(out, _PREFIX "Parameter must be a %d-byte fractional integer"
          " smaller or equal than %s with a maximum of %d digits",
          bytelen, hbuf2.buf, fraction);
    }
    else {
      cfg_append_printable_line(out, _PREFIX "Parameter must be a %d-byte signed integer"
          " with a maximum of %d digits",
          bytelen, fraction);
    }
  }
}

void
cfg_help_netaddr(struct autobuf *out, bool preamble,
    bool prefix, const int8_t *af_types, size_t af_types_count) {
  int8_t type;
  bool first;
  size_t i;

  if (preamble) {
    abuf_puts(out, _PREFIX "Parameter must be an address of the following type: ");
  }

  first = true;
  for (i=0; i<af_types_count; i++) {
    type = af_types[i];

    if (type == -1) {
      continue;
    }

    if (first) {
      first = false;
    }
    else {
      abuf_puts(out, ", ");
    }

    switch (type) {
      case AF_INET:
        abuf_puts(out, "IPv4");
        break;
      case AF_INET6:
        abuf_puts(out, "IPv6");
        break;
      case AF_MAC48:
        abuf_puts(out, "MAC48");
        break;
      case AF_EUI64:
        abuf_puts(out, "EUI64");
        break;
      default:
        abuf_puts(out, "Unspec (-)");
        break;
    }
  }

  if (prefix) {
    abuf_puts(out, "\n" _PREFIX "    (the address can have an optional prefix)");
  }
  abuf_puts(out, "\n");
}

void
cfg_help_acl(struct autobuf *out, bool preamble,
    bool prefix, const int8_t *af_types, size_t af_types_count) {
  if (preamble) {
    abuf_puts(out, _PREFIX "Parameter is an apache2 style access control list made from a list of network addresses of the following types:\n");
  }

  cfg_help_netaddr(out, false, prefix, af_types, af_types_count);

  abuf_puts(out,
      _PREFIX "    Each of the addresses/prefixes can start with a"
              " '+' to add them to the whitelist and '-' to add it to the blacklist"
              " (default is the whitelist).\n"
      _PREFIX "    In addition to this there are four keywords to configure the ACL:\n"
      _PREFIX "    - '" ACL_FIRST_ACCEPT "' to parse the whitelist first\n"
      _PREFIX "    - '" ACL_FIRST_REJECT "' to parse the blacklist first\n"
      _PREFIX "    - '" ACL_DEFAULT_ACCEPT "' to accept input if it doesn't match either list\n"
      _PREFIX "    - '" ACL_DEFAULT_REJECT "' to not accept it if it doesn't match either list\n"
      _PREFIX "    (default mode is '" ACL_FIRST_ACCEPT "' and '" ACL_DEFAULT_REJECT "')\n");
}
