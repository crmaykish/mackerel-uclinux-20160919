/*
 * XR68C681 serial driver
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

struct xr68c681_port
{
	struct uart_port port;
};

static struct xr68c681_port xr68c681_ports[1];

// CONSOLE SETUP

int xr68c681_console_setup(struct console *cp, char *arg)
{
	// TODO: don't rely on the bootloader to set up the DUART
	return 0;
}

void xr68c681_console_write(struct console *co, const char *str, unsigned int count)
{
	unsigned i = 0;

	while (str[i] != 0 && i < count)
	{
		duart_putc(str[i]);
		i++;
	}
}

static struct console xr68c681_console = {
	.name = "ttyS",
	.write = xr68c681_console_write,
	.device = uart_console_device,
	.setup = xr68c681_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
};

static int __init xr68c681_console_init(void)
{
	register_console(&xr68c681_console);
	return 0;
}

console_initcall(xr68c681_console_init);

// SERIAL DRIVER

static struct uart_driver xr68c681_uart = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = "ttyS",
	.major = TTY_MAJOR,
	.minor = 64,
	.nr = 1,
	.cons = &xr68c681_console,
};

static struct uart_ops xr68c681_ops = {
	// .tx_empty = xr68c681_tx_empty,
	// .set_mctrl = xr68c681_set_mctrl,
	// .get_mctrl = xr68c681_get_mctrl,
	// .stop_tx = xr68c681_stop_tx,
	// .start_tx = xr68c681_start_tx,
	// .stop_rx = xr68c681_stop_rx,
	// .enable_ms = xr68c681_enable_ms,
	// .break_ctl = xr68c681_break_ctl,
	// .startup = xr68c681_startup,
	// .shutdown = xr68c681_shutdown,
	// .set_termios = xr68c681_set_termios,
	// .type = xr68c681_type,
	// .release_port = xr68c681_release_port,
	// .request_port = xr68c681_request_port,
	// .config_port = xr68c681_config_port,
	// .verify_port = xr68c681_verify_port,
};

static int xr68c681_serial_probe(struct platform_device *pdev)
{
	duart_putc('X');
	duart_putc('X');
	duart_putc('X');
	duart_putc('X');

	// printk(KERN_INFO "serial_probe()\n");

	// dev_info(&pdev->dev, "serial_probe()\n");

	// struct uart_port *port;
	// port = &xr68c681_ports[pdev->id].port;
	// // port->backup_imr = 0;

	// port->iotype = UPIO_MEM;
	// port->flags = UPF_BOOT_AUTOCONF;
	// port->line = pdev->id;
	// port->ops = &xr68c681_ops;
	// port->fifosize = 1;

	// uart_add_one_port(&xr68c681_uart, &xr68c681_ports[pdev->id].port);
	// platform_set_drvdata(pdev, &xr68c681_ports[pdev->id]);

	return 0;
}

static int xr68c681_serial_remove(struct platform_device *pdev)
{
	printk(KERN_INFO "TODO serial_remove()\n");
}

static int xr68c681_serial_suspend(struct platform_device *pdev)
{
	printk(KERN_INFO "TODO serial_suspend()\n");
}

static int xr68c681_serial_resume(struct platform_device *pdev)
{
	printk(KERN_INFO "TODO serial_resume()\n");
}

static struct platform_driver xr68c681_serial_driver = {
	.probe = xr68c681_serial_probe,
	.remove = xr68c681_serial_remove,
	.suspend = xr68c681_serial_suspend,
	.resume = xr68c681_serial_resume,
	.driver = {
		.name = "xr68c681_duart",
		.owner = THIS_MODULE,
	},
};

static int __init xr68c681_serial_init(void)
{
	int ret;

	// printk(KERN_INFO "XR68C681 serial driver (c) 2024 Colin Maykish\n");

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
	platform_driver_unregister(&xr68c681_serial_driver);
	uart_unregister_driver(&xr68c681_uart);
}

// module_init(xr68c681_serial_init);
// module_exit(xr68c681_serial_exit);

MODULE_AUTHOR("Colin Maykish");
MODULE_DESCRIPTION("XR68C681 serial driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
