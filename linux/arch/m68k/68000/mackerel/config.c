#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clocksource.h>
#include <asm/traps.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/siginfo.h>
#include <linux/kallsyms.h>
#include <linux/signal.h>
#include <linux/ptrace.h>
#include <linux/console.h>
#include <linux/platform_device.h>
#include <asm/mackerel.h>
#include <asm/irq.h>

static irq_handler_t timer_interrupt;

static struct resource uart_res[] = {
	{
		.start = DUART1_BASE,
		.end = DUART1_BASE + 31,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = 1,
		.end = 1,
		.flags = IORESOURCE_IRQ,
	}};

static struct platform_device xr68c681_duart_device = {
	.name = "xr68c681-serial",
	.id = 0,
	.num_resources = ARRAY_SIZE(uart_res),
	.resource = uart_res,
};

static void mackerel_console_write(struct console *co, const char *str, unsigned int count)
{
	unsigned i = 0;

	while (str[i] != 0 && i < count)
	{
		duart_putc(str[i]);
		i++;
	}
}

static struct console mackerel_console_driver = {
	.name = "mackconsole",
	.flags = CON_PRINTBUFFER | CON_BOOT,
	.index = -1,
	.write = mackerel_console_write};

static irqreturn_t hw_tick(int irq, void *dummy)
{
	MEM(DUART1_OPR_RESET); // Stop counter, i.e. reset the timer

	return timer_interrupt(irq, dummy);
}

static struct irqaction mackerel_timer_irq = {
	.name = "timer",
	.flags = IRQF_TIMER,
	.handler = hw_tick,
};

void mackerel_reset(void)
{
	local_irq_disable();
}

void __init mackerel_init_IRQ(void)
{
}

void mackerel_sched_init(irq_handler_t handler)
{
	printk(KERN_INFO "Setting up Mackerel timer hardware\n");

	setup_irq(IRQ_NUM_TIMER, &mackerel_timer_irq);

	// Setup DUART as 50 Hz interrupt timer
	MEM(DUART1_IVR) = 0x40 + IRQ_NUM_DUART; // Interrupt base register
	MEM(DUART1_ACR) = 0xF0;					// Set timer mode X/16
	MEM(DUART1_IMR) = 0b00001000;			// Unmask counter interrupt
	MEM(DUART1_CUR) = 0x09;					// Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
	MEM(DUART1_CLR) = 0x00;					// Counter lower byte
	MEM(DUART1_OPR);						// Start counter

	timer_interrupt = handler;
}

void __init config_BSP(char *command, int len)
{
#ifdef CONFIG_MACKEREL_08
	printk(KERN_INFO "Mackerel-08 support by Colin Maykish <crmaykish@protonmail.com>\n");
#elif CONFIG_MACKEREL_10
	printk(KERN_INFO "Mackerel-10 support by Colin Maykish <crmaykish@protonmail.com>\n");
#endif

	// Disable all DUART interrupts
	MEM(DUART1_IMR) = 0;

	mach_reset = mackerel_reset;
	mach_sched_init = mackerel_sched_init;

	register_console(&mackerel_console_driver);
}

int __init mackerel_platform_init(void)
{
	if (platform_device_register(&xr68c681_duart_device))
	{
		panic("Could not register DUART device");
	}

	return 0;
}

arch_initcall(mackerel_platform_init);
