#ifndef PRIV_AODVV2_BUFFER_H
#define PRIV_AODVV2_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#include "net/aodvv2.h"
#include "net/gnrc/pktbuf.h"
#include "net/gnrc/ipv6.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   AODVv2 buffered packet
 *
 * This holds a pointer to a packet which was buffered by us, this means, that
 * a route hasn't found (yet) and we have saved it to send it later when a route
 * is found.
 *
 * The @ref _aodvv2_buffered_pkt_t::pkt field contains the IPv6 header with the
 * destination information.
 *
 * TODO(jeandudey): add timeouts for packets that have not found a route
 */
typedef struct {
    gnrc_pktsnip_t *pkt; /**< Packet */
    bool used; /**< Is this entry used */
} _aodvv2_buffered_pkt_t;

/**
 * @brief   Initialize AODVv2 packet buffer.
 */
void _aodvv2_buffer_init(void);

/**
 * @brief   Add a packet to the packet buffer
 *
 * @pre @p pkt != NULL
 *
 * @param[in]  pkt The packet to buffer.
 *
 * @return 0 on success.
 * @return -ENOMEM if no memory available.
 */
int _aodvv2_buffer_pkt_add(gnrc_pktsnip_t *pkt);

/**
 * @brief   Dispatch all buffered packets that match `targ_prefix` destination.
 *
 * @param[in]  targ_prefix Target prefix.
 * @param[in]  pfx_len     Length of the prefix in bits. 128 for full address.
 */
void _aodvv2_buffer_dispatch(const ipv6_addr_t *targ_prefix, uint8_t pfx_len);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_BUFFER_H */
