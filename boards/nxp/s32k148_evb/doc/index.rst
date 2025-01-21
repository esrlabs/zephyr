.. zephyr:board:: s32k148_evb

Overview
********

`NXP S32K148-Q176`_ is a low-cost evaluation and development board for general-purpose industrial
and automotive applications.
The S32K148-Q176 is based on the 32-bit Arm Cortex-M4F `NXP S32K148`_ microcontroller.

Hardware
********

- NXP S32K148
    - Arm Cortex-M4F @ up to 112 Mhz
    - 1.5 MB Flash
    - 128 KB SRAM
    - up to 127 I/Os
    - 3x FlexCAN with FD
    - eDMA, 12-bit ADC, MPU, ECC and more.

- Interfaces:
    - CAN, LIN, UART/SCI
    - Ethernet connector compatible with different ethernet daughter cards
    - 2 touchpads, potentiometer, user RGB LED and 2 buttons.

More information about the hardware and design resources can be found at
`NXP S32K148-Q176`_ website.

Supported Features
==================

The ``s32k148_evb`` board configuration supports the following hardware features:

============  ==========  ================================
Interface     Controller  Driver/Component
============  ==========  ================================
SYSMPU        on-chip     mpu
PORT          on-chip     pinctrl
GPIO          on-chip     gpio
LPUART        on-chip     serial
LPI2C         on-chip     i2c
LPSPI         on-chip     spi
FTM           on-chip     pwm
FlexCAN       on-chip     can
Watchdog      on-chip     watchdog
RTC           on-chip     counter
ADC           on-chip     adc
============  ==========  ================================

The default configuration can be found in the Kconfig file
:zephyr_file:`boards/nxp/s32k148_evb/s32k148_evb_defconfig`.

Connections and IOs
===================

This board has 5 GPIO ports named from ``gpioa`` to ``gpioe``.

Pin control can be further configured from your application overlay by adding
children nodes with the desired pinmux configuration to the singleton node
``pinctrl``. Supported properties are described in
:zephyr_file:`dts/bindings/pinctrl/nxp,port-pinctrl.yaml`.

LEDs
----

The NXP S32K148-Q176 board has one user RGB LED that can be used either as a GPIO
LED or as a PWM LED.

.. table:: RGB LED as GPIO LED
   :widths: auto

   ===============  ================  ===============  =====
   Devicetree node  Devicetree alias  Label            Pin
   ===============  ================  ===============  =====
   led1_red         led0              LED1_RGB_RED     PTE21
   led1_green       led1              LED1_RGB_GREEN   PTE22
   led1_blue        led2              LED1_RGB_BLUE    PTE23
   ===============  ================  ===============  =====

.. table:: RGB LED as PWM LED
   :widths: auto

   ===============  ========================  ==================  ================
   Devicetree node  Devicetree alias          Label               Pin
   ===============  ========================  ==================  ================
   led1_red_pwm     pwm-led0 / red-pwm-led    LED1_RGB_RED_PWM    PTE21 / FTM4_CH1
   led1_green_pwm   pwm-led1 / green-pwm-led  LED1_RGB_GREEN_PWM  PTE22 / FTM4_CH2
   led1_blue_pwm    pwm-led2 / blue-pwm-led   LED1_RGB_BLUE_PWM   PTE23 / FTM4_CH3
   ===============  ========================  ==================  ================

The user can control the LEDs in any way. An output of ``0`` illuminates the LED.

Buttons
-------

The NXP S32K148-Q176 board has two user buttons:

=======================  ==============  =====
Devicetree node          Label           Pin
=======================  ==============  =====
sw0 / button_3           SW3             PTC12
sw1 / button_4           SW4             PTC13
=======================  ==============  =====

Serial Console
==============

The serial console is provided via ``lpuart1`` on the OpenSDA adapter.

=====  ============
Pin    Pin Function
=====  ============
PTC7   LPUART1_TX
PTC6   LPUART1_RX
=====  ============

System Clock
============

The Arm Cortex-M4F core is configured to run at 80 MHz (RUN mode).

Programming and Debugging
*************************

Applications for the ``s32k148_evb`` board can be built in the usual way as
documented in :ref:`build_an_application`.

Flashing
========

Debugging
=========

References
**********

.. target-notes::

.. _NXP S32K148-Q176:
   https://www.nxp.com/design/design-center/development-boards-and-designs/automotive-development-platforms/s32k-mcu-platforms/s32k148-q176-evaluation-board-for-automotive-general-purpose:S32K148EVB

.. _NXP S32K148:
   https://www.nxp.com/products/processors-and-microcontrollers/s32-automotive-platform/s32k-auto-general-purpose-mcus/s32k1-microcontrollers-for-automotive-general-purpose:S32K1
