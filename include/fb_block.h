// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023 The Android Open Source Project
 */

#ifndef _FB_BLOCK_H_
#define _FB_BLOCK_H_

struct blk_desc;
struct disk_partition;

/**
 * fastboot_block_get_part_info() - Lookup block partion by name
 *
 * @part_name: Named partition to lookup
 * @dev_desc: Pointer to returned blk_desc pointer
 * @part_info: Pointer to returned struct disk_partition
 * @response: Pointer to fastboot response buffer
 */
int fastboot_block_get_part_info(const char *part_name,
				 struct blk_desc **dev_desc,
				 struct disk_partition *part_info,
				 char *response);

/**
 * fastboot_block_erase() - Erase partition on block device for fastboot
 *
 * @part_name: Named partition to erase
 * @response: Pointer to fastboot response buffer
 */
void fastboot_block_erase(const char *part_name, char *response);

#endif // _FB_BLOCK_H_
