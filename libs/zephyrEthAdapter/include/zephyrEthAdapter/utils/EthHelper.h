// Copyright 2025 Accenture.

#pragma once

#include <ip/IPAddress.h>
#include <ip/to_str.h>
#include <zephyr/net/socket.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/posix/fcntl.h>
#include <udp/UdpLogger.h>
#include <etl/memory.h>

namespace zethutils
{
namespace logger = ::util::logger;

static inline bool setblocking(int fd, bool val)
{
    int fl, res;

    fl = zsock_fcntl(fd, F_GETFL, 0);
    if (fl == -1)
    {
        logger::Logger::error(logger::UDP, "fcntl(F_GETFL): %d", errno);
        return false;
    }

    if (val)
    {
        fl &= ~O_NONBLOCK;
    }
    else
    {
        fl |= O_NONBLOCK;
    }

    res = zsock_fcntl(fd, F_SETFL, fl);
    if (fl == -1)
    {
        logger::Logger::error(logger::UDP, "fcntl(F_GETFL): %d", errno);
        return false;
    }
    return true;
}

inline sockaddr toSockAddr(ip::IPAddress const* ip, uint16_t port)
{
    sockaddr addr{};
    if (!ip)
    {
        addr.sa_family = AF_UNSPEC;
        return addr;
    }
    if (addressFamilyOf(*ip) == ip::IPAddress::IPV4)
    {
        sockaddr_in* ipv4_addr = reinterpret_cast<sockaddr_in*>(&addr);
        ipv4_addr->sin_family = AF_INET;
        ipv4_addr->sin_port   = htons(port);
        char strBuffer[INET_ADDRSTRLEN];
        zsock_inet_pton(AF_INET, ip::to_str(*ip, strBuffer).data(), &ipv4_addr->sin_addr);
    }
    else
    {
        sockaddr_in6* ipv6_addr = reinterpret_cast<sockaddr_in6*>(&addr);
        ipv6_addr->sin6_family = AF_INET6;
        ipv6_addr->sin6_port   = htons(port);
        char strBuffer[INET6_ADDRSTRLEN];
        zsock_inet_pton(AF_INET6, ip::to_str(*ip, strBuffer).data(), &ipv6_addr->sin6_addr);
    }
    return addr;
}

inline ip::IPAddress fromSockAddr(sockaddr const& addr)
{
    if (addr.sa_family == AF_INET)
    {
        sockaddr_in address = reinterpret_cast<const sockaddr_in&>(addr);
        ip::IPAddress ipAddr = ip::make_ip4(
            address.sin_addr.s4_addr[0],
            address.sin_addr.s4_addr[1],
            address.sin_addr.s4_addr[2],
            address.sin_addr.s4_addr[3]);
        return ipAddr;
    }
    else
    {
        sockaddr_in6 address = reinterpret_cast<const sockaddr_in6&>(addr);
        ip::IPAddress ipAddr = ip::make_ip6(
            address.sin6_addr.s6_addr32[0],
            address.sin6_addr.s6_addr32[1],
            address.sin6_addr.s6_addr32[2],
            address.sin6_addr.s6_addr32[3]);
        return ipAddr;
    }
}

inline ip::IPAddress fromSockAddr(sockaddr_ptr const& addr)
{
    if (addr.family == AF_INET)
    {
        sockaddr_in_ptr* address = net_sin_ptr(&addr);
        ip::IPAddress ipAddr = ip::make_ip4(
            address->sin_addr->s4_addr[0],
            address->sin_addr->s4_addr[1],
            address->sin_addr->s4_addr[2],
            address->sin_addr->s4_addr[3]);
        return ipAddr;
    }
    else
    {
        sockaddr_in6_ptr* address = net_sin6_ptr(&addr);
        ip::IPAddress ipAddr = ip::make_ip6(
            address->sin6_addr->s6_addr32[0],
            address->sin6_addr->s6_addr32[1],
            address->sin6_addr->s6_addr32[2],
            address->sin6_addr->s6_addr32[3]);
        return ipAddr;
    }
}

} // namespace zethutils
