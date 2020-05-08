
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

#ifndef NETADDR_H_
#define NETADDR_H_


#include <arpa/inet.h>

#include <assert.h>
#include <string.h>

#include "common/autobuf.h"

/* Pseude address families for mac/eui64 */
enum {
  AF_MAC48 = AF_MAX + 1,
  AF_EUI64 = AF_MAX + 2,
};

/* maximum number of octets for address */
enum { NETADDR_MAX_LENGTH = 16 };

/* text names for defining netaddr prefixes */
#define NETADDR_STR_ANY4       "any4"
#define NETADDR_STR_ANY6       "any6"
#define NETADDR_STR_LINKLOCAL4 "linklocal4"
#define NETADDR_STR_LINKLOCAL6 "linklocal6"
#define NETADDR_STR_ULA        "ula"

/**
 * Representation of an address including address type
 * At the moment we support AF_INET, AF_INET6 and AF_MAC48
 */
struct netaddr {
  /* 16 bytes of memory for address */
  uint8_t _addr[NETADDR_MAX_LENGTH];

  /* address type */
  uint8_t _type;

  /* address prefix length */
  uint8_t _prefix_len;
};

/**
 * Buffer for writing string representation of netaddr
 * and netaddr_socket objects
 */
struct netaddr_str {
  char buf[INET6_ADDRSTRLEN+16];
};

/**
 * Maximum text length of netaddr_to_string
 *
 * INET_ADDRSTRLEN and INET6_ADDRSTRLEN have been defined
 * in netinet/in.h, which has been included by this file
 */
enum {
  MAC48_ADDRSTRLEN = 18,
  MAC48_PREFIXSTRLEN = MAC48_ADDRSTRLEN + 3,
  EUI64_ADDRSTRLEN = 24,
  EUI64_PREFIXSTRLEN = EUI64_ADDRSTRLEN + 3,
  INET_PREFIXSTRLEN = INET_ADDRSTRLEN + 3,
  INET6_PREFIXSTRLEN = INET6_ADDRSTRLEN + 4,
};

int netaddr_from_binary_prefix(struct netaddr *dst,
    const void *binary, size_t len, uint8_t addr_type, uint8_t prefix_len);
int netaddr_to_binary(void *dst, const struct netaddr *src, size_t len);
int netaddr_to_autobuf(struct autobuf *, const struct netaddr *src);
int netaddr_create_host_bin(struct netaddr *host, const struct netaddr *netmask,
    const void *number, size_t num_length);
const char *netaddr_to_prefixstring(
    struct netaddr_str *dst, const struct netaddr *src, bool forceprefix);

bool netaddr_isequal_binary(const struct netaddr *addr,
    const void *bin, size_t len, uint16_t af, uint8_t prefix_len);
bool netaddr_is_in_subnet(const struct netaddr *subnet,
    const struct netaddr *addr);
bool netaddr_binary_is_in_subnet(const struct netaddr *subnet,
    const void *bin, size_t len, uint8_t af_family);

uint8_t netaddr_get_af_maxprefix(const uint32_t);

int netaddr_avlcmp(const void *, const void *);

/**
 * Sets the address type of a netaddr object to AF_UNSPEC
 * @param addr netaddr object
 */
static inline void
netaddr_invalidate(struct netaddr *addr) {
  memset(addr, 0, sizeof(*addr));
}
/**
 * Calculates the maximum prefix length of an address type
 * @param addr netaddr object
 * @return prefix length, 0 if unknown address family
 */
static inline uint8_t
netaddr_get_maxprefix(const struct netaddr *addr) {
  return netaddr_get_af_maxprefix(addr->_type);
}

/**
 * Converts a netaddr object into a string.
 * Prefix will be added if necessary.
 * @param dst target buffer
 * @param src netaddr source
 * @return pointer to target buffer, NULL if an error happened
 */
static inline const char *
netaddr_to_string(struct netaddr_str *dst, const struct netaddr *src) {
  return netaddr_to_prefixstring(dst, src, false);
}

/**
 * Creates a host address from a netmask and a host number part. This function
 * will copy the netmask and then overwrite the bits after the prefix length
 * with the one from the host number.
 * @param host target buffer
 * @param netmask prefix of result
 * @param host_number postfix of result
 * @return -1 if an error happened, 0 otherwise
 */
static inline int
netaddr_create_host(struct netaddr *host, const struct netaddr *netmask,
    const struct netaddr *host_number) {
  return netaddr_create_host_bin(host, netmask, host_number->_addr,
      netaddr_get_maxprefix(host_number));
}

/**
 * Extract an IPv4 address from an IPv6 IPv4-compatible address
 * @param dst target IPv4 address
 * @param src source IPv6 address
 */
static inline void
netaddr_extract_ipv4_compatible(struct netaddr *dst, const struct netaddr *src) {
  memcpy(&dst->_addr[0], &src->_addr[12], 4);
  memset(&dst->_addr[4], 0, 12);
  dst->_type = AF_INET;
  dst->_prefix_len = src->_prefix_len - 96;
}

/**
 * Read the binary representation of an address into a netaddr object
 * @param dst pointer to netaddr object
 * @param binary source pointer
 * @param len length of source buffer
 * @param addr_type address type of source,
 *   0 for autodetection based on length
 * @return 0 if successful read binary data, -1 otherwise
 */
static inline int
netaddr_from_binary(struct netaddr *dst, const void *binary,
    size_t len, uint8_t addr_type) {
  return netaddr_from_binary_prefix(dst, binary, len, addr_type, 255);
}

/**
 * Compares two addresses.
 * Address type will be compared last.
 * @param a1 address 1
 * @param a2 address 2
 * @return >0 if a1>a2, <0 if a1<a2, 0 otherwise
 */
static inline int
netaddr_cmp(const struct netaddr *a1, const struct netaddr *a2) {
  return memcmp(a1, a2, sizeof(*a1));
}

/**
 * @param n pointer to netaddr
 * @return pointer to start of binary address
 */
static inline const void *
netaddr_get_binptr(const struct netaddr *n) {
  return &n->_addr[0];
}

/**
 * @param n pointer to netaddr
 * @return number of bytes of binary address
 */
static inline size_t
netaddr_get_binlength(const struct netaddr *n) {
  return netaddr_get_maxprefix(n) >> 3;
}

/**
 * @param n pointer to netaddr
 * @return address family
 */
static inline uint8_t
netaddr_get_address_family(const struct netaddr *n) {
  return n->_type;
}

/**
 * @param n pointer to netaddr
 * @return prefix length
 */
static inline uint8_t
netaddr_get_prefix_length(const struct netaddr *n) {
  return n->_prefix_len;
}

/**
 * @param n pointer to netaddr
 * @param prefix_length new prefix length
 */
static inline void
netaddr_set_prefix_length(struct netaddr *n, uint8_t prefix_len) {
  n->_prefix_len = prefix_len;
}

#endif /* NETADDR_H_ */
