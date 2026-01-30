// Copyright 2021 Accenture.

#pragma once

#include "can/transceiver/ZephyrCanTransceiver.h"
#include "lifecycle/SingleContextLifecycleComponent.h"

#include <systems/ICanSystem.h>

#include <etl/singleton_base.h>

class StaticBsp;

namespace systems
{

class CanSystem
: public ::can::ICanSystem
, public ::lifecycle::SingleContextLifecycleComponent
, public ::etl::singleton_base<CanSystem>
{
public:
    /**
     * \param context The context in which the CanSystem will run which is unit8_t.
     */
    CanSystem(::async::ContextType const context);
    CanSystem(CanSystem const&)            = delete;
    CanSystem& operator=(CanSystem const&) = delete;

    /**
     * Initializes the CanSystem.
     * Invoke the transitionDone method which will update lifecycle manager that the component has
     * completed its transition.
     */
    void init() override;

    /**
     * Runs the CanSystem.
     * Enables CanRxRunnable, initialize and open CanFlex2Transceiver which setup and open CAN
     * connection, then invoke transitionDone to update lifecycle manager that the component has
     * completed its transition.
     */
    void run() override;

    /**
     * Shutdowns the CanSystem.
     * Close and shutdowns CanFlex2Transceiver which will cancel the CAN connection, disables
     * CanRxRunnable and then invoke transitionDone to update lifecycle manager that the component
     * has completed its transition.
     */
    void shutdown() override;

    /**
     * Checks if the given busId is equal to CAN_0. If it is returns a reference else nullptr.
     *
     * \param busId BusId.
     * \return A non-const reference to the CanFlex2Transceiver object.
     */
    ::can::ICanTransceiver* getCanTransceiver(uint8_t busId) override;

private:
    ::async::ContextType _context;
    bios::ZephyrCanTransceiver _transceiver0;
};

} // namespace systems
