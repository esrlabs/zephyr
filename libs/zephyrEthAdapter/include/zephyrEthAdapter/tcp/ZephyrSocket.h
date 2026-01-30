// Copyright 2025 Accenture.

#pragma once

#include <zephyr/net/net_context.h>
#include <zephyr/kernel.h>
#include <ip/IPEndpoint.h>
#include <ip/IPAddress.h>
#include <tcp/IDataListener.h>
#include <tcp/socket/AbstractSocket.h>
#include <platform/estdint.h>

extern "C" {
    struct SocketSuspendContext{};
    int zeth_net_tcp_set_option(struct net_context *context,
        int option,
        const void *value, size_t len);
    int zeth_get_send_data_total(struct net_context *context);
    int zeth_tcp_suspend(struct net_context *context, struct SocketSuspendContext *state);
    struct net_context* zeth_tcp_resume(const struct SocketSuspendContext *state);
}
namespace tcp
{
#define Z_TCP_NODELAY   1
#define Z_TCP_KEEPALIVE 2
#define Z_TCP_KEEPIDLE  3
#define Z_TCP_KEEPINTVL 4
#define Z_TCP_KEEPCNT   5

/**
 * Socket implementation for tcp stack of Zephyr.
 *
 *
 * \see AbstractSocket
 */
class ZephyrSocket : public AbstractSocket
{
public:
    ZephyrSocket();
    ZephyrSocket(ZephyrSocket const&)            = delete;
    ZephyrSocket& operator=(ZephyrSocket const&) = delete;

    virtual ~ZephyrSocket() = default;

    bool open(struct net_context* context);

    // AbstractSocket
    ErrorCode
    connect(ip::IPAddress const& ipAddr, uint16_t port, ConnectedDelegate delegate) override;

    ErrorCode close() override;

    void abort() override;

    ErrorCode flush() override;

    uint8_t read(uint8_t& byte) override;

    size_t read(uint8_t* buffer, size_t n) override;

    void discardData() override;

    ErrorCode send(::etl::span<uint8_t const> const& data) override;

    size_t available() override;

    bool isClosed() const override;

    bool isEstablished() const override;

    ErrorCode bind(ip::IPAddress const& ipAddr, uint16_t const port) override;

    ip::IPAddress getRemoteIPAddress() const override;

    ip::IPAddress getLocalIPAddress() const override;

    uint16_t getRemotePort() const override;

    uint16_t getLocalPort() const override;

    void disableNagleAlgorithm() override;

    void enableKeepAlive(uint32_t idle, uint32_t interval, uint32_t probes) override;

    void disableKeepAlive() override;

private:
    static void zeth_received_cb(struct net_context *ctx,
                  struct net_pkt *pkt,
                  union net_ip_header *ip_hdr,
                  union net_proto_header *proto_hdr,
                  int status,
                  void *user_data);

    void receivedCallback(struct net_context *ctx,
                  struct net_pkt *pkt,
                  union net_ip_header *ip_hdr,
                  union net_proto_header *proto_hdr,
                  int status);

    static void zeth_connected_cb(struct net_context *ctx, int status, void *user_data);

    void connectedCallback(struct net_context *ctx, int status);

    static void zeth_send_cb(struct net_context *context, int status, void *user_data);

    void sendCallback(struct net_context *context, int status);

    ConnectedDelegate _delegate;
    bool _connecting;
    bool _isAborted;
    struct net_context* _netContext;
    struct k_fifo _receive_q;

};

} // namespace tcp
