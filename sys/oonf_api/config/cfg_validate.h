
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

#ifndef CFG_VALIDATE_H_
#define CFG_VALIDATE_H_

#include "common/autobuf.h"
#include "common/common_types.h"

EXPORT int cfg_validate_printable(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    size_t len);
EXPORT int cfg_validate_strlen(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    size_t len);
EXPORT int cfg_validate_choice(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    const char **choices, size_t choice_count);
EXPORT int cfg_validate_int(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    int64_t min, int64_t max, uint16_t bytelen, uint16_t fraction, bool base2);
EXPORT int cfg_validate_netaddr(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    bool prefix, const int8_t *af_types, size_t af_types_count);
EXPORT int cfg_validate_acl(struct autobuf *out,
    const char *section_name, const char *entry_name, const char *value,
    bool prefix, const int8_t *af_types, size_t af_types_count);

#endif /* CFG_VALIDATE_H_ */
