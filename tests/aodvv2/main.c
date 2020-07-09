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
#include "net/rfc5444.h"
#include "net/ethernet.h"

#include "common.h"

#define TEST_ASSERT_EQUAL_ADDR(a, b) \
    TEST_ASSERT(ipv6_addr_equal((a), (b)))

static void _set_up(void)
{
    aodvv2_init();
    aodvv2_gnrc_netif_join(_mock_netif);
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

static void test_aodvv2_empty(void)
{
}

static Test *tests_aodvv2(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_aodvv2_empty),
    };

    EMB_UNIT_TESTCALLER(tests, _set_up, NULL, fixtures);

    return (Test *)&tests;
}

int main(void)
{
    _tests_init();
    gnrc_rfc5444_init();

    TESTS_START();
    TESTS_RUN(tests_aodvv2());
    TESTS_END();

    return 0;
}
