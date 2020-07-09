#ifndef PRIV_AODVV2_NEIGH_H
#define PRIV_AODVV2_NEIGH_H

#include <stdint.h>
#include <stdbool.h>

#include "net/ipv6/addr.h"
#include "timex.h"

#include "net/aodvv2/seqnum.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @{
 * @name    Neighbor State
 * @see [draft-perkins-manet-aodvv2-03, section 4.3]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.3)
 */
#define AODVV2_NEIGH_STATE_BLACKLISTED (0)
#define AODDV2_NEIGH_STATE_CONFIRMED   (1)
#define AODVV2_NEIGH_STATE_HEARD       (2)
/** @} */

/**
 * @brief    A Neighbor Set entry
 *
 * This structure contains information about a neighboring AODVv2 router which
 * we have heard of. This is mainly used to confirm bidirectionality over a link
 * to the given neighbor.
 *
 * @see [draft-perkins-manet-aodvv2-03, section 4.3]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.3)
 */
typedef struct {
    /**
     * @brief   IP address of the neighboring router
     */
    ipv6_addr_t addr;
    /**
     * @brief   Indicates whether the link to the neighbor is bidirectional.
     *
     * There are three possible states:
     *
     *  - (CONFIRMED)[@ref AODDV2_NEIGH_STATE_CONFIRMED]: indicates that the
     *  link to the neighbor has been confirmed as bidirectional.
     *
     *  - (HEARD)[@ref AODVV2_NEIGH_STATE_HEARD]: we've only heard of this
     *  neighbor, althought, we haven't confirmed bidirectionality.
     *
     *  - (BLACLISTED)[@ref AODVV2_NEIGH_STATE_BLACKLISTED]: indicates that the
     *  link to the neighbor is being treated as uni-directional.
     */
    uint8_t state;
    /**
     * @brief   Indicates the time at which the
     *          @ref _aodvv2_neigh_entry_t::state should be updated.
     *
     *  - If it's "blacklisted", will be reverted to "heard". The timeout is
     *  calculated at the time the neighbor is blacklisted and is equal to:
     *
     *  `CurrentTime + MAX_BLACKLIST_TIME`
     *
     *  - If it's "heard", and a RREP_Ack has been requested from the neighbor, it
     *  indicates the time at which Neighbor.State will be set to "blacklisted",
     *  if an RREP_Ack has not been received.
     *
     *  - If it's "heard", and no RREP_Ack has been requested, or if "confirmed"
     *  this time is set to null (0.0 s) to mean infinity.
     */
    timex_t timeout;
    /**
     * @brief   Network interface on which the link to the neighbor was
     *          established
     */
    uint16_t iface;
    /**
     * @brief   The next sequence number to use for the TIMESTAMP value in an
     *          RREP_Ack request, in order to detect replay of an RREP_Ack
     *          response.
     *
     * This is initialized to a random value.
     *
     * @see [draft-perkins-manet-aodvv2-03, section 11.3]
     *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-11.3)
     */
    aodvv2_seqnum_t ackseqnum;
    /**
     * @brief   The last heard sequence number used as the TIMESTAMP value in a
     *          RERR received from this neighbor, saved in order to detect
     *          replay of a RERR message.
     *
     * This is initialized to zero (0).
     *
     * @see [draft-perkins-manet-aodvv2-03, section 11.4]
     *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-11.4)
     */
    aodvv2_seqnum_t heard_rerr_seqnum;
    /**
     * @brief   Is this entry used?
     */
    bool used;
} _aodvv2_neigh_t;

/**
 * @brief   Initialize neighbor set.
 */
void _aodvv2_neigh_init(void);

/**
 * @brief   Own neighbors lock for a moment
 *
 * @note Release with @ref _aodv2_neigh_release
 */
void _aodvv2_neigh_acquire(void);

/**
 * @brief   Release neighbors lock
 */
void _aodvv2_neigh_release(void);

/**
 * @brief    Allocate a new neighbor.
 *
 * @note If the neighbor exists, a pointer to the existing neighbor will be
 * returned.
 *
 * @param[in]  addr  Node IPv6 Link-Local address.
 * @param[in]  iface Network interface.
 *
 * @return Pointer to @ref _aodvv2_neight_t
 * @return NULL if no memory available for the new neighbor.
 */
_aodvv2_neigh_t *_aodvv2_neigh_alloc(const ipv6_addr_t *addr, uint16_t iface);

/**
 * @brief   Get a neighbor.
 *
 * Returns a neighbor if it exists, and if it doesn't exists, a new one will be
 * created.
 *
 * @param[in]  addr  Node IPv6 Link-Local address.
 * @param[in]  iface Network interface.
 *
 * @return Pointer to @ref _aodvv2_neigh_t.
 * @return NULL if no memory available for the new neighbor.
 */
_aodvv2_neigh_t *_aodvv2_neigh_get(const ipv6_addr_t *addr, uint16_t iface);

/**
 * @brief   Update neighbor state depending on it's
 *          [state]((@ref _aodvv2_neigh_t::state) and it's
 *          [timeout](@ref _aodvv2_neigh_t::timeout) value.
 */
void _aodvv2_neigh_upd_state(_aodvv2_neigh_t *neigh);

/**
 * @brief   Set this `neigh` to #AODVV2_ROUTE_STATE_HEARD and update it's
 *          timeout information.
 *
 * @note If reqack is true, a RREP_Ack request will be sent to the neighbor
 * to confirm it's bidirectinality, if it doesn't answer, it will become
 * blacklisted after `CurrentTime + RREP_Ack_TIMEOUT`.
 *
 * @param[in] reqack Request a RREP_Ack to this neighbor?
 */
void _aodvv2_neigh_set_heard(_aodvv2_neigh_t *neigh, bool reqack);

/**
 * @brief   Request an RREP_Ack for the given neighbor.
 */
void _aodvv2_req_ack(_aodvv2_neigh_t *neigh);

/**
 * @brief   Is `t` zero (0.0 s)?
 *
 * @param[in] t Time to compare.
 */
static inline bool _timex_is_zero(timex_t t)
{
    return timex_cmp(t, timex_set(0, 0));
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_INTERNAL_H */
