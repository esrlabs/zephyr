// Copyright 2025 Accenture.

#include <../subsys/net/ip/tcp_internal.h>
#include "zephyrEthAdapter/tcp/ZephyrTcpWrapper.h"

struct SocketSuspendContext
{
    // net_context fields
    struct sockaddr_ptr local;
    struct sockaddr remote;
    uint16_t flags;
    void *user_data;
    // struct tcp fields
    uint8_t state;
    uint32_t seq;
    uint32_t ack;
    uint16_t send_win;
    uint32_t send_data_total;
    struct tcp_options recv_options;
	struct tcp_options send_options;
    uint32_t unacked_len;
    uint16_t recv_win;
};

int zeth_net_tcp_set_option(struct net_context *context,
    int option,
    const void *value, size_t len)
{
    return net_tcp_set_option(context, option, value, len);
}

int zeth_get_send_data_total(struct net_context *context)
{
    struct tcp *tcp_state = (struct tcp *)context->tcp;
    return tcp_state->send_data_total;
}

int zeth_tcp_suspend(struct net_context *context, struct SocketSuspendContext *state)
{
    struct tcp *tcp_state = context->tcp;

    // Save net_context state
    memcpy(&state->local, &context->local, sizeof(struct sockaddr_ptr));
    memcpy(&state->remote, &context->remote, sizeof(struct sockaddr));
    state->flags = context->flags;
    state->user_data = context->user_data;

    // Save struct tcp state
    state->state = tcp_state->state;
    state->seq = tcp_state->seq;
    state->ack = tcp_state->ack;
    state->send_win = tcp_state->send_win;
    state->send_data_total = tcp_state->send_data_total;
    memcpy(&state->recv_options, &tcp_state->recv_options, sizeof(struct tcp_options));
    memcpy(&state->send_options, &tcp_state->send_options, sizeof(struct tcp_options));
    state->unacked_len = tcp_state->unacked_len;
    state->recv_win = tcp_state->recv_win;

    return 0;
}

struct net_context* zeth_tcp_resume(const struct SocketSuspendContext *state)
{
    struct net_context *context;
    int ret = net_context_get(state->local.family, SOCK_STREAM, IPPROTO_TCP, &context);
    if (ret < 0)
    {
        return NULL;
    }
    // Restore net_context state
    memcpy(&context->local, &state->local, sizeof(struct sockaddr));
    memcpy(&context->remote, &state->remote, sizeof(struct sockaddr));
    context->flags = state->flags;
    context->user_data = state->user_data;

    // Restore struct tcp state
    struct tcp *tcp_state = context->tcp;
    tcp_state->state = state->state;
    tcp_state->seq = state->seq;
    tcp_state->ack = state->ack;
    tcp_state->send_win = state->send_win;
    tcp_state->send_data_total = state->send_data_total;
    memcpy(&tcp_state->recv_options, &state->recv_options, sizeof(struct tcp_options));
    memcpy(&tcp_state->send_options, &state->send_options, sizeof(struct tcp_options));
    tcp_state->unacked_len = state->unacked_len;
    tcp_state->recv_win = state->recv_win;

    return context;
}
