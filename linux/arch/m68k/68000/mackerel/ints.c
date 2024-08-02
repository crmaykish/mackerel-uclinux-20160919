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
    int new_vec = vec;

    // Determine the type of interrupt and how to handle it
    if (vec == 0x41) {
        // DUART interrupt source
        unsigned char misr = MEM(DUART1_MISR);

        if (misr & DUART_INTR_RXRDY) {
            // serial Rx interrupt
            new_vec = 0x41;
        }
        else if (misr & DUART_INTR_COUNTER) {
            // timer interrupt
            new_vec = 0x42;
        }
    }

    do_IRQ(new_vec - 0x40, fp);
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

	/* set up the vectors */
	for (i = 72; i < 256; ++i)
		_ramvec[i] = (e_vector) bad_interrupt;

	_ramvec[32] = system_call;

	_ramvec[65] = (e_vector) inthandler1;
	_ramvec[66] = (e_vector) inthandler2;
	_ramvec[67] = (e_vector) inthandler3;
	_ramvec[68] = (e_vector) inthandler4;
	_ramvec[69] = (e_vector) inthandler5;
	_ramvec[70] = (e_vector) inthandler6;
	_ramvec[71] = (e_vector) inthandler7;
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
