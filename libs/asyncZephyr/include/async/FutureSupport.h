// Copyright 2021 Accenture.

/**
 * \ingroup async
 */
#pragma once

#include "zephyr/kernel.h"

#include <async/Types.h>
#include <util/concurrent/IFutureSupport.h>

namespace async
{
class FutureSupport : public ::os::IFutureSupport
{
public:
    explicit FutureSupport(ContextType context);

    void wait() override;
    void notify() override;
    void assertTaskContext() override;
    bool verifyTaskContext() override;

private:
    ContextType _context;
    struct k_event _eventObject;
};

} // namespace async
