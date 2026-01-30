# `demo_app` - sample application using OpenBSW with Zephyr

This sample code demonstrates some of OpenBSW's features working with Zephyr.

As explained in the [top-level README.md](../../README.md),
once the Zephyr build environment is set up this application can be built for a variety of boards as follows...

```
west build -p -b <board> [--shield <shield>] openbsw-zephyr/samples/demo_app
```

For example, the application can be built for the `s32k148_evb` board (without networking) as follows...
```
west build -p -b s32k148_evb openbsw-zephyr/samples/demo_app
```
and it can be built for the `s32k148_evb` board with the `nxp_adtja1101` shield (and with networking) as follows...
```
west build -p -b s32k148_evb --shield nxp_adtja1101 openbsw-zephyr/samples/demo_app -- -DEXTRA_CONF_FILE=boards/s32k148_evb_network.conf
```

## CMake

The [`hello_world`](../hello_world/README.md) sample explained the build configuration used in its `CMakeLists.txt` and `prj.conf` files to bring an OpenBSW library into Zephyr.
Now take a look at the `CMakeLists.txt` and `prj.conf` files in this directory.
This adds several OpenBSW libraries and configuration options which can be selected per board using the `board/*.conf` files.

Explore the code and build configuration files further to understand OpenBSW's features and adaptation to Zephyr.

## Getting started with `native_sim`

Building the application with `<board>` set to `native_sim`
(using `west build` as shown above) creates
`build/zephyr/zephyr.exe` - a 32-bit linux executable to run on your posix host.

`native_sim` provides a simulation of an embedded platform with the goal of facilitating application development.
See [Zephyr's documentation for `native_sim`](https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html) for more detailed information.

### Interacting with the console

For `demo_app`, the `native_sim` board is configured (in `boards/native_sim.conf`) with...
```
CONFIG_UART_NATIVE_POSIX=y
CONFIG_NATIVE_UART_0_ON_OWN_PTY=y
```
which means that a pseudoterminal is used to simulate a serial port.
Running the executable in a terminal should produce output like this...
```
openbsw_zephyr_workspace> ./build/zephyr/zephyr.exe
uart connected to pseudotty: /dev/pts/9
*** Booting Zephyr OS build 2957a9e134b8 ***
```
While this is running, you can connect to the console running on the pseudoterminal indicated
(which in the above case is `/dev/pts/9`) 
using software such as `minicom` in another terminal...
```
minicom -D /dev/pts/9
```
You should see logging output in the terminal
and if you press the `return` key you will see a prompt `>`.
Enter the command `help` and press `return` - it should print the available commands.
Eg...
```
 help       - Show all commands or specific help for a command given as parameter.
 lc         - lifecycle command
   reboot   - reboot the system
   poweroff - poweroff the system
   udef     - forces an undefined instruction exception
   pabt     - forces a prefetch abort exception
   dabt     - forces a data abort exception
   assert   - forces an assert
 stats      - lifecycle statistics command
   cpu      - prints CPU statistics
   stack    - prints stack statistics
   all      - prints all statistics
 ok
```
The indented commands are subcommands of the group they are in.
For example, if you enter the command...
```
stats all
```
you should see output like this...
```
 task                    %       total   runs       avg       min       max
 ---------------------------------------------------------------------------
 sysadmin           0.00 %         0 ms    100      0 us      0 us      0 us
 can                0.00 %         0 ms    150      0 us      0 us      0 us
 demo               0.00 %         0 ms    100      0 us      0 us      0 us
 uds                0.00 %         0 ms    100      0 us      0 us      0 us
 background         0.00 %         0 ms     50      0 us      0 us      0 us

 isr group               %       total   runs       avg       min       max
 ---------------------------------------------------------------------------
 test               0.00 %         0 ms    100      0 us      0 us      0 us
 can                0.00 %         0 ms      0      0 us      0 us      0 us
 ethernet           0.00 %         0 ms      0      0 us      0 us      0 us

 measurement time:  1000000 us
 stack:task=sysadmin,size=1024,used=24
 stack:task=can,size=1024,used=24
 stack:task=demo,size=4096,used=24
 stack:task=uds,size=2048,used=24
 stack:task=background,size=1024,used=24
 ok
```

Note that on `native_sim`, the statistics on how much time is spent on each task shows zero time for all (as above).
This is because of the simulation of time on `native_sim` assumes an infinitely fast CPU.
See [Important Limitations](https://docs.zephyrproject.org/latest/boards/native/doc/arch_soc.html#important-limitations).
When the same console command is executed on a real board the statistics of time spent in each task is shown correctly.
Below is the output of `stats all` command on the `s32k148_evb` board...
```
 task                    %       total   runs       avg       min       max
 ---------------------------------------------------------------------------
 sysadmin           0.32 %         3 ms    101     32 us     30 us    109 us
 can                0.83 %         8 ms    200     41 us     35 us     50 us
 demo               1.85 %        18 ms    207     89 us     35 us   1282 us
 uds                0.37 %         3 ms    116     31 us     30 us     37 us
 background         0.00 %         0 ms      1     35 us     35 us     35 us

 isr group               %       total   runs       avg       min       max
 ---------------------------------------------------------------------------
 test               1.93 %        19 ms    636     30 us     17 us     33 us
 can                1.25 %        12 ms   2043      6 us      5 us      8 us
 ethernet           0.15 %         1 ms     69     21 us     16 us     29 us

 measurement time:   999875 us
 stack:task=sysadmin,size=1024,used=600
 stack:task=can,size=1024,used=560
 stack:task=demo,size=4096,used=912
 stack:task=uds,size=2048,used=560
 stack:task=background,size=1024,used=872
 ok
```

Using a pseudoterminal on your build platform closely simulates how you would interact
with the same console running on a development board through a real serial port.
Alternatively, if you were to rebuild `build/zephyr/zephyr.exe` with
`CONFIG_NATIVE_UART_0_ON_STDINOUT` in `boards/native_sim.conf` instead of
`CONFIG_NATIVE_UART_0_ON_OWN_PTY` as follows...
```
CONFIG_UART_NATIVE_POSIX=y
CONFIG_NATIVE_UART_0_ON_STDINOUT=y
```
and then run `build/zephyr/zephyr.exe`, the same console and logging output
should be directly seen in your terminal.

### Testing CAN

`demo_app` implements support for CAN on supported boards.
For `native_sim`, CAN is implemented using a `SocketCAN` interface named `vcan0`
which can be set up on your posix host as described
[here](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/can/index.html).

Assuming `vcan0` is set up,
you should see log output indicating successful initialization during startup...
```
0: Core0: CAN: DEBUG: init()
0: Core0: CAN: DEBUG: open()
```
and once startup is complete,
you should see log output showing frames being sent every second...
```
7010: Core0: DEMO: DEBUG: Sending frame 6
7010: Core0: CAN: DEBUG: write()
```
If you have installed `can-utils`, you can see the CAN messages that are sent to `vcan0` as follows...
```
> candump vcan0
  vcan0  558   [4]  00 00 00 00
  vcan0  558   [4]  00 00 00 01
  vcan0  558   [4]  00 00 00 02
```

Testing CAN on development boards is the same as for `native_sim`,
except that instead of using the virtual interface `vcan0`,
a CAN-USB dongle can be set up as a SocketCAN interface, for example named `can0`,
and the same `can-utils` will work with it, as described
[here](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/can/index.html).

### Testing Ethernet

`demo_app` implements a UDP echo server and a TCP loopback for testing purposes.

To test ethernet communication on `native_sim`,
first setup the interface `zeth` by running the `net-setup.sh` script
as described on the top-level [`README.md`](../../README.md).

Next, run the demo application in a separate terminal.
During initialization you should see the following log output...
```
0: Core0: UDP: INFO: UDP Echo server initialisation
0: Core0: UDP: INFO: Listening on port 4444.
0: Core0: TCP: INFO: Socket prepared at port 1234
```

Send a UDP frame using netcat and see how the demo application echos it back.
Note that the demo application will have its own IP address, which is different from the host address.
Both addresses are in the network created by the `net-setup.sh` script:

```
> nc -u 192.0.2.1 4444
```
then enter `hello` and press `return`. `hello` should be echoed back in the terminal...

```
> nc -u 192.0.2.1 4444
hello
hello
```
Alternatively you can test the UDP echo server by running inside the demo_app folder...
```
python3 echo_test_udp.py
```
Similarly, the TCP loopback can be tested by running...
```
python3 echo_test_tcp.py
```

## Build, flash and debug on embedded boards

Several embedded boards are supported by `demo_app`.
See [build_matrix.json](../../build_matrix.json).

Below are some extra notes related to some of these boards.

### `s32k148_evb` board

To build `demo_app` for the `s32k148_evb` board (without networking)...
```
openbsw_zephyr_workspace> west build -p -b s32k148_evb openbsw-zephyr/samples/demo_app
```

To build `demo_app` for the `s32k148_evb` board with the `nxp_adtja1101` shield and with networking configuration...
```
openbsw_zephyr_workspace> west build -p -b s32k148_evb --shield nxp_adtja1101 openbsw-zephyr/samples/demo_app -- -DEXTRA_CONF_FILE=boards/s32k148_evb_network.conf
```

To flash and debug on s32k148_evb please install pegdbserver and arm-none-eabi-gdb as described in [this document](https://eclipse-openbsw.github.io/openbsw/sphinx_docs/doc/dev/learning/setup/setup_s32k148_gdbserver.html).

Once you have installed the tools run pegdbserver in a separate window like this:

```
sudo ./pegdbserver_console -startserver -device=NXP_S32K1xx_S32K148F2M0M11
```

To debug the application running on the board...
```
openbsw_zephyr_workspace> arm-none-eabi-gdb -x openbsw/tools/gdb/pegdbserver.gdb build/zephyr/zephyr.elf
```

### `nucleo_g474re` and `nucleo_h755zi` boards

The `nucleo_g474re` board has a single core SoC (ARM Cortex-M4) with CAN.
To build the `demo_app` for `nucleo_g474re`...
```
openbsw_zephyr_workspace> west build -p -b nucleo_g474re openbsw-zephyr/samples/demo_app
```

The `nucleo_h755zi` board has a dual core SoC (ARM Cortex-M7 and ARM Cortex-M4) with CAN and Ethernet.
To build the `demo_app` for `nucleo_h755zi` (for the Cortex-M7 core which is regarded as the primary core within Zephyr)...
```
openbsw_zephyr_workspace> west build -p -b nucleo_h755zi_q/stm32h755xx/m7 openbsw-zephyr/samples/demo_app
```

To flash and debug on either of these boards,
first install [STM32CubeProgrammer](https://docs.zephyrproject.org/latest/develop/flash_debug/host-tools.html#stm32cubeprog-flash-host-tools)
as detailed on the Zephyr `nucleo_g474re` board's
[documentation page](https://docs.zephyrproject.org/latest/boards/st/nucleo_g474re/doc/index.html).
> [!NOTE]
> For WSL download and install the Linux version of `STM32CubeProgrammer`
> and remember to connect the USB device to WSL using [usbipd](https://github.com/dorssel/usbipd-win).

Alternatively the openocd runner can be used by passing the `-r openocd` argument to the west commands below.

After successful installation, `west` commands should automatically find the board.

To flash the image on the board...
```
openbsw_zephyr_workspace> west flash
```

To debug the application running on the board...
```
openbsw_zephyr_workspace> west debug
```

To connect to the board's console via serial port...
```
minicom -D /dev/ttyACM0
```

To test ethernet communication on the `nucleo_h755zi` board,
setup a host network interface with IP address `192.0.2.2`
(e.g. using a USB/Ethernet dongle connected to the nucleo board),
then see **Testing Ethernet** above for testing steps.
