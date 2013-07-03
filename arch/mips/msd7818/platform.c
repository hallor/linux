/*
 * Copyright (C) 2006,2007 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2006,2007 Eugene Konev <ejka@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <asm/bootinfo.h>
#include <asm/reboot.h>
#include <linux/io.h> // To satisfy dependencies of below - TODO: kernel bug?
#include <asm/mips_machine.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <video/msd7818fb.h>
#include "irq_msd7818.h"
// Inspired by various pieces of code

// TODO: probably we need to fill this
void __init plat_time_init(void)
{
}

const char *get_system_type(void)
{
    return "MStar MSD7818";
}

void __init msd7818_clocks_init(void);

static void msd7818_shutdown(void) {
    void __iomem * base = ioremap(0x1F006000, 0x20);
    __u32 count = 12000000; // WDT timer has 12MHZ (??)
    iowrite32(count & 0xFFFF, base + 0x10); // WDT_PERIOD_L
    iowrite32((count>>16) & 0xFFFF, base + 0x14); // WDT_PERIOD_H
}

static void msd7818_restart(char *command)
{
    msd7818_shutdown();
}

void __init plat_mem_setup(void)
{
    // Cut hole for framebuffer
    add_memory_region(0, 0x2100000, BOOT_MEM_RAM);
    add_memory_region(0x2100000 + 1920 * 1080 * 4, 64*1024*1024 - 1920 * 1080 * 4 - 0x2100000, BOOT_MEM_RAM);
    // TODO: this has to be called probably somewhere else
    msd7818_clocks_init();
    // TODO: there is probably better place for that + replace it with watchdog driver..
    _machine_restart = msd7818_restart;
    _machine_halt = msd7818_shutdown;
    // restart watchdog to some enormous value (it's started by bootloader)
    {
        void __iomem * base = ioremap(0x1F006000, 0x20);
        __u32 count = 0xFFFFFFFF; // WDT timer has 12MHZ (??)
        iowrite32(count & 0xFFFF, base + 0x10); // WDT_PERIOD_L
        iowrite32((count>>16) & 0xFFFF, base + 0x14); // WDT_PERIOD_H
        iounmap(base);
    }
}

void __init prom_init(void) {}

void __init prom_free_prom_memory(void) {}

// This is _PHYSICAL_ address (after ioremap it becomes 0x8F201300 - the same probably applies to all addresses used by LG)
// See -> http://www.johnloomis.org/microchip/pic32/memory/memory.html
#define UART_BASE 0x1F201300
static struct plat_serial8250_port uart_data[] = {
    {
        .irq = E_IRQ_UART,
        .regshift = 3,
        .flags		= (UPF_SKIP_TEST | UPF_FIXED_TYPE | UPF_FIXED_PORT | UPF_IOREMAP),
        .uartclk = 115200 * 16 * 66,
        .type = PORT_16550A,
        .iotype = UPIO_MEM32,
        .mapbase = UART_BASE,
    },
    {}
};

static struct platform_device uart_device = {
    .name   = "serial8250",
    .id     = PLAT8250_DEV_PLATFORM,
    .dev.platform_data = uart_data,
};

static struct resource ir_resources[] = {
    [0] = {
        .start = 0x1F007B00,
        .end = 0x1F007B20,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = E_FIQ_IR,
        .end = E_FIQ_IR,
        .flags = IORESOURCE_IRQ,
    },
};

static struct platform_device ir_device = {
    .name = "msd7818-ir",
    .num_resources = ARRAY_SIZE(ir_resources),
    .resource = ir_resources,
};

static struct resource watchdog_resources[] = {
    [0] = {
        .start = 0x1F006000,
        .end = 0x1F006020, // TODO: find registers
        .flags = IORESOURCE_MEM,
    },
};

static struct platform_device watchdog_device = {
    .name = "msd7818-watchdog",
    .num_resources = ARRAY_SIZE(watchdog_resources),
    .resource = watchdog_resources,
};


static struct plat_msd7818_fb  framebuffer_data =
{
    .iobase = 0x2100000,
    .size = 1920 * 1080 * 4,
    .width = 1920,
    .height = 1080,
    .depth = 16,
    .mode = MODE_ARGB4444,
    .duplicate_paints = 1,
};

static struct platform_device framebuffer_device = {
    .name = "msd7818-fb",
    .dev.platform_data = &framebuffer_data,
};

struct platform_device *devices[] __initdata = {
    &uart_device,
    &ir_device,
    &watchdog_device,
    &framebuffer_device,
};

static int __init msd7818_register_devices(void)
{
    platform_add_devices(devices, ARRAY_SIZE(devices));
    mips_machine_setup();
    return 0;
}
arch_initcall(msd7818_register_devices);
