// Copyright 2025 Accenture.

#include "platform/estdint.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

extern "C"
{

static const struct device *const uart_console_dev =
    DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

int32_t getByteFromStdin()
{
    unsigned char c = -1;
    uart_poll_in(uart_console_dev, &c);
    return c;
}

void putByteToStdout(uint8_t const byte)
{
    uart_poll_out(uart_console_dev, byte);
}

}