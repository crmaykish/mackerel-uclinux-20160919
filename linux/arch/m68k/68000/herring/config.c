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
    local_irq_disable();
}

static cycle_t mackerel_read_clk(struct clocksource *cs)
{
	unsigned long flags;
	u32 cycles;

	local_irq_save(flags);
	cycles = mackerel_tick_count + 100;	// TODO: This is definitely not the right value
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
    setup_irq(1, &mackerel_timer_irq);

    // Setup DUART timer as 50 Hz interrupt
    MEM(DUART_IVR) = 0x40;       // Interrupt base register
    MEM(DUART_ACR) = 0xF0;       // Set timer mode X/16
    MEM(DUART_IMR) = 0b00001000; // Unmask counter interrupt
    MEM(DUART_CUR) = 0x09;       // Counter upper byte, (3.6864MHz / 2 / 16 / 0x900) = 50 Hz
    MEM(DUART_CLR) = 0x00;       // Counter lower byte
    MEM(DUART_OPR);              // Start counter

    clocksource_register_hz(&mackerel_clk, 10 * 100); // TODO: this should be calculated properly from the interrupt rate and CPU speed and all that

    timer_interrupt = handler;
}

void __init config_BSP(char *command, int len)
{
    printk(KERN_INFO "\Herring-8 68k support by Colin Maykish <crmaykish@gmail.com>\n");

    mach_reset = mackerel_reset;
    mach_sched_init = mackerel_sched_init;
}
