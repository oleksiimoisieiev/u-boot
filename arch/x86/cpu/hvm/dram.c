// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2025, EPAM Systems
 */

#include <common.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/post.h>
#include <asm/arch/hvm.h>
#include <linux/sizes.h>

DECLARE_GLOBAL_DATA_PTR;

u32 hvm_get_low_memory_size(void)
{
	/*
	 * TODO: Set 2Gb memory for now. Should be changed when
	 * HVM will be able to get correct memory mapping.
	 */
	return 0x80000000;
}

u64 hvm_get_high_memory_size(void)
{
	/*
	 * TODO: Set 2Gb memory for now. Should be changed when
	 * HVM will be able to get correct memory mapping.
	 */
	return 0x80000000;
}

int dram_init(void)
{
	gd->ram_size = hvm_get_low_memory_size();
	gd->ram_size += hvm_get_high_memory_size();
	post_code(POST_DRAM);

	return 0;
}

int dram_init_banksize(void)
{
	u64 high_mem_size;

	gd->bd->bi_dram[0].start = 0;
	gd->bd->bi_dram[0].size = hvm_get_low_memory_size();

	high_mem_size = hvm_get_high_memory_size();
	if (high_mem_size) {
		gd->bd->bi_dram[1].start = SZ_4G;
		gd->bd->bi_dram[1].size = high_mem_size;
	}

	return 0;
}

/*
 * This function looks for the highest region of memory lower than 4GB which
 * has enough space for U-Boot where U-Boot is aligned on a page boundary.
 * It overrides the default implementation found elsewhere which simply
 * picks the end of ram, wherever that may be. The location of the stack,
 * the relocation address, and how far U-Boot is moved by relocation are
 * set in the global data structure.
 */
phys_addr_t board_get_usable_ram_top(phys_size_t total_size)
{
	return hvm_get_low_memory_size();
}
