#ifndef PRIV_AODVV2_H
#define PRIV_AODVV2_H

#include <stdlib.h>
#include <stdint.h>

#include "net/aodvv2/msg.h"
#include "priority_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @{
 * @name AODVv2 message priority
 * @see [draft-perkins-manet-aodvv2-03, section 6.5]
 *      (https://tools.ietf.org/html/draft-perkins-manet-aodvv2-03#section-6.5)
 */
/**
 * @brief   RREP_Ack message priority.
 *
 * @note Highest priority.
 *
 * This allows links between routers to be confirmed as bidirectional and avoids
 * undesired blacklisting of next hop routers.
 */
#define _AODVV2_MSG_PRIO_RREP_ACK           (5)
/**
 * @brief   RERR message priority for undeliverable IP packets.
 *
 * @note Second priority
 *
 * This avoids repeated forwarding of packets over broken routes that are still
 * in use by other routers.
 */
#define _AODVV2_MSG_PRIO_RERR_UNDELIVERABLE (4)
/**
 * @brief   RREP message priority.
 *
 * @note Third priority.
 *
 * This in order that RREQs do not time out.
 */
#define _AODVV2_MSG_PRIO_RREP               (3)
/**
 * @brief   RREQ message priority.
 *
 * @note Foruth priority.
 */
#define _AODVV2_MSG_PRIO_RREQ               (2)
/**
 * @brief   RERR message priority for newly invalidated routes.
 *
 * @note Fifth priority.
 */
#define _AODVV2_MSG_PRIO_RERR_INVALIDATED   (1)
/**
 * @brief   RERR message priority in response to RREP messages which cannot be
 *          forwarded.
 *
 * @note Lowest priority.
 *
 * In this case the route request will be retried at a later point.
 */

#define _AODVV2_MSG_PRIO_RERR_FORWARD_RREP  (0)
/** @} */

/**
 * @brief   AODVv2 message queue entry node
 */
typedef struct _priority_msg_node {
    priority_queue_node_t node;      /**< Node */
    aodvv2_message_t msg;            /**< AODVv2 message */
    ipv6_addr_t addr;                /**< Next hop address */
    uint16_t iface;                  /**< Network interface */
} _priority_msgqueue_node_t;

/**
 * @brief   Initialize message queue node
 *
 * @param[in]  node     The node to initialize.
 * @param[in]  priority Priority.
 * @param[in]  msg      AODVv2 message.
 * @param[in]  addr     Next hop address.
 * @param[in]  iface    Network interface
 */
static inline void _priority_msgqueue_node_init(_priority_msgqueue_node_t *node,
                                                uint32_t priority,
                                                const aodvv2_message_t *msg,
                                                const ipv6_addr_t *addr,
                                                uint16_t iface)
{
    /* mark as used */
    node->node.data = 1;
    node->node.next = NULL;
    node->node.priority = priority;

    memcpy(&node->msg, msg, sizeof(aodvv2_message_t));
    memcpy(&node->addr, addr, sizeof(ipv6_addr_t));
    node->iface = iface;
}

/**
 * @brief   Send an AODVv2 message.
 */
#define _AODVV2_MSG_TYPE_SND (0x8140)

/**
 * @brief   AODVv2 IPC message
 */
typedef struct {
    unsigned int prio;    /**< Priority */
    aodvv2_message_t msg; /**< AODVv2 message */
    ipv6_addr_t dst;      /**< Destination */
    uint16_t iface;       /**< Network interface */
} _aodvv2_ipc_msg_t;

/**
 * @brief   Send an AODVv2 message to the AODVv2 thread for it to be scheduled.
 *
 * @note The @p type parameter determines the priority.
 *
 * @param[in]  prio    Message priority.
 * @param[in]  message AODVv2 message.
 * @param[in]  dst     Destination address.
 * @param[in]  iface   Network interface.
 *
 * @return 1, if sending was successful.
 * @return -1, on error.
 */
int _aodvv2_send_message(uint16_t prio, aodvv2_message_t *message,
                         ipv6_addr_t *dst, uint16_t iface);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* PRIV_AODVV2_H */
