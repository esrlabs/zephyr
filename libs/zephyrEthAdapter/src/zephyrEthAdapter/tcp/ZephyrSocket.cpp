// Copyright 2025 Accenture.

#include "zephyrEthAdapter/tcp/ZephyrSocket.h"
#include <zephyr/net/net_pkt.h>
#include "zephyrEthAdapter/utils/EthHelper.h"

#include <ip/to_str.h>
#include <tcp/IDataListener.h>
#include <tcp/IDataSendNotificationListener.h>
#include <tcp/TcpLogger.h>

namespace tcp
{
namespace logger = ::util::logger;

using ::ip::IPAddress;
using ::ip::IPEndpoint;
using ::ip::make_ip4;

ZephyrSocket::ZephyrSocket()
: _delegate()
, _connecting(false)
, _isAborted(false)
, _netContext(nullptr)
{}

bool ZephyrSocket::open(struct net_context* context)
{
    //TASK_ASSERT_HOOK();

    if (isClosed())
    {
        _netContext = context;
    }
    else
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::open() called in illegal state != CLOSED");
        return false;
    }
    k_fifo_init(&_receive_q);
    int res = net_context_recv(_netContext, ZephyrSocket::zeth_received_cb, K_NO_WAIT, this);
    if (res < 0) {
        return false;
    }
    logger::Logger::info(logger::TCP, "ZephyrSocket::open() success");
    return true;
}

void ZephyrSocket::zeth_connected_cb(struct net_context *ctx, int status, void *user_data)
{
    ZephyrSocket* self = static_cast<ZephyrSocket*>(user_data);
    self->connectedCallback(ctx, status);
}

void ZephyrSocket::connectedCallback(struct net_context *ctx, int status)
{
    if (status == 0)
    {
        // Connection successful
        _connecting = false;
        _isAborted = false;
        _netContext = ctx;

        logger::Logger::info(logger::TCP, "ZephyrSocket::zeth_connected_cb(): Connection successful");
    }
    else
    {
        // Connection failed
        _connecting = false;
        _isAborted = true;

        logger::Logger::error(logger::TCP, "ZephyrSocket::zeth_connected_cb(): Connection failed with status %d", status);
    }
}

AbstractSocket::ErrorCode
ZephyrSocket::connect(ip::IPAddress const& ipAddr, uint16_t port, ConnectedDelegate delegate)
{
    k_timeout_t timeout = K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT);
    if (net_context_get_state(_netContext) == NET_CONTEXT_CONNECTED)
    {
        return AbstractSocket::ErrorCode::SOCKET_ERR_OK;
    }
    /* For STREAM sockets net_context_recv() only installs
    * recv callback w/o side effects, and it has to be done
    * first to avoid race condition, when TCP stream data
    * arrives right after connect.
    */
    int ret = net_context_recv(_netContext, ZephyrSocket::zeth_received_cb,
                K_NO_WAIT, this);

    if (ret < 0)
    {
        return AbstractSocket::ErrorCode::SOCKET_ERR_NOT_OK;
    }

    sockaddr addr = zethutils::toSockAddr(&ipAddr, port);
    ret = net_context_connect(_netContext, &addr, sizeof(addr), ZephyrSocket::zeth_connected_cb,
        timeout, this);

    if (ret < 0)
    {
        return AbstractSocket::ErrorCode::SOCKET_ERR_NOT_OK;
    }
    _delegate = delegate;
    return AbstractSocket::ErrorCode::SOCKET_ERR_OK;
}

bool ZephyrSocket::isClosed() const
{
    if (_netContext != nullptr)
    {
        return (net_context_get_state(_netContext) == NET_CONTEXT_IDLE);
    }
    return true;
}

bool ZephyrSocket::isEstablished() const
{
    if (_netContext != nullptr)
    {
        return (net_context_get_state(_netContext) == NET_CONTEXT_CONNECTED);
    }
    return false;
}

void ZephyrSocket::zeth_received_cb(struct net_context *ctx,
                  struct net_pkt *pkt,
                  union net_ip_header *ip_hdr,
                  union net_proto_header *proto_hdr,
                  int status,
                  void *user_data)
{
    ZephyrSocket* self = static_cast<ZephyrSocket*>(user_data);
    self->receivedCallback(ctx, pkt, ip_hdr, proto_hdr, status);
}

void ZephyrSocket::receivedCallback(struct net_context * /*ctx*/,
                struct net_pkt *pkt,
                union net_ip_header * /*ip_hdr*/,
                union net_proto_header * /*proto_hdr*/,
                int status)
{
    logger::Logger::debug(logger::TCP, " ZephyrSocket::receivedCallback(): status=%d pkt=%x", status, pkt);

    if (status != 0)
    {
        (void)close();
        logger::Logger::error(logger::TCP, "ZephyrSocket::receivedCallback(): Error: %d", status);
    }
    if (pkt != nullptr)
    {
        int len = net_pkt_remaining_data(pkt);
        k_fifo_put(&_receive_q, pkt);
        _dataListener->dataReceived(len);
    }
    else
    {
        // EOF
        (void)close();
        if (_dataListener != nullptr)
        {
            _dataListener->connectionClosed(IDataListener::ErrorCode::ERR_CONNECTION_CLOSED);
        }
    }
}

AbstractSocket::ErrorCode ZephyrSocket::close()
{
    if (_netContext == nullptr)
    {
        logger::Logger::debug(logger::TCP, "ZephyrSocket::close(): Socket already closed");
        return AbstractSocket::ErrorCode::SOCKET_ERR_OK;
    }
    // Reset Callbacks
    (void)net_context_recv(_netContext, NULL, K_NO_WAIT, NULL);
    if (net_context_put(_netContext) < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::close(): Error closing socket %p", _netContext);
        return AbstractSocket::ErrorCode::SOCKET_ERR_NOT_OK;
    }
    _netContext = nullptr;
    logger::Logger::debug(logger::TCP, "ZephyrSocket::close(): Socket %p closed", _netContext);
    return AbstractSocket::ErrorCode::SOCKET_ERR_OK;
}

void ZephyrSocket::abort()
{
}

AbstractSocket::ErrorCode ZephyrSocket::flush()
{
    return AbstractSocket::ErrorCode::SOCKET_ERR_NOT_OK;
}

uint8_t ZephyrSocket::read(uint8_t& byte)
{
    struct net_pkt* currentPkt = (net_pkt *)k_fifo_peek_head(&_receive_q);
    if (currentPkt == nullptr)
    {
        return 0;
    }
    uint8_t value[1];
    int res = net_pkt_read(currentPkt, &value[0], 1);
    if (res == 0)    //read successful
    {
        byte = value[0];
        if (net_pkt_remaining_data(currentPkt) == 0)
        {
            // If the packet is fully consumed, release it and update the receive window
            (void)k_fifo_get(&_receive_q, K_NO_WAIT);
            size_t fullPktLength = net_pkt_get_len(currentPkt);
            net_pkt_unref(currentPkt);
            net_context_update_recv_wnd(_netContext, fullPktLength);
        }
    }
    else
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::read(): Error reading byte: %d", res);
        return 0;
    }
    return 1;
}

size_t ZephyrSocket::read(uint8_t* buffer, size_t n)
{
    struct net_pkt* currentPkt = nullptr;
    size_t bytes2Read = n;

    while (bytes2Read > 0)
    {
        currentPkt = (net_pkt *)k_fifo_peek_head(&_receive_q);
        if (currentPkt == nullptr)
        {
            // Return the number of bytes read so far
            return n - bytes2Read;
        }
        size_t bytes2ReadFromPkt = net_pkt_remaining_data(currentPkt);
        int res;
        if (bytes2Read < bytes2ReadFromPkt)
        {
            bytes2ReadFromPkt = bytes2Read;
        }
        if (buffer != nullptr)
        {
            res = net_pkt_read(currentPkt, buffer, bytes2ReadFromPkt);
        }
        else
        {
            res = net_pkt_skip(currentPkt, bytes2ReadFromPkt);
        }
        if (res == 0)    //read successful
        {
            if (bytes2Read > bytes2ReadFromPkt)
            {
                bytes2Read -= bytes2ReadFromPkt;
            }
            else
            {
                bytes2Read = 0;
            }
            // If the packet is fully consumed, release it and update the receive window
            if (net_pkt_remaining_data(currentPkt) == 0)
            {
                // dequeue packet
                (void)k_fifo_get(&_receive_q, K_NO_WAIT);
                size_t fullPktLength = net_pkt_get_len(currentPkt);
                net_pkt_unref(currentPkt);
                net_context_update_recv_wnd(_netContext, fullPktLength);
            }
        }
        else
        {
            logger::Logger::error(logger::TCP, "ZephyrSocket::read(): Error reading packet: %d", res);
            return 0;
        }
    }
    return n;
}

void ZephyrSocket::discardData()
{
    while (k_fifo_is_empty(&_receive_q) == 0)  // there are packets in the queue
    {
        struct net_pkt* currentPkt = (net_pkt *)k_fifo_get(&_receive_q, K_NO_WAIT);
        size_t pktLength = net_pkt_get_len(currentPkt);
        net_pkt_unref(currentPkt);
        net_context_update_recv_wnd(_netContext, pktLength);
    }
}

void ZephyrSocket::zeth_send_cb(struct net_context *context, int status, void *user_data)
{
    ZephyrSocket* self = static_cast<ZephyrSocket*>(user_data);
    self->sendCallback(context, status);
}

void ZephyrSocket::sendCallback(struct net_context * /*context*/, int status)
{
    logger::Logger::debug(logger::TCP, " ZephyrSocket::sendCallback(): status=%d", status);

    if (status > 0)
    {
        _sendNotificationListener->dataSent(status, IDataSendNotificationListener::SendResult::DATA_SENT);
    }
}

AbstractSocket::ErrorCode ZephyrSocket::send(::etl::span<uint8_t const> const& data)
{
    if (_netContext == nullptr)
    {
        logger::Logger::error(logger::TCP, " ZephyrSocket::send(): socket not bound");
        return ErrorCode::SOCKET_ERR_NOT_OK;
    }
    if (net_context_send(_netContext, data.data(), 
        // contrary to documentation, returns number of bytes sent, or negative value on error
                         data.size(), ZephyrSocket::zeth_send_cb, K_NO_WAIT /* timeout unused */, NULL ) < 0)
    {
        return ErrorCode::SOCKET_ERR_NOT_OK;
    }
    return ErrorCode::SOCKET_ERR_OK;
}

size_t ZephyrSocket::available()
{
    size_t totBufSize = CONFIG_NET_BUF_TX_COUNT * CONFIG_NET_BUF_DATA_SIZE;
    size_t usedBufSize = zeth_get_send_data_total(_netContext);
    return totBufSize - usedBufSize;
}

AbstractSocket::ErrorCode ZephyrSocket::bind(ip::IPAddress const& ipAddr, uint16_t const port)
{
    int ret;
    sockaddr addr = zethutils::toSockAddr(&ipAddr, port);
    ret = net_context_bind(_netContext, &addr, sizeof(addr));
    if (ret < 0) {
        return AbstractSocket::ErrorCode::SOCKET_ERR_NOT_OK;
    }
    return AbstractSocket::ErrorCode::SOCKET_ERR_OK;
}

ip::IPAddress ZephyrSocket::getRemoteIPAddress() const
{
    return zethutils::fromSockAddr(_netContext->remote);
}

ip::IPAddress ZephyrSocket::getLocalIPAddress() const
{
    return zethutils::fromSockAddr(_netContext->local);
}

uint16_t ZephyrSocket::getRemotePort() const
{
    if (_netContext->local.family == AF_INET6)
    {
        uint16_t remotePort = net_sin6(&_netContext->remote)->sin6_port;
        return ntohs(remotePort);
    }
    uint16_t remotePort = net_sin(&_netContext->remote)->sin_port;
    return ntohs(remotePort);
}

uint16_t ZephyrSocket::getLocalPort() const
{
    if (_netContext->local.family == AF_INET6)
    {
        uint16_t localPort = net_sin6_ptr(&_netContext->local)->sin6_port;
        return ntohs(localPort);
    }
    uint16_t localPort = net_sin_ptr(&_netContext->local)->sin_port;
    return ntohs(localPort);
}

void ZephyrSocket::disableNagleAlgorithm()
{
    if (_netContext == nullptr)
    {
        return;
    }
    int yes = 1;
    int ret = zeth_net_tcp_set_option(_netContext, Z_TCP_NODELAY, (void*)&yes, sizeof(int));
    if (ret < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::disableNagleAlgorithm(): Failed: %d", ret);
    }
}

void ZephyrSocket::enableKeepAlive(uint32_t /*idle*/, uint32_t /*interval*/, uint32_t /*probes*/)
{
    if (_netContext == nullptr)
    {
        return;
    }
    int yes = 1;
    int ret = zeth_net_tcp_set_option(_netContext, Z_TCP_KEEPALIVE, (void*)&yes, sizeof(int));
    if (ret < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::enableKeepAlive(): Failed: %d", ret);
    }
}

void ZephyrSocket::disableKeepAlive()
{
    if (_netContext == nullptr)
    {
        return;
    }
    int no = 0;
    int ret = zeth_net_tcp_set_option(_netContext, Z_TCP_KEEPALIVE, (void*)&no, sizeof(int));
    if (ret < 0)
    {
        logger::Logger::error(logger::TCP, "ZephyrSocket::disableKeepAlive(): Failed: %d", ret);
    }
}

} // namespace tcp
