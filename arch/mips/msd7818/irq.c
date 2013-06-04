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

#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/clockchips.h>

#include <asm/time.h>
#include <asm/irq_cpu.h>
#include <asm/mipsregs.h>
//#include <asm/mach-ar7/ar7.h>
#include <uapi/linux/serial_reg.h>

// Copied from LG titania-3 board
#include "chip_int.h"

#define REG(addr) (*(volatile unsigned int *)(addr))
void prom_putchar(unsigned char c);

// INTERRUPTS

static void enable_msd7818_irq(struct irq_data *d)
{
    int eIntNum = d->irq;
    prom_putchar('!');
    printk(KERN_ERR "%s() %d\n", __FUNCTION__, eIntNum);
    if (eIntNum == E_IRQ_FIQ_ALL)
    {
            REG(REG_IRQ_MASK_L) &= ~IRQL_ALL;
            REG(REG_IRQ_MASK_H) &= ~IRQH_ALL;
            REG(REG_FIQ_MASK_L) &= ~FIQL_ALL;
            REG(REG_FIQ_MASK_H) &= ~FIQH_ALL;

            REG(REG_IRQ_EXP_MASK_L) &= ~IRQL_EXP_ALL;
            REG(REG_IRQ_EXP_MASK_H) &= ~IRQH_EXP_ALL;
            REG(REG_FIQ_EXP_MASK_L) &= ~FIQL_EXP_ALL;
            REG(REG_FIQ_EXP_MASK_H) &= ~FIQH_EXP_ALL;
    }
    else if ( (eIntNum >= E_IRQL_START) && (eIntNum <= E_IRQL_END) )
    {
            REG(REG_IRQ_MASK_L) &= ~(0x1 << (eIntNum-E_IRQL_START) );
    }
    else if ( (eIntNum >= E_IRQH_START) && (eIntNum <= E_IRQH_END) )
    {
            REG(REG_IRQ_MASK_H) &= ~(0x1 << (eIntNum-E_IRQH_START) );
    }
    else if ( (eIntNum >= E_FIQL_START) && (eIntNum <= E_FIQL_END) )
    {
            REG(REG_FIQ_MASK_L) &= ~(0x1 << (eIntNum-E_FIQL_START) );
    }
    else if ( (eIntNum >= E_FIQH_START) && (eIntNum <= E_FIQH_END) )
    {
            REG(REG_FIQ_MASK_H) &= ~(0x1 << (eIntNum-E_FIQH_START) );
    }
    else if ( (eIntNum >= E_IRQL_EXP_START) && (eIntNum <= E_IRQL_EXP_END) )
    {
            REG(REG_IRQ_EXP_MASK_L) &= ~(0x1 << (eIntNum-E_IRQL_EXP_START) );
    }
    else if ( (eIntNum >= E_IRQH_EXP_START) && (eIntNum <= E_IRQH_EXP_END) )
    {
            REG(REG_IRQ_EXP_MASK_H) &= ~(0x1 << (eIntNum-E_IRQH_EXP_START) );
    }
    else if ( (eIntNum >= E_FIQL_EXP_START) && (eIntNum <= E_FIQL_EXP_END) )
    {
            REG(REG_FIQ_EXP_MASK_L) &= ~(0x1 << (eIntNum-E_FIQL_EXP_START) );
    }
    else if ( (eIntNum >= E_FIQH_EXP_START) && (eIntNum <= E_FIQH_EXP_END) )
    {
            REG(REG_FIQ_EXP_MASK_H) &= ~(0x1 << (eIntNum-E_FIQH_EXP_START) );
    }
}

void disable_msd7818_irq(struct irq_data *d)
{
    int eIntNum = d->irq;
    prom_putchar('@');
    printk(KERN_ERR "%s() %d\n", __FUNCTION__, eIntNum);
    if (eIntNum == E_IRQ_FIQ_ALL)
    {
        REG(REG_IRQ_MASK_L) |= IRQL_ALL;
        REG(REG_IRQ_MASK_H) |= IRQH_ALL;
        REG(REG_FIQ_MASK_L) |= FIQL_ALL;
        REG(REG_FIQ_MASK_H) |= FIQH_ALL;

        REG(REG_IRQ_EXP_MASK_L) |= IRQL_EXP_ALL;
        REG(REG_IRQ_EXP_MASK_H) |= IRQH_EXP_ALL;
        REG(REG_FIQ_EXP_MASK_L) |= FIQL_EXP_ALL;
        REG(REG_FIQ_EXP_MASK_H) |= FIQH_EXP_ALL;
    }
    else if ( (eIntNum >= E_IRQL_START) && (eIntNum <= E_IRQL_END) )
    {
        REG(REG_IRQ_MASK_L) |= (0x1 << (eIntNum-E_IRQL_START) );
    }
    else if ( (eIntNum >= E_IRQH_START) && (eIntNum <= E_IRQH_END) )
    {
        REG(REG_IRQ_MASK_H) |= (0x1 << (eIntNum-E_IRQH_START) );
    }
    else if ( (eIntNum >= E_FIQL_START) && (eIntNum <= E_FIQL_END) )
    {
        REG(REG_FIQ_MASK_L) |= (0x1 << (eIntNum-E_FIQL_START) );
        //REG(REG_FIQ_CLEAR_L) |= (0x1 << (eIntNum-E_FIQL_START) );
        //REG(REG_FIQ_CLEAR_L) &= ~(0x1 << (eIntNum-E_FIQL_START) );
        REG(REG_FIQ_CLEAR_L) = (0x1 << (eIntNum-E_FIQL_START) );
        //REG(REG_FIQ_CLEAR_L) &= ~(0x1 << (eIntNum-E_FIQL_START) );

    }
    else if ( (eIntNum >= E_FIQH_START) && (eIntNum <= E_FIQH_END) )
    {
        REG(REG_FIQ_MASK_H) |= (0x1 << (eIntNum-E_FIQH_START) );
        //REG(REG_FIQ_CLEAR_H) |= (0x1 << (eIntNum-E_FIQH_START) );
        //REG(REG_FIQ_CLEAR_H) &= ~(0x1 << (eIntNum-E_FIQH_START) );
        REG(REG_FIQ_CLEAR_H) = (0x1 << (eIntNum-E_FIQH_START) );
    }
    else if ( (eIntNum >= E_IRQL_EXP_START) && (eIntNum <= E_IRQL_EXP_END) )
    {
        REG(REG_IRQ_EXP_MASK_L) |= (0x1 << (eIntNum-E_IRQL_EXP_START) );
    }
    else if ( (eIntNum >= E_IRQH_EXP_START) && (eIntNum <= E_IRQH_EXP_END) )
    {
        REG(REG_IRQ_EXP_MASK_H) |= (0x1 << (eIntNum-E_IRQH_EXP_START) );
    }
    else if ( (eIntNum >= E_FIQL_EXP_START) && (eIntNum <= E_FIQL_EXP_END) )
    {
        REG(REG_FIQ_EXP_MASK_L) |= (0x1 << (eIntNum-E_FIQL_EXP_START) );
        REG(REG_FIQ_EXP_CLEAR_L) = (0x1 << (eIntNum-E_FIQL_EXP_START) );
        //REG(REG_FIQ_EXP_CLEAR_L) |= (0x1 << (eIntNum-E_FIQL_EXP_START) );
        //REG(REG_FIQ_EXP_CLEAR_L) &= ~(0x1 << (eIntNum-E_FIQL_EXP_START) );
    }
    else if ( (eIntNum >= E_FIQH_EXP_START) && (eIntNum <= E_FIQH_EXP_END) )
    {
        REG(REG_FIQ_EXP_MASK_H) |= (0x1 << (eIntNum-E_FIQH_EXP_START) );
        REG(REG_FIQ_EXP_CLEAR_H) = (0x1 << (eIntNum-E_FIQH_EXP_START) );
        //REG(REG_FIQ_EXP_CLEAR_H) |= (0x1 << (eIntNum-E_FIQH_EXP_START) );
        //REG(REG_FIQ_EXP_CLEAR_H) &= ~(0x1 << (eIntNum-E_FIQH_EXP_START) );
    }
}

static struct irq_chip msd7818_irq_type = {
    .name = "msd7818-irq",
    .irq_mask = disable_msd7818_irq,
    .irq_unmask = enable_msd7818_irq,
};

static void __init irq_init(void)
{
    int i=0;
    struct irq_data d;
    printk("%s() begin\n", __FUNCTION__);
    mips_cpu_irq_init();

	for (i = MSTAR_INT_BASE; i <= (TITANIAINT_END+MSTAR_INT_BASE); i++) {
        irq_set_chip_and_handler(i, &msd7818_irq_type, handle_level_irq);
	}
    irq_set_chip_and_handler(0, &msd7818_irq_type, handle_level_irq);
    irq_set_chip_and_handler(1, &msd7818_irq_type, handle_level_irq);
    d.irq = E_IRQ_FIQ_ALL;
    disable_msd7818_irq(&d);
    set_c0_status(IE_IRQ0 | IE_IRQ1);

    printk(KERN_ERR "%s() e\n", __FUNCTION__);

}

uint32_t UART16550_READ(uint8_t addr);
void UART16550_WRITE(uint8_t addr, uint8_t data);

void __init arch_init_irq(void)
{
    int data;
    UART16550_WRITE(UART_LCR, UART16550_READ(UART_LCR) | UART_LCR_DLAB);
    data = UART16550_READ(UART_DLL) | UART16550_READ(UART_DLM) << 8;
    UART16550_WRITE(UART_LCR, UART16550_READ(UART_LCR) & ~UART_LCR_DLAB);
    printk("Serial divisor: %08x\n", data);

    printk(KERN_ERR "%s() b\n", __FUNCTION__);
	irq_init();
    printk(KERN_ERR "%s() e\n", __FUNCTION__);
}

static inline int clz(unsigned long x)
{
    __asm__(
    "    .set    push                    \n"
    "    .set    mips32                    \n"
    "    clz    %0, %1                    \n"
    "    .set    pop                    \n"
    : "=r" (x)
    : "r" (x));

    return x;
}

static inline unsigned int irq_ffs(unsigned int pending)
{
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
    return -clz(pending) + 31 - CAUSEB_IP;
#else
    unsigned int a0 = 7;
    unsigned int t0;

    t0 = s0 & 0xf000;
    t0 = t0 < 1;
    t0 = t0 << 2;
    a0 = a0 - t0;
    s0 = s0 << t0;

    t0 = s0 & 0xc000;
    t0 = t0 < 1;
    t0 = t0 << 1;
    a0 = a0 - t0;
    s0 = s0 << t0;

    t0 = s0 & 0x8000;
    t0 = t0 < 1;
    //t0 = t0 << 2;
    a0 = a0 - t0;
    //s0 = s0 << t0;

    return a0;
#endif
}

static unsigned int __cpuinit get_c0_compare_int(void)
{
	return CP0_LEGACY_COMPARE_IRQ;
}

static void hw0_dispatch(void)
{
    printk(KERN_ERR "@");
    {
        unsigned short u16Reglow,u16Reghigh;

        u16Reglow = (unsigned short)REG(REG_IRQ_PENDING_L);
        u16Reghigh = (unsigned short)REG(REG_IRQ_PENDING_H);

        if ( u16Reglow & IRQL_UART )
        {
            do_IRQ((unsigned int)E_IRQ_UART);
        }

        if ( u16Reglow & IRQL_MVD )
        {
            do_IRQ((unsigned int)E_IRQ_MVD);
        }

        if ( u16Reglow & IRQL_UHC )
        {
           do_IRQ((unsigned int)E_IRQ_UHC);
        }
        if ( u16Reglow & IRQL_DEB )
        {
            do_IRQ((unsigned int)E_IRQ_DEB);
        }

        if(u16Reglow & IRQL_EMAC)
        {
            do_IRQ((unsigned int)E_IRQ_EMAC);
        }

        if ( u16Reglow & IRQL_COMB )
        {
            do_IRQ((unsigned int)E_IRQ_COMB);
        }

        if( u16Reghigh & IRQH_VBI )
        {
            do_IRQ((unsigned int)E_IRQ_VBI);
        }

        if ( u16Reghigh & IRQH_TSP )
        {
            do_IRQ((unsigned int)E_IRQ_TSP);

        }

        if ( u16Reghigh & IRQH_HDMITX )
        {
            do_IRQ((unsigned int)E_IRQ_HDMITX);
        }
        if ( u16Reghigh & IRQH_GOP )
        {
            do_IRQ((unsigned int)E_IRQ_GOP);
        }
        if ( u16Reghigh & IRQH_PCM2MCU )
        {
            do_IRQ((unsigned int)E_IRQ_PCM2MCU);
        }
        if ( u16Reghigh & IRQH_RTC )
        {
            do_IRQ((unsigned int)E_IRQ_RTC);
        }

        //2008/10/23 Nick DDC2BI interrupt
        if ( u16Reghigh & IRQH_D2B )
        {
            do_IRQ((unsigned int)E_IRQ_D2B);
        }

        u16Reglow = (unsigned short)REG(REG_IRQ_EXP_PENDING_L);
        u16Reghigh = (unsigned short)REG(REG_IRQ_EXP_PENDING_H);

        //u16Reglow += MSTAR_INT_BASE;
        if ( u16Reglow & IRQ_SVD )
        {
            do_IRQ((unsigned int)E_IRQ_SVD);
        }
        if ( u16Reglow & IRQ_USB1 )
        {
            do_IRQ((unsigned int)E_IRQ_USB1);
        }
        if ( u16Reglow & IRQ_UHC1 )
        {
            do_IRQ((unsigned int)E_IRQ_UHC1);
        }
        if ( u16Reglow & IRQ_MIU )
        {
            do_IRQ((unsigned int)E_IRQ_MIU);
        }
        if ( u16Reglow & IRQ_DIP )
        {
            do_IRQ((unsigned int)E_IRQ_DIP);
        }
        if ( u16Reglow & IRQ_M4VE ) {
            do_IRQ((unsigned int)E_IRQ_M4VE);
        }

        //u16RegHigh += MSTAR_INT_BASE;
        if ( u16Reghigh & IRQH_EXP_HDMI ) {
            do_IRQ((unsigned int)E_IRQH_EXP_HDMI);
        }
    }

}

asmlinkage void plat_irq_dispatch(void)
{
    unsigned int pending = read_c0_cause() & read_c0_status() & ST0_IM;
    int irq;

    irq = irq_ffs(pending);
//    if (irq != 7)
//        prom_putchar('#');
//        printk("IRQ %i %08x\n", irq, pending);

    if (irq == 2)
        hw0_dispatch();
    else if (irq == 3)
        hw0_dispatch();
    if (irq >= 0)
        do_IRQ(MIPS_CPU_IRQ_BASE + irq);
    else
        spurious_interrupt();
}


void __init plat_time_init(void)
{
    printk(KERN_ERR "%s() b\n", __FUNCTION__);

    printk(KERN_ERR "%s() e\n", __FUNCTION__);
}
