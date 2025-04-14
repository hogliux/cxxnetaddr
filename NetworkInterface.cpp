//
//  NetworkInterface.cpp
//  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
//
//  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
//  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
//
//  Fielding DSP GmbH
//  Jägerstr. 36
//  14467 Potsdam, Germany
//
#include <memory>
#include <functional>
#include <cstring>

#include <net/if.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/ioctl.h>

#if __APPLE__
 #include <net/if_dl.h>
 #include <net/if_types.h>
 static constexpr ::sa_family_t kFamilyLinkLevel = AF_LINK;
#elif __linux__
 #include <linux/if_packet.h>
 static constexpr ::sa_family_t kFamilyLinkLevel = AF_PACKET;
#endif

#include "CxxUtilities.hpp"
#include "NetworkInterface.hpp"

namespace
{
std::unique_ptr<::ifaddrs, void (*)(::ifaddrs*)> getifaddrs_wrapper() {
    ::ifaddrs* addrs;
    if (::getifaddrs(&addrs) < 0) {
        return {nullptr, nullptr};
    }

    return {addrs, ::freeifaddrs};
}
}

NetworkInterface::NetworkInterface() = default;
NetworkInterface::NetworkInterface(NetworkInterface const& intf) : name(intf.name) {}
NetworkInterface::NetworkInterface(std::string const& str) : name(str) {}
NetworkInterface::NetworkInterface(NetworkInterface && intf) : name(std::move(intf.name)) {}
NetworkInterface::~NetworkInterface() = default;
NetworkInterface& NetworkInterface::operator=(NetworkInterface && intf) {
    name = std::move(intf.name);
    return *this;
}
bool NetworkInterface::isValid() const                    { return !name.empty(); }
std::uint32_t NetworkInterface::getIndex() const          { return ::if_nametoindex(name.c_str()); }
std::string const &NetworkInterface::getName() const      { return name; }

std::optional<NetworkInterface> NetworkInterface::fromString(std::string const& intf) {
    if (::if_nametoindex(intf.c_str()) == 0) {
        return {};
    }

    return NetworkInterface(intf);
}

std::optional<NetworkInterface> NetworkInterface::fromIntfIndex(std::uint32_t index) {
    char buffer[IF_NAMESIZE + 1];
    if (auto const* str = ::if_indextoname(index, buffer); str != nullptr) {
        return NetworkInterface(std::string(str));
    }

    return {};
}

std::vector<NetworkInterface> NetworkInterface::getAllInterfaces() {
    if (auto intfs = getifaddrs_wrapper(); intfs != nullptr) {
        std::vector<NetworkInterface> result;

        for (auto* intf = intfs.get(); intf != nullptr; intf = intf->ifa_next) {
            if (intf->ifa_addr == nullptr) {
                continue;
            }

            auto const family = intf->ifa_addr->sa_family;

            if (family != kFamilyLinkLevel && family != AF_INET && family != AF_INET6) {
                continue;
            }

            std::string name(intf->ifa_name);

            if (std::find_if(result.begin(), result.end(), [&name] (auto x) { return x.name == name; }) == result.end()) {
                result.emplace_back(NetworkInterface(std::string(intf->ifa_name)));
            }
        }

        return result;
    }

    return {};
}

NetworkInterface::Type NetworkInterface::getType() const {
    if (auto intfs = getifaddrs_wrapper(); intfs != nullptr && (! name.empty())) {
        auto has_mac = false;

        for (auto* intf = intfs.get(); intf != nullptr; intf = intf->ifa_next) {
            if (std::string(intf->ifa_name) != name) {
                continue;
            }

            if (intf->ifa_addr == nullptr) {
                continue;
            }

            auto const family = intf->ifa_addr->sa_family;

            if ((intf->ifa_flags & IFF_LOOPBACK) != 0) {
                return Type::loopback;
            }

            has_mac |= (family == kFamilyLinkLevel);

           #if __linux__
            if (family == kFamilyLinkLevel || family == AF_INET || family == AF_INET6) {
                ::ifreq req = {};
                std::strcpy(req.ifr_name, intf->ifa_name);

                auto sock = cxxutils::callAtEndOfScope(::socket(AF_INET, SOCK_STREAM, 0), [] (int fd) { ::close(fd); });
                if (::ioctl(sock, /*SIOCGIWNAME*/0x8B01, &req) != -1) {
                    return Type::wifi;
                }
            }
           #endif
        }

        return has_mac ? Type::ethernet : Type::unknown;
    }

    return Type::unknown;
}

std::optional<NetworkAddress> NetworkInterface::getIPAddress(bool preferIPv6) const {
    std::optional<NetworkAddress> addr;

    if (auto intfs = getifaddrs_wrapper(); intfs != nullptr) {
        for (auto* intf = intfs.get(); intf != nullptr; intf = intf->ifa_next) {
            if (std::string(intf->ifa_name) != name) {
                continue;
            }

            if (intf->ifa_addr == nullptr) {
                continue;
            }

            auto const family = intf->ifa_addr->sa_family;

            if (family != AF_INET && family != AF_INET6) {
                continue;
            }

            if ((family == AF_INET6 && preferIPv6) || (family == AF_INET && (! preferIPv6))) {
                auto retval = NetworkAddress::fromPOSIXSocketAddress(*intf->ifa_addr);
                
                if (retval.valid()) {
                    return retval;
                }
            }

            addr  = NetworkAddress::fromPOSIXSocketAddress(*intf->ifa_addr);
        }
    }

    return addr;
}

std::optional<NetworkAddress> NetworkInterface::getMACAddress() const {
    auto const macs = getAddresses(NetworkAddress::Family::ethernet);
    
    if (macs.empty()) {
        return {};
    }

    return macs.front();
}

std::vector<NetworkAddress> NetworkInterface::getAddresses(NetworkAddress::Family family) const {
    std::vector<NetworkAddress> result;

     if (auto intfs = getifaddrs_wrapper(); intfs != nullptr) {
        for (auto* intf = intfs.get(); intf != nullptr; intf = intf->ifa_next) {
            if (std::string(intf->ifa_name) != name) {
                continue;
            }

            if (intf->ifa_addr == nullptr) {
                continue;
            }

            auto const intf_family = intf->ifa_addr->sa_family;

            if  ((family == NetworkAddress::Family::ethernet && intf_family == kFamilyLinkLevel) ||
                 (family == NetworkAddress::Family::ipv4 && intf_family == AF_INET) ||
                 (family == NetworkAddress::Family::ipv6 && intf_family == AF_INET6) ||
                 (family == NetworkAddress::Family::unspecified)) {
                auto addr = NetworkAddress::fromPOSIXSocketAddress(*intf->ifa_addr);

                if (addr.valid()) {
                    result.emplace_back(addr);
                }
            }
        }
    }

    return result;
}

bool NetworkInterface::operator==(NetworkInterface const& o) const { return name == o.name; }
bool NetworkInterface::operator!=(NetworkInterface const& o) const { return name != o.name; }

#if __cplusplus >= 202002L
std::strong_ordering NetworkInterface::operator<=>(NetworkInterface const& o) const {
    return name <=> o.name;
}
#else
bool NetworkInterface::operator< (NetworkInterface const& o) const { return name <  o.name; }
bool NetworkInterface::operator> (NetworkInterface const& o) const { return name >  o.name; }
bool NetworkInterface::operator<=(NetworkInterface const& o) const { return name <= o.name; }
bool NetworkInterface::operator>=(NetworkInterface const& o) const { return name >= o.name; }
#endif
