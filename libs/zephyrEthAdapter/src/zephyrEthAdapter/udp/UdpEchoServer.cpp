// Copyright 2025 Accenture.

#include "zephyrEthAdapter/udp/UdpEchoServer.h"

#include <ip/to_str.h>
#include <udp/UdpLogger.h>

namespace udp
{

using ::util::logger::Logger;
using ::util::logger::UDP;

UdpEchoServer::UdpEchoServer(
    ip::IPAddress const& ipAddr, uint16_t const rxPort, ::async::ContextType asyncContext)
: _ipAddr(ipAddr), _rxPort(rxPort), _socket(), _receiveData(), _context(asyncContext)
{}

bool UdpEchoServer::start()
{
    Logger::info(UDP, "UDP Echo server initialisation");

    _socket.setDataListener(this);
    if (_socket.bind(&_ipAddr, _rxPort) == ::udp::AbstractDatagramSocket::ErrorCode::UDP_SOCKET_OK)
    {
        _socket.setDataListener(this);
        Logger::info(UDP, "Listening on port %d.", _rxPort);
        return true;
    }

    return false;
}

void UdpEchoServer::stop()
{
    Logger::info(UDP, "Closing port %d.", _rxPort);
    _socket.close();
}

void UdpEchoServer::dataReceived(
    ::udp::AbstractDatagramSocket& /*socket*/,
    ::ip::IPAddress sourceAddress,
    uint16_t sourcePort,
    ::ip::IPAddress /*destinationAddress*/,
    uint16_t length)
{
    if (length > sizeof(_receiveData))
    {
        Logger::error(UDP, "Received oversized packet.");
        return;
    }

    (void)_socket.read(&_receiveData[0U], length);
    Logger::debug(UDP, "Echoing back %d bytes.", length);
    (void)_socket.send(::udp::DatagramPacket(&_receiveData[0U], length, sourceAddress, sourcePort));
}

} // namespace udp
