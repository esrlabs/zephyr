// Copyright 2025 Accenture.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct SocketSuspendContext;

int zeth_net_tcp_set_option(struct net_context *context,
    int option,
    const void *value, size_t len);

int zeth_get_send_data_total(struct net_context *context);

int zeth_tcp_suspend(struct net_context *context, struct SocketSuspendContext *state);

struct net_context* zeth_tcp_resume(const struct SocketSuspendContext *state);

#ifdef __cplusplus
}
#endif
