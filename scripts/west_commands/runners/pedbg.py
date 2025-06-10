# Copyright 2025 Accenture
# SPDX-License-Identifier: Apache-2.0
"""
Runner for P&E Micro Multilink Debug Probe.
"""

import argparse
import os
import platform
import re
import shlex
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path

from runners.core import BuildConfiguration, RunnerCaps, RunnerConfig, ZephyrBinaryRunner

PEDBG_USB_CLASS = 'usb'
PEDBG_USB_VID = 0x1357
PEDBG_USB_PID = 0x0089


@dataclass
class PEDebugProbeConfig:
    """P&E Micro Multilink Debug Probe configuration parameters."""

    dev_id: str = 'pedbg'
    server_port: int = 7224
    speed: int = 5000


class PEDebugProbeRunner(ZephyrBinaryRunner):
    """Runner front-end for P&E Micro Multilink Debug Probe."""

    def __init__(
        self,
        runner_cfg: RunnerConfig,
        probe_cfg: PEDebugProbeConfig,
        device: str,
        sdk_path: str | None = None,
        pemicro_plugin_path: str | None = None,
        tool_opt: list[str] | None = None,
    ) -> None:
        super().__init__(runner_cfg)
        self.elf_file: str = runner_cfg.elf_file or ''
        self.probe_cfg: PEDebugProbeConfig = probe_cfg
        self.sdk_path: str | None = sdk_path
        self.device: str = device
        self.pemicro_plugin_path: str | None = pemicro_plugin_path

        self.tool_opt: list[str] = []
        if tool_opt:
            for opt in tool_opt:
                self.tool_opt.extend(shlex.split(opt))

        build_cfg = BuildConfiguration(runner_cfg.build_dir)
        self.arch = build_cfg.get('CONFIG_ARCH').replace('"', '')

    @classmethod
    def name(cls) -> str:
        return 'pedbg'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'debug', 'debugserver', 'attach'}, dev_id=True, tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Debug probe connection string as in "pedbg[:<address>]"
                  where <address> can be the serial ID of the probe.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''Additional options for GDB client when used with "debug" or "attach" commands
                  or for P&E GDB server when used with "debugserver" command.'''

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument(
            '--device',
            required=True,
            help='Specify the device name being debugged (e.g. "NXP_S32K1xx_S32K148F2M0M11")',
        )
        parser.add_argument(
            '--sdk-path',
            help='Path to Zephyr SDK (e.g. /opt/zephyr-sdk-0.17.0)'
        )
        parser.add_argument(
            '--pemicro-plugin-path',
            required=True,
            help='Path to P&E Micro plugin.'
        )
        parser.add_argument(
            '--server-port',
            default=PEDebugProbeConfig.server_port,
            type=int,
            help='P&E GDB server port',
        )
        parser.add_argument(
            '--speed',
            default=PEDebugProbeConfig.speed,
            type=int,
            help='Shift frequency is n KHz',
        )

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> 'PEDebugProbeRunner':
        probe_cfg = PEDebugProbeConfig(
            args.dev_id, server_port=args.server_port, speed=args.speed
        )

        return PEDebugProbeRunner(
            cfg,
            probe_cfg,
            args.device,
            sdk_path=args.sdk_path,
            pemicro_plugin_path=args.pemicro_plugin_path,
            tool_opt=args.tool_opt,
        )

    @staticmethod
    def find_usb_probes() -> list[str]:
        """Return a list of debug probe serial numbers connected via USB to this host."""
        # use system's native commands to enumerate and retrieve the USB serial ID
        # to avoid bloating this runner with third-party dependencies that often
        # require priviledged permissions to access the device info
        serialid_pattern = r'sdafd[0-9a-f]{6}'
        if platform.system() == 'Windows':
            coding = 'windows-1252'
            cmd = f'pnputil /enum-devices /connected /class "{PEDBG_USB_CLASS}"'
            pattern = (
                rf'instance id:\s+usb\\vid_{PEDBG_USB_VID:04x}.*'
                'pid_{PEDBG_USB_PID:04x}\\{serialid_pattern}'
            )
        else:
            coding = 'utf-8'
            serialid_pattern = r'sdafd[0-9a-f]{6}'
            cmd = f'lsusb -v -d {PEDBG_USB_VID:04x}:{PEDBG_USB_PID:04x}'
            pattern = f'iserial +[0-255] +{serialid_pattern}'

        try:
            outb = subprocess.check_output(shlex.split(cmd), stderr=subprocess.DEVNULL)
            out = outb.decode(coding).strip().lower()
        except subprocess.CalledProcessError as err:
            raise RuntimeError('error while looking for debug probes connected') from err

        devices: list[str] = []
        if out and 'no devices were found' not in out:
            devices = re.findall(pattern, out)

        return sorted(devices)

    @classmethod
    def get_probe(cls) -> str:
        """
        Find debugger probes connected and return the serial number.

        If there are multiple debugger probes connected print an information to disconnect unneeded
        probe(s).

        :raises RuntimeError: if no probes or multiple probes connected
        """
        probes_snr = cls.find_usb_probes()
        if not probes_snr:
            raise RuntimeError('there are no debug probes connected')
        elif len(probes_snr) == 1:
            return probes_snr[0]
        else:
            print('There are multiple debug probes connected')
            for i, probe in enumerate(probes_snr, 1):
                print(f'{i}. {probe}')

            print('Please disconnect any unnecessary probes and leave only one connected.')
            raise RuntimeError('multiple debug probes connected')

    @property
    def runtime_environment(self) -> dict[str, str] | None:
        """Execution environment used for the client process."""
        if platform.system() == 'Windows':
            python_lib = (
                self.sdk_path
                / 'S32DS'
                / 'build_tools'
                / 'msys32'
                / 'mingw32'
                / 'lib'
                / 'python2.7'
            )
            return {
                **os.environ,
                'PYTHONPATH': f'{python_lib}{os.pathsep}{python_lib / "site-packages"}',
            }

        return None

    def server_commands(self) -> list[str]:
        """Get launch commands to start the P&E GDB server."""
        path = Path(self.pemicro_plugin_path) if self.pemicro_plugin_path else Path()
        if platform.system() == 'Linux':
            app = path / 'lin' / 'pegdbserver_console'
        else:
            app = path / 'win32' / 'pegdbserver_console'

        cmd = [str(app)]
        cmd += [f'-device={self.device}']
        cmd += [
            '-startserver',
            '-singlesession',
            '-serverport=' + str(self.probe_cfg.server_port),
            '-gdbmiport=6224',
            '-interface=OPENSDA',
            '-speed=' + str(self.probe_cfg.speed),
            '-porD',
        ]
        return cmd

    def client_commands(self) -> list[str]:
        """Get launch commands to start the GDB client."""
        if self.arch == 'arm':
            client_path = 'arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb-py'
        elif self.arch == 'arm64':
            client_path = 'aarch64-zephyr-elf/bin/aarch64-zephyr-elf-gdb-py'
        else:
            raise RuntimeError(f'architecture {self.arch} not supported')

        client_exec = str(self.sdk_path / client_path)
        cmd = [client_exec]
        return cmd

    def do_run(self, command: str, **kwargs) -> None:
        """
        Execute the given command.

        :param command: command name to execute
        :raises RuntimeError: if target architecture or host OS is not supported
        :raises MissingProgram: if required tools are not found in the host
        """
        if platform.system() not in ('Windows', 'Linux'):
            raise RuntimeError(f'runner not supported on {platform.system()} systems')

        if self.arch not in ('arm', 'arm64'):
            raise RuntimeError(f'architecture {self.arch} not supported')

        self.sdk_path = Path(self.sdk_path)

        if not self.probe_cfg.dev_id:
            self.probe_cfg.dev_id = f'{self.get_probe()}'
            if self.probe_cfg.dev_id:
                self.logger.info(f'using debug probe "pedbg:{self.probe_cfg.dev_id}"')

        if command in ('attach', 'debug'):
            self.ensure_output('elf')
            self.do_attach_debug(command, **kwargs)
        else:
            self.do_debugserver(**kwargs)

    def do_attach_debug(self, command: str, **kwargs) -> None:
        """
        Launch the P&E GDB server and GDB client to start a debugging session.

        :param command: command name to execute
        """
        gdb_script: list[str] = []

        gdb_script.append('target remote localhost:' + str(self.probe_cfg.server_port))
        gdb_script.append('monitor reset')

        gdb_script.append(f'file {Path(self.elf_file).as_posix()}')
        if command == 'debug':
            gdb_script.append('load')

        with tempfile.TemporaryDirectory(suffix='pedbg') as tmpdir:
            gdb_cmds = Path(tmpdir) / 'runner.pedbg'
            gdb_cmds.write_text('\n'.join(gdb_script), encoding='utf-8')
            self.logger.debug(gdb_cmds.read_text(encoding='utf-8'))

            server_cmd = self.server_commands()
            print(server_cmd)
            client_cmd = self.client_commands()
            client_cmd.extend(['-x', gdb_cmds.as_posix()])
            client_cmd.extend(self.tool_opt)
            print(server_cmd)
            print(client_cmd)

            self.run_server_and_client(server_cmd, client_cmd)

    def do_debugserver(self, **kwargs) -> None:
        """Start the P&E GDB server on a given port with the given extra parameters from cli."""
        server_cmd = self.server_commands()
        server_cmd.extend(self.tool_opt)
        self.check_call(server_cmd)
