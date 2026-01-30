// Copyright 2025 Accenture.

#pragma once

#include "zephyr/kernel.h"

namespace interrupts
{
class SuspendResumeAllInterruptsLock
{
public:
    /**
     * Suspend all interrupts and store previous state in an class internal variable
     */
    void suspend()
    {
        _key = irq_lock();
    }

    /**
     * Resume all interrupts restoring the interrupt state that has been saved during the suspend()
     * call from the class internal variable
     */
    void resume() { irq_unlock(_key); }

private:
    unsigned int _key;
};

} /* namespace interrupts */
