/*
 * Copyright 2013 Greg Ungerer, <greg.ungerer@accelerated.com>
 * Copyright 2009 Sascha Hauer, <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/usb/otg.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/memory.h>
#include <asm/mach/map.h>

#include "common.h"
#include "devices-imx25.h"
#include "ehci.h"
#include "hardware.h"
#include "iomux-mx25.h"
#include "mx25.h"

static struct class *accelerated_class;
static struct device *sensor_dev;

static const struct imxuart_platform_data uart_pdata __initconst = {
	.flags = IMXUART_HAVE_RTSCTS | IMXUART_HAVE_DTRDCD | IMXUART_DTE,
};
static const struct imxuart_platform_data uart_pdata1 __initconst = {
};
static const struct imxuart_platform_data uart_pdata4 __initconst = {
	.flags = IMXUART_HAVE_RTSCTS | IMXUART_DTE |
	         IMXUART_HAVE_DTR_GPIO | IMXUART_HAVE_DCD_GPIO,
	.dcd_gpio = 29,
	.dtr_gpio = 30,
};

static iomux_v3_cfg_t dcd_pads[] = {
	/* GPIO (LEDS) */
	MX25_PAD_KPP_ROW3__GPIO_3_0,
	MX25_PAD_KPP_COL0__GPIO_3_1,
	MX25_PAD_KPP_COL1__GPIO_3_2,

	/* UART1 */
	MX25_PAD_UART1_TXD__UART1_TXD,
	MX25_PAD_UART1_RXD__UART1_RXD,
	MX25_PAD_UART1_RTS__UART1_RTS,
	MX25_PAD_UART1_CTS__UART1_CTS,
	MX25_PAD_KPP_ROW0__UART1_DTR,
	MX25_PAD_KPP_ROW2__UART1_DCD,

	/* UART2 */
	MX25_PAD_UART2_TXD__UART2_TXD,
	MX25_PAD_UART2_RXD__UART2_RXD,

	/* UART5 */
	MX25_PAD_CSI_D2__UART5_RXD_MUX,
	MX25_PAD_CSI_D3__UART5_TXD_MUX,
	MX25_PAD_CS4__UART5_CTS,    /* prototype hooked to gpio 4_10 */
	MX25_PAD_CS5__UART5_RTS,    /* prototype hooked to gpio 4_11 */
	MX25_PAD_CSI_D4__GPIO_1_29, /* DSR */
	MX25_PAD_CSI_D5__GPIO_1_30, /* DTR */

	/* MODEM */
	MX25_PAD_EXT_ARMCLK__GPIO_3_15,
	MX25_PAD_HSYNC__GPIO_1_22, /* Modem interrupt */

	/* SLIC */
	MX25_PAD_KPP_COL3__GPIO_3_4,
	MX25_PAD_ECB__GPIO_3_23, /* SLIC interrupt */
	MX25_PAD_LBA__GPIO_3_24, /* SIM detect */

	/* ERASE BUTTON */
	MX25_PAD_KPP_COL2__GPIO_3_3,

	/* Watchdog */
	MX25_PAD_CONTRAST__WDOG_B,

	/* FEC */
	MX25_PAD_FEC_MDC__FEC_MDC,
	MX25_PAD_FEC_MDIO__FEC_MDIO,
	MX25_PAD_FEC_RDATA0__FEC_RDATA0,
	MX25_PAD_FEC_RDATA1__FEC_RDATA1,
	MX25_PAD_FEC_RX_DV__FEC_RX_DV,
	MX25_PAD_FEC_TDATA0__FEC_TDATA0,
	MX25_PAD_FEC_TDATA1__FEC_TDATA1,
	MX25_PAD_FEC_TX_CLK__FEC_TX_CLK,
	MX25_PAD_FEC_TX_EN__FEC_TX_EN,

	/* SPI1 */
	MX25_PAD_CSPI1_MISO__CSPI1_MISO,
	MX25_PAD_CSPI1_MOSI__CSPI1_MOSI,
	MX25_PAD_CSPI1_RDY__CSPI1_RDY,
	MX25_PAD_CSPI1_SCLK__CSPI1_SCLK,
	MX25_PAD_CSPI1_SS0__CSPI1_SS0,
	MX25_PAD_CSPI1_SS1__GPIO_1_17,

	/* SPI2 */
	MX25_PAD_SD1_CMD__CSPI2_MOSI,
	MX25_PAD_SD1_CLK__CSPI2_MISO,
	MX25_PAD_SD1_DATA0__CSPI2_SCLK,
	MX25_PAD_SD1_DATA3__CSPI2_SS1,

	/* AUD3 */
	MX25_PAD_LD14__AUD3_RXC,
	MX25_PAD_LD9__AUD3_RXD,
	MX25_PAD_LD15__AUD3_RXFS,
	MX25_PAD_LD10__AUD3_TXC,
	MX25_PAD_LD8__AUD3_TXD,
	MX25_PAD_LD11__AUD3_TXFS,
};

#define	FLASH_KERNEL_MIN	0x00080000
#define	FLASH_KERNEL_MAX	0x00200000

static struct mtd_partition dcd_partitions[] = {
	{
		.name	= "u-boot",
		.offset	= 0,
		.size	= 0x00030000,
	},
	{
		.name	= "u-boot-env",
		.offset	= 0x00030000,
		.size	= 0x00010000,
	},
	{
		.name	= "config",
		.offset	= 0x00040000,
		.size	= 0x000c0000,
	},
	{
		.name	= "image",
		.offset	= 0x00100000,
		.size	= 0x00780000,
	},
	{
		.name	= "image1",
		.offset	= 0x00880000,
		.size	= 0x00780000,
	},
	{
		.name	= "all",
		.offset	= 0,
	},
};

static const struct flash_platform_data dcd_flash_data = {
	.parts		= dcd_partitions,
	.nr_parts	= ARRAY_SIZE(dcd_partitions),
};

static const struct fec_platform_data dcd_fec_pdata __initconst = {
	.phy = PHY_INTERFACE_MODE_RMII,
	.phy_addr = 2,
};

static int dcd_usbotg_init(struct platform_device *pdev)
{
	return mx25_initialize_usb_hw(pdev->id, MXC_EHCI_INTERFACE_DIFF_UNI);
}

static const struct mxc_usbh_platform_data otg_pdata __initconst = {
	.init	= dcd_usbotg_init,
	.portsc	= MXC_EHCI_MODE_UTMI,
};

static struct spi_board_info dcd_spi_flash_info[] = {
	{
		.modalias = "m25p80",
		.max_speed_hz = 25000000,
		.bus_num = 0,
		.chip_select = 1,
		.mode = SPI_MODE_0,
		.platform_data = &dcd_flash_data,
	},
};

static struct spi_board_info dcd_spi_slic_info[] = {
	{
		.modalias = "si32260",
		.max_speed_hz = 25000000,
		.bus_num = 1,
		.chip_select = 1,
		.mode = SPI_MODE_0 | SPI_CPOL | SPI_CPHA,
	},
};

static int dcd_spi0_cs[] = { -32, 17, -30, -29, };
static int dcd_spi1_cs[] = { -32, -31, -30, -29, };

static const struct spi_imx_master dcd_spi0_data __initconst = {
	.chipselect	= dcd_spi0_cs,
	.num_chipselect	= ARRAY_SIZE(dcd_spi0_cs),
};

static const struct spi_imx_master dcd_spi1_data __initconst = {
	.chipselect	= dcd_spi1_cs,
	.num_chipselect	= ARRAY_SIZE(dcd_spi1_cs),
};

static void dcd_dev_init(void)
{
	accelerated_class = class_create(THIS_MODULE, "accelerated");
	if (IS_ERR(accelerated_class))
		return;

	sensor_dev = device_create(accelerated_class, NULL, 0, NULL, "sensor");
	if (IS_ERR(sensor_dev))
		sensor_dev = NULL;

	/* if nothing is using the class, destroy it */
	if (!sensor_dev) {
		class_destroy(accelerated_class);
		accelerated_class = NULL;
	}
}

#define	GPIO_MODEMIRQ	IMX_GPIO_NR(1, 22)
#define	GPIO_UART4DCD	IMX_GPIO_NR(1, 29)
#define	GPIO_UART4DTR	IMX_GPIO_NR(1, 30)
#define	GPIO_MODEMRESET	IMX_GPIO_NR(3, 15)
#define	GPIO_UART1DCD	IMX_GPIO_NR(2, 31)
#define	GPIO_SIMDETECT	IMX_GPIO_NR(3, 24)

static void dcd_hw_init(void)
{
	gpio_request(GPIO_MODEMRESET, "MODEM RESET");
	gpio_direction_output(GPIO_MODEMRESET, 1);
	gpio_request(GPIO_MODEMIRQ, "MODEM IRQ");
	gpio_direction_output(GPIO_MODEMIRQ, 1);

	mxc_iomux_v3_setup_pad(MX25_PAD_KPP_ROW2__GPIO_2_31);
	gpio_request(GPIO_UART1DCD, "UART1 DCD");
	gpio_direction_output(GPIO_UART1DCD, 1);

	/*
	 * Reset sequence for the modem.
	 */
	mdelay(20);
	gpio_set_value(GPIO_MODEMRESET, 0);
	mdelay(500);
	gpio_set_value(GPIO_MODEMRESET, 1);
	mdelay(20);

	gpio_direction_input(GPIO_MODEMIRQ);

	gpio_direction_input(GPIO_UART1DCD);
	gpio_free(GPIO_UART1DCD);
	mxc_iomux_v3_setup_pad(MX25_PAD_KPP_ROW2__UART1_DCD);

	gpio_request(GPIO_SIMDETECT, "SIM DETECT");
	gpio_direction_input(GPIO_SIMDETECT);
	gpio_export(GPIO_SIMDETECT, 0);
	gpio_export_link(sensor_dev, "sim_detect", GPIO_SIMDETECT);
}

static void __init dcd_init(void)
{
	imx25_soc_init();
	mxc_iomux_v3_setup_multiple_pads(dcd_pads, ARRAY_SIZE(dcd_pads));

	dcd_dev_init();

	dcd_hw_init();

	imx25_add_imx_uart0(&uart_pdata);
	imx25_add_imx_uart1(&uart_pdata1);
	imx25_add_imx_uart4(&uart_pdata4);

	imx25_add_mxc_ehci_otg(&otg_pdata);

	imx25_add_imx2_wdt();

	imx25_add_fec(&dcd_fec_pdata);

	imx25_add_spi_imx0(&dcd_spi0_data);
	spi_register_board_info(dcd_spi_flash_info,
                ARRAY_SIZE(dcd_spi_flash_info));

	imx25_add_spi_imx1(&dcd_spi1_data);
	spi_register_board_info(dcd_spi_slic_info,
                ARRAY_SIZE(dcd_spi_slic_info));
}

static void __init dcd_timer_init(void)
{
	mx25_clocks_init();
}

MACHINE_START(MX25_3DS, "5300-DC")
	/* Maintainer: Accelerated Concepts Inc */
	.atag_offset = 0x100,
	.map_io = mx25_map_io,
	.init_early = imx25_init_early,
	.init_irq = mx25_init_irq,
	.init_time = dcd_timer_init,
	.init_machine = dcd_init,
	.restart = mxc_restart,
MACHINE_END
