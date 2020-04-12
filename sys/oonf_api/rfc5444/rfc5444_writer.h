
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

#ifndef RFC5444_WRITER_H_
#define RFC5444_WRITER_H_

struct rfc5444_writer;
struct rfc5444_writer_message;

#include "common/avl.h"
#include "common/list.h"
#include "common/netaddr.h"
#include "rfc5444/rfc5444_context.h"
#include "rfc5444/rfc5444_reader.h"
#include "rfc5444/rfc5444_tlv_writer.h"
/*
 * Macros to iterate over existing addresses in a message(fragment)
 * during message generation (finishMessageHeader/finishMessageTLVs
 * callbacks)
 */
#define for_each_fragment_address(first, last, address, loop) oonf_list_for_element_range(first, last, address, addr_node, loop)
#define for_each_message_address(message, address, loop) oonf_list_for_each_element(&message->addr_head, address, addr_node, loop)

/**
 * state machine values for the writer.
 * If compiled with WRITE_STATE_MACHINE, this can check if the functions
 * of the writer are called from the right context.
 */
enum rfc5444_internal_state {
  RFC5444_WRITER_NONE,
  RFC5444_WRITER_ADD_PKTHEADER,
  RFC5444_WRITER_ADD_PKTTLV,
  RFC5444_WRITER_ADD_HEADER,
  RFC5444_WRITER_ADD_MSGTLV,
  RFC5444_WRITER_ADD_ADDRESSES,
  RFC5444_WRITER_FINISH_MSGTLV,
  RFC5444_WRITER_FINISH_HEADER,
  RFC5444_WRITER_FINISH_PKTTLV,
  RFC5444_WRITER_FINISH_PKTHEADER
};

/**
 * This INTERNAL struct represents a single address tlv
 * of an address during message serialization.
 */
struct rfc5444_writer_addrtlv {
  /* tree _target_node of tlvs of a certain type/exttype */
  struct avl_node tlv_node;

  /* backpointer to tlvtype */
  struct rfc5444_writer_tlvtype *tlvtype;

  /* tree _target_node of tlvs used by a single address */
  struct avl_node addrtlv_node;

  /* backpointer to address */
  struct rfc5444_writer_address *address;

  /* tlv type and extension is stored in writer_tlvtype */

  /* tlv value length */
  uint16_t length;

  /*
   * if multiple tlvs with the same type/ext have the same
   * value for a continous block of addresses, they should
   * use the same storage for the value (the pointer should
   * be the same)
   */
  void *value;

  /*
   * true if the TLV has the same length/value for the
   * address before this one too
   */
  bool same_length;
  bool same_value;
};

/**
 * This struct represents a single address during the RFC5444
 * message creation.
 */
struct rfc5444_writer_address {
  /* index of the address */
  int index;

  /* address/prefix */
  struct netaddr address;

  /* node of address list in writer_message */
  struct oonf_list_entity _addr_node;

  /* node for quick access ( O(log n)) to addresses */
  struct avl_node _addr_tree_node;

  /* tree to connect all TLVs of this address */
  struct avl_tree _addrtlv_tree;

  /* address block with same prefix/prefixlen until certain address */
  struct rfc5444_writer_address *_block_end;
  uint8_t _block_headlen;
  bool _block_multiple_prefixlen;

  /* original index of the address when it was added to the output list */
  int _orig_index;

  /* handle mandatory addresses for message fragmentation */
  bool _mandatory_addr;
  bool _done;
};

/**
 * This INTERNAL struct is preallocated for each tlvtype that can be added
 * to an address of a certain message type.
 */
struct rfc5444_writer_tlvtype {
  /* tlv type and extension is stored in writer_tlvtype */
  uint8_t type;

  /* tlv extension type */
  uint8_t exttype;

  /* _target_node of tlvtype list in rfc5444_writer_message */
  struct oonf_list_entity _tlvtype_node;

  /* back pointer to message creator */
  struct rfc5444_writer_message *_creator;

  /* head of writer_addrtlv list */
  struct avl_tree _tlv_tree;

  /* tlv type*256 + tlv_exttype */
  int _full_type;

  /* internal data for address compression */
  int _tlvblock_count[RFC5444_MAX_ADDRLEN];
  bool _tlvblock_multi[RFC5444_MAX_ADDRLEN];
};

/**
 * This struct represents a single content provider of
 * tlvs for a message context.
 */
struct rfc5444_writer_content_provider {
  /* priority of content provider */
  int priority;

  /* message type for this content provider */
  uint8_t msg_type;

  /* callbacks for adding tlvs and addresses to a message */
  void (*addMessageTLVs)(struct rfc5444_writer *);
  void (*addAddresses)(struct rfc5444_writer *);
  void (*finishMessageTLVs)(struct rfc5444_writer *,
    struct rfc5444_writer_address *start,
    struct rfc5444_writer_address *end, bool complete);

  /* node for tree of content providers for a message creator */
  struct avl_node _provider_node;

  /* back pointer to message creator */
  struct rfc5444_writer_message *creator;
};

/**
 * This struct represents a single target (IP) for
 * the rfc5444 writer
 */
struct rfc5444_writer_target {
  /* buffer for packet generation */
  uint8_t *packet_buffer;

  /* maximum number of bytes per packets allowed for target */
  size_t packet_size;

  /* callback for target specific packet handling */
  void (*addPacketHeader)(struct rfc5444_writer *, struct rfc5444_writer_target *);
  void (*finishPacketHeader)(struct rfc5444_writer *, struct rfc5444_writer_target *);
  void (*sendPacket)(struct rfc5444_writer *, struct rfc5444_writer_target *, void *, size_t);

  /* internal handling for packet sequence numbers */
  bool _has_seqno;
  uint16_t _seqno;

  /* node for list of all targets*/
  struct oonf_list_entity _target_node;

  /* packet buffer is currently flushed */
  bool _is_flushed;

  /* buffer for constructing the current packet */
  struct rfc5444_tlv_writer_data _pkt;

  /* number of bytes used by messages */
  size_t _bin_msgs_size;
};

/**
 * This struct is allocated for each message type that can
 * be generated by the writer.
 */
struct rfc5444_writer_message {
  /* _target_node for tree of message creators */
  struct avl_node _msgcreator_node;

  /* tree of message content providers */
  struct avl_tree _provider_tree;

  /*
   * true if the creator has already registered
   * false if the creator was registered because of a tlvtype or content
   * provider registration
   */
  bool _registered;

  /* true if a different message must be generated for each target */
  bool target_specific;

  /* message type */
  uint8_t type;

  /* message address length */
  uint8_t addr_len;

  /* message hopcount */
  bool has_hopcount;
  uint8_t hopcount;

  /* message hoplimit */
  bool has_hoplimit;
  uint8_t hoplimit;

  /* message originator */
  bool has_origaddr;
  uint8_t orig_addr[RFC5444_MAX_ADDRLEN];

  /* message sequence number */
  uint16_t seqno;
  bool has_seqno;

  /* head of writer_address list/tree */
  struct oonf_list_entity _addr_head;
  struct avl_tree _addr_tree;

  /* head of message specific tlvtype list */
  struct oonf_list_entity _msgspecific_tlvtype_head;

  /* callbacks for controling the message header fields */
  void (*addMessageHeader)(struct rfc5444_writer *, struct rfc5444_writer_message *);
  void (*finishMessageHeader)(struct rfc5444_writer *, struct rfc5444_writer_message *,
      struct rfc5444_writer_address *, struct rfc5444_writer_address *, bool);

  /* callback to determine if a message shall be forwarded */
  bool (*forward_target_selector)(struct rfc5444_writer_target *);

  /* number of bytes necessary for addressblocks including tlvs */
  size_t _bin_addr_size;

  /* custom user data */
  void *user;
};

/**
 * This struct represents a content provider for adding
 * tlvs to a packet header.
 */
struct rfc5444_writer_pkthandler {
  /* _target_node for list of packet handlers */
  struct oonf_list_entity _pkthandle_node;

  /* callbacks for packet handler */
  void (*addPacketTLVs)(struct rfc5444_writer *, struct rfc5444_writer_target *);
  void (*finishPacketTLVs)(struct rfc5444_writer *, struct rfc5444_writer_target *);
};

/**
 * This struct represents the internal state of a
 * rfc5444 writer.
 */
struct rfc5444_writer {
  /* buffer for messages */
  uint8_t *msg_buffer;

  /* length of message buffer */
  size_t msg_size;

  /* buffer for addrtlv values of a message */
  uint8_t *addrtlv_buffer;
  size_t addrtlv_size;

  /* callbacks for memory management, NULL for calloc()/free() */
  struct rfc5444_writer_address* (*malloc_address_entry)(void);
  struct rfc5444_writer_addrtlv* (*malloc_addrtlv_entry)(void);

  void (*free_address_entry)(void *);
  void (*free_addrtlv_entry)(void *);

  /*
   * target of current generated message
   * only used for target specific message types
   */
  struct rfc5444_writer_target *msg_target;

  /* tree of all message handlers */
  struct avl_tree _msgcreators;

  /* list of all packet handlers */
  struct oonf_list_entity _pkthandlers;

  /* list of all targets */
  struct oonf_list_entity _targets;

  /* list of generic tlvtypes */
  struct oonf_list_entity _addr_tlvtype_head;

  /* buffer for constructing the current message */
  struct rfc5444_tlv_writer_data _msg;

  /* number of bytes of addrtlv buffer currently used */
  size_t _addrtlv_used;

  /* internal state of writer */
  enum rfc5444_internal_state _state;
};

/* functions that can be called from addAddress callback */
struct rfc5444_writer_address *rfc5444_writer_add_address(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, const struct netaddr *, bool mandatory);
enum rfc5444_result rfc5444_writer_add_addrtlv(struct rfc5444_writer *writer,
    struct rfc5444_writer_address *addr, struct rfc5444_writer_tlvtype *tlvtype,
    const void *value, size_t length, bool allow_dup);

/* functions that can be called from add/finishMessageTLVs callback */
enum rfc5444_result rfc5444_writer_add_messagetlv(struct rfc5444_writer *writer,
    uint8_t type, uint8_t exttype, const void *value, size_t length);
enum rfc5444_result rfc5444_writer_allocate_messagetlv(struct rfc5444_writer *writer,
    bool has_exttype, size_t length);
enum rfc5444_result rfc5444_writer_set_messagetlv(struct rfc5444_writer *writer,
    uint8_t type, uint8_t exttype, const void *value, size_t length);

/* functions that can be called from add/finishMessageHeader callback */
void rfc5444_writer_set_msg_addrlen(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, uint8_t addrlen);
void rfc5444_writer_set_msg_header(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, bool has_originator,
    bool has_hopcount, bool has_hoplimit, bool has_seqno);
void rfc5444_writer_set_msg_originator(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, const void *originator);
void rfc5444_writer_set_msg_hopcount(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, uint8_t hopcount);
void rfc5444_writer_set_msg_hoplimit(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, uint8_t hoplimit);
void rfc5444_writer_set_msg_seqno(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, uint16_t seqno);

/* functions that can be called from add/finishPacketTLVs callback */
enum rfc5444_result rfc5444_writer_add_packettlv(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target,
    uint8_t type, uint8_t exttype, void *value, size_t length);
enum rfc5444_result rfc5444_writer_allocate_packettlv(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target,
    bool has_exttype, size_t length);
enum rfc5444_result rfc5444_writer_set_packettlv(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target,
    uint8_t type, uint8_t exttype, void *value, size_t length);

/* functions that can be called from add/finishPacketHeader */
void rfc5444_writer_set_pkt_header(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target, bool has_seqno);
void rfc5444_writer_set_pkt_seqno(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target, uint16_t seqno);

/* functions that can be called outside the callbacks */
int rfc5444_writer_register_addrtlvtype(struct rfc5444_writer *writer,
    struct rfc5444_writer_tlvtype *type, int msgtype);
void rfc5444_writer_unregister_addrtlvtype(struct rfc5444_writer *writer,
    struct rfc5444_writer_tlvtype *tlvtype);

int rfc5444_writer_register_msgcontentprovider(
    struct rfc5444_writer *writer, struct rfc5444_writer_content_provider *cpr,
    struct rfc5444_writer_tlvtype *addrtlvs, size_t addrtlv_count);
void rfc5444_writer_unregister_content_provider(
    struct rfc5444_writer *writer, struct rfc5444_writer_content_provider *cpr,
    struct rfc5444_writer_tlvtype *addrtlvs, size_t addrtlv_count);

struct rfc5444_writer_message *rfc5444_writer_register_message(
    struct rfc5444_writer *writer, uint8_t msgid, bool if_specific, uint8_t addr_len);
void rfc5444_writer_unregister_message(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg);

void rfc5444_writer_register_pkthandler(struct rfc5444_writer *writer,
    struct rfc5444_writer_pkthandler *pkt);
void rfc5444_writer_unregister_pkthandler(struct rfc5444_writer *writer,
    struct rfc5444_writer_pkthandler *pkt);

void rfc5444_writer_register_target(struct rfc5444_writer *writer,
    struct rfc5444_writer_target *target);
void rfc5444_writer_unregister_target(
    struct rfc5444_writer *writer, struct rfc5444_writer_target *target);

/* prototype for message creation target filter */
typedef bool (*rfc5444_writer_targetselector)(struct rfc5444_writer *, struct rfc5444_writer_target *, void *);

bool rfc5444_writer_singletarget_selector(struct rfc5444_writer *, struct rfc5444_writer_target *, void *);
bool rfc5444_writer_alltargets_selector(struct rfc5444_writer *, struct rfc5444_writer_target *, void *);

enum rfc5444_result rfc5444_writer_create_message(
    struct rfc5444_writer *writer, uint8_t msgid,
    rfc5444_writer_targetselector useIf, void *param);

enum rfc5444_result rfc5444_writer_forward_msg(struct rfc5444_writer *writer,
    uint8_t *msg, size_t len);

void rfc5444_writer_flush(struct rfc5444_writer *, struct rfc5444_writer_target *, bool);

void rfc5444_writer_init(struct rfc5444_writer *);
void rfc5444_writer_cleanup(struct rfc5444_writer *writer);

/* internal functions that are not exported to the user */
void _rfc5444_writer_free_addresses(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg);
void _rfc5444_writer_begin_packet(struct rfc5444_writer *writer, struct rfc5444_writer_target *target);

/**
 * creates a message of a certain ID for a single target
 * @param writer pointer to writer context
 * @param msgid type of message
 * @param target pointer to outgoing target
 * @return RFC5444_OKAY if message was created and added to packet buffer,
 *   RFC5444_... otherwise
 */
static inline enum rfc5444_result
rfc5444_writer_create_message_singletarget(
    struct rfc5444_writer *writer, uint8_t msgid, struct rfc5444_writer_target *target) {
  return rfc5444_writer_create_message(writer, msgid, rfc5444_writer_singletarget_selector, target);
}

/**
 * creates a message of a certain ID for all target
 * @param writer pointer to writer context
 * @param msgid type of message
 * @return RFC5444_OKAY if message was created and added to packet buffer,
 *   RFC5444_... otherwise
 */
static inline enum rfc5444_result
rfc5444_writer_create_message_alltarget(
    struct rfc5444_writer *writer, uint8_t msgid) {
  return rfc5444_writer_create_message(writer, msgid, rfc5444_writer_alltargets_selector, NULL);
}

#endif /* RFC5444_WRITER_H_ */
