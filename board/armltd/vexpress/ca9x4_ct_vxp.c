/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * David Mueller, ELSOFT AG, <d.mueller@elsoft.ch>
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * (C) Copyright 2004
 * ARM Ltd.
 * Philippe Robin, <philippe.robin@arm.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <netdev.h>
#include <asm/io.h>
#include <asm/arch/systimer.h>
#include <asm/arch/sysctrl.h>
#include <asm/arch/wdt.h>

static ulong timestamp;
static ulong lastdec;

static struct wdt *wdt_base = (struct wdt *)WDT_BASE;
static struct systimer *systimer_base = (struct systimer *)SYSTIMER_BASE;
static struct sysctrl *sysctrl_base = (struct sysctrl *)SCTL_BASE;

static void flash__init(void);
static void vexpress_timer_init(void);
DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_SHOW_BOOT_PROGRESS)
void show_boot_progress(int progress)
{
    printf("Boot reached stage %d\n", progress);
}
#endif

static inline void delay(ulong loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b" : "=r" (loops) : "0" (loops));
}

int board_init(void)
{
	/* adress of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR + 0x00000100;
	gd->bd->bi_env = (struct environment_s *)(CONFIG_SYS_FLASH_BASE +
						  CONFIG_ENV_OFFSET);

	gd->bd->bi_arch_number = MACH_TYPE_VEXPRESS;
	gd->flags = 0;

	icache_enable();
	flash__init();
	vexpress_timer_init();

	return 0;
}

int board_eth_init(bd_t *bis)
{
	int rc = 0;
#ifdef CONFIG_SMC911X
	rc = smc911x_initialize(0, CONFIG_SMC911X_BASE);
#endif
	return rc;
}

int misc_init_r(void)
{
	setenv("verify", "n");
	return 0;
}

static void flash__init(void)
{
	/* Setup the sytem control register to allow writing to flash */
	uint tmp = *(uint *)(VEXPRESS_FLASHCTRL);
	tmp |= VEXPRESS_FLASHPROG_FLVPPEN;
	*(uint *)(VEXPRESS_FLASHCTRL) = tmp;
}

int dram_init(void)
{
	/* Populate memory size values */
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
	return 0;
}

int timer_init(void)
{
	return 0;
}

/*
 * Start timer:
 *    Setup a 32 bit timer, running at 1KHz
 *    Versatile Express Motherboard provides 1 MHz timer
 */
static void vexpress_timer_init(void)
{
	/*
	 * Set clock frequency in system controller:
	 *   VEXPRESS_REFCLK is 32KHz
	 *   VEXPRESS_TIMCLK is 1MHz
	 */
	writel(SP810_TIMER0_EnSel | SP810_TIMER1_EnSel |
	       SP810_TIMER2_EnSel | SP810_TIMER3_EnSel |
	       readl(&sysctrl_base->scctrl), &sysctrl_base->scctrl);

	/*
	 * Set Timer0 to be:
	 *   Enabled, free running, no interrupt, 32-bit, wrapping
	 */
	writel(SYSTIMER_RELOAD, &systimer_base->Timer0Load);
	writel(SYSTIMER_RELOAD, &systimer_base->Timer0Value);
	writel(SYSTIMER_EN | SYSTIMER_32BIT | \
	       readl(&systimer_base->Timer0Control), \
	       &systimer_base->Timer0Control);

	reset_timer_masked();
}

int interrupt_init(void)
{
	return 0;
}

/* Use the ARM Watchdog System to cause reset */
void reset_cpu(ulong addr)
{
	writeb(WDT_EN, &wdt_base->WdogControl);
	writel(WDT_RESET_LOAD, &wdt_base->WdogLoad);
}

/*
 * Delay x useconds AND perserve advance timstamp value
 *     assumes timer is ticking at 1 msec
 */
void udelay(ulong usec)
{
	ulong tmo, tmp;

	tmo = usec / 1000;
	tmp = get_timer(0);	/* get current timestamp */

	/*
	 * If setting this forward will roll time stamp	then
	 * reset "advancing" timestamp to 0 and set lastdec value
	 * otherwise set the advancing stamp to the wake up time
	 */
	if ((tmo + tmp + 1) < tmp)
		reset_timer_masked();
	else
		tmo += tmp;

	while (get_timer_masked() < tmo)
		; /* loop till wakeup event */
}

ulong get_timer(ulong base)
{
	return get_timer_masked() - base;
}

void reset_timer_masked(void)
{
	lastdec = readl(&systimer_base->Timer0Value) / 1000;
	timestamp = 0;
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer_masked(void)
{
	ulong now = readl(&systimer_base->Timer0Value) / 1000;

	if (lastdec >= now) {	/* normal mode (non roll) */
		timestamp += lastdec - now;
	} else {		/* count down timer overflowed */
		/*
		 * nts = ts + ld - now
		 * ts = old stamp, ld = time before passing through - 1
		 * now = amount of time after passing though - 1
		 * nts = new "advancing time stamp"
		 */
		timestamp += lastdec +  SYSTIMER_RELOAD - now;
	}
	lastdec = now;

	return timestamp;
}

void lowlevel_init(void)
{
}

ulong get_board_rev(void){
	ulong *rev_reg = (u32 *)SYS_ID;
	return *rev_reg;
}
