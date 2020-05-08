
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

#ifndef RFC5444_PARSER_H_
#define RFC5444_PARSER_H_

#include "common/avl.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_context.h"

/* Bitarray with 256 elements for skipping addresses/tlvs */
struct rfc5444_reader_bitarray256 {
  uint32_t a[256/32];
};

/* type of context for a rfc5444_reader_tlvblock_context */
enum rfc5444_reader_tlvblock_context_type {
  RFC5444_CONTEXT_PACKET,
  RFC5444_CONTEXT_MESSAGE,
  RFC5444_CONTEXT_ADDRESS
};

/**
 * This struct temporary holds the content of a decoded TLV.
 */
struct rfc5444_reader_tlvblock_entry {
  /* tree of TLVs */
  struct avl_node node;

  /* tlv type */
  uint8_t type;

  /* tlv flags */
  uint8_t flags;

  /* tlv type extension */
  uint8_t type_ext;

  /* tlv value length */
  uint16_t length;

  /*
   * pointer to tlv value, NULL if length == 0
   * this pointer is NOT aligned
   */
  uint8_t *single_value;

  /* index range of tlv (for address blocks) */
  uint8_t index1, index2;

  /*
   * this points to the next tlvblock entry if there is more than one
   * fitting to the current callback (e.g. multiple linkmetric tlvs)
   */
  struct rfc5444_reader_tlvblock_entry *next_entry;

  /* internal sorting order for types: tlvtype * 256 + exttype */
  uint16_t _order;

  /*
   * pointer to start of value array, can be different from
   * "value" because of multivalue tlvs
   */
  uint8_t *_value;

  /* true if this is a multivalue tlv */
  bool _multivalue_tlv;

  /* internal bitarray to mark tlvs that shall be skipped by the next handler */
  struct rfc5444_reader_bitarray256 int_drop_tlv;
};

/* common context for packet, message and address TLV block */
struct rfc5444_reader_tlvblock_context {
  /* backpointer to reader */
  struct rfc5444_reader *reader;

  /* pointer to tlvblock consumer */
  struct rfc5444_reader_tlvblock_consumer *consumer;

  /* applicable for all TLV blocks */
  enum rfc5444_reader_tlvblock_context_type type;

  /* packet context */
  uint8_t pkt_version;
  uint8_t pkt_flags;

  bool has_pktseqno;
  uint16_t pkt_seqno;

  /*
   * message context
   * only for message and address TLV blocks
   */
  uint8_t msg_type;
  uint8_t msg_flags;
  uint8_t addr_len;

  bool has_hopcount;
  uint8_t hopcount;

  bool has_hoplimit;
  uint8_t hoplimit;

  bool has_origaddr;
  struct netaddr orig_addr;

  uint16_t seqno;
  bool has_seqno;

  /* processing callbacks can set this variable to prevent forwarding */
  bool _do_not_forward;

  /*
   * address context
   * only for address TLV blocks
   */
  struct netaddr addr;
};

/* internal representation of a parsed address block */
struct rfc5444_reader_addrblock_entry {
  /* single linked list of address blocks */
  struct oonf_list_entity oonf_list_node;

  /* corresponding tlv block */
  struct avl_tree tlvblock;

  /* number of addresses */
  uint8_t num_addr;

  /* start index/length of middle part of address */
  uint8_t mid_start, mid_len;

  /*
   * pointer to list of prefixes, NULL if same prefix length
   * for all addresses
   */
  uint8_t *prefixes;

  /* pointer to array of middle address parts */
  uint8_t *mid_src;

  /* storage for head/tail of address */
  uint8_t addr[RFC5444_MAX_ADDRLEN];

  /* storage for fixed prefix length */
  uint8_t prefixlen;

  /* bitarray to mark addresses that shall be skipped by the next handler */
  struct rfc5444_reader_bitarray256 dropAddr;
};

/**
 * representation of a consumer for a tlv block and context
 */
struct rfc5444_reader_tlvblock_consumer_entry {
  /* sorted list of consumer entries */
  struct oonf_list_entity _node;

  /* set by the consumer if the entry is mandatory */
  bool mandatory;

  /* set by the consumer to define the type of the tlv */
  uint8_t type;

  /* set by the consumer to define the required type extension */
  uint8_t type_ext;

  /* set by the consumer to require a certain type extension */
  bool match_type_ext;

  /* set by the consumer to define the minimum length of the TLVs value */
  uint16_t min_length;

  /*
   * set by the consumer to define the maximum length of the TLVs value.
   * If smaller than min_length, this value will be assumed the same as
   * min_length.
   */
  uint16_t max_length;

  /* set by consumer to activate length checking */
  bool match_length;

  /* set by the consumer to make the parser copy the TLV value into a private buffer */
  void *copy_value;

  /*
   * set by parser as a pointer to the TLVs data
   * This pointer will only be valid during the runtime of the
   * corresponding callback. Do not copy the pointer into a global
   * variable
   */
  struct rfc5444_reader_tlvblock_entry *tlv;

  /* set by the consumer callback together with a RFC5444_DROP_TLV to drop this TLV */
  bool drop;
};

/* representation of a tlv block consumer */
struct rfc5444_reader_tlvblock_consumer {
  /* sorted tree of consumers for a packet, message or address tlv block */
  struct avl_node _node;

  /* order of this consumer */
  int order;

  /* if true the consumer will be called for all messages */
  bool default_msg_consumer;

  /*
   * message id of message and address consumers, ignored if
   * default_msg_consumer is true
   */
  uint8_t msg_id;

  /* true if an address block consumer, false if message/packet consumer */
  bool addrblock_consumer;

  /* List of sorted consumer entries */
  struct oonf_list_entity _consumer_list;

  /* consumer for TLVblock context start and end*/
  enum rfc5444_result (*start_callback)(struct rfc5444_reader_tlvblock_context *context);
  enum rfc5444_result (*end_callback)(
      struct rfc5444_reader_tlvblock_context *context, bool dropped);

  /* consumer for single TLV */
  enum rfc5444_result (*tlv_callback)(struct rfc5444_reader_tlvblock_entry *,
      struct rfc5444_reader_tlvblock_context *context);

  /* consumer for tlv block and context */
  enum rfc5444_result (*block_callback)(struct rfc5444_reader_tlvblock_context *context);
  enum rfc5444_result (*block_callback_failed_constraints)(
      struct rfc5444_reader_tlvblock_context *context);
};

/* representation of the internal state of a rfc5444 parser */
struct rfc5444_reader {
  /* sorted tree of packet consumers */
  struct avl_tree packet_consumer;

  /* sorted tree of message/addr consumers */
  struct avl_tree message_consumer;

  /* callback for message forwarding */
  void (*forward_message)(struct rfc5444_reader_tlvblock_context *context, uint8_t *buffer, size_t length);

  /* callbacks for memory management */
  struct rfc5444_reader_tlvblock_entry* (*malloc_tlvblock_entry)(void);
  struct rfc5444_reader_addrblock_entry* (*malloc_addrblock_entry)(void);

  void (*free_tlvblock_entry)(void *);
  void (*free_addrblock_entry)(void *);
};

void rfc5444_reader_init(struct rfc5444_reader *);
void rfc5444_reader_cleanup(struct rfc5444_reader *);
void rfc5444_reader_add_packet_consumer(struct rfc5444_reader *parser,
    struct rfc5444_reader_tlvblock_consumer *consumer,
    struct rfc5444_reader_tlvblock_consumer_entry *entries, size_t entrycount);
void rfc5444_reader_add_message_consumer(struct rfc5444_reader *,
    struct rfc5444_reader_tlvblock_consumer *,
    struct rfc5444_reader_tlvblock_consumer_entry *,
    size_t entrycount);

void rfc5444_reader_remove_packet_consumer(
    struct rfc5444_reader *, struct rfc5444_reader_tlvblock_consumer *);
void rfc5444_reader_remove_message_consumer(
    struct rfc5444_reader *, struct rfc5444_reader_tlvblock_consumer *);

enum rfc5444_result rfc5444_reader_handle_packet(
    struct rfc5444_reader *parser, uint8_t *buffer, size_t length);

uint8_t *rfc5444_reader_get_tlv_value(
    struct rfc5444_reader_tlvblock_entry *tlv);

/**
 * Call to set the do-not-forward flag in message context
 * @param context pointer to message context
 */
static inline void
rfc5444_reader_prevent_forwarding(struct rfc5444_reader_tlvblock_context *context) {
  context->_do_not_forward = true;
}

#endif /* RFC5444_PARSER_H_ */
