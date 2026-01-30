// Copyright 2025 Accenture.

#include "zephyrEthAdapter/tcp/ZephyrServerSocket.h"

#include "zephyrEthAdapter/tcp/ZephyrSocket.h"
#include "zephyrEthAdapter/utils/EthHelper.h"

#include <ip/IPAddress.h>
#include <tcp/TcpLogger.h>
#include <tcp/socket/ISocketProvidingConnectionListener.h>

using ip::IPAddress;

namespace tcp
{
namespace logger = ::util::logger;

ZephyrServerSocket::ZephyrServerSocket() 
: AbstractServerSocket() 
, _netContext(nullptr)
{}

ZephyrServerSocket::ZephyrServerSocket(
    uint16_t const port, ISocketProvidingConnectionListener& providingListener)
: AbstractServerSocket(port, providingListener)
, _netContext(nullptr)
{}

bool ZephyrServerSocket::bind(IPAddress const& localIpAddress, uint16_t port)
{
    _port = port;
    if ((_port == 0) || (_socketProvidingConnectionListener == nullptr))
    {
        return false;
    }

    if (_netContext != nullptr)
    {
        logger::Logger::error(logger::TCP, " ZephyrServerSocket::bind(): socket already bound");
        return false;
    }
    if (!createAndBindSocket(localIpAddress, _port))
    {
        return false;
    }

    return true;
}

bool ZephyrServerSocket::createAndBindSocket(IPAddress const& localIpAddress, uint16_t port)
{
    int res;
    if (ip::addressFamilyOf(localIpAddress))
    {
        res = net_context_get(AF_INET, SOCK_STREAM, IPPROTO_TCP, &_netContext);
    }
    else
    {
        res = net_context_get(AF_INET6, SOCK_STREAM, IPPROTO_TCP, &_netContext);
    }
    if (res < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrServerSocket::bind(): creation failed");
        return false;
    }
    sockaddr addr = zethutils::toSockAddr(&localIpAddress, port);

    res = net_context_bind(_netContext, &addr, sizeof(addr));
    if (res < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrServerSocket::bind(): bind failed");
        net_context_put(_netContext);
        _netContext = nullptr;
        return false;
    }

    return true;
}

bool ZephyrServerSocket::accept()
{
    if ((_port == 0) || (_socketProvidingConnectionListener == nullptr)
        || ((_netContext != nullptr) && !isClosed()))
    {
        return false;
    }
    if (_netContext == nullptr)
    {
        if (!createAndBindSocket(ip::make_ip4(0, 0, 0, 0), _port))
        {
            return false;
        }
    }
    int res = net_context_listen(_netContext, 1);
    if (res < 0) {
        return false;
    }

    res = net_context_accept(_netContext, ZephyrServerSocket::zeth_accepted_cb, K_NO_WAIT, this);
    if (res < 0) {
        return false;
    }
 
    logger::Logger::info(logger::TCP, "Socket prepared at port %d", _port);
    return true;
}

void ZephyrServerSocket::zeth_accepted_cb(struct net_context *new_ctx,
                  struct sockaddr *addr, socklen_t addrlen,
                  int status, void *user_data)
{
    ZephyrServerSocket* self = static_cast<ZephyrServerSocket*>(user_data);
    self->acceptedCallback(new_ctx, addr, addrlen, status);
}

void ZephyrServerSocket::acceptedCallback(struct net_context *new_ctx,
                  struct sockaddr *addr, socklen_t /*addrlen*/,
                  int status)
{
    if (status == 0)
    {
        if (_socketProvidingConnectionListener != nullptr)
        {
            uint16_t remotePort;
            ip::IPAddress remoteAddress;

            if (addr->sa_family == AF_INET)
            {
                struct sockaddr_in* addr_in = reinterpret_cast<struct sockaddr_in *>(addr);
                remotePort = addr_in->sin_port;
                remoteAddress = zethutils::fromSockAddr(*addr);
            }
            else
            {
                struct sockaddr_in6* addr_in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
                remotePort = addr_in6->sin6_port;
                remoteAddress = zethutils::fromSockAddr(*addr);
            }
            ZephyrSocket* const pSocket = static_cast<ZephyrSocket*>(
                _socketProvidingConnectionListener->getSocket(remoteAddress, remotePort));
            if (pSocket == nullptr)
            {
                logger::Logger::debug(
                    logger::TCP,
                    "ZephyrServerSocket::acceptCallback(): SocketProvidingConnectionListener provided no "
                    "socket");
                return;
            }
            logger::Logger::debug(
                logger::TCP, "ZephyrServerSocket: accepted connection at port %d", _port);

            pSocket->open(new_ctx);

            _socketProvidingConnectionListener->connectionAccepted(*pSocket);
        }
    }
    else
    {
        logger::Logger::error(
            logger::TCP, "ZephyrServerSocket::acceptCallback(): error during accept");
    }
}

bool ZephyrServerSocket::isClosed() const
{
    if (_netContext != nullptr)
    {
        return (net_context_get_state(_netContext) == NET_CONTEXT_IDLE);
    }
    return true;
}

void ZephyrServerSocket::close()
{
    if (_netContext == nullptr)
    {
        logger::Logger::debug(logger::TCP, "ZephyrServerSocket::close(): Socket not initialized or already closed");
        return;
    }
    // Reset Callbacks
    (void)net_context_accept(_netContext, NULL, K_NO_WAIT, NULL);
    if (net_context_put(_netContext) < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrServerSocket::close(): Error closing socket %p", _netContext);
        return;
    }
    _netContext = nullptr;
    logger::Logger::debug(logger::TCP, "ZephyrServerSocket::close(): Socket %p closed", _netContext);
    return;
}

} // namespace tcp
