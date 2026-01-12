#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/socket.h>

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

// Helper to check if an address is valid IPv4
static int is_valid_ipv4(const char *addr) {
    struct in_addr result;
    return inet_pton(AF_INET, addr, &result) == 1;
}

// Helper to check if an address is valid IPv6
static int is_valid_ipv6(const char *addr) {
    struct in6_addr result;
    return inet_pton(AF_INET6, addr, &result) == 1;
}

// Helper to check if address matches family constraint
// Returns: 1 = valid match, 0 = family mismatch or invalid
static int address_matches_family(const char *source_addr, int family) {
    if (!source_addr) {
        return 0;
    }

    struct in_addr addr_v4;
    struct in6_addr addr_v6;

    if (inet_pton(AF_INET, source_addr, &addr_v4) == 1) {
        // It's IPv4 - reject if family is IPv6-only
        if (family == AF_INET6) {
            return 0;
        }
        return 1;
    } else if (inet_pton(AF_INET6, source_addr, &addr_v6) == 1) {
        // It's IPv6 - reject if family is IPv4-only
        if (family == AF_INET) {
            return 0;
        }
        return 1;
    }

    // Invalid address
    return 0;
}

// Test assertion helper
static void test_assert(int condition, const char *test_name, const char *message) {
    if (condition) {
        printf("✓ %s: %s\n", test_name, message);
        tests_passed++;
    } else {
        printf("✗ %s: %s FAILED\n", test_name, message);
        tests_failed++;
    }
}

// ============================================================================
// IPv4 Address Tests
// ============================================================================

static void test_ipv4_addresses(void) {
    const char *test_name = "IPv4 addresses";

    // Valid IPv4 addresses
    test_assert(is_valid_ipv4("192.168.1.1"), test_name, "standard private address");
    test_assert(is_valid_ipv4("10.0.0.1"), test_name, "10.x private address");
    test_assert(is_valid_ipv4("172.16.0.1"), test_name, "172.16.x private address");
    test_assert(is_valid_ipv4("127.0.0.1"), test_name, "loopback address");
    test_assert(is_valid_ipv4("255.255.255.255"), test_name, "broadcast address");
    test_assert(is_valid_ipv4("0.0.0.0"), test_name, "zero address");
    test_assert(is_valid_ipv4("8.8.8.8"), test_name, "public address");
}

static void test_ipv4_invalid(void) {
    const char *test_name = "IPv4 invalid";

    test_assert(!is_valid_ipv4("256.1.1.1"), test_name, "octet out of range");
    test_assert(!is_valid_ipv4("1.2.3"), test_name, "too few octets");
    test_assert(!is_valid_ipv4("1.2.3.4.5"), test_name, "too many octets");
    test_assert(!is_valid_ipv4("not.an.ip"), test_name, "non-numeric");
    test_assert(!is_valid_ipv4(""), test_name, "empty string");
    test_assert(!is_valid_ipv4(" 192.168.1.1"), test_name, "leading space");
    test_assert(!is_valid_ipv4("192.168.1.1 "), test_name, "trailing space");
}

// ============================================================================
// IPv6 Address Tests
// ============================================================================

static void test_ipv6_addresses(void) {
    const char *test_name = "IPv6 addresses";

    // Valid IPv6 addresses
    test_assert(is_valid_ipv6("2001:db8::1"), test_name, "compressed format");
    test_assert(is_valid_ipv6("::1"), test_name, "loopback");
    test_assert(is_valid_ipv6("fe80::1"), test_name, "link-local");
    test_assert(is_valid_ipv6("::"), test_name, "all zeros compressed");
    test_assert(is_valid_ipv6("2001:0db8:0000:0000:0000:0000:0000:0001"),
                test_name, "full uncompressed format");
}

static void test_ipv6_special(void) {
    const char *test_name = "IPv6 special";

    // IPv4-mapped IPv6 addresses
    test_assert(is_valid_ipv6("::ffff:192.168.1.1"), test_name, "IPv4-mapped IPv6");
    test_assert(is_valid_ipv6("::ffff:c0a8:0101"), test_name, "IPv4-mapped IPv6 hex format");

    // Various valid formats
    test_assert(is_valid_ipv6("2001:db8:85a3::8a2e:370:7334"), test_name, "mixed compression");
    test_assert(is_valid_ipv6("2001:db8:85a3:0:0:8a2e:370:7334"), test_name, "partial zeros");
}

static void test_ipv6_invalid(void) {
    const char *test_name = "IPv6 invalid";

    test_assert(!is_valid_ipv6("gggg::1"), test_name, "invalid hex characters");
    test_assert(!is_valid_ipv6("2001:db8::1::2"), test_name, "double compression");
    test_assert(!is_valid_ipv6(""), test_name, "empty string");
    test_assert(!is_valid_ipv6(" ::1"), test_name, "leading space");
    test_assert(!is_valid_ipv6("localhost"), test_name, "hostname not IPv6");
    test_assert(!is_valid_ipv6("192.168.1.1"), test_name, "IPv4 not IPv6");
}

// ============================================================================
// Address Family Matching Tests
// ============================================================================

static void test_family_matching_unspec(void) {
    const char *test_name = "AF_UNSPEC family matching";

    // AF_UNSPEC should accept both IPv4 and IPv6
    test_assert(address_matches_family("192.168.1.1", AF_UNSPEC),
                test_name, "IPv4 with AF_UNSPEC");
    test_assert(address_matches_family("2001:db8::1", AF_UNSPEC),
                test_name, "IPv6 with AF_UNSPEC");
    test_assert(address_matches_family("::1", AF_UNSPEC),
                test_name, "IPv6 loopback with AF_UNSPEC");
}

static void test_family_matching_ipv4_only(void) {
    const char *test_name = "AF_INET family matching";

    // AF_INET should accept IPv4 only
    test_assert(address_matches_family("192.168.1.1", AF_INET),
                test_name, "IPv4 with AF_INET");
    test_assert(!address_matches_family("2001:db8::1", AF_INET),
                test_name, "IPv6 rejected with AF_INET");
    test_assert(!address_matches_family("::1", AF_INET),
                test_name, "IPv6 loopback rejected with AF_INET");
}

static void test_family_matching_ipv6_only(void) {
    const char *test_name = "AF_INET6 family matching";

    // AF_INET6 should accept IPv6 only
    test_assert(address_matches_family("2001:db8::1", AF_INET6),
                test_name, "IPv6 with AF_INET6");
    test_assert(address_matches_family("::1", AF_INET6),
                test_name, "IPv6 loopback with AF_INET6");
    test_assert(!address_matches_family("192.168.1.1", AF_INET6),
                test_name, "IPv4 rejected with AF_INET6");
}

static void test_family_matching_invalid(void) {
    const char *test_name = "Invalid address family matching";

    // Invalid addresses should always be rejected
    test_assert(!address_matches_family("not.an.ip", AF_UNSPEC),
                test_name, "invalid address with AF_UNSPEC");
    test_assert(!address_matches_family("not.an.ip", AF_INET),
                test_name, "invalid address with AF_INET");
    test_assert(!address_matches_family("not.an.ip", AF_INET6),
                test_name, "invalid address with AF_INET6");
    test_assert(!address_matches_family(NULL, AF_UNSPEC),
                test_name, "NULL address");
    test_assert(!address_matches_family("", AF_UNSPEC),
                test_name, "empty string");
}

// ============================================================================
// Edge Cases
// ============================================================================

static void test_edge_cases(void) {
    const char *test_name = "Edge cases";

    // IPv4-mapped IPv6 should be treated as IPv6
    test_assert(is_valid_ipv6("::ffff:192.168.1.1"),
                test_name, "IPv4-mapped IPv6 is valid IPv6");
    test_assert(!is_valid_ipv4("::ffff:192.168.1.1"),
                test_name, "IPv4-mapped IPv6 is not valid IPv4");

    // Address equivalence (different representations)
    struct in6_addr addr1, addr2;
    test_assert(inet_pton(AF_INET6, "2001:db8::1", &addr1) == 1,
                test_name, "parse compressed IPv6");
    test_assert(inet_pton(AF_INET6, "2001:0db8:0000:0000:0000:0000:0000:0001", &addr2) == 1,
                test_name, "parse full IPv6");
    test_assert(memcmp(&addr1, &addr2, sizeof(addr1)) == 0,
                test_name, "compressed and full IPv6 are equivalent");
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(void) {
    printf("Running bootstrap source binding validation tests...\n\n");

    // IPv4 tests
    test_ipv4_addresses();
    test_ipv4_invalid();

    // IPv6 tests
    test_ipv6_addresses();
    test_ipv6_special();
    test_ipv6_invalid();

    // Family matching tests
    test_family_matching_unspec();
    test_family_matching_ipv4_only();
    test_family_matching_ipv6_only();
    test_family_matching_invalid();

    // Edge cases
    test_edge_cases();

    printf("\n");
    printf("============================================\n");
    if (tests_failed == 0) {
        printf("✅ All %d validation tests passed!\n", tests_passed);
        printf("============================================\n");
        return 0;
    } else {
        printf("❌ %d tests passed, %d tests FAILED\n", tests_passed, tests_failed);
        printf("============================================\n");
        return 1;
    }
}
