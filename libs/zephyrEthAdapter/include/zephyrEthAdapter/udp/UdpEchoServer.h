// Copyright 2025 Accenture.

#pragma once

#include <async/Async.h>
#include <udp/IDataListener.h>
#include <zephyrEthAdapter/udp/ZephyrDatagramSocket.h>

namespace udp
{

/**
 * A simple class that listens on a given IP address and port and will echo received data back to
 * the sender.
 */
class UdpEchoServer : public ::udp::IDataListener
{
public:
    /**
     * Constructs an instance of UdpEchoServer at a given \p ipAddr and \p rxPort.
     * \param ipAddr    IP address of the netif to listen on. IP address 0.0.0.0 will listen on all
     *                  netifs.
     * \param rxPort    Port to listen on.
     */
    UdpEchoServer(
        ::ip::IPAddress const& ipAddr, uint16_t rxPort, ::async::ContextType asyncContext);

    /**
     * Binds to the IP address and port passed in the constructor.
     * \return true if UdpEchoServer is successfully started and listening, false otherwise.
     */
    bool start();

    /**
     * Closes the listening socket of this UdpEchoServer.
     */
    void stop();

    void dataReceived(
        ::udp::AbstractDatagramSocket& socket,
        ::ip::IPAddress sourceAddress,
        uint16_t sourcePort,
        ::ip::IPAddress destinationAddress,
        uint16_t length) override;

private:
    ip::IPAddress const _ipAddr;
    uint16_t const _rxPort;
    ::udp::ZephyrDatagramSocket _socket;
    uint8_t _receiveData[1518U];
    ::async::ContextType _context;
};

} // namespace udp
