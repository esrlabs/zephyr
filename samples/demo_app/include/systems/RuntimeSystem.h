// Copyright 2023 Accenture.

#pragma once

#include <console/AsyncCommandWrapper.h>
#include <lifecycle/AsyncLifecycleComponent.h>
#include <lifecycle/console/StatisticsCommand.h>

namespace systems
{

class RuntimeSystem
: public ::lifecycle::AsyncLifecycleComponent
, private ::async::IRunnable
{
public:
    explicit RuntimeSystem(
        ::async::ContextType context, ::async::AsyncBinding::RuntimeMonitorType& runtimeMonitor);
    RuntimeSystem(RuntimeSystem const&)            = delete;
    RuntimeSystem& operator=(RuntimeSystem const&) = delete;

    void init() override;
    void run() override;
    void shutdown() override;

private:
    void execute() override;

    void runtimeCommand(::util::command::CommandContext& context);

private:
    ::async::ContextType const _context;
    ::async::TimeoutType _timeout;

    ::lifecycle::StatisticsCommand _statisticsCommand;
    ::console::AsyncCommandWrapper _asyncCommandWrapper_for_statisticsCommand;
};

} // namespace systems
