#ifndef PRIV_AODVV2_LRS_H
#define PRIV_AODVV2_LRS_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#include "net/aodvv2/msg.h"
#include "net/aodvv2/seqnum.h"
#include "net/ipv6/addr.h"
#include "timex.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @{
 * @name Local Route state
 * @see [draft-perkins-manet-aodvv2-03, section 4.5]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.5)
 */
/**
 * @brief   A route obtained from a Route Request message, which has not yet
 *          been confirmed as bidirectional.
 *
 * It WILL NOT be stored in the NIB to forward general data-plane traffic, but
 * it can be used to transmit RREP packets along with a request for
 * bidirectional link verification.  An Unconfirmed route is not otherwise
 * considered a valid route. This state is only used for routes obtained
 * through RREQ messages.
 */
#define AODVV2_ROUTE_STATE_UNCONFIRMED (0)
/**
 * @brief  A route that has been confirmed to be bidirectional, but has not
 *         been used in the last `ACTIVE_INTERVAL`.
 *
 * It can be used for forwarding IP packets, and therefore it is considered a
 * valid route.
 */
#define AODVV2_ROUTE_STATE_IDLE        (1)
/**
 * @brief   A valid route that has been used for forwarding IP packets during
 *          the last ACTIVE_INTERVAL.
 */
#define AODVV2_ROUTE_STATE_ACTIVE      (2)
/**
 * @brief   A route that has expired or has broken.
 *
 * It WILL NOT be used for forwarding IP packets.  Invalid routes contain the
 * destination's sequence number, which may be useful when assessing freshness
 * of incoming routing information.
 */
#define AODVV2_ROUTE_STATE_INVALID     (3)
/** @} */

/**
 * @brief   Convert a Local Route state to an string.
 */
static inline const char *aodvv2_lrs_state_to_str(uint8_t state)
{
    switch (state) {
        case AODVV2_ROUTE_STATE_UNCONFIRMED:
            return "UNCONFIRMED";
        case AODVV2_ROUTE_STATE_IDLE:
            return "IDLE";
        case AODVV2_ROUTE_STATE_ACTIVE:
            return "ACTIVE";
        case AODVV2_ROUTE_STATE_INVALID:
            return "INVALID";
        default:
            return "";
    }

    return "";
}

/**
 * @brief   A Local Route
 *
 * @see [draft-perkins-manet-aodvv2-03, section 4.5]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-4.5)
 *
 * TODO(jeandudey): add precursors optional feature for enhanced route error
 * reporting.
 */
typedef struct {
    /**
     * @brief   Destination address.
     *
     * Combined with @ref _aodvv2_local_route_t::pfx_len, describes the set of
     * destination addresses for which this route enables forwarding.
     */
    ipv6_addr_t addr;
    /**
     * @brief   Prefix length.
     *
     * The prefix length, in bits, associated with
     * @ref _aodvv2_local_route_t::addr.
     */
    uint8_t pfx_len;
    /**
     * @brief   Sequence number.
     *
     * Associated with @ref _aodvv2_local_route_t::addr, obtained from the last
     * route message that successfully updated this entry.
     */
    aodvv2_seqnum_t seqnum;
    /**
     * @brief   The source IP address of the IP packet containing the AODVv2
     *          message advertising the route to
     *          @ref _aodvv2_local_route_t::addr.
     *
     * IP address of the AODVv2 router used for the next hop on the path toward
     * @ref _aodvv2_local_route_tLocalRoute.Address.
     */
    ipv6_addr_t next_hop;
    /**
     * @brief   The interface used to send IP packets toward LocalRoute.Address.
     */
    uint16_t iface;
    /**
     * @brief   Last time this route was used.
     *
     * If this route is installed in the NIB, the time it was last used to
     * forward an IP packet.  If not, the time at which the the Local Route was
     * created.
     */
    timex_t last_used;
    /**
     * @brief   The time @ref _aodvv2_local_route_t::seqnum was last updated.
     */
    timex_t last_seqnum_update;
    /**
     * @brief   The type of metric associated with this route.
     */
    uint8_t metric_type;
    /**
     * @brief   The cost of the route toward @ref aodvv2_local_route_t::addr
     *          expressed in units consistent with
     *          @ref aodvv2_local_route_t::metric_type.
     */
    uint8_t metric;
    /**
     * @brief   If not unspecified, the IP address of the router that originated
     *          the Sequence Number for this route.
     */
    ipv6_addr_t seqnortr;
    /**
     * @brief   The last known state (Unconfirmed, Idle, Active, or Invalid) of
     *          the route.
     */
    uint8_t state;
    /**
     * @brief   Is this entry used?
     */
    bool used;
} _aodvv2_local_route_t;

/**
 * @brife   Acquire LRS lock
 */
void _aodvv2_lrs_acquire(void);

/**
 * @brief   Release LRS lock
 */
void _aodvv2_lrs_release(void);

/**
 * @brief   Initialize LRS.
 */
void _aodvv2_lrs_init(void);

/**
 * @brief   Process an incoming Route Message (`RteMsg`).
 *
 * @param[in]  rtemsg Route Message (RREP or RREQ).
 * @param[in]  sender IPv6 address of the neighboring router that sent the
 *                    RteMsg.
 * @param[in]  iface  Network interface where we received the RteMsg.
 *
 * @return 0 on success.
 * @return -EINVAL on invalid parameters.
 */
int _aodvv2_lrs_process(aodvv2_message_t *rtemsg, const ipv6_addr_t *sender,
                        const uint16_t iface);

/**
 * @brief   Find a route towards `dst`.
 *
 * @param[in]  dst Destination IPv6 address.
 *
 * @return Pointer to @ref _aodvv2_local_route_t if found.
 * @return NULL if not found.
 */
_aodvv2_local_route_t *_aodvv2_lrs_find(const ipv6_addr_t *dst);

/**
 * @brief   Allocate new LRS entry.
 *
 * @param[in]  addr        Route destination address.
 * @param[in]  pfx_len     `addr` prefix length in bits.
 * @param[in]  metric_type Metric type.
 *
 * @return Pointer to @ref _aodvv2_local_route_t on success.
 * @return NULL if no space available.
 */
_aodvv2_local_route_t *_aodvv2_lrs_alloc(const ipv6_addr_t *addr,
                                         uint8_t pfx_len,
                                         uint8_t metric_type);

/**
 * @brief   Check if the given Local Route matches the Advertised Route.
 *
 * @param[in]  lr     The Local Route
 * @param[in]  advrte Advertised Route.
 */
bool _aodvv2_lrs_match(_aodvv2_local_route_t *lr, aodvv2_message_t *advrte);

static inline void _advrte_get_metric(const aodvv2_message_t *advrte,
                                      uint8_t *metric_type,
                                      uint8_t *metric)
{
    assert(advrte != NULL);
    assert(metric_type != NULL || metric != NULL);
    assert(advrte->type == AODVV2_MSGTYPE_RREQ ||
           advrte->type == AODVV2_MSGTYPE_RREP);

    uint8_t type = 0;
    uint8_t m = 0;
    if (advrte->type == AODVV2_MSGTYPE_RREQ) {
        type = advrte->rreq.metric_type;
        m = advrte->rreq.orig_metric;
    } else if (advrte->type == AODVV2_MSGTYPE_RREP) {
        type = advrte->rrep.metric_type;
        m = advrte->rrep.targ_metric;
    }

    if (metric_type) {
        *metric_type = type;
    }
    if (metric) {
        *metric = m;
    }
}

static inline void _advrte_get_addr(const aodvv2_message_t *advrte,
                                    const ipv6_addr_t **addr,
                                    uint8_t *pfx_len)
{
    assert(advrte != NULL);
    assert(addr != NULL || pfx_len != NULL);
    assert(advrte->type == AODVV2_MSGTYPE_RREQ ||
           advrte->type == AODVV2_MSGTYPE_RREP);

    const ipv6_addr_t *a = NULL;
    uint8_t len = 0;
    if (advrte->type == AODVV2_MSGTYPE_RREQ) {
        a = &advrte->rreq.orig_prefix;
        len = advrte->rreq.orig_pfx_len;
    }
    else if (advrte->type == AODVV2_MSGTYPE_RREP) {
        a = &advrte->rrep.targ_prefix;
        len = advrte->rrep.targ_pfx_len;
    }

    if (addr) {
        *addr = a;
    }
    if (pfx_len) {
        *pfx_len = len > 128 ? 128 : len;
    }
}

static inline void _advrte_get_seqnortr(const aodvv2_message_t *advrte,
                                        const ipv6_addr_t **seqnortr)
{
    assert(advrte != NULL);
    assert(seqnortr != NULL);
    assert(advrte->type == AODVV2_MSGTYPE_RREQ ||
           advrte->type == AODVV2_MSGTYPE_RREP);

    const ipv6_addr_t *n = NULL;
    if (advrte->type == AODVV2_MSGTYPE_RREQ) {
        n = &advrte->rreq.seqnortr;
    }
    else if (advrte->type == AODVV2_MSGTYPE_RREP) {
        n = &advrte->rrep.seqnortr;
    }

    if (seqnortr) {
        *seqnortr = n;
    }
}

static inline void _advrte_get_seqnum(const aodvv2_message_t *advrte,
                                      aodvv2_seqnum_t *seqnum)
{
    assert(advrte != NULL);
    assert(seqnum != NULL);
    assert(advrte->type == AODVV2_MSGTYPE_RREQ ||
           advrte->type == AODVV2_MSGTYPE_RREP);

    aodvv2_seqnum_t s = 0;
    if (advrte->type == AODVV2_MSGTYPE_RREQ) {
        s = advrte->rreq.orig_seqnum;
    }
    else if (advrte->type == AODVV2_MSGTYPE_RREP) {
        s = advrte->rrep.targ_seqnum;
    }

    if (seqnum) {
        *seqnum = s;
    }
}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_LRS_H */
