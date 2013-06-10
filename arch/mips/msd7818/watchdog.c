/* Watchdog driver for MSD7818, bit fake - to keep kernel running for now.
 *
 * TODO: add "normal" watchdog interface
 */

#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/err.h>

struct watchdog_device {
    struct platform_device * pdev;
    void __iomem * regs;
    struct timer_list timer;
};

static void watchdog_timer(unsigned long data)
{
    struct watchdog_device * wd = (struct watchdog_device*) data;
    BUG_ON(!wd);
//    dev_dbg(&wd->pdev->dev, "tick\n");

    // Update registers
    {
        __u32 count = 6 * 12000000; // WDT has 12MHZ (??) - 6sec timeout
        iowrite32(count & 0xFFFF, wd->regs + 0x10); // WDT_PERIOD_L
        iowrite32((count>>16) & 0xFFFF, wd->regs + 0x14); // WDT_PERIOD_H
    }
    // Restart timer
    mod_timer(&wd->timer, jiffies + msecs_to_jiffies(5000));
}


static int watchdog_probe(struct platform_device *pdev) {
    struct resource * res = NULL;
    int ret = 0;
    struct watchdog_device * wd = NULL;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No IOMEM resources supplied - unable to probe.\n");
        return -EINVAL;
    }

    wd = kzalloc(sizeof(struct watchdog_device), GFP_KERNEL);
    if (!wd) {
        dev_err(&pdev->dev, "Failed to allocate memory.\n");
        return -ENOMEM;
    }

    wd->pdev = pdev;

    wd->regs = devm_request_and_ioremap(&pdev->dev, res);
    if (IS_ERR(wd->regs)) {
        dev_err(&pdev->dev, "Failed to request resources.\n");
        ret = PTR_ERR(wd->regs);
        goto free;
    }

    init_timer(&wd->timer);
    wd->timer.function = watchdog_timer;
    wd->timer.expires = jiffies + msecs_to_jiffies(1); // Start now
    wd->timer.data = (int) wd;
    add_timer(&wd->timer);

    platform_set_drvdata(pdev, wd);
    dev_notice(&pdev->dev, "Registered msd7818 watchdog driver at %08x\n", res->start);

    return 0;

free:
    kfree(wd);
    return ret;
}

static int watchdog_remove(struct platform_device *pdev) {
    struct watchdog_device * wd = platform_get_drvdata(pdev);

    BUG_ON(!wd); // TODO: something more sane ;)

    del_timer_sync(&wd->timer);
    iounmap(wd->regs);
    kfree(wd);

    dev_warn(&pdev->dev, "Succesfully removed msd7818 watchdog. System may reset any time...\n");

    return 0;
}

static struct platform_driver watchdog_driver = {
    .probe = watchdog_probe,
    .remove = watchdog_remove,
    .driver = {
        .name = "msd7818-watchdog",
        .owner = THIS_MODULE,
    },
};

static int __init watchdog_init(void) {
    return platform_driver_register(&watchdog_driver);
}
late_initcall(watchdog_init);

static void __exit watchdog_exit(void) {
    platform_driver_unregister(&watchdog_driver);
}
