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
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mtd/physmap.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <asm/irq_cpu.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <asm/bootinfo.h>
#include <asm/addrspace.h>
#include <asm/mips_machine.h>
#include <uapi/linux/serial_reg.h>

#define UART_REG(addr) *((volatile unsigned int*)(0xBF201300 + ((addr)<< 3)))

uint32_t UART16550_READ(uint8_t addr)
{
    uint32_t data;

    if (addr>80) //ERROR: Not supported
    {
        return(0);
    }

    data = UART_REG(addr);
    return(data);
}

void UART16550_WRITE(uint8_t addr, uint8_t data)
{
    if (addr>80) //ERROR: Not supported
    {
        return;
    }
    UART_REG(addr) = data;
}

#define UART_TX		0	/* Out: Transmit buffer */
#define UART_LSR	5	/* In:  Line Status Register */
#define UART_LSR_THRE		0x20 /* Transmit-hold-register empty */

void prom_putchar(unsigned char c)
{
    if (c == '\r')
        prom_putchar('&');
    while ((UART16550_READ(UART_LSR) & UART_LSR_THRE) == 0)
        ;
    UART16550_WRITE(UART_TX, c);
}


const char *get_system_type(void)
{
    return "MStar MSD7818";
}

void __init prom_init(void)
{
    int data;
    printk("%s()\n", __FUNCTION__);
    UART16550_WRITE(UART_LCR, serial_in(UART_LCR) | UART_LCR_DLAB);
    data = UART16550_READ(UART_DLL) | serial_in(UART_DLM) << 8;
    UART16550_WRITE(UART_LCR, serial_in(UART_LCR) & ~UART_LCR_DLAB);
    printk("Serial divisor: %08x\n", data);
}

void __init msd7818_clocks_init(void);

void __init plat_mem_setup(void)
{
    printk("%s() begin\n", __FUNCTION__);        
    add_memory_region(0, 64*1024*1024, BOOT_MEM_RAM);
    msd7818_clocks_init();
    printk("%s() end\n", __FUNCTION__);        
}

void __init prom_free_prom_memory(void)
{
    printk("%s()\n", __FUNCTION__);    
    
}

 struct resource msd7818_uart_resources[] = {
	{
		.start	= 0xBF201300,
		.end	= 0xBF201300 + 7,
		.flags	= IORESOURCE_MEM,
	},
};

static struct plat_serial8250_port msd7818_uart_data[] = {
{
    .irq = 8,
    .regshift = 3,
    .flags		= (UPF_SKIP_TEST |\
         UPF_FIXED_TYPE | UPF_IOREMAP ),\
    .uartclk = 115200*16 * 42,
    .type = PORT_16550,
    .iotype = UPIO_MEM32,
    .mapbase = 0xBF201300,
    .membase = 0,
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
    printk("%s() begin\n", __FUNCTION__);
    msd7818_uart_data[0].membase = ioremap(0xBF201300, 256);
    platform_device_register(&uart_device);
    mips_machine_setup();    
    printk("%s() end\n", __FUNCTION__);
    return 0;
}
arch_initcall(msd7818_register_devices);
