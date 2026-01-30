// Copyright 2023 Accenture.

#pragma once

#include <async/AsyncBinding.h>
#include <etl/optional.h>
#include <runtime/StatisticsContainer.h>
#include <util/command/GroupCommand.h>

namespace lifecycle
{
class StatisticsCommand : public ::util::command::GroupCommand
{
public:
    StatisticsCommand(::async::AsyncBinding::RuntimeMonitorType& runtimeMonitor);

    void setTicksPerUs(uint32_t ticksPerUs);
    void cyclic_1000ms();

protected:
    DECLARE_COMMAND_GROUP_GET_INFO
    virtual void executeCommand(::util::command::CommandContext& context, uint8_t idx);

private:
    ::async::AsyncBinding::RuntimeMonitorType& _runtimeMonitor;

    using TaskStatistics = ::runtime::declare::StatisticsContainer<
        ::runtime::RuntimeStatistics,
        ::async::AsyncBindingType::AdapterType::TASK_COUNT>;

    using IsrGroupStatistics
        = ::runtime::declare::StatisticsContainer<::runtime::RuntimeStatistics, ISR_GROUP_COUNT>;

    TaskStatistics _taskStatistics;
    IsrGroupStatistics _isrGroupStatistics;

    ::etl::optional<uint32_t> _ticksPerUs;
    uint32_t _totalRuntime;
};

} // namespace lifecycle
