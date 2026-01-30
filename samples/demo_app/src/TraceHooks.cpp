// Copyright 2024 Accenture.

#include "async/Config.h"
#include "async/Hook.h"

#include <tracing_user.h>
#include <zephyr/init.h>

#if defined(CONFIG_SOC_SERIES_S32K1)
#define FLEXCAN_NODE DT_NODELABEL(flexcan0)
#ifdef PLATFORM_SUPPORT_ETHERNET
#define ENET_MAC_NODE DT_NODELABEL(enet_mac)
#endif

static int32_t const canIrqNums[DT_IRQN(FLEXCAN_NODE)]
    = {DT_IRQ_BY_IDX(FLEXCAN_NODE, 0, irq),
       DT_IRQ_BY_IDX(FLEXCAN_NODE, 1, irq),
       DT_IRQ_BY_IDX(FLEXCAN_NODE, 2, irq),
       DT_IRQ_BY_IDX(FLEXCAN_NODE, 3, irq),
       DT_IRQ_BY_IDX(FLEXCAN_NODE, 4, irq)};

#ifdef PLATFORM_SUPPORT_ETHERNET
static int32_t const enetMacIrqNums[DT_IRQN(ENET_MAC_NODE)]
    = {DT_IRQ_BY_IDX(ENET_MAC_NODE, 0, irq),
       DT_IRQ_BY_IDX(ENET_MAC_NODE, 1, irq),
       DT_IRQ_BY_IDX(ENET_MAC_NODE, 2, irq)};
#endif

#endif

/**
 * Tracing works via weak symbols.
 * To put the trace hook implementations here is a workaround to make sure
 * the weak symbols defined in the zephyr lib get overwritten.
 * Symbols in the zephyrAdapterImpl lib can not overwrite the weak symbols
 * since the zephyr lib is first in link order (because of the dependency).
 * This could be solved by putting the zephyrAdapterImpl in between the
 * --whole-archive section of the linking.
 */
extern "C"
{
static bool _application_initialized = false;

void sys_trace_thread_switched_in_user(void)
{
    /**
     * Ignore hook if application hasn't been initialized, i.e. constructors haven't been called yet.
     * Otherwise handling of the async hooks could cause problems.
     */
    if (!_application_initialized)
    {
        return;
    }

    unsigned int key = irq_lock();

    /* Can't use k_current_get as thread base and z_tls_current might be incorrect */
    int prio = k_thread_priority_get(k_sched_current_thread_query());
    asyncEnterTask(prio - 1);

    irq_unlock(key);
}

void sys_trace_thread_switched_out_user(void)
{
    /**
     * Ignore hook if application hasn't been initialized, i.e. constructors haven't been called yet.
     * Otherwise handling of the async hooks could cause problems.
     */
    if (!_application_initialized)
    {
        return;
    }

    unsigned int key = irq_lock();

    /* Can't use k_current_get as thread base and z_tls_current might be incorrect */
    int prio = k_thread_priority_get(k_sched_current_thread_query());
    asyncLeaveTask(prio - 1);

    irq_unlock(key);
}

void sys_trace_isr_enter_user(void)
{
    /**
     * Ignore hook if application hasn't been initialized, i.e. constructors haven't been called yet.
     * Otherwise handling of the async hooks could cause problems.
     */
    if (!_application_initialized)
    {
        return;
    }
#ifdef CONFIG_ARM

#if defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
    int32_t irqNo = z_soc_irq_get_active();
#else
    int32_t irqNo = __get_IPSR();
#endif
    irqNo -= 16;

#endif

#if defined(CONFIG_SOC_SERIES_S32K1)
    for (uint8_t i = 0; i < sizeof(canIrqNums) / sizeof(int32_t); i++)
    {
        if (irqNo == canIrqNums[i])
        {
            asyncEnterIsrGroup(ISR_GROUP_CAN);
            return;
        }
    }
#ifdef PLATFORM_SUPPORT_ETHERNET
    for (uint8_t i = 0; i < sizeof(enetMacIrqNums) / sizeof(int32_t); i++)
    {
        if (irqNo == enetMacIrqNums[i])
        {
            asyncEnterIsrGroup(ISR_GROUP_ETHERNET);
            return;
        }
    }
#endif
    asyncEnterIsrGroup(ISR_GROUP_TEST);
#else
    asyncEnterIsrGroup(ISR_GROUP_TEST);
#endif
}

void sys_trace_isr_exit_user(void)
{
    /**
     * Ignore hook if application hasn't been initialized, i.e. constructors haven't been called yet.
     * Otherwise handling of the async hooks could cause problems.
     */
    if (!_application_initialized)
    {
        return;
    }
#ifdef CONFIG_ARM

#if defined(CONFIG_ARM_CUSTOM_INTERRUPT_CONTROLLER)
    int32_t irqNo = z_soc_irq_get_active();
#else
    int32_t irqNo = __get_IPSR();
#endif
    irqNo -= 16;

#endif

#if defined(CONFIG_SOC_SERIES_S32K1)
    for (uint8_t i = 0; i < sizeof(canIrqNums) / sizeof(int32_t); i++)
    {
        if (irqNo == canIrqNums[i])
        {
            asyncLeaveIsrGroup(ISR_GROUP_CAN);
            return;
        }
    }
#ifdef PLATFORM_SUPPORT_ETHERNET
    for (uint8_t i = 0; i < sizeof(enetMacIrqNums) / sizeof(int32_t); i++)
    {
        if (irqNo == enetMacIrqNums[i])
        {
            asyncLeaveIsrGroup(ISR_GROUP_ETHERNET);
            return;
        }
    }
#endif
    asyncLeaveIsrGroup(ISR_GROUP_TEST);
#else
    asyncLeaveIsrGroup(ISR_GROUP_TEST);
#endif
}

static int application_initialized(void)
{
    _application_initialized = true;
    return 0;
}

/** 
 * We use the switch to the APPLICATION level to know that constructors have been invoked.
 */
SYS_INIT(application_initialized, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

}
