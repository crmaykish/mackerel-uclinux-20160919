#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/traps.h>
#include <asm/machdep.h>
#include <asm/mackerel.h>

/* assembler routines */
asmlinkage void system_call(void);
asmlinkage void buserr(void);
asmlinkage void trap(void);
asmlinkage void trap3(void);
asmlinkage void trap4(void);
asmlinkage void trap5(void);
asmlinkage void trap6(void);
asmlinkage void trap7(void);
asmlinkage void trap8(void);
asmlinkage void trap9(void);
asmlinkage void trap10(void);
asmlinkage void trap11(void);
asmlinkage void trap12(void);
asmlinkage void trap13(void);
asmlinkage void trap14(void);
asmlinkage void trap15(void);
asmlinkage void trap33(void);
asmlinkage void trap34(void);
asmlinkage void trap35(void);
asmlinkage void trap36(void);
asmlinkage void trap37(void);
asmlinkage void trap38(void);
asmlinkage void trap39(void);
asmlinkage void trap40(void);
asmlinkage void trap41(void);
asmlinkage void trap42(void);
asmlinkage void trap43(void);
asmlinkage void trap44(void);
asmlinkage void trap45(void);
asmlinkage void trap46(void);
asmlinkage void trap47(void);
asmlinkage irqreturn_t bad_interrupt(int, void *);
asmlinkage irqreturn_t inthandler(void);
asmlinkage irqreturn_t inthandler1(void);
asmlinkage irqreturn_t inthandler2(void);
asmlinkage irqreturn_t inthandler3(void);
asmlinkage irqreturn_t inthandler4(void);
asmlinkage irqreturn_t inthandler5(void);
asmlinkage irqreturn_t inthandler6(void);
asmlinkage irqreturn_t inthandler7(void);

void process_int(int vec, struct pt_regs *fp)
{
    int irq_num = 0;

    // Determine the type of interrupt and how to handle it
    if (vec >= 65 && vec < 72)
    {
#ifdef CONFIG_MACKEREL_08
        // Mackerel-08 has both the timer and serial Rx interrupts coming from the DUART on the same vector (65)
        // Check which source triggered the interrupt and determine the IRQ number to act on
        if (vec == 65)
        {
            unsigned char misr = MEM(DUART1_MISR);

            if (misr & DUART_INTR_RXRDY)
            {
                // serial Rx interrupt
                irq_num = 1;
            }
            else if (misr & DUART_INTR_COUNTER)
            {
                // timer interrupt
                irq_num = 4;
            }
        }
        else
        {
            irq_num = vec - 64;
        }
#elif CONFIG_MACKEREL_10
        // Mackerel-10 does not have multiple interrupt sources from the DUART
        // A simple translation from vector to IRQ number is sufficient
        irq_num = vec - 64;
#endif
    }
    else
    {
        printk("Unknown interrupt: 0x%02X\n", vec);
    }

    if (irq_num == IRQ_NUM_IDE) {
        // Clear the IDE interrupt by reading the status.
        // TODO: Why doesn't the IDE driver do this automatically?
        u8 t = MEM(MACKEREL_IDE_STATUS);
    }

    do_IRQ(irq_num, fp);
}

static void intc_irq_unmask(struct irq_data *d)
{
    // TODO
}

static void intc_irq_mask(struct irq_data *d)
{
    // TODO
}

static struct irq_chip intc_irq_chip = {
    .name = "MACKEREL-INTC",
    .irq_mask = intc_irq_mask,
    .irq_unmask = intc_irq_unmask,
};

void __init trap_init(void)
{
    int i;

    // NOTE: The autovectors and user interrupts share the same interrupt numbers
    // i.e. autovector 1 and user interrupt 1 will map to the same handler
    // Make sure each interrupt number is used by only one hardware peripheral

    // Autovectors
    _ramvec[25] = (e_vector)inthandler1;
    _ramvec[26] = (e_vector)inthandler2;
    _ramvec[27] = (e_vector)inthandler3;
    _ramvec[28] = (e_vector)inthandler4;
    _ramvec[29] = (e_vector)inthandler5;
    _ramvec[30] = (e_vector)inthandler6;
    _ramvec[31] = (e_vector)inthandler7;

    _ramvec[32] = system_call;

    // Vectored user interrupts
    _ramvec[65] = (e_vector)inthandler1;
    _ramvec[66] = (e_vector)inthandler2;
    _ramvec[67] = (e_vector)inthandler3;
    _ramvec[68] = (e_vector)inthandler4;
    _ramvec[69] = (e_vector)inthandler5;
    _ramvec[70] = (e_vector)inthandler6;
    _ramvec[71] = (e_vector)inthandler7;

    for (i = 72; i < 256; ++i)
        _ramvec[i] = (e_vector)bad_interrupt;
}

void __init init_IRQ(void)
{
    int i;

    for (i = 0; (i < NR_IRQS); i++)
    {
        irq_set_chip(i, &intc_irq_chip);
        irq_set_handler(i, handle_level_irq);
    }
}
