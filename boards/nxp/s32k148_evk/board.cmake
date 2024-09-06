# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(trace32
  "--startup-args"
  "elfFile=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
)

board_runner_args(nxp_s32dbg
  "--soc-family-name" "S32K1"
  "--soc-name" "S32K148"
)

include(${ZEPHYR_BASE}/boards/common/nxp_s32dbg.board.cmake)
include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
