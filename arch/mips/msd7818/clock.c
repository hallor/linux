/*
 * Copyright (C) 2007 Felix Fietkau <nbd@openwrt.org>
 * Copyright (C) 2007 Eugene Konev <ejka@openwrt.org>
 * Copyright (C) 2009 Florian Fainelli <florian@openwrt.org>
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <asm/time.h>

struct clk {
	unsigned long rate;
};

static struct clk msd7818_cpu_clk;

void __init msd7818_clocks_init(void)
{
    msd7818_cpu_clk.rate = 552000000;
    // TODO: this is probably configurable somewhere
    mips_hpt_frequency = msd7818_cpu_clk.rate / 2; // HPT frequency set by u-boot is ~ 1/2 cpu clock
}

/*
 * Linux clock API
 */
int clk_enable(struct clk *clk)
{
	return 0;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
    return clk->rate;
}
EXPORT_SYMBOL(clk_get_rate);

struct clk *clk_get(struct device *dev, const char *id)
{
    if (!strncmp(id, "cpu", 4))
        return &msd7818_cpu_clk;
    
	return ERR_PTR(-ENOENT);
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
}
EXPORT_SYMBOL(clk_put);


// Needed for clock operation (returns proper irq number for systick interrupt)
unsigned int __cpuinit get_c0_compare_int(void)
{
    return CP0_LEGACY_COMPARE_IRQ;
}
