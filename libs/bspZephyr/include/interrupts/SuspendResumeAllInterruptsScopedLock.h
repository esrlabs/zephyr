// Copyright 2025 Accenture.

#pragma once

#include "zephyr/kernel.h"

namespace interrupts
{
class SuspendResumeAllInterruptsScopedLock
{
public:
    /**
     * Create a lock object instance with disabling of all interrupts
     * Store the current interrupt state on instance creation in a private member variable
     */
    SuspendResumeAllInterruptsScopedLock()
    : _key(irq_lock())
    {}

    /**
     * Destroy the lock object instance and restore the internally stored interrupt state from
     * before this object instance has been created
     */
    ~SuspendResumeAllInterruptsScopedLock() { irq_unlock(_key); }

private:
    unsigned int _key;
};

} /* namespace interrupts */
