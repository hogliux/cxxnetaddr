//
//  NetworkAddress_test.hpp
//  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
//
//  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
//  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
//
//  Fielding DSP GmbH
//  Jägerstr. 36
//  14467 Potsdam, Germany
//
#include <gtest/gtest.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "NetworkAddress.hpp"
#include "NetworkInterface.hpp"

// Test creating an invalid NetworkAddress
TEST(NetworkAddressTest, InvalidAddress) {
    NetworkAddress addr;
    EXPECT_FALSE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::unspecified);
}

// Test creating an IPv4 address using individual octets
TEST(NetworkAddressTest, IPv4AddressCreationWithOctets) {
    NetworkAddress addr(192, 168, 1, 1, 8080);
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv4);
    EXPECT_EQ(addr.port(), 8080);
    EXPECT_EQ(addr.toString(), "192.168.1.1:8080");
}

// Test creating an IPv4 address using a 32-bit integer
TEST(NetworkAddressTest, IPv4AddressCreationWithUint32) {
    uint32_t ip = (192 << 24) | (168 << 16) | (1 << 8) | 1; // Equivalent to 192.168.1.1
    NetworkAddress addr(ip, 80);
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv4);
    EXPECT_EQ(addr.port(), 80);
    EXPECT_EQ(addr.toString(), "192.168.1.1:80");
}

// Test checking if an IPv4 address is multicast
TEST(NetworkAddressTest, IPv4MulticastAddress) {
    NetworkAddress addr(224, 0, 0, 1); // Multicast address
    EXPECT_TRUE(addr.isMulticast());
}

// Test creating an IPv6 address using individual words
TEST(NetworkAddressTest, IPv6AddressCreation) {
    NetworkAddress addr(
        0x2001, 0x0db8, 0x85a3, 0x0000,
        0x0000, 0x8a2e, 0x0370, 0x7334,
        443
    );
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv6);
    EXPECT_EQ(addr.port(), 443);
    EXPECT_EQ(addr.toString(), "[2001:db8:85a3::8a2e:370:7334]:443");
}

// Test checking if an IPv6 address is link-local
TEST(NetworkAddressTest, IPv6LinkLocalAddress) {
    NetworkAddress addr(
        0xfe80, 0x0000, 0x0000, 0x0000,
        0x0202, 0xb3ff, 0xfe1e, 0x8329
    );
    EXPECT_TRUE(addr.isLinkLocal());
}

// Test creating a MAC address using octets
TEST(NetworkAddressTest, MACAddressCreation) {
    NetworkAddress addr(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x0800);
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ethernet);
    EXPECT_EQ(addr.protocol(), 0x0800);
    EXPECT_EQ(addr.toString(), "00:1A:2B:3C:4D:5E");
}

// Test checking if a MAC address is multicast
TEST(NetworkAddressTest, MACMulticastAddress) {
    NetworkAddress addr(0x01, 0x00, 0x5E, 0x00, 0x00, 0xFB); // Multicast MAC address
    EXPECT_TRUE(addr.isMulticast());
}

// Test comparison operators for IPv4 addresses
TEST(NetworkAddressTest, IPv4AddressComparison) {
    NetworkAddress addr1(192, 168, 1, 1, 80);
    NetworkAddress addr2(192, 168, 1, 1, 80);
    NetworkAddress addr3(10, 0, 0, 1, 80);

    EXPECT_EQ(addr1, addr2);
    EXPECT_NE(addr1, addr3);
    EXPECT_GT(addr1, addr3);
    EXPECT_LT(addr3, addr1);
}

// Test creating a UNIX socket address
TEST(NetworkAddressTest, UNIXSocketAddressCreation) {
    auto addr = NetworkAddress::fromUNIXSocketPath("/tmp/socket");
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::unixSocket);
    EXPECT_EQ(addr.toString(), "/tmp/socket");
}

// Test modifying the port of an IPv4 address
TEST(NetworkAddressTest, IPv4AddressWithPort) {
    NetworkAddress addr(127, 0, 0, 1);
    EXPECT_EQ(addr.port(), 0);
    auto addrWithPort = addr.withPort(8080);
    EXPECT_EQ(addrWithPort.port(), 8080);
    EXPECT_EQ(addrWithPort.toString(), "127.0.0.1:8080");
}

// Test fromIPString method for IPv4
TEST(NetworkAddressTest, FromIPv4String) {
    auto addrOpt = NetworkAddress::fromIPString("192.168.1.100:3000");
    ASSERT_TRUE(addrOpt.has_value());
    auto addr = addrOpt.value();
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv4);
    EXPECT_EQ(addr.port(), 3000);
    EXPECT_EQ(addr.toString(), "192.168.1.100:3000");
}

// Test fromIPString method for IPv6
TEST(NetworkAddressTest, FromIPv6StringWithoutPort) {
    auto addrOpt = NetworkAddress::fromIPString("2001:0db8::1");
    ASSERT_TRUE(addrOpt.has_value());
    auto addr = addrOpt.value();
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv6);
    EXPECT_EQ(addr.toString(), "2001:db8::1");
}

// Test fromIPString method for IPv6
TEST(NetworkAddressTest, FromIPv6StringWithoutPortWithSquareBrackets) {
    auto addrOpt = NetworkAddress::fromIPString("[2001:0db8::1]");
    ASSERT_TRUE(addrOpt.has_value());
    auto addr = addrOpt.value();
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv6);
    EXPECT_EQ(addr.toString(), "2001:db8::1");
}

// Test fromIPString method for IPv6
TEST(NetworkAddressTest, FromIPv6StringWithPort) {
    auto addrOpt = NetworkAddress::fromIPString("[2001:0db8::1]:8080");
    ASSERT_TRUE(addrOpt.has_value());
    auto addr = addrOpt.value();
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv6);
    EXPECT_EQ(addr.port(), 8080);
    EXPECT_EQ(addr.toString(), "[2001:db8::1]:8080");
}

// Test fromMACString method
TEST(NetworkAddressTest, FromMACString) {
    auto addrOpt = NetworkAddress::fromMACString("00:1A:2B:3C:4D:5E");
    ASSERT_TRUE(addrOpt.has_value());
    auto addr = addrOpt.value();
    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ethernet);
    EXPECT_EQ(addr.toString(), "00:1A:2B:3C:4D:5E");
}

// Test isLinkLocal for MAC address
TEST(NetworkAddressTest, MACLinkLocalAddress) {
    NetworkAddress addr(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);
    EXPECT_TRUE(addr.isLinkLocal());
}

// Test comparison operators for MAC addresses
TEST(NetworkAddressTest, MACAddressComparison) {
    NetworkAddress addr1(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);
    NetworkAddress addr2(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);
    NetworkAddress addr3(0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA);

    EXPECT_EQ(addr1, addr2);
    EXPECT_NE(addr1, addr3);
    EXPECT_LT(addr1, addr3);
}

// Test creating an IPv6 address and checking if it's multicast
TEST(NetworkAddressTest, IPv6MulticastAddress) {
    NetworkAddress addr(
        0xff02, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0001
    );
    EXPECT_TRUE(addr.isMulticast());
}

// Test copying and moving NetworkAddress
TEST(NetworkAddressTest, CopyAndMoveSemantics) {
    NetworkAddress addr1(127, 0, 0, 1, 80);
    NetworkAddress addr2(addr1); // Copy constructor
    EXPECT_EQ(addr1, addr2);

    NetworkAddress addr3(std::move(addr1)); // Move constructor
    EXPECT_EQ(addr3, addr2);
    EXPECT_FALSE(addr1.valid()); // addr1 should now be invalid
}

// Test constructing NetworkAddress from POSIX sockaddr
TEST(NetworkAddressTest, FromPOSIXSocketAddress) {
    ::sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.1.1", &sockaddr.sin_addr);

    NetworkAddress addr = NetworkAddress::fromPOSIXSocketAddress(
        *reinterpret_cast<struct ::sockaddr*>(&sockaddr), sizeof(sockaddr)
    );

    EXPECT_TRUE(addr.valid());
    EXPECT_EQ(addr.family(), NetworkAddress::Family::ipv4);
    EXPECT_EQ(addr.port(), 8080);
    EXPECT_EQ(addr.toString(), "192.168.1.1:8080");
}

// Test withProtocol method for MAC address
TEST(NetworkAddressTest, MACAddressWithProtocol) {
    NetworkAddress addr(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);
    EXPECT_EQ(addr.protocol(), 0);
    auto addrWithProtocol = addr.withProtocol(0x0806); // ARP protocol
    EXPECT_EQ(addrWithProtocol.protocol(), 0x0806);
}

// Test withInterface method (without depending on actual interfaces)
TEST(NetworkAddressTest, WithInterface) {
    NetworkInterface dummyIntf; // Assuming default constructor creates an invalid interface
    NetworkAddress addr(
        0xfe80, 0x0000, 0x0000, 0x0000,
        0x0202, 0xb3ff, 0xfe1e, 0x8329
    );
    auto addrWithIntf = addr.withInterface(dummyIntf);
    EXPECT_FALSE(addrWithIntf.interface().has_value());
}

// Test that isMulticast returns false for unicast addresses
TEST(NetworkAddressTest, UnicastAddressIsNotMulticast) {
    NetworkAddress ipv4Addr(192, 168, 1, 1);
    NetworkAddress ipv6Addr(
        0x2001, 0x0db8, 0x85a3, 0x0000,
        0x0000, 0x8a2e, 0x0370, 0x7334
    );
    NetworkAddress macAddr(0x00, 0x1A, 0x2B, 0x3C, 0x4D, 0x5E);

    EXPECT_FALSE(ipv4Addr.isMulticast());
    EXPECT_FALSE(ipv6Addr.isMulticast());
    EXPECT_FALSE(macAddr.isMulticast());
}
