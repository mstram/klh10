/* KLH10 CPU state and register definitions
*/
/* $Id: kn10def.h,v 2.5 2002/05/21 10:02:31 klh Exp $
*/
/*  Copyright � 1992, 1993, 2001 Kenneth L. Harrenstien
**  All Rights Reserved
**
**  This file is part of the KLH10 Distribution.  Use, modification, and
**  re-distribution is permitted subject to the terms in the file
**  named "LICENSE", which contains the full text of the legal notices
**  and should always accompany this Distribution.
**
**  This software is provided "AS IS" with NO WARRANTY OF ANY KIND.
**
**  This notice (including the copyright and warranty disclaimer)
**  must be included in all copies or derivations of this software.
*/
/*
 * $Log: kn10def.h,v $
 * Revision 2.5  2002/05/21 10:02:31  klh
 * Fix SYNCH implementation of KL timebase
 *
 * Revision 2.4  2002/03/21 09:50:08  klh
 * Mods for CMDRUN (concurrent mode)
 *
 * Revision 2.3  2001/11/10 21:28:59  klh
 * Final 2.0 distribution checkin
 *
 */

#ifndef KN10DEF_INCLUDED
#define KN10DEF_INCLUDED 1

#ifdef RCSID
 RCSID(kn10def_h,"$Id: kn10def.h,v 2.5 2002/05/21 10:02:31 klh Exp $")
#endif

#include "osdsup.h"	/* Ensure any OS-dependent stuff is available */

#include "word10.h"	/* Basic PDP-10 word definitions & facilities */

#ifndef EXTDEF
# define EXTDEF extern	/* Default is to declare (not define) vars */
#endif

/* INSBRK - Sum of all interrupt flags.  This is invoked whenever something
** (PI interrupt, trap, device I/O, etc) wants to break out of the normal
** instruction execution loop once the current instruction is done.
** See the INTF macros in osdsup.h.
*/
#define INSBRK_INIT()	INTF_INIT(cpu.mr_insbreak)
#define INSBRKSET()	INTF_SET(cpu.mr_insbreak)
#define INSBRKTEST()	INTF_TEST(cpu.mr_insbreak)
#define INSBRK_ACTBEG()	INTF_ACTBEG(cpu.mr_insbreak)
#define INSBRK_ACTEND()	INTF_ACTEND(cpu.mr_insbreak)

/* Accumulator reference definitions */

#define AC_BITS 4
#define AC_N (1<<AC_BITS)
#define AC_MASK (AC_N-1)
#define AC_0 0
#define AC_17 017		/* Last AC - same as AC_MASK */

/* For speed, the current AC block is always kept in a fixed place at the
** start of the cpu struct.  A different array contains all ac blocks,
** including the "shadow" ac block; when the current ac block is changed,
** the acs are copied back into this array and the newly selected block
** restored to the "current ACs".
**	The various pointers to specific blocks are needed for making
** mapped memory references, which may or may not involve the previous
** context ACs.  Whenever the reference is to the current ACs the pointer
** is always to the actual cpu.acs block, not its shadow in cpu.acblks.
*/

#define ACBLKS_N 8	/* Max # of AC blocks supported */

/* AC blocks are allocated as cpu.acblks in the cpu struct farther on. */
typedef w10_t *acptr_t;		/* Pointer to an AC */
typedef struct {
	w10_t ac[AC_N];
	} acblk_t;		/* AC block - 16 words */

struct acregs {		/* Pointers to specific AC blocks */
	acptr_t cur,	/* Current AC block, always points to cpu.acs */
		prev,	/* Previous context AC block */
		xea,	/*	AC block for EA calc */
		xrw,	/*	AC block for mem access */
		xbea,	/*	AC block for byte ptr EA calc */
		xbrw;	/*	AC block for byte ptr mem access */
};

/* Macro to simplify the setting of new AC block mappings
*/
#define acmap_set(new, old) (		\
    (cpu.acblk.xea = cpu.acblk.xrw =	\
	cpu.acblk.xbea = cpu.acblk.xbrw = \
	cpu.acblk.cur = (new)),		\
    (cpu.acblk.prev = (old)) )

/* Fundamental macro to get pointer to AC, given mapping and AC #.
**	No arg checking is done; the AC # MUST be (AC_0 <= # <= AC_17).
*/
#define ac_xmap(a,p)	(&((p)[(a)]))

/* Derive new AC # given offset - implements wraparound of AC # */
#define ac_off(a,i)	(((a)+(i))&AC_MASK)

/* Standard macro used for all AC references (NOT indexing or fast memory)
*/
#define ac_map(a)	ac_xmap((a),cpu.acs.ac)
#define ac_mapd(a)	((dw10_t *)ac_map(a))	/* Only use if safedouble */

/* Given pointer to AC entry, convert to and from words and halfwords.
** These work for either kind of AC reference.
*/
#define ac_pget(p)	(*(p))
#define ac_pgetlh(p)	LHPGET(p)
#define ac_pgetrh(p)	RHPGET(p)
#define ac_pset(p,w)	(*(p) = (w))
#define ac_psetlh(p,h)	LHPSET(p,h)
#define ac_psetrh(p,h)	RHPSET(p,h)

/* Define standard AC read/write operations
**	(again, NOT for use with indexing or fast-memory refs)
*/
#define ac_get(a)	(*ac_map(a))
#define ac_getlh(a)	LHPGET(ac_map(a))
#define ac_getrh(a)	RHPGET(ac_map(a))

#define ac_set(a,w)	(*ac_map(a) = (w))
#define ac_setlh(a,v)	LHPSET(ac_map(a), (v))
#define ac_setrh(a,v)	RHPSET(ac_map(a), (v))
#define ac_setlrh(a,l,r) XWDPSET(ac_map(a),l,r)

/* Macros to fetch AC contents when indexing or as fast memory.  These
** require an AC block mapping pointer in addition to the AC #.
*/
#define ac_xget(a,p)	(*ac_xmap(a,p))
#define ac_xgetrh(a,p)	RHPGET(ac_xmap(a,p))

/* Facilities for handling double-word AC refs.
**	Note that an AC # of AC_17 is unsafe and always indicates a wrapped
**	double-word of AC17 and AC0.
**
** NOTE: ac_dpget and ac_dget CANNOT be used in the same way as ac_get()
**	(ie the value cannot be used in an assignment), because for the slow
**	case there is no way to assemble an anonymous structure value.
**
** p.s. The reason for the stupid "value" of integer 0 is because some
**	so-called C compilers botch conditional expressions that have
**	type void.  Better ones just optimize it away.
*/
#define ac_issafedouble(a) ((a)!=AC_17)	/* TRUE unless last AC */

#define ac_dpget(dp,a) ( ac_issafedouble(a) \
		? ((*(dp) = *ac_mapd(a)),0) \
		: ((dp)->w[0] = ac_get(AC_17), (dp)->w[1] = ac_get(AC_0), 0 ))

#define ac_dpset(dp,a) ( ac_issafedouble(a) \
		? ((*ac_mapd(a) = *(dp)), 0) \
		: (ac_set(AC_17, (dp)->w[0]), ac_set(AC_0, (dp)->w[1]), 0) )
#define ac_dget(a,d) (ac_issafedouble(a) \
		? (((d) = *ac_mapd(a)),0) \
		: ((d).w[0] = ac_get(AC_17), (d).w[1] = ac_get(AC_0), 0 ))
#define ac_dset(a,d) (ac_issafedouble(a) \
		? ((*ac_mapd(a) = (d)), 0) \
		: (ac_set(AC_17, (d).w[0]), ac_set(AC_0, (d).w[1]), 0) )

/* Memory definitions */

/* Get virtual addressing and virtual memory definitions */

#include "kn10pag.h"		/* Include pager definitions */

/* Well, they used to take up a few pages here... */

/* Program Counter (PC) macros
**
** All references to and manipulations of PC should use a PC_ macro.
** This permits relatively straightforward experimentation with
** different PC implementations.
*/
/* Likewise, the type used to hold a PC may be different from that
** for a virtual address, although for the time being they are still
** the same.
** NOTE: Later when questing for speed, attention needs to be given to the
**	cases of mr_bkpt and klh10.c's setting of pcva_t variables.
**
** NOTE also: possibility of a vm_PCfetch() macro to fetch c(PC)
**	specially (knows PC is local, no vaddr_t conversion, etc).
*/
typedef vaddr_t pcva_t;		/* Type of PC virt addr */
typedef int     pcinc_t;	/* Type of all instruction routines */
# define PCINC_0 0		/* Instr jumped */
# define PCINC_1 1		/* Instr normal */
# define PCINC_2 2		/* Instr skipped */

/* Macros to obtain a PC attribute.
*/
#define PC_VADDR  cpu.mr_PC		/* Get PC as a vaddr_t */
#define PC_30     va_30(cpu.mr_PC)	/* Full 30-bit PC value */
#define PC_SECT   va_sect(cpu.mr_PC)	/* PC section # (right-justified) */
#define PC_SECTF  va_sectf(cpu.mr_PC)	/* PC section as a field value */
#define PC_INSECT va_insect(cpu.mr_PC)	/* PC in-section part */
#define PC_ISEXT  PC_SECTF		/* TRUE if PC in NZ section */
#define PC_PAGE   va_xapage(cpu.mr_PC)	/* Full XA page # of PC */
#define PC_PAGOFF va_pagoff(cpu.mr_PC)
#define PC_AC	  va_ac(cpu.mr_PC)
#if KLH10_EXTADR
# define PC_ISACREF ((cpu.mr_PC & (VAF_SBAD|VAF_NNOTA))==0)
#else
# define PC_ISACREF ((cpu.mr_PC & VAF_NNOTA)==0)
#endif

/* Macros to set or change the PC.
**	It would be possible to optimize the SETs if all PC refs
**	(vm_fetch in particular) always knew it was local-format.
*/
#define PC_ADD(n)  va_ladd(cpu.mr_PC,n)	/* Add n to PC (always local) */
#define PC_SET30(a) va_lmake30(cpu.mr_PC,a)    /* Set PC from int w/o JPC */
#define PC_SET(va)  va_lmake30(cpu.mr_PC,va_30(va)) /* " from vaddr_t */

#if KLH10_JPC		/* Take a jump.  Special so can set JPC if desired */
# define PC_JUMP(e) (cpu.mr_jpc = cpu.mr_PC, PC_SET(e))
#else
# define PC_JUMP(e) PC_SET(e)
#endif

/* Special macro to increment PC based on instruction execution.
** This must NOT be replaced by an expression such as
**		(PC = (PC+(x)) & H10MASK)
** because C can't guarantee whether PC or (x) is evaluated first!
*/
#if KLH10_EXTADR
# define PC_ADDXCT(x) { register pcinc_t i__ = (x); if (i__) PC_ADD(i__); }
#else
# define PC_ADDXCT(x) { register pcinc_t i__ = (x); cpu.mr_PC += i__; } /* gcc4 fix by Roch Kusiak */
#endif

/* Macros for putting PC into a word.
*/
#define PC_TOWORD(w)  LRHSET(w, PC_SECT, PC_INSECT)	  /* Put PC in wd */
#define PC_XWDPC1(w,l) LRHSET(w,l,(cpu.mr_PC+1)&H10MASK)  /* <l>,,<PC+1> */
#define PC_1TOWORD(w) PC_XWDPC1(w, PC_SECT)		  /* <sect>,,<PC+1> */
#define PC_F1WORD(w)  PC_XWDPC1(w, cpu.mr_pcflags)	  /* <flags>,,<PC+1> */

/* Macro to put "PC-word" in word: PC+1 plus flags if sect 0
*/
#if KLH10_EXTADR
# define PC_1WORD(w) (PC_ISEXT ? PC_1TOWORD(w) : PC_F1WORD(w))
#else
# define PC_1WORD(w) PC_F1WORD(w)
#endif


/* Processor Map change macros
**	This macro should be invoked whenever something happens to
**	change the address space mapping, either the page map or
**	AC block selection.
*/
#if KLH10_PCCACHE
# define PCCACHE_RESET() (cpu.mr_cachevp = NULL)
#else
# define PCCACHE_RESET()
#endif

/* Processor PC Flag (PCF) macros */

#define PCFSET(f)	(cpu.mr_pcflags |= (f))
#define PCFTRAPSET(f)	(cpu.mr_pcflags |= (f), INSBRKSET())
#define PCFCLEAR(f)	(cpu.mr_pcflags &= ~(f))
#define PCFTEST(f)	(cpu.mr_pcflags & (f))

/* Macro for OP10 facilities (from kn10ops) to use for flag setting */
#define OP10_PCFSET(f) (cpu.mr_pcflags |= (f), \
	(((f)&(PCF_TR1|PCF_TR2)) ? INSBRKSET() : 0))


/* PDP-10 processor flags
**	These defs are the left half bit values of PDP-10 PC flags.
**	Unless otherwise specified, all exist on all models.
*/
#define PCF_ARO	0400000	/* Arithmetic Overflow (or Prev Ctxt Public) */
#define PCF_CR0	0200000	/* Carry 0 - Carry out of bit 0 */
#define PCF_CR1	0100000	/* Carry 1 - Carry out of bit 1 */
#define PCF_FOV	 040000	/* Floating Overflow */
#define PCF_FPD	 020000	/* First Part Done */
#define PCF_USR	 010000	/* User Mode */
#define PCF_UIO	  04000	/* User In-Out (or Prev Ctxt User) */
#define PCF_PUB	  02000	/* [KL/KI]    Public Mode */
#define PCF_AFI	  01000	/* [KL/KI]    Addr Failure Inhibit */
#define PCF_TR2	   0400	/* [KL/KI/KS] Trap 2 (PDL overflow) */
#define PCF_TR1	   0200	/* [KL/KI/KS] Trap 1 (Arith overflow) */
#define PCF_FXU	   0100	/* Floating Exponent Underflow */
#define PCF_DIV	    040	/* No Divide */
#define PCF_MASK (H10MASK&~037)	/* Can't mung low 5 bits, save for I(X) */

/* Duplicate flag meanings in exec mode */
#define PCF_PCP	PCF_ARO	/* [KL/KI]    Previous Context Public */
#define PCF_PCU	PCF_UIO	/* [KL/KI/KS] Previous Context User */
#if KLH10_ITS_1PROC
# define PCF_1PR PCF_AFI	/* KS10 One-proceed flag - re-use AFI */
#else
# define PCF_1PR 0
#endif

/* Instruction opcode macros and defs */

/* These cannot come earlier because they depend on some typedefs such
** as vaddr_t and pcinc_t.
*/

#include "opdefs.h"	/* PDP-10 instruction opcodes and declarations */
			/* Includes IW_ facilities */


/* Processor/microcode configuration */

/* APRID bit definitions.
**	It appears that some monitors actually pay attention to these bits.
**	While the KS and KL have superficially similar APRID fields, the
**	meanings of the bits are completely different, so each needs its
**	own specific definitions.
*/

/* KS notes:
**	The PRM originally had no option bits defined; all of the options
** defined below were later additions.
*/
#if KLH10_CPU_KS
# define AIF_UCOPT	0777000 /* LH: Microcode options field */
# if KLH10_SYS_ITS
#  define AIF_ITS	 020000	/*    ITS ucode [ITS KS10 only!] */
# else
#  define AIF_INHCST	0400000	/*    "Inhibit CST update" supported */
#  define AIF_NOCST	0200000	/*    No CST */
#  define AIF_SEX	0100000	/*    "Exotic uCode" (ie non-standard) */
#  define AIF_UBLT	 040000	/*    BLTUB, BLTBU supported */
#  define AIF_KIPG	 020000	/*    KI paging (old T10) */
#  define AIF_KLPG	 010000	/*    KL paging (T20) */
# endif
# define AIF_UCVER	   0777	/* LH: ucode version # field */
# define AIF_HWOPT	0700000	/* RH: Hardware options field */
# define AIF_SNO	 077777	/* RH: Processor serial # (15 bits!) */

/* Default the ucode version and APR serial number */

# ifndef KLH10_APRID_UCVER
#  if KLH10_SYS_ITS
#   define KLH10_APRID_UCVER 0262	/* Last ITS KS ucode version */
#  else
#   define KLH10_APRID_UCVER 0130	/* Last DEC KS ucode version? */
#  endif
# endif
# ifndef KLH10_APRID_SERIALNO
#  define KLH10_APRID_SERIALNO 4097	/* This is popular for some reason */
# endif				/* (maybe cuz an impossible SN for a KL) */

#endif /* KS */


/* KL notes:
**	Most of the KL bits are documented in the PRM, but 
** AIF_KLB, AIF_PMV, and AIF_MCA were later additions.
**	When ITS ran on a KL10A, it used the same bit as the KS10 (AIF_ITS)
** to indicate it was non-standard ucode.  But there's no need to
** resurrect MC, so don't worry about it.
*/
#if KLH10_CPU_KL
# define AIF_UCOPT	0777000 /* LH: Microcode options field */
# define AIF_T20	0400000	/*  0  UCode supports T20 paging */
# define AIF_EXA	0200000	/*  1  uCode supports Extended Addressing */
# define AIF_SEX	0100000	/*  2  "Exotic uCode" (ie non-standard) */
# define AIF_KLB	 040000	/*  3  KL10B CPU (?) */
# define AIF_PMV	 020000	/*  4  PMOVE/PMOVEM */
# define AIF_MCAOLD	 010000	/*  5  TOPS-20 R5 microcode MCA25 keep bit */
# define AIF_VER	   0777	/* LH: ucode version # field */

# define AIF_HWOPT	0770000	/* RH: Hardware options field */
# define AIF_50H	0400000	/*  18 50Hz power */
# define AIF_CCA	0200000	/*  19 Cache */
# define AIF_CHN	0100000	/*  20 "Channel" - has RH20s? */
# define AIF_KLX	 040000	/*  21 Extended KL10, else Single-section KL */
# define AIF_OSC	 020000	/*  22 "Master Oscillator" (golly) */
# define AIF_MCA	 010000	/*  23 MCA25 Keep Bit (per MCA25 doc p.A-10) */
# define AIF_SNO	  07777	/* RH: Hardware Serial Number (only 12 bits) */

/* Default the ucode version and APR serial number */

# ifndef KLH10_APRID_UCVER
#  if KLH10_SYS_ITS
#   define KLH10_APRID_UCVER 02--	/* Last ITS KL ucode version? */
#  else
#   define KLH10_APRID_UCVER 0442	/* Last DEC KL ucode version? */
#  endif
# endif
# ifndef KLH10_APRID_SERIALNO
#  define KLH10_APRID_SERIALNO 3600	/* Well, what else to use? */
# endif

#endif /* KL */

/* APR "device" definitions
*/

struct aprregs {
	int aprf_set;		/* APR flag settings & chan # */
	int aprf_ena;		/* APR flags enabled for interrupt */
	int aprf_lev;		/* APR level bit to interrupt on */
};

/* APR device flags.  Note these all fit within 16 bits on the KS.
**	The KL is a bit messier but can still be managed.
*/
#if KLH10_CPU_KL
# define APRW_IORST	0200000	/* 19 W: Clear all external I/O devices */
# define APRR_SWPBSY	0200000	/* 19 R: Sweep busy */
#endif
#define APRW_ENA	0100000	/* 20 Enable ints on selected flags */
#define APRW_DIS	 040000	/* 21 Disable ints on selected flags */
#define APRW_CLR	 020000	/* 22 Clear flags */
#define APRW_SET	 010000	/* 23 Set flags */
#define APRF_MASK	  07760	/*    Flag mask */
#if KLH10_CPU_KL
# define APRF_SBUS	  04000	/* 24 S-Bus Error */
# define APRF_NXM	  02000	/* 25 No Memory (locks ERA) */
# define APRF_IOPF	  01000	/* 26 IO Page Failure */
# define APRF_MBPAR	   0400	/* 27 MB Parity (locks ERA) */
# define APRF_CDPAR	   0200	/* 28 Cache Dir Parity */
# define APRF_ADPAR	   0100	/* 29 Address Parity (locks ERA) */
# define APRF_PWR	    040	/* 30 Nuclear Parity (ok, Power Failure) */
# define APRF_SWPDON	    020	/* 31 Sweep Done */
#elif KLH10_CPU_KS
# define APRF_24	  04000	/* 24 Unused? */
# define APRF_INT80	  02000	/* 25 Interrupt 8080 FE when set */
# define APRF_PWR	  01000	/* 26 Power failure */
# define APRF_NXM	   0400	/* 27 Non-ex memory */
# define APRF_BMD	   0200	/* 28 Bad Memory data */
# define APRF_ECC	   0100	/* 29 Corrected memory data */
# define APRF_TIM	    040	/* 30 Timer Interval done */
# define APRF_FEINT	    020	/* 31 Interrupt from 8080 FE */
#endif
#define APRR_INTREQ	    010	/* Something's requesting an int */
#define APRF_CHN	     07	/* Mask for APR channel */


/* PI System definitions
**
** Terminology note: ITS still uses the word "channel" to mean the same thing
** as what DEC now calls "level".
*/

/* PI device flags, written by CONO PI, (KS: WRPI)
**	Note all fit within 16 bits, except for high-order KL bits that
**	aren't really part of the PI system.
*/
#if KLH10_CPU_KL
# define PIF_WEPADR	0400000	/* Write Even Parity - Address */
# define PIF_WEPDAT	0200000	/* Write Even Parity - Data */
# define PIF_WEPDIR	0100000	/* Write Even Parity - Directory */
#endif
#define PIW_LDRQ	020000	/* Drop IRQs on selected levels */
#define PIW_CLR		010000	/* Clear PI system */
#define PIW_LIRQ	04000	/* Initiate interrupt on selected levels */
#define PIW_LON		02000	/* Turn on selected levels (enable) */	
#define PIW_LOFF	01000	/* Turn off   "       "   (disable) */
#define PIW_OFF		0400	/* Turn off PI system */
#define PIW_ON		0200	/* Turn on  "     "   */
#define PILEVS		0177	/* Mask for all PI level select bits */
#define PILEV1		0100	/* Bits used to select particular PI levels */
#define PILEV2		 040
#define PILEV3		 020
#define PILEV4		 010
#define PILEV5		  04
#define PILEV6		  02
#define PILEV7		  01

/* Flags read by CONI PI, (KS: RDPI)
**	LH 0177 bits contain levels with program requests active (preq)
**	RH 0177 bits contain levels turned on (enabled)
**	RH 0177<<8   contain levels being held (PI In Progress)
*/
#define PIR_PIPSHIFT 8	/* Shift arg to find PIP bits in RH */
#define PIR_ON	PIW_ON	/* PI system on */

#if KLH10_CPU_KL
	/* PI Function Word fields */
#define PIFN_ASP	0700000	/* LH: Address space to use for fns 4 & 5 */
#define  PIFN_ASPEPT	 0	/*	Exec Process Table */
#define  PIFN_ASPEVA	0100000	/*	Exec virtual address */
#define  PIFN_ASPPHY	0400000	/*	Physical address */

#define PIFN_FN		 070000	/* LH: Function code */
#define  PIFN_F0	  0	/*	Internal device or zero word */
#define  PIFN_FSTD	 010000	/*	Standard interrupt EPT+(40+2N) */
#define  PIFN_FVEC	 020000	/*	Vector interrupt (dev/adr) */
#define  PIFN_FINC	 030000	/*	Increment */
#define  PIFN_FEXA	 040000	/*	DTE20 Examine */
#define  PIFN_FDEP	 050000	/*	DTE20 Deposit */
#define  PIFN_FBYT	 060000	/*	DTE20 Byte Transfer */
#define  PIFN_FAOS	 070000	/*	IPA20-L (NIA20, CI) inc & return val */

#define PIFN_Q		  04000	/* LH: Q bit */
#define PIFN_DEV	  03600	/* LH: Physical Device # */
#define PIFN_LHADR	    037	/* LH: High 5 bits of address */
				/* RH: Low 18 bits of address */

#endif /* KL */

struct piregs {
	int pisys_on;	/* PIR_ON bit set if PI system on, else 0 */
	int pilev_on;	/* Levels active (enabled) - can take ints */
	int pilev_pip;	/* Levels holding ints (PI in Progress) */

	int pilev_preq;	/* Levels to initiate prog reqs on */
			/* (these take effect even if pilev_on is off!) */

	int pilev_dreq;	/* Device PI requests outstanding */
	int pilev_aprreq;	/* "Devices" in APR */
#if KLH10_CPU_KL
	int pilev_timreq;	/* Interval timer PI request */
	int pilev_rhreq;	/* RH20 device requests */
	int pilev_dtereq;	/* DTE20 device requests */
	int pilev_diareq;	/* DIA20 device requests */

#elif KLH10_CPU_KS
	int pilev_ub1req;	/* Devices on UBA #1 */
	int pilev_ub3req;	/* Devices on UBA #3 */
#endif
};

/* Defined & initialized in kn10cpu.c PI code; later make EXTDEFs */
extern int pilev_bits[];	/* Bit masks indexed by PI level number */
extern unsigned char pilev_nums[]; /* PI level nums indexed by PI bit mask */

/* Timing system definitions */

#if KLH10_CPU_KL

/* Bits for CONO MTR,  (those marked [I] are also read on CONI)
*/
# define MTR_SUA	0400000	/*     Set Up Accounts */
# define MTR_AEPI	 040000	/* [I] Acct: Executive PI Account */
# define MTR_AENPI	 020000	/* [I] Acct: Executive Non-PI Account */
# define MTR_AON	 010000	/* [I] Acct: Turn on */
# define MTR_TBON	  04000	/* [I] Time Base: turn on */
# define MTR_TBOFF	  02000	/*     Time Base: turn off */
# define MTR_TBCLR	  01000	/*     Time Base: clear */
# define MTR_ICPIA	     07	/* [I] Interval Counter: PI Assignment */

/* Bits for CONO TIM,
*/
# define TIM_WCLR	0400000	/* Clear Interval Counter */
# define TIM_ON		 040000	/* Turn on Interval Counter */
# define TIM_WFCLR	 020000	/* Clear Interval Flags */
# define TIM_RDONE	 020000	/* Interval Done */
# define TIM_ROVFL	 010000	/* Interval Overflow */
# define TIM_PERIOD	  07777	/* Interval Period */
/* Note: LH of CONI has current interval counter in its TIM_PERIOD field */

/* According to the PRM, all hardware counters are 16 bits except
** the interval counter (12 bits).
**
** All "doubleword" counts are 59-bit unsigned quantities with the
** hardware counter at the low end.
** The representation in a PDP-10 doubleword has the high 36 bits
** in the high-order word, and the low 23 in bits 1-23 of the low word,
** with bit 0 zero.
**	The low 12 bits of the doubleword are reserved for
** extensibility to future faster machines (!).
*/

#endif /* KL */


struct timeregs {

#if KLH10_CPU_KS
	w10_t tim_intreg;	/* Interval Time reg set by WRINT */

	uint32 tim_base[3];	/* 71-bit time base, native form */
	dw10_t wrbase;		/* Time Base of last WRTIM */
	osrtm_t osbase;		/* OS Time Base of last WRTIM */

#elif KLH10_CPU_KL
	unsigned mtr_flgs;	/* MTR condition flags */
	int mtr_on;		/* TRUE if accounting on */
	int mtr_ifexe;		/*   TRUE to include exec non-PI */
	int mtr_ifexepi;	/*   TRUE to include exec PI */
	uint32 mtr_eact;	/* Execution acct hardware cntr */
	uint32 mtr_mact;	/* Mem ref acct hardware cntr */

	int tim_on;		/* TRUE if interval counter on */
	int tim_lev;		/* PI level bit to interrupt on, if any */
	unsigned int tim_flgs;	/* TIM condition flags and PIA */
	unsigned int tim_intper;	/* Interval counter period */
	unsigned int tim_intcnt;	/* Interval countdown */

	int tim_tbon;		/* Time base on/off flag */
# if KLH10_RTIME_SYNCH
	/* Time base - virt usec since last RDTIME */
	uint32 tim_ibased;	/* usec due to done interval interrupt */
	uint32 tim_ibaser;	/* usec due to rem ticks since last interval */
# elif KLH10_RTIME_OSGET
	osrtm_t tim_osbase;	/* OS Time Base of last WRTIME reset */
# endif

	uint32 tim_perf;	/* Performance analysis hardware cntr */
#endif
};

#define TIMEBASEFILE "APR.TIMEBASE"	/* Change this name later */

/* Determine whether any clock countdown is needed at all */
#define IFCLOCKED (KLH10_RTIME_SYNCH || KLH10_ITIME_SYNCH || KLH10_QTIME_SYNCH)
#define IFPOLLED (!KLH10_CTYIO_INT || !KLH10_IMPIO_INT)

#include "kn10clk.h"		/* New clock stuff */


/* Stuff needed for ITS pager quantum counter, used by both kn10cpu and kn10pag.
   See time code comments in KN10CPU.
*/
#if KLH10_SYS_ITS
# if KLH10_QTIME_SYNCH
#  define quant_unfreeze(q) ((q) + (CLK_USEC_UNTIL_ITICK()<<2))
#  define quant_freeze(q)   ((q) - (CLK_USEC_UNTIL_ITICK()<<2))
# elif KLH10_QTIME_OSREAL || KLH10_QTIME_OSVIRT
   extern int32 quant_freeze(int32), quant_unfreeze(int32);
# endif
#endif

/* UPT/EPT definitions
*/

/* UPT Offsets
**	ITS: In non-time sharing and at clock level, UPT=EPT.
*/

#if KLH10_PAG_KI
# define UPT_PMU 0	 /* 000-377: T10 User page map table (pages 0-777) */
# define UPT_PME340 0400 /* 400-417: T10 Exec page map table (pages 340-377) */
#endif

#if KLH10_CPU_KI
# define UPT_PFT 0420	/* User page failure trap instr */
#elif KLH10_CPU_KLX
# define UPT_LUU 0420	/* 30-bit addr of LUUO dispatch block */
#endif
#define UPT_TR1	0421	/* User mode arith ovfl trap. */
#define UPT_TR2	0422	/* User mode pdl ov trap. */
#define UPT_TR3	0423	/* User mode trap 3 in non-one-proceed microcode. */

    /* Although MUUO trapping is not logically related to paging,
    ** in practice the MUUO algorithm is a function of the pager being
    ** used, as well as the CPU, so the different versions here are
    ** selected likewise.  These categories correspond to those in
    ** PRM 2-126 (fig 2.3)
    */
#if KLH10_CPU_KLX || (KLH10_CPU_KS && KLH10_PAG_KL)	/* KLX or T20 KS */
# define UPT_UUO 0424	/* MUUO PC flags, opcode, A, and PCS stored here. */
# define UPT_UPC 0425	/* MUUO old PC */
# define UPT_UEA 0426	/* MUUO Effective Address */
# define UPT_UCX 0427	/* MUUO process context (from RDUBR) */
#elif KLH10_CPU_KL0 && KLH10_PAG_KL			/* T20 KL0 */
# define UPT_UUO 0425	/* MUUO opcode, A, PC and Eff Addr */
# define UPT_UPC 0426	/* MUUO old PC and flags */
# define UPT_UCX 0427	/* MUUO process context (from RDUBR) */
#elif (KLH10_CPU_KL0 || KLH10_CPU_KS) && (KLH10_PAG_KI || KLH10_PAG_ITS)
# define UPT_UUO 0424	/* MUUO opcode, A, PC and Eff Addr */
# define UPT_UPC 0425	/* MUUO old PC and flags */
# define UPT_UCX 0426	/* MUUO process context (from RDUBR) */
#elif KLH10_CPU_KI
# define UPT_UUO 0424	/* MUUO opcode, A, PC and Eff Addr */
# define UPT_UPC 0425	/* MUUO old PC and flags */
#endif


#define UPT_UEN	0430	/* MUUO new PC - Exec mode, no trap */
#define UPT_UET	0431	/* MUUO new PC - Exec mode, trap instr */
#if KLH10_SYS_ITS && KLH10_CPU_KS
# define UPT_1PO 0432	/* One-proceed old PC stored here (if 1proc ucode) */
# define UPT_1PN 0433	/* One-proceed new PC obtained from here (if " )   */
#elif KLH10_CPU_KI || KLH10_CPU_KL
# define UPT_USN 0432	/* MUUO new PC - Supv mode, no trap */
# define UPT_UST 0433	/* MUUO new PC - Supv mode, trap instr */
#endif
#define UPT_UUN	0434	/* MUUO new PC - User mode, no trap */
#define UPT_UUT	0435	/* MUUO new PC - User mode, trap instr */
#if KLH10_CPU_KI || KLH10_CPU_KL
# define UPT_UPN 0436	/* MUUO new PC - Public mode, no trap */
# define UPT_UPT 0437	/* MUUO new PC - Public mode, trap instr */
#endif


#if KLH10_CPU_KS || KLH10_CPU_KL
# if KLH10_PAG_KI	/* T10 paging */
#  define UPT_PFW 0500	/* Page Fail Word */
#  define UPT_PFO 0501	/* Page Fail Old PC+flags word */
#  define UPT_PFN 0502	/* Page Fail New PC+flags word */
# elif KLH10_PAG_KL	/* T20 paging */
#  define UPT_SC0 0540	/* T20 User Section 0 pointer */
#  if KLH10_CPU_KS || KLH10_CPU_KLX
#   define UPT_PFW 0500	/* Page Fail Word */
#   define UPT_PFF 0501	/* Page Fail Flags */
#   define UPT_PFO 0502	/* Page Fail Old PC */
#   define UPT_PFN 0503	/* Page Fail New PC */
#  elif KLH10_CPU_KL0
#   define UPT_PFW 0501	/* Page Fail Word */
#   define UPT_PFO 0502	/* Page Fail Old PC+flags word */
#   define UPT_PFN 0503	/* Page Fail New PC+flags word */
#  endif
# endif
#endif /* KS+KL */

#if KLH10_CPU_KL
# define UPT_PXT 0504	/* User Process Execution Time (doubleword) */
# define UPT_MRC 0506	/* User Memory Reference Count (doubleword) */
#endif


/* EPT Locations */

#if KLH10_CPU_KL
# define EPT_CL0 0	/* Channel Logout Area 0 (of 8) (quadword) */
#endif
#if KLH10_CPU_KI
# define EPT_UUO 040	/* Exec LUUO stored here */
# define EPT_UUH 041	/* Exec LUUO handler instruction */
#endif
#define EPT_PI0	040	/* PI0LOC+2*PICHN = Addr of instr pair for PICHN. */
			/* Note KL has a PI0!  Otherwise, locs */
			/* actually used are 42-57 inclusive. */
#if KLH10_CPU_KL
# define EPT_CB0 060	/* Channel Block Fill Word 0 (of 4) */
#endif

#if KLH10_CPU_KS
# define EPT_UIT 0100	/* EPT_UIT+N contains addr of the interrupt */
			/* table for unibus adapter N.  Only */
			/* adapters 1 and 3 ever exist. */
#endif /* KS */

#if KLH10_CPU_KL
# define EPT_DT0 0140	/* DTE20 Control Block 0 (of 4) (8 wds each) */
#endif
#if KLH10_PAG_KI
# define EPT_PME400 0200 /* 200-377: T10 Exec page map table (pages 400-777) */
#endif
#if KLH10_CPU_KLX
# define EPT_LUU 0420	/* Exec mode 30-bit LUUO block location */
#elif KLH10_CPU_KI
# define EPT_PFT 0420	/* Exec page failure trap instr */
#endif
#define EPT_TR1	0421	/* Exec mode arith ovfl trap. */
#define EPT_TR2	0422	/* Exec mode pdl ov trap. */
#define EPT_TR3	0423	/* Exec mode trap 3 (1 proceed?). */

#if KLH10_SYS_ITS
/*
    In the ITS microcode the three words used to deliver a page fail are
    determined from the current interrupt level.  At level I, the page fail
    word is stored in EPTPFW+<3*I>, the old PC is stored in EPTPFO+<3*I>,
    and the new PC is obtained from EPTPFN+<3*I>.  If no interrupts are in
    progress we just use EPTPFW, EPTPFO and EPTPFN.
*/
#define EPT_PFW	0440	/* Page fail word stored here. */
#define EPT_PFO	0441	/* Page fail old PC stored here. */
#define EPT_PFN	0442	/* Page fail new PC obtained from here. */
#endif	/* ITS */

#if KLH10_CPU_KL
/* The PRM doesn't mention this, but it appears that TOPS-20 (at least) uses
** EPT locations 444-457 inclusive as a special communication area with
** the Master DTE.  See dvdte.h for more details.
*/
#endif

#if KLH10_CPU_KL
# define EPT_TBS 0510	/* Time Base (doubleword) */
# define EPT_PRF 0512	/* Performance Analysis Counter (doubleword) */
# define EPT_ICI 0514	/* Interval Counter Interrupt Instruction */
#endif

#if KLH10_PAG_KL
# define EPT_SC0 0540	/* T20 Exec Section 0 pointer */
#endif
#if KLH10_PAG_KI
# define EPT_PME0 0600	/* 600-757: T10 Exec page map table (pages 0-337) */
			/* (note pages 340-377 are in UPT!) */
#endif

/* FE (console) stuff needed for interaction */

/* Halt codes returned to FE when KLH10 CPU stops.
*/
enum haltcode {	/* Must not be zero */
	HALT_PROG=1,	/* Program halt (JRST 4,) */
	HALT_FECTY,	/* FE Console interrupt */
	HALT_FECMD,	/* FE Console command ready to execute */
	HALT_BKPT,	/* Hit breakpoint */
	HALT_STEP,	/* Single-Stepping */
	HALT_EXSAFE,	/* Badness in exec mode */
	HALT_PANIC	/* Panic - internal error, bad state */
};

/* FE command mode */
enum femode {		/* runnable running ctyon Description */
    FEMODE_CMDCONF,	/* 0        0       0     not inited, cmd i/o */
    FEMODE_CMDHALT,	/* 1        0       0     halted,  cmd i/o */
    FEMODE_CMDRUN,	/* 1        1       0     running, cmd i/o */
    FEMODE_CTYRUN	/* 1        1       1     running, CTY I/O */
};

struct feregs {
	enum femode fe_mode;	/* FE input handling mode */
	int fe_cmdready;	/* TRUE if FE cmd input buff ready to exeute */
	int fe_runenable;	/* TRUE to run KN10 during cmd input */
	int fe_intchr;		/* If non-zero, FE interrupt/cmd escape char */
	int fe_intcnt;		/* # times intchr seen before handled */
	int fe_ctyon;		/* TRUE if passing input to PDP-10 CTY */
	int fe_ctyinp;		/* # chars CTY input waiting, if any */
	int fe_debug;		/* TRUE to print debug info */
	int fe_ctydebug;	/* TRUE to print CTY debugging info */

#if KLH10_CPU_KS
	int fe_iowait;		/* # usec clock ticks to delay I/O response */
# if KLH10_CTYIO_ADDINT
	int cty_lastint;	/* # 1ms ticks after last output to give int */
	int cty_prevlastint;	/* Previous value of above */
	struct clkent *cty_lastclk;	/* Clock timer for last-output int */
# endif
#endif /* KLH10_CPU_KS */
};

#if KLH10_CPU_KS

/* FE <-> KS10 Communications Area locations */
#define FECOM_SWIT0	030	/* Simulated switch 0. Set by 8080 SH cmd. */
#define FECOM_KALIV	031	/* Keep Alive & Status. */
#define FECOM_CTYIN	032	/* CTY input. */
#define FECOM_CTYOT	033	/* CTY output. */
#define FECOM_KLKIN	034	/* KLINIK user input word (from 8080). */
#define FECOM_KLKOT	035	/* KLINIK user output word  (to 8080). */
#define FECOM_RHBAS	036	/* BOOT RH11 base address. */
#define FECOM_QNUM	037	/* BOOT Unit Number. */
#define FECOM_BOOTP	040	/* Magtape Boot Format and Slave Number. */

#endif /* KS */

#if KLH10_CPU_KL		/* Low core locs checked by T20 scheduler */
# define FECOM_SWIT0	030	/* SHLTW - Halt request if NZ, see SCHED.MAC */
	/*		020 */	/* SCTLW - Request word, various bits */
#endif

/* Global Machine state variables
**	This is a structure so references to the contents can all be made
**	as offsets from a base address (which may be in a register).  On
**	some architectures such as the SPARC this allows a 1-instruction
**	reference as opposed to the usual 3-instruction sequence for a
**	random global reference.
**	On other architectures it doesn't matter but doesn't hurt either.
*/

struct machstate {
	/* Current ACs; at start of struct to make indexing faster.
	** This block is swapped with cpu.acblks[n]
	** whenever the current AC block changes.
	*/
	acblk_t acs;
	int mr_acbcur;		/* AC block # of current acs above */

	/* General internal machine registers */
	h10_t mr_pcflags;	/* Current PC flags */
	pcva_t mr_PC;		/* Current virtual PC */
#if KLH10_PCCACHE
	vmptr_t mr_cachevp;	/* Current cached PC map pointer */
#endif
#if KLH10_JPC
	pcva_t mr_jpc;		/* Virtual PC of last jump instruction */
	pcva_t mr_ujpc;		/* Last JPC when user mode left */
	pcva_t mr_ejpc;		/* Last JPC when exec mode left */
#endif
#if KLH10_CPU_KS
	w10_t mr_hsb;		/* Halt Status Block base address */
#elif KLH10_CPU_KL
	vaddr_t mr_abk_addr;	/* Address Break virtual address */
	pagno_t mr_abk_pagno;	/* Page containing break address, -1 if none */
	pment_t *mr_abk_pmap;	/* Page map containing break address page */
	pment_t mr_abk_pmflags;	/* Saved page access flags & VMF_ACC */
	pment_t mr_abk_pmmask;	/* Mask to clear VMF_READ and/or VMF_WRITE */
	int mr_abk_cond;	/* Break condition flags, as follows: */
# define ABK_IFETCH 8		/*   Break on instruction fetch */
# define ABK_READ   4		/*   Break on read */
# define ABK_WRITE  2		/*   Break on write */
# define ABK_USER   1		/*   Break address is user (else exec) */
#elif KLH10_CPU_KI
	w10_t mr_adrbrk;	/* Address Break register */
#endif
	w10_t mr_aprid;		/* Processor ID */
	w10_t mr_ebr;		/* Executive Base Register */
	w10_t mr_ubr;		/* User Base Register */
	paddr_t mr_ebraddr, mr_ubraddr;	/* Word addresses for above */
	int mr_paging;		/* TRUE if paging & traps enabled */
	int mr_usrmode;		/* TRUE if in USER mode (else EXEC mode) */
	int mr_inpxct;		/* TRUE if executing PXCT operand */
#if KLH10_CPU_KLX
	pcva_t mr_pxctpc;	/* Saved PC if PXCT bit 12 set */
#endif
	h10_t mr_intrap;	/* NZ (Trap flags set) if XCTing trap instr */
#if KLH10_ITS_1PROC
	int mr_in1proc;		/* TRUE if executing one-proceed instr */
#elif KLH10_CPU_KI || KLH10_CPU_KL
	int mr_inafi;		/* TRUE if executing AFI instruction */
#endif
	int mr_injrstf;		/* TRUE if executing JRSTF/JEN */
	int mr_inpi;		/* TRUE if executing PI instruction */

	osintf_t mr_insbreak;	/* See INSBRKSET, INSBRKTEST */
	osintf_t intf_fecty;	/* For FE CTY interrupts */
#if KLH10_EVHS_INT
	osintf_t intf_evsig;	/* For event-reg signals */
#endif
	osintf_t intf_ctyio;	/* For CTY I/O */
	osintf_t intf_clk;	/* Clock interval interrupt */

	vmptr_t physmem;	/* Ptr to physical memory area */
	struct acregs acblk;	/* AC block mappings */
	struct vmregs vmap;	/* Pager context mappings */
	struct pagregs pag;	/* Paging registers */
	struct aprregs aprf;	/* APR device stuff */
	struct piregs pi;	/* PI system stuff */
	struct timeregs tim;	/* Timer and clock stuff */
	struct clkregs clk;	/* Internal Clock stuff */
	w10_t mr_dsw;		/* Console Data Switches */
	int mr_serialno;	/* CPU serial number */

	/* Debugging stuff */
	int mr_debug;		/* TRUE to print general CPU debug info */
	int mr_dotrace;		/* TRUE if doing execution trace */
	int mr_1step;		/* TRUE to stop after next instr */
	int mr_exsafe;		/* NZ does exec-mode safety checks; 2 = halt */
	pcva_t mr_bkpt;		/* Non-zero to stop when mr_PC == mr_bkpt */
	pcva_t mr_haltpc;	/* PC of a halt instruction */

	/* Miscellaneous cruft */
	int mr_runnable;	/* TRUE if CPU runnable (ie init complete) */
	int mr_running;		/* TRUE if CPU running (not halted) */
	int mm_shared;		/* TRUE if using shared phys memory */
	int mm_locked;		/* TRUE if want memory locked */
	osmm_t mm_physegid;	/* Phys memory shared segment ID (can be 0) */
	struct feregs fe;	/* FE stuff */
#if KLH10_CPU_KS
	int io_ctydelay;	/* Ugh.  To emulate I/O device delays. */
#endif

	/* Bulk storage, at end so it's less annoying when debugging. */

	w10_t acblks[ACBLKS_N][16];	/* Actual AC blocks!  16 wds each */
	opfp_t opdisp[I_N];		/* I_xxx Routine dispatch table */
	pment_t pr_umap[PAG_MAXVIRTPGS]; /* Internal user mode map table */
	pment_t pr_emap[PAG_MAXVIRTPGS]; /*   "      exec  "    "    "   */
};

EXTDEF struct machstate cpu;


/* Processor function declarations - from kn10cpu.c */

extern void apr_picheck(void);		/* Check for pending PI */
extern void apr_pcfcheck(void);		/* Check PC flags for side effects */
extern void apr_int(void);		/* Invoke interrupt system */
extern void apr_halt(enum haltcode);	/* Halt processor */
extern void pi_dismiss(void);		/* Dismiss current interrupt */
#if KLH10_CPU_KL
extern void mtr_update(void);		/* Update accounts */
#endif

/* Finally, declare panic routine to call when all else fails. */

extern void panic(char *, ...);

#endif /* ifndef  KN10DEF_INCLUDED */
