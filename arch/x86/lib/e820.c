// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015, Bin Meng <bmeng.cn@gmail.com>
 */

#include <common.h>
#include <efi_loader.h>
#include <asm/e820.h>
#include <asm/global_data.h>
#if CONFIG_IS_ENABLED(EFI_APP)
#include <malloc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_IS_ENABLED(EFI_APP)
static int fill_e820_table(struct efi_mem_desc *memmap_desc, int size, int desc_size,
			   unsigned int max_entries, struct e820_entry *entries)
{
	struct efi_mem_desc *mmap;
	struct efi_mem_desc *base, *end;
	int count, e820_cnt = 0;
	u32 e820type;

	base = malloc(size + sizeof(*base));
	if (!base)
		return -ENOMEM;

	end = (void *)memmap_desc + size;
	memcpy(base, memmap_desc, (ulong)end - (ulong)memmap_desc);
	count = ((ulong)end - (ulong)memmap_desc) / desc_size;
	end = (struct efi_mem_desc *)((ulong)base + count * desc_size);

	for (mmap = base; mmap < end;
	     mmap = efi_get_next_mem_desc(mmap, desc_size)) {
		switch (mmap->type) {
		case EFI_RESERVED_MEMORY_TYPE:
		case EFI_RUNTIME_SERVICES_CODE:
		case EFI_RUNTIME_SERVICES_DATA:
		case EFI_MMAP_IO:
		case EFI_MMAP_IO_PORT:
		case EFI_PAL_CODE:
			e820type = E820_RESERVED;
			break;

		case EFI_UNUSABLE_MEMORY:
			e820type = E820_UNUSABLE;
			break;

		case EFI_ACPI_RECLAIM_MEMORY:
			e820type = E820_ACPI;
			break;

		case EFI_LOADER_CODE:
		case EFI_LOADER_DATA:
		case EFI_BOOT_SERVICES_CODE:
		case EFI_BOOT_SERVICES_DATA:
		case EFI_CONVENTIONAL_MEMORY:
			e820type = E820_RAM;
			break;

		case EFI_ACPI_MEMORY_NVS:
			e820type = E820_NVS;
			break;

		default:
			printf("Invalid EFI memory descriptor type (0x%x)!\n",
			       mmap->type);
			continue;
		}
		if (e820_cnt == max_entries - 1)
			break;

		entries[e820_cnt].type = e820type;
		entries[e820_cnt].addr = mmap->physical_start;
		entries[e820_cnt].size = mmap->num_pages << EFI_PAGE_SHIFT;

		e820_cnt++;
	}

	return e820_cnt;
}

/*
 * Install e820 table based on the efi memory map provided by BIOS.
 * This call converts efi memory table format to e820 table format that can
 * be passed to the kernel.
 */
__weak unsigned int install_e820_map(unsigned int max_entries,
				     struct e820_entry *entries)
{
	struct efi_priv *priv = efi_get_priv();
	struct efi_boot_services *boot = priv->sys_table->boottime;
	efi_uintn_t size, desc_size;
	efi_status_t ret;

	ret = efi_store_memory_map(priv);
	if (ret)
		return ret;

	ret = boot->get_memory_map(&size, priv->memmap_desc, &priv->memmap_key,
				   &desc_size, &priv->memmap_version);
	if (ret) {
		puts(" No memory map\n");
		return ret;
	}
	return fill_e820_table(priv->memmap_desc, size, desc_size, max_entries, entries);
}
#else
/*
 * Install a default e820 table with 4 entries as follows:
 *
 *	0x000000-0x0a0000	Useable RAM
 *	0x0a0000-0x100000	Reserved for ISA
 *	0x100000-gd->ram_size	Useable RAM
 *	CONFIG_PCIE_ECAM_BASE	PCIe ECAM
 */
__weak unsigned int install_e820_map(unsigned int max_entries,
				     struct e820_entry *entries)
{
	entries[0].addr = 0;
	entries[0].size = ISA_START_ADDRESS;
	entries[0].type = E820_RAM;
	entries[1].addr = ISA_START_ADDRESS;
	entries[1].size = ISA_END_ADDRESS - ISA_START_ADDRESS;
	entries[1].type = E820_RESERVED;
	entries[2].addr = ISA_END_ADDRESS;
	entries[2].size = gd->ram_size - ISA_END_ADDRESS;
	entries[2].type = E820_RAM;
	entries[3].addr = CONFIG_PCIE_ECAM_BASE;
	entries[3].size = CONFIG_PCIE_ECAM_SIZE;
	entries[3].type = E820_RESERVED;

	return 4;
}
#endif /* CONFIG_IS_ENABLED(EFI_APP) */

#if CONFIG_IS_ENABLED(EFI_LOADER)
void efi_add_known_memory(void)
{
	struct e820_entry e820[E820MAX];
	unsigned int i, num;
	u64 start, ram_top;
	int type;

	num = install_e820_map(ARRAY_SIZE(e820), e820);

	ram_top = (u64)gd->ram_top & ~EFI_PAGE_MASK;
	if (!ram_top)
		ram_top = 0x100000000ULL;

	for (i = 0; i < num; ++i) {
		start = e820[i].addr;

		switch (e820[i].type) {
		case E820_RAM:
			type = EFI_CONVENTIONAL_MEMORY;
			break;
		case E820_RESERVED:
			type = EFI_RESERVED_MEMORY_TYPE;
			break;
		case E820_ACPI:
			type = EFI_ACPI_RECLAIM_MEMORY;
			break;
		case E820_NVS:
			type = EFI_ACPI_MEMORY_NVS;
			break;
		case E820_UNUSABLE:
		default:
			type = EFI_UNUSABLE_MEMORY;
			break;
		}

		if (type == EFI_CONVENTIONAL_MEMORY) {
			efi_add_conventional_memory_map(start,
							start + e820[i].size,
							ram_top);
		} else {
			efi_add_memory_map(start, e820[i].size, type);
		}
	}
}
#endif /* CONFIG_IS_ENABLED(EFI_LOADER) */
