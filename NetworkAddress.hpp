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

/**
 * @class NetworkAddress
 * @brief Represents a network address (IPv4, IPv6, MAC, or UNIX socket).
 *
 * This class provides an abstraction over various network address types,
 * encapsulating their representation and associated operations.
 */
class NetworkAddress
{
public:
    //===============================================================
    /**
     * @enum Family
     * @brief Enumerates the types of network address families.
     */
    enum class Family
    {
        ipv4,          /**< IPv4 address */
        ipv6,          /**< IPv6 address */
        ethernet,      /**< Ethernet (MAC) address */
        unixSocket,    /**< UNIX socket address */

        unspecified    /**< Unspecified address family */
    };

    //===============================================================
    /**
     * @brief Default constructor.
     *
     * Creates an invalid NetworkAddress object.
     */
    NetworkAddress();

    // Constructors for IPv4 addresses
    /**
     * @brief Constructs an IPv4 address from octets.
     *
     * @param o1 First octet.
     * @param o2 Second octet.
     * @param o3 Third octet.
     * @param o4 Fourth octet.
     * @param port Port number (default is 0).
     */
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3, std::uint8_t o4,
                   std::uint16_t port = 0);

    /**
     * @brief Constructs an IPv4 address from a 32-bit integer.
     *
     * @param saddr The IPv4 address as a 32-bit integer in host byte order.
     * @param port Port number (default is 0).
     */
    NetworkAddress(std::uint32_t saddr, std::uint16_t port = 0);

#if __cplusplus >= 202002L
    /**
     * @brief Constructs an IPv4 address from a span of octets.
     *
     * @param ipv4 Span of 4 octets representing the IPv4 address.
     * @param port Port number (default is 0).
     */
    NetworkAddress(std::span<std::uint8_t const, 4u> ipv4, std::uint16_t port = 0);
#endif

    // Constructors for IPv6 addresses
    /**
     * @brief Constructs an IPv6 address from words.
     *
     * @param w1-w8 Eight 16-bit words representing the IPv6 address.
     * @param port Port number (default is 0).
     */
    NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                   std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                   std::uint16_t port = 0);

    /**
     * @brief Constructs a scoped IPv6 address from words.
     *
     * @param w1-w8 Eight 16-bit words representing the IPv6 address.
     * @param port Port number.
     * @param intf NetworkInterface associated with the scope.
     */
    NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                   std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                   std::uint16_t port, NetworkInterface const& intf);

#if __cplusplus >= 202002L
    /**
     * @brief Constructs an IPv6 address from a span of words.
     *
     * @param ipv6 Span of 8 words representing the IPv6 address.
     * @param port Port number (default is 0).
     */
    NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port = 0);

    /**
     * @brief Constructs a scoped IPv6 address from a span of words.
     *
     * @param ipv6 Span of 8 words representing the IPv6 address.
     * @param port Port number.
     * @param intf NetworkInterface associated with the scope.
     */
    NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port, NetworkInterface const& intf);
#endif

    // Constructors for MAC addresses
    /**
     * @brief Constructs a MAC address from octets.
     *
     * @param o1-o6 Six octets representing the MAC address.
     * @param protocol Protocol identifier (default is 0).
     */
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                   std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                   std::uint16_t protocol = 0);

    /**
     * @brief Constructs a MAC address with an associated interface.
     *
     * @param o1-o6 Six octets representing the MAC address.
     * @param protocol Protocol identifier.
     * @param intf NetworkInterface associated with the MAC address.
     */
    NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                   std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                   std::uint16_t protocol, NetworkInterface const& intf);

#if __cplusplus >= 202002L
    /**
     * @brief Constructs a MAC address from a span of octets.
     *
     * @param mac Span of 6 octets representing the MAC address.
     * @param port Protocol identifier (default is 0).
     */
    NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t port = 0);

    /**
     * @brief Constructs a MAC address with an associated interface from a span of octets.
     *
     * @param mac Span of 6 octets representing the MAC address.
     * @param port Protocol identifier.
     * @param intf NetworkInterface associated with the MAC address.
     */
    NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t port, NetworkInterface const& intf);
#endif

    // Constructor for UNIX socket addresses
    /**
     * @brief Constructs a UNIX socket address from a file path.
     *
     * @param path The UNIX socket file path.
     * @return A NetworkAddress representing the UNIX socket.
     */
    static NetworkAddress fromUNIXSocketPath(std::string const& path);

    //===============================================================
    /** Methods applicable to all address types */

    /**
     * @brief Constructs a NetworkAddress from a POSIX sockaddr structure.
     *
     * @param addr The sockaddr structure.
     * @param maxlen Maximum number of bytes to parse in sockaddr. Usually not required as size can
     *               be determined from data inside ::sockaddr itself.
     * @return A NetworkAddress representing the address.
     */
    static NetworkAddress fromPOSIXSocketAddress(::sockaddr const& addr, ::socklen_t maxlen = std::numeric_limits<::socklen_t>::max());

    /**
     * @brief Checks if the NetworkAddress is valid.
     *
     * @return True if valid, false otherwise.
     */
    bool valid() const;

    /**
     * @brief Gets the address family of the NetworkAddress.
     *
     * @return The address family.
     */
    Family family() const;

    /**
     * @brief Converts the NetworkAddress to a string representation.
     *
     * @return The string representation of the address.
     */
    std::string toString() const;

    /**
     * @brief Gets a const reference to the underlying sockaddr.
     *
     * @return A const reference to the sockaddr.
     */
    ::sockaddr const& socket() const;

    /**
     * @brief Gets the length of the underlying sockaddr.
     *
     * @return The length of the sockaddr structure.
     */
    ::socklen_t socketLength() const;

    //===============================================================
    /** Methods applicable to IP and MAC addresses */

    /**
     * @brief Checks if the address is a multicast address.
     *
     * @return True if multicast, false otherwise.
     */
    bool isMulticast() const;

    /**
     * @brief Gets the associated network interface.
     *
     * @return An optional NetworkInterface.
     */
    std::optional<NetworkInterface> interface() const;

    /**
     * @brief Creates a new NetworkAddress with the specified interface.
     *
     * @param intf The NetworkInterface to associate.
     * @return A new NetworkAddress with the interface.
     */
    NetworkAddress withInterface(NetworkInterface const& intf) const;

    /**
     * @brief Checks if the address is link-local.
     *
     * Always true for MAC addresses.
     *
     * @return True if link-local, false otherwise.
     */
    bool isLinkLocal() const;

    //===============================================================
    /** Methods applicable to IP addresses */

    /**
     * @brief Parses an IP address from a string.
     *
     * Does not perform DNS resolution.
     *
     * @param ipString The IP address string.
     * @param defaultPort The default port if not specified in the string.
     * @param parsePortInString Whether to parse the port from the string.
     * @return An optional NetworkAddress.
     */
    static std::optional<NetworkAddress> fromIPString(std::string const& ipString,
                                                      std::uint16_t defaultPort = 0,
                                                      bool parsePortInString = true);

    /**
     * @brief Gets the port number of the address.
     *
     * @return The port number.
     */
    std::uint16_t port() const;

    /**
     * @brief Creates a new NetworkAddress with the specified port.
     *
     * @param port The port number to set.
     * @return A new NetworkAddress with the port.
     */
    NetworkAddress withPort(std::uint16_t port) const;

    //===============================================================
    /** Methods applicable to MAC addresses */

    /**
     * @brief Parses a MAC address from a string.
     *
     * @param macString The MAC address string.
     * @return An optional NetworkAddress.
     */
    static std::optional<NetworkAddress> fromMACString(std::string const& macString);

    /**
     * @brief Gets the protocol identifier.
     *
     * @return The protocol identifier.
     */
    std::uint16_t protocol() const;

    /**
     * @brief Creates a new NetworkAddress with the specified protocol.
     *
     * @param protocol The protocol identifier to set.
     * @return A new NetworkAddress with the protocol.
     */
    NetworkAddress withProtocol(std::uint16_t protocol) const;

    //===============================================================
    NetworkAddress(NetworkAddress const&);
    NetworkAddress(NetworkAddress&&);
    NetworkAddress& operator=(NetworkAddress&&);

    bool operator==(NetworkAddress const& other) const;
    bool operator!=(NetworkAddress const& other) const;
#if __cplusplus >= 202002L
    std::strong_ordering operator<=>(NetworkAddress const&) const;
#else
    bool operator< (NetworkAddress const& other) const;
    bool operator> (NetworkAddress const& other) const;
    bool operator<=(NetworkAddress const& other) const;
    bool operator>=(NetworkAddress const& other) const;
#endif

private:
    NetworkAddress(::sockaddr const& addr, ::socklen_t len);
    NetworkAddress(::sa_family_t family);
    NetworkAddress(std::string const& path);
    int cmp(NetworkAddress const& other) const;
    ::sockaddr_storage storage;
};
