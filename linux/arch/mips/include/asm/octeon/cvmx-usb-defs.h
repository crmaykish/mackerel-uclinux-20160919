/***********************license start***************
 * Author: Cavium Networks
 *
 * Contact: support@caviumnetworks.com
 * This file is part of the OCTEON SDK
 *
 * Copyright (c) 2003-2008 Cavium Networks
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, as
 * published by the Free Software Foundation.
 *
 * This file is distributed in the hope that it will be useful, but
 * AS-IS and WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, TITLE, or
 * NONINFRINGEMENT.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this file; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * or visit http://www.gnu.org/licenses/.
 *
 * This file may also be available under a different license from Cavium.
 * Contact Cavium Networks for more information
 ***********************license end**************************************/

#ifndef __CVMX_USB_DEFS_H__
#define __CVMX_USB_DEFS_H__

#ifndef __BYTE_ORDER
#define	__BYTE_ORDER	__BIG_ENDIAN
#endif

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_daint_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t outepint:16;
		uint32_t inepint:16;
#else
		uint32_t inepint:16;
		uint32_t outepint:16;
#endif
	} s;
	struct cvmx_usbcx_daint_s cn30xx;
	struct cvmx_usbcx_daint_s cn31xx;
	struct cvmx_usbcx_daint_s cn50xx;
	struct cvmx_usbcx_daint_s cn52xx;
	struct cvmx_usbcx_daint_s cn52xxp1;
	struct cvmx_usbcx_daint_s cn56xx;
	struct cvmx_usbcx_daint_s cn56xxp1;
} cvmx_usbcx_daint_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_daintmsk_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t outepmsk:16;
		uint32_t inepmsk:16;
#else
		uint32_t inepmsk:16;
		uint32_t outepmsk:16;
#endif
	} s;
	struct cvmx_usbcx_daintmsk_s cn30xx;
	struct cvmx_usbcx_daintmsk_s cn31xx;
	struct cvmx_usbcx_daintmsk_s cn50xx;
	struct cvmx_usbcx_daintmsk_s cn52xx;
	struct cvmx_usbcx_daintmsk_s cn52xxp1;
	struct cvmx_usbcx_daintmsk_s cn56xx;
	struct cvmx_usbcx_daintmsk_s cn56xxp1;
} cvmx_usbcx_daintmsk_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dcfg_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_23_31:9;
		uint32_t epmiscnt:5;
		uint32_t reserved_13_17:5;
		uint32_t perfrint:2;
		uint32_t devaddr:7;
		uint32_t reserved_3_3:1;
		uint32_t nzstsouthshk:1;
		uint32_t devspd:2;
#else
		uint32_t devspd:2;
		uint32_t nzstsouthshk:1;
		uint32_t reserved_3_3:1;
		uint32_t devaddr:7;
		uint32_t perfrint:2;
		uint32_t reserved_13_17:5;
		uint32_t epmiscnt:5;
		uint32_t reserved_23_31:9;
#endif
	} s;
	struct cvmx_usbcx_dcfg_s cn30xx;
	struct cvmx_usbcx_dcfg_s cn31xx;
	struct cvmx_usbcx_dcfg_s cn50xx;
	struct cvmx_usbcx_dcfg_s cn52xx;
	struct cvmx_usbcx_dcfg_s cn52xxp1;
	struct cvmx_usbcx_dcfg_s cn56xx;
	struct cvmx_usbcx_dcfg_s cn56xxp1;
} cvmx_usbcx_dcfg_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dctl_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_12_31:20;
		uint32_t pwronprgdone:1;
		uint32_t cgoutnak:1;
		uint32_t sgoutnak:1;
		uint32_t cgnpinnak:1;
		uint32_t sgnpinnak:1;
		uint32_t tstctl:3;
		uint32_t goutnaksts:1;
		uint32_t gnpinnaksts:1;
		uint32_t sftdiscon:1;
		uint32_t rmtwkupsig:1;
#else
		uint32_t rmtwkupsig:1;
		uint32_t sftdiscon:1;
		uint32_t gnpinnaksts:1;
		uint32_t goutnaksts:1;
		uint32_t tstctl:3;
		uint32_t sgnpinnak:1;
		uint32_t cgnpinnak:1;
		uint32_t sgoutnak:1;
		uint32_t cgoutnak:1;
		uint32_t pwronprgdone:1;
		uint32_t reserved_12_31:20;
#endif
	} s;
	struct cvmx_usbcx_dctl_s cn30xx;
	struct cvmx_usbcx_dctl_s cn31xx;
	struct cvmx_usbcx_dctl_s cn50xx;
	struct cvmx_usbcx_dctl_s cn52xx;
	struct cvmx_usbcx_dctl_s cn52xxp1;
	struct cvmx_usbcx_dctl_s cn56xx;
	struct cvmx_usbcx_dctl_s cn56xxp1;
} cvmx_usbcx_dctl_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_diepctlx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t epena:1;
		uint32_t epdis:1;
		uint32_t setd1pid:1;
		uint32_t setd0pid:1;
		uint32_t snak:1;
		uint32_t cnak:1;
		uint32_t txfnum:4;
		uint32_t stall:1;
		uint32_t reserved_20_20:1;
		uint32_t eptype:2;
		uint32_t naksts:1;
		uint32_t dpid:1;
		uint32_t usbactep:1;
		uint32_t nextep:4;
		uint32_t mps:11;
#else
		uint32_t mps:11;
		uint32_t nextep:4;
		uint32_t usbactep:1;
		uint32_t dpid:1;
		uint32_t naksts:1;
		uint32_t eptype:2;
		uint32_t reserved_20_20:1;
		uint32_t stall:1;
		uint32_t txfnum:4;
		uint32_t cnak:1;
		uint32_t snak:1;
		uint32_t setd0pid:1;
		uint32_t setd1pid:1;
		uint32_t epdis:1;
		uint32_t epena:1;
#endif
	} s;
	struct cvmx_usbcx_diepctlx_s cn30xx;
	struct cvmx_usbcx_diepctlx_s cn31xx;
	struct cvmx_usbcx_diepctlx_s cn50xx;
	struct cvmx_usbcx_diepctlx_s cn52xx;
	struct cvmx_usbcx_diepctlx_s cn52xxp1;
	struct cvmx_usbcx_diepctlx_s cn56xx;
	struct cvmx_usbcx_diepctlx_s cn56xxp1;
} cvmx_usbcx_diepctlx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_diepintx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_7_31:25;
		uint32_t inepnakeff:1;
		uint32_t intknepmis:1;
		uint32_t intkntxfemp:1;
		uint32_t timeout:1;
		uint32_t ahberr:1;
		uint32_t epdisbld:1;
		uint32_t xfercompl:1;
#else
		uint32_t xfercompl:1;
		uint32_t epdisbld:1;
		uint32_t ahberr:1;
		uint32_t timeout:1;
		uint32_t intkntxfemp:1;
		uint32_t intknepmis:1;
		uint32_t inepnakeff:1;
		uint32_t reserved_7_31:25;
#endif
	} s;
	struct cvmx_usbcx_diepintx_s cn30xx;
	struct cvmx_usbcx_diepintx_s cn31xx;
	struct cvmx_usbcx_diepintx_s cn50xx;
	struct cvmx_usbcx_diepintx_s cn52xx;
	struct cvmx_usbcx_diepintx_s cn52xxp1;
	struct cvmx_usbcx_diepintx_s cn56xx;
	struct cvmx_usbcx_diepintx_s cn56xxp1;
} cvmx_usbcx_diepintx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_diepmsk_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_7_31:25;
		uint32_t inepnakeffmsk:1;
		uint32_t intknepmismsk:1;
		uint32_t intkntxfempmsk:1;
		uint32_t timeoutmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t epdisbldmsk:1;
		uint32_t xfercomplmsk:1;
#else
		uint32_t xfercomplmsk:1;
		uint32_t epdisbldmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t timeoutmsk:1;
		uint32_t intkntxfempmsk:1;
		uint32_t intknepmismsk:1;
		uint32_t inepnakeffmsk:1;
		uint32_t reserved_7_31:25;
#endif
	} s;
	struct cvmx_usbcx_diepmsk_s cn30xx;
	struct cvmx_usbcx_diepmsk_s cn31xx;
	struct cvmx_usbcx_diepmsk_s cn50xx;
	struct cvmx_usbcx_diepmsk_s cn52xx;
	struct cvmx_usbcx_diepmsk_s cn52xxp1;
	struct cvmx_usbcx_diepmsk_s cn56xx;
	struct cvmx_usbcx_diepmsk_s cn56xxp1;
} cvmx_usbcx_diepmsk_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dieptsizx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_31_31:1;
		uint32_t mc:2;
		uint32_t pktcnt:10;
		uint32_t xfersize:19;
#else
		uint32_t xfersize:19;
		uint32_t pktcnt:10;
		uint32_t mc:2;
		uint32_t reserved_31_31:1;
#endif
	} s;
	struct cvmx_usbcx_dieptsizx_s cn30xx;
	struct cvmx_usbcx_dieptsizx_s cn31xx;
	struct cvmx_usbcx_dieptsizx_s cn50xx;
	struct cvmx_usbcx_dieptsizx_s cn52xx;
	struct cvmx_usbcx_dieptsizx_s cn52xxp1;
	struct cvmx_usbcx_dieptsizx_s cn56xx;
	struct cvmx_usbcx_dieptsizx_s cn56xxp1;
} cvmx_usbcx_dieptsizx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_doepctlx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t epena:1;
		uint32_t epdis:1;
		uint32_t setd1pid:1;
		uint32_t setd0pid:1;
		uint32_t snak:1;
		uint32_t cnak:1;
		uint32_t reserved_22_25:4;
		uint32_t stall:1;
		uint32_t snp:1;
		uint32_t eptype:2;
		uint32_t naksts:1;
		uint32_t dpid:1;
		uint32_t usbactep:1;
		uint32_t reserved_11_14:4;
		uint32_t mps:11;
#else
		uint32_t mps:11;
		uint32_t reserved_11_14:4;
		uint32_t usbactep:1;
		uint32_t dpid:1;
		uint32_t naksts:1;
		uint32_t eptype:2;
		uint32_t snp:1;
		uint32_t stall:1;
		uint32_t reserved_22_25:4;
		uint32_t cnak:1;
		uint32_t snak:1;
		uint32_t setd0pid:1;
		uint32_t setd1pid:1;
		uint32_t epdis:1;
		uint32_t epena:1;
#endif
	} s;
	struct cvmx_usbcx_doepctlx_s cn30xx;
	struct cvmx_usbcx_doepctlx_s cn31xx;
	struct cvmx_usbcx_doepctlx_s cn50xx;
	struct cvmx_usbcx_doepctlx_s cn52xx;
	struct cvmx_usbcx_doepctlx_s cn52xxp1;
	struct cvmx_usbcx_doepctlx_s cn56xx;
	struct cvmx_usbcx_doepctlx_s cn56xxp1;
} cvmx_usbcx_doepctlx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_doepintx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_5_31:27;
		uint32_t outtknepdis:1;
		uint32_t setup:1;
		uint32_t ahberr:1;
		uint32_t epdisbld:1;
		uint32_t xfercompl:1;
#else
		uint32_t xfercompl:1;
		uint32_t epdisbld:1;
		uint32_t ahberr:1;
		uint32_t setup:1;
		uint32_t outtknepdis:1;
		uint32_t reserved_5_31:27;
#endif
	} s;
	struct cvmx_usbcx_doepintx_s cn30xx;
	struct cvmx_usbcx_doepintx_s cn31xx;
	struct cvmx_usbcx_doepintx_s cn50xx;
	struct cvmx_usbcx_doepintx_s cn52xx;
	struct cvmx_usbcx_doepintx_s cn52xxp1;
	struct cvmx_usbcx_doepintx_s cn56xx;
	struct cvmx_usbcx_doepintx_s cn56xxp1;
} cvmx_usbcx_doepintx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_doepmsk_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_5_31:27;
		uint32_t outtknepdismsk:1;
		uint32_t setupmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t epdisbldmsk:1;
		uint32_t xfercomplmsk:1;
#else
		uint32_t xfercomplmsk:1;
		uint32_t epdisbldmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t setupmsk:1;
		uint32_t outtknepdismsk:1;
		uint32_t reserved_5_31:27;
#endif
	} s;
	struct cvmx_usbcx_doepmsk_s cn30xx;
	struct cvmx_usbcx_doepmsk_s cn31xx;
	struct cvmx_usbcx_doepmsk_s cn50xx;
	struct cvmx_usbcx_doepmsk_s cn52xx;
	struct cvmx_usbcx_doepmsk_s cn52xxp1;
	struct cvmx_usbcx_doepmsk_s cn56xx;
	struct cvmx_usbcx_doepmsk_s cn56xxp1;
} cvmx_usbcx_doepmsk_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_doeptsizx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_31_31:1;
		uint32_t mc:2;
		uint32_t pktcnt:10;
		uint32_t xfersize:19;
#else
		uint32_t xfersize:19;
		uint32_t pktcnt:10;
		uint32_t mc:2;
		uint32_t reserved_31_31:1;
#endif
	} s;
	struct cvmx_usbcx_doeptsizx_s cn30xx;
	struct cvmx_usbcx_doeptsizx_s cn31xx;
	struct cvmx_usbcx_doeptsizx_s cn50xx;
	struct cvmx_usbcx_doeptsizx_s cn52xx;
	struct cvmx_usbcx_doeptsizx_s cn52xxp1;
	struct cvmx_usbcx_doeptsizx_s cn56xx;
	struct cvmx_usbcx_doeptsizx_s cn56xxp1;
} cvmx_usbcx_doeptsizx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dptxfsizx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t dptxfsize:16;
		uint32_t dptxfstaddr:16;
#else
		uint32_t dptxfstaddr:16;
		uint32_t dptxfsize:16;
#endif
	} s;
	struct cvmx_usbcx_dptxfsizx_s cn30xx;
	struct cvmx_usbcx_dptxfsizx_s cn31xx;
	struct cvmx_usbcx_dptxfsizx_s cn50xx;
	struct cvmx_usbcx_dptxfsizx_s cn52xx;
	struct cvmx_usbcx_dptxfsizx_s cn52xxp1;
	struct cvmx_usbcx_dptxfsizx_s cn56xx;
	struct cvmx_usbcx_dptxfsizx_s cn56xxp1;
} cvmx_usbcx_dptxfsizx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dsts_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_22_31:10;
		uint32_t soffn:14;
		uint32_t reserved_4_7:4;
		uint32_t errticerr:1;
		uint32_t enumspd:2;
		uint32_t suspsts:1;
#else
		uint32_t suspsts:1;
		uint32_t enumspd:2;
		uint32_t errticerr:1;
		uint32_t reserved_4_7:4;
		uint32_t soffn:14;
		uint32_t reserved_22_31:10;
#endif
	} s;
	struct cvmx_usbcx_dsts_s cn30xx;
	struct cvmx_usbcx_dsts_s cn31xx;
	struct cvmx_usbcx_dsts_s cn50xx;
	struct cvmx_usbcx_dsts_s cn52xx;
	struct cvmx_usbcx_dsts_s cn52xxp1;
	struct cvmx_usbcx_dsts_s cn56xx;
	struct cvmx_usbcx_dsts_s cn56xxp1;
} cvmx_usbcx_dsts_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dtknqr1_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t eptkn:24;
		uint32_t wrapbit:1;
		uint32_t reserved_5_6:2;
		uint32_t intknwptr:5;
#else
		uint32_t intknwptr:5;
		uint32_t reserved_5_6:2;
		uint32_t wrapbit:1;
		uint32_t eptkn:24;
#endif
	} s;
	struct cvmx_usbcx_dtknqr1_s cn30xx;
	struct cvmx_usbcx_dtknqr1_s cn31xx;
	struct cvmx_usbcx_dtknqr1_s cn50xx;
	struct cvmx_usbcx_dtknqr1_s cn52xx;
	struct cvmx_usbcx_dtknqr1_s cn52xxp1;
	struct cvmx_usbcx_dtknqr1_s cn56xx;
	struct cvmx_usbcx_dtknqr1_s cn56xxp1;
} cvmx_usbcx_dtknqr1_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dtknqr2_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t eptkn:32;
#else
		uint32_t eptkn:32;
#endif
	} s;
	struct cvmx_usbcx_dtknqr2_s cn30xx;
	struct cvmx_usbcx_dtknqr2_s cn31xx;
	struct cvmx_usbcx_dtknqr2_s cn50xx;
	struct cvmx_usbcx_dtknqr2_s cn52xx;
	struct cvmx_usbcx_dtknqr2_s cn52xxp1;
	struct cvmx_usbcx_dtknqr2_s cn56xx;
	struct cvmx_usbcx_dtknqr2_s cn56xxp1;
} cvmx_usbcx_dtknqr2_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dtknqr3_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t eptkn:32;
#else
		uint32_t eptkn:32;
#endif
	} s;
	struct cvmx_usbcx_dtknqr3_s cn30xx;
	struct cvmx_usbcx_dtknqr3_s cn31xx;
	struct cvmx_usbcx_dtknqr3_s cn50xx;
	struct cvmx_usbcx_dtknqr3_s cn52xx;
	struct cvmx_usbcx_dtknqr3_s cn52xxp1;
	struct cvmx_usbcx_dtknqr3_s cn56xx;
	struct cvmx_usbcx_dtknqr3_s cn56xxp1;
} cvmx_usbcx_dtknqr3_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_dtknqr4_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t eptkn:32;
#else
		uint32_t eptkn:32;
#endif
	} s;
	struct cvmx_usbcx_dtknqr4_s cn30xx;
	struct cvmx_usbcx_dtknqr4_s cn31xx;
	struct cvmx_usbcx_dtknqr4_s cn50xx;
	struct cvmx_usbcx_dtknqr4_s cn52xx;
	struct cvmx_usbcx_dtknqr4_s cn52xxp1;
	struct cvmx_usbcx_dtknqr4_s cn56xx;
	struct cvmx_usbcx_dtknqr4_s cn56xxp1;
} cvmx_usbcx_dtknqr4_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gahbcfg_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_9_31:23;
		uint32_t ptxfemplvl:1;
		uint32_t nptxfemplvl:1;
		uint32_t reserved_6_6:1;
		uint32_t dmaen:1;
		uint32_t hbstlen:4;
		uint32_t glblintrmsk:1;
#else
		uint32_t glblintrmsk:1;
		uint32_t hbstlen:4;
		uint32_t dmaen:1;
		uint32_t reserved_6_6:1;
		uint32_t nptxfemplvl:1;
		uint32_t ptxfemplvl:1;
		uint32_t reserved_9_31:23;
#endif
	} s;
	struct cvmx_usbcx_gahbcfg_s cn30xx;
	struct cvmx_usbcx_gahbcfg_s cn31xx;
	struct cvmx_usbcx_gahbcfg_s cn50xx;
	struct cvmx_usbcx_gahbcfg_s cn52xx;
	struct cvmx_usbcx_gahbcfg_s cn52xxp1;
	struct cvmx_usbcx_gahbcfg_s cn56xx;
	struct cvmx_usbcx_gahbcfg_s cn56xxp1;
} cvmx_usbcx_gahbcfg_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_ghwcfg1_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t epdir:32;
#else
		uint32_t epdir:32;
#endif
	} s;
	struct cvmx_usbcx_ghwcfg1_s cn30xx;
	struct cvmx_usbcx_ghwcfg1_s cn31xx;
	struct cvmx_usbcx_ghwcfg1_s cn50xx;
	struct cvmx_usbcx_ghwcfg1_s cn52xx;
	struct cvmx_usbcx_ghwcfg1_s cn52xxp1;
	struct cvmx_usbcx_ghwcfg1_s cn56xx;
	struct cvmx_usbcx_ghwcfg1_s cn56xxp1;
} cvmx_usbcx_ghwcfg1_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_ghwcfg2_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_31_31:1;
		uint32_t tknqdepth:5;
		uint32_t ptxqdepth:2;
		uint32_t nptxqdepth:2;
		uint32_t reserved_20_21:2;
		uint32_t dynfifosizing:1;
		uint32_t periosupport:1;
		uint32_t numhstchnl:4;
		uint32_t numdeveps:4;
		uint32_t fsphytype:2;
		uint32_t hsphytype:2;
		uint32_t singpnt:1;
		uint32_t otgarch:2;
		uint32_t otgmode:3;
#else
		uint32_t otgmode:3;
		uint32_t otgarch:2;
		uint32_t singpnt:1;
		uint32_t hsphytype:2;
		uint32_t fsphytype:2;
		uint32_t numdeveps:4;
		uint32_t numhstchnl:4;
		uint32_t periosupport:1;
		uint32_t dynfifosizing:1;
		uint32_t reserved_20_21:2;
		uint32_t nptxqdepth:2;
		uint32_t ptxqdepth:2;
		uint32_t tknqdepth:5;
		uint32_t reserved_31_31:1;
#endif
	} s;
	struct cvmx_usbcx_ghwcfg2_s cn30xx;
	struct cvmx_usbcx_ghwcfg2_s cn31xx;
	struct cvmx_usbcx_ghwcfg2_s cn50xx;
	struct cvmx_usbcx_ghwcfg2_s cn52xx;
	struct cvmx_usbcx_ghwcfg2_s cn52xxp1;
	struct cvmx_usbcx_ghwcfg2_s cn56xx;
	struct cvmx_usbcx_ghwcfg2_s cn56xxp1;
} cvmx_usbcx_ghwcfg2_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_ghwcfg3_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t dfifodepth:16;
		uint32_t reserved_13_15:3;
		uint32_t ahbphysync:1;
		uint32_t rsttype:1;
		uint32_t optfeature:1;
		uint32_t vendor_control_interface_support:1;

		uint32_t i2c_selection:1;
		uint32_t otgen:1;
		uint32_t pktsizewidth:3;
		uint32_t xfersizewidth:4;
#else
		uint32_t xfersizewidth:4;
		uint32_t pktsizewidth:3;
		uint32_t otgen:1;
		uint32_t i2c_selection:1;
		uint32_t vendor_control_interface_support:1;
		uint32_t optfeature:1;
		uint32_t rsttype:1;
		uint32_t ahbphysync:1;
		uint32_t reserved_13_15:3;
		uint32_t dfifodepth:16;
#endif
	} s;
	struct cvmx_usbcx_ghwcfg3_s cn30xx;
	struct cvmx_usbcx_ghwcfg3_s cn31xx;
	struct cvmx_usbcx_ghwcfg3_s cn50xx;
	struct cvmx_usbcx_ghwcfg3_s cn52xx;
	struct cvmx_usbcx_ghwcfg3_s cn52xxp1;
	struct cvmx_usbcx_ghwcfg3_s cn56xx;
	struct cvmx_usbcx_ghwcfg3_s cn56xxp1;
} cvmx_usbcx_ghwcfg3_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_ghwcfg4_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_30_31:2;
		uint32_t numdevmodinend:4;
		uint32_t endedtrfifo:1;
		uint32_t sessendfltr:1;
		uint32_t bvalidfltr:1;
		uint32_t avalidfltr:1;
		uint32_t vbusvalidfltr:1;
		uint32_t iddgfltr:1;
		uint32_t numctleps:4;
		uint32_t phydatawidth:2;
		uint32_t reserved_6_13:8;
		uint32_t ahbfreq:1;
		uint32_t enablepwropt:1;
		uint32_t numdevperioeps:4;
#else
		uint32_t numdevperioeps:4;
		uint32_t enablepwropt:1;
		uint32_t ahbfreq:1;
		uint32_t reserved_6_13:8;
		uint32_t phydatawidth:2;
		uint32_t numctleps:4;
		uint32_t iddgfltr:1;
		uint32_t vbusvalidfltr:1;
		uint32_t avalidfltr:1;
		uint32_t bvalidfltr:1;
		uint32_t sessendfltr:1;
		uint32_t endedtrfifo:1;
		uint32_t numdevmodinend:4;
		uint32_t reserved_30_31:2;
#endif
	} s;
	struct cvmx_usbcx_ghwcfg4_cn30xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_25_31:7;
		uint32_t sessendfltr:1;
		uint32_t bvalidfltr:1;
		uint32_t avalidfltr:1;
		uint32_t vbusvalidfltr:1;
		uint32_t iddgfltr:1;
		uint32_t numctleps:4;
		uint32_t phydatawidth:2;
		uint32_t reserved_6_13:8;
		uint32_t ahbfreq:1;
		uint32_t enablepwropt:1;
		uint32_t numdevperioeps:4;
#else
		uint32_t numdevperioeps:4;
		uint32_t enablepwropt:1;
		uint32_t ahbfreq:1;
		uint32_t reserved_6_13:8;
		uint32_t phydatawidth:2;
		uint32_t numctleps:4;
		uint32_t iddgfltr:1;
		uint32_t vbusvalidfltr:1;
		uint32_t avalidfltr:1;
		uint32_t bvalidfltr:1;
		uint32_t sessendfltr:1;
		uint32_t reserved_25_31:7;
#endif
	} cn30xx;
	struct cvmx_usbcx_ghwcfg4_cn30xx cn31xx;
	struct cvmx_usbcx_ghwcfg4_s cn50xx;
	struct cvmx_usbcx_ghwcfg4_s cn52xx;
	struct cvmx_usbcx_ghwcfg4_s cn52xxp1;
	struct cvmx_usbcx_ghwcfg4_s cn56xx;
	struct cvmx_usbcx_ghwcfg4_s cn56xxp1;
} cvmx_usbcx_ghwcfg4_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gintmsk_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t wkupintmsk:1;
		uint32_t sessreqintmsk:1;
		uint32_t disconnintmsk:1;
		uint32_t conidstschngmsk:1;
		uint32_t reserved_27_27:1;
		uint32_t ptxfempmsk:1;
		uint32_t hchintmsk:1;
		uint32_t prtintmsk:1;
		uint32_t reserved_23_23:1;
		uint32_t fetsuspmsk:1;
		uint32_t incomplpmsk:1;
		uint32_t incompisoinmsk:1;
		uint32_t oepintmsk:1;
		uint32_t inepintmsk:1;
		uint32_t epmismsk:1;
		uint32_t reserved_16_16:1;
		uint32_t eopfmsk:1;
		uint32_t isooutdropmsk:1;
		uint32_t enumdonemsk:1;
		uint32_t usbrstmsk:1;
		uint32_t usbsuspmsk:1;
		uint32_t erlysuspmsk:1;
		uint32_t i2cint:1;
		uint32_t ulpickintmsk:1;
		uint32_t goutnakeffmsk:1;
		uint32_t ginnakeffmsk:1;
		uint32_t nptxfempmsk:1;
		uint32_t rxflvlmsk:1;
		uint32_t sofmsk:1;
		uint32_t otgintmsk:1;
		uint32_t modemismsk:1;
		uint32_t reserved_0_0:1;
#else
		uint32_t reserved_0_0:1;
		uint32_t modemismsk:1;
		uint32_t otgintmsk:1;
		uint32_t sofmsk:1;
		uint32_t rxflvlmsk:1;
		uint32_t nptxfempmsk:1;
		uint32_t ginnakeffmsk:1;
		uint32_t goutnakeffmsk:1;
		uint32_t ulpickintmsk:1;
		uint32_t i2cint:1;
		uint32_t erlysuspmsk:1;
		uint32_t usbsuspmsk:1;
		uint32_t usbrstmsk:1;
		uint32_t enumdonemsk:1;
		uint32_t isooutdropmsk:1;
		uint32_t eopfmsk:1;
		uint32_t reserved_16_16:1;
		uint32_t epmismsk:1;
		uint32_t inepintmsk:1;
		uint32_t oepintmsk:1;
		uint32_t incompisoinmsk:1;
		uint32_t incomplpmsk:1;
		uint32_t fetsuspmsk:1;
		uint32_t reserved_23_23:1;
		uint32_t prtintmsk:1;
		uint32_t hchintmsk:1;
		uint32_t ptxfempmsk:1;
		uint32_t reserved_27_27:1;
		uint32_t conidstschngmsk:1;
		uint32_t disconnintmsk:1;
		uint32_t sessreqintmsk:1;
		uint32_t wkupintmsk:1;
#endif
	} s;
	struct cvmx_usbcx_gintmsk_s cn30xx;
	struct cvmx_usbcx_gintmsk_s cn31xx;
	struct cvmx_usbcx_gintmsk_s cn50xx;
	struct cvmx_usbcx_gintmsk_s cn52xx;
	struct cvmx_usbcx_gintmsk_s cn52xxp1;
	struct cvmx_usbcx_gintmsk_s cn56xx;
	struct cvmx_usbcx_gintmsk_s cn56xxp1;
} cvmx_usbcx_gintmsk_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gintsts_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t wkupint:1;
		uint32_t sessreqint:1;
		uint32_t disconnint:1;
		uint32_t conidstschng:1;
		uint32_t reserved_27_27:1;
		uint32_t ptxfemp:1;
		uint32_t hchint:1;
		uint32_t prtint:1;
		uint32_t reserved_23_23:1;
		uint32_t fetsusp:1;
		uint32_t incomplp:1;
		uint32_t incompisoin:1;
		uint32_t oepint:1;
		uint32_t iepint:1;
		uint32_t epmis:1;
		uint32_t reserved_16_16:1;
		uint32_t eopf:1;
		uint32_t isooutdrop:1;
		uint32_t enumdone:1;
		uint32_t usbrst:1;
		uint32_t usbsusp:1;
		uint32_t erlysusp:1;
		uint32_t i2cint:1;
		uint32_t ulpickint:1;
		uint32_t goutnakeff:1;
		uint32_t ginnakeff:1;
		uint32_t nptxfemp:1;
		uint32_t rxflvl:1;
		uint32_t sof:1;
		uint32_t otgint:1;
		uint32_t modemis:1;
		uint32_t curmod:1;
#else
		uint32_t curmod:1;
		uint32_t modemis:1;
		uint32_t otgint:1;
		uint32_t sof:1;
		uint32_t rxflvl:1;
		uint32_t nptxfemp:1;
		uint32_t ginnakeff:1;
		uint32_t goutnakeff:1;
		uint32_t ulpickint:1;
		uint32_t i2cint:1;
		uint32_t erlysusp:1;
		uint32_t usbsusp:1;
		uint32_t usbrst:1;
		uint32_t enumdone:1;
		uint32_t isooutdrop:1;
		uint32_t eopf:1;
		uint32_t reserved_16_16:1;
		uint32_t epmis:1;
		uint32_t iepint:1;
		uint32_t oepint:1;
		uint32_t incompisoin:1;
		uint32_t incomplp:1;
		uint32_t fetsusp:1;
		uint32_t reserved_23_23:1;
		uint32_t prtint:1;
		uint32_t hchint:1;
		uint32_t ptxfemp:1;
		uint32_t reserved_27_27:1;
		uint32_t conidstschng:1;
		uint32_t disconnint:1;
		uint32_t sessreqint:1;
		uint32_t wkupint:1;
#endif
	} s;
	struct cvmx_usbcx_gintsts_s cn30xx;
	struct cvmx_usbcx_gintsts_s cn31xx;
	struct cvmx_usbcx_gintsts_s cn50xx;
	struct cvmx_usbcx_gintsts_s cn52xx;
	struct cvmx_usbcx_gintsts_s cn52xxp1;
	struct cvmx_usbcx_gintsts_s cn56xx;
	struct cvmx_usbcx_gintsts_s cn56xxp1;
} cvmx_usbcx_gintsts_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gnptxfsiz_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t nptxfdep:16;
		uint32_t nptxfstaddr:16;
#else
		uint32_t nptxfstaddr:16;
		uint32_t nptxfdep:16;
#endif
	} s;
	struct cvmx_usbcx_gnptxfsiz_s cn30xx;
	struct cvmx_usbcx_gnptxfsiz_s cn31xx;
	struct cvmx_usbcx_gnptxfsiz_s cn50xx;
	struct cvmx_usbcx_gnptxfsiz_s cn52xx;
	struct cvmx_usbcx_gnptxfsiz_s cn52xxp1;
	struct cvmx_usbcx_gnptxfsiz_s cn56xx;
	struct cvmx_usbcx_gnptxfsiz_s cn56xxp1;
} cvmx_usbcx_gnptxfsiz_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gnptxsts_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_31_31:1;
		uint32_t nptxqtop:7;
		uint32_t nptxqspcavail:8;
		uint32_t nptxfspcavail:16;
#else
		uint32_t nptxfspcavail:16;
		uint32_t nptxqspcavail:8;
		uint32_t nptxqtop:7;
		uint32_t reserved_31_31:1;
#endif
	} s;
	struct cvmx_usbcx_gnptxsts_s cn30xx;
	struct cvmx_usbcx_gnptxsts_s cn31xx;
	struct cvmx_usbcx_gnptxsts_s cn50xx;
	struct cvmx_usbcx_gnptxsts_s cn52xx;
	struct cvmx_usbcx_gnptxsts_s cn52xxp1;
	struct cvmx_usbcx_gnptxsts_s cn56xx;
	struct cvmx_usbcx_gnptxsts_s cn56xxp1;
} cvmx_usbcx_gnptxsts_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gotgctl_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_20_31:12;
		uint32_t bsesvld:1;
		uint32_t asesvld:1;
		uint32_t dbnctime:1;
		uint32_t conidsts:1;
		uint32_t reserved_12_15:4;
		uint32_t devhnpen:1;
		uint32_t hstsethnpen:1;
		uint32_t hnpreq:1;
		uint32_t hstnegscs:1;
		uint32_t reserved_2_7:6;
		uint32_t sesreq:1;
		uint32_t sesreqscs:1;
#else
		uint32_t sesreqscs:1;
		uint32_t sesreq:1;
		uint32_t reserved_2_7:6;
		uint32_t hstnegscs:1;
		uint32_t hnpreq:1;
		uint32_t hstsethnpen:1;
		uint32_t devhnpen:1;
		uint32_t reserved_12_15:4;
		uint32_t conidsts:1;
		uint32_t dbnctime:1;
		uint32_t asesvld:1;
		uint32_t bsesvld:1;
		uint32_t reserved_20_31:12;
#endif
	} s;
	struct cvmx_usbcx_gotgctl_s cn30xx;
	struct cvmx_usbcx_gotgctl_s cn31xx;
	struct cvmx_usbcx_gotgctl_s cn50xx;
	struct cvmx_usbcx_gotgctl_s cn52xx;
	struct cvmx_usbcx_gotgctl_s cn52xxp1;
	struct cvmx_usbcx_gotgctl_s cn56xx;
	struct cvmx_usbcx_gotgctl_s cn56xxp1;
} cvmx_usbcx_gotgctl_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gotgint_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_20_31:12;
		uint32_t dbncedone:1;
		uint32_t adevtoutchg:1;
		uint32_t hstnegdet:1;
		uint32_t reserved_10_16:7;
		uint32_t hstnegsucstschng:1;
		uint32_t sesreqsucstschng:1;
		uint32_t reserved_3_7:5;
		uint32_t sesenddet:1;
		uint32_t reserved_0_1:2;
#else
		uint32_t reserved_0_1:2;
		uint32_t sesenddet:1;
		uint32_t reserved_3_7:5;
		uint32_t sesreqsucstschng:1;
		uint32_t hstnegsucstschng:1;
		uint32_t reserved_10_16:7;
		uint32_t hstnegdet:1;
		uint32_t adevtoutchg:1;
		uint32_t dbncedone:1;
		uint32_t reserved_20_31:12;
#endif
	} s;
	struct cvmx_usbcx_gotgint_s cn30xx;
	struct cvmx_usbcx_gotgint_s cn31xx;
	struct cvmx_usbcx_gotgint_s cn50xx;
	struct cvmx_usbcx_gotgint_s cn52xx;
	struct cvmx_usbcx_gotgint_s cn52xxp1;
	struct cvmx_usbcx_gotgint_s cn56xx;
	struct cvmx_usbcx_gotgint_s cn56xxp1;
} cvmx_usbcx_gotgint_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grstctl_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t ahbidle:1;
		uint32_t dmareq:1;
		uint32_t reserved_11_29:19;
		uint32_t txfnum:5;
		uint32_t txfflsh:1;
		uint32_t rxfflsh:1;
		uint32_t intknqflsh:1;
		uint32_t frmcntrrst:1;
		uint32_t hsftrst:1;
		uint32_t csftrst:1;
#else
		uint32_t csftrst:1;
		uint32_t hsftrst:1;
		uint32_t frmcntrrst:1;
		uint32_t intknqflsh:1;
		uint32_t rxfflsh:1;
		uint32_t txfflsh:1;
		uint32_t txfnum:5;
		uint32_t reserved_11_29:19;
		uint32_t dmareq:1;
		uint32_t ahbidle:1;
#endif
	} s;
	struct cvmx_usbcx_grstctl_s cn30xx;
	struct cvmx_usbcx_grstctl_s cn31xx;
	struct cvmx_usbcx_grstctl_s cn50xx;
	struct cvmx_usbcx_grstctl_s cn52xx;
	struct cvmx_usbcx_grstctl_s cn52xxp1;
	struct cvmx_usbcx_grstctl_s cn56xx;
	struct cvmx_usbcx_grstctl_s cn56xxp1;
} cvmx_usbcx_grstctl_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grxfsiz_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_16_31:16;
		uint32_t rxfdep:16;
#else
		uint32_t rxfdep:16;
		uint32_t reserved_16_31:16;
#endif
	} s;
	struct cvmx_usbcx_grxfsiz_s cn30xx;
	struct cvmx_usbcx_grxfsiz_s cn31xx;
	struct cvmx_usbcx_grxfsiz_s cn50xx;
	struct cvmx_usbcx_grxfsiz_s cn52xx;
	struct cvmx_usbcx_grxfsiz_s cn52xxp1;
	struct cvmx_usbcx_grxfsiz_s cn56xx;
	struct cvmx_usbcx_grxfsiz_s cn56xxp1;
} cvmx_usbcx_grxfsiz_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grxstspd_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_25_31:7;
		uint32_t fn:4;
		uint32_t pktsts:4;
		uint32_t dpid:2;
		uint32_t bcnt:11;
		uint32_t epnum:4;
#else
		uint32_t epnum:4;
		uint32_t bcnt:11;
		uint32_t dpid:2;
		uint32_t pktsts:4;
		uint32_t fn:4;
		uint32_t reserved_25_31:7;
#endif
	} s;
	struct cvmx_usbcx_grxstspd_s cn30xx;
	struct cvmx_usbcx_grxstspd_s cn31xx;
	struct cvmx_usbcx_grxstspd_s cn50xx;
	struct cvmx_usbcx_grxstspd_s cn52xx;
	struct cvmx_usbcx_grxstspd_s cn52xxp1;
	struct cvmx_usbcx_grxstspd_s cn56xx;
	struct cvmx_usbcx_grxstspd_s cn56xxp1;
} cvmx_usbcx_grxstspd_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grxstsph_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_21_31:11;
		uint32_t pktsts:4;
		uint32_t dpid:2;
		uint32_t bcnt:11;
		uint32_t chnum:4;
#else
		uint32_t chnum:4;
		uint32_t bcnt:11;
		uint32_t dpid:2;
		uint32_t pktsts:4;
		uint32_t reserved_21_31:11;
#endif
	} s;
	struct cvmx_usbcx_grxstsph_s cn30xx;
	struct cvmx_usbcx_grxstsph_s cn31xx;
	struct cvmx_usbcx_grxstsph_s cn50xx;
	struct cvmx_usbcx_grxstsph_s cn52xx;
	struct cvmx_usbcx_grxstsph_s cn52xxp1;
	struct cvmx_usbcx_grxstsph_s cn56xx;
	struct cvmx_usbcx_grxstsph_s cn56xxp1;
} cvmx_usbcx_grxstsph_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grxstsrd_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_25_31:7;
		uint32_t fn:4;
		uint32_t pktsts:4;
		uint32_t dpid:2;
		uint32_t bcnt:11;
		uint32_t epnum:4;
#else
		uint32_t epnum:4;
		uint32_t bcnt:11;
		uint32_t dpid:2;
		uint32_t pktsts:4;
		uint32_t fn:4;
		uint32_t reserved_25_31:7;
#endif
	} s;
	struct cvmx_usbcx_grxstsrd_s cn30xx;
	struct cvmx_usbcx_grxstsrd_s cn31xx;
	struct cvmx_usbcx_grxstsrd_s cn50xx;
	struct cvmx_usbcx_grxstsrd_s cn52xx;
	struct cvmx_usbcx_grxstsrd_s cn52xxp1;
	struct cvmx_usbcx_grxstsrd_s cn56xx;
	struct cvmx_usbcx_grxstsrd_s cn56xxp1;
} cvmx_usbcx_grxstsrd_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_grxstsrh_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_21_31:11;
		uint32_t pktsts:4;
		uint32_t dpid:2;
		uint32_t bcnt:11;
		uint32_t chnum:4;
#else
		uint32_t chnum:4;
		uint32_t bcnt:11;
		uint32_t dpid:2;
		uint32_t pktsts:4;
		uint32_t reserved_21_31:11;
#endif
	} s;
	struct cvmx_usbcx_grxstsrh_s cn30xx;
	struct cvmx_usbcx_grxstsrh_s cn31xx;
	struct cvmx_usbcx_grxstsrh_s cn50xx;
	struct cvmx_usbcx_grxstsrh_s cn52xx;
	struct cvmx_usbcx_grxstsrh_s cn52xxp1;
	struct cvmx_usbcx_grxstsrh_s cn56xx;
	struct cvmx_usbcx_grxstsrh_s cn56xxp1;
} cvmx_usbcx_grxstsrh_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gsnpsid_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t synopsysid:32;
#else
		uint32_t synopsysid:32;
#endif
	} s;
	struct cvmx_usbcx_gsnpsid_s cn30xx;
	struct cvmx_usbcx_gsnpsid_s cn31xx;
	struct cvmx_usbcx_gsnpsid_s cn50xx;
	struct cvmx_usbcx_gsnpsid_s cn52xx;
	struct cvmx_usbcx_gsnpsid_s cn52xxp1;
	struct cvmx_usbcx_gsnpsid_s cn56xx;
	struct cvmx_usbcx_gsnpsid_s cn56xxp1;
} cvmx_usbcx_gsnpsid_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_gusbcfg_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_17_31:15;
		uint32_t otgi2csel:1;
		uint32_t phylpwrclksel:1;
		uint32_t reserved_14_14:1;
		uint32_t usbtrdtim:4;
		uint32_t hnpcap:1;
		uint32_t srpcap:1;
		uint32_t ddrsel:1;
		uint32_t physel:1;
		uint32_t fsintf:1;
		uint32_t ulpi_utmi_sel:1;
		uint32_t phyif:1;
		uint32_t toutcal:3;
#else
		uint32_t toutcal:3;
		uint32_t phyif:1;
		uint32_t ulpi_utmi_sel:1;
		uint32_t fsintf:1;
		uint32_t physel:1;
		uint32_t ddrsel:1;
		uint32_t srpcap:1;
		uint32_t hnpcap:1;
		uint32_t usbtrdtim:4;
		uint32_t reserved_14_14:1;
		uint32_t phylpwrclksel:1;
		uint32_t otgi2csel:1;
		uint32_t reserved_17_31:15;
#endif
	} s;
	struct cvmx_usbcx_gusbcfg_s cn30xx;
	struct cvmx_usbcx_gusbcfg_s cn31xx;
	struct cvmx_usbcx_gusbcfg_s cn50xx;
	struct cvmx_usbcx_gusbcfg_s cn52xx;
	struct cvmx_usbcx_gusbcfg_s cn52xxp1;
	struct cvmx_usbcx_gusbcfg_s cn56xx;
	struct cvmx_usbcx_gusbcfg_s cn56xxp1;
} cvmx_usbcx_gusbcfg_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_haint_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_16_31:16;
		uint32_t haint:16;
#else
		uint32_t haint:16;
		uint32_t reserved_16_31:16;
#endif
	} s;
	struct cvmx_usbcx_haint_s cn30xx;
	struct cvmx_usbcx_haint_s cn31xx;
	struct cvmx_usbcx_haint_s cn50xx;
	struct cvmx_usbcx_haint_s cn52xx;
	struct cvmx_usbcx_haint_s cn52xxp1;
	struct cvmx_usbcx_haint_s cn56xx;
	struct cvmx_usbcx_haint_s cn56xxp1;
} cvmx_usbcx_haint_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_haintmsk_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_16_31:16;
		uint32_t haintmsk:16;
#else
		uint32_t haintmsk:16;
		uint32_t reserved_16_31:16;
#endif
	} s;
	struct cvmx_usbcx_haintmsk_s cn30xx;
	struct cvmx_usbcx_haintmsk_s cn31xx;
	struct cvmx_usbcx_haintmsk_s cn50xx;
	struct cvmx_usbcx_haintmsk_s cn52xx;
	struct cvmx_usbcx_haintmsk_s cn52xxp1;
	struct cvmx_usbcx_haintmsk_s cn56xx;
	struct cvmx_usbcx_haintmsk_s cn56xxp1;
} cvmx_usbcx_haintmsk_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hccharx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t chena:1;
		uint32_t chdis:1;
		uint32_t oddfrm:1;
		uint32_t devaddr:7;
		uint32_t ec:2;
		uint32_t eptype:2;
		uint32_t lspddev:1;
		uint32_t reserved_16_16:1;
		uint32_t epdir:1;
		uint32_t epnum:4;
		uint32_t mps:11;
#else
		uint32_t mps:11;
		uint32_t epnum:4;
		uint32_t epdir:1;
		uint32_t reserved_16_16:1;
		uint32_t lspddev:1;
		uint32_t eptype:2;
		uint32_t ec:2;
		uint32_t devaddr:7;
		uint32_t oddfrm:1;
		uint32_t chdis:1;
		uint32_t chena:1;
#endif
	} s;
	struct cvmx_usbcx_hccharx_s cn30xx;
	struct cvmx_usbcx_hccharx_s cn31xx;
	struct cvmx_usbcx_hccharx_s cn50xx;
	struct cvmx_usbcx_hccharx_s cn52xx;
	struct cvmx_usbcx_hccharx_s cn52xxp1;
	struct cvmx_usbcx_hccharx_s cn56xx;
	struct cvmx_usbcx_hccharx_s cn56xxp1;
} cvmx_usbcx_hccharx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hcfg_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_3_31:29;
		uint32_t fslssupp:1;
		uint32_t fslspclksel:2;
#else
		uint32_t fslspclksel:2;
		uint32_t fslssupp:1;
		uint32_t reserved_3_31:29;
#endif
	} s;
	struct cvmx_usbcx_hcfg_s cn30xx;
	struct cvmx_usbcx_hcfg_s cn31xx;
	struct cvmx_usbcx_hcfg_s cn50xx;
	struct cvmx_usbcx_hcfg_s cn52xx;
	struct cvmx_usbcx_hcfg_s cn52xxp1;
	struct cvmx_usbcx_hcfg_s cn56xx;
	struct cvmx_usbcx_hcfg_s cn56xxp1;
} cvmx_usbcx_hcfg_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hcintx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_11_31:21;
		uint32_t datatglerr:1;
		uint32_t frmovrun:1;
		uint32_t bblerr:1;
		uint32_t xacterr:1;
		uint32_t nyet:1;
		uint32_t ack:1;
		uint32_t nak:1;
		uint32_t stall:1;
		uint32_t ahberr:1;
		uint32_t chhltd:1;
		uint32_t xfercompl:1;
#else
		uint32_t xfercompl:1;
		uint32_t chhltd:1;
		uint32_t ahberr:1;
		uint32_t stall:1;
		uint32_t nak:1;
		uint32_t ack:1;
		uint32_t nyet:1;
		uint32_t xacterr:1;
		uint32_t bblerr:1;
		uint32_t frmovrun:1;
		uint32_t datatglerr:1;
		uint32_t reserved_11_31:21;
#endif
	} s;
	struct cvmx_usbcx_hcintx_s cn30xx;
	struct cvmx_usbcx_hcintx_s cn31xx;
	struct cvmx_usbcx_hcintx_s cn50xx;
	struct cvmx_usbcx_hcintx_s cn52xx;
	struct cvmx_usbcx_hcintx_s cn52xxp1;
	struct cvmx_usbcx_hcintx_s cn56xx;
	struct cvmx_usbcx_hcintx_s cn56xxp1;
} cvmx_usbcx_hcintx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hcintmskx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_11_31:21;
		uint32_t datatglerrmsk:1;
		uint32_t frmovrunmsk:1;
		uint32_t bblerrmsk:1;
		uint32_t xacterrmsk:1;
		uint32_t nyetmsk:1;
		uint32_t ackmsk:1;
		uint32_t nakmsk:1;
		uint32_t stallmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t chhltdmsk:1;
		uint32_t xfercomplmsk:1;
#else
		uint32_t xfercomplmsk:1;
		uint32_t chhltdmsk:1;
		uint32_t ahberrmsk:1;
		uint32_t stallmsk:1;
		uint32_t nakmsk:1;
		uint32_t ackmsk:1;
		uint32_t nyetmsk:1;
		uint32_t xacterrmsk:1;
		uint32_t bblerrmsk:1;
		uint32_t frmovrunmsk:1;
		uint32_t datatglerrmsk:1;
		uint32_t reserved_11_31:21;
#endif
	} s;
	struct cvmx_usbcx_hcintmskx_s cn30xx;
	struct cvmx_usbcx_hcintmskx_s cn31xx;
	struct cvmx_usbcx_hcintmskx_s cn50xx;
	struct cvmx_usbcx_hcintmskx_s cn52xx;
	struct cvmx_usbcx_hcintmskx_s cn52xxp1;
	struct cvmx_usbcx_hcintmskx_s cn56xx;
	struct cvmx_usbcx_hcintmskx_s cn56xxp1;
} cvmx_usbcx_hcintmskx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hcspltx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t spltena:1;
		uint32_t reserved_17_30:14;
		uint32_t compsplt:1;
		uint32_t xactpos:2;
		uint32_t hubaddr:7;
		uint32_t prtaddr:7;
#else
		uint32_t prtaddr:7;
		uint32_t hubaddr:7;
		uint32_t xactpos:2;
		uint32_t compsplt:1;
		uint32_t reserved_17_30:14;
		uint32_t spltena:1;
#endif
	} s;
	struct cvmx_usbcx_hcspltx_s cn30xx;
	struct cvmx_usbcx_hcspltx_s cn31xx;
	struct cvmx_usbcx_hcspltx_s cn50xx;
	struct cvmx_usbcx_hcspltx_s cn52xx;
	struct cvmx_usbcx_hcspltx_s cn52xxp1;
	struct cvmx_usbcx_hcspltx_s cn56xx;
	struct cvmx_usbcx_hcspltx_s cn56xxp1;
} cvmx_usbcx_hcspltx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hctsizx_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t dopng:1;
		uint32_t pid:2;
		uint32_t pktcnt:10;
		uint32_t xfersize:19;
#else
		uint32_t xfersize:19;
		uint32_t pktcnt:10;
		uint32_t pid:2;
		uint32_t dopng:1;
#endif
	} s;
	struct cvmx_usbcx_hctsizx_s cn30xx;
	struct cvmx_usbcx_hctsizx_s cn31xx;
	struct cvmx_usbcx_hctsizx_s cn50xx;
	struct cvmx_usbcx_hctsizx_s cn52xx;
	struct cvmx_usbcx_hctsizx_s cn52xxp1;
	struct cvmx_usbcx_hctsizx_s cn56xx;
	struct cvmx_usbcx_hctsizx_s cn56xxp1;
} cvmx_usbcx_hctsizx_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hfir_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_16_31:16;
		uint32_t frint:16;
#else
		uint32_t frint:16;
		uint32_t reserved_16_31:16;
#endif
	} s;
	struct cvmx_usbcx_hfir_s cn30xx;
	struct cvmx_usbcx_hfir_s cn31xx;
	struct cvmx_usbcx_hfir_s cn50xx;
	struct cvmx_usbcx_hfir_s cn52xx;
	struct cvmx_usbcx_hfir_s cn52xxp1;
	struct cvmx_usbcx_hfir_s cn56xx;
	struct cvmx_usbcx_hfir_s cn56xxp1;
} cvmx_usbcx_hfir_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hfnum_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t frrem:16;
		uint32_t frnum:16;
#else
		uint32_t frnum:16;
		uint32_t frrem:16;
#endif
	} s;
	struct cvmx_usbcx_hfnum_s cn30xx;
	struct cvmx_usbcx_hfnum_s cn31xx;
	struct cvmx_usbcx_hfnum_s cn50xx;
	struct cvmx_usbcx_hfnum_s cn52xx;
	struct cvmx_usbcx_hfnum_s cn52xxp1;
	struct cvmx_usbcx_hfnum_s cn56xx;
	struct cvmx_usbcx_hfnum_s cn56xxp1;
} cvmx_usbcx_hfnum_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hprt_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_19_31:13;
		uint32_t prtspd:2;
		uint32_t prttstctl:4;
		uint32_t prtpwr:1;
		uint32_t prtlnsts:2;
		uint32_t reserved_9_9:1;
		uint32_t prtrst:1;
		uint32_t prtsusp:1;
		uint32_t prtres:1;
		uint32_t prtovrcurrchng:1;
		uint32_t prtovrcurract:1;
		uint32_t prtenchng:1;
		uint32_t prtena:1;
		uint32_t prtconndet:1;
		uint32_t prtconnsts:1;
#else
		uint32_t prtconnsts:1;
		uint32_t prtconndet:1;
		uint32_t prtena:1;
		uint32_t prtenchng:1;
		uint32_t prtovrcurract:1;
		uint32_t prtovrcurrchng:1;
		uint32_t prtres:1;
		uint32_t prtsusp:1;
		uint32_t prtrst:1;
		uint32_t reserved_9_9:1;
		uint32_t prtlnsts:2;
		uint32_t prtpwr:1;
		uint32_t prttstctl:4;
		uint32_t prtspd:2;
		uint32_t reserved_19_31:13;
#endif
	} s;
	struct cvmx_usbcx_hprt_s cn30xx;
	struct cvmx_usbcx_hprt_s cn31xx;
	struct cvmx_usbcx_hprt_s cn50xx;
	struct cvmx_usbcx_hprt_s cn52xx;
	struct cvmx_usbcx_hprt_s cn52xxp1;
	struct cvmx_usbcx_hprt_s cn56xx;
	struct cvmx_usbcx_hprt_s cn56xxp1;
} cvmx_usbcx_hprt_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hptxfsiz_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t ptxfsize:16;
		uint32_t ptxfstaddr:16;
#else
		uint32_t ptxfstaddr:16;
		uint32_t ptxfsize:16;
#endif
	} s;
	struct cvmx_usbcx_hptxfsiz_s cn30xx;
	struct cvmx_usbcx_hptxfsiz_s cn31xx;
	struct cvmx_usbcx_hptxfsiz_s cn50xx;
	struct cvmx_usbcx_hptxfsiz_s cn52xx;
	struct cvmx_usbcx_hptxfsiz_s cn52xxp1;
	struct cvmx_usbcx_hptxfsiz_s cn56xx;
	struct cvmx_usbcx_hptxfsiz_s cn56xxp1;
} cvmx_usbcx_hptxfsiz_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_hptxsts_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t ptxqtop:8;
		uint32_t ptxqspcavail:8;
		uint32_t ptxfspcavail:16;
#else
		uint32_t ptxfspcavail:16;
		uint32_t ptxqspcavail:8;
		uint32_t ptxqtop:8;
#endif
	} s;
	struct cvmx_usbcx_hptxsts_s cn30xx;
	struct cvmx_usbcx_hptxsts_s cn31xx;
	struct cvmx_usbcx_hptxsts_s cn50xx;
	struct cvmx_usbcx_hptxsts_s cn52xx;
	struct cvmx_usbcx_hptxsts_s cn52xxp1;
	struct cvmx_usbcx_hptxsts_s cn56xx;
	struct cvmx_usbcx_hptxsts_s cn56xxp1;
} cvmx_usbcx_hptxsts_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_nptxdfifox_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t data:32;
#else
		uint32_t data:32;
#endif
	} s;
	struct cvmx_usbcx_nptxdfifox_s cn30xx;
	struct cvmx_usbcx_nptxdfifox_s cn31xx;
	struct cvmx_usbcx_nptxdfifox_s cn50xx;
	struct cvmx_usbcx_nptxdfifox_s cn52xx;
	struct cvmx_usbcx_nptxdfifox_s cn52xxp1;
	struct cvmx_usbcx_nptxdfifox_s cn56xx;
	struct cvmx_usbcx_nptxdfifox_s cn56xxp1;
} cvmx_usbcx_nptxdfifox_t;

typedef union {
	uint32_t u32;
	struct cvmx_usbcx_pcgcctl_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint32_t reserved_5_31:27;
		uint32_t physuspended:1;
		uint32_t rstpdwnmodule:1;
		uint32_t pwrclmp:1;
		uint32_t gatehclk:1;
		uint32_t stoppclk:1;
#else
		uint32_t stoppclk:1;
		uint32_t gatehclk:1;
		uint32_t pwrclmp:1;
		uint32_t rstpdwnmodule:1;
		uint32_t physuspended:1;
		uint32_t reserved_5_31:27;
#endif
	} s;
	struct cvmx_usbcx_pcgcctl_s cn30xx;
	struct cvmx_usbcx_pcgcctl_s cn31xx;
	struct cvmx_usbcx_pcgcctl_s cn50xx;
	struct cvmx_usbcx_pcgcctl_s cn52xx;
	struct cvmx_usbcx_pcgcctl_s cn52xxp1;
	struct cvmx_usbcx_pcgcctl_s cn56xx;
	struct cvmx_usbcx_pcgcctl_s cn56xxp1;
} cvmx_usbcx_pcgcctl_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_bist_status_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_7_63:57;
		uint64_t u2nc_bis:1;
		uint64_t u2nf_bis:1;
		uint64_t e2hc_bis:1;
		uint64_t n2uf_bis:1;
		uint64_t usbc_bis:1;
		uint64_t nif_bis:1;
		uint64_t nof_bis:1;
#else
		uint64_t nof_bis:1;
		uint64_t nif_bis:1;
		uint64_t usbc_bis:1;
		uint64_t n2uf_bis:1;
		uint64_t e2hc_bis:1;
		uint64_t u2nf_bis:1;
		uint64_t u2nc_bis:1;
		uint64_t reserved_7_63:57;
#endif
	} s;
	struct cvmx_usbnx_bist_status_cn30xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_3_63:61;
		uint64_t usbc_bis:1;
		uint64_t nif_bis:1;
		uint64_t nof_bis:1;
#else
		uint64_t nof_bis:1;
		uint64_t nif_bis:1;
		uint64_t usbc_bis:1;
		uint64_t reserved_3_63:61;
#endif
	} cn30xx;
	struct cvmx_usbnx_bist_status_cn30xx cn31xx;
	struct cvmx_usbnx_bist_status_s cn50xx;
	struct cvmx_usbnx_bist_status_s cn52xx;
	struct cvmx_usbnx_bist_status_s cn52xxp1;
	struct cvmx_usbnx_bist_status_s cn56xx;
	struct cvmx_usbnx_bist_status_s cn56xxp1;
} cvmx_usbnx_bist_status_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_clk_ctl_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_20_63:44;
		uint64_t divide2:2;
		uint64_t hclk_rst:1;
		uint64_t p_x_on:1;
		uint64_t reserved_14_15:2;
		uint64_t p_com_on:1;
		uint64_t p_c_sel:2;
		uint64_t cdiv_byp:1;
		uint64_t sd_mode:2;
		uint64_t s_bist:1;
		uint64_t por:1;
		uint64_t enable:1;
		uint64_t prst:1;
		uint64_t hrst:1;
		uint64_t divide:3;
#else
		uint64_t divide:3;
		uint64_t hrst:1;
		uint64_t prst:1;
		uint64_t enable:1;
		uint64_t por:1;
		uint64_t s_bist:1;
		uint64_t sd_mode:2;
		uint64_t cdiv_byp:1;
		uint64_t p_c_sel:2;
		uint64_t p_com_on:1;
		uint64_t reserved_14_15:2;
		uint64_t p_x_on:1;
		uint64_t hclk_rst:1;
		uint64_t divide2:2;
		uint64_t reserved_20_63:44;
#endif
	} s;
	struct cvmx_usbnx_clk_ctl_cn30xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_18_63:46;
		uint64_t hclk_rst:1;
		uint64_t p_x_on:1;
		uint64_t p_rclk:1;
		uint64_t p_xenbn:1;
		uint64_t p_com_on:1;
		uint64_t p_c_sel:2;
		uint64_t cdiv_byp:1;
		uint64_t sd_mode:2;
		uint64_t s_bist:1;
		uint64_t por:1;
		uint64_t enable:1;
		uint64_t prst:1;
		uint64_t hrst:1;
		uint64_t divide:3;
#else
		uint64_t divide:3;
		uint64_t hrst:1;
		uint64_t prst:1;
		uint64_t enable:1;
		uint64_t por:1;
		uint64_t s_bist:1;
		uint64_t sd_mode:2;
		uint64_t cdiv_byp:1;
		uint64_t p_c_sel:2;
		uint64_t p_com_on:1;
		uint64_t p_xenbn:1;
		uint64_t p_rclk:1;
		uint64_t p_x_on:1;
		uint64_t hclk_rst:1;
		uint64_t reserved_18_63:46;
#endif
	} cn30xx;
	struct cvmx_usbnx_clk_ctl_cn30xx cn31xx;
	struct cvmx_usbnx_clk_ctl_cn50xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_20_63:44;
		uint64_t divide2:2;
		uint64_t hclk_rst:1;
		uint64_t reserved_16_16:1;
		uint64_t p_rtype:2;
		uint64_t p_com_on:1;
		uint64_t p_c_sel:2;
		uint64_t cdiv_byp:1;
		uint64_t sd_mode:2;
		uint64_t s_bist:1;
		uint64_t por:1;
		uint64_t enable:1;
		uint64_t prst:1;
		uint64_t hrst:1;
		uint64_t divide:3;
#else
		uint64_t divide:3;
		uint64_t hrst:1;
		uint64_t prst:1;
		uint64_t enable:1;
		uint64_t por:1;
		uint64_t s_bist:1;
		uint64_t sd_mode:2;
		uint64_t cdiv_byp:1;
		uint64_t p_c_sel:2;
		uint64_t p_com_on:1;
		uint64_t p_rtype:2;
		uint64_t reserved_16_16:1;
		uint64_t hclk_rst:1;
		uint64_t divide2:2;
		uint64_t reserved_20_63:44;
#endif
	} cn50xx;
	struct cvmx_usbnx_clk_ctl_cn50xx cn52xx;
	struct cvmx_usbnx_clk_ctl_cn50xx cn52xxp1;
	struct cvmx_usbnx_clk_ctl_cn50xx cn56xx;
	struct cvmx_usbnx_clk_ctl_cn50xx cn56xxp1;
} cvmx_usbnx_clk_ctl_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_ctl_status_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_6_63:58;
		uint64_t dma_0pag:1;
		uint64_t dma_stt:1;
		uint64_t dma_test:1;
		uint64_t inv_a2:1;
		uint64_t l2c_emod:2;
#else
		uint64_t l2c_emod:2;
		uint64_t inv_a2:1;
		uint64_t dma_test:1;
		uint64_t dma_stt:1;
		uint64_t dma_0pag:1;
		uint64_t reserved_6_63:58;
#endif
	} s;
	struct cvmx_usbnx_ctl_status_s cn30xx;
	struct cvmx_usbnx_ctl_status_s cn31xx;
	struct cvmx_usbnx_ctl_status_s cn50xx;
	struct cvmx_usbnx_ctl_status_s cn52xx;
	struct cvmx_usbnx_ctl_status_s cn52xxp1;
	struct cvmx_usbnx_ctl_status_s cn56xx;
	struct cvmx_usbnx_ctl_status_s cn56xxp1;
} cvmx_usbnx_ctl_status_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn0_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn0_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn0_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn0_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn0_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn0_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn0_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn0_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn0_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn1_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn1_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn1_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn1_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn1_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn1_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn1_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn1_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn1_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn2_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn2_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn2_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn2_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn2_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn2_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn2_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn2_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn2_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn3_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn3_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn3_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn3_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn3_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn3_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn3_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn3_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn3_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn4_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn4_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn4_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn4_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn4_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn4_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn4_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn4_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn4_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn5_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn5_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn5_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn5_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn5_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn5_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn5_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn5_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn5_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn6_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn6_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn6_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn6_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn6_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn6_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn6_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn6_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn6_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_inb_chn7_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_inb_chn7_s cn30xx;
	struct cvmx_usbnx_dma0_inb_chn7_s cn31xx;
	struct cvmx_usbnx_dma0_inb_chn7_s cn50xx;
	struct cvmx_usbnx_dma0_inb_chn7_s cn52xx;
	struct cvmx_usbnx_dma0_inb_chn7_s cn52xxp1;
	struct cvmx_usbnx_dma0_inb_chn7_s cn56xx;
	struct cvmx_usbnx_dma0_inb_chn7_s cn56xxp1;
} cvmx_usbnx_dma0_inb_chn7_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn0_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn0_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn0_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn0_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn0_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn0_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn0_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn0_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn0_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn1_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn1_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn1_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn1_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn1_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn1_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn1_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn1_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn1_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn2_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn2_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn2_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn2_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn2_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn2_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn2_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn2_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn2_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn3_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn3_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn3_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn3_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn3_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn3_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn3_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn3_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn3_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn4_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn4_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn4_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn4_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn4_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn4_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn4_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn4_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn4_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn5_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn5_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn5_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn5_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn5_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn5_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn5_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn5_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn5_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn6_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn6_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn6_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn6_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn6_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn6_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn6_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn6_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn6_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma0_outb_chn7_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_36_63:28;
		uint64_t addr:36;
#else
		uint64_t addr:36;
		uint64_t reserved_36_63:28;
#endif
	} s;
	struct cvmx_usbnx_dma0_outb_chn7_s cn30xx;
	struct cvmx_usbnx_dma0_outb_chn7_s cn31xx;
	struct cvmx_usbnx_dma0_outb_chn7_s cn50xx;
	struct cvmx_usbnx_dma0_outb_chn7_s cn52xx;
	struct cvmx_usbnx_dma0_outb_chn7_s cn52xxp1;
	struct cvmx_usbnx_dma0_outb_chn7_s cn56xx;
	struct cvmx_usbnx_dma0_outb_chn7_s cn56xxp1;
} cvmx_usbnx_dma0_outb_chn7_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_dma_test_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_40_63:24;
		uint64_t done:1;
		uint64_t req:1;
		uint64_t f_addr:18;
		uint64_t count:11;
		uint64_t channel:5;
		uint64_t burst:4;
#else
		uint64_t burst:4;
		uint64_t channel:5;
		uint64_t count:11;
		uint64_t f_addr:18;
		uint64_t req:1;
		uint64_t done:1;
		uint64_t reserved_40_63:24;
#endif
	} s;
	struct cvmx_usbnx_dma_test_s cn30xx;
	struct cvmx_usbnx_dma_test_s cn31xx;
	struct cvmx_usbnx_dma_test_s cn50xx;
	struct cvmx_usbnx_dma_test_s cn52xx;
	struct cvmx_usbnx_dma_test_s cn52xxp1;
	struct cvmx_usbnx_dma_test_s cn56xx;
	struct cvmx_usbnx_dma_test_s cn56xxp1;
} cvmx_usbnx_dma_test_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_int_enb_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_38_63:26;
		uint64_t nd4o_dpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_rpe:1;
		uint64_t ltl_f_pf:1;
		uint64_t ltl_f_pe:1;
		uint64_t u2n_c_pe:1;
		uint64_t u2n_c_pf:1;
		uint64_t u2n_d_pf:1;
		uint64_t u2n_d_pe:1;
		uint64_t n2u_pe:1;
		uint64_t n2u_pf:1;
		uint64_t uod_pf:1;
		uint64_t uod_pe:1;
		uint64_t rq_q3_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q2_f:1;
		uint64_t rg_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t l2_fi_f:1;
		uint64_t l2_fi_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2c_s_e:1;
		uint64_t dcred_f:1;
		uint64_t dcred_e:1;
		uint64_t lt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t pr_po_e:1;
#else
		uint64_t pr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t lt_pu_f:1;
		uint64_t dcred_e:1;
		uint64_t dcred_f:1;
		uint64_t l2c_s_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2_fi_e:1;
		uint64_t l2_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t rg_fi_f:1;
		uint64_t rq_q2_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q3_e:1;
		uint64_t uod_pe:1;
		uint64_t uod_pf:1;
		uint64_t n2u_pf:1;
		uint64_t n2u_pe:1;
		uint64_t u2n_d_pe:1;
		uint64_t u2n_d_pf:1;
		uint64_t u2n_c_pf:1;
		uint64_t u2n_c_pe:1;
		uint64_t ltl_f_pe:1;
		uint64_t ltl_f_pf:1;
		uint64_t nd4o_rpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_dpf:1;
		uint64_t reserved_38_63:26;
#endif
	} s;
	struct cvmx_usbnx_int_enb_s cn30xx;
	struct cvmx_usbnx_int_enb_s cn31xx;
	struct cvmx_usbnx_int_enb_cn50xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_38_63:26;
		uint64_t nd4o_dpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_rpe:1;
		uint64_t ltl_f_pf:1;
		uint64_t ltl_f_pe:1;
		uint64_t reserved_26_31:6;
		uint64_t uod_pf:1;
		uint64_t uod_pe:1;
		uint64_t rq_q3_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q2_f:1;
		uint64_t rg_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t l2_fi_f:1;
		uint64_t l2_fi_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2c_s_e:1;
		uint64_t dcred_f:1;
		uint64_t dcred_e:1;
		uint64_t lt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t pr_po_e:1;
#else
		uint64_t pr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t lt_pu_f:1;
		uint64_t dcred_e:1;
		uint64_t dcred_f:1;
		uint64_t l2c_s_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2_fi_e:1;
		uint64_t l2_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t rg_fi_f:1;
		uint64_t rq_q2_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q3_e:1;
		uint64_t uod_pe:1;
		uint64_t uod_pf:1;
		uint64_t reserved_26_31:6;
		uint64_t ltl_f_pe:1;
		uint64_t ltl_f_pf:1;
		uint64_t nd4o_rpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_dpf:1;
		uint64_t reserved_38_63:26;
#endif
	} cn50xx;
	struct cvmx_usbnx_int_enb_cn50xx cn52xx;
	struct cvmx_usbnx_int_enb_cn50xx cn52xxp1;
	struct cvmx_usbnx_int_enb_cn50xx cn56xx;
	struct cvmx_usbnx_int_enb_cn50xx cn56xxp1;
} cvmx_usbnx_int_enb_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_int_sum_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_38_63:26;
		uint64_t nd4o_dpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_rpe:1;
		uint64_t ltl_f_pf:1;
		uint64_t ltl_f_pe:1;
		uint64_t u2n_c_pe:1;
		uint64_t u2n_c_pf:1;
		uint64_t u2n_d_pf:1;
		uint64_t u2n_d_pe:1;
		uint64_t n2u_pe:1;
		uint64_t n2u_pf:1;
		uint64_t uod_pf:1;
		uint64_t uod_pe:1;
		uint64_t rq_q3_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q2_f:1;
		uint64_t rg_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t lt_fi_f:1;
		uint64_t lt_fi_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2c_s_e:1;
		uint64_t dcred_f:1;
		uint64_t dcred_e:1;
		uint64_t lt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t pr_po_e:1;
#else
		uint64_t pr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t lt_pu_f:1;
		uint64_t dcred_e:1;
		uint64_t dcred_f:1;
		uint64_t l2c_s_e:1;
		uint64_t l2c_a_f:1;
		uint64_t lt_fi_e:1;
		uint64_t lt_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t rg_fi_f:1;
		uint64_t rq_q2_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q3_e:1;
		uint64_t uod_pe:1;
		uint64_t uod_pf:1;
		uint64_t n2u_pf:1;
		uint64_t n2u_pe:1;
		uint64_t u2n_d_pe:1;
		uint64_t u2n_d_pf:1;
		uint64_t u2n_c_pf:1;
		uint64_t u2n_c_pe:1;
		uint64_t ltl_f_pe:1;
		uint64_t ltl_f_pf:1;
		uint64_t nd4o_rpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_dpf:1;
		uint64_t reserved_38_63:26;
#endif
	} s;
	struct cvmx_usbnx_int_sum_s cn30xx;
	struct cvmx_usbnx_int_sum_s cn31xx;
	struct cvmx_usbnx_int_sum_cn50xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_38_63:26;
		uint64_t nd4o_dpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_rpe:1;
		uint64_t ltl_f_pf:1;
		uint64_t ltl_f_pe:1;
		uint64_t reserved_26_31:6;
		uint64_t uod_pf:1;
		uint64_t uod_pe:1;
		uint64_t rq_q3_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q2_f:1;
		uint64_t rg_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t lt_fi_f:1;
		uint64_t lt_fi_e:1;
		uint64_t l2c_a_f:1;
		uint64_t l2c_s_e:1;
		uint64_t dcred_f:1;
		uint64_t dcred_e:1;
		uint64_t lt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t pr_po_e:1;
#else
		uint64_t pr_po_e:1;
		uint64_t pr_pu_f:1;
		uint64_t nr_po_e:1;
		uint64_t nr_pu_f:1;
		uint64_t lr_po_e:1;
		uint64_t lr_pu_f:1;
		uint64_t pt_po_e:1;
		uint64_t pt_pu_f:1;
		uint64_t nt_po_e:1;
		uint64_t nt_pu_f:1;
		uint64_t lt_po_e:1;
		uint64_t lt_pu_f:1;
		uint64_t dcred_e:1;
		uint64_t dcred_f:1;
		uint64_t l2c_s_e:1;
		uint64_t l2c_a_f:1;
		uint64_t lt_fi_e:1;
		uint64_t lt_fi_f:1;
		uint64_t rg_fi_e:1;
		uint64_t rg_fi_f:1;
		uint64_t rq_q2_f:1;
		uint64_t rq_q2_e:1;
		uint64_t rq_q3_f:1;
		uint64_t rq_q3_e:1;
		uint64_t uod_pe:1;
		uint64_t uod_pf:1;
		uint64_t reserved_26_31:6;
		uint64_t ltl_f_pe:1;
		uint64_t ltl_f_pf:1;
		uint64_t nd4o_rpe:1;
		uint64_t nd4o_rpf:1;
		uint64_t nd4o_dpe:1;
		uint64_t nd4o_dpf:1;
		uint64_t reserved_38_63:26;
#endif
	} cn50xx;
	struct cvmx_usbnx_int_sum_cn50xx cn52xx;
	struct cvmx_usbnx_int_sum_cn50xx cn52xxp1;
	struct cvmx_usbnx_int_sum_cn50xx cn56xx;
	struct cvmx_usbnx_int_sum_cn50xx cn56xxp1;
} cvmx_usbnx_int_sum_t;

typedef union {
	uint64_t u64;
	struct cvmx_usbnx_usbp_ctl_status_s {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t txrisetune:1;
		uint64_t txvreftune:4;
		uint64_t txfslstune:4;
		uint64_t txhsxvtune:2;
		uint64_t sqrxtune:3;
		uint64_t compdistune:3;
		uint64_t otgtune:3;
		uint64_t otgdisable:1;
		uint64_t portreset:1;
		uint64_t drvvbus:1;
		uint64_t lsbist:1;
		uint64_t fsbist:1;
		uint64_t hsbist:1;
		uint64_t bist_done:1;
		uint64_t bist_err:1;
		uint64_t tdata_out:4;
		uint64_t siddq:1;
		uint64_t txpreemphasistune:1;
		uint64_t dma_bmode:1;
		uint64_t usbc_end:1;
		uint64_t usbp_bist:1;
		uint64_t tclk:1;
		uint64_t dp_pulld:1;
		uint64_t dm_pulld:1;
		uint64_t hst_mode:1;
		uint64_t tuning:4;
		uint64_t tx_bs_enh:1;
		uint64_t tx_bs_en:1;
		uint64_t loop_enb:1;
		uint64_t vtest_enb:1;
		uint64_t bist_enb:1;
		uint64_t tdata_sel:1;
		uint64_t taddr_in:4;
		uint64_t tdata_in:8;
		uint64_t ate_reset:1;
#else
		uint64_t ate_reset:1;
		uint64_t tdata_in:8;
		uint64_t taddr_in:4;
		uint64_t tdata_sel:1;
		uint64_t bist_enb:1;
		uint64_t vtest_enb:1;
		uint64_t loop_enb:1;
		uint64_t tx_bs_en:1;
		uint64_t tx_bs_enh:1;
		uint64_t tuning:4;
		uint64_t hst_mode:1;
		uint64_t dm_pulld:1;
		uint64_t dp_pulld:1;
		uint64_t tclk:1;
		uint64_t usbp_bist:1;
		uint64_t usbc_end:1;
		uint64_t dma_bmode:1;
		uint64_t txpreemphasistune:1;
		uint64_t siddq:1;
		uint64_t tdata_out:4;
		uint64_t bist_err:1;
		uint64_t bist_done:1;
		uint64_t hsbist:1;
		uint64_t fsbist:1;
		uint64_t lsbist:1;
		uint64_t drvvbus:1;
		uint64_t portreset:1;
		uint64_t otgdisable:1;
		uint64_t otgtune:3;
		uint64_t compdistune:3;
		uint64_t sqrxtune:3;
		uint64_t txhsxvtune:2;
		uint64_t txfslstune:4;
		uint64_t txvreftune:4;
		uint64_t txrisetune:1;
#endif
	} s;
	struct cvmx_usbnx_usbp_ctl_status_cn30xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t reserved_38_63:26;
		uint64_t bist_done:1;
		uint64_t bist_err:1;
		uint64_t tdata_out:4;
		uint64_t reserved_30_31:2;
		uint64_t dma_bmode:1;
		uint64_t usbc_end:1;
		uint64_t usbp_bist:1;
		uint64_t tclk:1;
		uint64_t dp_pulld:1;
		uint64_t dm_pulld:1;
		uint64_t hst_mode:1;
		uint64_t tuning:4;
		uint64_t tx_bs_enh:1;
		uint64_t tx_bs_en:1;
		uint64_t loop_enb:1;
		uint64_t vtest_enb:1;
		uint64_t bist_enb:1;
		uint64_t tdata_sel:1;
		uint64_t taddr_in:4;
		uint64_t tdata_in:8;
		uint64_t ate_reset:1;
#else
		uint64_t ate_reset:1;
		uint64_t tdata_in:8;
		uint64_t taddr_in:4;
		uint64_t tdata_sel:1;
		uint64_t bist_enb:1;
		uint64_t vtest_enb:1;
		uint64_t loop_enb:1;
		uint64_t tx_bs_en:1;
		uint64_t tx_bs_enh:1;
		uint64_t tuning:4;
		uint64_t hst_mode:1;
		uint64_t dm_pulld:1;
		uint64_t dp_pulld:1;
		uint64_t tclk:1;
		uint64_t usbp_bist:1;
		uint64_t usbc_end:1;
		uint64_t dma_bmode:1;
		uint64_t reserved_30_31:2;
		uint64_t tdata_out:4;
		uint64_t bist_err:1;
		uint64_t bist_done:1;
		uint64_t reserved_38_63:26;
#endif
	} cn30xx;
	struct cvmx_usbnx_usbp_ctl_status_cn30xx cn31xx;
	struct cvmx_usbnx_usbp_ctl_status_cn50xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t txrisetune:1;
		uint64_t txvreftune:4;
		uint64_t txfslstune:4;
		uint64_t txhsxvtune:2;
		uint64_t sqrxtune:3;
		uint64_t compdistune:3;
		uint64_t otgtune:3;
		uint64_t otgdisable:1;
		uint64_t portreset:1;
		uint64_t drvvbus:1;
		uint64_t lsbist:1;
		uint64_t fsbist:1;
		uint64_t hsbist:1;
		uint64_t bist_done:1;
		uint64_t bist_err:1;
		uint64_t tdata_out:4;
		uint64_t reserved_31_31:1;
		uint64_t txpreemphasistune:1;
		uint64_t dma_bmode:1;
		uint64_t usbc_end:1;
		uint64_t usbp_bist:1;
		uint64_t tclk:1;
		uint64_t dp_pulld:1;
		uint64_t dm_pulld:1;
		uint64_t hst_mode:1;
		uint64_t reserved_19_22:4;
		uint64_t tx_bs_enh:1;
		uint64_t tx_bs_en:1;
		uint64_t loop_enb:1;
		uint64_t vtest_enb:1;
		uint64_t bist_enb:1;
		uint64_t tdata_sel:1;
		uint64_t taddr_in:4;
		uint64_t tdata_in:8;
		uint64_t ate_reset:1;
#else
		uint64_t ate_reset:1;
		uint64_t tdata_in:8;
		uint64_t taddr_in:4;
		uint64_t tdata_sel:1;
		uint64_t bist_enb:1;
		uint64_t vtest_enb:1;
		uint64_t loop_enb:1;
		uint64_t tx_bs_en:1;
		uint64_t tx_bs_enh:1;
		uint64_t reserved_19_22:4;
		uint64_t hst_mode:1;
		uint64_t dm_pulld:1;
		uint64_t dp_pulld:1;
		uint64_t tclk:1;
		uint64_t usbp_bist:1;
		uint64_t usbc_end:1;
		uint64_t dma_bmode:1;
		uint64_t txpreemphasistune:1;
		uint64_t reserved_31_31:1;
		uint64_t tdata_out:4;
		uint64_t bist_err:1;
		uint64_t bist_done:1;
		uint64_t hsbist:1;
		uint64_t fsbist:1;
		uint64_t lsbist:1;
		uint64_t drvvbus:1;
		uint64_t portreset:1;
		uint64_t otgdisable:1;
		uint64_t otgtune:3;
		uint64_t compdistune:3;
		uint64_t sqrxtune:3;
		uint64_t txhsxvtune:2;
		uint64_t txfslstune:4;
		uint64_t txvreftune:4;
		uint64_t txrisetune:1;
#endif
	} cn50xx;
	struct cvmx_usbnx_usbp_ctl_status_cn50xx cn52xx;
	struct cvmx_usbnx_usbp_ctl_status_cn50xx cn52xxp1;
	struct cvmx_usbnx_usbp_ctl_status_cn56xx {
#if __BYTE_ORDER == __BIG_ENDIAN
		uint64_t txrisetune:1;
		uint64_t txvreftune:4;
		uint64_t txfslstune:4;
		uint64_t txhsxvtune:2;
		uint64_t sqrxtune:3;
		uint64_t compdistune:3;
		uint64_t otgtune:3;
		uint64_t otgdisable:1;
		uint64_t portreset:1;
		uint64_t drvvbus:1;
		uint64_t lsbist:1;
		uint64_t fsbist:1;
		uint64_t hsbist:1;
		uint64_t bist_done:1;
		uint64_t bist_err:1;
		uint64_t tdata_out:4;
		uint64_t siddq:1;
		uint64_t txpreemphasistune:1;
		uint64_t dma_bmode:1;
		uint64_t usbc_end:1;
		uint64_t usbp_bist:1;
		uint64_t tclk:1;
		uint64_t dp_pulld:1;
		uint64_t dm_pulld:1;
		uint64_t hst_mode:1;
		uint64_t reserved_19_22:4;
		uint64_t tx_bs_enh:1;
		uint64_t tx_bs_en:1;
		uint64_t loop_enb:1;
		uint64_t vtest_enb:1;
		uint64_t bist_enb:1;
		uint64_t tdata_sel:1;
		uint64_t taddr_in:4;
		uint64_t tdata_in:8;
		uint64_t ate_reset:1;
#else
		uint64_t ate_reset:1;
		uint64_t tdata_in:8;
		uint64_t taddr_in:4;
		uint64_t tdata_sel:1;
		uint64_t bist_enb:1;
		uint64_t vtest_enb:1;
		uint64_t loop_enb:1;
		uint64_t tx_bs_en:1;
		uint64_t tx_bs_enh:1;
		uint64_t reserved_19_22:4;
		uint64_t hst_mode:1;
		uint64_t dm_pulld:1;
		uint64_t dp_pulld:1;
		uint64_t tclk:1;
		uint64_t usbp_bist:1;
		uint64_t usbc_end:1;
		uint64_t dma_bmode:1;
		uint64_t txpreemphasistune:1;
		uint64_t siddq:1;
		uint64_t tdata_out:4;
		uint64_t bist_err:1;
		uint64_t bist_done:1;
		uint64_t hsbist:1;
		uint64_t fsbist:1;
		uint64_t lsbist:1;
		uint64_t drvvbus:1;
		uint64_t portreset:1;
		uint64_t otgdisable:1;
		uint64_t otgtune:3;
		uint64_t compdistune:3;
		uint64_t sqrxtune:3;
		uint64_t txhsxvtune:2;
		uint64_t txfslstune:4;
		uint64_t txvreftune:4;
		uint64_t txrisetune:1;
#endif
	} cn56xx;
	struct cvmx_usbnx_usbp_ctl_status_cn50xx cn56xxp1;
} cvmx_usbnx_usbp_ctl_status_t;

#ifdef CONFIG_USB_DWC_OTG
#define	CVMX_USBNX_DMA0_INB_CHN0(id) \
	(CVMX_ADD_IO_SEG(0x00016F0000000818ull) + ((id) & 1) * 0x100000000000ull)

#define	CVMX_USBNX_DMA0_OUTB_CHN0(id) \
	(CVMX_ADD_IO_SEG(0x00016F0000000858ull) + ((id) & 1) * 0x100000000000ull)

#define	CVMX_USBCX_GOTGCTL(id) \
	(CVMX_ADD_IO_SEG(0x00016F0010000000ull) + ((id) & 1) * 0x100000000000ull)

#define	CVMX_USBNX_CLK_CTL(id) \
	(CVMX_ADD_IO_SEG(0x0001180068000010ull) + ((id) & 1) * 0x10000000ull)

static inline int octeon_usb_is_ref_clk(void)
{
	return 1;
}
#endif /* CONFIG_USB_DWC_OTG */

#endif
