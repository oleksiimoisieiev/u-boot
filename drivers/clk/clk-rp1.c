// SPDX-License-Identifier: GPL-2.0+
/*
 * Clock driver for RP1 PCIe multifunction chip.
 *
 * Copyright (C) 2024 EPAM Systems
 *
 * Derived from linux clk-rp1 driver
 *   Copyright (C) 2023 Raspberry Pi Ltd.
 */

#include <clk.h>
#include <clk-uclass.h>
#include <dm.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/clk-provider.h>
#include <linux/math64.h>

#include <dt-bindings/clk/rp1.h>

#include <mfd/rp1_platform.h>

#define PLL_SYS_CS			0x08000
#define PLL_SYS_PWR			0x08004
#define PLL_SYS_FBDIV_INT		0x08008
#define PLL_SYS_FBDIV_FRAC		0x0800c
#define PLL_SYS_PRIM			0x08010
#define PLL_SYS_SEC			0x08014

#define PLL_AUDIO_CS			0x0c000
#define PLL_AUDIO_PWR			0x0c004
#define PLL_AUDIO_FBDIV_INT		0x0c008
#define PLL_AUDIO_FBDIV_FRAC		0x0c00c
#define PLL_AUDIO_PRIM			0x0c010
#define PLL_AUDIO_SEC			0x0c014
#define PLL_AUDIO_TERN			0x0c018

#define PLL_VIDEO_CS			0x10000
#define PLL_VIDEO_PWR			0x10004
#define PLL_VIDEO_FBDIV_INT		0x10008
#define PLL_VIDEO_FBDIV_FRAC		0x1000c
#define PLL_VIDEO_PRIM			0x10010
#define PLL_VIDEO_SEC			0x10014

#define GPCLK_OE_CTRL			0x00000

#define CLK_SYS_CTRL			0x00014
#define CLK_SYS_DIV_INT			0x00018
#define CLK_SYS_SEL			0x00020

#define CLK_SLOW_SYS_CTRL		0x00024
#define CLK_SLOW_SYS_DIV_INT		0x00028
#define CLK_SLOW_SYS_SEL		0x00030

#define CLK_DMA_CTRL			0x00044
#define CLK_DMA_DIV_INT			0x00048
#define CLK_DMA_SEL			0x00050

#define CLK_UART_CTRL			0x00054
#define CLK_UART_DIV_INT		0x00058
#define CLK_UART_SEL			0x00060

#define CLK_ETH_CTRL			0x00064
#define CLK_ETH_DIV_INT			0x00068
#define CLK_ETH_SEL			0x00070

#define CLK_PWM0_CTRL			0x00074
#define CLK_PWM0_DIV_INT		0x00078
#define CLK_PWM0_DIV_FRAC		0x0007c
#define CLK_PWM0_SEL			0x00080

#define CLK_PWM1_CTRL			0x00084
#define CLK_PWM1_DIV_INT		0x00088
#define CLK_PWM1_DIV_FRAC		0x0008c
#define CLK_PWM1_SEL			0x00090

#define CLK_AUDIO_IN_CTRL		0x00094
#define CLK_AUDIO_IN_DIV_INT		0x00098
#define CLK_AUDIO_IN_SEL		0x000a0

#define CLK_AUDIO_OUT_CTRL		0x000a4
#define CLK_AUDIO_OUT_DIV_INT		0x000a8
#define CLK_AUDIO_OUT_SEL		0x000b0

#define CLK_I2S_CTRL			0x000b4
#define CLK_I2S_DIV_INT			0x000b8
#define CLK_I2S_SEL			0x000c0

#define CLK_MIPI0_CFG_CTRL		0x000c4
#define CLK_MIPI0_CFG_DIV_INT		0x000c8
#define CLK_MIPI0_CFG_SEL		0x000d0

#define CLK_MIPI1_CFG_CTRL		0x000d4
#define CLK_MIPI1_CFG_DIV_INT		0x000d8
#define CLK_MIPI1_CFG_SEL		0x000e0

#define CLK_PCIE_AUX_CTRL		0x000e4
#define CLK_PCIE_AUX_DIV_INT		0x000e8
#define CLK_PCIE_AUX_SEL		0x000f0

#define CLK_USBH0_MICROFRAME_CTRL	0x000f4
#define CLK_USBH0_MICROFRAME_DIV_INT	0x000f8
#define CLK_USBH0_MICROFRAME_SEL	0x00100

#define CLK_USBH1_MICROFRAME_CTRL	0x00104
#define CLK_USBH1_MICROFRAME_DIV_INT	0x00108
#define CLK_USBH1_MICROFRAME_SEL	0x00110

#define CLK_USBH0_SUSPEND_CTRL		0x00114
#define CLK_USBH0_SUSPEND_DIV_INT	0x00118
#define CLK_USBH0_SUSPEND_SEL		0x00120

#define CLK_USBH1_SUSPEND_CTRL		0x00124
#define CLK_USBH1_SUSPEND_DIV_INT	0x00128
#define CLK_USBH1_SUSPEND_SEL		0x00130

#define CLK_ETH_TSU_CTRL		0x00134
#define CLK_ETH_TSU_DIV_INT		0x00138
#define CLK_ETH_TSU_SEL			0x00140

#define CLK_ADC_CTRL			0x00144
#define CLK_ADC_DIV_INT			0x00148
#define CLK_ADC_SEL			0x00150

#define CLK_SDIO_TIMER_CTRL		0x00154
#define CLK_SDIO_TIMER_DIV_INT		0x00158
#define CLK_SDIO_TIMER_SEL		0x00160

#define CLK_SDIO_ALT_SRC_CTRL		0x00164
#define CLK_SDIO_ALT_SRC_DIV_INT	0x00168
#define CLK_SDIO_ALT_SRC_SEL		0x00170

#define CLK_GP0_CTRL			0x00174
#define CLK_GP0_DIV_INT			0x00178
#define CLK_GP0_DIV_FRAC		0x0017c
#define CLK_GP0_SEL			0x00180

#define CLK_GP1_CTRL			0x00184
#define CLK_GP1_DIV_INT			0x00188
#define CLK_GP1_DIV_FRAC		0x0018c
#define CLK_GP1_SEL			0x00190

#define CLK_GP2_CTRL			0x00194
#define CLK_GP2_DIV_INT			0x00198
#define CLK_GP2_DIV_FRAC		0x0019c
#define CLK_GP2_SEL			0x001a0

#define CLK_GP3_CTRL			0x001a4
#define CLK_GP3_DIV_INT			0x001a8
#define CLK_GP3_DIV_FRAC		0x001ac
#define CLK_GP3_SEL			0x001b0

#define CLK_GP4_CTRL			0x001b4
#define CLK_GP4_DIV_INT			0x001b8
#define CLK_GP4_DIV_FRAC		0x001bc
#define CLK_GP4_SEL			0x001c0

#define CLK_GP5_CTRL			0x001c4
#define CLK_GP5_DIV_INT			0x001c8
#define CLK_GP5_DIV_FRAC		0x001cc
#define CLK_GP5_SEL			0x001d0

#define CLK_SYS_RESUS_CTRL		0x0020c

#define CLK_SLOW_SYS_RESUS_CTRL		0x00214

#define FC0_REF_KHZ			0x0021c
#define FC0_MIN_KHZ			0x00220
#define FC0_MAX_KHZ			0x00224
#define FC0_DELAY			0x00228
#define FC0_INTERVAL			0x0022c
#define FC0_SRC				0x00230
#define FC0_STATUS			0x00234
#define FC0_RESULT			0x00238
#define FC_SIZE				0x20
#define FC_COUNT			8
#define FC_NUM(idx, off)		((idx) * 32 + (off))

#define AUX_SEL				1

#define VIDEO_CLOCKS_OFFSET		0x4000
#define VIDEO_CLK_VEC_CTRL		(VIDEO_CLOCKS_OFFSET + 0x0000)
#define VIDEO_CLK_VEC_DIV_INT		(VIDEO_CLOCKS_OFFSET + 0x0004)
#define VIDEO_CLK_VEC_SEL		(VIDEO_CLOCKS_OFFSET + 0x000c)
#define VIDEO_CLK_DPI_CTRL		(VIDEO_CLOCKS_OFFSET + 0x0010)
#define VIDEO_CLK_DPI_DIV_INT		(VIDEO_CLOCKS_OFFSET + 0x0014)
#define VIDEO_CLK_DPI_SEL		(VIDEO_CLOCKS_OFFSET + 0x001c)
#define VIDEO_CLK_MIPI0_DPI_CTRL	(VIDEO_CLOCKS_OFFSET + 0x0020)
#define VIDEO_CLK_MIPI0_DPI_DIV_INT	(VIDEO_CLOCKS_OFFSET + 0x0024)
#define VIDEO_CLK_MIPI0_DPI_DIV_FRAC	(VIDEO_CLOCKS_OFFSET + 0x0028)
#define VIDEO_CLK_MIPI0_DPI_SEL		(VIDEO_CLOCKS_OFFSET + 0x002c)
#define VIDEO_CLK_MIPI1_DPI_CTRL	(VIDEO_CLOCKS_OFFSET + 0x0030)
#define VIDEO_CLK_MIPI1_DPI_DIV_INT	(VIDEO_CLOCKS_OFFSET + 0x0034)
#define VIDEO_CLK_MIPI1_DPI_DIV_FRAC	(VIDEO_CLOCKS_OFFSET + 0x0038)
#define VIDEO_CLK_MIPI1_DPI_SEL		(VIDEO_CLOCKS_OFFSET + 0x003c)

#define DIV_INT_8BIT_MAX		0x000000ffu /* max divide for most clocks */
#define DIV_INT_16BIT_MAX		0x0000ffffu /* max divide for GPx, PWM */
#define DIV_INT_24BIT_MAX		0x00ffffffu /* max divide for CLK_SYS */

#define FC0_STATUS_DONE			BIT(4)
#define FC0_STATUS_RUNNING		BIT(8)
#define FC0_RESULT_FRAC_SHIFT		5

#define PLL_PRIM_DIV1_SHIFT		16
#define PLL_PRIM_DIV1_MASK		0x00070000
#define PLL_PRIM_DIV2_SHIFT		12
#define PLL_PRIM_DIV2_MASK		0x00007000

#define PLL_SEC_DIV_SHIFT		8
#define PLL_SEC_DIV_WIDTH		5
#define PLL_SEC_DIV_MASK		0x00001f00

#define PLL_CS_LOCK			BIT(31)
#define PLL_CS_REFDIV_SHIFT		0

#define PLL_PWR_PD			BIT(0)
#define PLL_PWR_DACPD			BIT(1)
#define PLL_PWR_DSMPD			BIT(2)
#define PLL_PWR_POSTDIVPD		BIT(3)
#define PLL_PWR_4PHASEPD		BIT(4)
#define PLL_PWR_VCOPD			BIT(5)
#define PLL_PWR_MASK			0x0000003f

#define PLL_SEC_RST			BIT(16)
#define PLL_SEC_IMPL			BIT(31)

/* PLL phase output for both PRI and SEC */
#define PLL_PH_EN			BIT(4)
#define PLL_PH_PHASE_SHIFT		0

#define RP1_PLL_PHASE_0			0
#define RP1_PLL_PHASE_90		1
#define RP1_PLL_PHASE_180		2
#define RP1_PLL_PHASE_270		3

/* Clock fields for all clocks */
#define CLK_CTRL_ENABLE			BIT(11)
#define CLK_CTRL_AUXSRC_MASK		0x000003e0
#define CLK_CTRL_AUXSRC_SHIFT		5
#define CLK_CTRL_SRC_SHIFT		0
#define CLK_DIV_FRAC_BITS		16

#define KHz				1000
#define MHz				(KHz * KHz)
#define LOCK_TIMEOUT_NS			100000000
#define FC_TIMEOUT_NS			100000000

#define MAX_CLK_PARENTS	16

#define MEASURE_CLOCK_RATE
const char * const fc0_ref_clk_name = "clk_slow_sys";

#define ABS_DIFF(a, b) ((a) > (b) ? (a) - (b) : (b) - (a))
#define DIV_NEAREST(a, b) (((a) + ((b) >> 1)) / (b))
#define DIV_U64_NEAREST(a, b) div_u64(((a) + ((b) >> 1)), (b))

struct rp1_clockman {
	struct udevice *dev;
	void __iomem *regs;
	spinlock_t regs_lock; /* spinlock for all clocks */
};

struct rp1_pll_core_data {
	const char *name;
	u32 cs_reg;
	u32 pwr_reg;
	u32 fbdiv_int_reg;
	u32 fbdiv_frac_reg;
	unsigned long flags;
	u32 fc0_src;
};

struct rp1_pll_data {
	const char *name;
	const char *source_pll;
	u32 ctrl_reg;
	unsigned long flags;
	u32 fc0_src;
};

struct rp1_pll_ph_data {
	const char *name;
	const char *source_pll;
	unsigned int phase;
	unsigned int fixed_divider;
	u32 ph_reg;
	unsigned long flags;
	u32 fc0_src;
};

struct rp1_pll_divider_data {
	const char *name;
	const char *source_pll;
	u32 sec_reg;
	unsigned long flags;
	u32 fc0_src;
};

struct rp1_clock_data {
	const char *name;
	const char *const parents[MAX_CLK_PARENTS];
	int num_std_parents;
	int num_aux_parents;
	unsigned long flags;
	u32 oe_mask;
	u32 clk_src_mask;
	u32 ctrl_reg;
	u32 div_int_reg;
	u32 div_frac_reg;
	u32 sel_reg;
	u32 div_int_max;
	unsigned long max_freq;
	u32 fc0_src;
};

struct rp1_pll_core {
	struct clk hw;
	struct rp1_clockman *clockman;
	const struct rp1_pll_core_data *data;
	unsigned long cached_rate;
};

struct rp1_pll {
	struct clk hw;
	struct clk_divider div;
	struct rp1_clockman *clockman;
	const struct rp1_pll_data *data;
	unsigned long cached_rate;
};

struct rp1_pll_ph {
	struct clk hw;
	struct rp1_clockman *clockman;
	const struct rp1_pll_ph_data *data;
};

struct rp1_clock {
	struct clk hw;
	struct rp1_clockman *clockman;
	const struct rp1_clock_data *data;
	unsigned long cached_rate;
};

struct rp1_clk_change {
	struct clk *hw;
	unsigned long new_rate;
};

struct rp1_clk_change rp1_clk_chg_tree[3];

static inline
void clockman_write(struct rp1_clockman *clockman, u32 reg, u32 val)
{
	writel(val, clockman->regs + reg);
}

static inline u32 clockman_read(struct rp1_clockman *clockman, u32 reg)
{
	return readl(clockman->regs + reg);
}

static struct rp1_clock_data rp1_data[] = {
[RP1_CLK_ETH_TSU] = {
			.name = "clk_eth_tsu",
			.parents = {"xosc",
				    "pll_video_sec",
				    "clksrc_gp0",
				    "clksrc_gp1",
				    "clksrc_gp2",
				    "clksrc_gp3",
				    "clksrc_gp4",
				    "clksrc_gp5"},
			.num_std_parents = 0,
			.num_aux_parents = 8,
			.ctrl_reg = CLK_ETH_TSU_CTRL,
			.div_int_reg = CLK_ETH_TSU_DIV_INT,
			.div_frac_reg = 0,
			.sel_reg = CLK_ETH_TSU_SEL,
			.div_int_max = DIV_INT_8BIT_MAX,
			.max_freq = 50 * MHz,
			.fc0_src = FC_NUM(5, 7),
	},
};

static u32 rp1_clock_choose_div(unsigned long rate, unsigned long parent_rate,
				const struct rp1_clock_data *data)
{
	u64 div;

	/*
	 * Due to earlier rounding, calculated parent_rate may differ from
	 * expected value. Don't fail on a small discrepancy near unity divide.
	 */
	if (!rate || rate > parent_rate + (parent_rate >> CLK_DIV_FRAC_BITS))
		return 0;

	/*
	 * Always express div in fixed-point format for fractional division;
	 * If no fractional divider is present, the fraction part will be zero.
	 */
	if (data->div_frac_reg) {
		div = (u64)parent_rate << CLK_DIV_FRAC_BITS;
		div = DIV_U64_NEAREST(div, rate);
	} else {
		div = DIV_U64_NEAREST(parent_rate, rate);
		div <<= CLK_DIV_FRAC_BITS;
	}

	div = clamp(div,
		    1ull << CLK_DIV_FRAC_BITS,
		    (u64)data->div_int_max << CLK_DIV_FRAC_BITS);

	return div;
}

static ulong rp1_clock_set_rate(struct clk *hw, unsigned long rate)
{
	struct rp1_clockman *clockman = dev_get_priv(hw->dev);
	const struct rp1_clock_data *data = &rp1_data[hw->id];
	u32 div = rp1_clock_choose_div(rate, 0x2faf080, data);

	if (hw->id != RP1_CLK_ETH_TSU)
		return 0;

	WARN(rate > 4000000000ll, "rate is -ve (%d)\n", (int)rate);

	if (WARN(!div,
		 "clk divider calculated as 0! (%s, rate %ld, parent rate %d)\n",
		 data->name, rate, 0x2faf080))
		div = 1 << CLK_DIV_FRAC_BITS;

	spin_lock(&clockman->regs_lock);

	clockman_write(clockman, data->div_int_reg, div >> CLK_DIV_FRAC_BITS);
	if (data->div_frac_reg)
		clockman_write(clockman, data->div_frac_reg, div << (32 - CLK_DIV_FRAC_BITS));

	spin_unlock(&clockman->regs_lock);

	return 0;
}

static int rp1_clock_on(struct clk *hw)
{
	const struct rp1_clock_data *data = &rp1_data[hw->id];
	struct rp1_clockman *clockman = dev_get_priv(hw->dev);

	if (hw->id != RP1_CLK_ETH_TSU)
		return 0;

	spin_lock(&clockman->regs_lock);
	clockman_write(clockman, data->ctrl_reg,
		       clockman_read(clockman, data->ctrl_reg) | CLK_CTRL_ENABLE);
	/* If this is a GPCLK, turn on the output-enable */
	if (data->oe_mask)
		clockman_write(clockman, GPCLK_OE_CTRL,
			       clockman_read(clockman, GPCLK_OE_CTRL) | data->oe_mask);

	spin_unlock(&clockman->regs_lock);

	return 0;
}

static int rp1_clk_probe(struct udevice *dev)
{
	struct rp1_clockman *clockman = dev_get_priv(dev);
	u32 chip_id, platform;

	rp1_get_platform(&chip_id, &platform);

	spin_lock_init(&clockman->regs_lock);

	clockman->regs = dev_remap_addr(dev);
	if (!clockman->regs)
		return -EINVAL;

	return 0;
}

static const struct udevice_id rp1_clk_of_match[] = {
	{ .compatible = "raspberrypi,rp1-clocks" },
	{ /* sentinel */ }
};

static struct clk_ops rp1_clk_ops = {
	.set_rate = rp1_clock_set_rate,
	.enable = rp1_clock_on,
};

U_BOOT_DRIVER(clk_rp1) = {
	.name = "rp1-clk",
	.id = UCLASS_CLK,
	.of_match = rp1_clk_of_match,
	.probe = rp1_clk_probe,
	.ops = &rp1_clk_ops,
	.priv_auto	= sizeof(struct rp1_clockman),
	.flags = CLK_IGNORE_UNUSED,
};
