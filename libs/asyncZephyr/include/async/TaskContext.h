// Copyright 2021 Accenture.

/**
 * \ingroup async
 */
#pragma once

#include "async/EventDispatcher.h"
#include "async/EventPolicy.h"
#include "async/RunnableExecutor.h"
#include "async/Types.h"
#include "zephyr/kernel.h"

#include <bsp/timer/SystemTimer.h>
#include <etl/delegate.h>
#include <etl/span.h>
#include <timer/Timer.h>

namespace async
{
template<class Binding>
class TaskContext : public EventDispatcher<2U, LockType>
{
public:
    using TaskFunctionType = ::etl::delegate<void(TaskContext<Binding>&)>;

    struct StackUsage
    {
        StackUsage();

        uint32_t _stackSize;
        uint32_t _usedSize;
    };

    TaskContext();

    void initTask(ContextType context, TaskFunctionType taskFunction);
    void createTask(
        ContextType const context,
        k_thread& task,
        char const* const name,
        int priority,
        k_thread_stack_t* stack,
        size_t stackSize,
        TaskFunctionType const taskFunction);
    void createTimer(k_timer& timer, char const* name);
    void startTask();

    char const* getName() const;
    bool getStackUsage(StackUsage& stackUsage) const;

    void execute(RunnableType& runnable);
    void schedule(RunnableType& runnable, TimeoutType& timeout, uint32_t delay, TimeUnitType unit);
    void scheduleAtFixedRate(
        RunnableType& runnable, TimeoutType& timeout, uint32_t period, TimeUnitType unit);
    void cancel(TimeoutType& timeout);

    void callTaskFunction();
    void dispatch();
    void stopDispatch();

    void setTimeout(uint32_t timeInUs);

    static void defaultTaskFunction(TaskContext<Binding>& taskContext);

private:
    friend class EventPolicy<TaskContext<Binding>, 0U>;
    friend class EventPolicy<TaskContext<Binding>, 1U>;

    void setEvents(EventMaskType eventMask);
    EventMaskType waitEvents();

private:
    using TimerType = ::timer::Timer<LockType>;

    static EventMaskType const STOP_EVENT_MASK = static_cast<EventMaskType>(
        static_cast<EventMaskType>(1U) << static_cast<EventMaskType>(EVENT_COUNT));
    static EventMaskType const WAIT_EVENT_MASK = (STOP_EVENT_MASK << 1U) - 1U;

    void handleTimeout();

    static void staticTaskFunction(void* param, void* unused1, void* unused2);
    static void staticTimerFunction(struct k_timer* timer_id);

    RunnableExecutor<RunnableType, EventPolicy<TaskContext<Binding>, 0U>, LockType>
        _runnableExecutor;
    TimerType _timer;
    EventPolicy<TaskContext<Binding>, 1U> _timerEventPolicy;
    TaskFunctionType _taskFunction;
    k_tid_t _taskId;
    struct k_timer* _timerHandle;
    struct k_event _eventObject;
    ContextType _context;
};

/**
 * Inline implementations.
 */
template<class Binding>
inline TaskContext<Binding>::TaskContext()
: _runnableExecutor(*this)
, _timer()
, _timerEventPolicy(*this)
, _taskFunction()
, _taskId(nullptr)
, _timerHandle(nullptr)
, _context(CONTEXT_INVALID)
{
    _timerEventPolicy.setEventHandler(
        HandlerFunctionType::create<TaskContext, &TaskContext::handleTimeout>(*this));
    _runnableExecutor.init();
}

template<class Binding>
void TaskContext<Binding>::initTask(ContextType const context, TaskFunctionType const taskFunction)
{
    _context      = context;
    _taskFunction = taskFunction;
}

template<class Binding>
void TaskContext<Binding>::createTask(
    ContextType const context,
    k_thread& task,
    char const* const name,
    int priority,
    k_thread_stack_t* stack,
    size_t stackSize,
    TaskFunctionType const taskFunction)
{
    _context      = context;
    _taskFunction = taskFunction.is_valid()
                        ? taskFunction
                        : TaskFunctionType::template create<&TaskContext::defaultTaskFunction>();

    _taskId = k_thread_create(
        &task,
        stack,
        stackSize,
        &staticTaskFunction,
        this,
        NULL,
        NULL,
        priority,
        K_USER,
        K_FOREVER);
    k_thread_name_set(_taskId, name);

    k_event_init(&_eventObject);
}

template<class Binding>
void TaskContext<Binding>::createTimer(k_timer& timer, char const* const name)
{
    k_timer_init(&timer, &staticTimerFunction, NULL);
    k_timer_user_data_set(&timer, this);
    _timerHandle = &timer;
}

template<class Binding>
void TaskContext<Binding>::startTask()
{
    if (_taskId != nullptr)
    {
        k_thread_start(_taskId);
    }
}

template<class Binding>
inline char const* TaskContext<Binding>::getName() const
{
    if (_taskId != nullptr)
    {
        return k_thread_name_get(_taskId);
    }
    else
    {
        return "<undefined>";
    }
}

template<class Binding>
inline bool TaskContext<Binding>::getStackUsage(StackUsage& stackUsage) const
{
    stackUsage._stackSize = _taskId->stack_info.size;

    size_t unused;
    if (k_thread_stack_space_get(_taskId, &unused))
    {
        return false;
    }
    else
    {
        stackUsage._usedSize = stackUsage._stackSize - unused;
        return true;
    }
}

template<class Binding>
inline void TaskContext<Binding>::execute(RunnableType& runnable)
{
    _runnableExecutor.enqueue(runnable);
}

template<class Binding>
inline void TaskContext<Binding>::schedule(
    RunnableType& runnable, TimeoutType& timeout, uint32_t const delay, TimeUnitType const unit)
{
    if (!_timer.isActive(timeout))
    {
        timeout._runnable = &runnable;
        timeout._context  = _context;
        if (_timer.set(timeout, delay * static_cast<uint32_t>(unit), getSystemTimeUs32Bit()))
        {
            _timerEventPolicy.setEvent();
        }
    }
}

template<class Binding>
inline void TaskContext<Binding>::scheduleAtFixedRate(
    RunnableType& runnable, TimeoutType& timeout, uint32_t const period, TimeUnitType const unit)
{
    if (!_timer.isActive(timeout))
    {
        timeout._runnable = &runnable;
        timeout._context  = _context;
        if (_timer.setCyclic(timeout, period * static_cast<uint32_t>(unit), getSystemTimeUs32Bit()))
        {
            _timerEventPolicy.setEvent();
        }
    }
}

template<class Binding>
inline void TaskContext<Binding>::cancel(TimeoutType& timeout)
{
    _timer.cancel(timeout);
}

template<class Binding>
inline void TaskContext<Binding>::setEvents(EventMaskType const eventMask)
{
    k_event_post(&_eventObject, eventMask);
}

template<class Binding>
inline EventMaskType
TaskContext<Binding>::waitEvents()
{
    uint32_t events
        = k_event_wait(&_eventObject, WAIT_EVENT_MASK, false /* don't reset events */, K_FOREVER);
    k_event_clear(&_eventObject, events);
    return events;
}

template<class Binding>
void TaskContext<Binding>::setTimeout(uint32_t const timeInUs)
{
    if (timeInUs > 0U)
    {
        k_timer_start(_timerHandle, K_USEC(timeInUs), K_NO_WAIT /* one shot: period 0 */);
    }
    else
    {
        (void)_timerEventPolicy.setEvent();
    }
}

template<class Binding>
void TaskContext<Binding>::callTaskFunction()
{
    _taskFunction(*this);
}

template<class Binding>
void TaskContext<Binding>::dispatch()
{
    EventMaskType eventMask = 0U;
    while ((eventMask & STOP_EVENT_MASK) == 0U)
    {
        eventMask = waitEvents();
        handleEvents(eventMask);
    }
}

template<class Binding>
inline void TaskContext<Binding>::stopDispatch()
{
    _runnableExecutor.shutdown();
    setEvents(STOP_EVENT_MASK);
}

template<class Binding>
void TaskContext<Binding>::defaultTaskFunction(TaskContext<Binding>& taskContext)
{
    taskContext.dispatch();
}

template<class Binding>
void TaskContext<Binding>::handleTimeout()
{
    while (_timer.processNextTimeout(getSystemTimeUs32Bit())) {}
    uint32_t nextDelta;
    if (_timer.getNextDelta(getSystemTimeUs32Bit(), nextDelta))
    {
        setTimeout(nextDelta);
    }
}

template<class Binding>
void TaskContext<Binding>::staticTaskFunction(void* param, void* unused1, void* unused2)
{
    (void)unused1;
    (void)unused2;
    TaskContext& taskContext = *reinterpret_cast<TaskContext*>(param);
    taskContext.callTaskFunction();
}

template<class Binding>
void TaskContext<Binding>::staticTimerFunction(struct k_timer* timer_id)
{
    TaskContext& taskContext = *reinterpret_cast<TaskContext*>(k_timer_user_data_get(timer_id));
    taskContext._timerEventPolicy.setEvent();
}

template<class Binding>
TaskContext<Binding>::StackUsage::StackUsage() : _stackSize(0U), _usedSize(0U)
{}

} // namespace async
