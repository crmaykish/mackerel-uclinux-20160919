/*
 *  Accelerated Concepts 6300-DX board support
 *
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2012 Greg Ungerer <greg.ungerer@accelerated.com>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 */

#include <linux/gpio.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/platform_data/pca953x.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/etherdevice.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define	NB_FLASH_BASE		0x1f000000

#define	NB_FLASH_UBOOT_BASE	0x00000000
#define	NB_FLASH_UBOOT_SZ	0x00060000
#define	NB_FLASH_UBOOT_ENV_SZ	0x00010000
#define	NB_FLASH_ART_BASE	0x00070000
#define	NB_FLASH_ART_SZ		0x00010000
#define	NB_FLASH_CONFIG_BASE	0x00080000
#define	NB_FLASH_CONFIG_SZ	0x00380000
#define	NB_FLASH_UKERNEL_BASE	0x00400000
#define	NB_FLASH_UKERNEL_SZ	0x00200000
#define	NB_FLASH_UKERNEL_MIN	0x00080000
#define	NB_FLASH_UKERNEL_MAX	0x00600000
#define	NB_FLASH_IMAGE_SIZE	0x00c00000
#define	NB_FLASH_STORE_BASE	0x01000000
#define	NB_FLASH_STORE_SIZE	0x00400000

#define NB_CALDATA_OFFSET	0x1000
#define	NB_CALDATA_MAC_OFFSET	0x1002
#define	NB_CALDATA_SMAC_OFFSET	0x1009

/*
 * Define the CPU GPIO pins.
 */
#define	NB_GPIO_RSSI_5		1
#define	NB_GPIO_RSSI_4		13
#define	NB_GPIO_RSSI_3		14
#define	NB_GPIO_RSSI_2		15
#define	NB_GPIO_RSSI_1		16
#define	NB_GPIO_WIFI		17
#define	NB_GPIO_LAN_LED_1	19
#define	NB_GPIO_LAN_LED_3	21
#define	NB_GPIO_LAN_LED_2	22
#define	NB_GPIO_LAN_LED_0	23

#define	NB_GPIO_WDT_WDI		26

#define	NB_GPIO_IO_INTR		27

#define	NB_GPIO_RESET		12

/*
 * Define the I2C I/O expander GPIO pins.
 */
#define	NB_GPIO_IO_BASE		32
#define	NB_GPIO_USB_OC2		32
#define	NB_GPIO_USB_ENABLE2	33
#define	NB_GPIO_SW_LEFT		35
#define	NB_GPIO_SW_CENTER	36
#define	NB_GPIO_SW_UP		37
#define	NB_GPIO_SW_RIGHT	38
#define	NB_GPIO_SW_DOWN		39
#define	NB_GPIO_USB_ENABLE1	40
#define	NB_GPIO_USB_OC1		41
#define	NB_GPIO_USB_HUB_RESET	42
#define	NB_GPIO_USB_OC3		43
#define	NB_GPIO_USB_ENABLE3	44

#define	KEY_POLL_INTERVAL	50	/* ms */
#define	KEY_DEBOUNCE_INTERVAL	(2 * KEY_POLL_INTERVAL)

#define	OC_POLL_INTERVAL	500	/* ms */
#define	OC_TIMEOUT		10000	/* ms */

/*
 * Base MAC addresses. We need a default for the kernel, the real values
 * are set once we hit user space.
 */
static u8 nb_macs[] = {
	0x00, 0x27, 0x04, 0x03, 0x02, 0x01,
	0x00, 0x27, 0x04, 0x03, 0x02, 0x02,
	0x00, 0x27, 0x04, 0x03, 0x02, 0x03,
};

/*
 * Define our flash layout.
 *
 * Strait forward layout, excepting the kernel+rootfs partitions. The kernel
 * (actually the uImage kernel file) is stored at a fixed offset. The root
 * filesystem is stored after the kernel, on the next flash 64k boundary. So
 * it is not at a fixed constant offset, but at an offset that depends on the
 * kernels binary size. We adjust the partition table boundaries at run time.
 */
static struct mtd_partition nb_partitions[] = {
	{
		.name		= "u-boot",
		.offset		= 0,
		.size		= NB_FLASH_UBOOT_SZ,
	}, {
		.name		= "u-boot-env",
		.offset		= NB_FLASH_UBOOT_SZ,
		.size		= NB_FLASH_UBOOT_ENV_SZ,
	}, {
		.name		= "art",
		.offset		= NB_FLASH_ART_BASE,
		.size		= NB_FLASH_ART_SZ,
	}, {
		.name		= "config",
		.offset		= NB_FLASH_CONFIG_BASE,
		.size		= NB_FLASH_CONFIG_SZ,
	}, {
		.name		= "kernel",
		.offset		= NB_FLASH_UKERNEL_BASE,
		.size		= NB_FLASH_UKERNEL_SZ,
	}, {
		.name		= "rootfs",
		.offset		= NB_FLASH_UKERNEL_BASE + NB_FLASH_UKERNEL_SZ,
	}, {
		.name		= "image",
		.offset		= NB_FLASH_UKERNEL_BASE,
		.size		= NB_FLASH_IMAGE_SIZE,
	}, {
		.name		= "all",
	}, {
		.name		= "store",
		.offset		= NB_FLASH_STORE_BASE,
		.size		= NB_FLASH_STORE_SIZE,
	}, {
		.name		= "kernel1",
		.offset		= NB_FLASH_STORE_BASE + NB_FLASH_UKERNEL_BASE,
		.size		= NB_FLASH_UKERNEL_SZ,
	}, {
		.name		= "rootfs1",
		.offset		= NB_FLASH_STORE_BASE + NB_FLASH_UKERNEL_BASE +
				  NB_FLASH_UKERNEL_SZ,
	}, {
		.name		= "image1",
		.offset		= NB_FLASH_STORE_BASE + NB_FLASH_UKERNEL_BASE,
		.size		= NB_FLASH_IMAGE_SIZE,
	}
};

static struct flash_platform_data nb_flash_data = {
	.parts		= nb_partitions,
	.nr_parts	= ARRAY_SIZE(nb_partitions),
};

static struct i2c_gpio_platform_data nb_i2c_gpio_data = {
	.sda_pin        = 20,
	.scl_pin        = 18,
};

static struct platform_device nb_i2c_gpio_device = {
	.name		= "i2c-gpio",
	.id		= 0,
	.dev = {
		.platform_data	= &nb_i2c_gpio_data,
	}
};

static struct pca953x_platform_data nb_pca953x_data = {
	.gpio_base	= NB_GPIO_IO_BASE,
};

static struct i2c_board_info nb_i2c_board_info[] __initdata = {
	{
		I2C_BOARD_INFO("pca9555", 0x20),
		.platform_data  = &nb_pca953x_data,
	},
	{
		I2C_BOARD_INFO("pcf2116", 0x3a),
	},
};

#ifndef CONFIG_LEDMAN
/*
 * Define the board LEDs (only if ledman is not being used).
 */
static struct gpio_led nb_gpio_leds[] __initdata = {
	{
		.name		= "rssi1",
		.gpio		= NB_GPIO_RSSI_1,
		.active_low	= 0,
	},
	{
		.name		= "rssi2",
		.gpio		= NB_GPIO_RSSI_2,
		.active_low	= 0,
	},
	{
		.name		= "rssi3",
		.gpio		= NB_GPIO_RSSI_3,
		.active_low	= 0,
	},
	{
		.name		= "rssi4",
		.gpio		= NB_GPIO_RSSI_4,
		.active_low	= 0,
	},
	{
		.name		= "rssi5",
		.gpio		= NB_GPIO_RSSI_5,
		.active_low	= 0,
	},
	{
		.name		= "wifi",
		.gpio		= NB_GPIO_WIFI,
		.active_low	= 0,
	},
	{
		.name		= "lanled0",
		.gpio		= NB_GPIO_LAN_LED_0,
		.active_low	= 0,
	},
	{
		.name		= "lanled1",
		.gpio		= NB_GPIO_LAN_LED_1,
		.active_low	= 0,
	},
	{
		.name		= "lanled2",
		.gpio		= NB_GPIO_LAN_LED_2,
		.active_low	= 0,
	},
	{
		.name		= "lanled3",
		.gpio		= NB_GPIO_LAN_LED_3,
		.active_low	= 0,
	},
};
#endif /* CONFIG_LEDMAN */

/*
 * Define the JOG switch buttons for the gpio-keys input subsystem.
 */
static struct gpio_keys_button nb_gpio_keys[] __initdata = {
	{
		.desc		= "Reset button",
		.type		= EV_KEY,
		.code		= BTN_0,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_RESET,
		.active_low	= 1,
	},
	{
		.desc		= "Left button",
		.type		= EV_KEY,
		.code		= KEY_LEFT,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_SW_LEFT,
		.active_low	= 1,
	},
	{
		.desc		= "Center button",
		.type		= EV_KEY,
		.code		= KEY_ENTER,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_SW_CENTER,
		.active_low	= 1,
	},
	{
		.desc		= "Up button",
		.type		= EV_KEY,
		.code		= KEY_UP,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_SW_UP,
		.active_low	= 1,
	},
	{
		.desc		= "Right button",
		.type		= EV_KEY,
		.code		= KEY_RIGHT,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_SW_RIGHT,
		.active_low	= 1,
	},
	{
		.desc		= "Down button",
		.type		= EV_KEY,
		.code		= KEY_DOWN,
		.debounce_interval = KEY_DEBOUNCE_INTERVAL,
		.gpio		= NB_GPIO_SW_DOWN,
		.active_low	= 1,
	}
};

static void __init nb_soc_gpio_setup(void)
{
	u32 t;

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	t |= AR933X_BOOTSTRAP_MDIO_GPIO_EN;
	ath79_reset_wr(AR933X_RESET_REG_BOOTSTRAP, t);

	gpio_request(NB_GPIO_IO_INTR, "IO_INT");
	gpio_direction_input(NB_GPIO_IO_INTR);
}

/*
 * Over-current setup/detect and power control for the three USB ports.
 * We poll the over-current bits and turn off power to the USB ports if
 * we detect an over-current condition.
 */
struct gpio_usb_oc {
	char		*oc_name;
	int		oc_gpio;
	char		*pwr_name;
	int		pwr_gpio;
	unsigned int	pwr_state;
	unsigned long	pwr_time;
};

static struct gpio_usb_oc nb_usb_oc[] = {
	{
		.oc_name	= "USB1 over-current",
		.oc_gpio	= NB_GPIO_USB_OC1,
		.pwr_name	= "USB1 power",
		.pwr_gpio	= NB_GPIO_USB_ENABLE1,
	},
	{
		.oc_name	= "USB2 over-current",
		.oc_gpio	= NB_GPIO_USB_OC2,
		.pwr_name	= "USB2 power",
		.pwr_gpio	= NB_GPIO_USB_ENABLE2,
	},
	{
		.oc_name	= "USB3 over-current",
		.oc_gpio	= NB_GPIO_USB_OC3,
		.pwr_name	= "USB3 power",
		.pwr_gpio	= NB_GPIO_USB_ENABLE3,
	},
};

static struct task_struct *nb_threadp;

static void nb_usb_power_up(struct gpio_usb_oc *ucp)
{
	gpio_set_value_cansleep(ucp->pwr_gpio, 0);
	ucp->pwr_time = jiffies;
	ucp->pwr_state = 1;
}

static void nb_usb_power_down(struct gpio_usb_oc *ucp)
{
	gpio_set_value_cansleep(ucp->pwr_gpio, 1);
	ucp->pwr_time = jiffies;
	ucp->pwr_state = 0;
}

static void nb_check_usb(struct gpio_usb_oc *ucp)
{
	if (ucp->pwr_state) {
		if (gpio_get_value_cansleep(ucp->oc_gpio) == 0) {
			printk(KERN_WARNING "NB: %s over-current "
				"condition, temporarily disabling\n",
				ucp->pwr_name);
			nb_usb_power_down(ucp);
		}
	} else {
		unsigned long t = ucp->pwr_time + msecs_to_jiffies(OC_TIMEOUT);
		if (time_is_before_jiffies(t)) {
			printk(KERN_WARNING "NB: restarting %s\n",
				ucp->pwr_name);
			nb_usb_power_up(ucp);
		}
	}
}

static int __init nb_ocinit(void)
{
	struct gpio_usb_oc *ucp = &nb_usb_oc[0];
	int i;

	for (i = 0; i < ARRAY_SIZE(nb_usb_oc); i++, ucp++) {
		gpio_request(ucp->oc_gpio, ucp->oc_name);
		gpio_direction_input(ucp->oc_gpio);
		gpio_export(ucp->oc_gpio, 0);
		gpio_request(ucp->pwr_gpio, ucp->pwr_name);
		gpio_direction_output(ucp->pwr_gpio, 1);
		gpio_export(ucp->pwr_gpio, 0);
		nb_usb_power_up(ucp);
	}
	return 0;
}

static int nb_ocpoll(void *data)
{
	int i;

	for (;;) {
		for (i = 0; i < ARRAY_SIZE(nb_usb_oc); i++)
			nb_check_usb(&nb_usb_oc[i]);
		msleep(OC_POLL_INTERVAL);
	}

	return 0;
}

static int __init nb_i2c_gpio_setup(void)
{
	/* Turn on the USB power chips, then reset USB hub */
	nb_ocinit();
	nb_threadp = kthread_run(nb_ocpoll, NULL, "usbocpoll");

	gpio_request(NB_GPIO_USB_HUB_RESET, "USB Hub Reset");
	gpio_direction_output(NB_GPIO_USB_HUB_RESET, 1);
	gpio_export(NB_GPIO_USB_HUB_RESET, 0);
	mdelay(20);
	gpio_set_value_cansleep(NB_GPIO_USB_HUB_RESET, 0);
	mdelay(20);
	gpio_set_value_cansleep(NB_GPIO_USB_HUB_RESET, 1);

	/* Setup the JOG switch keys */
	ath79_register_gpio_keys_polled(-1, KEY_POLL_INTERVAL,
		ARRAY_SIZE(nb_gpio_keys), nb_gpio_keys);

	return 0;
}
late_initcall(nb_i2c_gpio_setup);

#ifdef CONFIG_PROM_ART
/*
 * We need to get the ART data region out of the flash before the mtd driver
 * takes over driving it. It will be direct mapped during early startup. The
 * array we stash it in will be freed with the init data at the end of kernel
 * startup.
 */
static u8 __initdata nb_art[NB_FLASH_ART_SZ];

/*
 * We try to be a little clever with the kernel+rootfs images so they don't
 * waste too much flash space. The root filesystem is always at the next 64k
 * boundary after the kernel uimage. So we need to adjust the MTD partitions
 * to match the actual kernel and filesystem we are loading. The kernel will
 * pass in the size of the uimage in its environment (we process that in the
 * ath79 prom code here in the kernel) and we use that here to adjust the
 * offset of the root filesystem.
 */
static void __init nb_adjust_partitions(int p, unsigned int ksize)
{
	/* Align to 64k boundary and sanity check */
	ksize = (ksize + 0xffff) & 0xffff0000;
	if ((ksize < NB_FLASH_UKERNEL_MIN) || (ksize >= NB_FLASH_UKERNEL_MAX))
		return;

	nb_partitions[p].size = ksize;
	nb_partitions[p + 1].offset = nb_partitions[p].offset + ksize;
}

/*
 * If we got passed eth MAC addresses from u-boot (processed via the prom
 * code) and they are valid we will use them. Otherwise we default to a
 * base set of Accelerated Concepts ones.
 */
static void __init nb_get_prom_ethaddrs(void)
{
	extern u8 ath9k_prom_eth0addr[6];
	extern u8 ath9k_prom_eth1addr[6];
	extern u8 ath9k_prom_eth2addr[6];

	if (is_valid_ether_addr(ath9k_prom_eth0addr))
		memcpy(&nb_macs[0], ath9k_prom_eth0addr, 6);
	if (is_valid_ether_addr(ath9k_prom_eth1addr))
		memcpy(&nb_macs[6], ath9k_prom_eth1addr, 6);
	if (is_valid_ether_addr(ath9k_prom_eth2addr))
		memcpy(&nb_macs[12], ath9k_prom_eth2addr, 6);

	memcpy(nb_art + NB_CALDATA_MAC_OFFSET, ath9k_prom_eth2addr, 6);
}
#endif

static void __init nb_setup(void)
{
#ifdef CONFIG_PROM_ART
	u8 *art = (u8 *) KSEG1ADDR(NB_FLASH_BASE + NB_FLASH_ART_BASE);
	extern unsigned long ath79_prom_isize;

	memcpy(nb_art, art, sizeof(nb_art));
	nb_adjust_partitions(4, ath79_prom_isize);
	nb_adjust_partitions(9, ath79_prom_isize);
	nb_get_prom_ethaddrs();
#endif

	nb_soc_gpio_setup();

	i2c_register_board_info(0, nb_i2c_board_info,
		ARRAY_SIZE(nb_i2c_board_info));
	platform_device_register(&nb_i2c_gpio_device);

#ifndef CONFIG_LEDMAN
	ath79_register_leds_gpio(-1, ARRAY_SIZE(nb_gpio_leds), nb_gpio_leds);
#endif

	ath79_register_m25p80(&nb_flash_data);
	ath79_init_mac(ath79_eth1_data.mac_addr, &nb_macs[0], 0);
	ath79_init_mac(ath79_eth0_data.mac_addr, &nb_macs[6], 0);
	ath79_register_mdio(0, 0x0);
	ath79_register_eth(1);
	ath79_register_eth(0);
#ifdef CONFIG_PROM_ART
	ath79_register_wmac(nb_art + NB_CALDATA_OFFSET, NULL);
#endif
	ath79_register_usb();
}

MIPS_MACHINE(ATH79_MACH_6300_DX, "6300-DX",
	"Accelerated Concepts 6300-DX", nb_setup);
