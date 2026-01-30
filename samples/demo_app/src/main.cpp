// Copyright 2024 Accenture.

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/reboot.h>
#include <async/AsyncBinding.h>
#include <logger/logger.h>
#include <console/console.h>
#include "app/DemoLogger.h"
#include "app/appConfig.h"
#include <lifecycle/LifecycleLogger.h>
#include <lifecycle/LifecycleManager.h>
#include "systems/DemoSystem.h"
#include "systems/RuntimeSystem.h"
#include "systems/SysAdminSystem.h"

#ifdef PLATFORM_SUPPORT_CAN
#include "systems/CanSystem.h"
#include "systems/TransportSystem.h"
#include "systems/UdsSystem.h"
#include "systems/DoCanSystem.h"
#endif

using ::util::logger::LIFECYCLE;
using ::util::logger::DEMO;
using ::util::logger::Logger;

using AsyncAdapter        = ::async::AsyncBinding::AdapterType;
using AsyncRuntimeMonitor = ::async::AsyncBinding::RuntimeMonitorType;
using AsyncContextHook    = ::async::AsyncBinding::ContextHookType;

constexpr size_t MaxNumComponents         = 16;
constexpr size_t MaxNumLevels             = 8;
constexpr size_t MaxNumComponentsPerLevel = MaxNumComponents;

using LifecycleManager = ::lifecycle::declare::
    LifecycleManager<MaxNumComponents, MaxNumLevels, MaxNumComponentsPerLevel>;

char const* const isrGroupNames[ISR_GROUP_COUNT] = {
    "test",
    "can"
#ifdef PLATFORM_SUPPORT_ETHERNET
    , "ethernet"
#endif
};

AsyncRuntimeMonitor runtimeMonitor{
    AsyncContextHook::InstanceType::GetNameType::create<&AsyncAdapter::getTaskName>(),
    isrGroupNames};

LifecycleManager lifecycleManager{
    TASK_SYSADMIN,
    ::lifecycle::LifecycleManager::GetTimestampType::create<&getSystemTimeUs32Bit>()};

::systems::RuntimeSystem runtimeSystem{TASK_BACKGROUND, runtimeMonitor};
::systems::SysAdminSystem sysAdminSystem{TASK_SYSADMIN, lifecycleManager};

#ifdef PLATFORM_SUPPORT_CAN
::systems::CanSystem canSystem{TASK_CAN};
::transport::TransportSystem transportSystem{TASK_UDS};
::docan::DoCanSystem doCanSystem{transportSystem, canSystem, TASK_CAN};
::uds::UdsSystem udsSystem{lifecycleManager, transportSystem, TASK_UDS, LOGICAL_ADDRESS};
#endif

::systems::DemoSystem demoSystem{
    TASK_DEMO,
    lifecycleManager
#ifdef PLATFORM_SUPPORT_CAN
    ,
    canSystem
#endif
};

K_THREAD_STACK_DEFINE(sysadminStack, 1024);
using SysadminTask = AsyncAdapter::Task<TASK_SYSADMIN, K_THREAD_STACK_SIZEOF(sysadminStack)>;
SysadminTask sysadminTask{"sysadmin", sysadminStack};

K_THREAD_STACK_DEFINE(demoStack, 4 * 1024);
using DemoTask = AsyncAdapter::Task<TASK_DEMO, K_THREAD_STACK_SIZEOF(demoStack)>;
DemoTask demoTask{"demo", demoStack};

K_THREAD_STACK_DEFINE(canStack, 1024);
using CanTask = AsyncAdapter::Task<TASK_CAN, K_THREAD_STACK_SIZEOF(canStack)>;
CanTask canTask{"can", canStack};

K_THREAD_STACK_DEFINE(udsStack, 2 * 1024);
using UdsTask = AsyncAdapter::Task<TASK_UDS, K_THREAD_STACK_SIZEOF(udsStack)>;
UdsTask udsTask{"uds", udsStack};

K_THREAD_STACK_DEFINE(backgroundStack, 1024);
using BackgroundTask = AsyncAdapter::Task<TASK_BACKGROUND, K_THREAD_STACK_SIZEOF(backgroundStack)>;
BackgroundTask backgroundTask{"background", backgroundStack};

AsyncContextHook contextHook{runtimeMonitor};

::async::TimeoutType timeout;
::async::TimeoutType timeout2;

class LifecycleMonitor : private ::lifecycle::ILifecycleListener
{
public:
    explicit LifecycleMonitor(LifecycleManager& manager) { manager.addLifecycleListener(*this); }

    bool isReadyForReset() const { return _isReadyForReset; }

private:
    void lifecycleLevelReached(
        uint8_t const level,
        ::lifecycle::ILifecycleComponent::Transition::Type const /* transition */) override
    {
        if (0 == level)
        {
            _isReadyForReset = true;
        }
    }

private:
    bool _isReadyForReset = false;
};

void staticShutdown()
{
    Logger::info(LIFECYCLE, "Lifecycle shutdown complete");
    ::logger::flush();

    sys_reboot(SYS_REBOOT_COLD);
}

LifecycleMonitor lifecycleMonitor(lifecycleManager);

class DemoRunnable : public ::async::IRunnable
{
public:
    explicit DemoRunnable() {};

    void execute() override {
        Logger::debug(DEMO, "demo ctx: %d name: %s", 
            AsyncAdapter::getCurrentTaskContext(),
            AsyncAdapter::getTaskName(AsyncAdapter::getCurrentTaskContext()));
        if (AsyncAdapter::getCurrentTaskContext() == TASK_SYSADMIN) {
            async::execute(TASK_DEMO, *this);
        }
    };
};

class DemoRunnable2 : public ::async::IRunnable
{
public:
    explicit DemoRunnable2() {};

    void execute() override {
        ::logger::run();
        ::console::run();
    };
};

DemoRunnable demoRunnable;
DemoRunnable2 demoRunnable2;

int main(void)
{
    ::logger::init();

    ::console::init();
    ::console::enable();

    AsyncAdapter::init();

    lifecycleManager.addComponent("runtime", runtimeSystem, 1U);
#ifdef PLATFORM_SUPPORT_CAN
    lifecycleManager.addComponent("can", canSystem, 2U);
    lifecycleManager.addComponent("transport", transportSystem, 4U);
    lifecycleManager.addComponent("docan", doCanSystem, 5U);
    lifecycleManager.addComponent("uds", udsSystem, 6U);
#endif
    lifecycleManager.addComponent("sysadmin", sysAdminSystem, 7U);
    lifecycleManager.addComponent("demo", demoSystem, 8U);

    lifecycleManager.transitionToLevel(MaxNumLevels);

    runtimeMonitor.start();

    AsyncAdapter::run();

    ::async::scheduleAtFixedRate(
        TASK_SYSADMIN, demoRunnable, timeout, 1000, ::async::TimeUnit::MILLISECONDS);
    ::async::scheduleAtFixedRate(
        TASK_DEMO, demoRunnable2, timeout2, 10, ::async::TimeUnit::MILLISECONDS);

    while (1) {
        k_msleep(1000);
        if (lifecycleMonitor.isReadyForReset())
        {
            staticShutdown();
        }
    }

    return 0;
}
