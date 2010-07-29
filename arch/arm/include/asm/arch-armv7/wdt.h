/*
 * (C) Copyright 2010
 * Matt Waddel, <matt.waddel@www.linaro.org>
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
#ifndef _WDT_H_
#define _WDT_H_

/* Watchdog timer (SP805) register base address */
#define WDT_BASE	0x100E5000

#define WDT_EN		0x2
#define WDT_RESET_LOAD	0x0

struct wdt {
	u32 WdogLoad;		/* 0x000 */
	u32 WdogValue;
	u32 WdogControl;
	u32 WdogIntClr;
	u32 WdogRIS;
	u32 WdogMIS;
	u32 res1[0x2F9];
	u32 WdogLock;		/* 0xC00 */
	u32 res2[0xBE];
	u32 WdogITCR;		/* 0xF00 */
	u32 WdogITOP;
	u32 res3[0x35];
	u32 WdogPeriphID0;	/* 0xFE0 */
	u32 WdogPeriphID1;
	u32 WdogPeriphID2;
	u32 WdogPeriphID3;
	u32 WdogPCellID0;
	u32 WdogPCellID1;
	u32 WdogPCellID2;
	u32 WdogPCellID3;
};

#endif /* _WDT_H_ */
