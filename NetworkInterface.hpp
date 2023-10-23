//
//  NetworkInterface.hpp
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
#include <vector>
#include <compare>
#include <optional>
#include <string>

#include "NetworkAddress.hpp"

/**
 * @class NetworkInterface
 * @brief Represents a network interface on the system.
 *
 * This class provides an abstraction over system network interfaces,
 * allowing retrieval of interface information such as name, type,
 * and associated network addresses.
 */
class NetworkInterface
{
public:
    /**
     * @brief Default constructor.
     *
     * Creates an invalid NetworkInterface object.
     */
    NetworkInterface();

    //===============================================================
    /**
     * @enum Type
     * @brief Enumerates the types of network interfaces.
     */
    enum class Type
    {
        loopback,   /**< Loopback interface */
        ethernet,   /**< Ethernet interface */
        wifi,       /**< Wi-Fi interface */
        cellular,   /**< Cellular interface */
        vpn,        /**< VPN interface */

        unknown     /**< Unknown interface type */
    };

    //===============================================================
    /**
     * @brief Creates a NetworkInterface from its name.
     *
     * @param intf The name of the interface.
     * @return An optional NetworkInterface.
     */
    static std::optional<NetworkInterface> fromString(std::string const& intf);

    /**
     * @brief Creates a NetworkInterface from its index.
     *
     * @param index The index of the interface.
     * @return An optional NetworkInterface.
     */
    static std::optional<NetworkInterface> fromIntfIndex(std::uint32_t index);

    //===============================================================
    /**
     * @brief Retrieves all available network interfaces.
     *
     * @return A vector of NetworkInterface objects.
     */
    static std::vector<NetworkInterface> getAllInterfaces();

    //===============================================================
    /**
     * @brief Checks if the NetworkInterface is valid.
     *
     * @return True if valid, false otherwise.
     */
    bool isValid() const;

    /**
     * @brief Gets the index of the network interface.
     *
     * @return The interface index.
     */
    std::uint32_t getIndex() const;

    /**
     * @brief Gets the name of the network interface.
     *
     * @return The interface name.
     */
    std::string const& getName() const;

    /**
     * @brief Gets the type of the network interface.
     *
     * @return The interface type.
     */
    Type getType() const;

    //===============================================================
    /**
     * @brief Retrieves the primary IP address of the interface.
     *
     * @param preferIPv6 Set to true to prefer IPv6 addresses.
     * @return An optional NetworkAddress representing the IP address.
     */
    std::optional<NetworkAddress> getIPAddress(bool preferIPv6 = false) const;

    /**
     * @brief Retrieves the MAC address of the interface.
     *
     * Equivalent to getAddresses(NetworkAddress::Family::ethernet)[0].
     *
     * @return An optional NetworkAddress representing the MAC address.
     */
    std::optional<NetworkAddress> getMACAddress() const;

    //===============================================================
    /**
     * @brief Retrieves all addresses associated with the interface.
     *
     * @param family The address family to filter by. If filter is
     *        Family::unspecified, then all types of addresses are
     *        returned
     * @return A vector of NetworkAddress objects.
     */
    std::vector<NetworkAddress> getAddresses(NetworkAddress::Family family = NetworkAddress::Family::unspecified) const;

    //===============================================================
    NetworkInterface(NetworkInterface const&);
    NetworkInterface(NetworkInterface&&);
    ~NetworkInterface();
    NetworkInterface& operator=(NetworkInterface&&);

    bool operator==(NetworkInterface const& other) const;
    bool operator!=(NetworkInterface const& other) const;

#if __cplusplus >= 202002L
    std::strong_ordering operator<=>(NetworkInterface const&) const;
#else
    bool operator< (NetworkInterface const& other) const;
    bool operator> (NetworkInterface const& other) const;
    bool operator<=(NetworkInterface const& other) const;
    bool operator>=(NetworkInterface const& other) const;
#endif

private:
    NetworkInterface(std::string const& name);
    std::string name = {};
};
