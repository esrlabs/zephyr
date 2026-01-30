// Copyright 2025 Accenture.

#pragma once

#include <zephyr/net/net_context.h>

#include <tcp/socket/AbstractServerSocket.h>
#include <platform/estdint.h>

namespace tcp
{
class AbstractSocket;

/**
 * Implementation of AbstractServerSocket for the Zephyr TCP/IP stack.
 *
 *
 * \see AbstractServerSocket
 */
class ZephyrServerSocket : public AbstractServerSocket
{
public:
    ZephyrServerSocket();
    ZephyrServerSocket(ZephyrServerSocket const&)            = delete;
    ZephyrServerSocket& operator=(ZephyrServerSocket const&) = delete;

    /**
     * constructor
     * \param port    the port to open
     * \param providingListener   ISocketProvidingConnectionListener attached
     *          to this ZephyrServerSocket
     */
    ZephyrServerSocket(uint16_t port, ISocketProvidingConnectionListener& providingListener);

    virtual ~ZephyrServerSocket() = default;

    /**
     * \see AbstractServerSocket::accept()
     */
    bool accept() override;

    /**
     * \see AbstractServerSocket::bind()
     */
    bool bind(ip::IPAddress const& localIpAddress, uint16_t port) override;

    /**
     * \see AbstractServerSocket::close()
     */
    void close() override;

    bool isClosed() const override;

private:
    bool createAndBindSocket(ip::IPAddress const& localIpAddress, uint16_t port);
    static void zeth_accepted_cb(struct net_context *new_ctx,
                    struct sockaddr *addr, socklen_t addrlen,
                    int status, void *user_data);
    void acceptedCallback(struct net_context *new_ctx,
                    struct sockaddr *addr, socklen_t addrlen,
                    int status);

    struct net_context* _netContext;
};

} // namespace tcp
