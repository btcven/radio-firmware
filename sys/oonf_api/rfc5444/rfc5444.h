
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
#ifndef RFC5444_CONVERSION_H_
#define RFC5444_CONVERSION_H_

#include "common/common_types.h"

enum {
  /* timetlv_max = 14 * 2^28 * 1000 / 1024 = 14000 << 18 = 3 670 016 000 ms */
  RFC5444_TIMETLV_MAX = 0xdac00000,

  /* timetlv_min = 1000/1024 ms */
  RFC5444_TIMETLV_MIN = 0x00000001,

  /* metric_max = 1<<24 - 256 */
  RFC5444_METRIC_MAX = 0xffff00,

  /* metric_min = 1 */
  RFC5444_METRIC_MIN = 0x000001,

  /* larger than possible metric value */
  RFC5444_METRIC_INFINITE = 0xffffff,

  /* infinite path cost */
  RFC5444_METRIC_INFINITE_PATH = 0xffffffff,
};

EXPORT uint8_t rfc5444_timetlv_get_from_vector(
    uint8_t *vector, size_t vector_length, uint8_t hopcount);
EXPORT uint8_t rfc5444_timetlv_encode(uint64_t);
EXPORT uint64_t rfc5444_timetlv_decode(uint8_t);

EXPORT uint16_t rfc5444_metric_encode(uint32_t);
EXPORT uint32_t rfc5444_metric_decode(uint16_t);

EXPORT int rfc5444_seqno_difference(uint16_t, uint16_t);

static INLINE int
rfc5444_seqno_is_larger(uint16_t s1, uint16_t s2) {
  /*
   * The sequence number S1 is said to be "greater than" the sequence
   * number S2 if:
   * o  S1 > S2 AND S1 - S2 < MAXVALUE/2 OR
   * o  S2 > S1 AND S2 - S1 > MAXVALUE/2
   */
  return (s1 > s2 && (s1-s2) < (1<<15))
      || (s2 > s1 && (s2-s1) > (1<<15));
}

static INLINE int
rfc5444_seqno_is_smaller(uint16_t s1, uint16_t s2) {
  return s1 != s2 && !rfc5444_seqno_is_larger(s1, s2);
}

#endif /* RFC5444_CONVERSION_H_ */
