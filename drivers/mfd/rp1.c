// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 EPAM Systems
 *
 * Derived from linux rp1 driver
 * Copyright (c) 2018-22 Raspberry Pi Ltd.
 */

#include <linux/types.h>
#include <dm.h>
#include <dt-bindings/mfd/rp1.h>
#include <dm/device_compat.h>
#include <mfd/rp1_platform.h>
#include <pci.h>
#include <asm/io.h>
#include <linux/io.h>
#include <dm/of_access.h>

#define RP1_B0_CHIP_ID 0x10001927
#define RP1_C0_CHIP_ID 0x20001927

#define RP1_PLATFORM_ASIC BIT(1)
#define RP1_PLATFORM_FPGA BIT(0)

#define RP1_DRIVER_NAME "rp1"

#define PCI_VENDOR_ID_RPI 0x1de4
#define PCI_DEVICE_ID_RP1_C0 0x0001
#define PCI_DEVICE_REV_RP1_C0 2

#define SYSINFO_CHIP_ID_OFFSET	0x00000000
#define SYSINFO_PLATFORM_OFFSET	0x00000004

struct rp1_dev {
	phys_addr_t bar_start;
};

static struct rp1_dev *g_rp1;
static u32 g_chip_id, g_platform;

static inline dma_addr_t rp1_io_to_phys(struct rp1_dev *rp1, unsigned int offset)
{
	return rp1->bar_start + offset;
}

static u32 rp1_reg_read(struct rp1_dev *rp1, unsigned int base_addr, u32 offset)
{
	resource_size_t phys = rp1_io_to_phys(rp1, base_addr);
	void __iomem *regblock = ioremap(phys, 0x1000);
	u32 value = readl(regblock + offset);

	iounmap(regblock);
	return value;
}

void rp1_get_platform(u32 *chip_id, u32 *platform)
{
	if (chip_id)
		*chip_id = g_chip_id;
	if (platform)
		*platform = g_platform;
}

static int rp1_get_bar_region(struct udevice *dev, phys_addr_t *bar_start)
{
	struct pci_controller *hose;

	if (!dev->parent) {
		dev_err(dev, "could not find parent device\n");
		return -EINVAL;
	}

	hose = dev_get_uclass_priv(dev->parent);

	if (!hose || !hose->region_count) {
		dev_err(dev, "parent is not pci-controller device\n");
		return -EINVAL;
	}

	*bar_start = hose->regions[0].phys_start;
	return 0;
}

static int rp1_probe(struct udevice *dev)
{
	int ret;
	struct rp1_dev *rp1 = dev_get_priv(dev);
	struct device_node *rp1_node;

	/* Turn on bus-mastering */
	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);

	ret = rp1_get_bar_region(dev, &rp1->bar_start);
	if (ret)
		return ret;

	/* Get chip id */
	g_chip_id = rp1_reg_read(rp1, RP1_SYSINFO_BASE, SYSINFO_CHIP_ID_OFFSET);
	g_platform = rp1_reg_read(rp1, RP1_SYSINFO_BASE, SYSINFO_PLATFORM_OFFSET);
	dev_dbg(dev, "chip_id 0x%x%s\n", g_chip_id,
		(g_platform & RP1_PLATFORM_FPGA) ? " FPGA" : "");

	if (g_chip_id != RP1_C0_CHIP_ID) {
		dev_err(dev, "wrong chip id (%x)\n", g_chip_id);
		return -EINVAL;
	}

	for_each_of_allnodes(rp1_node) {
		if (rp1_node->name && strcmp("rp1", rp1_node->name))
			break;
	}

	g_rp1 = rp1;

	return 0;
}

static const struct pci_device_id dev_id_table[] = {
	{ PCI_DEVICE(PCI_VENDOR_ID_RPI, PCI_DEVICE_ID_RP1_C0), },
	{ 0, }
};

static int rp1_pcie_read_config(const struct udevice *bus, pci_dev_t bdf,
				uint offset, ulong *valuep, enum pci_size_t size)
{
	/*
	 * Leaving this call because pci subsystem calls for read_config
	 * and produces error then this callback is not set.
	 * Just return 0 here.s
	 */
	*valuep = 0;
	return 0;
}

static const struct dm_pci_ops rp1_pcie_ops = {
	.read_config	= rp1_pcie_read_config,
};

/*
 * Set compatible to "brcm,rp1-simple-bus" instead of "simple-bus"
 * as it was done in kernel source because u-boot doesn't probe correct
 * driver and applies simple-bus drivers to all this nodes.
 */

static const struct udevice_id rp1_ids[] = {
	{.compatible = "brcm,rp1-simple-bus"},
	{}
};

U_BOOT_DRIVER(rp1_driver) = {
	.name			= "rp1_driver",
	.id			= UCLASS_SIMPLE_BUS,
	.probe			= rp1_probe,
	.priv_auto		= sizeof(struct rp1_dev),
	.ops			= &rp1_pcie_ops,
	.of_match		= rp1_ids,
};

U_BOOT_PCI_DEVICE(rp1_driver, dev_id_table);
