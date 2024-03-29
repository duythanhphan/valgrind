/* -*- mode: C; c-basic-offset: 3; -*- */

#include <stdio.h>    // fprintf
#include <stdlib.h>   // exit
#include "vtest.h"

#define DEFOP(op,ukind) op, #op, ukind

/* The opcodes appear in the same order here as in libvex_ir.h
   That is not necessary but helpful when supporting a new architecture.
*/
static irop_t irops[] = {
  { DEFOP(Iop_Add8,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Add16,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Add32,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Add64,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Sub8,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Sub16,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Sub32,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Sub64,   UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_Mul8,    UNDEF_LEFT), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_Mul16,   UNDEF_LEFT), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_Mul32,   UNDEF_LEFT), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Mul64,   UNDEF_LEFT), .s390x = 0, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_Or8,     UNDEF_OR),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Or16,    UNDEF_OR),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Or32,    UNDEF_OR),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Or64,    UNDEF_OR),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_And8,    UNDEF_AND),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_And16,   UNDEF_AND),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_And32,   UNDEF_AND),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_And64,   UNDEF_AND),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Xor8,    UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Xor16,   UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Xor32,   UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Xor64,   UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Shl8,    UNDEF_SHL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Shl16,   UNDEF_SHL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Shl32,   UNDEF_SHL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Shl64,   UNDEF_SHL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_Shr8,    UNDEF_SHR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc32/64 assert
  { DEFOP(Iop_Shr16,   UNDEF_SHR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc32/64 assert
  { DEFOP(Iop_Shr32,   UNDEF_SHR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Shr64,   UNDEF_SHR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_Sar8,    UNDEF_SAR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc32/64 assert
  { DEFOP(Iop_Sar16,   UNDEF_SAR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc32/64 assert
  { DEFOP(Iop_Sar32,   UNDEF_SAR),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Sar64,   UNDEF_SAR),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 1, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_CmpEQ8,  UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CmpEQ16, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CmpEQ32, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpEQ64, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_CmpNE8,  UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CmpNE16, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CmpNE32, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpNE64, UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_Not8,       UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Not16,      UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Not32,      UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Not64,      UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_CasCmpEQ8,  UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpEQ16, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpEQ32, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpEQ64, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },

  { DEFOP(Iop_CasCmpNE8,  UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpNE16, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpNE32, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CasCmpNE64, UNDEF_NONE), .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_ExpCmpNE8,  UNDEF_UNKNOWN), }, // exact (expensive) equality
  { DEFOP(Iop_ExpCmpNE16, UNDEF_UNKNOWN), }, // exact (expensive) equality
  { DEFOP(Iop_ExpCmpNE32, UNDEF_UNKNOWN), }, // exact (expensive) equality
  { DEFOP(Iop_ExpCmpNE64, UNDEF_UNKNOWN), }, // exact (expensive) equality
  { DEFOP(Iop_MullS8,     UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MullS16,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MullS32,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  // s390 has signed multiplication of 64-bit values but the result
  // is 64-bit (not 128-bit). So we cannot test this op standalone.
  { DEFOP(Iop_MullS64,    UNDEF_LEFT), .s390x = 0, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_MullU8,     UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MullU16,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MullU32,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_MullU64,    UNDEF_LEFT), .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_Clz64,      UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_Clz32,      UNDEF_ALL),  .s390x = 0, .amd64 = 0, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_Ctz64,      UNDEF_ALL),  .s390x = 0, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_Ctz32,      UNDEF_ALL),  .s390x = 0, .amd64 = 0, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CmpLT32S,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpLT64S,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc, mips assert
  { DEFOP(Iop_CmpLE32S,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpLE64S,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // ppc, mips assert
  { DEFOP(Iop_CmpLT32U,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpLT64U,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_CmpLE32U,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_CmpLE64U,   UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_CmpNEZ8,    UNDEF_ALL), },   // not supported by mc_translate
  { DEFOP(Iop_CmpNEZ16,   UNDEF_ALL), },   // not supported by mc_translate
  { DEFOP(Iop_CmpNEZ32,   UNDEF_ALL), },   // not supported by mc_translate
  { DEFOP(Iop_CmpNEZ64,   UNDEF_ALL), },   // not supported by mc_translate
  { DEFOP(Iop_CmpwNEZ32,  UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_CmpwNEZ64,  UNDEF_ALL),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Left8,      UNDEF_UNKNOWN), },  // not supported by mc_translate
  { DEFOP(Iop_Left16,     UNDEF_UNKNOWN), },  // not supported by mc_translate
  { DEFOP(Iop_Left32,     UNDEF_UNKNOWN), },  // not supported by mc_translate
  { DEFOP(Iop_Left64,     UNDEF_UNKNOWN), },  // not supported by mc_translate
  { DEFOP(Iop_Max32U,     UNDEF_UNKNOWN), },  // not supported by mc_translate
  { DEFOP(Iop_CmpORD32U,  UNDEF_ORD), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // support added in vbit-test
  { DEFOP(Iop_CmpORD64U,  UNDEF_ORD), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // support added in vbit-test
  { DEFOP(Iop_CmpORD32S,  UNDEF_ORD), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // support added in vbit-test
  { DEFOP(Iop_CmpORD64S,  UNDEF_ORD), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // support added in vbit-test
  { DEFOP(Iop_DivU32,     UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_DivS32,     UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_DivU64,     UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_DivS64,     UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_DivU64E,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_DivS64E,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32 asserts
  { DEFOP(Iop_DivU32E,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_DivS32E,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  // On s390 the DivMod operations always appear in a certain context
  // So they cannot be tested in isolation on that platform.
  { DEFOP(Iop_DivModU64to32,  UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_DivModS64to32,  UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_DivModU128to64, UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_DivModS128to64, UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_DivModS64to64,  UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_8Uto16,    UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1,  .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_8Uto32,    UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_8Uto64,    UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_16Uto32,   UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_16Uto64,   UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_32Uto64,   UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_8Sto16,    UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_8Sto32,    UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_8Sto64,    UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_16Sto32,   UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_16Sto64,   UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_32Sto64,   UNDEF_SEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_64to8,     UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 1, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_32to8,     UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_64to16,    UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_16to8,     UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_16HIto8,   UNDEF_UPPER),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_8HLto16,   UNDEF_CONCAT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },  // ppc isel
  { DEFOP(Iop_32to16,    UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  { DEFOP(Iop_32HIto16,  UNDEF_UPPER),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_16HLto32,  UNDEF_CONCAT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },  // ppc isel
  { DEFOP(Iop_64to32,    UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_64HIto32,  UNDEF_UPPER),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_32HLto64,  UNDEF_CONCAT), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_128to64,   UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_128HIto64, UNDEF_UPPER),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_64HLto128, UNDEF_CONCAT), .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_Not1,      UNDEF_ALL),    .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_32to1,     UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_64to1,     UNDEF_TRUNC),  .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_1Uto8,     UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_1Uto32,    UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_1Uto64,    UNDEF_ZEXT),   .s390x = 1, .amd64 = 1, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 0, .mips32 = 0 }, // ppc32, mips assert
  { DEFOP(Iop_1Sto8,     UNDEF_ALL),    .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_1Sto16,    UNDEF_ALL), }, // not handled by mc_translate
  { DEFOP(Iop_1Sto32,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_1Sto64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_AddF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_SubF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_MulF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_DivF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_AddF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_SubF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_MulF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_DivF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_AddF64r32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_SubF64r32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_MulF64r32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_DivF64r32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_NegF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_AbsF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_NegF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_AbsF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_SqrtF64,   UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_SqrtF32,   UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_CmpF64,    UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_CmpF32,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_CmpF128,   UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F64toI16S, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F64toI32S, UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_F64toI64S, UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_F64toI64U, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_F64toI32U, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_I32StoF64, UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_I64StoF64, UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_I64UtoF64, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_I64UtoF32, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_I32UtoF32, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I32UtoF64, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toI32S, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toI64S, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toI32U, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toI64U, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I32StoF32, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_I64StoF32, UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toF64,  UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_F64toF32,  UNDEF_ALL), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_ReinterpF64asI64, UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_ReinterpI64asF64, UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_ReinterpF32asI32, UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 1, .ppc32 = 1, .mips32 = 1 },
  // ppc requires this op to show up in a specific context. So it cannot be
  // tested standalone on that platform.
  { DEFOP(Iop_ReinterpI32asF32, UNDEF_SAME), .s390x = 1, .amd64 = 1, .x86 = 1, .arm = 1, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_F64HLtoF128, UNDEF_CONCAT),    .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128HItoF64, UNDEF_UPPER),     .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128LOtoF64, UNDEF_TRUNC), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_AddF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_SubF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MulF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_DivF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_NegF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_AbsF128,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_SqrtF128,      UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I32StoF128,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I64StoF128,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I32UtoF128,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_I64UtoF128,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F32toF128,     UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F64toF128,     UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toI32S,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toI64S,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toI32U,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toI64U,    UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toF64,     UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_F128toF32,     UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_AtanF64,       UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_Yl2xF64,       UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_Yl2xp1F64,     UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_PRemF64,       UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_PRemC3210F64,  UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_PRem1F64,      UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_PRem1C3210F64, UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_ScaleF64,      UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_SinF64,        UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_CosF64,        UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_TanF64,        UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_2xm1F64,       UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_RoundF64toInt, UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_RoundF32toInt, UNDEF_ALL), .s390x = 0, .amd64 = 1, .x86 = 1, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 1 },
  { DEFOP(Iop_MAddF32,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MSubF32,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 0, .ppc32 = 0, .mips32 = 0 },
  { DEFOP(Iop_MAddF64,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_MSubF64,       UNDEF_ALL), .s390x = 1, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_MAddF64r32,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_MSubF64r32,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_Est5FRSqrt,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_RoundF64toF64_NEAREST, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_RoundF64toF64_NegINF,  UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_RoundF64toF64_PosINF,  UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_RoundF64toF64_ZERO,    UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },
  { DEFOP(Iop_TruncF64asF32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 }, // mips asserts
  { DEFOP(Iop_RoundF64toF32, UNDEF_ALL), .s390x = 0, .amd64 = 0, .x86 = 0, .arm = 0, .ppc64 = 1, .ppc32 = 1, .mips32 = 0 },

  /* ------------------ 32-bit SIMD Integer ------------------ */
  { DEFOP(Iop_QAdd32S, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub32S, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add16x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub16x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HAdd16Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HAdd16Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HSub16Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HSub16Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add8x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub8x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HAdd8Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HAdd8Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HSub8Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_HSub8Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sad8Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ16x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ8x4, UNDEF_UNKNOWN), },
  /* ------------------ 64-bit SIMD FP ------------------------ */
  { DEFOP(Iop_I32UtoFx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_I32StoFx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_FtoI32Ux2_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_FtoI32Sx2_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F32ToFixed32Ux2_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F32ToFixed32Sx2_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Fixed32UToF32x2_RN, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Fixed32SToF32x2_RN, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGE32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recps32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrte32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrts32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Neg32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs32Fx2, UNDEF_UNKNOWN), },
  /* ------------------ 64-bit SIMD Integer. ------------------ */
  { DEFOP(Iop_CmpNEZ8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd64Ux1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd64Sx1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub64Ux1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub64Sx1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PolynomialMul8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulHi16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulHi32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QRDMulHi16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QRDMulHi32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cnt8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal64x1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl64x1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal64x1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN64Sx1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN64x1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN64x1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin16Sto8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin16Sto8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin32Sto16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowBin16to8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowBin32to16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveOddLanes8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveEvenLanes8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveOddLanes16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveEvenLanes16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatOddLanes8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatOddLanes16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatEvenLanes8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatEvenLanes16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SetElem8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SetElem16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SetElem32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Extract64, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse16_8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse32_8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse32_16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Perm8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetMSBs8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrte32x2, UNDEF_UNKNOWN), },
  /* ------------------ Decimal Floating Point ------------------ */
  { DEFOP(Iop_AddD64,                UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_SubD64,                UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_MulD64,                UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_DivD64,                UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_AddD128,               UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_SubD128,               UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_MulD128,               UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_DivD128,               UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ShlD64,                UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ShrD64,                UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ShlD128,               UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ShrD128,               UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D32toD64,              UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D64toD128,             UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_I32StoD128,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_I32UtoD128,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_I64StoD128,            UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_I64UtoD128,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D64toD32,              UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D128toD64,             UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_I32StoD64,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_I32UtoD64,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_I64StoD64,             UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_I64UtoD64,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D64toI32S,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D64toI32U,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D64toI64S,             UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D64toI64U,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D128toI64S,            UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D128toI64U,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D128toI32S,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_D128toI32U,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_RoundD64toInt,         UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_RoundD128toInt,        UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_CmpD64,                UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_CmpD128,               UNDEF_ALL),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_CmpExpD64,             UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_CmpExpD128,            UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_QuantizeD64,           UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_QuantizeD128,          UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_SignificanceRoundD64,  UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_SignificanceRoundD128, UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ExtractExpD64,         UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ExtractExpD128,        UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ExtractSigD64,         UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_ExtractSigD128,        UNDEF_ALL),  .s390x = 1, .ppc64 = 0, .ppc32 = 0 },
  { DEFOP(Iop_InsertExpD64,          UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_InsertExpD128,         UNDEF_ALL),  .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D64HLtoD128,           UNDEF_CONCAT), .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D128HItoD64,           UNDEF_UPPER),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_D128LOtoD64,           UNDEF_TRUNC),  .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_DPBtoBCD,              UNDEF_ALL),    .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_BCDtoDPB,              UNDEF_ALL),    .s390x = 0, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ReinterpI64asD64,      UNDEF_SAME),   .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  { DEFOP(Iop_ReinterpD64asI64,      UNDEF_SAME),   .s390x = 1, .ppc64 = 1, .ppc32 = 1 },
  /* ------------------ 128-bit SIMD FP. ------------------ */
  { DEFOP(Iop_Add32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLT32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLE32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpUN32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGE32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMax32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwMin32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RSqrt32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Neg32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recps32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrte32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrts32Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_I32UtoFx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_I32StoFx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_FtoI32Ux4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_FtoI32Sx4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QFtoI32Ux4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QFtoI32Sx4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RoundF32x4_RM, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RoundF32x4_RP, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RoundF32x4_RN, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RoundF32x4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F32ToFixed32Ux4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F32ToFixed32Sx4_RZ, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Fixed32UToF32x4_RN, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Fixed32SToF32x4_RN, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F32toF16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_F16toF32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLT32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLE32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpUN32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RSqrt32F0x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLT64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLE64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpUN64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RSqrt64Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLT64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpLE64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpUN64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RSqrt64F0x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V128to64, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V128HIto64, UNDEF_UNKNOWN), },
  { DEFOP(Iop_64HLtoV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_64UtoV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SetV128lo64, UNDEF_UNKNOWN), },
  { DEFOP(Iop_32UtoV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V128to32, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SetV128lo32, UNDEF_UNKNOWN), },
  /* ------------------ 128-bit SIMD Integer. ------------------ */
  { DEFOP(Iop_NotV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_AndV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_OrV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_XorV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd64Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QAdd64Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub64Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSub64Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MulHi32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MullEven8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MullEven16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MullEven8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_MullEven16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mull32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulHi16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulHi32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QRDMulHi16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QRDMulHi32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulLong16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QDMulLong32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PolynomialMul8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PolynomialMull8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAdd32Fx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_PwAddL32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Abs32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Avg32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpEQ64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT64Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpGT32Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cnt8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Clz32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Cls32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShlN64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ShrN64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_SarN64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shl64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Shr64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sar64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sal64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rol8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rol16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rol32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShl64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSal64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN32Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN64Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QShlN64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QSalN64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin16Sto8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin32Sto16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin16Sto8Sx16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin32Sto16Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin16Uto8Ux16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowBin32Uto16Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowBin16to8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowBin32to16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowUn16to8x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowUn32to16x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NarrowUn64to32x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn16Sto8Sx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn32Sto16Sx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn64Sto32Sx2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn16Sto8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn32Sto16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn64Sto32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn16Uto8Ux8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn32Uto16Ux4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_QNarrowUn64Uto32Ux2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen8Uto16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen16Uto32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen32Uto64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen8Sto16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen16Sto32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Widen32Sto64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveHI64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveLO64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveOddLanes8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveEvenLanes8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveOddLanes16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveEvenLanes16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveOddLanes32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_InterleaveEvenLanes32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatOddLanes8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatOddLanes16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatOddLanes32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatEvenLanes8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatEvenLanes16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CatEvenLanes32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetElem64x2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Dup32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_ExtractV128, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse16_8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse32_8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse32_16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_16x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Reverse64_32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Perm8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Perm32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_GetMSBs8x16, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32x4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Rsqrte32x4, UNDEF_UNKNOWN), },
  /* ------------------ 256-bit SIMD Integer. ------------------ */
  { DEFOP(Iop_V256to64_0, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V256to64_1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V256to64_2, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V256to64_3, UNDEF_UNKNOWN), },
  { DEFOP(Iop_64x4toV256, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V256toV128_0, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V256toV128_1, UNDEF_UNKNOWN), },
  { DEFOP(Iop_V128HLtoV256, UNDEF_UNKNOWN), },
  { DEFOP(Iop_AndV256, UNDEF_UNKNOWN), },
  { DEFOP(Iop_OrV256,  UNDEF_UNKNOWN), },
  { DEFOP(Iop_XorV256, UNDEF_UNKNOWN), },
  { DEFOP(Iop_NotV256, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ32x8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_CmpNEZ64x4, UNDEF_UNKNOWN), },
  /* ------------------ 256-bit SIMD FP. ------------------ */
  { DEFOP(Iop_Add64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Add32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sub32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Mul32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Div32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Sqrt64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_RSqrt32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Recip32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min32Fx8, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Max64Fx4, UNDEF_UNKNOWN), },
  { DEFOP(Iop_Min64Fx4, UNDEF_UNKNOWN), },
};


/* Return a descriptor for OP, iff it exists and it is implemented
   for the current architecture. */
irop_t *
get_irop(IROp op)
{
   unsigned i;

   for (i = 0; i < sizeof irops / sizeof *irops; ++i) {
      irop_t *p = irops + i;
      if (p->op == op) {
#ifdef __s390x__
#define S390X_FEATURES "../../../tests/s390x_features"
         switch (op) {
         case Iop_I32StoD64:    // CDFTR
         case Iop_I32StoD128:   // CXFTR
         case Iop_I32UtoD64:    // CDLFTR
         case Iop_I32UtoD128:   // CXLFTR
         case Iop_I64UtoD64:    // CDLGTR
         case Iop_I64UtoD128:   // CXLGTR
         case Iop_D64toI32S:    // CFDTR
         case Iop_D128toI32S:   // CFXTR
         case Iop_D64toI64U:    // CLGDTR
         case Iop_D64toI32U:    // CLFDTR
         case Iop_D128toI64U:   // CLGXTR
         case Iop_D128toI32U:   // CLFXTR
         case Iop_I32UtoF32:
         case Iop_I32UtoF64:
         case Iop_I32UtoF128:
         case Iop_I64UtoF32:
         case Iop_I64UtoF64:
         case Iop_I64UtoF128:
         case Iop_F32toI32U:
         case Iop_F32toI64U:
         case Iop_F64toI32U:
         case Iop_F64toI64U:
         case Iop_F128toI32U:
         case Iop_F128toI64U: {
            int rc;
            /* These IROps require the floating point extension facility */
            rc = system(S390X_FEATURES " s390x-fpext");
            // s390x_features returns 1 if feature does not exist
            rc /= 256;
            if (rc != 0) return NULL;
         }
         }
         return p->s390x ? p : NULL;
#endif
#ifdef __x86_64__
         return p->amd64 ? p : NULL;
#endif
#ifdef __powerpc__
#ifdef __powerpc64__
         return p->ppc64 ? p : NULL;
#else
         return p->ppc32 ? p : NULL;
#endif
#endif
#ifdef __mips__
         return p->mips32 ? p : NULL;
#endif
#ifdef __arm__
         return p->arm ? p : NULL;
#endif
#ifdef __i386__
         return p->x86 ? p : NULL;
#endif
         return NULL;
      }
   }

   fprintf(stderr, "unknown opcode %d\n", op);
   exit(1);
}
