/*****************************************************************************
Copyright (c) 2006, BlueChip of Cyborg Systems
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

# Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

# Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

# Neither the name of the Cyborg Systems nor the names of its contributors may
  be used to endorse or promote products derived from this software without
  specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

// some quick botches to get it to compile as it stands

typedef   signed char   I8;
typedef unsigned char  UI8;
typedef unsigned short UI16;
//typedef unsigned char  bool;
#define true  (1)
#define false (0)
#define BYTE  (1)
#define WORD  (2)

#define QQ 0 // timings where M or M1 is unknown

// PF states the use of fP (flag.PV) by the last instr
// The uses listed here are the GENERAL case
// See individual instr's for detailed information
typedef enum  ParityFlagUseage
{
  // Identity    Op          Meaning
  pfPARITY,   // Logic       1 if number of bits set in result is EVEN
  pfOVERFLOW, // Math        1 if sub result < 0, or add result > 255
  pfREPEAT,   // Block       1 if block operation is about to repeat
  pfZERO,     // BIT         a copy of fZ
  pfSIGN,     // NEG         a copy of fS
  pfINT,      // LD A,[I|R]  a copy of IFF2
  pfUNKNOWN,  // ...         if (PV>=UNKNOWN) *all* flags have unknown origin
  pfPOP,      // POP AF      Whatever was on the stack
  pfEXAF,     // EX  AF,AF'  Whatever was in F'
}
PFLAG;




// Z80 internal stuff
typedef   struct  Z80_CPU
{
// Z80 Registers
  UI8   r[14];      // [0]->{B  C  D  E  H  L  F  A  I  R  IXh IXl IYh IYl}
  UI16  rUpd;       // bit mask of registers updated by last instr

  UI8   r_[8];      // [0]->{B' C' D' E' H' L' F' A'}
  UI16  rUpd_;      // bit mask of registers updated by last instr

  UI16  rp[8];      // [0]->{BC, DE, HL, SP, AF, PC, IX, IY}
  UI16  rpUpd;      // bit mask of registers updated by last instr

  // By default HL, these flags are updated by 0xDD and 0xFD prefixes
  UI8   idxHL;      // 0xDD->IX,  0xFD->IY,  else HL
  UI8   idxH;       // 0xDD->IXh, 0xFD->IYh, else H
  UI8   idxL;       // 0xDD->IXl, 0xFD->IYl, else  L

// Z80 flags
// NOTE: Flags are often "misused", so the uses here are the 'general' case.
// See individual instr's for detailed information
  bool  fC;         // 0 - Carry                 : Carry or Rotate bit
  bool  fN;         // 1 - (Negative)/"Subtract" : Last math 0=Add, 1=Subtract
  bool  fP;         // 2 - See PFLAG notes       : aka "flag.pv"
  bool  f3;         // 3 - bit3 of result        : Undocumented
  bool  fH;         // 4 - Half-carry            : Carry into bit-4
  bool  f5;         // 5 - bit5 of result        : Undocumented
  bool  fS;         // 6 - Sign                  : Last Math result 0=+,  1=-
  bool  fZ;         // 7 - Zero                  : Last Math result 0=!0, 1=0

  UI8   fUpd;       // bit set for each flag affected

  PFLAG PF;         // fP useage by last instr (see typedef of PFLAG)

// Interrupt details
  bool  HALT;       // Is the processor halted?
  bool  IFF1;       // Intr flip-flop #1 - true -> Accept Maskable-Intr's
  bool  IFF2;       // Intr flip-flop #2 - a copy of IFF1 during an NMI
  UI8   IM;         // Interrupt mode {0,1,2}
  bool  IHLD;       // Temp disable of Intr's for this instr [EI or NONI]

  bool  NMI;        // Set this to fire an NMI
  bool  INT;        // Set this to fire a maskable-INT

  UI8   DBUS;       // Data bus holds the data required by IM 0 and IM 2

// Information regarding the last executed instruction
// Feedback for a soft-ICE / Internal use
  UI16  oldPC;      // Program Counter BEFORE exec  (rp[PC] is PC AFTER exec)

  UI8   ibc;        // InstructionByteCount
  UI8   op[4];      // Opcode

  UI16  memUpdAddr; // Memory Update Address  (Address of 1st byte written)
  UI8   memUpdSz;   // Memory Update Size     (0 if no update occurred)
  UI16  memUpdVal;  // Memory Update Value    (value written to memory)

  UI8   M;          // M-cycles
  UI8   M1;         // M1-cycles
  UI8   T;          // T-states

  // This is primarily to detect if a debugger steps out of a block instr
  bool  rpt;        // 'true' iff a rpt instr has rewound the PC (Ie. LDIR)
  UI16  rptPC;      // iff rpt==true then rpcPC==addr of 1st byte of rpt instr

// Function pointers for bus activity
  // Each processor has it's own Address, Data and I/O bus
  // The programmer will supply functions to handle the busses
  UI8   (*peek8)  (UI16);       // read    8bit value from 16bit addr (/MREQ)
  void  (*poke8)  (UI16, UI8);  // write   8bit value to   16bit addr (/MREQ)

  // Although the Z80 is an 8 bit processor with 8 bit I/O
  // A15...A8 *do* have a value put on them during I/O intructions
  // So the first parameter is a (possibly unexpected) 16-bit value
  // See individual instr's for detailed information
  UI8   (*in8)    (UI16);       // input   8bit value from 16bit port (/IORQ)
  void  (*out8)   (UI16, UI8);  // output  8bit value to   16bit port (/IORQ)
}
Z80CPU;



// PEEK16 is two peek8 operations - Z80 is LITTLE-endian (that is, LSB first)
#define PEEK16(addr)  \
  (Z->peek8(addr) | (((UI16)Z->peek8((addr)+1))<<8))
#define POKE16(addr,val)  \
  do{ Z->poke8(addr, (val)&0xFF); Z->poke8((addr)+1, (val)>>8); }while(0)

// 8-bit registers, opcode order  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define B     (0)
#define C     (1)
#define D     (2)
#define E     (3)
#define H     (4)
#define L     (5)
#define AT_HL (6)       // (HL) is not a register
#define F     (6)       // ...so r[6] is used to hold the Flags
#define A     (7)
// ... Other 8-bit registers  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define I     (8)
#define R     (9)
#define IXh   (10)
#define IXl   (11)
#define IYh   (12)
#define IYl   (13)

// Translate H,L -> H,L/IXh,IXl,/IYh,IYl  -=[ DO _NOT_ EDIT THIS ]=-
#define rXY(x)  ( ((x)==H) ? (Z->idxH) : ( ((x)==L) ? (Z->idxL) : (x) ) )

// 16-bit registers, opcode order  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define BC    (0)
#define DE    (1)
#define HL    (2)
#define SP    (3)   // For PUSH/POP instr's: IF rp==3 THEN rp=4   [SP -> AF]
#define AF    (4)   // For PUSH/POP instr's: IF rp==3 THEN rp=4   [SP -> AF]
// ... other 16-bit registers
#define PC    (5)
#define IX    (6)
#define IY    (7)

// -=[ DO _NOT_ EDIT THIS ]=-
#define RP2(x)  ( ((x)==3) ? 4 : (x) )    // Used by PUSH/POP  [SP -> AF]

// Translate HL -> HL/IX/IY  -=[ DO _NOT_ EDIT THIS ]=-
#define rpXY(x) ( ((x)==HL) ? Z->idxHL : (x) )


// 8bit register update masks  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define mB    (1<<B)
#define mC    (1<<C)
#define mD    (1<<D)
#define mE    (1<<E)
#define mH    (1<<H)
#define mL    (1<<L)
#define mA    (1<<A)
#define mF    (1<<F)

#define mI    (1<<I)
#define mR    (1<<R)

#define mIXh  (1<<IXh)
#define mIXl  (1<<IXl)
#define mIYh  (1<<IYh)
#define mIYl  (1<<IYl)

// 16bit register update masks  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define mBC   (1<<BC)
#define mDE   (1<<DE)
#define mHL   (1<<HL)
#define mSP   (1<<SP)
#define mAF   (1<<AF)

#define mPC   (1<<PC)
#define mIX   (1<<IX)
#define mIY   (1<<IY)



// Convert opcode register identity to register mask
#define mROP(x)   (1<<(x))

// Given (HL, H, L) will ensure HL and H & L are synchronised
// Where "HL" is from the list:
//  { BC [B,C], DE [D,E], HL [H,L], AF [A,F], IX [IXh,IXl], IY [IYh, IYl] }
// There HAS to be a nicer way to do this!
#define SYNCH(_RP, _Rh, _Rl)   \
  do                                                                \
  {                                                                 \
    if (Z->rpUpd &m##_RP)                   /* RP Updated */        \
    {                                                               \
      if ((Z->rp[_RP]>>8) != Z->r[_Rh])     /* RP-h updated */      \
      {                                                             \
        Z->r[_Rh] = Z->rp[_RP]>>8;                                  \
        REGUPD(m##_Rh);                                             \
      }                                                             \
      if ((Z->rp[_RP]&0xFF) != Z->r[_Rl])   /* RP-l updated */      \
      {                                                             \
        Z->r[_Rl] = Z->rp[_RP]&0xFF;                                \
        REGUPD(m##_Rl);                                             \
      }                                                             \
    }                                                               \
    else if (Z->rUpd &(m##_Rh|m##_Rl))      /* Rh or Rl updated */  \
    {                                                               \
      Z->rp[_RP] = (((UI16)Z->r[_Rh])<<8) | Z->r[_Rl];              \
      R16UPD(m##_RP);                                               \
    }                                                               \
  }while(0)

typedef struct  Displacement
{
  UI8   n;  // Byte value from RAM   - signed displacement as 2's
  I8    d;  // Signed displacement   - sign extended 'abs' value
  bool  s;  // sign 0==+ 1==-        - displacement sign
  UI8   u;  // UNsigned displacement - unsigned displacement
}
DIS;

// 2's complement a DISplacement
#define TWOs(x) \
  do                            \
  {                             \
    x.s = x.n>>7;               \
    x.u = x.s?((~x.n)+1):x.n;   \
    x.d = x.s?(-x.u):x.u;       \
  } while(0)

// Retrieve 'd' from the opcode if 0xDD or 0xFD prefix is active

#define XYGETD  \
  do                                  \
  {                                   \
    if (Z->idxHL!=HL)                 \
    {                                 \
      d.n = NEXTBYTE; /* //-!-m */    \
      TIMING(QQ,QQ,QQ);               \
    }                                 \
    else                              \
    {                                 \
      d.n = 0;                        \
    }                                 \
    TWOs(d);                          \
  }while(0)

// Flag bits  -=[ DO _NOT_ EDIT THESE VALUES ]=-
// Actual flag uses vary from the general case listed here
// See individual instr's for detailed information
#define mfC   (0x01)    // Carry
#define mfN   (0x02)    // (Negative)/"Subtract"
#define mfP   (0x04)    // See PFLAG
#define mf3   (0x08)    // bit3 of result ("undocumented")
#define mfH   (0x10)    // Half-carry
#define mf5   (0x20)    // bit5 of result ("undocumented")
#define mfS   (0x40)    // Sign
#define mfZ   (0x80)    // Zero

// Condition codes, opcode order  -=[ DO _NOT_ EDIT THESE VALUES ]=-
#define ccNZ  (0)       // Non-Zero    (fZ==0)
#define ccZ   (1)       // Zero        (fZ==1)
#define ccNC  (2)       // No-Carry    (fC==0)
#define ccC   (3)       // Carry       (fC==1)
#define ccPO  (4)       // Parity-Odd  (fP==1)
#define ccPE  (5)       // Parity-Even (fP==0)
#define ccP   (6)       // Plus        (fS==0)
#define ccM   (7)       // Minus       (fS==1)



// Condition Met, JPcc, CALLcc, RETcc (not JRcc)
// result = q !^ flag    [XNOR]
// Best case: Z/NZ => 1 if  ... Worst case: PE/PO/M/P => 3 if's
#define CC_MET(_pp,_q)  \
  (!((_q)^                                                     \
    (((_pp)==0) ? (Z->fZ) :                                    \
                  (((_pp)==1) ? (Z->fC) :                      \
                                (((_pp)==2) ? (Z->fP) :        \
                                              (Z->fS)) ) ) ) )

// Gives '1' iff the sum of bits set in the 8-bit value 'x' is an EVEN number
#define PAR8(x)       ((0x9669>>(((x)^((x)>>4))&0xF))&1)

// Used during flag setting operations
#define SIGN(x)       ((x)>>7)
#define ZERO(x)       ((x)?0:1)
#define BIT3(x)       (((x)>>3)&1)
#define BIT5(x)       (((x)>>5)&1)
#define BIT1(x)       ((x)&1)       // A couple of instr's put this in f5

// Tell the user that a register was updated
// Also used internally to keep 8bit-reg's & 16bit-reg-pairs matched
#define REGUPD(r)     do {  Z->rUpd  |= (r);  }while(0)
#define REGUPD_(r)    do {  Z->rUpd_ |= (r);  }while(0)
#define R16UPD(r)     do {  Z->rpUpd |= (r);  }while(0)

// Tell the user which flags were affected
#define FLGUPD(f)     do {  Z->fUpd |= (f);  }while(0)

// Swap two values without using a temporary variable
#define SWAP(x,y)     do {  (x) ^= (y);  (y) ^= (x);  (x) ^= (y);  }while(0)

// Tell user about updated memory
#define MEMUPD(_addr,_sz,_val)   \
  do {                                                                    \
    Z->memUpdAddr = _addr;                                                \
    Z->memUpdSz   = _sz;  /* if (sz) then { memory update occurred } */   \
    Z->memUpdVal  = _val;                                                 \
  }while(0)

// Tell the user about the timing details of the instrcution
#define TIMING(_m,_m1,_t)  \
  do {              \
    Z->M  += _m;    \
    Z->M1 += _m1;   \
    Z->T  += _t;    \
  }while(0)

// Defines the length of the mnemonic, so operands align in the output
#define MNEMs   "%-6s"

// This will read the next Byte or Little-Endian-Word **of an instruction**
// and increment the InstructionByteCounter
#define NEXTBYTE  (Z->peek8(Z->rp[PC]+(Z->ibc++)))
#define NEXTWORD  (NEXTBYTE|(((UI16)NEXTBYTE)<<8))



typedef enum  Z80ModusOperandi
{
  zEXEC,
  zRESET,
}
Z80MODE;

bool    z80(Z80CPU* Z, Z80MODE mode);
void 	z80_debug(Z80CPU *z);
