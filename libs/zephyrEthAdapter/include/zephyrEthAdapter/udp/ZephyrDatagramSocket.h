// Copyright 2025 Accenture.

#pragma once

#include <zephyr/net/net_context.h>

#include <ip/IPAddress.h>
#include <udp/socket/AbstractDatagramSocket.h>

#include <etl/optional.h>
#include <platform/estdint.h>

namespace udp
{
/**
 * Implementation of AbstractDatagramSocket using Zephyr Network Stack.
 *
 *
 * \see AbstractDatagramSocket
 */
class ZephyrDatagramSocket : public AbstractDatagramSocket
{
public:
    ZephyrDatagramSocket();

    virtual ~ZephyrDatagramSocket() = default;

    /**
     * \see AbstractDatagramSocket::bind();
     */
    ErrorCode bind(ip::IPAddress const* pIpAddress, uint16_t port) override;

    /**
     * \see AbstractDatagramSocket::join();
     */
    ErrorCode join(ip::IPAddress const& groupAddr) override;

    /**
     * \see AbstractDatagramSocket::isBound();
     */
    bool isBound() const override;

    /**
     * \see AbstractDatagramSocket::close();
     */
    void close() override;

    /**
     * \see AbstractDatagramSocket::isClosed();
     */
    bool isClosed() const override;

    /**
     * \see AbstractDatagramSocket::connect();
     */
    ErrorCode
    connect(ip::IPAddress const& address, uint16_t port, ip::IPAddress* pLocalAddress) override;

    /**
     * \see AbstractDatagramSocket::disconnect();
     */
    void disconnect() override;

    /**
     * \see AbstractDatagramSocket::isConnected();
     */
    bool isConnected() const override;

    /**
     * \see AbstractDatagramSocket::read();
     */
    size_t read(uint8_t* buffer, size_t n) override;

    /**
     * \see AbstractDatagramSocket::send();
     */
    ErrorCode send(::etl::span<uint8_t const> const& data) override;

    /**
     * \see AbstractDatagramSocket::send();
     */
    ErrorCode send(DatagramPacket const& packet) override;

    /**
     * \see AbstractDatagramSocket::getIPAddress();
     */
    ip::IPAddress const* getIPAddress() const override;

    /**
     * \see AbstractDatagramSocket::getLocalIPAddress();
     */
    ip::IPAddress const* getLocalIPAddress() const override;

    /**
     * \see AbstractDatagramSocket::getPort();
     */
    uint16_t getPort() const override;

    /**
     * \see AbstractDatagramSocket::getLocalPort();
     */
    uint16_t getLocalPort() const override;

    void receivedCallback(struct net_context *ctx,
                           struct net_pkt *pkt,
                           union net_ip_header *ip_hdr,
                           union net_proto_header *proto_hdr,
                           int status);

private:
    static void zsock_received_cb(struct net_context *ctx,
                           struct net_pkt *pkt,
                           union net_ip_header *ip_hdr,
                           union net_proto_header *proto_hdr,
                           int status,
                           void *user_data);

    int _socket;
    struct net_context* _netContext;
    struct net_pkt* _currentPkt;
    bool _isBound;
    bool _isConnected;
    etl::optional<sockaddr> _localAddr;
    etl::optional<sockaddr> _peerAddr;
    ip::IPAddress::Family _addressFamily;
};

} // namespace udp
