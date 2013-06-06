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

// Heavily C-P from LG Titania3 kernel code.
// TODO: we have (probably) some generic irq controller - use it somehow in future

#include <asm/irq_cpu.h>
#include <linux/io.h> // To satisfy dependencies of below - TODO: kernel bug?
#include <asm/mipsregs.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <uapi/linux/serial_reg.h>

#include "irq_msd7818.h"

// TODO: replace with ioread/write, update base address to 0x1F203200
#define REG(addr) (*(volatile unsigned int *)(addr))

// INTERRUPTS

static void enable_msd7818_irq(struct irq_data *d)
{
    int irq = d->irq;

    if (irq == E_IRQ_FIQ_ALL)
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
    else if ( (irq >= E_IRQL_START) && (irq <= E_IRQL_END) )
    {
            REG(REG_IRQ_MASK_L) &= ~(0x1 << (irq-E_IRQL_START) );
    }
    else if ( (irq >= E_IRQH_START) && (irq <= E_IRQH_END) )
    {
            REG(REG_IRQ_MASK_H) &= ~(0x1 << (irq-E_IRQH_START) );
    }
    else if ( (irq >= E_FIQL_START) && (irq <= E_FIQL_END) )
    {
            REG(REG_FIQ_MASK_L) &= ~(0x1 << (irq-E_FIQL_START) );
    }
    else if ( (irq >= E_FIQH_START) && (irq <= E_FIQH_END) )
    {
            REG(REG_FIQ_MASK_H) &= ~(0x1 << (irq-E_FIQH_START) );
    }
    else if ( (irq >= E_IRQL_EXP_START) && (irq <= E_IRQL_EXP_END) )
    {
            REG(REG_IRQ_EXP_MASK_L) &= ~(0x1 << (irq-E_IRQL_EXP_START) );
    }
    else if ( (irq >= E_IRQH_EXP_START) && (irq <= E_IRQH_EXP_END) )
    {
            REG(REG_IRQ_EXP_MASK_H) &= ~(0x1 << (irq-E_IRQH_EXP_START) );
    }
    else if ( (irq >= E_FIQL_EXP_START) && (irq <= E_FIQL_EXP_END) )
    {
            REG(REG_FIQ_EXP_MASK_L) &= ~(0x1 << (irq-E_FIQL_EXP_START) );
    }
    else if ( (irq >= E_FIQH_EXP_START) && (irq <= E_FIQH_EXP_END) )
    {
            REG(REG_FIQ_EXP_MASK_H) &= ~(0x1 << (irq-E_FIQH_EXP_START) );
    }
}

static void disable_msd7818_irq(struct irq_data *d)
{
    int irq = d->irq;
    if (irq == E_IRQ_FIQ_ALL)
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
    else if ( (irq >= E_IRQL_START) && (irq <= E_IRQL_END) )
    {
        REG(REG_IRQ_MASK_L) |= (0x1 << (irq-E_IRQL_START) );
    }
    else if ( (irq >= E_IRQH_START) && (irq <= E_IRQH_END) )
    {
        REG(REG_IRQ_MASK_H) |= (0x1 << (irq-E_IRQH_START) );
    }
    else if ( (irq >= E_FIQL_START) && (irq <= E_FIQL_END) )
    {
        REG(REG_FIQ_MASK_L) |= (0x1 << (irq-E_FIQL_START) );
        REG(REG_FIQ_CLEAR_L) = (0x1 << (irq-E_FIQL_START) );
    }
    else if ( (irq >= E_FIQH_START) && (irq <= E_FIQH_END) )
    {
        REG(REG_FIQ_MASK_H) |= (0x1 << (irq-E_FIQH_START) );
        REG(REG_FIQ_CLEAR_H) = (0x1 << (irq-E_FIQH_START) );
    }
    else if ( (irq >= E_IRQL_EXP_START) && (irq <= E_IRQL_EXP_END) )
    {
        REG(REG_IRQ_EXP_MASK_L) |= (0x1 << (irq-E_IRQL_EXP_START) );
    }
    else if ( (irq >= E_IRQH_EXP_START) && (irq <= E_IRQH_EXP_END) )
    {
        REG(REG_IRQ_EXP_MASK_H) |= (0x1 << (irq-E_IRQH_EXP_START) );
    }
    else if ( (irq >= E_FIQL_EXP_START) && (irq <= E_FIQL_EXP_END) )
    {
        REG(REG_FIQ_EXP_MASK_L) |= (0x1 << (irq-E_FIQL_EXP_START) );
        REG(REG_FIQ_EXP_CLEAR_L) = (0x1 << (irq-E_FIQL_EXP_START) );
    }
    else if ( (irq >= E_FIQH_EXP_START) && (irq <= E_FIQH_EXP_END) )
    {
        REG(REG_FIQ_EXP_MASK_H) |= (0x1 << (irq-E_FIQH_EXP_START) );
        REG(REG_FIQ_EXP_CLEAR_H) = (0x1 << (irq-E_FIQH_EXP_START) );
    }
}

static struct irq_chip msd7818_irq_type = {
    .name = "msd7818-irq",
    .irq_mask = disable_msd7818_irq,
    .irq_unmask = enable_msd7818_irq,
};

void __init arch_init_irq(void)
{
    int i=0;
    struct irq_data d;

    mips_cpu_irq_init();

    for (i = MSD7818_INT_BASE; i <= (MSD7818_INT_BASE + MSD7818_INT_END); ++i)
    {
        irq_set_chip_and_handler(i, &msd7818_irq_type, handle_level_irq);
    }

    irq_set_chip_and_handler(0, &msd7818_irq_type, handle_level_irq);
    irq_set_chip_and_handler(1, &msd7818_irq_type, handle_level_irq);
    d.irq = E_IRQ_FIQ_ALL;
    disable_msd7818_irq(&d);

    // Enable irq lines at CPU core (no interrupts should be generated by peripherals)
    set_c0_status(IE_IRQ0 | IE_IRQ1);
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

// TODO: refactor this crap
static inline unsigned int irq_ffs(unsigned int pending)
{
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
    return -clz(pending) + 31 - CAUSEB_IP;
#else
#error foo
#endif
}

// Dispatch interrupts from controller
static void hw0_dispatch(void)
{
    {
        __u16 cause_low,cause_high;

        cause_low = (__u16)REG(REG_IRQ_PENDING_L);
        cause_high = (__u16)REG(REG_IRQ_PENDING_H);

        if ( cause_low & IRQL_UART )
        {
            do_IRQ((unsigned int)E_IRQ_UART);
        }

        if ( cause_low & IRQL_MVD )
        {
            do_IRQ((unsigned int)E_IRQ_MVD);
        }

        if ( cause_low & IRQL_UHC )
        {
           do_IRQ((unsigned int)E_IRQ_UHC);
        }
        if ( cause_low & IRQL_DEB )
        {
            do_IRQ((unsigned int)E_IRQ_DEB);
        }

        if(cause_low & IRQL_EMAC)
        {
            do_IRQ((unsigned int)E_IRQ_EMAC);
        }

        if ( cause_low & IRQL_COMB )
        {
            do_IRQ((unsigned int)E_IRQ_COMB);
        }

        if( cause_high & IRQH_VBI )
        {
            do_IRQ((unsigned int)E_IRQ_VBI);
        }

        if ( cause_high & IRQH_TSP )
        {
            do_IRQ((unsigned int)E_IRQ_TSP);

        }

        if ( cause_high & IRQH_HDMITX )
        {
            do_IRQ((unsigned int)E_IRQ_HDMITX);
        }
        if ( cause_high & IRQH_GOP )
        {
            do_IRQ((unsigned int)E_IRQ_GOP);
        }
        if ( cause_high & IRQH_PCM2MCU )
        {
            do_IRQ((unsigned int)E_IRQ_PCM2MCU);
        }
        if ( cause_high & IRQH_RTC )
        {
            do_IRQ((unsigned int)E_IRQ_RTC);
        }

        //2008/10/23 Nick DDC2BI interrupt
        if ( cause_high & IRQH_D2B )
        {
            do_IRQ((unsigned int)E_IRQ_D2B);
        }

        cause_low = (unsigned short)REG(REG_IRQ_EXP_PENDING_L);
        cause_high = (unsigned short)REG(REG_IRQ_EXP_PENDING_H);

        if ( cause_low & IRQ_SVD )
        {
            do_IRQ((unsigned int)E_IRQ_SVD);
        }
        if ( cause_low & IRQ_USB1 )
        {
            do_IRQ((unsigned int)E_IRQ_USB1);
        }
        if ( cause_low & IRQ_UHC1 )
        {
            do_IRQ((unsigned int)E_IRQ_UHC1);
        }
        if ( cause_low & IRQ_MIU )
        {
            do_IRQ((unsigned int)E_IRQ_MIU);
        }
        if ( cause_low & IRQ_DIP )
        {
            do_IRQ((unsigned int)E_IRQ_DIP);
        }
        if ( cause_low & IRQ_M4VE ) {
            do_IRQ((unsigned int)E_IRQ_M4VE);
        }

        if ( cause_high & IRQH_EXP_HDMI ) {
            do_IRQ((unsigned int)E_IRQH_EXP_HDMI);
        }
    }

}


asmlinkage void plat_irq_dispatch(void)
{
    unsigned int pending = read_c0_cause() & read_c0_status() & ST0_IM;
    int irq;

    irq = irq_ffs(pending);

    // TODO: probably hw0_dispatch should be split or sth
    if (irq == 2)
        hw0_dispatch();
    else if (irq == 3)
        hw0_dispatch();
    else if (irq >= 0)
        do_IRQ(MIPS_CPU_IRQ_BASE + irq);
    else
        spurious_interrupt();
}
