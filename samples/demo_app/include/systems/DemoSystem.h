// Copyright 2023 Accenture.

#pragma once

#include <console/AsyncCommandWrapper.h>
#include <lifecycle/AsyncLifecycleComponent.h>
#include <lifecycle/console/LifecycleControlCommand.h>
#ifdef PLATFORM_SUPPORT_CAN
#include <systems/ICanSystem.h>
#include <can/framemgmt/ICANFrameListener.h>
#include <can/filter/IntervalFilter.h>
#endif
#ifdef PLATFORM_SUPPORT_ETHERNET
#include <zephyrEthAdapter/udp/UdpEchoServer.h>
#include <zephyrEthAdapter/tcp/ZephyrServerSocket.h>
#include <zephyrEthAdapter/tcp/ZephyrSocket.h>
#include <tcp/util/LoopbackTestServer.h>
#endif

namespace systems
{

class DemoSystem
: public ::lifecycle::AsyncLifecycleComponent
, private ::async::IRunnable
{
public:
    explicit DemoSystem(
        ::async::ContextType context,
        ::lifecycle::ILifecycleManager& lifecycleManager
#ifdef PLATFORM_SUPPORT_CAN
        ,
        ::can::ICanSystem& canSystem
#endif
    );

    DemoSystem(DemoSystem const&)            = delete;
    DemoSystem& operator=(DemoSystem const&) = delete;

    void init() override;
    void run() override;
    void shutdown() override;

    void cyclic();

private:
    void execute() override;
#if defined(CONFIG_BOARD_S32K148EVB) && defined(CONFIG_ADC)
    int32_t getPotentiometerValue();
#endif

private:
    ::async::ContextType const _context;
    ::async::TimeoutType _timeout;
#ifdef PLATFORM_SUPPORT_CAN
    class CanReceiver : public ::can::ICANFrameListener
    {
    public:
        CanReceiver() :
            _canFilter(100U, 700U)
        {}

        void frameReceived(::can::CANFrame const& canFrame) override;
        ::can::IFilter& getFilter() override { return _canFilter; }
    private:
        can::IntervalFilter _canFilter;

    };
    ::can::ICanSystem& _canSystem;
    CanReceiver _canReceiver;
#endif
#ifdef PLATFORM_SUPPORT_ETHERNET
    ::udp::UdpEchoServer udpEchoServer;
    ::tcp::ZephyrSocket _tcpSocket;
    ::tcp::LoopbackTestServer _tcpLoopback;
    ::tcp::ZephyrServerSocket _zephyrServerSocketIpv4;
#endif
};

} // namespace systems
