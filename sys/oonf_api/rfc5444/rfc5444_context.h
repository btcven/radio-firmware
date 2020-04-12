
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
#ifndef RFC5444_CONTEXT_H_
#define RFC5444_CONTEXT_H_

#include "rfc5444/rfc5444_api_config.h"

/*
 * Return values for reader callbacks and API calls (and internal functions)
 * The RFC5444_DROP_... constants are ordered, higher values mean
 * dropping more of the context.
 * All return values less than zero represent an error.
 *
 * RFC5444_DROP_TLV means to drop the current tlv
 * RFC5444_DROP_ADDRESS means to drop the current address
 * RFC5444_DROP_MESSAGE means to drop the current message
 * RFC5444_DROP_MESSAGE_BUT_FORWARD means to drop the current message
 *   but forward allow to forward it
 * RFC5444_DROP_PACKET means to drop the whole packet
 *
 * RFC5444_OKAY means everything is okay
 *
 * RFC5444_UNSUPPORTED_VERSION
 *   version field of rfc5444 is not 0
 * RFC5444_END_OF_BUFFER
 *   end of rfc5444 data stream before end of message/tlv
 * RFC5444_BAD_TLV_IDXFLAGS
 *   illegal combination of thassingleindex and thasmultiindex flags
 * RFC5444_BAD_TLV_VALUEFLAGS
 *   illegal combination of thasvalue and thasextlen flag
 * RFC5444_BAD_TLV_LENGTH
 *   length of tlv is no multiple of number of values
 * RFC5444_OUT_OF_MEMORY
 *   dynamic memory allocation failed
 * RFC5444_EMPTY_ADDRBLOCK
 *   address block with 0 addresses found
 * RFC5444_BAD_MSG_TAILFLAGS
 *   illegal combination of ahasfulltail and ahaszerotail flag
 * RFC5444_BAD_MSG_PREFIXFLAGS
 *   illegal combination of ahassingleprelen and ahasmultiprelen flag
 * RFC5444_DUPLICATE_TLV
 *   address tlv already exists
 * RFC5444_OUT_OF_ADDRTLV_MEM
 *   internal buffer for address tlv values too small
 * RFC5444_MTU_TOO_SMALL
 *   non-fragmentable part of message does not fit into max sizes packet
 * RFC5444_NO_MSGCREATOR
 *   cannot create a message without a message creator
 * RFC5444_FW_MESSAGE_TOO_LONG
 *   bad format of forwarded message, does not fit into max sized packet
 * RFC5444_FW_BAD_SIZE
 *   bad format of forwarded message, size field wrong
 */
enum rfc5444_result {
#if DISALLOW_CONSUMER_CONTEXT_DROP == false
  RFC5444_RESULT_MAX           =  5,
  RFC5444_DROP_PACKET          =  5,
  RFC5444_DROP_MESSAGE         =  4,
  RFC5444_DROP_MSG_BUT_FORWARD =  3,
  RFC5444_DROP_ADDRESS         =  2,
  RFC5444_DROP_TLV             =  1,
#endif
  RFC5444_OKAY                 =  0,
  RFC5444_UNSUPPORTED_VERSION  = -1,
  RFC5444_END_OF_BUFFER        = -2,
  RFC5444_BAD_TLV_IDXFLAGS     = -3,
  RFC5444_BAD_TLV_VALUEFLAGS   = -4,
  RFC5444_BAD_TLV_LENGTH       = -5,
  RFC5444_OUT_OF_MEMORY        = -6,
  RFC5444_EMPTY_ADDRBLOCK      = -7,
  RFC5444_BAD_MSG_TAILFLAGS    = -8,
  RFC5444_BAD_MSG_PREFIXFLAGS  = -9,
  RFC5444_DUPLICATE_TLV        = -10,
  RFC5444_OUT_OF_ADDRTLV_MEM   = -11,
  RFC5444_MTU_TOO_SMALL        = -12,
  RFC5444_NO_MSGCREATOR        = -13,
  RFC5444_FW_MESSAGE_TOO_LONG  = -14,
  RFC5444_FW_BAD_SIZE          = -15,
  RFC5444_RESULT_MIN           = -15,
};

const char *rfc5444_strerror(enum rfc5444_result result);

/* maximum address length */
enum { RFC5444_MAX_ADDRLEN = 16 };

/* packet flags */
enum {
  RFC5444_PKT_FLAGMASK         = 0x0f,
  RFC5444_PKT_FLAG_SEQNO       = 0x08,
  RFC5444_PKT_FLAG_TLV         = 0x04,

/* message flags */
  RFC5444_MSG_FLAG_ORIGINATOR  = 0x80,
  RFC5444_MSG_FLAG_HOPLIMIT    = 0x40,
  RFC5444_MSG_FLAG_HOPCOUNT    = 0x20,
  RFC5444_MSG_FLAG_SEQNO       = 0x10,

  RFC5444_MSG_FLAG_ADDRLENMASK = 0x0f,

/* addressblock flags */
  RFC5444_ADDR_FLAG_HEAD       = 0x80,
  RFC5444_ADDR_FLAG_FULLTAIL   = 0x40,
  RFC5444_ADDR_FLAG_ZEROTAIL   = 0x20,
  RFC5444_ADDR_FLAG_SINGLEPLEN = 0x10,
  RFC5444_ADDR_FLAG_MULTIPLEN  = 0x08,

/* tlv flags */
  RFC5444_TLV_FLAG_TYPEEXT     = 0x80,
  RFC5444_TLV_FLAG_SINGLE_IDX  = 0x40,
  RFC5444_TLV_FLAG_MULTI_IDX   = 0x20,
  RFC5444_TLV_FLAG_VALUE       = 0x10,
  RFC5444_TLV_FLAG_EXTVALUE    = 0x08,
  RFC5444_TLV_FLAG_MULTIVALUE  = 0x04,
};

#endif /* RFC5444_CONTEXT_H_ */
