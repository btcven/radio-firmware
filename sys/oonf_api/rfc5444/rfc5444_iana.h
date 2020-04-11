
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

#ifndef RFC5444_IANA_H_
#define RFC5444_IANA_H_

#include "common/common_types.h"
#include "common/netaddr.h"

/*
 * IANA registered IP/UDP-port number
 * and multicast groups for MANET (RFC 5498)
 */

enum rfc5444_iana {
  RFC5444_MANET_IPPROTO  = 138,
  RFC5444_MANET_UDP_PORT = 269,
};

EXPORT extern const struct netaddr RFC5444_MANET_MULTICAST_V4;
EXPORT extern const struct netaddr RFC5444_MANET_MULTICAST_V6;

/*
 * text variants of the constants above for defaults in
 * configuration sections
 */
#define RFC5444_MANET_IPPROTO_TXT      "138"
#define RFC5444_MANET_UDP_PORT_TXT     "269"
#define RFC5444_MANET_MULTICAST_V4_TXT "224.0.0.109"
#define RFC5444_MANET_MULTICAST_V6_TXT "ff02::6d"

/*
 * this is a list of all globally defined IANA
 * message types
 */

enum rfc5444_msgtype_iana {
  /* RFC 6130 (NHDP) */
  RFC5444_MSGTYPE_HELLO = 0,

  /* OLSRv2 draft 19 */
  RFC5444_MSGTYPE_TC = 1,
};

/*
 * this is a list of all globally defined IANA
 * packet TLVs and their allocated values
 */

enum rfc5444_pkttlvs_iana {
  /* RFC 6622 (rfc5444-sec) */
  RFC5444_PKTTLV_ICV       = 5,
  RFC5444_PKTTLV_TIMESTAMP = 6,
};

/*
 * this is a list of all globally defined IANA
 * message TLVs and their allocated values
 */

enum rfc5444_msgtlvs_iana {
  /* RFC 5497 (timetlv) */
  RFC5444_MSGTLV_INTERVAL_TIME = 0,
  RFC5444_MSGTLV_VALIDITY_TIME = 1,

  /* RFC 6622 (rfc5444-sec) */
  RFC5444_MSGTLV_ICV           = 5,
  RFC5444_MSGTLV_TIMESTAMP     = 6,

  /* OLSRv2 draft-19 */
  RFC5444_MSGTLV_MPR_WILLING   = 224,
  RFC5444_MSGTLV_CONT_SEQ_NUM  = 225,
};

/* values for MPR_WILLING message TLV */
#define RFC5444_WILLINGNESS_DEFAULT_STRING  "7"

enum rfc5444_willingness_values {
  RFC5444_WILLINGNESS_UNDEFINED = -1,
  RFC5444_WILLINGNESS_MIN = 0,
  RFC5444_WILLINGNESS_NEVER = 0,
  RFC5444_WILLINGNESS_DEFAULT = 7,
  RFC5444_WILLINGNESS_ALWAYS = 15,
  RFC5444_WILLINGNESS_MAX = 15,

  RFC5444_WILLINGNESS_FLOODING_MASK = 0xf0,
  RFC5444_WILLINGNESS_FLOODING_SHIFT  = 4,
  RFC5444_WILLINGNESS_ROUTING_MASK  = 0x0f,
};

/* extension types of CONT_SEQ_NUM TLV */
enum rfc5444_cont_seq_num_ext {
  RFC5444_CONT_SEQ_NUM_COMPLETE = 0,
  RFC5444_CONT_SEQ_NUM_INCOMPLETE = 1,
};

/*
 * this is a list of all globally defined IANA
 * address TLVs and their allocated values
 */

enum rfc5444_addrtlv_iana {
  /* RFC 5497 (timetlv) */
  RFC5444_ADDRTLV_INTERVAL_TIME = 0,
  RFC5444_ADDRTLV_VALIDITY_TIME = 1,

  /* RFC 6130 (NHDP) */
  RFC5444_ADDRTLV_LOCAL_IF      = 2,
  RFC5444_ADDRTLV_LINK_STATUS   = 3,
  RFC5444_ADDRTLV_OTHER_NEIGHB  = 4,

  /* RFC 6622 (rfc5444-sec) */
  RFC5444_ADDRTLV_ICV           = 5,
  RFC5444_ADDRTLV_TIMESTAMP     = 6,

  /* OLSRv2 draft-17 */
  RFC5444_ADDRTLV_LINK_METRIC   = 224,
  RFC5444_ADDRTLV_MPR           = 225,
  RFC5444_ADDRTLV_NBR_ADDR_TYPE = 226,
  RFC5444_ADDRTLV_GATEWAY       = 227,
};

/* values for LOCAL_IF address TLV */
enum rfc5444_localif_values {
  RFC5444_LOCALIF_THIS_IF       = 0,
  RFC5444_LOCALIF_OTHER_IF      = 1,
};

/* values for LINK_STATUS address TLV */
enum rfc5444_linkstatus_values {
  RFC5444_LINKSTATUS_LOST       = 0,
  RFC5444_LINKSTATUS_SYMMETRIC  = 1,
  RFC5444_LINKSTATUS_HEARD      = 2,
};

/* values for OTHER_NEIGHB address TLV */
enum rfc5444_otherneigh_values {
  RFC5444_OTHERNEIGHB_LOST      = 0,
  RFC5444_OTHERNEIGHB_SYMMETRIC = 1,
};

/* values for LINK_METRIC address TLV */
enum rfc5444_linkmetric_values {
  RFC5444_LINKMETRIC_INCOMING_LINK  = 1<<15,
  RFC5444_LINKMETRIC_OUTGOING_LINK  = 1<<14,
  RFC5444_LINKMETRIC_INCOMING_NEIGH = 1<<13,
  RFC5444_LINKMETRIC_OUTGOING_NEIGH = 1<<12,

  RFC5444_LINKMETRIC_COST_MASK = 0x0fff,
  RFC5444_LINKMETRIC_FLAGS_MASK = 0xf000,
};

/* values for MPR address TLV */
enum rfc5444_mpr_values {
  RFC5444_MPR_NOMPR = 0,
  RFC5444_MPR_FLOODING = 1,
  RFC5444_MPR_ROUTING = 2,
  RFC5444_MPR_FLOOD_ROUTE = 3,
};

/* values for NBR_ADDR_TYPE address TLV */
enum rfc5444_nbr_addr_type_values {
  RFC5444_NBR_ADDR_TYPE_ORIGINATOR = 1,
  RFC5444_NBR_ADDR_TYPE_ROUTABLE = 2,
  RFC5444_NBR_ADDR_TYPE_ROUTABLE_ORIG = 3,
};

#endif /* RFC5444_IANA_H_ */
