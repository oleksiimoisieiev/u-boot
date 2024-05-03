/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2024 EPAM systems
 */

#ifndef __ASM_ARM_ARCH_CLK_H__
#define __ASM_ARM_ARCH_CLK_H__

#include <asm/global_data.h>

static inline unsigned long get_mck_clk_rate(void)
{
	return gd->arch.macb_pclk_rate_hz;
}

static inline unsigned long get_macb_pclk_rate(unsigned int dev_id)
{
	return get_mck_clk_rate();
}

#endif /* __ASM_ARM_ARCH_CLK_H__ */
