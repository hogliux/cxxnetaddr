//
//  NetworkAddress.cpp
//  cxxnetaddr - https://github.com/hogliux/cxxnetaddr
//
//  Created by Fabian Renn-Giles, fabian@fieldingdsp.com on 18th September 2022.
//  Copyright © 2022 Fielding DSP GmbH, All rights reserved.
//
//  Fielding DSP GmbH
//  Jägerstr. 36
//  14467 Potsdam, Germany
//
#include <type_traits>
#include <cassert>
#include <array>
#include <cstring>
#include <stdexcept>
#include <iomanip>
#include <sstream>

#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>

#include "CxxUtilities.hpp"

#include "NetworkAddress.hpp"
#include "NetworkInterface.hpp"

namespace
{
//===============================================================
// Convert between our family enum and UNIX's sa_family_t
constexpr NetworkAddress::Family unix2addr(::sa_family_t family) noexcept {
    switch (family) {
    case AF_UNIX:   return NetworkAddress::Family::unixSocket;
    case AF_PACKET: return NetworkAddress::Family::ethernet;
    case AF_INET:   return NetworkAddress::Family::ipv4;
    case AF_INET6:  return NetworkAddress::Family::ipv6;
    default: break;
    }

    return NetworkAddress::Family::unspecified;
}

constexpr ::sa_family_t addr2unix(NetworkAddress::Family family) noexcept {
    switch (family) {
    case NetworkAddress::Family::unixSocket: return AF_UNIX;
    case NetworkAddress::Family::ethernet:   return AF_PACKET;
    case NetworkAddress::Family::ipv4:       return AF_INET;
    case NetworkAddress::Family::ipv6:       return AF_INET6;
    default: break;
    }

    return AF_UNSPEC;
}

template <NetworkAddress::Family family>
constexpr auto family2type(std::integral_constant<NetworkAddress::Family, family>) {
    if constexpr (family == NetworkAddress::Family::unixSocket) return std::type_identity<sockaddr_un> ();
    if constexpr (family == NetworkAddress::Family::ethernet)   return std::type_identity<sockaddr_ll> ();
    if constexpr (family == NetworkAddress::Family::ipv4)       return std::type_identity<sockaddr_in> ();
    if constexpr (family == NetworkAddress::Family::ipv6)       return std::type_identity<sockaddr_in6>();
}

void assertWrongFamilyType() {
    // You are trying to use a NetworkAddress method that
    // is not supported by the your NetworkAddress' family type
    assert(false);
}

//===============================================================
template <NetworkAddress::Family, bool shouldCopyBack = true> struct SocketImpl;

template <typename S, typename Lambda>
auto sockcall(S& s, Lambda && lambda) {
    static constexpr auto kMaxEnumValue = std::integral_constant<NetworkAddress::Family, NetworkAddress::Family::unspecified>();
    static constexpr auto kIsConstCall = std::is_const_v<std::remove_reference_t<S>>;

    return cxxutils::constexpr_apply(unix2addr(s.ss_family), kMaxEnumValue, [_lambda = std::move(lambda), &s] (auto f) {
        return _lambda(SocketImpl<decltype(f)::value, ! kIsConstCall>(s));
    });
}

//===============================================================
// to avoid C++ undefined behavior, the type-erased sockaddr_storage
// is memcpy to the actual type via RAII. As the actual type lives on the stack,
// all tested compilers optimize away the memcpys.
template <NetworkAddress::Family family, bool shouldCopyBack> 
struct SocketMethods {
    using Child = SocketImpl<family, shouldCopyBack>;
    using Type = decltype(family2type(std::integral_constant<NetworkAddress::Family, family>()))::type;
    using RefType = std::conditional_t<shouldCopyBack,::sockaddr_storage&, ::sockaddr_storage const&>;

    SocketMethods(RefType _storage) noexcept : storage(_storage) {
        assert(storage.ss_family == addr2unix(family));
        std::memcpy(&sock, &storage, sizeof(Type));
    }

    ~SocketMethods() noexcept {
        if constexpr (shouldCopyBack)
            std::memcpy(&storage, &sock, sizeof(Type));
    }

    bool isMulticast() const                   { assertWrongFamilyType(); return false; }
    bool isLinkLocal() const                   { assertWrongFamilyType(); return false; }
    NetworkInterface interface() const         { assertWrongFamilyType(); return {}; }
    std::uint16_t port() const                 { assertWrongFamilyType(); return 0; }
    std::uint16_t protocol() const             { assertWrongFamilyType(); return 0; }
    void setInterface(NetworkInterface const&) { assertWrongFamilyType(); }
    void setPort(std::uint16_t)                { assertWrongFamilyType(); }
    void setProtocol(std::uint16_t)            { assertWrongFamilyType(); }


    auto _this()       { return static_cast<Child*>      (this); }
    auto _this() const { return static_cast<Child const*>(this); }
    
    RefType storage;
    Type sock;
};

//===============================================================
//===============================================================
// The acutal implmentations of NetworkAddress' methods and
// constructors, for all the different socket types, starts here

//===============================================================
// IPv4 and IPv6 common base class
template <NetworkAddress::Family family, bool shouldCopyBack>
struct IPSocketImpl : SocketMethods<family, shouldCopyBack>
{
    using Base = SocketMethods<family, shouldCopyBack>;
    using Child = Base::Child;

    IPSocketImpl(Base::RefType _storage) noexcept : Base(_storage) {}

    ::socklen_t socketLength() const noexcept { return static_cast<::socklen_t>(sizeof(typename Base::Type)); }

    std::string toString() const {
        char host[256];
        char service[8];
        auto const& sock = Base::sock;

        auto status = ::getnameinfo(reinterpret_cast<::sockaddr const*>(&sock),
                                    socketLength(),
                                    host, sizeof(host), service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV);
        
        if (status != 0) {
            throw std::runtime_error(::gai_strerror(status));
        }

        auto result = std::string(host);
        
        if (Base::_this()->port() != 0) {
            if constexpr (family == NetworkAddress::Family::ipv6) {
                result = std::string("[") + result + std::string("]");
            }

            result += std::string(":") + std::string(service);
        }

        return result;
    }
};

//===============================================================
// IPv4 implementation of methods
template <bool shouldCopyBack>
struct SocketImpl<NetworkAddress::Family::ipv4, shouldCopyBack> : IPSocketImpl<NetworkAddress::Family::ipv4, shouldCopyBack> {
    using Base = SocketMethods<NetworkAddress::Family::ipv4, shouldCopyBack>;

    SocketImpl(Base::RefType _storage) noexcept : IPSocketImpl<NetworkAddress::Family::ipv4, shouldCopyBack>(_storage) {}

    void init(std::span<std::uint8_t const, 4> const& octets, std::uint16_t port) noexcept {
        std::uint32_t saddr;
        std::memcpy(&saddr, octets.data(), sizeof(Base::sock.sin_addr.s_addr));
        init(saddr, port);
    }

    void init(std::uint32_t saddr, std::uint16_t port) noexcept {
        Base::sock.sin_addr.s_addr = saddr;
        Base::sock.sin_port = ::htons(port);
    }

    void init(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3, std::uint8_t o4,
              std::uint16_t port) noexcept {
        init(std::array<std::uint8_t, 4>{{o1, o2, o3, o4}}, port);
    }
    
    std::string toString() const                                { return IPSocketImpl<NetworkAddress::Family::ipv4, shouldCopyBack>::toString(); }
    bool isMulticast() const noexcept                           { return (::ntohl(Base::sock.sin_addr.s_addr) & 0xf0000000u) == 0xe0000000u; }
    void setInterface(NetworkInterface const&) noexcept         {}
    std::optional<NetworkInterface> interface() const noexcept  { return {}; }
    bool isLinkLocal() const noexcept                           { return (::ntohl(Base::sock.sin_addr.s_addr) & 0xffff0000u) == 0xa9fe0000u; }
    std::uint16_t port() const noexcept                         { return ::ntohs(Base::sock.sin_port); }
    void setPort(std::uint16_t p) noexcept                      { Base::sock.sin_port = ::htons(p); }
};

//===============================================================
// IPv6 implementation of methods
template <bool shouldCopyBack>
struct SocketImpl<NetworkAddress::Family::ipv6, shouldCopyBack> : IPSocketImpl<NetworkAddress::Family::ipv6, shouldCopyBack> {
    using Base = SocketMethods<NetworkAddress::Family::ipv6, shouldCopyBack>;

    SocketImpl(Base::RefType _storage) noexcept : IPSocketImpl<NetworkAddress::Family::ipv6, shouldCopyBack>(_storage) {}

    void init(std::span<std::uint16_t const, 8u> sw, std::uint16_t port, NetworkInterface const& intf) noexcept {
        static_assert(sizeof(Base::sock.sin6_addr.s6_addr) == sw.size() * sizeof(std::uint16_t));

        auto words = std::invoke([&sw] {
            std::array<std::uint16_t, 8u> result;
            std::copy(sw.begin(), sw.end(), result.begin());
            return result;
        });

        std::transform(words.begin(), words.end(), words.begin(), [] (auto x) { return ::htons(x); });
        std::memcpy(&Base::sock.sin6_addr.s6_addr, words.data(),  words.size() * sizeof(std::uint16_t));
        Base::sock.sin6_port = ::htons(port);
        Base::sock.sin6_scope_id = intf.getIndex();
    }

    void init(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
              std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
              std::uint16_t port, NetworkInterface const& intf) noexcept {
        init(std::array<std::uint16_t, 8>{{w1, w2, w3, w4, w5, w6, w7, w8}}, port, intf);
    }

    std::string toString() const                               { return IPSocketImpl<NetworkAddress::Family::ipv6, shouldCopyBack>::toString(); }
    bool isMulticast() const noexcept                          { return (Base::sock.sin6_addr.s6_addr[0] == 0xff); }
    void setInterface(NetworkInterface const& intf) noexcept   { Base::sock.sin6_scope_id = intf.getIndex(); }
    std::optional<NetworkInterface> interface() const noexcept { return NetworkInterface::fromIntfIndex(Base::sock.sin6_scope_id); }
    bool isLinkLocal() const noexcept                          { return (Base::sock.sin6_addr.s6_addr[0] == 0xfe) && (Base::sock.sin6_addr.s6_addr[1] == 0x80); }
    std::uint16_t port() const noexcept                        { return ::ntohs(Base::sock.sin6_port); }
    void setPort(std::uint16_t p) noexcept                     { Base::sock.sin6_port = ::htons(p); }
};

//===============================================================
// Link-level implementation of methods
template <bool shouldCopyBack>
struct SocketImpl<NetworkAddress::Family::ethernet, shouldCopyBack> : SocketMethods<NetworkAddress::Family::ethernet, shouldCopyBack> {
    using Base = SocketMethods<NetworkAddress::Family::ethernet, shouldCopyBack>;

    SocketImpl(Base::RefType _storage) noexcept : Base(_storage) {}

    void init(std::span<std::uint8_t const, 6u> mac, std::uint16_t protocol, NetworkInterface const& intf) noexcept {
        static_assert(mac.size() <= sizeof(Base::sock.sll_addr));
        std::memcpy(Base::sock.sll_addr, mac.data(), mac.size());
        Base::sock.sll_protocol = ::htons(protocol);
        Base::sock.sll_ifindex = intf.getIndex();
        Base::sock.sll_halen = mac.size();
    }

    void init(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
              std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
              std::uint16_t protocol, NetworkInterface const& intf) noexcept {
        init(std::array<std::uint8_t, 6>{{o1, o2, o3, o4, o5, o6}}, protocol, intf);
    }

    ::socklen_t socketLength() const                           { return sizeof(sockaddr_ll) - sizeof(std::declval<sockaddr_ll>().sll_addr) + Base::sock.sll_halen; }
    bool isMulticast() const noexcept                          { return (Base::sock.sll_addr[0] & 0x1) != 0; }
    void setInterface(NetworkInterface const& intf) noexcept   { Base::sock.sll_ifindex = intf.getIndex(); }
    std::optional<NetworkInterface> interface() const noexcept { return NetworkInterface::fromIntfIndex(Base::sock.sll_ifindex); }
    bool isLinkLocal() const noexcept                          { return true; }
    std::uint16_t protocol() const noexcept                    { return ::ntohs(Base::sock.sll_protocol); }
    void setProtocol(std::uint16_t p) noexcept                 { Base::sock.sll_protocol = ::htons(p); }
    std::string toString() const {
        std::ostringstream ss;

        std::for_each(Base::sock.sll_addr, Base::sock.sll_addr + Base::sock.sll_halen, [&ss] (unsigned char octet) {
            ss << ':' << std::right << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(octet);
        });

        return ss.str().substr(1);
    }
};

//===============================================================
// unix socket implementation of methods
template <bool shouldCopyBack>
struct SocketImpl<NetworkAddress::Family::unixSocket, shouldCopyBack> : SocketMethods<NetworkAddress::Family::unixSocket, shouldCopyBack> {
    using Base = SocketMethods<NetworkAddress::Family::unixSocket, shouldCopyBack>;

    SocketImpl(Base::RefType _storage) noexcept : Base(_storage) {}

    void init(std::string const& path) noexcept {
        static constexpr auto kMaxUnixPathLength = sizeof(::sockaddr_storage) - offsetof(::sockaddr_un, sun_path) - 1;
        assert(path.size() <= kMaxUnixPathLength);
        // see "man 7 unix" on length calculations here 
        auto const len = std::min(path.size(), kMaxUnixPathLength);
        std::memcpy(Base::sock.sun_path, path.substr(0, len).c_str(), len + 1);
    }

    ::socklen_t socketLength() const {
        auto const len = offsetof(::sockaddr_un, sun_path) + ::strnlen(Base::sock.sun_path, sizeof(std::declval<::sockaddr_un>().sun_path)) + 1;
        return static_cast<::socklen_t>(len);
    }
    
    std::string toString() const {
        auto const len = strnlen(Base::sock.sun_path, sizeof(std::declval<::sockaddr_un>().sun_path));
        return std::string(Base::sock.sun_path, Base::sock.sun_path + len);
    }
};
}

//===============================================================
// Glue-code invoking non type-erased methods
NetworkAddress::NetworkAddress() : storage { .ss_family = AF_UNSPEC } {}
NetworkAddress::NetworkAddress(::sa_family_t family) : storage { .ss_family = family } {}
NetworkAddress::NetworkAddress(NetworkAddress const& o) { std::memcpy(&storage, &o.socket(), o.socketLength()); }
NetworkAddress::NetworkAddress(NetworkAddress && o) { std::memcpy(&storage, &o.socket(), o.socketLength()); o.storage.ss_family = AF_UNSPEC; }
NetworkAddress::NetworkAddress(::sockaddr const& other, ::socklen_t len) {
    static constexpr auto kUnspecifiedFamily = std::integral_constant<Family, Family::unspecified>();
    if (! cxxutils::constexpr_apply(unix2addr(other.sa_family), kUnspecifiedFamily, [this, len, &other] (auto f) {
        std::memcpy(&storage, &other, std::min(static_cast<std::size_t>(len), sizeof(typename decltype(family2type(f))::type)));
    })) {
        std::memcpy(&storage, &other, sizeof(::sockaddr));
    }
}
NetworkAddress::NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3, std::uint8_t o4,
                               std::uint16_t port) : storage { .ss_family = AF_INET } {
    SocketImpl<NetworkAddress::Family::ipv4>(storage).init(o1, o2, o3, o4, port);
}
NetworkAddress::NetworkAddress(std::uint32_t saddr, std::uint16_t port) : storage { .ss_family = AF_INET } {
    SocketImpl<NetworkAddress::Family::ipv4>(storage).init(::htonl(saddr), port);
}
#if __cplusplus >= 202002L
NetworkAddress::NetworkAddress(std::span<std::uint8_t const, 4u> ipv4, std::uint16_t port) : storage { .ss_family = AF_INET } {
    SocketImpl<NetworkAddress::Family::ipv4>(storage).init(ipv4, port);
}
#endif

NetworkAddress::NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                               std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                               std::uint16_t port) 
    : NetworkAddress(w1, w2, w3, w4, w5, w6, w7, w8, port, {})
{}
NetworkAddress::NetworkAddress(std::uint16_t w1, std::uint16_t w2, std::uint16_t w3, std::uint16_t w4,
                               std::uint16_t w5, std::uint16_t w6, std::uint16_t w7, std::uint16_t w8,
                               std::uint16_t port, NetworkInterface const& intf)
    : storage { .ss_family = AF_INET6 } {
    SocketImpl<NetworkAddress::Family::ipv6>(storage).init(w1, w2, w3, w4, w5, w6, w7, w8, port, intf);
}

#if __cplusplus >= 202002L
NetworkAddress::NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port)
    : NetworkAddress(ipv6, port, {})
{}
NetworkAddress::NetworkAddress(std::span<std::uint16_t const, 8u> ipv6, std::uint16_t port, NetworkInterface const& intf)
    : storage { .ss_family = AF_INET6 } {
    SocketImpl<NetworkAddress::Family::ipv6>(storage).init(ipv6, port, intf);
}
#endif

NetworkAddress::NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                               std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                               std::uint16_t protocol)
    : NetworkAddress(o1, o2, o3, o4, o5, o6, protocol, NetworkInterface())
{}
NetworkAddress::NetworkAddress(std::uint8_t o1, std::uint8_t o2, std::uint8_t o3,
                               std::uint8_t o4, std::uint8_t o5, std::uint8_t o6,
                               std::uint16_t protocol, NetworkInterface const& intf)
    : storage { .ss_family = AF_PACKET } {
    SocketImpl<NetworkAddress::Family::ethernet>(storage).init(o1, o2, o3, o4, o5, o6, protocol, intf);
}
#if __cplusplus >= 202002L
NetworkAddress::NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t protocol)
    : NetworkAddress(mac, protocol, {})
{}
NetworkAddress::NetworkAddress(std::span<std::uint8_t const, 6u> mac, std::uint16_t protocol, NetworkInterface const& intf)
    : storage { .ss_family = AF_PACKET } {
    SocketImpl<NetworkAddress::Family::ethernet>(storage).init(mac, protocol, intf);
}
#endif

NetworkAddress::NetworkAddress(std::string const& path) : storage { .ss_family = AF_UNIX } {
    SocketImpl<NetworkAddress::Family::unixSocket>(storage).init(path);
}

NetworkAddress& NetworkAddress::operator=(NetworkAddress && o) {
    std::memcpy(&storage, &o.socket(), o.socketLength());
    o.storage.ss_family = AF_UNSPEC;
    return *this;
}

int NetworkAddress::cmp(NetworkAddress const& o) const {
    auto const len1 = static_cast<int>(*sockcall(  storage, [] (auto s) { return s.socketLength(); }));
    auto const len2 = static_cast<int>(*sockcall(o.storage, [] (auto s) { return s.socketLength(); }));

    if (len1 == len2) {
        return std::memcmp(&storage, &o.storage, len1);
    }

    return len1 - len2;
}

bool NetworkAddress::operator==(NetworkAddress const& o) const { return cmp(o) == 0; }
bool NetworkAddress::operator!=(NetworkAddress const& o) const { return cmp(o) != 0; }

#if __cplusplus >= 202002L
std::strong_ordering NetworkAddress::operator<=>(NetworkAddress const& o) const {
    auto const r = cmp(o);

    if      (r < 0) { return std::strong_ordering::less;  }
    else if (r > 0) { return std::strong_ordering::greater; }
    
    return std::strong_ordering::equal;
}
#else
bool NetworkAddress::operator< (NetworkAddress const& o) const { return cmp(o) <  0; }
bool NetworkAddress::operator> (NetworkAddress const& o) const { return cmp(o) >  0; }
bool NetworkAddress::operator<=(NetworkAddress const& o) const { return cmp(o) <= 0; }
bool NetworkAddress::operator>=(NetworkAddress const& o) const { return cmp(o) >= 0; }
#endif

NetworkAddress NetworkAddress::fromUNIXSocketPath(std::string const& path) { return NetworkAddress(path); }
NetworkAddress NetworkAddress::fromPOSIXSocketAddress(::sockaddr const& addr, ::socklen_t len) { return NetworkAddress(addr, len); }
bool NetworkAddress::valid() const                                { return family() != Family::unspecified; }
NetworkAddress::Family NetworkAddress::family() const             { return unix2addr(storage.ss_family); }
std::string NetworkAddress::toString() const                      { return *sockcall(storage, [] (auto s) { return s.toString(); }); }
::sockaddr const& NetworkAddress::socket() const                  { return *reinterpret_cast<::sockaddr const*>(&storage); }
::socklen_t NetworkAddress::socketLength() const                  { return *sockcall(storage, [] (auto s) { return s.socketLength(); }); }
bool NetworkAddress::isMulticast() const                          { return *sockcall(storage, [] (auto s) { return s.isMulticast(); }); }
std::optional<NetworkInterface> NetworkAddress::interface() const { return  sockcall(storage, [] (auto s) { return s.interface(); }); }
bool NetworkAddress::isLinkLocal() const                          { return *sockcall(storage, [] (auto s) { return s.isLinkLocal(); }); }
std::uint16_t NetworkAddress::port() const                        { return *sockcall(storage, [] (auto s) { return s.port(); }); }
std::uint16_t NetworkAddress::protocol() const                    { return *sockcall(storage, [] (auto s) { return s.protocol(); }); }

NetworkAddress NetworkAddress::withInterface(NetworkInterface const& intf) const { 
    NetworkAddress result(*this);
    [[maybe_unused]] auto success = sockcall(result.storage, [&intf] (auto s) { s.setInterface(intf); });
    assert(success);
    return result;
}

NetworkAddress NetworkAddress::withPort(std::uint16_t port) const {
    NetworkAddress result(*this);
    [[maybe_unused]] auto success = sockcall(result.storage, [port] (auto s) { s.setPort(port); });
    assert(success);
    return result;
}

NetworkAddress NetworkAddress::withProtocol(std::uint16_t protocol) const {
    NetworkAddress result(*this);
    [[maybe_unused]] auto success = sockcall(result.storage, [protocol] (auto s) { s.setProtocol(protocol); });
    assert(success); 
    return result;
}
    
std::optional<NetworkAddress> NetworkAddress::fromIPString(std::string const& ipString,
                                                           std::uint16_t defaultPort,
                                                           bool parsePortInString) {
    auto const parts = std::invoke([&ipString] () -> std::array<std::string, 2> {
        auto const result = std::invoke([&ipString] () -> std::array<std::string, 2> {
            auto const firstloc = ipString.find(':');
            auto const lastloc = ipString.rfind(':');
            auto const bracket = ipString.rfind(']');

            if (lastloc == std::string::npos || ((firstloc != lastloc) && ((bracket == std::string::npos) || (bracket > lastloc)))) {
                return {{ipString, {}}};
            }

            auto ipPart = ipString.substr(0, lastloc);
            auto portPort = ipString.substr(lastloc + 1);

            return  {{ipPart, portPort}};
        });

        if (result[0].front() == '[' && result[0].back() == ']') {
            return  {{result[0].substr(1, result[0].size() - 2), result[1]}};
        }

        return result;
    });

    ::addrinfo hints = {};
    
    hints.ai_flags = AI_NUMERICSERV | AI_NUMERICHOST;
    auto const portString = parsePortInString && (! parts[1].empty()) ? parts[1] : std::to_string(defaultPort);

    std::unique_ptr<addrinfo, void (*)(addrinfo*)> ainfo(std::invoke([&] () -> addrinfo* 
    {
        addrinfo* r = nullptr;
        auto retval = ::getaddrinfo(parts[0].c_str(), portString.c_str(), &hints, &r);
        if (retval != 0) {
            return nullptr;
        }

        return r;
    }), freeaddrinfo);

    if (ainfo == nullptr) {
        return {};
    }

    NetworkAddress result;
    std::memcpy(&result.storage, ainfo->ai_addr, ainfo->ai_addrlen);

    return result;
}

std::optional<NetworkAddress> NetworkAddress::fromMACString(std::string const& macString) {
    auto const parts = std::invoke([&macString] () {
        std::istringstream ss(macString);
        std::vector<std::string> result;

        for (std::string s; std::getline(ss, s, ':');) {
            result.emplace_back(s);
        }

        return result;
    });

    if (parts.size() != ETH_ALEN) {
        return {};
    }


    std::array<std::uint8_t, ETH_ALEN> mac;
    for (std::size_t i = 0; i < ETH_ALEN; ++i) {
        auto const& part = parts[i];

        if (part.empty() || part.size() > 2) {
            return {};
        }

        auto const num = std::invoke([&part] () -> std::optional<int> {
            std::istringstream ss(part);
            int result;

            if (ss >> std::hex >> result) {
                return result;
            }
            
            return {};
        });

        if ((! num) || *num < 0 || *num > 255) {
            return {};
        }

        mac[i] = static_cast<std::uint8_t>(*num);
    }

    return NetworkAddress(mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
