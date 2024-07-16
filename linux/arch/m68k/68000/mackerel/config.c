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

static u32 mackerel_tick_count;
static irq_handler_t timer_interrupt;

static struct resource uart_res[] = {
	{
		.start = DUART1_BASE,
		.end = DUART1_BASE + 31,
		.flags = IORESOURCE_MEM,
	},
	{
		//         .start          = IRQ_USER + 12,
		//         .end            = IRQ_USER + 12,
		// TODO ?
		.start = 1,
		.end = 1,
		.flags = IORESOURCE_IRQ,
	}};

/*
 * UART device
 */
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
	.flags = CON_PRINTBUFFER, // | CON_BOOT,
	.index = -1,
	.write = mackerel_console_write};

static irqreturn_t hw_tick(int irq, void *dummy)
{
	MEM(DUART2_OPR_RESET); // Stop counter, i.e. reset the timer

	mackerel_tick_count += 10;
	return timer_interrupt(irq, dummy);
}

static struct irqaction mackerel_timer_irq = {
	.name = "timer",
	.flags = /*IRQF_DISABLED | */ IRQF_TIMER,
	.handler = hw_tick,
};

void mackerel_reset(void)
{
	local_irq_disable();
}

static cycle_t mackerel_read_clk(struct clocksource *cs)
{
	unsigned long flags;
	u32 cycles;

	local_irq_save(flags);
	cycles = mackerel_tick_count + 100; // TODO: This is definitely not the right value
	local_irq_restore(flags);

	return cycles;
}

static struct clocksource mackerel_clk = {
	.name = "timer",
	.rating = 250,
	.read = mackerel_read_clk,
	.mask = CLOCKSOURCE_MASK(32),
	.flags = CLOCK_SOURCE_IS_CONTINUOUS,
};

void mackerel_sched_init(irq_handler_t handler)
{
	int timer_int_vec = 2;

	setup_irq(timer_int_vec, &mackerel_timer_irq);

	// Setup DUART 2 as 50 Hz interrupt timer
	MEM(DUART2_IVR) = 0x40 + timer_int_vec; // Interrupt base register
	MEM(DUART2_ACR) = 0xF0;					// Set timer mode X/16
	MEM(DUART2_IMR) = 0b00001000;			// Unmask counter interrupt
	MEM(DUART2_CUR) = 0x09;					// Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
	MEM(DUART2_CLR) = 0x00;					// Counter lower byte
	MEM(DUART2_OPR);						// Start counter

	clocksource_register_hz(&mackerel_clk, 10 * 100); // TODO: this should be calculated properly from the interrupt rate and CPU speed and all that

	timer_interrupt = handler;
}

void __init config_BSP(char *command, int len)
{
	printk(KERN_INFO "Mackerel 68k support by Colin Maykish <crmaykish@gmail.com>\n");

	// Disable all DUART interrupts
	MEM(DUART1_IMR) = 0;
	MEM(DUART2_IMR) = 0;

	mach_reset = mackerel_reset;
	mach_sched_init = mackerel_sched_init;

	register_console(&mackerel_console_driver);
}

int __init mackerel_platform_init(void)
{

	printk(KERN_INFO "Registering DUART device\n");

	if (platform_device_register(&xr68c681_duart_device))
	{
		printk(KERN_ERR "Failed to register DUART\n");
		panic("Could not register DUART device");
	}
	else
	{
		printk(KERN_INFO "Registered DUART\n");
	}

	return 0;
}

arch_initcall(mackerel_platform_init);
