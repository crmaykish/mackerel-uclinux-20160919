#include <linux/module.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/printk.h>
#include <asm/mackerel.h>

static u8 mackerel_ide_read_status(ide_hwif_t *hwif)
{
    return MEM(MACKEREL_IDE_STATUS);
}

static void mackerel_ide_exec_command(ide_hwif_t *hwif, u8 cmd)
{
    MEM(MACKEREL_IDE_COMMAND) = cmd;
}

static u8 mackerel_ide_read_altstatus(ide_hwif_t *hwif)
{
    return MEM(MACKEREL_IDE_ALT_STATUS);
}

static void mackerel_ide_write_devctl(ide_hwif_t *hwif, u8 ctl)
{
    MEM(MACKEREL_IDE_ALT_STATUS) = ctl;
}

static void mackerel_ide_dev_select(ide_drive_t *drive)
{
    u8 select = drive->select | ATA_DEVICE_OBS;
    MEM(MACKEREL_IDE_DRIVE_SEL) = select;
}

static void mackerel_ide_tf_load(ide_drive_t *drive, struct ide_taskfile *tf, u8 valid)
{
    if (valid & IDE_VALID_FEATURE)
        MEM(MACKEREL_IDE_FEATURE) = tf->feature;
    if (valid & IDE_VALID_NSECT)
        MEM(MACKEREL_IDE_SECTOR_COUNT) = tf->nsect;
    if (valid & IDE_VALID_LBAL)
        MEM(MACKEREL_IDE_SECTOR_START) = tf->lbal;
    if (valid & IDE_VALID_LBAM)
        MEM(MACKEREL_IDE_LBA_MID) = tf->lbam;
    if (valid & IDE_VALID_LBAH)
        MEM(MACKEREL_IDE_LBA_HIGH) = tf->lbah;
    if (valid & IDE_VALID_DEVICE)
        MEM(MACKEREL_IDE_DRIVE_SEL) = tf->device;
}

static void mackerel_ide_tf_read(ide_drive_t *drive, struct ide_taskfile *tf, u8 valid)
{
    if (valid & IDE_VALID_ERROR)
        tf->error = MEM(MACKEREL_IDE_FEATURE);
    if (valid & IDE_VALID_NSECT)
        tf->nsect = MEM(MACKEREL_IDE_SECTOR_COUNT);
    if (valid & IDE_VALID_LBAL)
        tf->lbal = MEM(MACKEREL_IDE_SECTOR_START);
    if (valid & IDE_VALID_LBAM)
        tf->lbam = MEM(MACKEREL_IDE_LBA_MID);
    if (valid & IDE_VALID_LBAH)
        tf->lbah = MEM(MACKEREL_IDE_LBA_HIGH);
    if (valid & IDE_VALID_DEVICE)
        tf->device = MEM(MACKEREL_IDE_DRIVE_SEL);
}

static void mackerel_ide_input_data(ide_drive_t *drive, struct ide_cmd *cmd, void *buf, unsigned int len)
{
    int i;
    int count = (len + 1) / 2;
    u16 *ptr = (u16 *)buf;

    for (i = 0; i < count; i++)
    {
        ptr[i] = MEM16(MACKEREL_IDE_DATA);
    }
}

static void mackerel_ide_output_data(ide_drive_t *drive, struct ide_cmd *cmd, void *buf, unsigned int len)
{
    int i;
    int count = (len + 1) / 2;
    u16 *ptr = (u16 *)buf;

    for (i = 0; i < count; i++)
    {
        MEM16(MACKEREL_IDE_DATA) = ptr[i];
    }
}

static const struct ide_tp_ops mackerel_ide_tp_ops = {
    .read_status = mackerel_ide_read_status,
    .exec_command = mackerel_ide_exec_command,
    .read_altstatus = mackerel_ide_read_altstatus,
    .write_devctl = mackerel_ide_write_devctl,
    .dev_select = mackerel_ide_dev_select,
    .tf_load = mackerel_ide_tf_load,
    .tf_read = mackerel_ide_tf_read,
    .input_data = mackerel_ide_input_data,
    .output_data = mackerel_ide_output_data,
};

static const struct ide_port_info mackerel_ide_port_info = {
    .tp_ops = &mackerel_ide_tp_ops,
    .host_flags = IDE_HFLAG_NO_DMA,
    .chipset = ide_generic,
};

static int __init mackerel_ide_init(void)
{
    struct ide_host *host;
    struct ide_hw hw, *hws[] = {&hw};
    int rc;

    printk(KERN_INFO "Mackerel IDE driver (c) 2024 Colin Maykish\n");

    memset(&hw, 0, sizeof(hw));

    hw.irq = IRQ_NUM_IDE;
    hw.io_ports.data_addr = MACKEREL_IDE_BASE;

    host = ide_host_alloc(&mackerel_ide_port_info, hws, 1);
    if (host == NULL)
    {
        rc = -ENOMEM;
        return rc;
    }

    rc = ide_host_register(host, &mackerel_ide_port_info, hws);

    if (rc)
    {
        ide_host_free(host);
        return rc;
    }

    return 0;
}

module_init(mackerel_ide_init);

MODULE_DESCRIPTION("Mackerel IDE Driver");
MODULE_AUTHOR("Colin Maykish");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
