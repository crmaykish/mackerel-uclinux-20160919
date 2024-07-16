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
#define XR68C681_SERIAL_DEV_NAME "ttyS"
#define XR68C681_SERIAL_MINOR 64
#define PORT_XR68C681 2000

struct xr68c681_uart_port
{
	struct uart_port port;
};

// TODO: this chip actually has two ports
static struct xr68c681_uart_port xr68c681_uart_ports[1];
#define XR68C681_MAXPORTS ARRAY_SIZE(xr68c681_uart_ports)

#ifdef CONFIG_SERIAL_XR68C681_CONSOLE
static struct console xr68c681_console;
#endif

/* UART driver structure. */
static struct uart_driver xr68c681_driver = {
	.owner = THIS_MODULE,
	.driver_name = DRIVER_NAME,
	.dev_name = XR68C681_SERIAL_DEV_NAME,
	.major = TTY_MAJOR,
	.minor = XR68C681_SERIAL_MINOR,
	.nr = XR68C681_MAXPORTS,
#ifdef CONFIG_SERIAL_XR68C681_CONSOLE
	.cons = &xr68c681_console,
#endif
};

static void xr68c681_stop_rx(struct uart_port *port)
{
	// INT_CTRL &= ~FTDI_RXIE;
}

static void xr68c681_stop_tx(struct uart_port *port)
{
	// INT_CTRL &= ~FTDI_TXIE;
}

static unsigned int xr68c681_tx_empty(struct uart_port *port)
{
	// return (FTDI_STAT & FTDI_TXE) ? 0 : TIOCSER_TEMT;

	// TODO: backwards?

	if (MEM(DUART1_SRB) & 0b00000100)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

static void xr68c681_tx_chars(struct xr68c681_uart_port *pp)
{
	// struct uart_port *port = &pp->port;
	// struct circ_buf *xmit = &port->state->xmit;

	// if (port->x_char) {
	// 	/* Send special char - probably flow control */
	// 	FTDI_DATA = port->x_char;
	// 	port->x_char = 0;
	// 	port->icount.tx++;
	// 	return;
	// }

	// if (uart_circ_empty(xmit) || uart_tx_stopped(port)) {
	// 	INT_CTRL &= ~FTDI_TXIE;
	// 	return;
	// }

	// while (!(FTDI_STAT & FTDI_TXE)) {
	// 	FTDI_DATA = xmit->buf[xmit->tail];
	// 	xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE -1);
	// 	port->icount.tx++;
	// 	if (uart_circ_empty(xmit))
	// 		break;
	// }

	// if (uart_circ_chars_pending(xmit) < WAKEUP_CHARS)
	// 	uart_write_wakeup(port);

	// if (uart_circ_empty(xmit))
	// 	INT_CTRL &= ~FTDI_TXIE;
}

static void xr68c681_uart_putchar(struct uart_port *port, int ch) {
	duart_putc(ch);
}

static void xr68c681_start_tx(struct uart_port *port)
{
	// INT_CTRL |= FTDI_TXIE;

	struct circ_buf *xmit = &port->state->xmit;

	while (!uart_circ_empty(xmit))
	{
		xr68c681_uart_putchar(port, xmit->buf[xmit->tail]);
		xmit->tail = (xmit->tail + 1) & (UART_XMIT_SIZE - 1);
		port->icount.tx++;
	}
}

static void xr68c681_rx_chars(struct xr68c681_uart_port *pp)
{
	struct uart_port *port = &pp->port;
	unsigned char ch, flag;

	// while (!(FTDI_STAT & FTDI_RXF)) {
	// 	ch = FTDI_DATA;
	// 	flag = TTY_NORMAL;
	// 	port->icount.rx++;

	// 	if (uart_handle_sysrq_char(port, ch))
	// 		continue;
	// 	tty_insert_flip_char(&port->state->port, ch, flag);
	// }

	// tty_flip_buffer_push(&port->state->port);
}

static irqreturn_t xr68c681_interrupt(int irq, void *data)
{
	struct uart_port *port = data;
	struct xr68c681_uart_port *pp = container_of(port, struct xr68c681_uart_port, port);
	irqreturn_t ret = IRQ_NONE;

	unsigned long flags;

	// if (!(FTDI_STAT & FTDI_RXF)) {
	// 	xr68c681_rx_chars(pp);
	// 	ret = IRQ_HANDLED;
	// } else

	// if (!(FTDI_STAT & FTDI_TXE)) {
	// 	xr68c681_tx_chars(pp);
	// 	ret = IRQ_HANDLED;
	// }

	return ret;
}

static unsigned int xr68c681_get_mctrl(struct uart_port *port)
{
	return TIOCM_CTS | TIOCM_DSR | TIOCM_CAR;
}

static void xr68c681_set_mctrl(struct uart_port *port, unsigned int sigs)
{
}

static void xr68c681_enable_ms(struct uart_port *port)
{
}

static void xr68c681_break_ctl(struct uart_port *port, int break_state)
{
}

static int xr68c681_startup(struct uart_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	printk(KERN_INFO "startup()\n");

	// INT_CTRL &= ~(FTDI_IEN | FTDI_RXIE | FTDI_TXIE);

	if (request_irq(port->irq, xr68c681_interrupt, 0, "XR68C681UART", port))
		printk(KERN_ERR "XR68C681UART: unable to attach XR68C681UART %d "
						"interrupt vector=%d\n",
			   port->line, port->irq);

	/* Enable RX interrupts now */
	// INT_CTRL |= FTDI_IEN | FTDI_RXIE;

	spin_unlock_irqrestore(&port->lock, flags);

	return 0;
}

static void xr68c681_shutdown(struct uart_port *port)
{
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);

	/* Disable all interrupts now */
	// INT_CTRL &= ~(FTDI_IEN | FTDI_RXIE | FTDI_TXIE);
	free_irq(port->irq, port);

	spin_unlock_irqrestore(&port->lock, flags);
}

static void xr68c681_set_termios(struct uart_port *port, struct ktermios *termios,
								 struct ktermios *old)
{
}

static const char *xr68c681_type(struct uart_port *port)
{
	return (port->type == PORT_XR68C681) ? "XR68C681 DUART module" : NULL;
}

static void xr68c681_release_port(struct uart_port *port)
{
	/* Nothing to release... */
}

static int xr68c681_verify_port(struct uart_port *port, struct serial_struct *ser)
{
	if ((ser->type != PORT_UNKNOWN) && (ser->type != PORT_XR68C681))
		return -EINVAL;
	return 0;
}

static int xr68c681_request_port(struct uart_port *port)
{
	/* UARTs always present */
	return 0;
}

static void xr68c681_config_port(struct uart_port *port, int flags)
{
	port->type = PORT_XR68C681;
	port->fifosize = 3; // TODO is this right? does it matter?

	// 	/* Clear mask, so no surprise interrupts. */
	// 	INT_CTRL &= ~(FTDI_IEN | FTDI_RXIE | FTDI_TXIE);
}

static const struct uart_ops xr68c681_uart_ops = {
	.tx_empty = xr68c681_tx_empty,
	.get_mctrl = xr68c681_get_mctrl,
	.set_mctrl = xr68c681_set_mctrl,
	.start_tx = xr68c681_start_tx,
	.stop_tx = xr68c681_stop_tx,
	.stop_rx = xr68c681_stop_rx,
	.enable_ms = xr68c681_enable_ms,
	.break_ctl = xr68c681_break_ctl,
	.startup = xr68c681_startup,
	.shutdown = xr68c681_shutdown,
	.set_termios = xr68c681_set_termios,
	.type = xr68c681_type,
	.release_port = xr68c681_release_port,
	.request_port = xr68c681_request_port,
	.config_port = xr68c681_config_port,
	.verify_port = xr68c681_verify_port,
};

#ifdef CONFIG_SERIAL_XR68C681_CONSOLE

static int xr68c681_console_setup(struct console *co, char *options)
{
	struct uart_port *port;
	int baud = 9600;
	int bits = 8;
	int parity = 'n';
	int flow = 'n';

	if ((co->index < 0) || (co->index >= XR68C681_MAXPORTS))
		co->index = 0;
	port = &xr68c681_uart_ports[co->index].port;
	if (port->membase == 0)
		return -ENODEV;

	if (options)
		uart_parse_options(options, &baud, &parity, &bits, &flow);

	return uart_set_options(port, co, baud, parity, bits, flow);
}

static void xr68c681_console_write(struct console *co, const char *s, unsigned int count)
{
	struct uart_port *port = &xr68c681_uart_ports[co->index].port;
	unsigned long flags;

	spin_lock_irqsave(&port->lock, flags);
	uart_console_write(port, s, count, xr68c681_uart_putchar);
	spin_unlock_irqrestore(&port->lock, flags);
}

static struct console xr68c681_console = {
	.name = XR68C681_SERIAL_DEV_NAME,
	.write = xr68c681_console_write,
	.device = uart_console_device,
	.setup = xr68c681_console_setup,
	.flags = CON_PRINTBUFFER,
	.index = -1,
	.data = &xr68c681_driver,
};

#endif

static int xr68c681_probe(struct platform_device *pdev)
{

	// struct alce_platform_uart *platp = pdev->dev.platform_data;
	// struct uart_port *port;
	// int i;

	// if (is_early_platform_device(pdev))
	// 	return xr68c681_probe_earlyprintk(pdev);

	// for (i = 0; ((i < XR68C681_MAXPORTS) && (platp[i].mapbase)); i++) {
	// 	port = &xr68c681_uart_ports[i].port;

	// 	port->line = i;
	// 	port->type = PORT_XR68C681;
	// 	port->mapbase = platp[i].mapbase;
	// 	port->membase = (platp[i].membase) ? platp[i].membase :
	// 		(unsigned char __iomem *) platp[i].mapbase;
	// 	port->iotype = SERIAL_IO_MEM;
	// 	port->irq = platp[i].irq;
	// 	port->uartclk = 12000000;
	// 	port->ops = &xr68c681_uart_ops;
	// 	port->flags = ASYNC_BOOT_AUTOCONF;

	// 	uart_add_one_port(&xr68c681_driver, port);
	// }

	struct uart_port *port = &xr68c681_uart_ports[pdev->id].port;

	printk(KERN_INFO "XR68C681 probe()\n");

	port->line = pdev->id;
	port->ops = &xr68c681_uart_ops;
	port->flags = ASYNC_BOOT_AUTOCONF; // TODO: UPF_?
	port->mapbase = DUART1_BASE;	   // TODO fix
	port->membase = DUART1_BASE;	   // TODO fix
	port->uartclk = 1843200;
	port->irq = 1; // TODO fix

	uart_add_one_port(&xr68c681_driver, port);
	// TODO: platform_set_drvdata?

	return 0;
}

static int xr68c681_remove(struct platform_device *pdev)
{
	/*struct uart_port *port;
	int i;
	for (i = 0; (i < XR68C681_MAXPORTS); i++) {
		port = &xr68c681_uart_ports[i].port;
		if (port)
			uart_remove_one_port(&xr68c681_driver, port);
	}
	*/
	return 0;
}

static struct platform_driver xr68c681_platform_driver = {
	.probe = xr68c681_probe,
	.remove = xr68c681_remove,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init xr68c681_serial_init(void)
{
	int ret;

	printk(KERN_INFO "XR68C681 serial driver (c) 2024 Colin Maykish\n");

	ret = uart_register_driver(&xr68c681_driver);
	if (ret)
	{
		printk(KERN_ERR "Failed to register uart driver\n");
		return ret;
	}
	else
	{
		printk("Registered UART driver\n");
	}

	ret = platform_driver_register(&xr68c681_platform_driver);
	if (ret != 0)
	{
		printk(KERN_ERR "Failed to register platform serial driver\n");
		uart_unregister_driver(&xr68c681_driver);
	}
	else
	{
		printk("Registered platform serial driver\n");
	}

	return 0;
}

static void __exit xr68c681_serial_exit(void)
{
	platform_driver_unregister(&xr68c681_platform_driver);
	uart_unregister_driver(&xr68c681_driver);
}

module_init(xr68c681_serial_init);
module_exit(xr68c681_serial_exit);

MODULE_AUTHOR("Colin Maykish");
MODULE_DESCRIPTION("XR68C681 serial driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:" DRIVER_NAME);
