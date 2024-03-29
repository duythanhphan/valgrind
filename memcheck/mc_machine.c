
/*--------------------------------------------------------------------*/
/*--- Contains machine-specific (guest-state-layout-specific)      ---*/
/*--- support for origin tracking.                                 ---*/
/*---                                                 mc_machine.c ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of MemCheck, a heavyweight Valgrind tool for
   detecting memory errors.

   Copyright (C) 2008-2012 OpenWorks Ltd
      info@open-works.co.uk

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU General Public License is contained in the file COPYING.

   Neither the names of the U.S. Department of Energy nor the
   University of California nor the names of its contributors may be
   used to endorse or promote products derived from this software
   without prior written permission.
*/

#include "pub_tool_basics.h"
#include "pub_tool_poolalloc.h"     // For mc_include.h
#include "pub_tool_hashtable.h"     // For mc_include.h
#include "pub_tool_libcassert.h"
#include "pub_tool_libcprint.h"
#include "pub_tool_tooliface.h"

#include "mc_include.h"

#undef MC_SIZEOF_GUEST_STATE

#if defined(VGA_x86)
# include "libvex_guest_x86.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestX86State)
#endif

#if defined(VGA_amd64)
# include "libvex_guest_amd64.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestAMD64State)
#endif

#if defined(VGA_ppc32)
# include "libvex_guest_ppc32.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestPPC32State)
#endif

#if defined(VGA_ppc64)
# include "libvex_guest_ppc64.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestPPC64State)
#endif

#if defined(VGA_s390x)
# include "libvex_guest_s390x.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestS390XState)
#endif

#if defined(VGA_arm)
# include "libvex_guest_arm.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestARMState)
#endif

#if defined(VGA_mips32)
# include "libvex_guest_mips32.h"
# define MC_SIZEOF_GUEST_STATE sizeof(VexGuestMIPS32State)
#endif

static inline Bool host_is_big_endian ( void ) {
   UInt x = 0x11223344;
   return 0x1122 == *(UShort*)(&x);
}
static inline Bool host_is_little_endian ( void ) {
   UInt x = 0x11223344;
   return 0x3344 == *(UShort*)(&x);
}


/* Let (offset,szB) describe a reference to the guest state section
   [offset, offset+szB).

   This function returns the corresponding guest state reference to be
   used for the origin tag (which of course will be in the second
   shadow area), or -1 if this piece of guest state is not to be
   tracked.

   Since origin tags are 32-bits long, we expect any returned value
   (except -1) to be a multiple of 4, between 0 and
   sizeof(guest-state)-4 inclusive.

   This is inherently (guest-)architecture specific.  For x86 and
   amd64 we do some somewhat tricky things to give %AH .. %DH their
   own tags.  On ppc32/64 we do some marginally tricky things to give
   all 16 %CR components their own tags.

   This function only deals with references to the guest state whose
   offsets are known at translation time (that is, references arising
   from Put and Get).  References whose offset is not known until run
   time (that is, arise from PutI and GetI) are handled by
   MC_(get_otrack_reg_array_equiv_int_type) below.

   Note that since some guest state arrays (eg, the x86 FP reg stack)
   are accessed both as arrays (eg, x87 insns) and directly (eg, MMX
   insns), the two functions must be consistent for those sections of
   guest state -- that is, they must both say the area is shadowed, or
   both say it is not.

   This function is dependent on the host's endianness, hence we
   assert that the use case is supported.
*/
static Int get_otrack_shadow_offset_wrk ( Int offset, Int szB ); /*fwds*/

Int MC_(get_otrack_shadow_offset) ( Int offset, Int szB )
{
   Int cand = get_otrack_shadow_offset_wrk( offset, szB );
   if (cand == -1) 
      return cand;
   tl_assert(0 == (cand & 3));
   tl_assert(cand <= MC_SIZEOF_GUEST_STATE-4);
   return cand;
}


static Int get_otrack_shadow_offset_wrk ( Int offset, Int szB )
{
   /* -------------------- ppc64 -------------------- */

#  if defined(VGA_ppc64)

#  define GOF(_fieldname) \
      (offsetof(VexGuestPPC64State,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestPPC64State*)0)->guest_##_fieldname))

   Int  sz   = szB;
   Int  o    = offset;
   tl_assert(sz > 0);
   tl_assert(host_is_big_endian());

   if (sz == 8 || sz == 4) {
      /* The point of this is to achieve
         if ((o == GOF(GPRn) && sz == 8) || (o == 4+GOF(GPRn) && sz == 4))
            return GOF(GPRn);
         by testing ox instead of o, and setting ox back 4 bytes when sz == 4.
      */
      Int ox = sz == 8 ? o : (o - 4);
      if (ox == GOF(GPR0)) return ox;
      if (ox == GOF(GPR1)) return ox;
      if (ox == GOF(GPR2)) return ox;
      if (ox == GOF(GPR3)) return ox;
      if (ox == GOF(GPR4)) return ox;
      if (ox == GOF(GPR5)) return ox;
      if (ox == GOF(GPR6)) return ox;
      if (ox == GOF(GPR7)) return ox;
      if (ox == GOF(GPR8)) return ox;
      if (ox == GOF(GPR9)) return ox;
      if (ox == GOF(GPR10)) return ox;
      if (ox == GOF(GPR11)) return ox;
      if (ox == GOF(GPR12)) return ox;
      if (ox == GOF(GPR13)) return ox;
      if (ox == GOF(GPR14)) return ox;
      if (ox == GOF(GPR15)) return ox;
      if (ox == GOF(GPR16)) return ox;
      if (ox == GOF(GPR17)) return ox;
      if (ox == GOF(GPR18)) return ox;
      if (ox == GOF(GPR19)) return ox;
      if (ox == GOF(GPR20)) return ox;
      if (ox == GOF(GPR21)) return ox;
      if (ox == GOF(GPR22)) return ox;
      if (ox == GOF(GPR23)) return ox;
      if (ox == GOF(GPR24)) return ox;
      if (ox == GOF(GPR25)) return ox;
      if (ox == GOF(GPR26)) return ox;
      if (ox == GOF(GPR27)) return ox;
      if (ox == GOF(GPR28)) return ox;
      if (ox == GOF(GPR29)) return ox;
      if (ox == GOF(GPR30)) return ox;
      if (ox == GOF(GPR31)) return ox;
   }

   if (o == GOF(LR)  && sz == 8) return o;
   if (o == GOF(CTR) && sz == 8) return o;

   if (o == GOF(CIA)       && sz == 8) return -1;
   if (o == GOF(IP_AT_SYSCALL) && sz == 8) return -1; /* slot unused */
   if (o == GOF(FPROUND)   && sz == 1) return -1;
   if (o == GOF(DFPROUND)  && sz == 1) return -1;
   if (o == GOF(EMNOTE)    && sz == 4) return -1;
   if (o == GOF(TISTART)   && sz == 8) return -1;
   if (o == GOF(TILEN)     && sz == 8) return -1;
   if (o == GOF(VSCR)      && sz == 4) return -1;
   if (o == GOF(VRSAVE)    && sz == 4) return -1;
   if (o == GOF(REDIR_SP)  && sz == 8) return -1;

   // With ISA 2.06, the "Vector-Scalar Floating-point" category
   // provides facilities to support vector and scalar binary floating-
   // point operations.  A unified register file is an integral part
   // of this new facility, combining floating point and vector registers
   // using a 64x128-bit vector.  These are referred to as VSR[0..63].
   // The floating point registers are now mapped into double word element 0
   // of VSR[0..31]. The 32x128-bit vector registers defined by the "Vector
   // Facility [Category: Vector]" are now mapped to VSR[32..63].

   //  Floating point registers . . .
   if (o == GOF(VSR0) && sz == 8) return o;
   if (o == GOF(VSR1) && sz == 8) return o;
   if (o == GOF(VSR2) && sz == 8) return o;
   if (o == GOF(VSR3) && sz == 8) return o;
   if (o == GOF(VSR4) && sz == 8) return o;
   if (o == GOF(VSR5) && sz == 8) return o;
   if (o == GOF(VSR6) && sz == 8) return o;
   if (o == GOF(VSR7) && sz == 8) return o;
   if (o == GOF(VSR8) && sz == 8) return o;
   if (o == GOF(VSR9) && sz == 8) return o;
   if (o == GOF(VSR10) && sz == 8) return o;
   if (o == GOF(VSR11) && sz == 8) return o;
   if (o == GOF(VSR12) && sz == 8) return o;
   if (o == GOF(VSR13) && sz == 8) return o;
   if (o == GOF(VSR14) && sz == 8) return o;
   if (o == GOF(VSR15) && sz == 8) return o;
   if (o == GOF(VSR16) && sz == 8) return o;
   if (o == GOF(VSR17) && sz == 8) return o;
   if (o == GOF(VSR18) && sz == 8) return o;
   if (o == GOF(VSR19) && sz == 8) return o;
   if (o == GOF(VSR20) && sz == 8) return o;
   if (o == GOF(VSR21) && sz == 8) return o;
   if (o == GOF(VSR22) && sz == 8) return o;
   if (o == GOF(VSR23) && sz == 8) return o;
   if (o == GOF(VSR24) && sz == 8) return o;
   if (o == GOF(VSR25) && sz == 8) return o;
   if (o == GOF(VSR26) && sz == 8) return o;
   if (o == GOF(VSR27) && sz == 8) return o;
   if (o == GOF(VSR28) && sz == 8) return o;
   if (o == GOF(VSR29) && sz == 8) return o;
   if (o == GOF(VSR30) && sz == 8) return o;
   if (o == GOF(VSR31) && sz == 8) return o;

   /* For the various byte sized XER/CR pieces, use offset 8
      in VSR0 .. VSR19. */
   tl_assert(SZB(VSR0) == 16);
   if (o == GOF(XER_SO) && sz == 1) return 8 +GOF(VSR0);
   if (o == GOF(XER_OV) && sz == 1) return 8 +GOF(VSR1);
   if (o == GOF(XER_CA) && sz == 1) return 8 +GOF(VSR2);
   if (o == GOF(XER_BC) && sz == 1) return 8 +GOF(VSR3);

   if (o == GOF(CR0_321) && sz == 1) return 8 +GOF(VSR4);
   if (o == GOF(CR0_0)   && sz == 1) return 8 +GOF(VSR5);
   if (o == GOF(CR1_321) && sz == 1) return 8 +GOF(VSR6);
   if (o == GOF(CR1_0)   && sz == 1) return 8 +GOF(VSR7);
   if (o == GOF(CR2_321) && sz == 1) return 8 +GOF(VSR8);
   if (o == GOF(CR2_0)   && sz == 1) return 8 +GOF(VSR9);
   if (o == GOF(CR3_321) && sz == 1) return 8 +GOF(VSR10);
   if (o == GOF(CR3_0)   && sz == 1) return 8 +GOF(VSR11);
   if (o == GOF(CR4_321) && sz == 1) return 8 +GOF(VSR12);
   if (o == GOF(CR4_0)   && sz == 1) return 8 +GOF(VSR13);
   if (o == GOF(CR5_321) && sz == 1) return 8 +GOF(VSR14);
   if (o == GOF(CR5_0)   && sz == 1) return 8 +GOF(VSR15);
   if (o == GOF(CR6_321) && sz == 1) return 8 +GOF(VSR16);
   if (o == GOF(CR6_0)   && sz == 1) return 8 +GOF(VSR17);
   if (o == GOF(CR7_321) && sz == 1) return 8 +GOF(VSR18);
   if (o == GOF(CR7_0)   && sz == 1) return 8 +GOF(VSR19);

   /* Vector registers .. use offset 0 in VSR0 .. VSR63. */
   if (o >= GOF(VSR0)  && o+sz <= GOF(VSR0) +SZB(VSR0))  return 0+ GOF(VSR0);
   if (o >= GOF(VSR1)  && o+sz <= GOF(VSR1) +SZB(VSR1))  return 0+ GOF(VSR1);
   if (o >= GOF(VSR2)  && o+sz <= GOF(VSR2) +SZB(VSR2))  return 0+ GOF(VSR2);
   if (o >= GOF(VSR3)  && o+sz <= GOF(VSR3) +SZB(VSR3))  return 0+ GOF(VSR3);
   if (o >= GOF(VSR4)  && o+sz <= GOF(VSR4) +SZB(VSR4))  return 0+ GOF(VSR4);
   if (o >= GOF(VSR5)  && o+sz <= GOF(VSR5) +SZB(VSR5))  return 0+ GOF(VSR5);
   if (o >= GOF(VSR6)  && o+sz <= GOF(VSR6) +SZB(VSR6))  return 0+ GOF(VSR6);
   if (o >= GOF(VSR7)  && o+sz <= GOF(VSR7) +SZB(VSR7))  return 0+ GOF(VSR7);
   if (o >= GOF(VSR8)  && o+sz <= GOF(VSR8) +SZB(VSR8))  return 0+ GOF(VSR8);
   if (o >= GOF(VSR9)  && o+sz <= GOF(VSR9) +SZB(VSR9))  return 0+ GOF(VSR9);
   if (o >= GOF(VSR10) && o+sz <= GOF(VSR10)+SZB(VSR10)) return 0+ GOF(VSR10);
   if (o >= GOF(VSR11) && o+sz <= GOF(VSR11)+SZB(VSR11)) return 0+ GOF(VSR11);
   if (o >= GOF(VSR12) && o+sz <= GOF(VSR12)+SZB(VSR12)) return 0+ GOF(VSR12);
   if (o >= GOF(VSR13) && o+sz <= GOF(VSR13)+SZB(VSR13)) return 0+ GOF(VSR13);
   if (o >= GOF(VSR14) && o+sz <= GOF(VSR14)+SZB(VSR14)) return 0+ GOF(VSR14);
   if (o >= GOF(VSR15) && o+sz <= GOF(VSR15)+SZB(VSR15)) return 0+ GOF(VSR15);
   if (o >= GOF(VSR16) && o+sz <= GOF(VSR16)+SZB(VSR16)) return 0+ GOF(VSR16);
   if (o >= GOF(VSR17) && o+sz <= GOF(VSR17)+SZB(VSR17)) return 0+ GOF(VSR17);
   if (o >= GOF(VSR18) && o+sz <= GOF(VSR18)+SZB(VSR18)) return 0+ GOF(VSR18);
   if (o >= GOF(VSR19) && o+sz <= GOF(VSR19)+SZB(VSR19)) return 0+ GOF(VSR19);
   if (o >= GOF(VSR20) && o+sz <= GOF(VSR20)+SZB(VSR20)) return 0+ GOF(VSR20);
   if (o >= GOF(VSR21) && o+sz <= GOF(VSR21)+SZB(VSR21)) return 0+ GOF(VSR21);
   if (o >= GOF(VSR22) && o+sz <= GOF(VSR22)+SZB(VSR22)) return 0+ GOF(VSR22);
   if (o >= GOF(VSR23) && o+sz <= GOF(VSR23)+SZB(VSR23)) return 0+ GOF(VSR23);
   if (o >= GOF(VSR24) && o+sz <= GOF(VSR24)+SZB(VSR24)) return 0+ GOF(VSR24);
   if (o >= GOF(VSR25) && o+sz <= GOF(VSR25)+SZB(VSR25)) return 0+ GOF(VSR25);
   if (o >= GOF(VSR26) && o+sz <= GOF(VSR26)+SZB(VSR26)) return 0+ GOF(VSR26);
   if (o >= GOF(VSR27) && o+sz <= GOF(VSR27)+SZB(VSR27)) return 0+ GOF(VSR27);
   if (o >= GOF(VSR28) && o+sz <= GOF(VSR28)+SZB(VSR28)) return 0+ GOF(VSR28);
   if (o >= GOF(VSR29) && o+sz <= GOF(VSR29)+SZB(VSR29)) return 0+ GOF(VSR29);
   if (o >= GOF(VSR30) && o+sz <= GOF(VSR30)+SZB(VSR30)) return 0+ GOF(VSR30);
   if (o >= GOF(VSR31) && o+sz <= GOF(VSR31)+SZB(VSR31)) return 0+ GOF(VSR31);
   if (o >= GOF(VSR32) && o+sz <= GOF(VSR32)+SZB(VSR32)) return 0+ GOF(VSR32);
   if (o >= GOF(VSR33) && o+sz <= GOF(VSR33)+SZB(VSR33)) return 0+ GOF(VSR33);
   if (o >= GOF(VSR34) && o+sz <= GOF(VSR34)+SZB(VSR34)) return 0+ GOF(VSR34);
   if (o >= GOF(VSR35) && o+sz <= GOF(VSR35)+SZB(VSR35)) return 0+ GOF(VSR35);
   if (o >= GOF(VSR36) && o+sz <= GOF(VSR36)+SZB(VSR36)) return 0+ GOF(VSR36);
   if (o >= GOF(VSR37) && o+sz <= GOF(VSR37)+SZB(VSR37)) return 0+ GOF(VSR37);
   if (o >= GOF(VSR38) && o+sz <= GOF(VSR38)+SZB(VSR38)) return 0+ GOF(VSR38);
   if (o >= GOF(VSR39) && o+sz <= GOF(VSR39)+SZB(VSR39)) return 0+ GOF(VSR39);
   if (o >= GOF(VSR40) && o+sz <= GOF(VSR40)+SZB(VSR40)) return 0+ GOF(VSR40);
   if (o >= GOF(VSR41) && o+sz <= GOF(VSR41)+SZB(VSR41)) return 0+ GOF(VSR41);
   if (o >= GOF(VSR42) && o+sz <= GOF(VSR42)+SZB(VSR42)) return 0+ GOF(VSR42);
   if (o >= GOF(VSR43) && o+sz <= GOF(VSR43)+SZB(VSR43)) return 0+ GOF(VSR43);
   if (o >= GOF(VSR44) && o+sz <= GOF(VSR44)+SZB(VSR44)) return 0+ GOF(VSR44);
   if (o >= GOF(VSR45) && o+sz <= GOF(VSR45)+SZB(VSR45)) return 0+ GOF(VSR45);
   if (o >= GOF(VSR46) && o+sz <= GOF(VSR46)+SZB(VSR46)) return 0+ GOF(VSR46);
   if (o >= GOF(VSR47) && o+sz <= GOF(VSR47)+SZB(VSR47)) return 0+ GOF(VSR47);
   if (o >= GOF(VSR48) && o+sz <= GOF(VSR48)+SZB(VSR48)) return 0+ GOF(VSR48);
   if (o >= GOF(VSR49) && o+sz <= GOF(VSR49)+SZB(VSR49)) return 0+ GOF(VSR49);
   if (o >= GOF(VSR50) && o+sz <= GOF(VSR50)+SZB(VSR50)) return 0+ GOF(VSR50);
   if (o >= GOF(VSR51) && o+sz <= GOF(VSR51)+SZB(VSR51)) return 0+ GOF(VSR51);
   if (o >= GOF(VSR52) && o+sz <= GOF(VSR52)+SZB(VSR52)) return 0+ GOF(VSR52);
   if (o >= GOF(VSR53) && o+sz <= GOF(VSR53)+SZB(VSR53)) return 0+ GOF(VSR53);
   if (o >= GOF(VSR54) && o+sz <= GOF(VSR54)+SZB(VSR54)) return 0+ GOF(VSR54);
   if (o >= GOF(VSR55) && o+sz <= GOF(VSR55)+SZB(VSR55)) return 0+ GOF(VSR55);
   if (o >= GOF(VSR56) && o+sz <= GOF(VSR56)+SZB(VSR56)) return 0+ GOF(VSR56);
   if (o >= GOF(VSR57) && o+sz <= GOF(VSR57)+SZB(VSR57)) return 0+ GOF(VSR57);
   if (o >= GOF(VSR58) && o+sz <= GOF(VSR58)+SZB(VSR58)) return 0+ GOF(VSR58);
   if (o >= GOF(VSR59) && o+sz <= GOF(VSR59)+SZB(VSR59)) return 0+ GOF(VSR59);
   if (o >= GOF(VSR60) && o+sz <= GOF(VSR60)+SZB(VSR60)) return 0+ GOF(VSR60);
   if (o >= GOF(VSR61) && o+sz <= GOF(VSR61)+SZB(VSR61)) return 0+ GOF(VSR61);
   if (o >= GOF(VSR62) && o+sz <= GOF(VSR62)+SZB(VSR62)) return 0+ GOF(VSR62);
   if (o >= GOF(VSR63) && o+sz <= GOF(VSR63)+SZB(VSR63)) return 0+ GOF(VSR63);

   VG_(printf)("MC_(get_otrack_shadow_offset)(ppc64)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

   /* -------------------- ppc32 -------------------- */

#  elif defined(VGA_ppc32)

#  define GOF(_fieldname) \
      (offsetof(VexGuestPPC32State,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestPPC32State*)0)->guest_##_fieldname))
   Int  o  = offset;
   Int  sz = szB;
   tl_assert(sz > 0);
   tl_assert(host_is_big_endian());

   if (o == GOF(GPR0) && sz == 4) return o;
   if (o == GOF(GPR1) && sz == 4) return o;
   if (o == GOF(GPR2) && sz == 4) return o;
   if (o == GOF(GPR3) && sz == 4) return o;
   if (o == GOF(GPR4) && sz == 4) return o;
   if (o == GOF(GPR5) && sz == 4) return o;
   if (o == GOF(GPR6) && sz == 4) return o;
   if (o == GOF(GPR7) && sz == 4) return o;
   if (o == GOF(GPR8) && sz == 4) return o;
   if (o == GOF(GPR9) && sz == 4) return o;
   if (o == GOF(GPR10) && sz == 4) return o;
   if (o == GOF(GPR11) && sz == 4) return o;
   if (o == GOF(GPR12) && sz == 4) return o;
   if (o == GOF(GPR13) && sz == 4) return o;
   if (o == GOF(GPR14) && sz == 4) return o;
   if (o == GOF(GPR15) && sz == 4) return o;
   if (o == GOF(GPR16) && sz == 4) return o;
   if (o == GOF(GPR17) && sz == 4) return o;
   if (o == GOF(GPR18) && sz == 4) return o;
   if (o == GOF(GPR19) && sz == 4) return o;
   if (o == GOF(GPR20) && sz == 4) return o;
   if (o == GOF(GPR21) && sz == 4) return o;
   if (o == GOF(GPR22) && sz == 4) return o;
   if (o == GOF(GPR23) && sz == 4) return o;
   if (o == GOF(GPR24) && sz == 4) return o;
   if (o == GOF(GPR25) && sz == 4) return o;
   if (o == GOF(GPR26) && sz == 4) return o;
   if (o == GOF(GPR27) && sz == 4) return o;
   if (o == GOF(GPR28) && sz == 4) return o;
   if (o == GOF(GPR29) && sz == 4) return o;
   if (o == GOF(GPR30) && sz == 4) return o;
   if (o == GOF(GPR31) && sz == 4) return o;

   if (o == GOF(LR)  && sz == 4) return o;
   if (o == GOF(CTR) && sz == 4) return o;

   if (o == GOF(CIA)       && sz == 4) return -1;
   if (o == GOF(IP_AT_SYSCALL) && sz == 4) return -1; /* slot unused */
   if (o == GOF(FPROUND)   && sz == 1) return -1;
   if (o == GOF(DFPROUND)  && sz == 1) return -1;
   if (o == GOF(VRSAVE)    && sz == 4) return -1;
   if (o == GOF(EMNOTE)    && sz == 4) return -1;
   if (o == GOF(TISTART)   && sz == 4) return -1;
   if (o == GOF(TILEN)     && sz == 4) return -1;
   if (o == GOF(VSCR)      && sz == 4) return -1;
   if (o == GOF(REDIR_SP)  && sz == 4) return -1;
   if (o == GOF(SPRG3_RO)  && sz == 4) return -1;

   // With ISA 2.06, the "Vector-Scalar Floating-point" category
   // provides facilities to support vector and scalar binary floating-
   // point operations.  A unified register file is an integral part
   // of this new facility, combining floating point and vector registers
   // using a 64x128-bit vector.  These are referred to as VSR[0..63].
   // The floating point registers are now mapped into double word element 0
   // of VSR[0..31]. The 32x128-bit vector registers defined by the "Vector
   // Facility [Category: Vector]" are now mapped to VSR[32..63].

   //  Floating point registers . . .
   if (o == GOF(VSR0) && sz == 8) return o;
   if (o == GOF(VSR1) && sz == 8) return o;
   if (o == GOF(VSR2) && sz == 8) return o;
   if (o == GOF(VSR3) && sz == 8) return o;
   if (o == GOF(VSR4) && sz == 8) return o;
   if (o == GOF(VSR5) && sz == 8) return o;
   if (o == GOF(VSR6) && sz == 8) return o;
   if (o == GOF(VSR7) && sz == 8) return o;
   if (o == GOF(VSR8) && sz == 8) return o;
   if (o == GOF(VSR9) && sz == 8) return o;
   if (o == GOF(VSR10) && sz == 8) return o;
   if (o == GOF(VSR11) && sz == 8) return o;
   if (o == GOF(VSR12) && sz == 8) return o;
   if (o == GOF(VSR13) && sz == 8) return o;
   if (o == GOF(VSR14) && sz == 8) return o;
   if (o == GOF(VSR15) && sz == 8) return o;
   if (o == GOF(VSR16) && sz == 8) return o;
   if (o == GOF(VSR17) && sz == 8) return o;
   if (o == GOF(VSR18) && sz == 8) return o;
   if (o == GOF(VSR19) && sz == 8) return o;
   if (o == GOF(VSR20) && sz == 8) return o;
   if (o == GOF(VSR21) && sz == 8) return o;
   if (o == GOF(VSR22) && sz == 8) return o;
   if (o == GOF(VSR23) && sz == 8) return o;
   if (o == GOF(VSR24) && sz == 8) return o;
   if (o == GOF(VSR25) && sz == 8) return o;
   if (o == GOF(VSR26) && sz == 8) return o;
   if (o == GOF(VSR27) && sz == 8) return o;
   if (o == GOF(VSR28) && sz == 8) return o;
   if (o == GOF(VSR29) && sz == 8) return o;
   if (o == GOF(VSR30) && sz == 8) return o;
   if (o == GOF(VSR31) && sz == 8) return o;

   /* For the various byte sized XER/CR pieces, use offset 8
      in VSR0 .. VSR19. */
   tl_assert(SZB(VSR0) == 16);
   if (o == GOF(XER_SO) && sz == 1) return 8 +GOF(VSR0);
   if (o == GOF(XER_OV) && sz == 1) return 8 +GOF(VSR1);
   if (o == GOF(XER_CA) && sz == 1) return 8 +GOF(VSR2);
   if (o == GOF(XER_BC) && sz == 1) return 8 +GOF(VSR3);

   if (o == GOF(CR0_321) && sz == 1) return 8 +GOF(VSR4);
   if (o == GOF(CR0_0)   && sz == 1) return 8 +GOF(VSR5);
   if (o == GOF(CR1_321) && sz == 1) return 8 +GOF(VSR6);
   if (o == GOF(CR1_0)   && sz == 1) return 8 +GOF(VSR7);
   if (o == GOF(CR2_321) && sz == 1) return 8 +GOF(VSR8);
   if (o == GOF(CR2_0)   && sz == 1) return 8 +GOF(VSR9);
   if (o == GOF(CR3_321) && sz == 1) return 8 +GOF(VSR10);
   if (o == GOF(CR3_0)   && sz == 1) return 8 +GOF(VSR11);
   if (o == GOF(CR4_321) && sz == 1) return 8 +GOF(VSR12);
   if (o == GOF(CR4_0)   && sz == 1) return 8 +GOF(VSR13);
   if (o == GOF(CR5_321) && sz == 1) return 8 +GOF(VSR14);
   if (o == GOF(CR5_0)   && sz == 1) return 8 +GOF(VSR15);
   if (o == GOF(CR6_321) && sz == 1) return 8 +GOF(VSR16);
   if (o == GOF(CR6_0)   && sz == 1) return 8 +GOF(VSR17);
   if (o == GOF(CR7_321) && sz == 1) return 8 +GOF(VSR18);
   if (o == GOF(CR7_0)   && sz == 1) return 8 +GOF(VSR19);

   /* Vector registers .. use offset 0 in VSR0 .. VSR63. */
   if (o >= GOF(VSR0)  && o+sz <= GOF(VSR0) +SZB(VSR0))  return 0+ GOF(VSR0);
   if (o >= GOF(VSR1)  && o+sz <= GOF(VSR1) +SZB(VSR1))  return 0+ GOF(VSR1);
   if (o >= GOF(VSR2)  && o+sz <= GOF(VSR2) +SZB(VSR2))  return 0+ GOF(VSR2);
   if (o >= GOF(VSR3)  && o+sz <= GOF(VSR3) +SZB(VSR3))  return 0+ GOF(VSR3);
   if (o >= GOF(VSR4)  && o+sz <= GOF(VSR4) +SZB(VSR4))  return 0+ GOF(VSR4);
   if (o >= GOF(VSR5)  && o+sz <= GOF(VSR5) +SZB(VSR5))  return 0+ GOF(VSR5);
   if (o >= GOF(VSR6)  && o+sz <= GOF(VSR6) +SZB(VSR6))  return 0+ GOF(VSR6);
   if (o >= GOF(VSR7)  && o+sz <= GOF(VSR7) +SZB(VSR7))  return 0+ GOF(VSR7);
   if (o >= GOF(VSR8)  && o+sz <= GOF(VSR8) +SZB(VSR8))  return 0+ GOF(VSR8);
   if (o >= GOF(VSR9)  && o+sz <= GOF(VSR9) +SZB(VSR9))  return 0+ GOF(VSR9);
   if (o >= GOF(VSR10) && o+sz <= GOF(VSR10)+SZB(VSR10)) return 0+ GOF(VSR10);
   if (o >= GOF(VSR11) && o+sz <= GOF(VSR11)+SZB(VSR11)) return 0+ GOF(VSR11);
   if (o >= GOF(VSR12) && o+sz <= GOF(VSR12)+SZB(VSR12)) return 0+ GOF(VSR12);
   if (o >= GOF(VSR13) && o+sz <= GOF(VSR13)+SZB(VSR13)) return 0+ GOF(VSR13);
   if (o >= GOF(VSR14) && o+sz <= GOF(VSR14)+SZB(VSR14)) return 0+ GOF(VSR14);
   if (o >= GOF(VSR15) && o+sz <= GOF(VSR15)+SZB(VSR15)) return 0+ GOF(VSR15);
   if (o >= GOF(VSR16) && o+sz <= GOF(VSR16)+SZB(VSR16)) return 0+ GOF(VSR16);
   if (o >= GOF(VSR17) && o+sz <= GOF(VSR17)+SZB(VSR17)) return 0+ GOF(VSR17);
   if (o >= GOF(VSR18) && o+sz <= GOF(VSR18)+SZB(VSR18)) return 0+ GOF(VSR18);
   if (o >= GOF(VSR19) && o+sz <= GOF(VSR19)+SZB(VSR19)) return 0+ GOF(VSR19);
   if (o >= GOF(VSR20) && o+sz <= GOF(VSR20)+SZB(VSR20)) return 0+ GOF(VSR20);
   if (o >= GOF(VSR21) && o+sz <= GOF(VSR21)+SZB(VSR21)) return 0+ GOF(VSR21);
   if (o >= GOF(VSR22) && o+sz <= GOF(VSR22)+SZB(VSR22)) return 0+ GOF(VSR22);
   if (o >= GOF(VSR23) && o+sz <= GOF(VSR23)+SZB(VSR23)) return 0+ GOF(VSR23);
   if (o >= GOF(VSR24) && o+sz <= GOF(VSR24)+SZB(VSR24)) return 0+ GOF(VSR24);
   if (o >= GOF(VSR25) && o+sz <= GOF(VSR25)+SZB(VSR25)) return 0+ GOF(VSR25);
   if (o >= GOF(VSR26) && o+sz <= GOF(VSR26)+SZB(VSR26)) return 0+ GOF(VSR26);
   if (o >= GOF(VSR27) && o+sz <= GOF(VSR27)+SZB(VSR27)) return 0+ GOF(VSR27);
   if (o >= GOF(VSR28) && o+sz <= GOF(VSR28)+SZB(VSR28)) return 0+ GOF(VSR28);
   if (o >= GOF(VSR29) && o+sz <= GOF(VSR29)+SZB(VSR29)) return 0+ GOF(VSR29);
   if (o >= GOF(VSR30) && o+sz <= GOF(VSR30)+SZB(VSR30)) return 0+ GOF(VSR30);
   if (o >= GOF(VSR31) && o+sz <= GOF(VSR31)+SZB(VSR31)) return 0+ GOF(VSR31);
   if (o >= GOF(VSR32) && o+sz <= GOF(VSR32)+SZB(VSR32)) return 0+ GOF(VSR32);
   if (o >= GOF(VSR33) && o+sz <= GOF(VSR33)+SZB(VSR33)) return 0+ GOF(VSR33);
   if (o >= GOF(VSR34) && o+sz <= GOF(VSR34)+SZB(VSR34)) return 0+ GOF(VSR34);
   if (o >= GOF(VSR35) && o+sz <= GOF(VSR35)+SZB(VSR35)) return 0+ GOF(VSR35);
   if (o >= GOF(VSR36) && o+sz <= GOF(VSR36)+SZB(VSR36)) return 0+ GOF(VSR36);
   if (o >= GOF(VSR37) && o+sz <= GOF(VSR37)+SZB(VSR37)) return 0+ GOF(VSR37);
   if (o >= GOF(VSR38) && o+sz <= GOF(VSR38)+SZB(VSR38)) return 0+ GOF(VSR38);
   if (o >= GOF(VSR39) && o+sz <= GOF(VSR39)+SZB(VSR39)) return 0+ GOF(VSR39);
   if (o >= GOF(VSR40) && o+sz <= GOF(VSR40)+SZB(VSR40)) return 0+ GOF(VSR40);
   if (o >= GOF(VSR41) && o+sz <= GOF(VSR41)+SZB(VSR41)) return 0+ GOF(VSR41);
   if (o >= GOF(VSR42) && o+sz <= GOF(VSR42)+SZB(VSR42)) return 0+ GOF(VSR42);
   if (o >= GOF(VSR43) && o+sz <= GOF(VSR43)+SZB(VSR43)) return 0+ GOF(VSR43);
   if (o >= GOF(VSR44) && o+sz <= GOF(VSR44)+SZB(VSR44)) return 0+ GOF(VSR44);
   if (o >= GOF(VSR45) && o+sz <= GOF(VSR45)+SZB(VSR45)) return 0+ GOF(VSR45);
   if (o >= GOF(VSR46) && o+sz <= GOF(VSR46)+SZB(VSR46)) return 0+ GOF(VSR46);
   if (o >= GOF(VSR47) && o+sz <= GOF(VSR47)+SZB(VSR47)) return 0+ GOF(VSR47);
   if (o >= GOF(VSR48) && o+sz <= GOF(VSR48)+SZB(VSR48)) return 0+ GOF(VSR48);
   if (o >= GOF(VSR49) && o+sz <= GOF(VSR49)+SZB(VSR49)) return 0+ GOF(VSR49);
   if (o >= GOF(VSR50) && o+sz <= GOF(VSR50)+SZB(VSR50)) return 0+ GOF(VSR50);
   if (o >= GOF(VSR51) && o+sz <= GOF(VSR51)+SZB(VSR51)) return 0+ GOF(VSR51);
   if (o >= GOF(VSR52) && o+sz <= GOF(VSR52)+SZB(VSR52)) return 0+ GOF(VSR52);
   if (o >= GOF(VSR53) && o+sz <= GOF(VSR53)+SZB(VSR53)) return 0+ GOF(VSR53);
   if (o >= GOF(VSR54) && o+sz <= GOF(VSR54)+SZB(VSR54)) return 0+ GOF(VSR54);
   if (o >= GOF(VSR55) && o+sz <= GOF(VSR55)+SZB(VSR55)) return 0+ GOF(VSR55);
   if (o >= GOF(VSR56) && o+sz <= GOF(VSR56)+SZB(VSR56)) return 0+ GOF(VSR56);
   if (o >= GOF(VSR57) && o+sz <= GOF(VSR57)+SZB(VSR57)) return 0+ GOF(VSR57);
   if (o >= GOF(VSR58) && o+sz <= GOF(VSR58)+SZB(VSR58)) return 0+ GOF(VSR58);
   if (o >= GOF(VSR59) && o+sz <= GOF(VSR59)+SZB(VSR59)) return 0+ GOF(VSR59);
   if (o >= GOF(VSR60) && o+sz <= GOF(VSR60)+SZB(VSR60)) return 0+ GOF(VSR60);
   if (o >= GOF(VSR61) && o+sz <= GOF(VSR61)+SZB(VSR61)) return 0+ GOF(VSR61);
   if (o >= GOF(VSR62) && o+sz <= GOF(VSR62)+SZB(VSR62)) return 0+ GOF(VSR62);
   if (o >= GOF(VSR63) && o+sz <= GOF(VSR63)+SZB(VSR63)) return 0+ GOF(VSR63);

   VG_(printf)("MC_(get_otrack_shadow_offset)(ppc32)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

   /* -------------------- amd64 -------------------- */

#  elif defined(VGA_amd64)

#  define GOF(_fieldname) \
      (offsetof(VexGuestAMD64State,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestAMD64State*)0)->guest_##_fieldname))
   Int  o      = offset;
   Int  sz     = szB;
   Bool is1248 = sz == 8 || sz == 4 || sz == 2 || sz == 1;
   tl_assert(sz > 0);
   tl_assert(host_is_little_endian());

   if (o == GOF(RAX) && is1248) return o;
   if (o == GOF(RCX) && is1248) return o;
   if (o == GOF(RDX) && is1248) return o;
   if (o == GOF(RBX) && is1248) return o;
   if (o == GOF(RSP) && is1248) return o;
   if (o == GOF(RBP) && is1248) return o;
   if (o == GOF(RSI) && is1248) return o;
   if (o == GOF(RDI) && is1248) return o;
   if (o == GOF(R8)  && is1248) return o;
   if (o == GOF(R9)  && is1248) return o;
   if (o == GOF(R10) && is1248) return o;
   if (o == GOF(R11) && is1248) return o;
   if (o == GOF(R12) && is1248) return o;
   if (o == GOF(R13) && is1248) return o;
   if (o == GOF(R14) && is1248) return o;
   if (o == GOF(R15) && is1248) return o;

   if (o == GOF(CC_DEP1) && sz == 8) return o;
   if (o == GOF(CC_DEP2) && sz == 8) return o;

   if (o == GOF(CC_OP)   && sz == 8) return -1; /* slot used for %AH */
   if (o == GOF(CC_NDEP) && sz == 8) return -1; /* slot used for %BH */
   if (o == GOF(DFLAG)   && sz == 8) return -1; /* slot used for %CH */
   if (o == GOF(RIP)     && sz == 8) return -1; /* slot unused */
   if (o == GOF(IP_AT_SYSCALL) && sz == 8) return -1; /* slot unused */
   if (o == GOF(IDFLAG)  && sz == 8) return -1; /* slot used for %DH */
   if (o == GOF(ACFLAG)  && sz == 8) return -1; /* slot unused */
   if (o == GOF(FS_ZERO) && sz == 8) return -1; /* slot unused */
   if (o == GOF(GS_0x60) && sz == 8) return -1; /* slot unused */
   if (o == GOF(TISTART) && sz == 8) return -1; /* slot unused */
   if (o == GOF(TILEN)   && sz == 8) return -1; /* slot unused */

   /* Treat %AH, %BH, %CH, %DH as independent registers.  To do this
      requires finding 4 unused 32-bit slots in the second-shadow
      guest state, respectively: CC_OP CC_NDEP DFLAG IDFLAG, since
      none of those are tracked. */
   tl_assert(SZB(CC_OP)   == 8);
   tl_assert(SZB(CC_NDEP) == 8);
   tl_assert(SZB(IDFLAG)  == 8);
   tl_assert(SZB(DFLAG)   == 8);

   if (o == 1+ GOF(RAX) && szB == 1) return GOF(CC_OP);
   if (o == 1+ GOF(RBX) && szB == 1) return GOF(CC_NDEP);
   if (o == 1+ GOF(RCX) && szB == 1) return GOF(DFLAG);
   if (o == 1+ GOF(RDX) && szB == 1) return GOF(IDFLAG);

   /* skip XMM and FP admin stuff */
   if (o == GOF(SSEROUND) && szB == 8) return -1;
   if (o == GOF(FTOP)     && szB == 4) return -1;
   if (o == GOF(FPROUND)  && szB == 8) return -1;
   if (o == GOF(EMNOTE)   && szB == 4) return -1;
   if (o == GOF(FC3210)   && szB == 8) return -1;

   /* XMM registers */
   if (o >= GOF(YMM0)  && o+sz <= GOF(YMM0) +SZB(YMM0))  return GOF(YMM0);
   if (o >= GOF(YMM1)  && o+sz <= GOF(YMM1) +SZB(YMM1))  return GOF(YMM1);
   if (o >= GOF(YMM2)  && o+sz <= GOF(YMM2) +SZB(YMM2))  return GOF(YMM2);
   if (o >= GOF(YMM3)  && o+sz <= GOF(YMM3) +SZB(YMM3))  return GOF(YMM3);
   if (o >= GOF(YMM4)  && o+sz <= GOF(YMM4) +SZB(YMM4))  return GOF(YMM4);
   if (o >= GOF(YMM5)  && o+sz <= GOF(YMM5) +SZB(YMM5))  return GOF(YMM5);
   if (o >= GOF(YMM6)  && o+sz <= GOF(YMM6) +SZB(YMM6))  return GOF(YMM6);
   if (o >= GOF(YMM7)  && o+sz <= GOF(YMM7) +SZB(YMM7))  return GOF(YMM7);
   if (o >= GOF(YMM8)  && o+sz <= GOF(YMM8) +SZB(YMM8))  return GOF(YMM8);
   if (o >= GOF(YMM9)  && o+sz <= GOF(YMM9) +SZB(YMM9))  return GOF(YMM9);
   if (o >= GOF(YMM10) && o+sz <= GOF(YMM10)+SZB(YMM10)) return GOF(YMM10);
   if (o >= GOF(YMM11) && o+sz <= GOF(YMM11)+SZB(YMM11)) return GOF(YMM11);
   if (o >= GOF(YMM12) && o+sz <= GOF(YMM12)+SZB(YMM12)) return GOF(YMM12);
   if (o >= GOF(YMM13) && o+sz <= GOF(YMM13)+SZB(YMM13)) return GOF(YMM13);
   if (o >= GOF(YMM14) && o+sz <= GOF(YMM14)+SZB(YMM14)) return GOF(YMM14);
   if (o >= GOF(YMM15) && o+sz <= GOF(YMM15)+SZB(YMM15)) return GOF(YMM15);
   if (o >= GOF(YMM16) && o+sz <= GOF(YMM16)+SZB(YMM16)) return GOF(YMM16);

   /* MMX accesses to FP regs.  Need to allow for 32-bit references
      due to dirty helpers for frstor etc, which reference the entire
      64-byte block in one go. */
   if (o >= GOF(FPREG[0])
       && o+sz <= GOF(FPREG[0])+SZB(FPREG[0])) return GOF(FPREG[0]);
   if (o >= GOF(FPREG[1])
       && o+sz <= GOF(FPREG[1])+SZB(FPREG[1])) return GOF(FPREG[1]);
   if (o >= GOF(FPREG[2])
       && o+sz <= GOF(FPREG[2])+SZB(FPREG[2])) return GOF(FPREG[2]);
   if (o >= GOF(FPREG[3])
       && o+sz <= GOF(FPREG[3])+SZB(FPREG[3])) return GOF(FPREG[3]);
   if (o >= GOF(FPREG[4])
       && o+sz <= GOF(FPREG[4])+SZB(FPREG[4])) return GOF(FPREG[4]);
   if (o >= GOF(FPREG[5])
       && o+sz <= GOF(FPREG[5])+SZB(FPREG[5])) return GOF(FPREG[5]);
   if (o >= GOF(FPREG[6])
       && o+sz <= GOF(FPREG[6])+SZB(FPREG[6])) return GOF(FPREG[6]);
   if (o >= GOF(FPREG[7])
       && o+sz <= GOF(FPREG[7])+SZB(FPREG[7])) return GOF(FPREG[7]);

   /* Map high halves of %RAX,%RCX,%RDX,%RBX to the whole register.
      This is needed because the general handling of dirty helper
      calls is done in 4 byte chunks.  Hence we will see these.
      Currently we only expect to see artefacts from CPUID. */
   if (o == 4+ GOF(RAX) && sz == 4) return GOF(RAX);
   if (o == 4+ GOF(RCX) && sz == 4) return GOF(RCX);
   if (o == 4+ GOF(RDX) && sz == 4) return GOF(RDX);
   if (o == 4+ GOF(RBX) && sz == 4) return GOF(RBX);

   VG_(printf)("MC_(get_otrack_shadow_offset)(amd64)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

   /* --------------------- x86 --------------------- */

#  elif defined(VGA_x86)

#  define GOF(_fieldname) \
      (offsetof(VexGuestX86State,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestX86State*)0)->guest_##_fieldname))

   Int  o     = offset;
   Int  sz    = szB;
   Bool is124 = sz == 4 || sz == 2 || sz == 1;
   tl_assert(sz > 0);
   tl_assert(host_is_little_endian());

   if (o == GOF(EAX) && is124) return o;
   if (o == GOF(ECX) && is124) return o;
   if (o == GOF(EDX) && is124) return o;
   if (o == GOF(EBX) && is124) return o;
   if (o == GOF(ESP) && is124) return o;
   if (o == GOF(EBP) && is124) return o;
   if (o == GOF(ESI) && is124) return o;
   if (o == GOF(EDI) && is124) return o;

   if (o == GOF(CC_DEP1) && sz == 4) return o;
   if (o == GOF(CC_DEP2) && sz == 4) return o;

   if (o == GOF(CC_OP)   && sz == 4) return -1; /* slot used for %AH */
   if (o == GOF(CC_NDEP) && sz == 4) return -1; /* slot used for %BH */
   if (o == GOF(DFLAG)   && sz == 4) return -1; /* slot used for %CH */
   if (o == GOF(EIP)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(IP_AT_SYSCALL) && sz == 4) return -1; /* slot unused */
   if (o == GOF(IDFLAG)  && sz == 4) return -1; /* slot used for %DH */
   if (o == GOF(ACFLAG)  && sz == 4) return -1; /* slot unused */
   if (o == GOF(TISTART) && sz == 4) return -1; /* slot unused */
   if (o == GOF(TILEN)   && sz == 4) return -1; /* slot unused */
   if (o == GOF(NRADDR)  && sz == 4) return -1; /* slot unused */

   /* Treat %AH, %BH, %CH, %DH as independent registers.  To do this
      requires finding 4 unused 32-bit slots in the second-shadow
      guest state, respectively: CC_OP CC_NDEP DFLAG IDFLAG since none
      of those are tracked. */
   tl_assert(SZB(CC_OP)   == 4);
   tl_assert(SZB(CC_NDEP) == 4);
   tl_assert(SZB(DFLAG)   == 4);
   tl_assert(SZB(IDFLAG)  == 4);
   if (o == 1+ GOF(EAX) && szB == 1) return GOF(CC_OP);
   if (o == 1+ GOF(EBX) && szB == 1) return GOF(CC_NDEP);
   if (o == 1+ GOF(ECX) && szB == 1) return GOF(DFLAG);
   if (o == 1+ GOF(EDX) && szB == 1) return GOF(IDFLAG);

   /* skip XMM and FP admin stuff */
   if (o == GOF(SSEROUND) && szB == 4) return -1;
   if (o == GOF(FTOP)     && szB == 4) return -1;
   if (o == GOF(FPROUND)  && szB == 4) return -1;
   if (o == GOF(EMNOTE)   && szB == 4) return -1;
   if (o == GOF(FC3210)   && szB == 4) return -1;

   /* XMM registers */
   if (o >= GOF(XMM0)  && o+sz <= GOF(XMM0)+SZB(XMM0)) return GOF(XMM0);
   if (o >= GOF(XMM1)  && o+sz <= GOF(XMM1)+SZB(XMM1)) return GOF(XMM1);
   if (o >= GOF(XMM2)  && o+sz <= GOF(XMM2)+SZB(XMM2)) return GOF(XMM2);
   if (o >= GOF(XMM3)  && o+sz <= GOF(XMM3)+SZB(XMM3)) return GOF(XMM3);
   if (o >= GOF(XMM4)  && o+sz <= GOF(XMM4)+SZB(XMM4)) return GOF(XMM4);
   if (o >= GOF(XMM5)  && o+sz <= GOF(XMM5)+SZB(XMM5)) return GOF(XMM5);
   if (o >= GOF(XMM6)  && o+sz <= GOF(XMM6)+SZB(XMM6)) return GOF(XMM6);
   if (o >= GOF(XMM7)  && o+sz <= GOF(XMM7)+SZB(XMM7)) return GOF(XMM7);

   /* MMX accesses to FP regs.  Need to allow for 32-bit references
      due to dirty helpers for frstor etc, which reference the entire
      64-byte block in one go. */
   if (o >= GOF(FPREG[0])
       && o+sz <= GOF(FPREG[0])+SZB(FPREG[0])) return GOF(FPREG[0]);
   if (o >= GOF(FPREG[1])
       && o+sz <= GOF(FPREG[1])+SZB(FPREG[1])) return GOF(FPREG[1]);
   if (o >= GOF(FPREG[2])
       && o+sz <= GOF(FPREG[2])+SZB(FPREG[2])) return GOF(FPREG[2]);
   if (o >= GOF(FPREG[3])
       && o+sz <= GOF(FPREG[3])+SZB(FPREG[3])) return GOF(FPREG[3]);
   if (o >= GOF(FPREG[4])
       && o+sz <= GOF(FPREG[4])+SZB(FPREG[4])) return GOF(FPREG[4]);
   if (o >= GOF(FPREG[5])
       && o+sz <= GOF(FPREG[5])+SZB(FPREG[5])) return GOF(FPREG[5]);
   if (o >= GOF(FPREG[6])
       && o+sz <= GOF(FPREG[6])+SZB(FPREG[6])) return GOF(FPREG[6]);
   if (o >= GOF(FPREG[7])
       && o+sz <= GOF(FPREG[7])+SZB(FPREG[7])) return GOF(FPREG[7]);

   /* skip %GS and other segment related stuff.  We could shadow
      guest_LDT and guest_GDT, although it seems pointless.
      guest_CS .. guest_SS are too small to shadow directly and it
      also seems pointless to shadow them indirectly (that is, in 
      the style of %AH .. %DH). */
   if (o == GOF(CS) && sz == 2) return -1;
   if (o == GOF(DS) && sz == 2) return -1;
   if (o == GOF(ES) && sz == 2) return -1;
   if (o == GOF(FS) && sz == 2) return -1;
   if (o == GOF(GS) && sz == 2) return -1;
   if (o == GOF(SS) && sz == 2) return -1;
   if (o == GOF(LDT) && sz == 4) return -1;
   if (o == GOF(GDT) && sz == 4) return -1;

   VG_(printf)("MC_(get_otrack_shadow_offset)(x86)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

   /* -------------------- s390x -------------------- */

#  elif defined(VGA_s390x)
#  define GOF(_fieldname) \
      (offsetof(VexGuestS390XState,guest_##_fieldname))
   Int  o      = offset;
   Int  sz     = szB;
   tl_assert(sz > 0);
   tl_assert(host_is_big_endian());

   /* no matter what byte(s) we change, we have changed the full 8 byte value
      and need to track this change for the whole register */
   if (o >= GOF(r0) && sz <= 8 && o <= (GOF(r15) + 8 - sz))
      return GOF(r0) + ((o-GOF(r0)) & -8) ;


   /* fprs are accessed 4 or 8 byte at once. Again, we track that change for
      the full register */
   if ((sz == 8 || sz == 4) && o >= GOF(f0) && o <= GOF(f15)+8-sz)
      return GOF(f0) + ((o-GOF(f0)) & -8) ;

   /* access registers are accessed 4 bytes at once */
   if (sz == 4 && o >= GOF(a0) && o <= GOF(a15))
      return o;

   /* we access the guest counter either fully or one of the 4byte words */
   if (o == GOF(counter) && (sz == 8 || sz ==4))
      return o;
   if (o == GOF(counter) + 4 && sz == 4)
      return o;

   if (o == GOF(EMNOTE) && sz == 4) return -1;

   if (o == GOF(CC_OP)    && sz == 8) return -1;
   if (o == GOF(CC_DEP1)  && sz == 8) return o;
   if (o == GOF(CC_DEP2)  && sz == 8) return o;
   if (o == GOF(CC_NDEP)  && sz == 8) return -1;
   if (o == GOF(TISTART)  && sz == 8) return -1;
   if (o == GOF(TILEN)    && sz == 8) return -1;
   if (o == GOF(NRADDR)   && sz == 8) return -1;
   if (o == GOF(IP_AT_SYSCALL) && sz == 8) return -1;
   if (o == GOF(fpc)      && sz == 4) return -1;
   if (o == GOF(IA)       && sz == 8) return -1;
   if (o == (GOF(IA) + 4) && sz == 4) return -1;
   if (o == GOF(SYSNO)    && sz == 8) return -1;
   VG_(printf)("MC_(get_otrack_shadow_offset)(s390x)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF


   /* --------------------- arm --------------------- */

#  elif defined(VGA_arm)

#  define GOF(_fieldname) \
      (offsetof(VexGuestARMState,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestARMState*)0)->guest_##_fieldname))

   Int  o     = offset;
   Int  sz    = szB;
   tl_assert(sz > 0);
   tl_assert(host_is_little_endian());

   if (o == GOF(R0)  && sz == 4) return o;
   if (o == GOF(R1)  && sz == 4) return o;
   if (o == GOF(R2)  && sz == 4) return o;
   if (o == GOF(R3)  && sz == 4) return o;
   if (o == GOF(R4)  && sz == 4) return o;
   if (o == GOF(R5)  && sz == 4) return o;
   if (o == GOF(R6)  && sz == 4) return o;
   if (o == GOF(R7)  && sz == 4) return o;
   if (o == GOF(R8)  && sz == 4) return o;
   if (o == GOF(R9)  && sz == 4) return o;
   if (o == GOF(R10) && sz == 4) return o;
   if (o == GOF(R11) && sz == 4) return o;
   if (o == GOF(R12) && sz == 4) return o;
   if (o == GOF(R13) && sz == 4) return o;
   if (o == GOF(R14) && sz == 4) return o;

   /* EAZG: These may be completely wrong. */
   if (o == GOF(R15T)  && sz == 4) return -1; /* slot unused */
   if (o == GOF(CC_OP) && sz == 4) return -1; /* slot unused */

   if (o == GOF(CC_DEP1) && sz == 4) return o;
   if (o == GOF(CC_DEP2) && sz == 4) return o;

   if (o == GOF(CC_NDEP) && sz == 4) return -1; /* slot unused */

   if (o == GOF(QFLAG32) && sz == 4) return o;

   if (o == GOF(GEFLAG0) && sz == 4) return o;
   if (o == GOF(GEFLAG1) && sz == 4) return o;
   if (o == GOF(GEFLAG2) && sz == 4) return o;
   if (o == GOF(GEFLAG3) && sz == 4) return o;

   //if (o == GOF(SYSCALLNO)     && sz == 4) return -1; /* slot unused */
   //if (o == GOF(CC)     && sz == 4) return -1; /* slot unused */
   //if (o == GOF(EMNOTE)     && sz == 4) return -1; /* slot unused */
   //if (o == GOF(TISTART)     && sz == 4) return -1; /* slot unused */
   //if (o == GOF(NRADDR)     && sz == 4) return -1; /* slot unused */

   if (o == GOF(FPSCR)    && sz == 4) return -1;
   if (o == GOF(TPIDRURO) && sz == 4) return -1;
   if (o == GOF(ITSTATE)  && sz == 4) return -1;

   /* Accesses to F or D registers */
   if (sz == 4 || sz == 8) {
      if (o >= GOF(D0)  && o+sz <= GOF(D0) +SZB(D0))  return GOF(D0);
      if (o >= GOF(D1)  && o+sz <= GOF(D1) +SZB(D1))  return GOF(D1);
      if (o >= GOF(D2)  && o+sz <= GOF(D2) +SZB(D2))  return GOF(D2);
      if (o >= GOF(D3)  && o+sz <= GOF(D3) +SZB(D3))  return GOF(D3);
      if (o >= GOF(D4)  && o+sz <= GOF(D4) +SZB(D4))  return GOF(D4);
      if (o >= GOF(D5)  && o+sz <= GOF(D5) +SZB(D5))  return GOF(D5);
      if (o >= GOF(D6)  && o+sz <= GOF(D6) +SZB(D6))  return GOF(D6);
      if (o >= GOF(D7)  && o+sz <= GOF(D7) +SZB(D7))  return GOF(D7);
      if (o >= GOF(D8)  && o+sz <= GOF(D8) +SZB(D8))  return GOF(D8);
      if (o >= GOF(D9)  && o+sz <= GOF(D9) +SZB(D9))  return GOF(D9);
      if (o >= GOF(D10) && o+sz <= GOF(D10)+SZB(D10)) return GOF(D10);
      if (o >= GOF(D11) && o+sz <= GOF(D11)+SZB(D11)) return GOF(D11);
      if (o >= GOF(D12) && o+sz <= GOF(D12)+SZB(D12)) return GOF(D12);
      if (o >= GOF(D13) && o+sz <= GOF(D13)+SZB(D13)) return GOF(D13);
      if (o >= GOF(D14) && o+sz <= GOF(D14)+SZB(D14)) return GOF(D14);
      if (o >= GOF(D15) && o+sz <= GOF(D15)+SZB(D15)) return GOF(D15);
      if (o >= GOF(D16) && o+sz <= GOF(D16)+SZB(D16)) return GOF(D16);
      if (o >= GOF(D17) && o+sz <= GOF(D17)+SZB(D17)) return GOF(D17);
      if (o >= GOF(D18) && o+sz <= GOF(D18)+SZB(D18)) return GOF(D18);
      if (o >= GOF(D19) && o+sz <= GOF(D19)+SZB(D19)) return GOF(D19);
      if (o >= GOF(D20) && o+sz <= GOF(D20)+SZB(D20)) return GOF(D20);
      if (o >= GOF(D21) && o+sz <= GOF(D21)+SZB(D21)) return GOF(D21);
      if (o >= GOF(D22) && o+sz <= GOF(D22)+SZB(D22)) return GOF(D22);
      if (o >= GOF(D23) && o+sz <= GOF(D23)+SZB(D23)) return GOF(D23);
      if (o >= GOF(D24) && o+sz <= GOF(D24)+SZB(D24)) return GOF(D24);
      if (o >= GOF(D25) && o+sz <= GOF(D25)+SZB(D25)) return GOF(D25);
      if (o >= GOF(D26) && o+sz <= GOF(D26)+SZB(D26)) return GOF(D26);
      if (o >= GOF(D27) && o+sz <= GOF(D27)+SZB(D27)) return GOF(D27);
      if (o >= GOF(D28) && o+sz <= GOF(D28)+SZB(D28)) return GOF(D28);
      if (o >= GOF(D29) && o+sz <= GOF(D29)+SZB(D29)) return GOF(D29);
      if (o >= GOF(D30) && o+sz <= GOF(D30)+SZB(D30)) return GOF(D30);
      if (o >= GOF(D31) && o+sz <= GOF(D31)+SZB(D31)) return GOF(D31);
   }

   /* Accesses to Q registers */
   if (sz == 16) {
      if (o >= GOF(D0)  && o+sz <= GOF(D0) +2*SZB(D0))  return GOF(D0);  // Q0
      if (o >= GOF(D2)  && o+sz <= GOF(D2) +2*SZB(D2))  return GOF(D2);  // Q1
      if (o >= GOF(D4)  && o+sz <= GOF(D4) +2*SZB(D4))  return GOF(D4);  // Q2
      if (o >= GOF(D6)  && o+sz <= GOF(D6) +2*SZB(D6))  return GOF(D6);  // Q3
      if (o >= GOF(D8)  && o+sz <= GOF(D8) +2*SZB(D8))  return GOF(D8);  // Q4
      if (o >= GOF(D10) && o+sz <= GOF(D10)+2*SZB(D10)) return GOF(D10); // Q5
      if (o >= GOF(D12) && o+sz <= GOF(D12)+2*SZB(D12)) return GOF(D12); // Q6
      if (o >= GOF(D14) && o+sz <= GOF(D14)+2*SZB(D14)) return GOF(D14); // Q7
      if (o >= GOF(D16) && o+sz <= GOF(D16)+2*SZB(D16)) return GOF(D16); // Q8
      if (o >= GOF(D18) && o+sz <= GOF(D18)+2*SZB(D18)) return GOF(D18); // Q9
      if (o >= GOF(D20) && o+sz <= GOF(D20)+2*SZB(D20)) return GOF(D20); // Q10
      if (o >= GOF(D22) && o+sz <= GOF(D22)+2*SZB(D22)) return GOF(D22); // Q11
      if (o >= GOF(D24) && o+sz <= GOF(D24)+2*SZB(D24)) return GOF(D24); // Q12
      if (o >= GOF(D26) && o+sz <= GOF(D26)+2*SZB(D26)) return GOF(D26); // Q13
      if (o >= GOF(D28) && o+sz <= GOF(D28)+2*SZB(D28)) return GOF(D28); // Q14
      if (o >= GOF(D30) && o+sz <= GOF(D30)+2*SZB(D30)) return GOF(D30); // Q15
   }

   if (o == GOF(TISTART) && sz == 4) return -1;
   if (o == GOF(TILEN)   && sz == 4) return -1;

   VG_(printf)("MC_(get_otrack_shadow_offset)(arm)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

   /* --------------------- mips32 --------------------- */

#  elif defined(VGA_mips32)

#  define GOF(_fieldname) \
      (offsetof(VexGuestMIPS32State,guest_##_fieldname))
#  define SZB(_fieldname) \
      (sizeof(((VexGuestMIPS32State*)0)->guest_##_fieldname))

   Int  o     = offset;
   Int  sz    = szB;
   tl_assert(sz > 0);
#  if defined (VG_LITTLEENDIAN)
   tl_assert(host_is_little_endian());
#  elif defined (VG_BIGENDIAN)
   tl_assert(host_is_big_endian());
#  else
#     error "Unknown endianness"
#  endif

   if (o == GOF(r0)  && sz == 4) return o;
   if (o == GOF(r1)  && sz == 4) return o;
   if (o == GOF(r2)  && sz == 4) return o;
   if (o == GOF(r3)  && sz == 4) return o;
   if (o == GOF(r4)  && sz == 4) return o;
   if (o == GOF(r5)  && sz == 4) return o;
   if (o == GOF(r6)  && sz == 4) return o;
   if (o == GOF(r7)  && sz == 4) return o;
   if (o == GOF(r8)  && sz == 4) return o;
   if (o == GOF(r9)  && sz == 4) return o;
   if (o == GOF(r10)  && sz == 4) return o;
   if (o == GOF(r11)  && sz == 4) return o;
   if (o == GOF(r12)  && sz == 4) return o;
   if (o == GOF(r13)  && sz == 4) return o;
   if (o == GOF(r14)  && sz == 4) return o;
   if (o == GOF(r15)  && sz == 4) return o;
   if (o == GOF(r16)  && sz == 4) return o;
   if (o == GOF(r17)  && sz == 4) return o;
   if (o == GOF(r18)  && sz == 4) return o;
   if (o == GOF(r19)  && sz == 4) return o;
   if (o == GOF(r20)  && sz == 4) return o;
   if (o == GOF(r21)  && sz == 4) return o;
   if (o == GOF(r22)  && sz == 4) return o;
   if (o == GOF(r23)  && sz == 4) return o;
   if (o == GOF(r24)  && sz == 4) return o;
   if (o == GOF(r25)  && sz == 4) return o;
   if (o == GOF(r26)  && sz == 4) return o;
   if (o == GOF(r27)  && sz == 4) return o;
   if (o == GOF(r28)  && sz == 4) return o;
   if (o == GOF(r29)  && sz == 4) return o;
   if (o == GOF(r30)  && sz == 4) return o;
   if (o == GOF(r31)  && sz == 4) return o;
   if (o == GOF(PC)  && sz == 4) return -1; /* slot unused */

   if (o == GOF(HI)  && sz == 4) return o;
   if (o == GOF(LO)  && sz == 4) return o;

   if (o == GOF(FIR)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(FCCR)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(FEXR)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(FENR)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(FCSR)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(ULR) && sz == 4) return -1;

   if (o == GOF(EMNOTE)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(TISTART)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(TILEN)     && sz == 4) return -1; /* slot unused */
   if (o == GOF(NRADDR)     && sz == 4) return -1; /* slot unused */

   if (o >= GOF(f0)  && o+sz <= GOF(f0) +SZB(f0))  return GOF(f0);
   if (o >= GOF(f1)  && o+sz <= GOF(f1) +SZB(f1))  return GOF(f1);
   if (o >= GOF(f2)  && o+sz <= GOF(f2) +SZB(f2))  return GOF(f2);
   if (o >= GOF(f3)  && o+sz <= GOF(f3) +SZB(f3))  return GOF(f3);
   if (o >= GOF(f4)  && o+sz <= GOF(f4) +SZB(f4))  return GOF(f4);
   if (o >= GOF(f5)  && o+sz <= GOF(f5) +SZB(f5))  return GOF(f5);
   if (o >= GOF(f6)  && o+sz <= GOF(f6) +SZB(f6))  return GOF(f6);
   if (o >= GOF(f7)  && o+sz <= GOF(f7) +SZB(f7))  return GOF(f7);
   if (o >= GOF(f8)  && o+sz <= GOF(f8) +SZB(f8))  return GOF(f8);
   if (o >= GOF(f9)  && o+sz <= GOF(f9) +SZB(f9))  return GOF(f9);
   if (o >= GOF(f10) && o+sz <= GOF(f10)+SZB(f10)) return GOF(f10);
   if (o >= GOF(f11) && o+sz <= GOF(f11)+SZB(f11)) return GOF(f11);
   if (o >= GOF(f12) && o+sz <= GOF(f12)+SZB(f12)) return GOF(f12);
   if (o >= GOF(f13) && o+sz <= GOF(f13)+SZB(f13)) return GOF(f13);
   if (o >= GOF(f14) && o+sz <= GOF(f14)+SZB(f14)) return GOF(f14);
   if (o >= GOF(f15) && o+sz <= GOF(f15)+SZB(f15)) return GOF(f15);

   if (o >= GOF(f16) && o+sz <= GOF(f16)+SZB(f16)) return GOF(f16);
   if (o >= GOF(f17)  && o+sz <= GOF(f17) +SZB(f17))  return GOF(f17);
   if (o >= GOF(f18)  && o+sz <= GOF(f18) +SZB(f18))  return GOF(f18);
   if (o >= GOF(f19)  && o+sz <= GOF(f19) +SZB(f19))  return GOF(f19);
   if (o >= GOF(f20)  && o+sz <= GOF(f20) +SZB(f20))  return GOF(f20);
   if (o >= GOF(f21)  && o+sz <= GOF(f21) +SZB(f21))  return GOF(f21);
   if (o >= GOF(f22)  && o+sz <= GOF(f22) +SZB(f22))  return GOF(f22);
   if (o >= GOF(f23)  && o+sz <= GOF(f23) +SZB(f23))  return GOF(f23);
   if (o >= GOF(f24)  && o+sz <= GOF(f24) +SZB(f24))  return GOF(f24);
   if (o >= GOF(f25)  && o+sz <= GOF(f25) +SZB(f25))  return GOF(f25);
   if (o >= GOF(f26) && o+sz <= GOF(f26)+SZB(f26)) return GOF(f26);
   if (o >= GOF(f27) && o+sz <= GOF(f27)+SZB(f27)) return GOF(f27);
   if (o >= GOF(f28) && o+sz <= GOF(f28)+SZB(f28)) return GOF(f28);
   if (o >= GOF(f29) && o+sz <= GOF(f29)+SZB(f29)) return GOF(f29);
   if (o >= GOF(f30) && o+sz <= GOF(f30)+SZB(f30)) return GOF(f30);
   if (o >= GOF(f31) && o+sz <= GOF(f31)+SZB(f31)) return GOF(f31);

   if ((o > GOF(NRADDR)) && (o <= GOF(NRADDR) +12 )) return -1; /*padding registers*/

   VG_(printf)("MC_(get_otrack_shadow_offset)(mips)(off=%d,sz=%d)\n",
               offset,szB);
   tl_assert(0);
#  undef GOF
#  undef SZB

#  else
#    error "FIXME: not implemented for this architecture"
#  endif
}


/* Let 'arr' describe an indexed reference to a guest state section
   (guest state array).

   This function returns the corresponding guest state type to be used
   when indexing the corresponding array in the second shadow (origin
   tracking) area.  If the array is not to be origin-tracked, return
   Ity_INVALID.

   This function must agree with MC_(get_otrack_shadow_offset) above.
   See comments at the start of MC_(get_otrack_shadow_offset).
*/
IRType MC_(get_otrack_reg_array_equiv_int_type) ( IRRegArray* arr )
{
   /* -------------------- ppc64 -------------------- */
#  if defined(VGA_ppc64)
   /* The redir stack. */
   if (arr->base == offsetof(VexGuestPPC64State,guest_REDIR_STACK[0])
       && arr->elemTy == Ity_I64
       && arr->nElems == VEX_GUEST_PPC64_REDIR_STACK_SIZE)
      return Ity_I64;

   VG_(printf)("get_reg_array_equiv_int_type(ppc64): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

   /* -------------------- ppc32 -------------------- */
#  elif defined(VGA_ppc32)
   /* The redir stack. */
   if (arr->base == offsetof(VexGuestPPC32State,guest_REDIR_STACK[0])
       && arr->elemTy == Ity_I32
       && arr->nElems == VEX_GUEST_PPC32_REDIR_STACK_SIZE)
      return Ity_I32;

   VG_(printf)("get_reg_array_equiv_int_type(ppc32): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

   /* -------------------- amd64 -------------------- */
#  elif defined(VGA_amd64)
   /* Ignore the FP tag array - pointless to shadow, and in any case
      the elements are too small */
   if (arr->base == offsetof(VexGuestAMD64State,guest_FPTAG)
       && arr->elemTy == Ity_I8 && arr->nElems == 8)
      return Ity_INVALID;

   /* The FP register array */
   if (arr->base == offsetof(VexGuestAMD64State,guest_FPREG[0])
       && arr->elemTy == Ity_F64 && arr->nElems == 8)
      return Ity_I64;

   VG_(printf)("get_reg_array_equiv_int_type(amd64): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

   /* --------------------- x86 --------------------- */
#  elif defined(VGA_x86)
   /* Ignore the FP tag array - pointless to shadow, and in any case
      the elements are too small */
   if (arr->base == offsetof(VexGuestX86State,guest_FPTAG)
       && arr->elemTy == Ity_I8 && arr->nElems == 8)
      return Ity_INVALID;

   /* The FP register array */
   if (arr->base == offsetof(VexGuestX86State,guest_FPREG[0])
       && arr->elemTy == Ity_F64 && arr->nElems == 8)
      return Ity_I64;

   VG_(printf)("get_reg_array_equiv_int_type(x86): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

   /* --------------------- arm --------------------- */
#  elif defined(VGA_arm)
   VG_(printf)("get_reg_array_equiv_int_type(arm): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

   /* --------------------- s390x --------------------- */
#  elif defined(VGA_s390x)
   /* Should never het here because s390x does not use Ist_PutI
      and Iex_GetI. */
   tl_assert(0);

/* --------------------- mips32 --------------------- */
#  elif defined(VGA_mips32)
   VG_(printf)("get_reg_array_equiv_int_type(mips32): unhandled: ");
   ppIRRegArray(arr);
   VG_(printf)("\n");
   tl_assert(0);

#  else
#    error "FIXME: not implemented for this architecture"
#  endif
}


/*--------------------------------------------------------------------*/
/*--- end                                             mc_machine.c ---*/
/*--------------------------------------------------------------------*/
