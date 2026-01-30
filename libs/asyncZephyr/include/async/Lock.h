// Copyright 2021 Accenture.

/**
 * \ingroup async
 */
#pragma once

#include "zephyr/kernel.h"

namespace async
{

class Lock
{
public:
    Lock();
    ~Lock();

private:
    unsigned int _key;
};

/**
 * Inline implementations.
 */
inline Lock::Lock() : _key(irq_lock()) {}

inline Lock::~Lock() { irq_unlock(_key); }

} // namespace async
