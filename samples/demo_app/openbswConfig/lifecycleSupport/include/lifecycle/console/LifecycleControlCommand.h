// Copyright 2023 Accenture.

#pragma once

#include <util/command/GroupCommand.h>

namespace lifecycle
{
class ILifecycleManager;
}

namespace lifecycle
{

class LifecycleControlCommand : public ::util::command::GroupCommand
{
public:
    LifecycleControlCommand(ILifecycleManager& lifecycleManager);

protected:
    DECLARE_COMMAND_GROUP_GET_INFO
    virtual void executeCommand(::util::command::CommandContext& context, uint8_t idx);

private:
    ILifecycleManager& _lifecycleManager;
};

} // namespace lifecycle
