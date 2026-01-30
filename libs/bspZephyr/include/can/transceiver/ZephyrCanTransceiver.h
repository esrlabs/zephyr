// Copyright 2021 Accenture.

#pragma once

#include <async/Async.h>
#include <async/util/Call.h>
#include <bsp/timer/SystemTimer.h>
#include <can/canframes/CANFrame.h>
#include <can/canframes/ICANFrameSentListener.h>
#include <can/framemgmt/IFilteredCANFrameSentListener.h>
#include <can/transceiver/AbstractCANTransceiver.h>
#include <etl/deque.h>
#include <etl/queue.h>
#include <etl/uncopyable.h>
#include <platform/estdint.h>
#include <zephyr/drivers/can.h>


struct device;

namespace bios
{
class ZephyrCanTransceiver
: public can::AbstractCANTransceiver
, public ::etl::uncopyable
{

public:
    ZephyrCanTransceiver(
        ::async::ContextType context,
        uint8_t busId,
        const struct device *const canDevice,
        uint32_t baudRate);

    ::can::ICanTransceiver::ErrorCode init() override;
    ::can::ICanTransceiver::ErrorCode open(::can::CANFrame const& frame) override;
    ::can::ICanTransceiver::ErrorCode open() override;
    ::can::ICanTransceiver::ErrorCode close() override;
    void shutdown() override;

    ::can::ICanTransceiver::ErrorCode write(can::CANFrame const& frame) override;

    ::can::ICanTransceiver::ErrorCode
    write(can::CANFrame const& frame, can::ICANFrameSentListener& listener) override;

    ErrorCode mute() override;
    ErrorCode unmute() override;

    uint32_t getBaudrate() const override;

    /**
     * \return the the Hardware Queue Timeout.
     */
    uint16_t getHwQueueTimeout() const override;

    /**
     * \return ID of first frame seen after open() or resetFirstFrame() or INVALID_FRAME_ID
     */
    virtual uint16_t getFirstFrameId() const;

    /**
     * resets information about the first frame, the next frame received will be the first frame
     */
    virtual void resetFirstFrame();

    uint32_t getOverrunCount() const { return _overrunCount; }

    /**
     * cyclicTask()
     *
     * it is called periodically after the expiration of time set
     * by ITimeouListener::setTimeout
     * It calls the services which are to be called
     * with a period of the set timeout.
     * Transceiver error pins and bus state are checked by this
     * function and their respective values are stored and the
     * respective listeners informed if unexpected states have been
     * encountered
     */
    void cyclicTask();

    void receiveTask();

    static void receiveCallback(const struct device *dev, struct can_frame *frame, void *user_data);

    static void transmitCallback(const struct device *dev, int error, void *user_data);

private:
    /** polling time */
    static uint32_t const ERROR_POLLING_TIMEOUT = 10;
    static uint8_t const RX_QUEUE_SIZE          = 32;

    static can_filter const MatchAllCanFilter;

    struct TxJobWithCallback
    {
        TxJobWithCallback(::can::ICANFrameSentListener& listener, ::can::CANFrame const& frame)
        : _listener(listener), _frame(frame)
        {}

        ::can::ICANFrameSentListener& _listener;
        ::can::CANFrame const& _frame;
    };

    using TxQueue = ::etl::deque<TxJobWithCallback, 3>;

    const struct device *const _canDevice;
    uint32_t _baudRate;

    ::etl::queue<can::CANFrame, RX_QUEUE_SIZE> _rxQueue;
    int _rxFilterId;

    uint16_t _txOfflineErrors;
    uint32_t _overrunCount;
    uint32_t _framesSentCount;

    TxQueue _txQueue;

    ::async::ContextType const _context;
    ::async::Function _cyclicTask;
    ::async::Function _receiveTask;
    ::async::TimeoutType _cyclicTaskTimeout;

    void buildCanFrame(can_frame& canFrame, ::can::CANFrame const& frame);
    can::ICanTransceiver::ErrorCode
    write(can::CANFrame const& frame, can::ICANFrameSentListener* pListener);
    uint8_t enqueueRxFrame(
        uint32_t id, uint8_t length, uint8_t payload[], bool extended, uint8_t const* filterMap);

    void canFrameSentCallback(int error);
    void canFrameReceivedCallback(struct can_frame *frame);

    void notifyRegisteredSentListener(can::CANFrame const& frame) { notifySentListeners(frame); }
};

} // namespace bios
