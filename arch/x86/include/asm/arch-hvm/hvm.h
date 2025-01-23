/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2025, EPAM Systems
 */

#ifndef _ARCH_HVM_H_
#define _ARCH_HVM_H_

/**
 * hvm_get_low_memory_size() - Get low memory size
 *
 * @return:	size of memory below 4GiB
 */
u32 hvm_get_low_memory_size(void);

/**
 * hvm_get_high_memory_size() - Get high memory size
 *
 * @return:	size of memory above 4GiB
 */
u64 hvm_get_high_memory_size(void);

#endif /* _ARCH_HVM_H_ */
