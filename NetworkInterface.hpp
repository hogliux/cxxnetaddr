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

#include <vector>
#include <compare>
#include <optional>
#include <string>

#include "NetworkAddress.hpp"

class NetworkInterface
{
public:
    NetworkInterface();

    //===============================================================
    enum class Type
    {
        loopback,
        ethernet,
        wifi,
        cellular,
        vpn,

        unknown
    };

    //===============================================================
    static std::optional<NetworkInterface> fromString(std::string const& intf);
    static std::optional<NetworkInterface> fromIntfIndex(std::uint32_t index);

    //===============================================================
    static std::vector<NetworkInterface> getAllInterfaces();

    //===============================================================
    bool isValid() const;
    std::uint32_t getIndex() const;
    std::string const& getName() const;
    Type getType() const;

    //===============================================================
    std::optional<NetworkAddress> getIPAddress(bool preferIPv6 = false) const;
    /** same as getAddresses(NetworkAddress::Family::ethernet)[0] */
    std::optional<NetworkAddress> getMACAddress() const;
    
    //===============================================================
    std::vector<NetworkAddress> getAddresses(NetworkAddress::Family family = NetworkAddress::Family::unspecified) const;
    
    //===============================================================
    NetworkInterface(NetworkInterface const&);
    NetworkInterface(NetworkInterface&&);
    ~NetworkInterface();
    NetworkInterface& operator=(NetworkInterface const&) = delete;
    NetworkInterface& operator=(NetworkInterface&&);

    bool operator==(NetworkInterface const& o) const;
    bool operator!=(NetworkInterface const& o) const;
   #if __cplusplus >= 202002L
    std::strong_ordering operator<=>(NetworkInterface const&) const;
   #else
    bool operator< (NetworkInterface const& o) const;
    bool operator> (NetworkInterface const& o) const;
    bool operator<=(NetworkInterface const& o) const;
    bool operator>=(NetworkInterface const& o) const;
   #endif
private:
    NetworkInterface(std::string const&);

    std::string name = {};
};
