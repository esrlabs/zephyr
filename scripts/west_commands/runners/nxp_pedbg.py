# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0
"""
Runner for NXP P&E Micro Multilink Debug Probe.
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

NXP_PEDBG_USB_CLASS = 'NXP Probes'
NXP_PEDBG_USB_VID = 0x1357
NXP_PEDBG_USB_PID = 0x0089

@dataclass
class NXPPEDebugProbeConfig:
    """NXP Debug Probe configuration parameters."""
    conn_str: str = 's32dbg'
    server_port: int = 7224
    speed: int = 5000

class NXPPEDebugProbeRunner(ZephyrBinaryRunner):
    """Runner front-end for NXP Debug Probe."""

    def __init__(self,
                 runner_cfg: RunnerConfig,
                 probe_cfg: NXPPEDebugProbeConfig,
                 core_name: str,
                 soc_name: str,
                 soc_family_name: str,
                 start_all_cores: bool,
                 s32ds_path: str | None = None,
                 tool_opt: list[str] | None = None) -> None:
        super().__init__(runner_cfg)
        self.elf_file: str = runner_cfg.elf_file or ''
        self.probe_cfg: NXPPEDebugProbeConfig = probe_cfg
        self.core_name: str = core_name
        self.soc_name: str = soc_name
        self.soc_family_name: str = soc_family_name
        self.start_all_cores: bool = start_all_cores
        self.s32ds_path_override: str | None = s32ds_path

        self.tool_opt: list[str] = []
        if tool_opt:
            for opt in tool_opt:
                self.tool_opt.extend(shlex.split(opt))

        build_cfg = BuildConfiguration(runner_cfg.build_dir)
        self.arch = build_cfg.get('CONFIG_ARCH').replace('"', '')

    @classmethod
    def name(cls) -> str:
        return 'nxp_pedbg'

    @classmethod
    def capabilities(cls) -> RunnerCaps:
        return RunnerCaps(commands={'debug', 'debugserver', 'attach'},
                          dev_id=True, tool_opt=True)

    @classmethod
    def dev_id_help(cls) -> str:
        return '''Debug probe connection string as in "s32dbg[:<address>]"
                  where <address> can be the IP address if TAP is available via Ethernet,
                  the serial ID of the probe or empty if TAP is available via USB.'''

    @classmethod
    def tool_opt_help(cls) -> str:
        return '''Additional options for GDB client when used with "debug" or "attach" commands
                  or for P&E GDB server when used with "debugserver" command.'''

    @classmethod
    def do_add_parser(cls, parser: argparse.ArgumentParser) -> None:
        parser.add_argument('--core-name',
                            # required=True,
                            help='Core name as supported by the debug probe (e.g. "R52_0_0")')
        parser.add_argument('--soc-name',
                            # required=True,
                            help='SoC name as supported by the debug probe (e.g. "S32K148")')
        parser.add_argument('--soc-family-name',
                            # required=True,
                            help='SoC family name as supported by the debug probe (e.g. "s32k1xx")')
        parser.add_argument('--start-all-cores',
                            action='store_true',
                            help='Start all SoC cores and not just the one being debugged. '
                                 'Use together with "debug" command.')
        parser.add_argument('--s32ds-path',
                            help='Override the path to NXP S32 Design Studio installation. '
                                 'By default, this runner will try to obtain it from the system '
                                 'path, if available.')
        parser.add_argument('--server-port',
                            default=NXPPEDebugProbeConfig.server_port,
                            type=int,
                            help='P&E GDB server port')
        parser.add_argument('--speed',
                            default=NXPPEDebugProbeConfig.speed,
                            type=int,
                            help='JTAG interface speed')

    @classmethod
    def do_create(cls, cfg: RunnerConfig, args: argparse.Namespace) -> 'NXPPEDebugProbeRunner':
        probe_cfg = NXPPEDebugProbeConfig(args.dev_id,
                                           server_port=args.server_port,
                                           speed=args.speed)

        return NXPPEDebugProbeRunner(cfg, probe_cfg, args.core_name, args.soc_name,
                                      args.soc_family_name, args.start_all_cores,
                                      s32ds_path=args.s32ds_path, tool_opt=args.tool_opt)

    @staticmethod
    def find_usb_probes() -> list[str]:
        """Return a list of debug probe serial numbers connected via USB to this host."""
        # use system's native commands to enumerate and retrieve the USB serial ID
        # to avoid bloating this runner with third-party dependencies that often
        # require priviledged permissions to access the device info
        pattern = r'sdafd[0-9a-f]{6}'
        if platform.system() == 'Windows':
            cmd = f'pnputil /enum-devices /connected /class "{NXP_PEDBG_USB_CLASS}"'
            serialid_pattern = f'instance id: +usb\\\\.*\\\\({pattern})'
        else:
            cmd = f'lsusb -v -d {NXP_PEDBG_USB_VID:04x}:{NXP_PEDBG_USB_PID:04x}'
            serialid_pattern = f'iserial +[0-255] +{pattern}'

        try:
            outb = subprocess.check_output(shlex.split(cmd), stderr=subprocess.DEVNULL)
            out = outb.decode('utf-8').strip().lower()
        except subprocess.CalledProcessError as err:
            raise RuntimeError('error while looking for debug probes connected') from err

        devices: list[str] = []
        if out and 'no devices were found' not in out:
            devices = re.findall(serialid_pattern, out)

        return sorted(devices)

    @classmethod
    def select_probe(cls) -> str:
        """
        Find debugger probes connected and return the serial number of the one selected.

        If there are multiple debugger probes connected and this runner is being executed
        in a interactive prompt, ask the user to select one of the probes.
        """
        probes_snr = cls.find_usb_probes()
        if not probes_snr:
            raise RuntimeError('there are no debug probes connected')
        elif len(probes_snr) == 1:
            return probes_snr[0]
        else:
            if not sys.stdin.isatty():
                raise RuntimeError(
                    f'refusing to guess which of {len(probes_snr)} connected probes to use '
                    '(Interactive prompts disabled since standard input is not a terminal). '
                    'Please specify a device ID on the command line.')

            print('There are multiple debug probes connected')
            for i, probe in enumerate(probes_snr, 1):
                print(f'{i}. {probe}')

            prompt = f'Please select one with desired serial number (1-{len(probes_snr)}): '
            while True:
                try:
                    value: int = int(input(prompt))
                except EOFError:
                    sys.exit(0)
                except ValueError:
                    continue
                if 1 <= value <= len(probes_snr):
                    break
            return probes_snr[value - 1]

    @property
    def runtime_environment(self) -> dict[str, str] | None:
        """Execution environment used for the client process."""
        if platform.system() == 'Windows':
            python_lib = (self.s32ds_path / 'S32DS' / 'build_tools' / 'msys32'
                        / 'mingw32' / 'lib' / 'python2.7')
            return {
                **os.environ,
                'PYTHONPATH': f'{python_lib}{os.pathsep}{python_lib / "site-packages"}'
            }

        return None

    def server_commands(self) -> list[str]:
        """Get launch commands to start the P&E GDB server."""
        server_exec = str(self.s32ds_path / 'eclipse' / 'plugins'
                / 'com.pemicro.debug.gdbjtag.pne_5.9.5.202412131551' / 'lin' / 'pegdbserver_console')
        cmd = [server_exec]
        if self.soc_name == 'S32K148':
            cmd += ['-device=NXP_S32K1xx_S32K148F2M0M11']
        cmd += ['-startserver', '-singlesession', '-serverport=' + str(self.probe_cfg.server_port),
                '-gdbmiport=6224', '-interface=OPENSDA', '-speed=' + str(self.probe_cfg.speed),
                '-port=USB1']
        return cmd

    def client_commands(self) -> list[str]:
        """Get launch commands to start the GDB client."""
        if self.arch == 'arm':
            client_exec_name = 'arm-none-eabi-gdb-py'
        elif self.arch == 'arm64':
            client_exec_name = 'aarch64-none-elf-gdb-py'
        else:
            raise RuntimeError(f'architecture {self.arch} not supported')

        client_exec = str(self.s32ds_path / 'S32DS' / 'tools' / 'gdb-arm'
                          / 'arm32-eabi' / 'bin' / client_exec_name)
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

        app_name = 's32ds' if platform.system() == 'Windows' else 's32ds.sh'
        self.s32ds_path = Path(self.require(app_name, path=self.s32ds_path_override)).parent

        if not self.probe_cfg.conn_str:
            self.probe_cfg.conn_str = f's32dbg:{self.select_probe()}'
            self.logger.info(f'using debug probe {self.probe_cfg.conn_str}')

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

        gdb_script.append(f'target remote localhost:' + str(self.probe_cfg.server_port))
        gdb_script.append(f'monitor reset')

        gdb_script.append(f'file {Path(self.elf_file).as_posix()}')
        if command == 'debug':
            gdb_script.append('load')

        with tempfile.TemporaryDirectory(suffix='nxp_pedbg') as tmpdir:
            gdb_cmds = Path(tmpdir) / 'runner.nxp_pedbg'
            gdb_cmds.write_text('\n'.join(gdb_script), encoding='utf-8')
            self.logger.debug(gdb_cmds.read_text(encoding='utf-8'))

            server_cmd = self.server_commands()
            print(server_cmd)
            client_cmd = self.client_commands()
            client_cmd.extend(['-x', gdb_cmds.as_posix()])
            client_cmd.extend(self.tool_opt)
            print(client_cmd)

            self.run_server_and_client(server_cmd, client_cmd, env=self.runtime_environment)

    def do_debugserver(self, **kwargs) -> None:
        """Start the P&E GDB server on a given port with the given extra parameters from cli."""
        server_cmd = self.server_commands()
        server_cmd.extend(self.tool_opt)
        self.check_call(server_cmd)
