// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025, EPAM Systems
 */

#include <common.h>
#include <cpu_func.h>
#include <env.h>
#include <init.h>
#include <pci.h>
#include <qfw.h>
#include <dm/platdata.h>
#include <asm/irq.h>
#include <asm/post.h>
#include <asm/processor.h>
#include <asm/arch/hvm.h>

void hvm_chipset_init(void)
{
}

#if CONFIG_IS_ENABLED(X86_32BIT_INIT)
int arch_cpu_init(void)
{
	post_code(POST_CPU_INIT);

	return x86_cpu_init_f();
}

int checkcpu(void)
{
	return 0;
}

int print_cpuinfo(void)
{
	post_code(POST_CPU_INFO);
	return default_print_cpuinfo();
}
#endif

int arch_early_init_r(void)
{
	hvm_chipset_init();

	return 0;
}
