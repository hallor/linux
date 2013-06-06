#include <linux/io.h>
#include <linux/interrupt.h>

#include "ir.h"
#include "irq_msd7818.h"

static void __iomem * base = NULL;

__u32 reg_read(int offs) {
    return ioread32(base + (offs << 2) );
}

void reg_write(__u32 value, int offs) {
    iowrite32(value, base + (offs << 2));
}

static irqreturn_t ir_interrupt(int irq, void *dev_id)
{
    printk(KERN_ERR "IR IRQ\n");
    reg_write(reg_read(REG_IR_CTRL) & ~IR_EN, REG_IR_CTRL);
    return IRQ_HANDLED;
}

void ir_init(void)
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
