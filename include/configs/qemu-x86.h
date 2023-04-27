/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2015, Bin Meng <bmeng.cn@gmail.com>
 */

/*
 * board/config.h - configuration options, board specific
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>

#ifdef CONFIG_DISTRO_DEFAULTS

#ifdef CONFIG_CMD_USB
#define BOOT_TARGET_DEVICES_USB(func) func(USB, usb, 0)
#else
#define BOOT_TARGET_DEVICES_USB(func)
#endif

#ifdef CONFIG_CMD_SCSI
#define BOOT_TARGET_DEVICES_SCSI(func) func(SCSI, scsi, 0)
#else
#define BOOT_TARGET_DEVICES_SCSI(func)
#endif

#ifdef CONFIG_CMD_VIRTIO
#define BOOT_TARGET_DEVICES_VIRTIO(func) func(VIRTIO, virtio, 0)
#else
#define BOOT_TARGET_DEVICES_VIRTIO(func)
#endif

#if defined(CONFIG_CMD_IDE)
#define BOOT_TARGET_DEVICES_IDE(func) func(IDE, ide, 0
#else
#define BOOT_TARGET_DEVICES_IDE(func)
#endif

#if defined(CONFIG_CMD_DHCP)
#define BOOT_TARGET_DEVICES_DHCP(func) func(DHCP, dhcp, na)
#else
#define BOOT_TARGET_DEVICES_DHCP(func)
#endif

#define BOOT_TARGET_DEVICES(func) \
	BOOT_TARGET_DEVICES_USB(func) \
	BOOT_TARGET_DEVICES_SCSI(func) \
	BOOT_TARGET_DEVICES_VIRTIO(func) \
	BOOT_TARGET_DEVICES_IDE(func) \
	BOOT_TARGET_DEVICES_DHCP(func)

#include <config_distro_bootcmd.h>
#endif

#include <configs/x86-common.h>

#define CFG_STD_DEVICES_SETTINGS	"stdin=serial,i8042-kbd\0" \
					"stdout=serial,vidconsole\0" \
					"stderr=serial,vidconsole\0"

/*
 * ATA/SATA support for QEMU x86 targets
 *   - Only legacy IDE controller is supported for QEMU '-M pc' target
 *   - AHCI controller is supported for QEMU '-M q35' target
 */

#endif	/* __CONFIG_H */
