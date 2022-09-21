//
//  NetworkAddress.hpp
//  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
//
//  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
//  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
//
//  Fielding DSP GmbH
//  Jägerstr. 36
//  14467 Potsdam, Germany
//
#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include <numeric>

#if __cplusplus >= 202002L
#include <span>
#include <compare>
#endif

#include <sys/socket.h>

class NetworkInterface;

class NetworkAddress
{
public:
    //===============================================================
    enum class Family
    {
        ipv4,
        ipv6,
        ethernet,
        unixSocket,

        unspecified
    };

    //===============================================================
    /** Create an invlalid addrdss */
    NetworkAddress();

    /** Create IPv4 address */
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3, std::uint8_t o4,
                   std::uint16_t port = 0);
    NetworkAddress(std::uint32_t saddr, std::uint16_t port = 0);
   #if __cplusplus >= 202002L
    NetworkAddress(std::span<std::uint8_t const, 4u> ipv4, std::uint16_t port = 0);
   #endif

    /** Create IPv6 address */
    NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                   std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                   std::uint16_t port = 0);
    NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                   std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                   std::uint16_t port, NetworkInterface const& intf);
   #if __cplusplus >= 202002L
    NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port = 0);
    NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port, NetworkInterface const& intf);
   #endif

    /** Create MAC address */
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                   std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                   std::uint16_t protocol = 0);
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                   std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                   std::uint16_t protocol, NetworkInterface const& intf);
   #if __cplusplus >= 202002L
    NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t port = 0);
    NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t port, NetworkInterface const& intf);
   #endif

    /** Create UNIX address */
    static NetworkAddress fromUNIXSocketPath(std::string const& path);

    //===============================================================
    /** methods that work on all socket types */
    static NetworkAddress fromPOSIXSocketAddress(::sockaddr const& addr, ::socklen_t len = std::numeric_limits<::socklen_t>::max());
    bool valid() const;
    Family family() const;
    std::string toString() const;
    ::sockaddr const& socket() const;
    ::socklen_t socketLength() const;

    //===============================================================
    // Only valid for IP addresses and MAC addresses
    bool isMulticast() const;
    
    std::optional<NetworkInterface> interface() const;
    NetworkAddress withInterface(NetworkInterface const&) const;

    /** Is the address a link-local address? Always true for MAC addresses. */
    bool isLinkLocal() const;

    //===============================================================
    // Only valid for IP addresses. Does *not* do a DNS lookup.
    // Will use defaultPort if there is no port section in the string
    // or if parsePortInString is false.
    static std::optional<NetworkAddress> fromIPString(std::string const& ipString,
                                                      std::uint16_t defaultPort = 0,
                                                      bool parsePortInString = true);

    std::uint16_t port() const;
    NetworkAddress withPort(std::uint16_t port) const;
    
    //===============================================================
    // Only valid for MAC addresses
    static std::optional<NetworkAddress> fromMACString(std::string const& macString);
    std::uint16_t protocol() const;
    NetworkAddress withProtocol(std::uint16_t protocol) const;

    //===============================================================
    NetworkAddress(NetworkAddress const&);
    NetworkAddress(NetworkAddress&&);
    NetworkAddress& operator=(NetworkAddress const&) = delete;
    NetworkAddress& operator=(NetworkAddress&&);

    bool operator==(NetworkAddress const&) const;
    bool operator!=(NetworkAddress const&) const;
   #if __cplusplus >= 202002L
    std::strong_ordering operator<=>(NetworkAddress const&) const;
   #else
    bool operator< (NetworkAddress const&) const;
    bool operator> (NetworkAddress const&) const;
    bool operator<=(NetworkAddress const&) const;
    bool operator>=(NetworkAddress const&) const;
   #endif
private:
    NetworkAddress(::sockaddr const&, ::socklen_t);
    NetworkAddress(::sa_family_t);
    NetworkAddress(std::string const&);
    int cmp(NetworkAddress const&) const;
    ::sockaddr_storage storage;
};
