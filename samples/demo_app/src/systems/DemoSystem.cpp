// Copyright 2023 Accenture.

#include "systems/DemoSystem.h"

#include "app/DemoLogger.h"

#include <bsp/SystemTime.h>

#include <etl/unaligned_type.h>
#ifdef PLATFORM_SUPPORT_CAN
#include <can/transceiver/AbstractCANTransceiver.h>
#endif
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/adc.h>

#ifdef CONFIG_PWM
static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));
#ifdef CONFIG_BOARD_S32K148EVB
static const struct pwm_dt_spec pwm_led1 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led1));
static const struct pwm_dt_spec pwm_led2 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led2));
#endif
#endif

#if defined(CONFIG_BOARD_S32K148EVB) && defined(CONFIG_ADC)
#define ADC_RESOLUTION 12
#define POTENTIOMETER_ADC_CHANNEL 28  // ADC channel for signal PTC28

static const struct device *adcDev = DEVICE_DT_GET(DT_NODELABEL(adc0));

static struct adc_channel_cfg potChannCfg = {
    .gain             = ADC_GAIN_1,
    .reference        = ADC_REF_INTERNAL,
    .acquisition_time = ADC_ACQ_TIME_DEFAULT,
    .channel_id       = POTENTIOMETER_ADC_CHANNEL,
    .differential     = 0,
};
#endif

namespace
{
constexpr uint32_t SYSTEM_CYCLE_TIME = 10;
}

namespace systems
{
using ::util::logger::DEMO;
using ::util::logger::Logger;

#ifdef PLATFORM_SUPPORT_ETHERNET
static constexpr uint16_t ECHO_RX_PORT = 4444U;
static ::ip::IPAddress IP_ADDRESS(::ip::make_ip4(0U, 0U, 0U, 0U));
#endif

DemoSystem::DemoSystem(
    ::async::ContextType const context,
    ::lifecycle::ILifecycleManager& lifecycleManager
#ifdef PLATFORM_SUPPORT_CAN
    ,
    ::can::ICanSystem& canSystem
#endif
    )
: _context(context)
#ifdef PLATFORM_SUPPORT_CAN
, _canSystem(canSystem)
#endif
#ifdef PLATFORM_SUPPORT_ETHERNET
, udpEchoServer(IP_ADDRESS, ECHO_RX_PORT, context)
, _tcpSocket()
, _tcpLoopback(_tcpSocket)
, _zephyrServerSocketIpv4(1234, _tcpLoopback)
#endif
{
    setTransitionContext(context);
}

void DemoSystem::init()
{
#ifdef PLATFORM_SUPPORT_ETHERNET
    udpEchoServer.start();
    _zephyrServerSocketIpv4.accept();
#endif
#if defined(CONFIG_BOARD_S32K148EVB) && defined(CONFIG_ADC)
    int err;

    if (!device_is_ready(adcDev))
    {
        Logger::error(DEMO, "ADC device not ready");
    }
    else
    {
        err = adc_channel_setup(adcDev, &potChannCfg);
        if (err < 0)
        {
            Logger::error(DEMO, "Failed to configure ADC channel %d (err: %d)",
                potChannCfg.channel_id, err);
        }
    }
#endif
    transitionDone();
}

#if defined(CONFIG_BOARD_S32K148EVB) && defined(CONFIG_ADC)
int32_t DemoSystem::getPotentiometerValue()
{
    int32_t adcBuf;
    struct adc_sequence sequence = {
        .channels = BIT(POTENTIOMETER_ADC_CHANNEL),
        .buffer = &adcBuf,
        .buffer_size = sizeof(adcBuf),
        .resolution = ADC_RESOLUTION
    };
    int err;

    err = adc_read(adcDev, &sequence);
    if (err == 0)
    {
        err = adc_raw_to_millivolts(adc_ref_internal(adcDev), potChannCfg.gain,
                sequence.resolution, &adcBuf);
        if (err < 0)
        {
            Logger::error(DEMO, "Value in mV not available (%d)", err);
            adcBuf = -1;
        }
    }
    else
    {
        Logger::error(DEMO, "Failed to read potentiometer value (%d)", err);
        adcBuf = -1;
    }

    return adcBuf;
}
#endif

void DemoSystem::run()
{
    ::async::scheduleAtFixedRate(
        _context, *this, _timeout, SYSTEM_CYCLE_TIME, ::async::TimeUnit::MILLISECONDS);
#ifdef PLATFORM_SUPPORT_CAN
    _canSystem.getCanTransceiver(::busid::CAN_0)->addCANFrameListener(_canReceiver);
#endif
    transitionDone();
}

void DemoSystem::shutdown()
{
#ifdef PLATFORM_SUPPORT_ETHERNET
    udpEchoServer.stop();
#endif
    transitionDone();
}

void DemoSystem::execute() { cyclic(); }

void DemoSystem::cyclic()
{
    static uint8_t timeCounter = 0;
    timeCounter++;

#if defined(CONFIG_PWM)
#if defined(CONFIG_BOARD_S32K148EVB) && defined(CONFIG_ADC)
    int32_t value = getPotentiometerValue();
    if (value >= 0)
    {
        pwm_set_dt(&pwm_led0, 1000000, value * 1000000 / 5000);
        pwm_set_dt(&pwm_led1, 1000000, value * 1000000 / 5000);
        pwm_set_dt(&pwm_led2, 1000000, value * 1000000 / 5000);
    }
#else
    pwm_set_dt(&pwm_led0, 1000000, timeCounter*5000);
#endif
#endif

#ifdef PLATFORM_SUPPORT_CAN
    static uint32_t previousSentTime = getSystemTimeMs32Bit();
    static uint32_t canSentCount     = 0;
    uint32_t const now               = getSystemTimeMs32Bit();
    uint32_t const deltaTimeMs       = now - previousSentTime;
    // Send a CAN frame every second.
    if (deltaTimeMs >= 1000)
    {
        previousSentTime = now;
        ::can::ICanTransceiver* canTransceiver
            = _canSystem.getCanTransceiver(::busid::CAN_0);
        if (canTransceiver != nullptr)
        {
            Logger::debug(DEMO, "Sending frame %d", canSentCount);
            etl::be_uint32_t canData{canSentCount};
            ::can::CANFrame frame(0x558, static_cast<uint8_t*>(canData.data()), 4);
            canTransceiver->write(frame);
            ++canSentCount;
        }
    }
#endif
}
#ifdef PLATFORM_SUPPORT_CAN
void DemoSystem::CanReceiver::frameReceived(::can::CANFrame const& canFrame)
{
    Logger::info(DEMO, "Frame received: 0x%x", canFrame.getId());
}
#endif

} // namespace systems
