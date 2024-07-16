/*
 * XR68C681 DUART serial driver
 *
 * (c) 2024 Colin Maykish
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/sysrq.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mackerel.h>

#define DRIVER_NAME "xr68c681-serial"

static int __init xr68c681_serial_init(void)
{
	int ret;

	printk(KERN_INFO "XR68C681 serial driver (c) 2024 Colin Maykish\n");

	// ret = uart_register_driver(&xr68c681_uart);
	// if (ret)
	// {
	// 	printk(KERN_ERR "Failed to register uart driver\n");
	// 	return ret;
	// }

	// ret = platform_driver_register(&xr68c681_serial_driver);
	// if (ret != 0)
	// {
	// 	printk(KERN_ERR "Failed to register platform serial driver\n");
	// 	uart_unregister_driver(&xr68c681_uart);
	// }

	return 0;
}

static void __exit xr68c681_serial_exit(void)
{
	// platform_driver_unregister(&xr68c681_serial_driver);
	// uart_unregister_driver(&xr68c681_uart);
}

module_init(xr68c681_serial_init);
module_exit(xr68c681_serial_exit);

MODULE_AUTHOR("Colin Maykish");
MODULE_DESCRIPTION("XR68C681 serial driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
