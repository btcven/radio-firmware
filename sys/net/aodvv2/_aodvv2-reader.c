/*
 * Copyright (C) 2014 Freie Universität Berlin
 * Copyright (C) 2014 Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_aodvv2
 * @{
 *
 * @file
 * @brief       RFC5444 RREP
 *
 * @author      Lotte Steenbrink <lotte.steenbrink@fu-berlin.de>
 * @author      Gustavo Grisales <gustavosinbandera1@hotmail.com>
 * @author      Jean Pierre Dudey <jeandudey@hotmail.com>
 * @}
 */

#include "_aodvv2-lrs.h"
#include "_aodvv2-mcmsg.h"
#include "_aodvv2-neigh.h"
#include "_aodvv2-reader.h"
#include "_aodvv2-seqnum.h"
#include "_aodvv2-writer.h"

#include "net/aodvv2.h"
#include "net/aodvv2/conf.h"
#include "net/aodvv2/metric.h"
#include "net/aodvv2/rcs.h"
#include "net/aodvv2/msg.h"

#include "net/gnrc/icmpv6/error.h"
#include "net/gnrc/ipv6/nib/ft.h"

#include "xtimer.h"

#include "rfc5444/rfc5444_iana.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define AODVV2_ROUTE_LIFETIME \
    (CONFIG_AODVV2_ACTIVE_INTERVAL + CONFIG_AODVV2_MAX_IDLETIME)

enum {
    IDX_MSGTLV_ACKREQ = 0,
    IDX_MSGTLV_TIMESTAMP,
};

enum {
    IDX_ADDRTLV_PATH_METRIC = 0,
    IDX_ADDRTLV_SEQ_NUM,
    IDX_ADDRTLV_ADDRESS_TYPE,
};

static enum rfc5444_result _rreq_msgtlvs(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _rreq_addrtlvs(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _rreq_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _rrep_msgtlvs(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _rrep_addrtlvs(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _rrep_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static enum rfc5444_result _rrep_ack_msgtlvs(struct rfc5444_reader_tlvblock_context *cont);
static enum rfc5444_result _rrep_ack_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped);

static struct rfc5444_reader_tlvblock_consumer _rrep_ack_consumer =
{
    .msg_id = AODVV2_MSGTYPE_RREP_ACK,
    .block_callback = _rrep_ack_msgtlvs,
    .end_callback = _rrep_ack_end,
};

static struct rfc5444_reader_tlvblock_consumer_entry _rrep_ack_tlvs[] =
{
    [IDX_MSGTLV_ACKREQ] = {
        .type = AODVV2_MSGTLV_ACKREQ,
        .min_length = 1,
        .max_length = 1,
    },
    [IDX_MSGTLV_TIMESTAMP] = {
        .type = RFC7182_MSGTLV_TIMESTAMP,
        .type_ext = 0,
        .min_length = 2,
        .max_length = 2,
    },
};

static struct rfc5444_reader_tlvblock_consumer _rreq_consumer =
{
    .msg_id = AODVV2_MSGTYPE_RREQ,
    .block_callback =_rreq_msgtlvs,
    .end_callback = _rreq_end,
};

static struct rfc5444_reader_tlvblock_consumer _rreq_address_consumer =
{
    .msg_id = AODVV2_MSGTYPE_RREQ,
    .addrblock_consumer = true,
    .block_callback = _rreq_addrtlvs,
};

static struct rfc5444_reader_tlvblock_consumer _rrep_consumer =
{
    .msg_id = AODVV2_MSGTYPE_RREP,
    .block_callback = _rrep_msgtlvs,
    .end_callback = _rrep_end,
};

static struct rfc5444_reader_tlvblock_consumer _rrep_address_consumer =
{
    .msg_id = AODVV2_MSGTYPE_RREP,
    .addrblock_consumer = true,
    .block_callback = _rrep_addrtlvs,
};

static struct rfc5444_reader_tlvblock_consumer_entry _rreq_rrep_addrtlvs[] =
{
    [IDX_ADDRTLV_PATH_METRIC] = {
        .type = AODVV2_ADDRTLV_PATH_METRIC,
        .min_length = 1,
        .max_length = 1,
    },
    [IDX_ADDRTLV_SEQ_NUM] = {
        .type = AODVV2_ADDRTLV_SEQ_NUM,
        .type_ext = 0,
        .min_length = 2,
        .max_length = 2,
    },
    [IDX_ADDRTLV_ADDRESS_TYPE] = {
        .type = AODVV2_ADDRTLV_ADDRESS_TYPE,
        .type_ext = 0,
        .min_length = 1,
        .max_length = 1,
    },
};

static aodvv2_message_t _msg;

static struct netaddr_str nbuf;
static char addr_str[IPV6_ADDR_MAX_STR_LEN];

static enum rfc5444_result _rreq_msgtlvs(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("aodvv2: parsing RREQ Message/TLVs\n");

    memset(&_msg, 0, sizeof(aodvv2_message_t));

    _msg.type = AODVV2_MSGTYPE_RREQ;

    if (!cont->has_hoplimit) {
        DEBUG("  missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    _msg.rreq.msg_hop_limit = cont->hoplimit;
    if (_msg.rreq.msg_hop_limit == 0) {
        DEBUG("  Hop limit is 0\n");
        return RFC5444_DROP_PACKET;
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _rreq_addrtlvs(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("aodvv2: parsing address/TLV (addr = %s)\n",
          netaddr_to_string(&nbuf, &cont->addr));

    struct rfc5444_reader_tlvblock_entry *tlv;
    aodvv2_msg_rreq_t *rreq = &_msg.rreq;

    /* ADDRESS_TYPE */
    uint8_t addrtype = AODVV2_ADDRTYPE_UNSPECIFIED;
    tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE].tlv;
    if (tlv) {
        addrtype = *(uint8_t *)tlv->single_value;
        DEBUG("  ADDRESS_TYPE = %d\n", addrtype);

        /* We shouldn't have UNSPECIFIED addresses as part of the message, the
         * only we assume is "unspecified" is SeqNoRtr address (it doesn't have
         * an ADDRESS_TYPE tlv) */
        if (addrtype == AODVV2_ADDRTYPE_UNSPECIFIED) {
            DEBUG("  invalid address included\n");
            return RFC5444_DROP_PACKET;
        }
    }

    if (addrtype == AODVV2_ADDRTYPE_ORIGPREFIX) {
        DEBUG("  ORIGPREFIX\n");
        netaddr_to_ipv6_addr(&cont->addr, &rreq->orig_prefix,
                             &rreq->orig_pfx_len);

        /* SEQ_NUM */
        tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_SEQ_NUM].tlv;
        if (tlv) {
            rreq->orig_seqnum = *(aodvv2_seqnum_t *)tlv->single_value;
            DEBUG("  SEQ_NUM = %d\n", rreq->orig_seqnum);
        }
        else {
            DEBUG("  missing SEQ_NUM\n");
            return RFC5444_DROP_PACKET;
        }

        /* PATH_METRIC */
        tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_PATH_METRIC].tlv;
        if (tlv) {
            if (tlv->type_ext != AODVV2_METRIC_TYPE_HOP_COUNT) {
                DEBUG("  MetricType not configured for use\n");
                return RFC5444_DROP_PACKET;
            }

            rreq->metric_type = AODVV2_METRIC_TYPE_HOP_COUNT;
            rreq->orig_metric = *(uint8_t *)tlv->single_value;
            DEBUG("  PATH_METRIC = %d\n", rreq->orig_metric);
        }
        else {
            DEBUG("  missing PATH_METRIC");
            return RFC5444_DROP_PACKET;
        }
    }
    else if (addrtype == AODVV2_ADDRTYPE_TARGPREFIX) {
        DEBUG("  TARGREFIX\n");
        netaddr_to_ipv6_addr(&cont->addr, &rreq->targ_prefix, NULL);

        /* SEQ_NUM */
        tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_SEQ_NUM].tlv;
        if (tlv) {
            rreq->targ_seqnum = *(aodvv2_seqnum_t *)tlv->single_value;
            DEBUG("  SEQ_NUM = %d\n", rreq->targ_seqnum);
        }
        else {
            rreq->targ_seqnum = 0;
        }
    }
    else if (addrtype == AODVV2_ADDRTYPE_UNSPECIFIED) {
        DEBUG("  SEQNORTR\n")
        netaddr_to_ipv6_addr(&cont->addr, &rreq->seqnortr, NULL);
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _rreq_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    DEBUG("aodvv2: process RREQ information\n");
    (void)cont;

    const gnrc_rfc5444_packet_data_t *pkt_data = gnrc_rfc5444_get_packet_data();
    aodvv2_msg_rreq_t *rreq = &_msg.rreq;

    /* Check that the neighbor is not blacklisted */
    _aodvv2_neigh_acquire();
    _aodvv2_neigh_t *neigh;
    if ((neigh = _aodvv2_neigh_get(&pkt_data->src, pkt_data->iface)) == NULL) {
        DEBUG("  neighbor set is full or neighbor doesn't exists\n");
        _aodvv2_neigh_release();
        return RFC5444_DROP_PACKET;
    }

    if (neigh->state == AODVV2_NEIGH_STATE_BLACKLISTED) {
        DEBUG("  neighbor is blacklisted\n");
        _aodvv2_neigh_release();
        return RFC5444_DROP_PACKET;
    }
    _aodvv2_neigh_release();

    /* Verify packet data */
    if ((rreq->msg_hop_limit == 0) ||
        (ipv6_addr_is_unspecified(&rreq->orig_prefix)) ||
        (rreq->orig_pfx_len == 0) ||
        (ipv6_addr_is_unspecified(&rreq->targ_prefix)) ||
        (rreq->orig_seqnum == 0) ||
        (!ipv6_addr_is_global(&rreq->orig_prefix)) ||
        (!ipv6_addr_is_global(&rreq->targ_prefix))) {
        DEBUG("  RREQ doesn't contain required data\n");
        return RFC5444_DROP_PACKET;
    }

    /* Check for unconfigured MetricType */
    if (rreq->metric_type != AODVV2_METRIC_TYPE_HOP_COUNT) {
        DEBUG("  MetricType is not configured for use\n");
        aodvv2_router_client_t client;
        if (aodvv2_rcs_get(&client, &rreq->targ_prefix) == 0) {
            gnrc_icmpv6_error_dst_unr_send(ICMPV6_ERROR_DST_UNR_METRIC_TYPE_MISMATCH,
                                           pkt_data->pkt);
        }
        return RFC5444_DROP_PACKET;
    }

    /* Finally, if all these verifications passed, check if we did drop this
     * message previously */
    if (dropped) {
        DEBUG("  packet dropped previously\n");
        return RFC5444_DROP_PACKET;
    }

    /* Check if this RREQ doesn't exceeds the maximum value for the configured
     * MetricType */
    uint8_t link_cost = aodvv2_metric_link_cost(rreq->metric_type);
    if (rreq->orig_metric >= (aodvv2_metric_max(rreq->metric_type) - link_cost)) {
        DEBUG("  metric limit reached\n");
        return RFC5444_DROP_PACKET;
    }

    aodvv2_metric_update(rreq->metric_type, &rreq->orig_metric);

    /* Process this RteMsg on the Local Route Set */
    if (_aodvv2_lrs_process(&_msg, &pkt_data->src, pkt_data->iface) < 0) {
        DEBUG("  couldn't process route information\n");
        return RFC5444_DROP_PACKET;
    }

    /* Process the McMsg (RREQ) to see if it's redundant (or not) */
    _aodvv2_mcmsg_t mcmsg = {
        .orig_prefix = rreq->orig_prefix,
        .orig_pfx_len = rreq->orig_pfx_len,
        .targ_prefix = rreq->targ_prefix,
        .metric_type = rreq->metric_type,
        .metric = rreq->orig_metric,
        .orig_seqnum = rreq->orig_seqnum,
        .targ_seqnum = rreq->targ_seqnum,
        .iface = pkt_data->iface,
    };
    if (_aodvv2_mcmsg_process(&mcmsg) == AODVV2_MCMSG_REDUNDANT) {
        DEBUG_PUTS("  packet is redundant");
        return RFC5444_DROP_PACKET;
    }

    aodvv2_router_client_t client;
    if (aodvv2_rcs_get(&client, &rreq->targ_prefix) == 0) {
        DEBUG("  RREQ is for us (client = %s/%d)\n",
              ipv6_addr_to_str(addr_str, &client.addr, sizeof(addr_str)),
              client.pfx_len);

        /* TODO(jeandudey): Check CONTROL_TRAFFIC_LIMIT */
        aodvv2_msg_rrep_t rrep = {
            .msg_hop_limit = CONFIG_AODVV2_MAX_HOPCOUNT - rreq->msg_hop_limit,
            .orig_prefix = rreq->orig_prefix,
            .targ_prefix = rreq->targ_prefix,
            /* TODO(jeandudey): SeqNoRtr */
            .seqnortr = {{0}},
            .targ_pfx_len = client.pfx_len,
            .targ_seqnum = _aodvv2_seqnum_new(),
            .metric_type = AODVV2_METRIC_TYPE_HOP_COUNT,
            .targ_metric = client.cost,
        };
        if (_aodvv2_writer_send_rrep(&rrep, &pkt_data->src,
                                     pkt_data->iface) < 0) {
            DEBUG("  couldn't send RREP\n");
            return RFC5444_DROP_PACKET;
        }

        /* TODO: verify best route to OrigPrefix and if "unconfirmed" send a
         * RREP_Ack to the next hop */
    }
    else {
        DEBUG("  RREQ is not for us\n");

        if (rreq->msg_hop_limit == 1) {
            DEBUG("  RREQ has reached forwarding limit");
            return RFC5444_DROP_PACKET;
        }

        /* TODO(jeandudey): Check CONTROL_TRAFFIC_LIMIT */

        _aodvv2_lrs_acquire();
        _aodvv2_local_route_t *lr;
        if ((lr = _aodvv2_lrs_find(&rreq->orig_prefix)) == NULL) {
            DEBUG("  no route to OrigPrefix found in Local Route\n");
            return RFC5444_DROP_PACKET;
        }
        rreq->msg_hop_limit -= 1;
        rreq->orig_metric = lr->metric;
        _aodvv2_lrs_release();

        _aodvv2_writer_send_rreq(rreq);
    }

    return RFC5444_OKAY;
}


static enum rfc5444_result _rrep_msgtlvs(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("aodvv2: parsing RREQ Message/TLVs\n");

    memset(&_msg, 0, sizeof(aodvv2_message_t));

    _msg.type = AODVV2_MSGTYPE_RREP;

    if (!cont->has_hoplimit) {
        DEBUG("  missing hop limit\n");
        return RFC5444_DROP_PACKET;
    }

    _msg.rrep.msg_hop_limit = cont->hoplimit;
    if (_msg.rrep.msg_hop_limit == 0) {
        DEBUG("  Hop limit is 0\n");
        return RFC5444_DROP_PACKET;
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _rrep_addrtlvs(struct rfc5444_reader_tlvblock_context *cont)
{
    DEBUG("aodvv2: parsing address/TLV (addr = %s)\n",
          netaddr_to_string(&nbuf, &cont->addr));

    struct rfc5444_reader_tlvblock_entry *tlv;
    aodvv2_msg_rrep_t *rrep = &_msg.rrep;

    /* ADDRESS_TYPE */
    uint8_t addrtype = AODVV2_ADDRTYPE_UNSPECIFIED;
    tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_ADDRESS_TYPE].tlv;
    if (tlv) {
        addrtype = *(uint8_t *)tlv->single_value;
        DEBUG("  ADDRESS_TYPE = %d\n", addrtype);

        /* We shouldn't have UNSPECIFIED addresses as part of the message, the
         * only we assume is "unspecified" is SeqNoRtr address (it doesn't have
         * an ADDRESS_TYPE tlv) */
        if (addrtype == AODVV2_ADDRTYPE_UNSPECIFIED) {
            DEBUG("  invalid address included\n");
            return RFC5444_DROP_PACKET;
        }
    }

    if (addrtype == AODVV2_ADDRTYPE_ORIGPREFIX) {
        DEBUG("  ORIGPREFIX\n");
        netaddr_to_ipv6_addr(&cont->addr, &rrep->orig_prefix, NULL);
    }
    else if (addrtype == AODVV2_ADDRTYPE_TARGPREFIX) {
        DEBUG("  TARGREFIX\n");
        netaddr_to_ipv6_addr(&cont->addr, &rrep->targ_prefix,
                             &rrep->targ_pfx_len);

        /* SEQ_NUM */
        tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_SEQ_NUM].tlv;
        if (tlv) {
            rrep->targ_seqnum = *(aodvv2_seqnum_t *)tlv->single_value;
            DEBUG("  SEQ_NUM = %d\n", rrep->targ_seqnum);
        }
        else {
            DEBUG("  missing SEQ_NUM\n");
            return RFC5444_DROP_PACKET;
        }

        /* PATH_METRIC */
        tlv = _rreq_rrep_addrtlvs[IDX_ADDRTLV_PATH_METRIC].tlv;
        if (tlv) {
            if (tlv->type_ext != AODVV2_METRIC_TYPE_HOP_COUNT) {
                /* TODO(jeandudey): send "Destination Unreachable" */
                return RFC5444_DROP_PACKET;
            }

            rrep->metric_type = AODVV2_METRIC_TYPE_HOP_COUNT;
            rrep->targ_metric = *(uint8_t *)tlv->single_value;
            DEBUG("  PATH_METRIC = %d\n", rrep->targ_metric);
        }
        else {
            DEBUG("  missing PATH_METRIC");
            return RFC5444_DROP_PACKET;
        }
    }
    else if (addrtype == AODVV2_ADDRTYPE_UNSPECIFIED) {
        DEBUG("  SEQNORTR\n")
        netaddr_to_ipv6_addr(&cont->addr, &rrep->seqnortr, NULL);
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _rrep_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    DEBUG("aodvv2: processing RREP information\n");
    (void)cont;

    aodvv2_msg_rrep_t *rrep = &_msg.rrep;

    if (dropped) {
        DEBUG("  packet dropped previously\n");
        return RFC5444_DROP_PACKET;
    }

    if (rrep->msg_hop_limit == 0) {
        DEBUG("  hop limit reached\n");
        return RFC5444_DROP_PACKET;
    }

    if (ipv6_addr_is_unspecified(&rrep->orig_prefix)) {
        DEBUG("  invalid OrigPrefix\n");
        return RFC5444_DROP_PACKET;
    }

    if (ipv6_addr_is_unspecified(&rrep->targ_prefix) || rrep->targ_pfx_len == 0) {
        DEBUG("  invalid TargPrefix/TargPrefixLen\n");
        return RFC5444_DROP_PACKET;
    }

    if (rrep->targ_seqnum == 0) {
        DEBUG("  invalid TargSeqNum\n");
        return RFC5444_DROP_PACKET;
    }

    uint8_t link_cost = aodvv2_metric_link_cost(rrep->metric_type);
    if ((aodvv2_metric_max(rrep->metric_type) - link_cost) <= rrep->targ_metric) {
        DEBUG("  metric limit reached\n");
        return RFC5444_DROP_PACKET;
    }

    //const gnrc_rfc5444_packet_data_t *pkt_data = gnrc_rfc5444_get_packet_data();

    aodvv2_metric_update(rrep->metric_type, &rrep->targ_metric);

    /* for every relevant address (RteMsg.Addr) in the RteMsg, HandlingRtr
    searches its route table to see if there is a route table entry with the
    same MetricType of the RteMsg, matching RteMsg.Addr. */

    //aodvv2_local_route_t *rt_entry =
    //    aodvv2_lrs_get_entry(&_msg.targ_node.addr, _msg.metric_type);

    //if (!rt_entry || (rt_entry->metric_type != _msg.metric_type)) {
    //    DEBUG_PUTS("aodvv2: creating new Local Route");

    //    aodvv2_local_route_t tmp = {0};
    //    aodvv2_lrs_fill_routing_entry_rrep(&_msg, &tmp, link_cost);
    //    aodvv2_lrs_add_entry(&tmp);

    //    /* Add entry to NIB forwarding table */
    //    DEBUG_PUTS("aodvv2: adding Local Route to NIB FT");
    //    if (gnrc_ipv6_nib_ft_add(&_msg.targ_node.addr,
    //                             _msg.targ_node.pfx_len, &_msg.sender,
    //                             _netif_pid, AODVV2_ROUTE_LIFETIME) < 0) {
    //        DEBUG_PUTS("aodvv2: couldn't add route");
    //    }
    //}
    //else {
    //    if (!aodvv2_lrs_offers_improvement(rt_entry, &_msg.targ_node)) {
    //        DEBUG_PUTS("aodvv2: RREP offers no improvement over known route");
    //        return RFC5444_DROP_PACKET;
    //    }

    //    /* The incoming routing information is better than existing routing
    //     * table information and SHOULD be used to improve the route table. */
    //    DEBUG_PUTS("aodvv2: updating Routing Table entry");
    //    aodvv2_lrs_fill_routing_entry_rrep(&_msg, rt_entry, link_cost);

    //    /* Add entry to nib forwarding table */
    //    gnrc_ipv6_nib_ft_del(&rt_entry->addr, rt_entry->pfx_len);

    //    DEBUG_PUTS("aodvv2: adding route to NIB FT");
    //    if (gnrc_ipv6_nib_ft_add(&rt_entry->addr, rt_entry->pfx_len,
    //                             &rt_entry->next_hop, _netif_pid,
    //                             AODVV2_ROUTE_LIFETIME) < 0) {
    //        DEBUG_PUTS("aodvv2: couldn't add route");
    //    }
    //}

    //aodvv2_router_client_t client;
    //if (aodvv2_rcs_get(&client, &_msg.orig_node.addr) == 0) {
    //    DEBUG("aodvv2: this is my RREP (SeqNum: %d)\n",
    //          _msg.orig_node.seqnum);
    //    DEBUG_PUTS("aodvv2: We are done here, thanks!");

    //    /* Send buffered packets for this address */
    //    aodvv2_buffer_dispatch(&_msg.targ_node.addr);
    //}
    //else {
    //    DEBUG_PUTS("aodvv2: not my RREP, passing it on to the next hop.");

    //    ipv6_addr_t *next_hop =
    //        aodvv2_lrs_get_next_hop(&_msg.orig_node.addr,
    //                                _msg.metric_type);
    //    aodvv2_send_rrep(&_msg, next_hop);
    //}
    return RFC5444_OKAY;
}

static enum rfc5444_result _rrep_ack_msgtlvs(struct rfc5444_reader_tlvblock_context *cont)
{
    assert(cont != NULL);
    struct rfc5444_reader_tlvblock_entry *tlv;

    _msg.type = AODVV2_MSGTYPE_RREP_ACK;
    _msg.rrep_ack.timestamp = 0;
    _msg.rrep_ack.ackreq = 0;

    bool is_request = false;
    tlv = _rrep_ack_tlvs[IDX_MSGTLV_ACKREQ].tlv;
    if (tlv) {
        is_request = true;
        memcpy(&_msg.rrep_ack.ackreq, tlv->single_value, sizeof(uint8_t));
    }

    bool has_timestamp = false;
    tlv = _rrep_ack_tlvs[IDX_MSGTLV_TIMESTAMP].tlv;
    if (tlv) {
        has_timestamp = true;
        memcpy(&_msg.rrep_ack.timestamp, tlv->single_value, sizeof(aodvv2_seqnum_t));
    }

    if (is_request && !has_timestamp) {
        DEBUG("aodvv2: RREP_Ack doesn't contains TIMESTAMP TLV\n");
        return RFC5444_DROP_PACKET;
    }

    return RFC5444_OKAY;
}

static enum rfc5444_result _rrep_ack_end(struct rfc5444_reader_tlvblock_context *cont, bool dropped)
{
    assert(cont != NULL);

    if (dropped) {
        DEBUG("aodvv2: dropped RREP_Ack message (cont = %p)", (void *)cont);
        return RFC5444_DROP_PACKET;
    }

    _aodvv2_neigh_acquire();
    const gnrc_rfc5444_packet_data_t *pkt_data = gnrc_rfc5444_get_packet_data();
    _aodvv2_neigh_t *neigh = _aodvv2_neigh_alloc(&pkt_data->src, pkt_data->iface);
    if (neigh == NULL) {
        DEBUG("aodvv2: couldn't allocate neigh\n");
        return RFC5444_DROP_PACKET;
    }

    bool is_request = _msg.rrep_ack.ackreq != 0;
    if (is_request) {
        neigh->ackseqnum = _msg.rrep_ack.timestamp;

        aodvv2_msg_rrep_ack_t rrep_ack = {
            .ackreq = 0,
            .timestamp = neigh->ackseqnum,
        };
        _aodvv2_writer_send_rrep_ack(&rrep_ack, &neigh->addr, neigh->iface);
    }
    else {
        DEBUG("aodvv2: processing RREP_Ack reply (addr = %s, iface = %d)\n",
              ipv6_addr_to_str(addr_str, &neigh->addr, sizeof(addr_str)),
              neigh->iface);

        /* Only process RREP_Ack for "heard" neighbors, if it's blacklisted
         * or "confirmed", ignore it, we don't need it */
        if (neigh->state != AODVV2_NEIGH_STATE_HEARD) {
            DEBUG("  neighbor is not heard\n");
            return RFC5444_DROP_PACKET;
        }

        /* If we ask for a RREP_Ack for the neighbor, we set a timeout
         * (expiration) for the request, if no timeout is set, we didn't ask
         * for a RREP_Ack reply. */
        if (_timex_is_zero(neigh->timeout)) {
            DEBUG("  unsocilited RREP_Ack reply\n");
            return RFC5444_DROP_PACKET;
        }

        /* Compare if the neighbor has the same TIMESTAMP value that we have,
         * if not, discard the message */
        if (neigh->ackseqnum != _msg.rrep_ack.timestamp) {
            DEBUG("  received TIMESTAMP doesn't match (AckSeqNum = %d, TIMESTAMP = %d)\n",
                  neigh->ackseqnum, _msg.rrep_ack.timestamp);
            neigh->ackseqnum += 1;
            return RFC5444_DROP_PACKET;
        }
    }
    _aodvv2_neigh_release();

    return RFC5444_OKAY;
}

void _aodvv2_reader_init(void)
{
    gnrc_rfc5444_reader_acquire();
    struct rfc5444_reader *reader = gnrc_rfc5444_reader();

    rfc5444_reader_add_message_consumer(reader, &_rrep_consumer, NULL, 0);
    rfc5444_reader_add_message_consumer(reader, &_rrep_address_consumer, _rreq_rrep_addrtlvs, ARRAY_SIZE(_rreq_rrep_addrtlvs));

    rfc5444_reader_add_message_consumer(reader, &_rreq_consumer, NULL, 0);
    rfc5444_reader_add_message_consumer(reader, &_rreq_address_consumer, _rreq_rrep_addrtlvs, ARRAY_SIZE(_rreq_rrep_addrtlvs));

    rfc5444_reader_add_message_consumer(reader, &_rrep_ack_consumer, _rrep_ack_tlvs, ARRAY_SIZE(_rrep_ack_tlvs));
    gnrc_rfc5444_reader_release();
}
