/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * Copyright (c) 2009-2013 NVIDIA Corporation
 * Copyright (c) 2013 Lucas Stach
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/clk_rst.h>
#include <asm/arch/usb.h>
#include <usb.h>
#include <usb/ulpi.h>
#include <libfdt.h>
#include <fdtdec.h>

#include "ehci.h"

#define HOSTPC1_DEVLC	0x84
#define HOSTPC1_PSPD(x)		(((x) >> 25) & 0x3)

#ifdef CONFIG_USB_ULPI
	#ifndef CONFIG_USB_ULPI_VIEWPORT
	#error	"To use CONFIG_USB_ULPI on Tegra Boards you have to also \
		define CONFIG_USB_ULPI_VIEWPORT"
	#endif
#endif

enum {
	USB_PORTS_MAX	= 3,		/* Maximum ports we allow */
};

/* Parameters we need for USB */
enum {
	PARAM_DIVN,                     /* PLL FEEDBACK DIVIDer */
	PARAM_DIVM,                     /* PLL INPUT DIVIDER */
	PARAM_DIVP,                     /* POST DIVIDER (2^N) */
	PARAM_CPCON,                    /* BASE PLLC CHARGE Pump setup ctrl */
	PARAM_LFCON,                    /* BASE PLLC LOOP FILter setup ctrl */
	PARAM_ENABLE_DELAY_COUNT,       /* PLL-U Enable Delay Count */
	PARAM_STABLE_COUNT,             /* PLL-U STABLE count */
	PARAM_ACTIVE_DELAY_COUNT,       /* PLL-U Active delay count */
	PARAM_XTAL_FREQ_COUNT,          /* PLL-U XTAL frequency count */
	PARAM_DEBOUNCE_A_TIME,          /* 10MS DELAY for BIAS_DEBOUNCE_A */
	PARAM_BIAS_TIME,                /* 20US DELAY AFter bias cell op */

	PARAM_COUNT
};

/* Possible port types (dual role mode) */
enum dr_mode {
	DR_MODE_NONE = 0,
	DR_MODE_HOST,		/* supports host operation */
	DR_MODE_DEVICE,		/* supports device operation */
	DR_MODE_OTG,		/* supports both */
};

/* Information about a USB port */
struct fdt_usb {
	struct usb_ctlr *reg;	/* address of registers in physical memory */
	int params[PARAM_COUNT]; /* timing parameters */
	unsigned utmi:1;	/* 1 if port has external tranceiver, else 0 */
	unsigned ulpi:1;	/* 1 if port has external ULPI transceiver */
	unsigned enabled:1;	/* 1 to enable, 0 to disable */
	unsigned has_legacy_mode:1; /* 1 if this port has legacy mode */
	unsigned initialized:1; /* has this port already been initialized? */
	enum dr_mode dr_mode;	/* dual role mode */
	enum periph_id periph_id;/* peripheral id */
	struct fdt_gpio_state vbus_gpio;	/* GPIO for vbus enable */
	struct fdt_gpio_state phy_reset_gpio; /* GPIO to reset ULPI phy */
};

static struct fdt_usb port[USB_PORTS_MAX];	/* List of valid USB ports */
static unsigned port_count;			/* Number of available ports */
static unsigned is_T30_compatible;
static unsigned is_T114_compatible;

/* UTMIP Idle Wait Delay */
static const u8 utmip_idle_wait_delay = 17;

/* UTMIP Elastic limit */
static const u8 utmip_elastic_limit = 16;

/* UTMIP High Speed Sync Start Delay */
static const u8 utmip_hs_sync_start_delay = 9;

/*
 * A known hardware issue where Connect Status Change bit of PORTSC register
 * of USB1 controller will be set after Port Reset.
 * We have to clear it in order for later device enumeration to proceed.
 * This ehci_powerup_fixup overrides the weak function ehci_powerup_fixup
 * in "ehci-hcd.c".
 */
void ehci_powerup_fixup(uint32_t *status_reg, uint32_t *reg)
{
	udelay(49990);
	if (is_T30_compatible) {
		/*
		 * Tegra 114 takes 50 ms to assert Port Enable bit.
		 * We have to exit earlier. Otherwise ehci-hcd.c will clear
		 * our Port Enable bit.
		 */
		if (is_T114_compatible)
			return;
		/* Tegra 3 takes about 55 ms to assert Port Enable bit. */
		udelay(5010);
		return;
	}
	udelay(10);
	if (((u32)status_reg & TEGRA_USB_ADDR_MASK) != TEGRA_USB1_BASE)
		return;
	/* For EHCI_PS_CSC to be cleared in ehci_hcd.c */
	if (ehci_readl(status_reg) & EHCI_PS_CSC)
		*reg |= EHCI_PS_CSC;
}

/*
 * This ehci_set_usbmode overrides the weak function ehci_set_usbmode
 * in "ehci-hcd.c".
 */
void ehci_set_usbmode(int index)
{
	struct fdt_usb *config;
	struct usb_ctlr *usbctlr;
	uint32_t tmp;

	config = &port[index];
	usbctlr = config->reg;

	tmp = ehci_readl(&usbctlr->usb_mode);
	tmp |= USBMODE_CM_HC;
	ehci_writel(&usbctlr->usb_mode, tmp);
}

/*
 * This ehci_get_port_speed overrides the weak function ehci_get_port_speed
 * in "ehci-hcd.c".
 */
int ehci_get_port_speed(struct ehci_hcor *hcor, uint32_t reg)
{
	uint32_t tmp;
	uint32_t *reg_ptr;

	if (is_T30_compatible) {
		reg_ptr = (uint32_t *)((u8 *)&hcor->or_usbcmd + HOSTPC1_DEVLC);
		tmp = ehci_readl(reg_ptr);
		return HOSTPC1_PSPD(tmp);
	} else
		return PORTSC_PSPD(reg);
}

/* Put the port into host mode */
static void set_host_mode(struct fdt_usb *config)
{
	/*
	 * If we are an OTG port, check if remote host is driving VBus and
	 * bail out in this case.
	 */
	if (config->dr_mode == DR_MODE_OTG &&
	    (readl(&config->reg->phy_vbus_sensors) & VBUS_VLD_STS))
		return;

	/*
	 * If not driving, we set the GPIO to enable VBUS. We assume
	 * that the pinmux is set up correctly for this.
	 */
	if (fdt_gpio_isvalid(&config->vbus_gpio)) {
		fdtdec_setup_gpio(&config->vbus_gpio);
		gpio_direction_output(config->vbus_gpio.gpio,
			(config->vbus_gpio.flags & FDT_GPIO_ACTIVE_LOW) ?
				 0 : 1);
		debug("set_host_mode: GPIO %d %s\n", config->vbus_gpio.gpio,
			(config->vbus_gpio.flags & FDT_GPIO_ACTIVE_LOW) ?
				"low" : "high");
	}
}

void usbf_reset_controller(struct fdt_usb *config, struct usb_ctlr *usbctlr)
{
	/* Reset the USB controller with 2us delay */
	reset_periph(config->periph_id, 2);

	/*
	 * Set USB1_NO_LEGACY_MODE to 1, Registers are accessible under
	 * base address
	 */
	if (config->has_legacy_mode)
		setbits_le32(&usbctlr->usb1_legacy_ctrl, USB1_NO_LEGACY_MODE);

	/* Put UTMIP1/3 in reset */
	setbits_le32(&usbctlr->susp_ctrl, UTMIP_RESET);

	/* Enable the UTMIP PHY */
	if (config->utmi)
		setbits_le32(&usbctlr->susp_ctrl, UTMIP_PHY_ENB);
}

/* set up the UTMI USB controller with the parameters provided */
static int init_utmi_usb_controller(struct fdt_usb *config)
{
	u32 val;
	int loop_count;
	const u32 *timing;
	struct usb_ctlr *usbctlr = config->reg;
	struct clk_rst_ctlr *clkrst;
	struct usb_ctlr *usb1ctlr;

	clock_enable(config->periph_id);

	/* Reset the usb controller */
	usbf_reset_controller(config, usbctlr);

	/* Stop crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN low */
	clrbits_le32(&usbctlr->utmip_misc_cfg1, UTMIP_PHY_XTAL_CLOCKEN);

	/* Follow the crystal clock disable by >100ns delay */
	udelay(1);

	/*
	 * To Use the A Session Valid for cable detection logic, VBUS_WAKEUP
	 * mux must be switched to actually use a_sess_vld threshold.
	 */
	if (config->dr_mode == DR_MODE_OTG &&
	    fdt_gpio_isvalid(&config->vbus_gpio))
		clrsetbits_le32(&usbctlr->usb1_legacy_ctrl,
				VBUS_SENSE_CTL_MASK,
				VBUS_SENSE_CTL_A_SESS_VLD <<
				VBUS_SENSE_CTL_SHIFT);

	/*
	 * PLL Delay CONFIGURATION settings. The following parameters control
	 * the bring up of the plls.
	 */
	timing = (u32 *)config->params;
	if (is_T30_compatible)
		goto pll_T30_init;

	val = readl(&usbctlr->utmip_misc_cfg1);
	clrsetbits_le32(&val, UTMIP_PLLU_STABLE_COUNT_MASK,
			timing[PARAM_STABLE_COUNT] <<
			UTMIP_PLLU_STABLE_COUNT_SHIFT);
	clrsetbits_le32(&val, UTMIP_PLL_ACTIVE_DLY_COUNT_MASK,
			timing[PARAM_ACTIVE_DELAY_COUNT] <<
			UTMIP_PLL_ACTIVE_DLY_COUNT_SHIFT);
	writel(val, &usbctlr->utmip_misc_cfg1);

	/* Set PLL enable delay count and crystal frequency count */
	val = readl(&usbctlr->utmip_pll_cfg1);
	clrsetbits_le32(&val, UTMIP_PLLU_ENABLE_DLY_COUNT_MASK,
			timing[PARAM_ENABLE_DELAY_COUNT] <<
			UTMIP_PLLU_ENABLE_DLY_COUNT_SHIFT);
	clrsetbits_le32(&val, UTMIP_XTAL_FREQ_COUNT_MASK,
			timing[PARAM_XTAL_FREQ_COUNT] <<
			UTMIP_XTAL_FREQ_COUNT_SHIFT);
	writel(val, &usbctlr->utmip_pll_cfg1);
	goto pll_init_done;

pll_T30_init:
	clkrst = (struct clk_rst_ctlr *)NV_PA_CLK_RST_BASE;

	val = readl(&clkrst->crc_utmip_pll_cfg2);
	clrsetbits_le32(&val, UTMIP_PLLU_STABLE_COUNT_MASK,
			timing[PARAM_STABLE_COUNT] <<
			UTMIP_PLLU_STABLE_COUNT_SHIFT);
	clrsetbits_le32(&val, UTMIP_PLL_ACTIVE_DLY_COUNT_MASK,
			timing[PARAM_ACTIVE_DELAY_COUNT] <<
			UTMIP_PLL_ACTIVE_DLY_COUNT_SHIFT);
	writel(val, &clkrst->crc_utmip_pll_cfg2);

	/* Set PLL enable delay count and crystal frequency count */
	val = readl(&clkrst->crc_utmip_pll_cfg1);
	clrsetbits_le32(&val, UTMIP_PLLU_ENABLE_DLY_COUNT_MASK,
			timing[PARAM_ENABLE_DELAY_COUNT] <<
			UTMIP_PLLU_ENABLE_DLY_COUNT_SHIFT);
	clrsetbits_le32(&val, UTMIP_XTAL_FREQ_COUNT_MASK,
			timing[PARAM_XTAL_FREQ_COUNT] <<
			UTMIP_XTAL_FREQ_COUNT_SHIFT);
	writel(val, &clkrst->crc_utmip_pll_cfg1);

	/* Disable Power Down state for PLL */
	clrbits_le32(&clkrst->crc_utmip_pll_cfg1,
		     PLLU_POWERDOWN | PLL_ENABLE_POWERDOWN |
		     PLL_ACTIVE_POWERDOWN);

	/* Recommended PHY settings for EYE diagram */
	val = readl(&usbctlr->utmip_xcvr_cfg0);
	clrsetbits_le32(&val, UTMIP_XCVR_SETUP_MASK,
			0x4 << UTMIP_XCVR_SETUP_SHIFT);
	clrsetbits_le32(&val, UTMIP_XCVR_SETUP_MSB_MASK,
			0x3 << UTMIP_XCVR_SETUP_MSB_SHIFT);
	clrsetbits_le32(&val, UTMIP_XCVR_HSSLEW_MSB_MASK,
			0x8 << UTMIP_XCVR_HSSLEW_MSB_SHIFT);
	writel(val, &usbctlr->utmip_xcvr_cfg0);

	clrsetbits_le32(&usbctlr->utmip_xcvr_cfg1,
			UTMIP_XCVR_TERM_RANGE_ADJ_MASK,
			0x7 << UTMIP_XCVR_TERM_RANGE_ADJ_SHIFT);

	/* Some registers can be controlled from USB1 only. */
	if (config->periph_id != PERIPH_ID_USBD) {
		clock_enable(PERIPH_ID_USBD);
		/* Disable Reset if in Reset state */
		reset_set_enable(PERIPH_ID_USBD, 0);
	}
	usb1ctlr = (struct usb_ctlr *)TEGRA_USB1_BASE;
	val = readl(&usb1ctlr->utmip_bias_cfg0);
	setbits_le32(&val, UTMIP_HSDISCON_LEVEL_MSB);
	clrsetbits_le32(&val, UTMIP_HSDISCON_LEVEL_MASK,
			0x1 << UTMIP_HSDISCON_LEVEL_SHIFT);
	clrsetbits_le32(&val, UTMIP_HSSQUELCH_LEVEL_MASK,
			0x2 << UTMIP_HSSQUELCH_LEVEL_SHIFT);
	writel(val, &usb1ctlr->utmip_bias_cfg0);

	/* Miscellaneous setting mentioned in Programming Guide */
	clrbits_le32(&usbctlr->utmip_misc_cfg0, UTMIP_SUSPEND_EXIT_ON_EDGE);
pll_init_done:

	/* Setting the tracking length time */
	clrsetbits_le32(&usbctlr->utmip_bias_cfg1,
			UTMIP_BIAS_PDTRK_COUNT_MASK,
			timing[PARAM_BIAS_TIME] <<
			UTMIP_BIAS_PDTRK_COUNT_SHIFT);

	/* Program debounce time for VBUS to become valid */
	clrsetbits_le32(&usbctlr->utmip_debounce_cfg0,
			UTMIP_DEBOUNCE_CFG0_MASK,
			timing[PARAM_DEBOUNCE_A_TIME] <<
			UTMIP_DEBOUNCE_CFG0_SHIFT);

	setbits_le32(&usbctlr->utmip_tx_cfg0, UTMIP_FS_PREAMBLE_J);

	/* Disable battery charge enabling bit */
	setbits_le32(&usbctlr->utmip_bat_chrg_cfg0, UTMIP_PD_CHRG);

	clrbits_le32(&usbctlr->utmip_xcvr_cfg0, UTMIP_XCVR_LSBIAS_SE);
	setbits_le32(&usbctlr->utmip_spare_cfg0, FUSE_SETUP_SEL);

	/*
	 * Configure the UTMIP_IDLE_WAIT and UTMIP_ELASTIC_LIMIT
	 * Setting these fields, together with default values of the
	 * other fields, results in programming the registers below as
	 * follows:
	 *         UTMIP_HSRX_CFG0 = 0x9168c000
	 *         UTMIP_HSRX_CFG1 = 0x13
	 */

	/* Set PLL enable delay count and Crystal frequency count */
	val = readl(&usbctlr->utmip_hsrx_cfg0);
	clrsetbits_le32(&val, UTMIP_IDLE_WAIT_MASK,
			utmip_idle_wait_delay << UTMIP_IDLE_WAIT_SHIFT);
	clrsetbits_le32(&val, UTMIP_ELASTIC_LIMIT_MASK,
			utmip_elastic_limit << UTMIP_ELASTIC_LIMIT_SHIFT);
	writel(val, &usbctlr->utmip_hsrx_cfg0);

	/* Configure the UTMIP_HS_SYNC_START_DLY */
	clrsetbits_le32(&usbctlr->utmip_hsrx_cfg1,
			UTMIP_HS_SYNC_START_DLY_MASK,
			utmip_hs_sync_start_delay <<
			UTMIP_HS_SYNC_START_DLY_SHIFT);

	/* Preceed the crystal clock disable by >100ns delay. */
	udelay(1);

	/* Resuscitate crystal clock by setting UTMIP_PHY_XTAL_CLOCKEN */
	setbits_le32(&usbctlr->utmip_misc_cfg1, UTMIP_PHY_XTAL_CLOCKEN);

	if (is_T30_compatible) {
		if (config->periph_id == PERIPH_ID_USBD)
			clrbits_le32(&clkrst->crc_utmip_pll_cfg2,
				     UTMIP_FORCE_PD_SAMP_A_POWERDOWN);
		if (config->periph_id == PERIPH_ID_USB3)
			clrbits_le32(&clkrst->crc_utmip_pll_cfg2,
				     UTMIP_FORCE_PD_SAMP_C_POWERDOWN);
	}
	/* Finished the per-controller init. */

	/* De-assert UTMIP_RESET to bring out of reset. */
	clrbits_le32(&usbctlr->susp_ctrl, UTMIP_RESET);

	/* Wait for the phy clock to become valid in 100 ms */
	for (loop_count = 100000; loop_count != 0; loop_count--) {
		if (readl(&usbctlr->susp_ctrl) & USB_PHY_CLK_VALID)
			break;
		udelay(1);
	}
	if (!loop_count)
		return -1;

	/* Disable ICUSB FS/LS transceiver */
	clrbits_le32(&usbctlr->icusb_ctrl, IC_ENB1);

	if (!is_T30_compatible) {
		/* Select UTMI parallel interface */
		if (config->periph_id == PERIPH_ID_USBD) {
			clrsetbits_le32(&usbctlr->port_sc1, PTS1_MASK,
					PTS_UTMI << PTS1_SHIFT);
			clrbits_le32(&usbctlr->port_sc1, STS1);
		} else {
			clrsetbits_le32(&usbctlr->port_sc1, PTS_MASK,
					PTS_UTMI << PTS_SHIFT);
			clrbits_le32(&usbctlr->port_sc1, STS);
		}
	}

	/* Deassert power down state */
	clrbits_le32(&usbctlr->utmip_xcvr_cfg0, UTMIP_FORCE_PD_POWERDOWN |
		     UTMIP_FORCE_PD2_POWERDOWN | UTMIP_FORCE_PDZI_POWERDOWN);
	clrbits_le32(&usbctlr->utmip_xcvr_cfg1, UTMIP_FORCE_PDDISC_POWERDOWN |
		     UTMIP_FORCE_PDCHRP_POWERDOWN |
		     UTMIP_FORCE_PDDR_POWERDOWN);

	if (is_T30_compatible) {
		/*
		 * BIAS Pad Power Down is common among all 3 USB
		 * controllers and can be controlled from USB1 only.
		 */
		usb1ctlr = (struct usb_ctlr *)TEGRA_USB1_BASE;
		clrbits_le32(&usb1ctlr->utmip_bias_cfg0, UTMIP_BIASPD);
		udelay(25);
		clrbits_le32(&usb1ctlr->utmip_bias_cfg1,
			     UTMIP_FORCE_PDTRK_POWERDOWN);
	}
	return 0;
}

#ifdef CONFIG_USB_ULPI
/* if board file does not set a ULPI reference frequency we default to 24MHz */
#ifndef CONFIG_ULPI_REF_CLK
#define CONFIG_ULPI_REF_CLK 24000000
#endif

/* set up the ULPI USB controller with the parameters provided */
static int init_ulpi_usb_controller(struct fdt_usb *config)
{
	u32 val;
	int loop_count;
	struct ulpi_viewport ulpi_vp;
	struct usb_ctlr *usbctlr = config->reg;

	/* set up ULPI reference clock on pllp_out4 */
	clock_enable(PERIPH_ID_DEV2_OUT);
	clock_set_pllout(CLOCK_ID_PERIPH, PLL_OUT4, CONFIG_ULPI_REF_CLK);

	/* reset ULPI phy */
	if (fdt_gpio_isvalid(&config->phy_reset_gpio)) {
		fdtdec_setup_gpio(&config->phy_reset_gpio);
		gpio_direction_output(config->phy_reset_gpio.gpio, 0);
		mdelay(5);
		gpio_set_value(config->phy_reset_gpio.gpio, 1);
	}

	/* Reset the usb controller */
	clock_enable(config->periph_id);
	usbf_reset_controller(config, usbctlr);

	/* enable pinmux bypass */
	setbits_le32(&usbctlr->ulpi_timing_ctrl_0,
		     ULPI_CLKOUT_PINMUX_BYP | ULPI_OUTPUT_PINMUX_BYP);

	/* Select ULPI parallel interface */
	clrsetbits_le32(&usbctlr->port_sc1, PTS_MASK, PTS_ULPI << PTS_SHIFT);

	/* enable ULPI transceiver */
	setbits_le32(&usbctlr->susp_ctrl, ULPI_PHY_ENB);

	/* configure ULPI transceiver timings */
	val = 0;
	writel(val, &usbctlr->ulpi_timing_ctrl_1);

	val |= ULPI_DATA_TRIMMER_SEL(4);
	val |= ULPI_STPDIRNXT_TRIMMER_SEL(4);
	val |= ULPI_DIR_TRIMMER_SEL(4);
	writel(val, &usbctlr->ulpi_timing_ctrl_1);
	udelay(10);

	val |= ULPI_DATA_TRIMMER_LOAD;
	val |= ULPI_STPDIRNXT_TRIMMER_LOAD;
	val |= ULPI_DIR_TRIMMER_LOAD;
	writel(val, &usbctlr->ulpi_timing_ctrl_1);

	/* set up phy for host operation with external vbus supply */
	ulpi_vp.port_num = 0;
	ulpi_vp.viewport_addr = (u32)&usbctlr->ulpi_viewport;

	if (ulpi_init(&ulpi_vp)) {
		printf("Tegra ULPI viewport init failed\n");
		return -1;
	}

	ulpi_set_vbus(&ulpi_vp, 1, 1);
	ulpi_set_vbus_indicator(&ulpi_vp, 1, 1, 0);

	/* enable wakeup events */
	setbits_le32(&usbctlr->port_sc1, WKCN | WKDS | WKOC);

	/* Enable and wait for the phy clock to become valid in 100 ms */
	setbits_le32(&usbctlr->susp_ctrl, USB_SUSP_CLR);
	for (loop_count = 100000; loop_count != 0; loop_count--) {
		if (readl(&usbctlr->susp_ctrl) & USB_PHY_CLK_VALID)
			break;
		udelay(1);
	}
	if (!loop_count)
		return -1;
	clrbits_le32(&usbctlr->susp_ctrl, USB_SUSP_CLR);

	return 0;
}
#else
static int init_ulpi_usb_controller(struct fdt_usb *config)
{
	printf("No code to set up ULPI controller, please enable"
			"CONFIG_USB_ULPI and CONFIG_USB_ULPI_VIEWPORT");
	return -1;
}
#endif

static void config_clock(const u32 *timing)
{
	clock_start_pll(CLOCK_ID_USB,
			timing[PARAM_DIVM], timing[PARAM_DIVN],
			timing[PARAM_DIVP], timing[PARAM_CPCON],
			timing[PARAM_LFCON]);
}

int fdt_decode_usb(const void *blob, int node, unsigned osc_frequency_hz,
	struct fdt_usb *config)
{
	const char *phy, *mode;
	int clk_node = 0, rate;

	/* Find the parameters for our oscillator frequency */
	do {
		clk_node = fdt_node_offset_by_compatible(blob,
				clk_node, "nvidia,usbparams");
		if (clk_node < 0)
			return -FDT_ERR_NOTFOUND;
		rate = fdtdec_get_int(blob, clk_node, "osc-frequency", 0);
	} while (rate != osc_frequency_hz);

	config->reg = (struct usb_ctlr *)fdtdec_get_addr(blob, node, "reg");
	mode = fdt_getprop(blob, node, "dr_mode", NULL);
	if (mode) {
		if (0 == strcmp(mode, "host"))
			config->dr_mode = DR_MODE_HOST;
		else if (0 == strcmp(mode, "peripheral"))
			config->dr_mode = DR_MODE_DEVICE;
		else if (0 == strcmp(mode, "otg"))
			config->dr_mode = DR_MODE_OTG;
		else {
			debug("%s: Cannot decode dr_mode '%s'\n", __func__,
			      mode);
			return -FDT_ERR_NOTFOUND;
		}
	} else {
		config->dr_mode = DR_MODE_HOST;
	}

	phy = fdt_getprop(blob, node, "phy_type", NULL);
	config->utmi = phy && 0 == strcmp("utmi", phy);
	config->ulpi = phy && 0 == strcmp("ulpi", phy);
	config->enabled = fdtdec_get_is_enabled(blob, node);
	config->has_legacy_mode = fdtdec_get_bool(blob, node,
						  "nvidia,has-legacy-mode");
	config->periph_id = clock_decode_periph_id(blob, node);
	if (config->periph_id == PERIPH_ID_NONE) {
		debug("%s: Missing/invalid peripheral ID\n", __func__);
		return -FDT_ERR_NOTFOUND;
	}
	fdtdec_decode_gpio(blob, node, "nvidia,vbus-gpio", &config->vbus_gpio);
	fdtdec_decode_gpio(blob, node, "nvidia,phy-reset-gpio",
			&config->phy_reset_gpio);
	debug("enabled=%d, legacy_mode=%d, utmi=%d, ulpi=%d, periph_id=%d, "
		"vbus=%d, phy_reset=%d, dr_mode=%d\n",
		config->enabled, config->has_legacy_mode, config->utmi,
		config->ulpi, config->periph_id, config->vbus_gpio.gpio,
		config->phy_reset_gpio.gpio, config->dr_mode);

	return fdtdec_get_int_array(blob, clk_node, "params",
		(u32 *)&config->params, PARAM_COUNT);
}

/*
 * Process a list of nodes, adding them to our list of SDMMC ports.
 *
 * @param blob          fdt blob
 * @param node_list     list of nodes to process (any <=0 are ignored)
 * @param count         number of nodes to process
 * @return 0 if ok, -1 on error
 */
static int process_nodes(const void *blob, int node_list[], int count)
{
	struct fdt_usb config;
	int node, i;
	int clk_done = 0;
	unsigned osc_freq = clock_get_rate(CLOCK_ID_OSC);

	port_count = 0;
	for (i = 0; i < count; i++) {
		if (port_count == USB_PORTS_MAX) {
			printf("tegrausb: Cannot register more than %d ports\n"
				, USB_PORTS_MAX);
			return -1;
		}

		debug("USB %d: ", i);
		node = node_list[i];
		if (!node)
			continue;
		if (fdt_decode_usb(blob, node, osc_freq, &config)) {
			debug("Cannot decode USB node %s\n",
			      fdt_get_name(blob, node, NULL));
			return -1;
		}
		if (!clk_done) {
			config_clock((u32 *)&config.params);
			clk_done = 1;
		}
		config.initialized = 0;

		/* add new USB port to the list of available ports */
		port[port_count++] = config;
	}

	return 0;
}

int board_usb_init(const void *blob)
{
	int node_list[USB_PORTS_MAX];
	int count, err;

	is_T30_compatible = 0;
	is_T114_compatible = 0;

	/* count may return <0 on error */
	count = fdtdec_find_aliases_for_id(blob, "usb",
			COMPAT_NVIDIA_TEGRA20_USB, node_list, USB_PORTS_MAX);
	if (!count)
		goto skip_T20_process;
	err = process_nodes(blob, node_list, count);
	if (err) {
		printf("%s: Error processing T20 compatible USB node(s)!\n",
		       __func__);
		return err;
	}
skip_T20_process:

	count = fdtdec_find_aliases_for_id(blob, "usb",
			COMPAT_NVIDIA_TEGRA114_USB, node_list, USB_PORTS_MAX);
	if (count)
		is_T114_compatible = 1;

	count = fdtdec_find_aliases_for_id(blob, "usb",
			COMPAT_NVIDIA_TEGRA30_USB, node_list, USB_PORTS_MAX);
	if (count)
		is_T30_compatible = 1;
	else
		goto skip_T30_process;
	err = process_nodes(blob, node_list, count);
	if (count && err) {
		printf("%s: Error processing T30 compatible USB node(s)!\n",
		       __func__);
		return err;
	}
skip_T30_process:

	return 0;
}

/**
 * Start up the given port number (ports are numbered from 0 on each board).
 * This returns values for the appropriate hccr and hcor addresses to use for
 * USB EHCI operations.
 *
 * @param index	port number to start
 * @param hccr		returns start address of EHCI HCCR registers
 * @param hcor		returns start address of EHCI HCOR registers
 * @return 0 if ok, -1 on error (generally invalid port number)
 */
int ehci_hcd_init(int index, struct ehci_hccr **hccr, struct ehci_hcor **hcor)
{
	struct fdt_usb *config;
	struct usb_ctlr *usbctlr;

	if (index >= port_count)
		return -1;

	config = &port[index];

	/* skip init, if the port is already initialized */
	if (config->initialized)
		goto success;

	if (config->utmi && init_utmi_usb_controller(config)) {
		printf("tegrausb: Cannot init port %d\n", index);
		return -1;
	}

	if (config->ulpi && init_ulpi_usb_controller(config)) {
		printf("tegrausb: Cannot init port %d\n", index);
		return -1;
	}

	set_host_mode(config);

	config->initialized = 1;

success:
	usbctlr = config->reg;
	*hccr = (struct ehci_hccr *)&usbctlr->cap_length;
	*hcor = (struct ehci_hcor *)&usbctlr->usb_cmd;

	if (is_T30_compatible) {
		/* Set to Host mode after Controller Reset was done */
		clrsetbits_le32(&usbctlr->usb_mode, USBMODE_CM_HC,
				USBMODE_CM_HC);
		/* Select UTMI parallel interface after setting host mode */
		if (config->utmi) {
			clrsetbits_le32((char *)&usbctlr->usb_cmd +
					HOSTPC1_DEVLC, PTS_MASK,
					PTS_UTMI << PTS_SHIFT);
			clrbits_le32((char *)&usbctlr->usb_cmd +
				     HOSTPC1_DEVLC, STS);
		}
	}
	return 0;
}

/*
 * Bring down the specified USB controller
 */
int ehci_hcd_stop(int index)
{
	struct usb_ctlr *usbctlr;

	usbctlr = port[index].reg;

	/* Stop controller */
	writel(0, &usbctlr->usb_cmd);
	udelay(1000);

	/* Initiate controller reset */
	writel(2, &usbctlr->usb_cmd);
	udelay(1000);

	port[index].initialized = 0;

	return 0;
}
