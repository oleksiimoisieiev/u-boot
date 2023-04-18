// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 Google LLC
 */

#include <asm/armv8/mmu.h>
#include <asm/esr.h>
#include <asm/proc-armv/ptrace.h>
#include <common.h>
#include <dm.h>

static bool esr_is_external_data_abort(unsigned int esr)
{
	return esr == (ESR_ELx_FSC_EXTABT | ESR_ELx_IL |
		((unsigned int)(ESR_ELx_EC_DABT_CUR) << ESR_ELx_EC_SHIFT));
}

int handle_synchronous_exception(struct pt_regs *pt_regs)
{
	struct mm_region *region;
	u64 far;

	/*
	 * HACK: Check for an external data abort, symptomatic of an
	 * injected exception
	 */
	if (!esr_is_external_data_abort(pt_regs->esr))
		return 0;

	asm volatile("mrs %0, far_el1" : "=r" (far));

	for (region = mem_map; region->size; region++) {
		struct arm_smccc_res res;
		u64 attrs = region->attrs & PMD_ATTRINDX_MASK;
		u64 size = region->size;
		u64 phys = region->phys;

		if (attrs == PTE_BLOCK_MEMTYPE(MT_NORMAL) ||
		    attrs == PTE_BLOCK_MEMTYPE(MT_NORMAL_NC))
			continue;

		if (far >= phys && far < (phys + size)) {
			arm_smccc_hvc(ARM_SMCCC_VENDOR_HYP_KVM_MMIO_GUARD_MAP_FUNC_ID,
				      (far & PAGE_MASK), attrs, 0, 0, 0, 0, 0, &res);
			return (res.a0 == SMCCC_RET_SUCCESS);
		}
	}

	return 0;
}

static int kvm_hyp_services_bind(struct udevice *dev)
{
	return 0;
}

U_BOOT_DRIVER(kvm_hyp_services) = {
	.name = "kvm-hyp-services",
	.id = UCLASS_FIRMWARE,
	.bind = kvm_hyp_services_bind,
};
