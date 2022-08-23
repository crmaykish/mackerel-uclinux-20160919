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
#include <asm/herring.h>
#include <asm/irq.h>

static u32 mackerel_tick_count;
static irq_handler_t timer_interrupt;

static irqreturn_t hw_tick(int irq, void *dummy)
{
	mackerel_tick_count += 10;
	return timer_interrupt(irq, dummy);
}

static struct irqaction mackerel_timer_irq = {
	.name	 = "timer",
	.flags	 = /*IRQF_DISABLED | */IRQF_TIMER,
	.handler = hw_tick,
};

void mackerel_reset(void)
{
    printk("mackerel_reset()\r\n");

    local_irq_disable();
}

static cycle_t mackerel_read_clk(struct clocksource *cs)
{
	unsigned long flags;
	u32 cycles;

	local_irq_save(flags);
	cycles = mackerel_tick_count + MEM(MFP_TBDR);	// TODO: This is definitely not the right value
	local_irq_restore(flags);

	return cycles;
}

static struct clocksource mackerel_clk = {
	.name	= "timer",
	.rating	= 250,
	.read	= mackerel_read_clk,
	.mask	= CLOCKSOURCE_MASK(32),
	.flags	= CLOCK_SOURCE_IS_CONTINUOUS,
};

void mackerel_sched_init(irq_handler_t handler)
{
    printk("mackerel_sched_init()\r\n");

    setup_irq(1, &mackerel_timer_irq);

    // // Set MFP Timer B to run at 36 Hz and trigger an interrupt on every tick
    MEM(MFP_TBDR) = 0;         // Timer B counter max (i.e 255);
    MEM(MFP_TBCR) = 0b0010111; // Timer B enabled, delay mode, /200 prescalar
    MEM(MFP_VR) = 0x40;        // Set base vector for MFP interrupt handlers
    MEM(MFP_IERA) = 0x01;      // Enable interrupts for Timer B
    MEM(MFP_IMRA) = 0x01;      // Unmask interrupt for Timer B

    clocksource_register_hz(&mackerel_clk, 10 * 100); // TODO: this should be calculated properly from the interrupt rate and CPU speed and all that

    timer_interrupt = handler;
}

void __init config_BSP(char *command, int len)
{
    printk(KERN_INFO "\Herring-8 68k support by Colin Maykish <crmaykish@gmail.com>\n");

    mach_reset = mackerel_reset;
    mach_sched_init = mackerel_sched_init;
}
