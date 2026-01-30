// Copyright 2021 Accenture.

/**
 * \ingroup async
 */
#pragma once

#include "zephyr/kernel.h"

namespace async
{
class ModifiableLock final
{
public:
    ModifiableLock();
    ~ModifiableLock();

    void unlock();
    void lock();

private:
    unsigned int _key;
    bool _isLocked;
};

/**
 * Inline implementations.
 */
inline ModifiableLock::ModifiableLock() : _key(irq_lock()), _isLocked(true) {}

inline ModifiableLock::~ModifiableLock()
{
    if (_isLocked)
    {
        irq_unlock(_key);
    }
}

inline void ModifiableLock::unlock()
{
    if (_isLocked)
    {
        irq_unlock(_key);
        _isLocked = false;
    }
}

inline void ModifiableLock::lock()
{
    if (!_isLocked)
    {
        _key      = irq_lock();
        _isLocked = true;
    }
}

} // namespace async
