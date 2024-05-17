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
#include <dm/simple_bus.h>
#include <common.h>
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

//	struct pci_controller *hose;
//	if (!dev->parent) {
//		dev_err(dev, "could not find parent device\n");
//		return -EINVAL;
//	}



	*bar_start = dm_pci_map_bar(dev, PCI_BASE_ADDRESS_1, 0,0,
			PCI_REGION_TYPE, PCI_REGION_MEM);
//	hose = dev_get_uclass_priv(dev->parent->parent);
//	PP("dev name = %s,",dev->name);
//	PP("dev par name = %s,",dev->parent->name);
//	PP("dev par name = %s,",dev->parent->parent->name);
//	PP("dev = %p hose = %p,",dev->parent, hose);
//	PP("cnt = %x,", hose->region_count);
//	if (!hose || !hose->region_count) {
//		dev_err(dev, "parent is not pci-controller device\n");
//		return -EINVAL;
//	}
//	PP("phys = %llx\n",*bar_start);
//	*bar_start = hose->regions[0].phys_start;
	return 0;
}

static int rp1_probe(struct udevice *dev)
{
	int ret;
	struct rp1_dev *rp1 = dev_get_priv(dev);
	struct simple_bus_plat *plat = dev_get_uclass_plat(dev);
	struct device_node *rp1_node;
	struct udevice *bus;
	PP("");
	/* Turn on bus-mastering */
	dm_pci_clrset_config16(dev, PCI_COMMAND, 0, PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY);
	PP("dev->name = %s", dev->name);
	//TODO : TODO
	dm_pci_write_config32(dev, PCI_BASE_ADDRESS_1,0);

	ret = rp1_get_bar_region(dev, &rp1->bar_start);
	if (ret)
		return ret;
	PP("rp1-bar_start = %llx", rp1->bar_start);

	/* Get chip id */
	g_chip_id = rp1_reg_read(rp1, RP1_SYSINFO_BASE, SYSINFO_CHIP_ID_OFFSET);
	g_platform = rp1_reg_read(rp1, RP1_SYSINFO_BASE, SYSINFO_PLATFORM_OFFSET);
	PP("chip_id 0x%x%s\n", g_chip_id,
		(g_platform & RP1_PLATFORM_FPGA) ? " FPGA" : "");
	PP("");

	if (g_chip_id != RP1_C0_CHIP_ID) {
		dev_err(dev, "wrong chip id (%x)\n", g_chip_id);
		return -EINVAL;
	}

	g_rp1 = rp1;

	plat->base = rp1->bar_start;
	plat->target = 0;
	plat->size = 0x400000;
	return 0;
}

static int rp1_bind(struct udevice *dev)
{
	struct device_node *rp1_node;
//	struct udevice *ndev;
//	int ret;
//	PP("dev- = %p", dev);
//	PP("dev->driver = %p", dev->driver);
//	PP("dev->name = %s", dev->name);
//	PP("dev->name = %s", dev->driver->name);
//	for_each_of_allnodes(rp1_node) {
//		if (rp1_node->name && (strcmp("rp1", rp1_node->name) == 0)) {
//			PP("found->name = %s", rp1_node->name);
//
//			break;
//		}
//	}
//	ret = device_find_global_by_ofnode(np_to_ofnode(rp1_node), &ndev);
//	PP("ret = %d", ret);
//	PP("dev- = %p", ndev);
//	PP("dev->driver = %p", ndev->driver);
//	PP("dev->name = %s", ndev->name);
//	PP("dev->name = %s", ndev->driver->name);
//	ndev->driver = dev->driver;
	PP("");
	device_set_name(dev, "rp1");
	return 0;
}
static int rp1_bus_post_bind(struct udevice *dev)
{
//	struct rp1_dev *plat = dev_get_uclass_plat(dev);
//	int ret;
//
//	if (CONFIG_IS_ENABLED(SIMPLE_BUS_CORRECT_RANGE)) {
//		uint64_t caddr, paddr, len;
//
//		/* only read range index 0 */
//		ret = fdt_read_range((void *)gd->fdt_blob, dev_of_offset(dev),
//				     0, &caddr, &paddr, &len);
//		if (!ret) {
//			plat->base = caddr;
//			plat->target = paddr;
//			plat->size = len;
//		}
//	} else {
//		u32 cell[3];
//
//		ret = dev_read_u32_array(dev, "ranges", cell,
//					 ARRAY_SIZE(cell));
//		if (!ret) {
//			plat->bar_start = cell[0];
//			plat->target = cell[1];
//			plat->size = cell[2];
//		}
//	}
	PP("");
	return dm_scan_fdt_dev(dev);
}
static int rp1_of_to_plat(struct udevice *dev)
{
	PP("");
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
		{.compatible = "sss-bus"},
		{}
};

UCLASS_DRIVER(rp1_driver) = {
	.id		= UCLASS_SIMPLE_BUS,
	.name		= "rp1_driver",
	.post_bind = rp1_bus_post_bind,
	.per_device_plat_auto	= sizeof(struct simple_bus_plat),
};
U_BOOT_DRIVER(rp1_driver) = {
	.name			= "rp1_driver",
	.id			= UCLASS_SIMPLE_BUS,
	.probe			= rp1_probe,
	.bind 			= rp1_bind,
	.of_to_plat		= rp1_of_to_plat,
	.priv_auto		= sizeof(struct rp1_dev),
	.ops			= &rp1_pcie_ops,
	//.of_match		= rp1_ids,

};

U_BOOT_PCI_DEVICE(rp1_driver, dev_id_table);
