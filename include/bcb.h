// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2020 Eugeniu Rosca <rosca.eugeniu@gmail.com>
 *
 * Android Bootloader Control Block Header
 */

#ifndef __BCB_H__
#define __BCB_H__

#include <android_bootloader_message.h>
#include <part.h>

#if IS_ENABLED(CONFIG_CMD_BCB)
int bcb_write_reboot_reason(const char* iface, int devnum, char *partp, const char *reasonp);
struct bootloader_message* bcb_load(struct blk_desc *block_description,
	     			    struct disk_partition *disk_partition);
int bcb_store(void);
void bcb_reset(void);
#else
#include <linux/errno.h>
static inline int bcb_write_reboot_reason(const char* iface, int devnum,
					  char *partp, const char *reasonp)
{
	return -EOPNOTSUPP;
}
static inline struct bootloader_message* bcb_load(struct blk_desc *block_description,
	     					  struct disk_partition *disk_partition)
{
	return NULL;
}
static inline int bcb_store(void)
{
	return -EOPNOTSUPP;
}
static inline void bcb_reset(void)
{
	return;
}
#endif

#endif /* __BCB_H__ */
