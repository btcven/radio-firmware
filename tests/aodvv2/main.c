/*
 * Copyright (C) 2020 Locha Inc
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test AODVv2 Router Client Set
 *
 * @author      Locha Mesh Developers <contact@locha.io>
 *
 * @}
 */

#include "embUnit.h"
#include "embUnit/embUnit.h"

#include "net/aodvv2.h"
#include "net/aodvv2/rcs.h"
#include "net/rfc5444.h"
#include "net/ethernet.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/udp.h"
#include "net/gnrc/netif/hdr.h"

#include "common.h"

#define TEST_ASSERT_EQUAL_ADDR(a, b) \
    TEST_ASSERT(ipv6_addr_equal((a), (b)))

static void _send(const ipv6_addr_t *src, const ipv6_addr_t *dst);

static void _set_up(void)
{
    gnrc_netif_acquire(_mock_netif);
    _mock_netif->ipv6.mtu = ETHERNET_DATA_LEN;
    _mock_netif->cur_hl = CONFIG_GNRC_NETIF_DEFAULT_HL;
    gnrc_netif_release(_mock_netif);
    gnrc_pktbuf_init();
    /* remove messages */
    while (msg_avail()) {
        msg_t msg;
        msg_receive(&msg);
    }
}

static void test_aodvv2_rreq(void)
{
    ipv6_addr_t rtraddr;
    ipv6_addr_t dst;
    ipv6_addr_from_str(&rtraddr, "fc00:200::1");
    ipv6_addr_from_str(&dst, "fc00:db8::1");

    TEST_ASSERT_EQUAL_INT(0, aodvv2_gnrc_netif_join(_mock_netif));
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_add(&rtraddr, 64, 0));

    /* Trigger route discovery process */
    _send(&rtraddr, &dst);

    /* Remove the UDP message we just sent */
    msg_t msg;
    msg_receive(&msg);
    TEST_ASSERT_EQUAL_INT(GNRC_NETAPI_MSG_TYPE_SND, msg.type);

    /* Receive the RFC 5444 message AODVv2 _should_ have generated */
    msg_receive(&msg);
}

static Test *tests_aodvv2(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_aodvv2_rreq),
    };

    EMB_UNIT_TESTCALLER(tests, _set_up, NULL, fixtures);

    return (Test *)&tests;
}

int main(void)
{
    _tests_init();
    gnrc_rfc5444_init();
    aodvv2_init();

    TESTS_START();
    TESTS_RUN(tests_aodvv2());
    TESTS_END();

    return 0;
}

static void _send(const ipv6_addr_t *src, const ipv6_addr_t *dst)
{
    gnrc_pktsnip_t *payload, *udp, *ip;

    /* allocate payload */
    payload = gnrc_pktbuf_add(NULL, NULL, 128, GNRC_NETTYPE_UNDEF);
    if (payload == NULL) {
        puts("Error: unable to copy data to packet buffer");
        return;
    }

    memset(payload->data, 0, 128);
    /* allocate UDP header, set source port := destination port */
    udp = gnrc_udp_hdr_build(payload, 1337, 1337);

    if (udp == NULL) {
        puts("Error: unable to allocate UDP header");
        gnrc_pktbuf_release(payload);
        return;
    }
    /* allocate IPv6 header */
    ip = gnrc_ipv6_hdr_build(udp, src, dst);
    if (ip == NULL) {
        puts("Error: unable to allocate IPv6 header");
        gnrc_pktbuf_release(udp);
        return;
    }
    /* add netif header, if interface was given */
    gnrc_pktsnip_t *netif_hdr = gnrc_netif_hdr_build(NULL, 0, NULL, 0);

    if (netif_hdr == NULL) {
        puts("Error: unable to allocate NETIF header");
        gnrc_pktbuf_release(ip);
        return;
    }
    gnrc_netif_hdr_set_netif(netif_hdr->data, _mock_netif);
    LL_PREPEND(ip, netif_hdr);

    /* send packet */
    if (!gnrc_netapi_dispatch_send(GNRC_NETTYPE_UDP, GNRC_NETREG_DEMUX_CTX_ALL, ip)) {
        puts("Error: unable to locate UDP thread");
        gnrc_pktbuf_release(ip);
        return;
    }
}
