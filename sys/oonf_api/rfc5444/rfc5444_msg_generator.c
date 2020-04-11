
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "rfc5444/rfc5444_writer.h"
#include "rfc5444/rfc5444_api_config.h"

#include "kernel_defines.h"

/* data necessary for automatic address compression */
struct _rfc5444_internal_addr_compress_session {
  struct rfc5444_writer_address *ptr;
  int total;
  int current;
  bool multiplen;
};

static void _calculate_tlv_flags(struct rfc5444_writer_address *addr, bool first);
static void _close_addrblock(struct _rfc5444_internal_addr_compress_session *acs,
    struct rfc5444_writer_message *msg, struct rfc5444_writer_address *last_addr, int);
static void _finalize_message_fragment(struct rfc5444_writer *writer,
    struct rfc5444_writer_message *msg, struct rfc5444_writer_address *first,
    struct rfc5444_writer_address *last, bool not_fragmented,
    rfc5444_writer_targetselector useIf, void *param);
static int _compress_address(struct _rfc5444_internal_addr_compress_session *acs,
    struct rfc5444_writer_message *msg, struct rfc5444_writer_address *addr,
    int same_prefixlen, bool first);
static void _write_addresses(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg,
    struct rfc5444_writer_address *first_addr, struct rfc5444_writer_address *last_addr);
static void _write_msgheader(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg);

/**
 * Create a message with a defined type
 * This function must NOT be called from the rfc5444 writer callbacks.
 *
 * @param writer pointer to writer context
 * @param msgid type of message
 * @param useIf pointer to interface selector
 * @param param last parameter of interface selector
 * @return RFC5444_OKAY if message was created and added to packet buffer,
 *   RFC5444_... otherwise
 */
enum rfc5444_result
rfc5444_writer_create_message(struct rfc5444_writer *writer, uint8_t msgid,
    rfc5444_writer_targetselector useIf, void *param) {
  struct rfc5444_writer_message *msg;
  struct rfc5444_writer_content_provider *prv;
  struct oonf_list_entity *ptr1;
  struct rfc5444_writer_address *addr = NULL, *last_processed = NULL;
  struct rfc5444_writer_address *first_addr = NULL, *first_mandatory = NULL;
  struct rfc5444_writer_tlvtype *tlvtype;
  struct rfc5444_writer_target *interface;

  struct _rfc5444_internal_addr_compress_session acs[RFC5444_MAX_ADDRLEN];
  int best_size, best_head, same_prefixlen = 0;
  int i, idx, non_mandatory;
  bool first;
  bool not_fragmented;
  size_t max_msg_size;
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_NONE);
#endif

  /* do nothing if no interface is defined */
  if (oonf_list_is_empty(&writer->_targets)) {
    return RFC5444_OKAY;
  }

  /* find message create instance for the requested message */
  msg = avl_find_element(&writer->_msgcreators, &msgid, msg, _msgcreator_node);
  if (msg == NULL) {
    /* error, no msgcreator found */
    return RFC5444_NO_MSGCREATOR;
  }

  /*
   * test if we need interface specific messages
   * and this is not the single_if selector
   */
  if (!msg->target_specific) {
    /* not interface specific */
    writer->msg_target = NULL;
  }
  else if (useIf == rfc5444_writer_singletarget_selector) {
    /* interface specific, but single_if selector is used */
    writer->msg_target = param;
  }
  else {
    /* interface specific, but generic selector is used */
    enum rfc5444_result result;

    oonf_list_for_each_element(&writer->_targets, interface, _target_node) {
      /* check if we should send over this interface */
      if (!useIf(writer, interface, param)) {
        continue;
      }

      /* create an unique message by recursive call */
      result = rfc5444_writer_create_message(writer, msgid, rfc5444_writer_singletarget_selector, interface);
      if (result != RFC5444_OKAY) {
        return result;
      }
    }
    return RFC5444_OKAY;
  }

  /*
   * initialize packet buffers for all interfaces if necessary
   * and calculate message MTU
   */
  max_msg_size = writer->msg_size;
  oonf_list_for_each_element(&writer->_targets, interface, _target_node) {
    size_t interface_msg_mtu;

    /* check if we should send over this interface */
    if (!useIf(writer, interface, param)) {
      continue;
    }

    /* start packet if necessary */
    if (interface->_is_flushed) {
      _rfc5444_writer_begin_packet(writer, interface);
    }

    interface_msg_mtu = interface->packet_size
        - (interface->_pkt.header + interface->_pkt.added + interface->_pkt.allocated);
    if (interface_msg_mtu < max_msg_size) {
      max_msg_size = interface_msg_mtu;
    }
  }

  /* initialize message tlvdata */
  _rfc5444_tlv_writer_init(&writer->_msg, max_msg_size, writer->msg_size);

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_ADD_HEADER;
#endif
  /* let the message creator write the message header */
  rfc5444_writer_set_msg_header(writer, msg, false, false, false, false);
  if (msg->addMessageHeader) {
    msg->addMessageHeader(writer, msg);
  }

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_ADD_MSGTLV;
#endif

  /* call content providers for message TLVs */
  avl_for_each_element(&msg->_provider_tree, prv, _provider_node) {
    if (prv->addMessageTLVs) {
      prv->addMessageTLVs(writer);
    }
  }

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_ADD_ADDRESSES;
#endif
  /* call content providers for addresses */
  avl_for_each_element(&msg->_provider_tree, prv, _provider_node) {
    if (prv->addAddresses) {
      prv->addAddresses(writer);
    }
  }

  not_fragmented = true;

  /* no addresses ? */
  if (oonf_list_is_empty(&msg->_addr_head)) {
    _finalize_message_fragment(writer, msg, NULL, NULL, true, useIf, param);
#if WRITER_STATE_MACHINE == true
    writer->_state = RFC5444_WRITER_NONE;
#endif
    _rfc5444_writer_free_addresses(writer, msg);
    return RFC5444_OKAY;
  }

  /* start address compression */
  first = true;
  addr = first_addr = oonf_list_first_element(&msg->_addr_head, addr, _addr_node);

  /* loop through addresses */
  idx = 0;
  non_mandatory = 0;
  ptr1 = msg->_addr_head.next;
  while(ptr1 != &msg->_addr_head) {
    addr = container_of(ptr1, struct rfc5444_writer_address, _addr_node);
    if (addr->_done && !addr->_mandatory_addr) {
      ptr1 = ptr1->next;
      continue;
    }

    if (first) {
      /* clear message specific tlvtype information for address compression */
      oonf_list_for_each_element(&msg->_msgspecific_tlvtype_head, tlvtype, _tlvtype_node) {
        memset(tlvtype->_tlvblock_count, 0, sizeof(tlvtype->_tlvblock_count));
        memset(tlvtype->_tlvblock_multi, 0, sizeof(tlvtype->_tlvblock_multi));
      }

      /* clear generic tlvtype information for address compression */
      oonf_list_for_each_element(&writer->_addr_tlvtype_head, tlvtype, _tlvtype_node) {
        memset(tlvtype->_tlvblock_count, 0, sizeof(tlvtype->_tlvblock_count));
        memset(tlvtype->_tlvblock_multi, 0, sizeof(tlvtype->_tlvblock_multi));
      }

      /* clear address compression session */
      memset(acs, 0, sizeof(acs));
      same_prefixlen = 1;
    }

    /* remember first mandatory address */
    if (first_addr == NULL && addr->_mandatory_addr) {
      first_addr = addr;
    }

    addr->index = idx++;

    /* calculate same_length/value for tlvs */
    _calculate_tlv_flags(addr, first);

    /* update session with address */
    same_prefixlen = _compress_address(acs, msg, addr, same_prefixlen, first);
    first = false;

    /* look for best current compression */
    best_head = -1;
    best_size = writer->_msg.max + 1;
#if DO_ADDR_COMPRESSION == true
    for (i = 0; i < msg->addr_len; i++) {
#else
    i=0;
    {
#endif
      int size = acs[i].total + acs[i].current;
      int count = addr->index - acs[i].ptr->index;

      /* a block of 255 addresses have an index difference of 254 */
      if (size < best_size && count <= 254) {
        best_head = i;
        best_size = size;
      }
    }

    /* fragmentation necessary ? */
    if (best_head == -1) {
      if (non_mandatory == 0) {
        /* the mandatory addresses plus one non-mandatory do not fit into a block! */
#if WRITER_STATE_MACHINE == true
        writer->_state = RFC5444_WRITER_NONE;
#endif
        _rfc5444_writer_free_addresses(writer, msg);
        return -1;
      }
      not_fragmented = false;

      /* close all address blocks */
      _close_addrblock(acs, msg, last_processed, 0);

      /* write message fragment */
      _finalize_message_fragment(writer, msg, first_addr, last_processed, not_fragmented, useIf, param);

      if (first_mandatory != NULL) {
        first_addr = first_mandatory;
      }
      else {
        first_addr = addr;
      }
      first = true;

      /* continue without stepping forward */
      continue;
    } else {
      /* add cost for this address to total costs */
#if DO_ADDR_COMPRESSION == true
      for (i = 0; i < msg->addr_len; i++) {
#else
      i=0;
      {
#endif
        acs[i].total += acs[i].current;

#if DEBUG_CLEANUP == true
        acs[i].current = 0;
#endif
      }
      last_processed = addr;
      if (!addr->_done) {
        addr->_done = true;

        if (!addr->_mandatory_addr) {
          non_mandatory++;
        }
      }
    }

    ptr1 = ptr1->next;
  }

  /* get last address */
  addr = oonf_list_last_element(&msg->_addr_head, addr, _addr_node);

  /* close all address blocks */
  _close_addrblock(acs, msg, addr, 0);

  /* write message fragment */
  _finalize_message_fragment(writer, msg, first_addr, addr, not_fragmented, useIf, param);

  /* free storage of addresses and address-tlvs */
  _rfc5444_writer_free_addresses(writer, msg);

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_NONE;
#endif
  return RFC5444_OKAY;
}

/**
 * Single interface selector callback for message creation
 * @param writer
 * @param interf
 * @param param pointer to the specified interface
 * @return true if param equals interf, false otherwise
 */
bool
rfc5444_writer_singletarget_selector(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_target *interf, void *param) {
  return interf == param;
}

/**
 * All interface selector callback for message creation
 * @param writer
 * @param interf
 * @param param
 * @return always true
 */
bool rfc5444_writer_alltargets_selector(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_target *interf __attribute__ ((unused)),
    void *param __attribute__ ((unused))) {
  return true;
}

/**
 * Write a binary rfc5444 message into the writers buffer to
 * forward it. This function handles the modification of hopcount
 * and hoplimit field. The original message will not be modified.
 * This function must NOT be called from the rfc5444 writer callbacks.
 *
 * The function does demand the writer context pointer as void*
 * to be compatible with the readers forward_message callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message to be forwarded
 * @param len number of bytes of message
 * @return RFC5444_OKAY if the message was put into the writer buffer,
 *   RFC5444_... if an error happened
 */
enum rfc5444_result
rfc5444_writer_forward_msg(struct rfc5444_writer *writer, uint8_t *msg, size_t len) {
  struct rfc5444_writer_target *target;
  struct rfc5444_writer_message *rfc5444_msg;
  int cnt, hopcount = -1, hoplimit = -1;
  uint16_t size;
  uint8_t flags, addr_len;
  uint8_t *ptr;
  size_t max_msg_size;

#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_NONE);
#endif

  rfc5444_msg = avl_find_element(&writer->_msgcreators, &msg[0], rfc5444_msg, _msgcreator_node);
  if (rfc5444_msg == NULL) {
    /* error, no msgcreator found */
    return RFC5444_NO_MSGCREATOR;
  }

  if (!rfc5444_msg->forward_target_selector) {
    /* no forwarding handler, do not forward */
    return RFC5444_OKAY;
  }

  /* check if message is small enough to be forwarded */
  max_msg_size = 0;
  oonf_list_for_each_element(&writer->_targets, target, _target_node) {
    size_t max;

    if (!rfc5444_msg->forward_target_selector(target)) {
      continue;
    }

    if (target->_is_flushed) {
      /* begin a new packet */
      _rfc5444_writer_begin_packet(writer,target);
    }

    max = target->_pkt.max - (target->_pkt.header + target->_pkt.added + target->_pkt.allocated);
    if (max_msg_size == 0 || max < max_msg_size) {
      max_msg_size = max;
    }
  }

  if (max_msg_size == 0) {
    /* no interface selected */
    return RFC5444_OKAY;
  }

  if (len > max_msg_size) {
    /* message too long, too much data in it */
    return RFC5444_FW_MESSAGE_TOO_LONG;
  }

  flags = msg[1];
  addr_len = (flags & RFC5444_MSG_FLAG_ADDRLENMASK) + 1;

  size = (msg[2] << 8) + msg[3];
  if (size != len) {
    /* bad message size */
    return RFC5444_FW_BAD_SIZE;
  }

  cnt = 4;
  if ((flags & RFC5444_MSG_FLAG_ORIGINATOR) != 0) {
    cnt += addr_len;
  }
  if ((flags & RFC5444_MSG_FLAG_HOPLIMIT) != 0) {
    hoplimit = cnt++;
  }
  if ((flags & RFC5444_MSG_FLAG_HOPCOUNT) != 0) {
    hopcount = cnt++;
  }
  if ((flags & RFC5444_MSG_FLAG_SEQNO) != 0) {
    cnt += 2;
  }

  if (hoplimit != -1 && msg[hoplimit] <= 1) {
    /* do not forward a message with hopcount 1 or 0 */
    return RFC5444_OKAY;
  }

  /* forward message */
  oonf_list_for_each_element(&writer->_targets, target, _target_node) {
    if (!rfc5444_msg->forward_target_selector(target)) {
      continue;
    }

    /* check if we have to flush the message buffer */
    if (target->_pkt.header + target->_pkt.added + target->_pkt.set + target->_bin_msgs_size + len
        > target->_pkt.max) {
      /* flush the old packet */
      rfc5444_writer_flush(writer, target, false);

      /* begin a new one */
      _rfc5444_writer_begin_packet(writer,target);
    }

    ptr = &target->_pkt.buffer[target->_pkt.header + target->_pkt.added
                            + target->_pkt.allocated + target->_bin_msgs_size];
    memcpy(ptr, msg, len);
    target->_bin_msgs_size += len;

    /* correct hoplimit if necesssary */
    if (hoplimit != -1) {
      ptr[hoplimit]--;
    }

    /* correct hopcount if necessary */
    if (hopcount != -1) {
      ptr[hopcount]++;
    }
  }
  return RFC5444_OKAY;
}

/**
 * Adds a tlv to a message.
 * This function must not be called outside the message add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return RFC5444_OKAY if tlv has been added to packet, RFC5444_... otherwise
 */
enum rfc5444_result
rfc5444_writer_add_messagetlv(struct rfc5444_writer *writer,
    uint8_t type, uint8_t exttype, const void *value, size_t length) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_MSGTLV);
#endif
  return _rfc5444_tlv_writer_add(&writer->_msg, type, exttype, value, length);
}

/**
 * Allocate memory for message tlv.
 * This function must not be called outside the message add_tlv callback.
 *
 * @param writer pointer to writer context
 * @param has_exttype true if tlv has an extended type
 * @param length number of bytes in tlv value, 0 if no value
 * @return RFC5444_OKAY if memory for tlv has been allocated, RFC5444_... otherwise
 */
enum rfc5444_result
rfc5444_writer_allocate_messagetlv(struct rfc5444_writer *writer,
    bool has_exttype, size_t length) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_MSGTLV);
#endif
  return _rfc5444_tlv_writer_allocate(&writer->_msg, has_exttype, length);
}

/**
 * Sets a tlv for a message, which memory has been already allocated.
 * This function must not be called outside the message finish_tlv callback.
 *
 * @param writer pointer to writer context
 * @param type tlv type
 * @param exttype tlv extended type, 0 if no extended type
 * @param value pointer to tlv value, NULL if no value
 * @param length number of bytes in tlv value, 0 if no value
 * @return RFC5444_OKAY if tlv has been set to packet, RFC5444_... otherwise
 */
enum rfc5444_result
rfc5444_writer_set_messagetlv(struct rfc5444_writer *writer,
    uint8_t type, uint8_t exttype, const void *value, size_t length) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_FINISH_MSGTLV);
#endif
  return _rfc5444_tlv_writer_set(&writer->_msg, type, exttype, value, length);
}

/**
 * Sets a new address length for a message
 * This function must not be called outside the message add_header callback.
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param addrlen address length, must be less or equal than 16
 */
void
rfc5444_writer_set_msg_addrlen(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_message *msg, uint8_t addrlen) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER);
#endif

  assert(addrlen <= RFC5444_MAX_ADDRLEN);
  assert(addrlen >= 1);

  if (msg->has_origaddr && msg->addr_len != addrlen) {
    /*
     * we have to fix the calculated header length when set_msg_header
     * was called before this function
     */
    writer->_msg.header = writer->_msg.header + addrlen - msg->addr_len;
  }
  msg->addr_len = addrlen;
}

/**
 * Initialize the header of a message.
 * This function must not be called outside the message add_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param has_originator true if header contains an originator address
 * @param has_hopcount true if header contains a hopcount
 * @param has_hoplimit true if header contains a hoplimit
 * @param has_seqno true if header contains a sequence number
 */
void
rfc5444_writer_set_msg_header(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg,
    bool has_originator, bool has_hopcount, bool has_hoplimit, bool has_seqno) {

#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER);
#endif

  msg->has_origaddr = has_originator;
  msg->has_hoplimit = has_hoplimit;
  msg->has_hopcount = has_hopcount;
  msg->has_seqno = has_seqno;

  /* fixed parts: _msg type, flags, length, tlvblock-length */
  writer->_msg.header = 6;

  if (has_originator) {
    writer->_msg.header += msg->addr_len;
  }
  if (has_hoplimit) {
    writer->_msg.header++;
  }
  if (has_hopcount) {
    writer->_msg.header++;
  }
  if (has_seqno) {
    writer->_msg.header += 2;
  }
}

/**
 * Set originator address of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param originator pointer to originator address buffer
 */
void
rfc5444_writer_set_msg_originator(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_message *msg, const void *originator) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER || writer->_state == RFC5444_WRITER_FINISH_HEADER);
#endif

  memcpy(&msg->orig_addr[0], originator, msg->addr_len);
}

/**
 * Set hopcount of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param hopcount
 */
void
rfc5444_writer_set_msg_hopcount(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_message *msg, uint8_t hopcount) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER || writer->_state == RFC5444_WRITER_FINISH_HEADER);
#endif
  msg->hopcount = hopcount;
}

/**
 * Set hoplimit of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param hoplimit
 */
void
rfc5444_writer_set_msg_hoplimit(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_message *msg, uint8_t hoplimit) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER || writer->_state == RFC5444_WRITER_FINISH_HEADER);
#endif
  msg->hoplimit = hoplimit;
}

/**
 * Set sequence number of a message header
 * This function must not be called outside the message
 * add_header or finish_header callback.
 *
 * @param writer pointer to writer context
 * @param msg pointer to message object
 * @param seqno sequence number of message header
 */
void
rfc5444_writer_set_msg_seqno(struct rfc5444_writer *writer __attribute__ ((unused)),
    struct rfc5444_writer_message *msg, uint16_t seqno) {
#if WRITER_STATE_MACHINE == true
  assert(writer->_state == RFC5444_WRITER_ADD_HEADER || writer->_state == RFC5444_WRITER_FINISH_HEADER);
#endif
  msg->seqno = seqno;
}

/**
 * Update address compression session when a potential address block
 * is finished.
 *
 * @param acs pointer to address compression session
 * @param msg pointer to message object
 * @param last_addr pointer to last address object
 * @param common_head length of common head
 * @return common_head (might be modified common_head was 1)
 */
static void
_close_addrblock(struct _rfc5444_internal_addr_compress_session *acs,
    struct rfc5444_writer_message *msg __attribute__ ((unused)),
    struct rfc5444_writer_address *last_addr, int common_head) {
  int best;
#if DO_ADDR_COMPRESSION == true
  int i, size;
  if (common_head > msg->addr_len) {
    /* nothing to do */
    return;
  }
#else
  assert(common_head == 0);
#endif
  /* check for best compression at closed blocks */
  best = common_head;
#if DO_ADDR_COMPRESSION == true
  size = acs[common_head].total;
  for (i = common_head + 1; i < msg->addr_len; i++) {
    if (acs[i].total < size) {
      size = acs[i].total;
      best = i;
    }
  }
#endif
  /* store address block for later binary generation */
  acs[best].ptr->_block_end = last_addr;
  acs[best].ptr->_block_multiple_prefixlen = acs[best].multiplen;
  acs[best].ptr->_block_headlen = best;

#if DO_ADDR_COMPRESSION == true
  for (i = common_head + 1; i < msg->addr_len; i++) {
    /* remember best block compression */
    acs[i].total = size;
  }
#endif
  return;
}

/**
 * calculate the tlv_flags for the tlv value (same length/value).
 * @param addr pointer to address object
 * @param first true if this is the first address of this message
 */
static void
_calculate_tlv_flags(struct rfc5444_writer_address *addr, bool first) {
  struct rfc5444_writer_addrtlv *tlv;

  if (first) {
    avl_for_each_element(&addr->_addrtlv_tree, tlv, addrtlv_node) {
      tlv->same_length = false;
      tlv->same_value = false;
    }
    return;
  }

  avl_for_each_element(&addr->_addrtlv_tree, tlv, addrtlv_node) {
    struct rfc5444_writer_addrtlv *prev = NULL;

    /* check if this is the first tlv of this type */
    if (avl_is_first(&tlv->tlvtype->_tlv_tree, &tlv->tlv_node)) {
      tlv->same_length = false;
      tlv->same_value = false;
      continue;
    }

    prev = avl_prev_element(tlv, tlv_node);

    if (tlv->address->index > prev->address->index + 1) {
      tlv->same_length = false;
      tlv->same_value = false;
      continue;
    }

    /* continous tlvs */
    tlv->same_length = tlv->length == prev->length;
    tlv->same_value = tlv->same_length &&
        (tlv->length == 0 || tlv->value == prev->value
            || memcmp(tlv->value, prev->value, tlv->length) == 0);
  }
}

/**
 * Update the address compression session with a new address.
 *
 * @param acs pointer to address compression session
 * @param msg pointer to message object
 * @param addr pointer to new address
 * @param same_prefixlen number of addresses (up to this) with the same
 *   prefix length
 * @param first true if this is the first address of the message
 * @return new number of messages with same prefix length
 */
static int
_compress_address(struct _rfc5444_internal_addr_compress_session *acs,
    struct rfc5444_writer_message *msg, struct rfc5444_writer_address *addr,
    int same_prefixlen, bool first) {
  struct rfc5444_writer_address *last_addr = NULL;
  struct rfc5444_writer_addrtlv *tlv;
  int i, common_head;
  const uint8_t *addrptr, *last_addrptr;
  uint8_t addrlen;
  bool special_prefixlen;

  addrlen = msg->addr_len;
  common_head = 0;
  special_prefixlen = netaddr_get_prefix_length(&addr->address) != addrlen * 8;

  addrptr = netaddr_get_binptr(&addr->address);

  /* add size for address part (and header if necessary) */
  if (!first) {
    /* get previous address */
    last_addr = oonf_list_prev_element(addr, _addr_node);

    /* remember how many entries with the same prefixlength we had */
    if (netaddr_get_prefix_length(&last_addr->address)
        == netaddr_get_prefix_length(&addr->address)) {
      same_prefixlen++;
    } else {
      same_prefixlen = 1;
    }

    /* add bytes to continue encodings with same prefix */
#if DO_ADDR_COMPRESSION == true
    last_addrptr = netaddr_get_binptr(&last_addr->address);
    for (common_head = 0; common_head < addrlen; common_head++) {
      if (last_addrptr[common_head] != addrptr[common_head]) {
        break;
      }
    }
#endif
    _close_addrblock(acs, msg, last_addr, common_head);
  }

  /* calculate new costs for next address including tlvs */
#if DO_ADDR_COMPRESSION == true
  for (i = 0; i < addrlen; i++) {
#else
  i = 0;
  {
#endif
    int new_cost = 0, continue_cost = 0;
    bool closed = false;

#if DO_ADDR_COMPRESSION == true
    closed = first || (i > common_head);
#else
    closed = true;
#endif
    /* cost of new address header */
    new_cost = 2 + (i > 0 ? 1 : 0) + msg->addr_len;
    if (special_prefixlen) {
      new_cost++;
    }

    if (!closed) {
      /* cost of continuing the last address header */
      continue_cost = msg->addr_len - i;
      if (acs[i].multiplen) {
        /* will stay multi_prefixlen */
        continue_cost++;
      }
      else if (same_prefixlen == 1) {
        /* will become multi_prefixlen */
        continue_cost += (acs[i].ptr->index - addr->index + 1);
        acs[i].multiplen = true;
      }
    }

    /* calculate costs for breaking/continuing tlv sequences */
    avl_for_each_element(&addr->_addrtlv_tree, tlv, addrtlv_node) {
      struct rfc5444_writer_tlvtype *tlvtype = tlv->tlvtype;
      int cost;

      cost = 2 + (tlv->tlvtype->exttype ? 1 : 0) + 2 + tlv->length;
      if (tlv->length > 255) {
        cost++;
      }
      if (tlv->length > 0) {
          cost++;
      }

      new_cost += cost;
      if (!tlv->same_length || closed) {
        /* this TLV does not continue over the border of an address block */
        continue_cost += cost;
        continue;
      }

      if (tlvtype->_tlvblock_multi[i]) {
        continue_cost += tlv->length;
      }
      else if (!tlv->same_value) {
        continue_cost += tlv->length * tlvtype->_tlvblock_count[i];
      }
    }

    if (closed || acs[i].total + continue_cost > acs[addrlen-1].total + new_cost) {
      /* new address block */
      acs[i].ptr = addr;
      acs[i].multiplen = false;

      acs[i].total = acs[addrlen-1].total;
      acs[i].current = new_cost;

      closed = true;
    }
    else {
      acs[i].current = continue_cost;
      closed = false;
    }

    /* update internal tlv calculation */
    avl_for_each_element(&addr->_addrtlv_tree, tlv, addrtlv_node) {
      struct rfc5444_writer_tlvtype *tlvtype = tlv->tlvtype;
      if (closed) {
        tlvtype->_tlvblock_count[i] = 1;
        tlvtype->_tlvblock_multi[i] = false;
      }
      else {
        tlvtype->_tlvblock_count[i]++;
        tlvtype->_tlvblock_multi[i] |= (!tlv->same_value);
      }
    }
  }
  return same_prefixlen;
}

/**
 * Write the address-TLVs of a specific type
 * @param addr_start first address for TLVs
 * @param addr_end last address for TLVs
 * @param tlvtype tlvtype to write into buffer
 * @param ptr target buffer pointer
 * @return modified target buffer pointer
 */
static uint8_t *
_write_tlvtype(struct rfc5444_writer_address *addr_start, struct rfc5444_writer_address *addr_end,
    struct rfc5444_writer_tlvtype *tlvtype, uint8_t *ptr) {
  struct rfc5444_writer_addrtlv *tlv_start, *tlv_end, *tlv;
  uint16_t total_len;
  uint8_t *flag;

  /* find first/last tlv for this address block */
  tlv_start = avl_find_ge_element(&tlvtype->_tlv_tree, &addr_start->_orig_index, tlv_start, tlv_node);

  while (tlv_start != NULL && tlv_start->address->_orig_index <= addr_end->_orig_index) {
    bool same_value;

    /* get end of local TLV-Block and value-mode */
    same_value = true;
    tlv_end = tlv_start;

    avl_for_element_to_last(&tlvtype->_tlv_tree, tlv_start, tlv, tlv_node) {
      if (tlv != tlv_start && tlv->address->index <= addr_end->index) {
        if (!tlv->same_length) {
          /* sequence of TLVs got interrupted */
          break;
        }
        tlv_end = tlv;
        same_value &= tlv->same_value;
      }
    }

    /* write tlv */
    *ptr++ = tlvtype->type;

    /* remember flag pointer */
    flag = ptr;
    *ptr++ = 0;
    if (tlvtype->exttype) {
      *flag |= RFC5444_TLV_FLAG_TYPEEXT;
      *ptr++ = tlvtype->exttype;
    }

    /* copy original length field */
    total_len = tlv_start->length;

    if (tlv_start->address == addr_start && tlv_end->address == addr_end) {
      /* no index necessary */
    } else if (tlv_start == tlv_end) {
      *flag |= RFC5444_TLV_FLAG_SINGLE_IDX;
      *ptr++ = tlv_start->address->index - addr_start->index;
    } else {
      *flag |= RFC5444_TLV_FLAG_MULTI_IDX;
      *ptr++ = tlv_start->address->index - addr_start->index;
      *ptr++ = tlv_end->address->index - addr_start->index;
    }

    /* length field is single_length*num for multivalue tlvs */
    if (!same_value) {
      total_len = total_len * ((tlv_end->address->index - tlv_start->address->index) + 1);
      *flag |= RFC5444_TLV_FLAG_MULTIVALUE;
    }


    /* write length field and corresponding flags */
    if (total_len > 255) {
      *flag |= RFC5444_TLV_FLAG_EXTVALUE;
      *ptr++ = total_len >> 8;
    }
    if (total_len > 0) {
      *flag |= RFC5444_TLV_FLAG_VALUE;
      *ptr++ = total_len & 255;
    }

    if (tlv_start->length > 0) {
      /* write value */
      if (same_value) {
        memcpy(ptr, tlv_start->value, tlv_start->length);
        ptr += tlv_start->length;
      } else {
        avl_for_element_range(tlv_start, tlv_end, tlv, tlv_node) {
          memcpy(ptr, tlv->value, tlv->length);
          ptr += tlv->length;
        }
      }
    }

    if (avl_is_last(&tlvtype->_tlv_tree, &tlv_end->tlv_node)) {
      tlv_start = NULL;
    } else {
      tlv_start = avl_next_element(tlv_end, tlv_node);
    }
  }
  return ptr;
}

/**
 * Write the address blocks to the message buffer.
 * @param writer pointer to writer context
 * @param msg pointer to message context
 * @param first_addr pointer to first address to be written
 * @param last_addr pointer to last address to be written
 */
static void
_write_addresses(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg,
    struct rfc5444_writer_address *first_addr, struct rfc5444_writer_address *last_addr) {
  struct rfc5444_writer_address *addr_start, *addr_end, *addr;
  struct rfc5444_writer_tlvtype *tlvtype;
  const uint8_t *addr_start_ptr, *addr_ptr;
  uint8_t *start, *ptr, *flag, *tlvblock_length;

  assert(first_addr->_block_end);

  addr_start = first_addr;
  ptr = &writer->_msg.buffer[writer->_msg.header + writer->_msg.added + writer->_msg.allocated];

  /* remember start */
  start = ptr;

  /* loop through address blocks */
  do {
    uint8_t head_len = 0, tail_len = 0, mid_len = 0;
#if DO_ADDR_COMPRESSION == true
    bool zero_tail = false;
#endif

    addr_start_ptr = netaddr_get_binptr(&addr_start->address);
    addr_end = addr_start->_block_end;

#if DO_ADDR_COMPRESSION == true
    if (addr_start != addr_end) {
      /* only use head/tail for address blocks with multiple addresses */
      int tail;
      head_len = addr_start->_block_headlen;
      tail_len = msg->addr_len - head_len - 1;

      /* calculate tail length and netmask length */
      oonf_list_for_element_range(addr_start, addr_end, addr, _addr_node) {
        addr_ptr = netaddr_get_binptr(&addr->address);

        /* stop if no tail is left */
        if (tail_len == 0) {
          break;
        }

        for (tail = 1; tail <= tail_len; tail++) {
          if (addr_start_ptr[msg->addr_len - tail] != addr_ptr[msg->addr_len - tail]) {
            tail_len = tail - 1;
            break;
          }
        }
      }

      zero_tail = tail_len > 0;
      for (tail = 0; zero_tail && tail < tail_len; tail++) {
        if (addr_start_ptr[msg->addr_len - tail - 1] != 0) {
          zero_tail = false;
        }
      }
    }
#endif
    mid_len = msg->addr_len - head_len - tail_len;

    /* write addrblock header */
    *ptr++ = addr_end->index - addr_start->index + 1;

    /* erase flag */
    flag = ptr;
    *ptr++ = 0;

#if DO_ADDR_COMPRESSION == true
    /* write head */
    if (head_len) {
      *flag |= RFC5444_ADDR_FLAG_HEAD;
      *ptr++ = head_len;
      memcpy(ptr, addr_start_ptr, head_len);
      ptr += head_len;
    }

    /* write tail */
    if (tail_len > 0) {
      *ptr++ = tail_len;
      if (zero_tail) {
        *flag |= RFC5444_ADDR_FLAG_ZEROTAIL;
      } else {
        *flag |= RFC5444_ADDR_FLAG_FULLTAIL;
        memcpy(ptr, &addr_start_ptr[msg->addr_len - tail_len], tail_len);
        ptr += tail_len;
      }
    }
#endif
    /* loop through addresses in block for MID part */
    oonf_list_for_element_range(addr_start, addr_end, addr, _addr_node) {
      addr_ptr = netaddr_get_binptr(&addr->address);
      memcpy(ptr, &addr_ptr[head_len], mid_len);
      ptr += mid_len;
    }

    /* loop through addresses in block for prefixlen part */
    if (addr_start->_block_multiple_prefixlen) {
      /* multiple prefixlen */
      *flag |= RFC5444_ADDR_FLAG_MULTIPLEN;
      oonf_list_for_element_range(addr_start, addr_end, addr, _addr_node) {
        *ptr++ = netaddr_get_prefix_length(&addr->address);
      }
    } else if (netaddr_get_prefix_length(&addr_start->address)!= msg->addr_len * 8) {
      /* single prefixlen */
      *flag |= RFC5444_ADDR_FLAG_SINGLEPLEN;
      *ptr++ = netaddr_get_prefix_length(&addr_start->address);
    }

    /* remember pointer for tlvblock length */
    tlvblock_length = ptr;
    ptr += 2;

    /* loop through all message specific address-tlv types */
    oonf_list_for_each_element(&msg->_msgspecific_tlvtype_head, tlvtype, _tlvtype_node) {
      ptr = _write_tlvtype(addr_start, addr_end, tlvtype, ptr);
    }

    /* look through  all generic address-tlv types */
    oonf_list_for_each_element(&writer->_addr_tlvtype_head, tlvtype, _tlvtype_node) {
      ptr = _write_tlvtype(addr_start, addr_end, tlvtype, ptr);
    }

    tlvblock_length[0] = (ptr - tlvblock_length - 2) >> 8;
    tlvblock_length[1] = (ptr - tlvblock_length - 2) & 255;
    addr_start = oonf_list_next_element(addr_end, _addr_node);
  } while (addr_end != last_addr);

  /* store size of address(tlv) data */
  msg->_bin_addr_size = ptr - start;
}

/**
 * Write header of message including mandatory tlvblock length field.
 * @param writer pointer to writer context
 * @param _msg pointer to message object
 */
static void
_write_msgheader(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg) {
  uint8_t *ptr, *flags;
  uint16_t total_size;
  ptr = writer->_msg.buffer;

  /* type */
  *ptr++ = msg->type;

  /* flags & addrlen */
  flags = ptr;
  *ptr++ = msg->addr_len - 1;

  /* size */
  total_size = writer->_msg.header + writer->_msg.added + writer->_msg.set + msg->_bin_addr_size;
  *ptr++ = total_size >> 8;
  *ptr++ = total_size & 255;

  if (msg->has_origaddr) {
    *flags |= RFC5444_MSG_FLAG_ORIGINATOR;
    memcpy(ptr, msg->orig_addr, msg->addr_len);
    ptr += msg->addr_len;
  }
  if (msg->has_hoplimit) {
    *flags |= RFC5444_MSG_FLAG_HOPLIMIT;
    *ptr++ = msg->hoplimit;
  }
  if (msg->has_hopcount) {
    *flags |= RFC5444_MSG_FLAG_HOPCOUNT;
    *ptr++ = msg->hopcount;
  }
  if (msg->has_seqno) {
    *flags |= RFC5444_MSG_FLAG_SEQNO;
    *ptr++ = msg->seqno >> 8;
    *ptr++ = msg->seqno & 255;
  }

  /* write tlv-block size */
  total_size = writer->_msg.added + writer->_msg.set;
  *ptr++ = total_size >> 8;
  *ptr++ = total_size & 255;
}

/**
 * Finalize a message fragment, copy it into the packet buffer and
 * cleanup message internal data.
 * @param writer pointer to writer context
 * @param _msg pointer to message object
 * @param first pointer to first address of this fragment
 * @param last pointer to last address of this fragment
 * @param not_fragmented true if this is the only fragment of this message
 * @param useIf pointer to callback for selecting outgoing _targets
 */
static void
_finalize_message_fragment(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg,
    struct rfc5444_writer_address *first, struct rfc5444_writer_address *last, bool not_fragmented,
    rfc5444_writer_targetselector useIf, void *param) {
  struct rfc5444_writer_content_provider *prv;
  struct rfc5444_writer_target *interface;
  uint8_t *ptr;
  size_t len;

  /* reset optional tlv length */
  writer->_msg.set = 0;

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_FINISH_MSGTLV;
#endif

  /* inform message providers */
  avl_for_each_element_reverse(&msg->_provider_tree, prv, _provider_node) {
    if (prv->finishMessageTLVs) {
      prv->finishMessageTLVs(writer, first, last, not_fragmented);
    }
  }

  if (first != NULL && last != NULL) {
    _write_addresses(writer, msg, first, last);
  }

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_FINISH_HEADER;
#endif

  /* inform message creator */
  if (msg->finishMessageHeader) {
    msg->finishMessageHeader(writer, msg, first, last, not_fragmented);
  }

  /* write header */
  _write_msgheader(writer, msg);

#if WRITER_STATE_MACHINE == true
  writer->_state = RFC5444_WRITER_NONE;
#endif

  /* precalculate number of fixed bytes of message header */
  len = writer->_msg.header + writer->_msg.added;

  oonf_list_for_each_element(&writer->_targets, interface, _target_node) {
    /* do we need to handle this interface ? */
    if (!useIf(writer, interface, param)) {
      continue;
    }

    /* calculate total size of packet and message, see if it fits into the current packet */
    if (interface->_pkt.header + interface->_pkt.added + interface->_pkt.set + interface->_bin_msgs_size
        + writer->_msg.header + writer->_msg.added + writer->_msg.set + msg->_bin_addr_size
        > interface->_pkt.max) {

      /* flush the old packet */
      rfc5444_writer_flush(writer, interface, false);

      /* begin a new one */
      _rfc5444_writer_begin_packet(writer, interface);
    }


    /* get pointer to end of _pkt buffer */
    ptr = &interface->_pkt.buffer[interface->_pkt.header + interface->_pkt.added
                                 + interface->_pkt.allocated + interface->_bin_msgs_size];

    /* copy message header and message tlvs into packet buffer */
    memcpy(ptr, writer->_msg.buffer, len + writer->_msg.set);

    /* copy address blocks and address tlvs into packet buffer */
    ptr += len + writer->_msg.set;
    memcpy(ptr, &writer->_msg.buffer[len + writer->_msg.allocated], msg->_bin_addr_size);

    /* increase byte count of packet */
    interface->_bin_msgs_size += len + writer->_msg.set + msg->_bin_addr_size;
  }

  /* clear length value of message address size */
  msg->_bin_addr_size = 0;

  /* reset message tlv variables */
  writer->_msg.set = 0;

  /* clear message buffer */
#if DEBUG_CLEANUP == true
  memset(&writer->_msg.buffer[len], 0, writer->_msg.max - len);
#endif
}
