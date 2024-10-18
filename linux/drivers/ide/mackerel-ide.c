#include <linux/module.h>
#include <linux/types.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <asm/mackerel.h>

static int __init mackerel_ide_init(void)
{
    printk(KERN_INFO "Mackerel IDE driver (c) 2024 Colin Maykish\n");

    return 0;
}

module_init(mackerel_ide_init);

MODULE_DESCRIPTION("Mackerel IDE Driver");
MODULE_AUTHOR("Colin Maykish");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
