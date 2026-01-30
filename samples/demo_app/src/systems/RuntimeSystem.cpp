// Copyright 2023 Accenture.

#include "systems/RuntimeSystem.h"

namespace
{
constexpr uint32_t SYSTEM_CYCLE_TIME = 1000;
}

namespace systems
{

RuntimeSystem::RuntimeSystem(
    ::async::ContextType const context, ::async::AsyncBinding::RuntimeMonitorType& runtimeMonitor)
: _context(context)
, _timeout()

, _statisticsCommand(runtimeMonitor)
, _asyncCommandWrapper_for_statisticsCommand(_statisticsCommand, context)
{
    setTransitionContext(context);
}

void RuntimeSystem::init()
{
    _statisticsCommand.setTicksPerUs(sys_clock_hw_cycles_per_sec() / USEC_PER_SEC);

    transitionDone();
}

void RuntimeSystem::run()
{
    ::async::scheduleAtFixedRate(
        _context, *this, _timeout, SYSTEM_CYCLE_TIME, ::async::TimeUnit::MILLISECONDS);

    transitionDone();
}

void RuntimeSystem::shutdown()
{
    _timeout.cancel();

    transitionDone();
}

void RuntimeSystem::execute() { _statisticsCommand.cyclic_1000ms(); }

} // namespace systems
