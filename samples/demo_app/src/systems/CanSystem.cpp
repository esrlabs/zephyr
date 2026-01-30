// Copyright 2021 Accenture.

#include "systems/CanSystem.h"

#include "async/Config.h"
#include "async/Hook.h"
#include "busid/BusId.h"
#include "lifecycle/ILifecycleManager.h"
#include "util/command/IParentCommand.h"

#include "zephyr/device.h"

#define CAN_INTERFACE DEVICE_DT_GET(DT_CHOSEN(zephyr_canbus))
#define CAN_BITRATE (DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bitrate, \
            DT_PROP_OR(DT_CHOSEN(zephyr_canbus), bus_speed, \
            CONFIG_CAN_DEFAULT_BITRATE)))

namespace systems
{

::systems::CanSystem::CanSystem(::async::ContextType context)
: ::lifecycle::SingleContextLifecycleComponent(context)
, ::etl::singleton_base<CanSystem>(*this)
, _context(context)
, _transceiver0(
      context,
      busid::CAN_0,
      CAN_INTERFACE,
      CAN_BITRATE
)
{}

void CanSystem::init() { transitionDone(); }

void CanSystem::run()
{
    (void)_transceiver0.init();
    (void)_transceiver0.open();

    transitionDone();
}

void CanSystem::shutdown()
{
    (void)_transceiver0.close();
    _transceiver0.shutdown();

    transitionDone();
}

::can::ICanTransceiver* CanSystem::getCanTransceiver(uint8_t busId)
{
    if (busId == ::busid::CAN_0)
    {
        return &_transceiver0;
    }
    return nullptr;
}

} // namespace systems
