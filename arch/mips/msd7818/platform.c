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
#include <linux/io.h> // To satisfy dependencies of below - TODO: kernel bug?
#include <asm/mips_machine.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>

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

void __init plat_mem_setup(void)
{
    add_memory_region(0, 64*1024*1024, BOOT_MEM_RAM);
    // TODO: this has to be called probably somewhere else
    msd7818_clocks_init();
}

void __init prom_init(void)
{
}

void __init prom_free_prom_memory(void)
{
}

#define UART_REG(addr) *((volatile unsigned int*)(0xBF201300 + ((addr)<< 3)))
 static unsigned int uart_in(struct uart_port *p, int offset)
 {
     return UART_REG(offset);
 }

 static void uart_out(struct uart_port *p, int offset, int value)
 {
     UART_REG(offset) = value;
 }

static struct plat_serial8250_port msd7818_uart_data[] = {
{
//    .irq = 8,
    .regshift = 3,
    .flags		= (UPF_SKIP_TEST | UPF_FIXED_TYPE | UPF_FIXED_PORT),
    .uartclk = 115200*16 * 42,
    .type = PORT_8250,
    .iotype = UPIO_MEM32,
    .mapbase = 0xBF201300,
    .membase = 0,
    .serial_in	= uart_in,
    .serial_out	= uart_out,

},
    {}
};

 struct platform_device uart_device = {
    .name		= "serial8250",
    .id		= PLAT8250_DEV_PLATFORM,
    .dev = {
        .platform_data = msd7818_uart_data,
    },
};

static int __init msd7818_register_devices(void)
{
    msd7818_uart_data[0].membase = ioremap(0xBF201300, 256);
    platform_device_register(&uart_device);
    mips_machine_setup();    
    return 0;
}
arch_initcall(msd7818_register_devices);
