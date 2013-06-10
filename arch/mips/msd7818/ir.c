// WARNING: copied all registers from LG / MStar  kernel (gpl'd), still needs refactor
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "ir.h"
#include "irq_msd7818.h"

// Map <ir> -> keypress
static const int keycodes[0xFF] =
{
    [0x41] = KEY_POWER,
    [0x43] = KEY_SWITCHVIDEOMODE, // TVRADIO
    [0x03] = KEY_LIST,
    [0x40] = KEY_MUTE,
    [0x45] = KEY_VOLUMEUP,
    [0x49] = KEY_VOLUMEDOWN,
    [0x06] = KEY_AUDIO,
    [0x44] = KEY_SUBTITLE,
    [0x0a] = KEY_TEXT, // TTX
    [0x48] = KEY_FAVORITES,
    [0x4d] = KEY_RED,
    [0x0e] = KEY_GREEN,
    [0x0f] = KEY_YELLOW,
    [0x4c] = KEY_BLUE,
    [0x55] = KEY_MENU,
    [0x54] = KEY_EXIT,
    [0x16] = KEY_UP,
    [0x51] = KEY_LEFT,
    [0x13] = KEY_ENTER,
    [0x50] = KEY_RIGHT,
    [0x59] = KEY_EPG,
    [0x1a] = KEY_DOWN,
    [0x58] = KEY_INFO,
    [0x5a] = KEY_RECORD,
    [0x19] = KEY_STOP,
    [0x18] = KEY_PLAYPAUSE,
    [0x52] = KEY_REWIND,
    [0x11] = KEY_PVR,
    [0x10] = KEY_FASTFORWARD,
    [0x4e] = KEY_1,
    [0x0d] = KEY_2,
    [0x0c] = KEY_3,
    [0x4a] = KEY_4,
    [0x09] = KEY_5,
    [0x08] = KEY_6,
    [0x46] = KEY_7,
    [0x05] = KEY_8,
    [0x04] = KEY_9,
    [0x42] = KEY_PREVIOUS,
    [0x01] = KEY_0,
    [0x00] = KEY_NEXT,
};


// TODO: Tune this configuration, make it somehow configurable and stuff
#define XTAL_CLOCK_FREQ             12000000
#define IR_CKDIV_NUM                        ((XTAL_CLOCK_FREQ+500000)/1000000)
#define IR_CLK                              (XTAL_CLOCK_FREQ/1000000)

#define irGetMinCnt(time, tolerance)        ((u32)(((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1))*((double)1-tolerance)))
#define irGetMaxCnt(time, tolerance)        ((u32)(((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1))*((double)1+tolerance)))

#define irGetCnt(time)                      ((u32)((double)time*((double)IR_CLK)/(IR_CKDIV_NUM+1)))

// IR Timing define - NEC CODE
#define IR_HEADER_CODE_TIME     9000 // us
#define IR_OFF_CODE_TIME        4500    // us
#define IR_OFF_CODE_RP_TIME     2500    // us
#define IR_LOGI_01H_TIME        560     // us
#define IR_LOGI_0_TIME          1125    // us
#define IR_LOGI_1_TIME          2250    // us
#define IR_TIMEOUT_CYC          140000  // us

#define IR_RP_TIMEOUT           irGetCnt(IR_TIMEOUT_CYC)
#define IR_HDC_UPB              irGetMaxCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_HDC_LOB              irGetMinCnt(IR_HEADER_CODE_TIME, 0.2)
#define IR_OFC_UPB              irGetMaxCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_LOB              irGetMinCnt(IR_OFF_CODE_TIME, 0.2)
#define IR_OFC_RP_UPB           irGetMaxCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_OFC_RP_LOB           irGetMinCnt(IR_OFF_CODE_RP_TIME, 0.2)
#define IR_LG01H_UPB            irGetMaxCnt(IR_LOGI_01H_TIME, 0.3)
#define IR_LG01H_LOB            irGetMinCnt(IR_LOGI_01H_TIME, 0.3)
#define IR_LG0_UPB              irGetMaxCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG0_LOB              irGetMinCnt(IR_LOGI_0_TIME, 0.2)
#define IR_LG1_UPB              irGetMaxCnt(IR_LOGI_1_TIME, 0.2)
#define IR_LG1_LOB              irGetMinCnt(IR_LOGI_1_TIME, 0.2)

// TODO: add multi-standard support later, for now keep only one remote supported just to have it "working"
struct msd7818_remote {
    struct input_dev *idev;
    struct platform_device *pdev;
    void __iomem * base;
    int irq;
};

#define IR_READ(dev, ofs) ioread32(dev->base + ofs * 4)
#define IR_WRITE(dev, ofs, val) iowrite32(val, dev->base + ofs * 4)

static irqreturn_t ir_interrupt(int irq, void *dev)
{
    struct msd7818_remote * r = (struct msd7818_remote *) dev;

    BUG_ON(!r);

    if( (IR_READ(r, REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY) == 0)
    {
        int i;
        __u32 code = 0;
        for(i=0; i<8; i++)
        {

            if(IR_READ(r, REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_FIFO_EMPTY)
                break;

            code = code << 8;
            code |= (IR_READ(r, REG_IR_CKDIV_NUM_KEY_DATA) >> 8) & 0xFF;
            IR_WRITE(r, REG_IR_FIFO_RD_PULSE, 1); // read confirm
            udelay(1); // TODO: perhaps this can be smaller
        }
        if (code >> 16 == 0x00FF) { // Proper remote
            int rep = IR_READ(r, REG_IR_SHOT_CNT_H_FIFO_STATUS) & IR_RPT_FLAG;
            code = (code >> 8) & 0xFF;
            if (!rep && keycodes[code]!=KEY_RESERVED) { //TODO: For now just pass single events - todo - handling of autorepeat
                input_event(r->idev, EV_KEY, keycodes[code], 1);
                input_sync(r->idev);

                input_event(r->idev, EV_KEY, keycodes[code], 0);
                input_sync(r->idev);
            }
        }
    }
    return IRQ_HANDLED;
}

// Fills all registers and stuff
static void ir_init_hw(struct msd7818_remote * r) {
    IR_WRITE(r, REG_IR_CTRL,
                IR_TIMEOUT_CHK_EN |
                           IR_INV            |
                           IR_RPCODE_EN      |
                           IR_LG01H_CHK_EN   |
                           IR_DCODE_PCHK_EN  |
                           IR_CCODE_CHK_EN   |
                           IR_LDCCHK_EN      |
                           IR_EN);

    // header code upper bound
    IR_WRITE(r, REG_IR_HDC_UPB, IR_HDC_UPB);

    // header code lower bound
    IR_WRITE(r, REG_IR_HDC_LOB, IR_HDC_LOB);

    // off code upper bound
    IR_WRITE(r, REG_IR_OFC_UPB, IR_OFC_UPB);

    // off code lower bound
    IR_WRITE(r, REG_IR_OFC_LOB, IR_OFC_LOB);

    // off code repeat upper bound
    IR_WRITE(r, REG_IR_OFC_RP_UPB, IR_OFC_RP_UPB);

    // off code repeat lower bound
    IR_WRITE(r, REG_IR_OFC_RP_LOB, IR_OFC_RP_LOB);

    // logical 0/1 high upper bound
    IR_WRITE(r, REG_IR_LG01H_UPB, IR_LG01H_UPB);

    // logical 0/1 high lower bound
    IR_WRITE(r, REG_IR_LG01H_LOB, IR_LG01H_LOB);

    // logical 0 upper bound
    IR_WRITE(r, REG_IR_LG0_UPB, IR_LG0_UPB);

    // logical 0 lower bound
    IR_WRITE(r, REG_IR_LG0_LOB, IR_LG0_LOB);

    // logical 1 upper bound
    IR_WRITE(r, REG_IR_LG1_UPB, IR_LG1_UPB);

    // logical 1 lower bound
    IR_WRITE(r, REG_IR_LG1_LOB, IR_LG1_LOB);

    // timeout cycles
    IR_WRITE(r, REG_IR_TIMEOUT_CYC_L, IR_RP_TIMEOUT & 0xFFFF);
    IR_WRITE(r, REG_IR_TIMEOUT_CYC_H_CODE_BYTE, 0x1f00 | 0x30 | ((IR_RP_TIMEOUT >> 16) & 0x0F) );

    IR_WRITE(r, REG_IR_CKDIV_NUM_KEY_DATA, IR_CKDIV_NUM);   // clock divider
    IR_WRITE(r, REG_IR_CCODE, ((u16)0xFF<<8) | 0x00);

    // Raw decode - data
    IR_WRITE(r, REG_IR_SEPR_BIT_FIFO_CTRL, 0xF);
    IR_WRITE(r, REG_IR_GLHRM_NUM, 0x2804);
}

static int ir_probe(struct platform_device *pdev) {
    struct resource * mem = NULL, *irq = NULL;
    struct msd7818_remote * r = NULL;
    int ret = 0;

    dev_dbg(&pdev->dev, "%s():%d\n", __FUNCTION__, __LINE__);

    mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (IS_ERR(mem)) {
        dev_err(&pdev->dev, "Missing IOMEM mapping.\n");
        return -ENODEV;
    }
    irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (IS_ERR(irq)) {
        dev_err(&pdev->dev, "Missing IRQ mapping.\n");
        return -ENODEV;
    }

    r = kmalloc(sizeof(struct msd7818_remote), GFP_KERNEL);
    if (IS_ERR(r)) {
        dev_err(&pdev->dev, "Out of memory.\n");
        return -ENOMEM;
    }

    r->irq = irq->start;
    r->base = ioremap(mem->start, resource_size(mem));
    if (IS_ERR(r->base)) {
        dev_err(&pdev->dev, "ioremap failed.\n");
        ret = PTR_ERR(r->base);
        goto err_malloc;
    }

    r->idev = input_allocate_device();
    if (IS_ERR(r->idev)) {
        dev_err(&pdev->dev, "Failed to allocate input device.\n");
        ret = -ENOMEM;
        goto err_unmap;
    }

    input_set_drvdata(r->idev, r);

    r->idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REP);
    { // setup buttons
        int i = 0;
        for (i=0; i<ARRAY_SIZE(keycodes); ++i) {
            if (keycodes[i]!= KEY_RESERVED)
                set_bit(keycodes[i], r->idev->keybit);
        }
    }
    ret = input_register_device(r->idev);
    if (ret) {
        dev_err(&pdev->dev, "Failed to register input device.\n");
        goto err_input;
    }

    r->pdev = pdev;
    platform_set_drvdata(pdev, r);
    // Attach do interrupt handler
    ret = request_irq(r->irq, ir_interrupt, 0, "msd7818-ir", r);
    if (ret) {
        dev_err(&pdev->dev, "Failed to attach to interrupt handler. \n");
        goto err_unreg;
    }

    // Configure registers
    ir_init_hw(r);

    dev_notice(&pdev->dev, "Registered IR Remote at 0x%08x, IRQ %d.", mem->start, irq->start);
    return 0;

err_unreg:
    input_unregister_device(r->idev);
err_input:
    input_free_device(r->idev);
err_unmap:
    iounmap(r->base);
err_malloc:
    kfree(r);
    return ret;
}

static int ir_remove(struct platform_device *pdev) {
    struct msd7818_remote * r = platform_get_drvdata(pdev);
    BUG_ON(IS_ERR(r));

    // Disable interrupts
    IR_WRITE(r, REG_IR_CTRL, IR_READ(r, REG_IR_CTRL) & ~IR_EN);

    // Release handlers
    free_irq(r->irq, r);
    input_unregister_device(r->idev);
    input_free_device(r->idev);
    iounmap(r->base);
    kfree(r);

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
