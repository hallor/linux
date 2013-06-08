#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>

#include "ir.h"
#include "irq_msd7818.h"

static void __iomem * base = NULL;

static __u32 reg_read(int offs) {
    return ioread32(base + (offs << 2) );
}

static void reg_write(__u32 value, int offs) {
    iowrite32(value, base + (offs << 2));
}

static irqreturn_t ir_interrupt(int irq, void *dev_id)
{
    printk(KERN_ERR "IR IRQ\n");
    reg_write(reg_read(REG_IR_CTRL) & ~IR_EN, REG_IR_CTRL);
    return IRQ_HANDLED;
}

void ir_do_init(void)
{
    int ret;
    printk(KERN_ERR "Registering IR driver...\n");
    base = ioremap(REG_IR_BASE, 0x18);
    if (!base) {
        printk(KERN_ERR "Failed to ioremap...\n");
        return;
    }

    ret = request_irq(E_FIQ_IR, ir_interrupt, 0, "IR", NULL);
    if (ret)
        printk(KERN_ERR "Failed to register irq handler.\n");

    printk(KERN_ERR "Configuring...\n");
    reg_write(IR_TIMEOUT_CHK_EN |
                           IR_INV            |
                           IR_RPCODE_EN      |
                           IR_LG01H_CHK_EN   |
                           IR_DCODE_PCHK_EN  |
                           IR_CCODE_CHK_EN   |
                           IR_LDCCHK_EN      |
                           IR_EN, REG_IR_CTRL);

    printk(KERN_ERR "Finised.\n");
}

static int ir_probe(struct platform_device *pdev) {
    printk(KERN_ERR "%s():%d\n", __FUNCTION__, __LINE__);
    return 0;
}

static int ir_remove(struct platform_device *pdev) {
    return 0;
}

static struct platform_driver ir_driver = {
    .probe = ir_probe,
    .remove = ir_remove,
    .driver = {
        .name = "msd7818-ir",
        .owner = THIS_MODULE,
    },
};

static int __init ir_init(void) {
    return platform_driver_register(&ir_driver);
}
late_initcall(ir_init);

static void __exit ir_exit(void) {
    platform_driver_unregister(&ir_driver);
}
