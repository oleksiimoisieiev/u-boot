/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2024 EPAM Systems
 *
 * Derived from linux rp1_platform.h
 * Copyright (c) 2021-2022 Raspberry Pi Ltd.
 */

#ifndef _RP1_PLATFORM_H
#define _RP1_PLATFORM_H

#include <linux/types.h>
#include <asm/byteorder.h>

#define RP1_B0_CHIP_ID 0x10001927
#define RP1_C0_CHIP_ID 0x20001927

#define RP1_PLATFORM_ASIC BIT(1)
#define RP1_PLATFORM_FPGA BIT(0)

void rp1_get_platform(u32 *chip_id, u32 *platform);

#endif
