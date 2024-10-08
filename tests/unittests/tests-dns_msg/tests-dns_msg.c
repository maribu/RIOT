/*
 * Copyright (C) 2024 ML!PA Consulting GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdint.h>
#include <string.h>
#include "net/af.h"
#include "net/ipv6.h"
#include "ztimer.h"

#include "net/dns/msg.h"

#include "tests-dns_msg.h"

static void test_dns_msg_valid_AAAA(void)
{
    const uint8_t dns_msg[] = {
        /* in scapy notation:
         * <DNS  id=0 qr=1 opcode=QUERY aa=0 tc=0 rd=1 ra=1 z=0 ad=0 cd=0 rcode=ok
         *       qdcount=1 ancount=1 nscount=0 arcount=0
         *       qd=<DNSQR  qname='example.org.' qtype=AAAA qclass=IN |>
         *       an=<DNSRR  rrname='\\xc0\x0c' type=AAAA rclass=IN ttl=300
         *                  rdata=2001:db8:4005:80b::200e |>
         *       ns=None ar=None |> */
        0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x07, 0x65, 0x78, 0x61,
        0x6d, 0x70, 0x6c, 0x65, 0x03, 0x6f, 0x72, 0x67,
        0x00, 0x00, 0x1c, 0x00, 0x01, 0xc0, 0x0c, 0x00,
        0x1c, 0x00, 0x01, 0x00, 0x00, 0x01, 0x2c, 0x00,
        0x10, 0x20, 0x01, 0x0d, 0xb8, 0x40, 0x05, 0x08,
        0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20,
        0x0e
    };
    const uint8_t addr[] = {
        /* 2001:db8:4005:80b::200e */
        0x20, 0x01, 0x0d, 0xb8, 0x40, 0x05, 0x08, 0x0b,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0e,
    };
    const uint32_t ttl = 300;

    uint8_t addr_out[16];
    uint32_t ttl_out;
    int res = dns_msg_parse_reply(dns_msg, sizeof(dns_msg), AF_INET6, &addr_out, &ttl_out);
    TEST_ASSERT_EQUAL_INT(16, res);
    TEST_ASSERT_EQUAL_INT(ttl, ttl_out);
    TEST_ASSERT_EQUAL_INT(0, memcmp(addr, addr_out, sizeof(addr)));
}

static void test_dns_msg_valid_dns64(void)
{
    const uint8_t dns_msg[] = {
        /* in scapy notation:
         * <DNS  id=0 qr=1 opcode=QUERY aa=0 tc=0 rd=1 ra=1 z=0 ad=0 cd=0 rcode=ok
                  qdcount=1 ancount=1 nscount=0 arcount=0
                  qd=<DNSQR  qname='example.org.' qtype=AAAA qclass=IN |>
                  an=<DNSRR  rrname='\\xc0\x0c' type=AAAA rclass=IN ttl=60
                             rdata=2001:db8:2b0:db32:0:1:8c52:7903 |>
                  ns=None ar=None |> */
        0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00, 0x07, 0x65, 0x78, 0x61,
        0x6d, 0x70, 0x6c, 0x65, 0x03, 0x6f, 0x72, 0x67,
        0x00, 0x00, 0x1c, 0x00, 0x01, 0xc0, 0x0c, 0x00,
        0x1c, 0x00, 0x01, 0x00, 0x00, 0x00, 0x3c, 0x00,
        0x10, 0x20, 0x01, 0x0d, 0xb8, 0x02, 0xb0, 0xdb,
        0x32, 0x00, 0x00, 0x00, 0x01, 0x8c, 0x52, 0x79,
        0x03
    };
    const uint8_t addr[] = {
        /* 2001:db8:2b0:db32:0:1:8c52:7903 */
        0x20, 0x01, 0x0d, 0xb8, 0x02, 0xb0, 0xdb, 0x32,
        0x00, 0x00, 0x00, 0x01, 0x8c, 0x52, 0x79, 0x03,
    };
    const uint32_t ttl = 60;

    uint8_t addr_out[16];
    uint32_t ttl_out;
    int res = dns_msg_parse_reply(dns_msg, sizeof(dns_msg), AF_INET6, &addr_out, &ttl_out);
    TEST_ASSERT_EQUAL_INT(16, res);
    TEST_ASSERT_EQUAL_INT(ttl, ttl_out);
    TEST_ASSERT_EQUAL_INT(0, memcmp(addr, addr_out, sizeof(addr)));
}

static void test_dns_msg_valid_dns64_w_long_cnames(void)
{
    const uint8_t dns_msg[] = {
        /* in scapy notation:
         * <DNS  id=0 qr=1 opcode=QUERY aa=0 tc=0 rd=1 ra=1 z=0 ad=0 cd=0 rcode=ok
         *       qdcount=1 ancount=3 nscount=0 arcount=0
         *       qd=<DNSQR  qname='the.too.long.name.for.the.example.net.' qtype=AAAA qclass=IN |>
         *       an=<DNSRR  rrname='\\xc0\x0c' type=CNAME rclass=IN ttl=600
         *                  rdata='the.too.long.name.for.the.example.net.' |
         *          <DNSRR  rrname='\\xc0C' type=CNAME rclass=IN ttl=90
         *                  rdata='this-is-becoming-ridic.ulous.naming.cloud.example.com.' |
         *          <DNSRR  rrname='\\xc0p' type=AAAA rclass=IN ttl=10
         *                  rdata=2001:db8:2b0:db32:0:1:1432:418d |>>>
         *       ns=None ar=None |>
         */
        0x00, 0x00, 0x81, 0x80, 0x00, 0x01, 0x00, 0x03,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x74, 0x68, 0x65,
        0x03, 0x74, 0x6f, 0x6f, 0x04, 0x6c, 0x6f, 0x6e,
        0x67, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x03, 0x66,
        0x6f, 0x72, 0x03, 0x74, 0x68, 0x65, 0x07, 0x65,
        0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x03, 0x6e,
        0x65, 0x74, 0x00, 0x00, 0x1c, 0x00, 0x01, 0xc0,
        0x0c, 0x00, 0x05, 0x00, 0x01, 0x00, 0x00, 0x02,
        0x58, 0x00, 0x21, 0x03, 0x74, 0x68, 0x65, 0x04,
        0x65, 0x76, 0x65, 0x6e, 0x10, 0x6c, 0x6f, 0x6f,
        0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f, 0x6f,
        0x6f, 0x6e, 0x67, 0x65, 0x72, 0x04, 0x6e, 0x61,
        0x6d, 0x65, 0xc0, 0x26, 0xc0, 0x43, 0x00, 0x05,
        0x00, 0x01, 0x00, 0x00, 0x00, 0x5a, 0x00, 0x37,
        0x16, 0x74, 0x68, 0x69, 0x73, 0x2d, 0x69, 0x73,
        0x2d, 0x62, 0x65, 0x63, 0x6f, 0x6d, 0x69, 0x6e,
        0x67, 0x2d, 0x72, 0x69, 0x64, 0x69, 0x63, 0x05,
        0x75, 0x6c, 0x6f, 0x75, 0x73, 0x06, 0x6e, 0x61,
        0x6d, 0x69, 0x6e, 0x67, 0x05, 0x63, 0x6c, 0x6f,
        0x75, 0x64, 0x07, 0x65, 0x78, 0x61, 0x6d, 0x70,
        0x6c, 0x65, 0x03, 0x63, 0x6f, 0x6d, 0x00, 0xc0,
        0x70, 0x00, 0x1c, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x0a, 0x00, 0x10, 0x20, 0x01, 0x0d, 0xb8, 0x02,
        0xb0, 0xdb, 0x32, 0x00, 0x00, 0x00, 0x01, 0x14,
        0x32, 0x41, 0x8d
    };
    const uint8_t addr[] = {
        /* 2001:db8:2b0:db32:0:1:1432:418d */
        0x20, 0x01, 0x0d, 0xb8, 0x02, 0xb0, 0xdb, 0x32,
        0x00, 0x00, 0x00, 0x01, 0x14, 0x32, 0x41, 0x8d,
    };
    const uint32_t ttl = 10;

    uint8_t addr_out[16];
    uint32_t ttl_out;
    int res = dns_msg_parse_reply(dns_msg, sizeof(dns_msg), AF_INET6, &addr_out, &ttl_out);
    TEST_ASSERT_EQUAL_INT(16, res);
    TEST_ASSERT_EQUAL_INT(ttl, ttl_out);
    TEST_ASSERT_EQUAL_INT(0, memcmp(addr, addr_out, sizeof(addr)));
}

Test *tests_dns_msg_tests(void)
{
    EMB_UNIT_TESTFIXTURES(fixtures) {
        new_TestFixture(test_dns_msg_valid_AAAA),
        new_TestFixture(test_dns_msg_valid_dns64),
        new_TestFixture(test_dns_msg_valid_dns64_w_long_cnames),
    };

    EMB_UNIT_TESTCALLER(dns_msg_tests, NULL, NULL, fixtures);

    return (Test *)&dns_msg_tests;
}

void tests_dns_msg(void)
{
    TESTS_RUN(tests_dns_msg_tests());
}
