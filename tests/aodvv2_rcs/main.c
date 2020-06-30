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

#include "net/aodvv2/conf.h"
#include "net/aodvv2/rcs.h"

#define TEST_ASSERT_EQUAL_ADDR(a, b) \
    TEST_ASSERT(ipv6_addr_equal((a), (b)))

static void _set_up(void)
{
}

static void test_aodvv2_rcs_add(void)
{
    ipv6_addr_t addr;
    ipv6_addr_from_str(&addr, "fc00:200::");

    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_add(&addr, 64, 1));
    TEST_ASSERT_EQUAL_INT(-EEXIST, aodvv2_rcs_add(&addr, 64, 1));
    TEST_ASSERT_EQUAL_INT(-EINVAL, aodvv2_rcs_add(&ipv6_addr_unspecified, 64, 0));
    TEST_ASSERT_EQUAL_INT(-EINVAL, aodvv2_rcs_add(&addr, 0, 0));

    aodvv2_router_client_t client;
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_get(&client, &addr));
    TEST_ASSERT_EQUAL_ADDR(&client.addr, &addr);
    TEST_ASSERT_EQUAL_INT(64, client.pfx_len);
    TEST_ASSERT_EQUAL_INT(1, client.cost);

    ipv6_addr_t addr_common;
    ipv6_addr_from_str(&addr_common, "fc00:200:0:0:cafe::");

    TEST_ASSERT_EQUAL_INT(-EEXIST, aodvv2_rcs_add(&addr_common, 64, 2));
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_add(&addr_common, 80, 2));
    TEST_ASSERT_EQUAL_INT(-EEXIST, aodvv2_rcs_add(&addr_common, 80, 2));

    aodvv2_router_client_t other_client;
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_get(&other_client, &addr_common));
    TEST_ASSERT_EQUAL_ADDR(&other_client.addr, &addr_common);
    TEST_ASSERT_EQUAL_INT(80, other_client.pfx_len);
    TEST_ASSERT_EQUAL_INT(2, other_client.cost);

    /* cleanup */
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_del(&addr, 64));
    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_del(&addr_common, 80));
}

static void test_aodvv2_rcs_del(void)
{
    ipv6_addr_t addr;
    ipv6_addr_from_str(&addr, "fc00:200::");

    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_add(&addr, 64, 1));

    TEST_ASSERT_EQUAL_INT(-EINVAL, aodvv2_rcs_del(&addr, 0));
    TEST_ASSERT_EQUAL_INT(-EINVAL, aodvv2_rcs_del(&ipv6_addr_unspecified, 64));

    TEST_ASSERT_EQUAL_INT(0, aodvv2_rcs_del(&addr, 64));
    TEST_ASSERT_EQUAL_INT(-ENOENT, aodvv2_rcs_del(&addr, 64));
}

static void test_aodvv2_rcs_is_client(void)
{
}

static Test *tests_aodvv2_rcs(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_aodvv2_rcs_add),
        new_TestFixture(test_aodvv2_rcs_del),
        new_TestFixture(test_aodvv2_rcs_is_client),
    };

    EMB_UNIT_TESTCALLER(tests, _set_up, NULL, fixtures);

    return (Test *)&tests;
}

int main(void)
{
    aodvv2_rcs_init();

    TESTS_START();
    TESTS_RUN(tests_aodvv2_rcs());
    TESTS_END();

    return 0;
}
