#ifndef _ASM_ARM_MODULE_H
#define _ASM_ARM_MODULE_H

#include <asm-generic/module.h>

#ifdef CONFIG_ARM_PLT_MODULE_LOAD

#define Elf_Shdr	Elf32_Shdr
#define Elf_Sym		Elf32_Sym
#define Elf_Ehdr	Elf32_Ehdr

/*
 * ARM can only do 24 bit relative calls. In the mainstream kernel this is
 * handled by mapping module memory to the 16 MB below the kernel. However,
 * in the no mmu case, this is not possible. So, to fall back, we create
 * a Procedure Linkage Table (PLT) and attach it to the module during
 * the in-kernel linking stage.
 * This code is adapted from the powerpc and v850 architectures,
 * and from that supplied by Rusty Russel
 */
struct arm_plt_entry {
	/* Jump Instruction */
	unsigned long jump[2];
};
#endif /* CONFIG_ARM_PLT_MODULE_LOAD */

struct unwind_table;

#ifdef CONFIG_ARM_UNWIND
enum {
	ARM_SEC_INIT,
	ARM_SEC_DEVINIT,
	ARM_SEC_CORE,
	ARM_SEC_EXIT,
	ARM_SEC_DEVEXIT,
	ARM_SEC_HOT,
	ARM_SEC_UNLIKELY,
	ARM_SEC_MAX,
};
#endif

struct mod_arch_specific {
#ifdef CONFIG_ARM_UNWIND
	struct unwind_table *unwind[ARM_SEC_MAX];
#endif
#ifdef CONFIG_ARM_PLT_MODULE_LOAD
	/* Index of the ELF header in the module in which to store the PLT */
	unsigned int plt_section;
#endif
#ifdef CONFIG_ARM_MODULE_PLTS
	struct elf32_shdr   *core_plt;
	struct elf32_shdr   *init_plt;
	int		    core_plt_count;
	int		    init_plt_count;
#endif
};

u32 get_module_plt(struct module *mod, unsigned long loc, Elf32_Addr val);

/*
 * Add the ARM architecture version to the version magic string
 */
#define MODULE_ARCH_VERMAGIC_ARMVSN "ARMv" __stringify(__LINUX_ARM_ARCH__) " "

/* Add __virt_to_phys patching state as well */
#ifdef CONFIG_ARM_PATCH_PHYS_VIRT
#define MODULE_ARCH_VERMAGIC_P2V "p2v8 "
#else
#define MODULE_ARCH_VERMAGIC_P2V ""
#endif

/* Add instruction set architecture tag to distinguish ARM/Thumb kernels */
#ifdef CONFIG_THUMB2_KERNEL
#define MODULE_ARCH_VERMAGIC_ARMTHUMB "thumb2 "
#else
#define MODULE_ARCH_VERMAGIC_ARMTHUMB ""
#endif

#define MODULE_ARCH_VERMAGIC \
	MODULE_ARCH_VERMAGIC_ARMVSN \
	MODULE_ARCH_VERMAGIC_ARMTHUMB \
	MODULE_ARCH_VERMAGIC_P2V

#ifdef CONFIG_ARM_PLT_MODULE_LOAD
#ifdef MODULE
/*
 * Force the creation of a .plt section. This will contain the PLT constructed
 * during dynamic link time
 */
asm(".section .plt; .align 3; .previous");
#endif /* MODULE */
#endif /* CONFIG_ARM_PLT_MODULE_LOAD */

#endif /* _ASM_ARM_MODULE_H */
