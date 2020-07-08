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
 * @brief       Test GNRC RFC 5444
 *
 * @author      Locha Mesh Developers <contact@locha.io>
 *
 * @}
 */

#include "embUnit.h"
#include "embUnit/embUnit.h"

#include "net/rfc5444.h"
#include "net/ethernet.h"

#include "common.h"

#define TEST_ASSERT_EQUAL_ADDR(a, b) \
    TEST_ASSERT(ipv6_addr_equal((a), (b)))

#define RIOT_MSGTYPE_TEST 225

static struct rfc5444_writer_message *_test_msg = NULL;

static int _add_message_header(struct rfc5444_writer *writer, struct rfc5444_writer_message *msg)
{
    rfc5444_writer_set_msg_header(writer, msg, false, false, true, false);
    rfc5444_writer_set_msg_hoplimit(writer, msg, 255);
    return 0;
}

static void _set_up(void)
{
    _common_set_up();
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

static void test_gnrc_rfc5444_conv_roundtrip(void)
{
    ipv6_addr_t addr;
    struct netaddr netaddr;
    uint8_t pfx_len;

    ipv6_addr_from_str(&addr, "fc00:db8::1");
    pfx_len = 128;
    ipv6_addr_to_netaddr(&addr, pfx_len, &netaddr);

    TEST_ASSERT_EQUAL_INT(0, memcmp(&addr, &netaddr._addr, sizeof(ipv6_addr_t)));

    addr = ipv6_addr_unspecified;
    netaddr_to_ipv6_addr(&netaddr, &addr, &pfx_len);
    TEST_ASSERT_EQUAL_INT(128, pfx_len);
    TEST_ASSERT_EQUAL_INT(0, memcmp(&addr, &netaddr._addr, sizeof(ipv6_addr_t)));
}

static void test_gnrc_rfcc5444_target_roundtrip(void)
{
    ipv6_addr_t dst;
    ipv6_addr_from_str(&dst, "fe80::");

    for (unsigned i = 0; i < CONFIG_RFC5444_TARGET_NUMOF; i++) {
        dst.u8[15] = i;
        TEST_ASSERT_EQUAL_INT(0, gnrc_rfc5444_add_writer_target(&dst, _mock_netif->pid));
    }

    /* we filled up the available targets, we can't add another this time */
    dst.u8[15] = 16;
    TEST_ASSERT_EQUAL_INT(-ENOMEM, gnrc_rfc5444_add_writer_target(&dst, _mock_netif->pid));

    /* try to get the raw target interface for each one */
    for (unsigned i = 0; i < CONFIG_RFC5444_TARGET_NUMOF; i++) {
        dst.u8[15] = i;
        TEST_ASSERT_NOT_NULL(gnrc_rfc5444_get_writer_target(&dst, _mock_netif->pid));
    }

    /* go ahead and delete each target we registered */
    for (unsigned i = 0; i < CONFIG_RFC5444_TARGET_NUMOF; i++) {
        dst.u8[15] = i;
        gnrc_rfc5444_del_writer_target(&dst, _mock_netif->pid);
    }

    /* try again to get the raw target interface for each one, this time none
     * should exist */
    for (unsigned i = 0; i < CONFIG_RFC5444_TARGET_NUMOF; i++) {
        dst.u8[15] = i;
        TEST_ASSERT_NULL(gnrc_rfc5444_get_writer_target(&dst, _mock_netif->pid));
    }
}

static void test_gnrc_rfc5444_send_message(void)
{
    msg_t msg;
    gnrc_pktsnip_t *pkt;

    ipv6_addr_t dst;
    ipv6_addr_from_str(&dst, "fe80::1");

    /* add out fake target */
    TEST_ASSERT_EQUAL_INT(0, gnrc_rfc5444_add_writer_target(&dst, _mock_netif->pid));

    gnrc_rfc5444_writer_acquire();

    struct rfc5444_writer *writer = gnrc_rfc5444_writer();
    TEST_ASSERT_MESSAGE(rfc5444_writer_create_message_alltarget(writer,
                                                                RIOT_MSGTYPE_TEST,
                                                                RFC5444_MAX_ADDRLEN) == RFC5444_OKAY,
                        "oonf_rfc5444 output is bogus, check your available memory");
    gnrc_rfc5444_writer_release();

    msg_receive(&msg);
    pkt = msg.content.ptr;

    TEST_ASSERT_EQUAL_INT(GNRC_NETAPI_MSG_TYPE_SND, msg.type);
    TEST_ASSERT_NOT_NULL(pkt->next);

    gnrc_rfc5444_del_writer_target(&dst, _mock_netif->pid);
}

static Test *tests_gnrc_rfc5444(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_gnrc_rfc5444_conv_roundtrip),
        new_TestFixture(test_gnrc_rfcc5444_target_roundtrip),
        new_TestFixture(test_gnrc_rfc5444_send_message),
    };

    EMB_UNIT_TESTCALLER(tests, _set_up, NULL, fixtures);

    return (Test *)&tests;
}

int main(void)
{
    _tests_init();
    gnrc_rfc5444_init();

    gnrc_rfc5444_writer_acquire();
    struct rfc5444_writer *writer = gnrc_rfc5444_writer();
    _test_msg = rfc5444_writer_register_message(writer, RIOT_MSGTYPE_TEST, false);
    _test_msg->addMessageHeader = _add_message_header;
    gnrc_rfc5444_writer_release();

    TESTS_START();
    TESTS_RUN(tests_gnrc_rfc5444());
    TESTS_END();

    return 0;
}
