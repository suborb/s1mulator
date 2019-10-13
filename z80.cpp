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

/*****************************************************************************
Bibliogrpahy
""""""""""""

BSD License generator:
http://www.opensource.org/licenses/bsd-license.php

Instructions by opcode with T, M & M1 states
http://z80.info/z80sean.txt

Parity Algorithm from:
http://graphics.stanford.edu/~seander/bithacks.html#ParityParallel

Thanks to YetiFoot on freenode/#asm for his spectrum emulator source code
to use as a reference work.
In accordance with GPL, no code from his work was used.
(well, I suppose that would be tricky as it wasn't written in C!)

Z80 Flag info (including undocumented flags 3 & 5) from:
http://www.z80.info/z80sflag.htm

Instruction decode Algorithm from:
http://z80.info/decoding.htm

Useful stuff, such as DAA algorithm, undocumented flags, interrupts, etc
http://www.worldofspectrum.org/faq/reference/z80reference.htm#DAA

Undocumented (by Zilog) info "Z80-Undocumented" (v0.91):
http://www.myquest.nl/nowind/?q=node/10

Detailed interrupt information
http://z80.info/interrup.htm
*****************************************************************************/

/*****************************************************************************
The 0xDD and 0xFD 'prefixes'
""""""""""""""""""""""""""""

By means of an explanation, Imagine there is an undocumented register in the
CPU called IDX, which may have one of three values; namely HL, IX or IY.

When an instruction is told to use HL, H or L, it actually uses the register
specified by INDEX.

If IDX==HL  {  (HL) -> (HL)  ,  HL -> HL,  H -> H  ,  L -> L    }
If IDX==IX  {  (IX) -> (IX+d),  HL -> IX,  H -> IXh,  L -> IXl  }
If IDX==IY  {  (IY) -> (IY+d),  HL -> IY,  H -> IYh,  L -> IYl  }

Exceptions to this rule are "EX DE,HL" and ALL instructions prefixed with 0xED

Because (HL) becomes (IX+d) an extra opcode byte is required to specify the
8-bit 2's complement offset 'd'.  Note that "JP (HL)" is a actually "JP HL"
and therefore becomes "JP IX", NOT "JP (IX+d)"

On cpu-reset, IDX is set to HL

We will now need two (not three) instructions to modifiy the IDX register.

Mnemonically:  LD IDX,IX   and   LD IDX,IY

These 'instructions' require 1 M-State, 1 M1-State and 4 T-States.  The
mythical IDX register is set appropriately, the R register is incremented
(by one) and NO other registers are affected; NO flags are affected; NO I/O
occurs; and NO memory read or write occurs.

The opcode for "LD IDX,IX" is 0xDD, and
the opcode for "LD IDX,IY" is 0xFD

We now have TWO problems to consider:

1. How do we (re)set IDX to HL (without a cpu-reset)?

   With the exception of our two new instructions, EVERY other instruction
   will (re)set IDX to HL ...AFTER the instruction is executed and BEFORE
   interrupts are handled.

2. If an interrupt occurs AFTER the DD/FD prefix (aka. "LD IDX,IX/IY") and
   BEFORE the affected instruction, by the time the interrupt is finished and
   execution continues, IDX will have been (re)set to HL, so the affected
   instruction will mistakenly use HL, instead of the intended IX/IY.

   To solve this problem, the DD/FD prefixes share a characteristic with the
   EI instruction.  That is, interrupt checking does NOT occur at the end of
   the DD/FD instruction-cycle.

   This means that if you execute a long stream of DD's or FD's, each one will
   appear to behave identically to a NOP instruction +plus+ interrupts will
   NOT be checked until the cpu has (re)set IDX to HL.  Some documents refer
   to this situation as "NONI", No Operation, No Interrupt.

   Interrupts will NOT occur  if  IDX!=HL  (or, if you prefer)
   Interrupts can ONLY occur when IDX==HL
*****************************************************************************/

/*****************************************************************************
Emulator Notes
""""""""""""""

*1 : Flags are unaffected by all LD operations
     WITH THE EXCEPTION OF:
       0xED,57 : LD A,I   and
       0xED,5F : LD A,R

   * Things which need to be checked because documentation is not clear

Release Notes
"""""""""""""
There is some missing M, M1 & T information

RFSH / r[R]  is not implemented

http://homepage.ntlworld.com/cyborgsystems/s1mp3/z80.c

IM 0  is not implemented

Will need to seperate out "instruction decode with comment" from
"execute instruction" for a debugger.

If anybody actually uses this code, let me know.
If you spot any bugs, or want me to implement some feature that is not here
then contact me and it will be made so.

BC  csbluechip@gmail.com
*****************************************************************************/


/*****************************************************************************
Version History
"""""""""""""""
Ver       Date (ymd)  Who     Comment
--------- ----------- ------- -----------------------------------------------
00.00.01  2006-08-08  BC      First release
00.00.02  2006-08-14  BC      LOTS of fixes in code & commenting
                              Test harness written.  Generates 'test.s'
                              Made a start on code comments
00.00.03  2006-08-14  BC      Bunch more work on output and comments
-----------------------------------------------------------------------------
*****************************************************************************/


#include  <string.h>
#include  <stdio.h>

#include  <time.h>


//#include  "rnd.h"
#define rndSeed(x)
#define rnd() (0xA5A5)



#include  "z80.h"

char    is[50];
char    cmnt[50];

char  PFs[] = "PVRZSI?KX";

// For creating mnemonic names  -=[ DO _NOT_ EDIT THESE STRINGS ]=-
const   char* rs[14]  = {"B","C","D","E","H","L","(HL)","A",
                         "I","R","IXh","IXl","IYh","IYl"};
const   char* rps[8]  = {"BC","DE","HL","SP","AF","PC","IX","IY"};

// For the user to name the registers
const   char* rsu[14] = {"B","C","D","E","H","L","F","A",
                         "I","R","IXh","IXl","IYh","IYl"};


// For creating mnemonic names  -=[ DO _NOT_ EDIT THESE STRINGS ]=-
const   char* ccs[8] = {"NZ","Z","NC","C","PO","PE","P","M"};

// This is a table of all instructions affected by IDX
// There simply wasn't enough of a pattern to do it effectively in logic
// +-------------------------------------------------+
// |                   60 70                         |
// |       21          61 71                   E1    |
// |       22          62 72                         |
// |       23          63 73                   E3    |
// |       24 34 44 54 64 74 84 94 A4 B4 -- --       |
// |       25 35 45 55 65 75 85 95 A5 B5       E5    |
// |       26 36 46 56 66    86 96 A6 B6             |
// |                   67 77                         |
// |                   68                            |
// | 09 19 29 39       69                      E9 F9 |
// |       2A          6A                            |
// |       2B          6B                CB          |
// |       2C    4C 5C 6C 7C 8C 9C AC BC             |
// |       2D    4D 5D 6D 7D 8D 9D AD BD             |
// |       2E    4E 5E 6E 7E 8E 9E AE BE             |
// |                   6F                            |
// +-------------------------------------------------+
// if (ixy[n&0xF][n>>4])  then  instr is effected
#define o (false)
#define M (true)
const bool  ixy[16][16] =
{
// 0- 1- 2- 3- 4- 5- 6- 7- 8- 9- A- B- C- D- E- F-
  {o ,o ,o ,o ,o ,o ,M ,M ,o ,o ,o ,o ,o ,o ,o ,o }, // -0
  {o ,o ,M ,o ,o ,o ,M ,M ,o ,o ,o ,o ,o ,o ,M ,o }, // -1
  {o ,o ,M ,o ,o ,o ,M ,M ,o ,o ,o ,o ,o ,o ,o ,o }, // -2
  {o ,o ,M ,o ,o ,o ,M ,M ,o ,o ,o ,o ,o ,o ,M ,o }, // -3
  {o ,o ,M ,M ,M ,M ,M ,M ,M ,M ,M ,M ,o ,o ,o ,o }, // -4
  {o ,o ,M ,M ,M ,M ,M ,M ,M ,M ,M ,M ,o ,o ,M ,o }, // -5
  {o ,o ,M ,M ,M ,M ,M ,o ,M ,M ,M ,M ,o ,o ,o ,o }, // -6
  {o ,o ,o ,o ,o ,o ,M ,M ,o ,o ,o ,o ,o ,o ,o ,o }, // -7
  {o ,o ,o ,o ,o ,o ,M ,o ,o ,o ,o ,o ,o ,o ,o ,o }, // -8
  {M ,M ,M ,M ,o ,o ,M ,o ,o ,o ,o ,o ,o ,o ,M ,M }, // -9
  {o ,o ,M ,o ,o ,o ,M ,o ,o ,o ,o ,o ,o ,o ,o ,o }, // -A
  {o ,o ,M ,o ,o ,o ,M ,o ,o ,o ,o ,o ,M ,o ,o ,o }, // -B
  {o ,o ,M ,o ,M ,M ,M ,M ,M ,M ,M ,M ,o ,o ,o ,o }, // -C
  {o ,o ,M ,o ,M ,M ,M ,M ,M ,M ,M ,M ,o ,o ,o ,o }, // -D
  {o ,o ,M ,o ,M ,M ,M ,M ,M ,M ,M ,M ,o ,o ,o ,o }, // -E
  {o ,o ,o ,o ,o ,o ,M ,o ,o ,o ,o ,o ,o ,o ,o ,o }  // -F
};
#undef  o
#undef  M

//+***************************************************************************
// label() - returns the label at a given address,
//           or, if no label is found, the address as a string-label
//****************************************************************************

char*   label(UI16 addr)
{

// this is just a placeholder function to get z80() to return valid code.

static  char  s[7];

  sprintf(s, "l_%04X", addr);

  return(s);

};


//+***************************************************************************
// z80(Z80, mode)
// Z80 is a pointer to an instance of a Z80CPU
// mode: zEXEC, zRESET
// returns 'true' if a valid instruction was executed
//****************************************************************************

bool    z80(Z80CPU* Z, Z80MODE mode)
{

// Used internally to evaluate opcodes
UI8     ib;             // Instruction Byte
UI8     x, y, z, p, q;  // Instruction decomposition {xx-yyy-zzz, xx-ppq-zzz}

UI8     pre;            // Most recent prefix byte

UI8     n;              // 8-bit unsigned number
DIS     d;              // Displacement for JR, (IX+d) etc.
UI16    nn;             // 16-bit unsigned number

bool    b;              // General Purpose use
bool    ret;            // Return value

// Search the code for "=-ALUN-=" for more info
#define mAluN   (0xC7)  // Mask
#define pAluN   (0xC6)  // ...and pattern to identify an "alu[y],n"
#define bAluN   (0x40)  // Bit mask to convert "alu[y],n" <-> "alu[y],r[z]"
bool    fAluN = false;  // Flag => "true" iff ((ib & mAluN) == pAluN)

// IM 0 needs to be able to identify an RST command on the databus
#define chkRST(x)  ((((x)&0xC7)==0xC7)?true:false)

// Reset an instance of a Z80CPU
// I suppose the C++ guys would call this a "constructor"
  if (mode==zRESET)
  {
    // PC resets to 0
    Z->rp[PC] = 0x0000;

    // Clear 0xDD/0xFD prefix
    Z->idxHL = HL;
    Z->idxH  = H;
    Z->idxL  = L;

    // Default Interrupt Mode is 0
    Z->HALT  = false;
    Z->IFF1  = 0;
    Z->IFF2  = 0;
    Z->IM    = 0;
    Z->IHLD  = false;
    Z->NMI   = false;
    Z->INT   = false;

    // AF and SP init to 0xFFFF
    Z->rp[SP] = 0xFFFF;
    Z->rp[AF] = 0xFFFF;  Z->r[A] = Z->rp[AF]>>8;  Z->r[F] = Z->rp[AF]&0xFF;
    Z->fC = 1;
    Z->fN = 1;
    Z->fP = 1;  Z->PF = pfUNKNOWN;
    Z->f3 = 1;
    Z->fH = 1;
    Z->f5 = 1;
    Z->fS = 1;
    Z->fZ = 1;

    Z->r[I] = 0;
    Z->r[R] = 0;

    // Stick random crap in all other registers
    rndSeed(time(NULL));
    Z->rp[BC] = rnd();  Z->r[B]   = Z->rp[BC]>>8;  Z->r[C]   = Z->rp[BC]&0xFF;
    Z->rp[DE] = rnd();  Z->r[D]   = Z->rp[DE]>>8;  Z->r[E]   = Z->rp[DE]&0xFF;
    Z->rp[HL] = rnd();  Z->r[H]   = Z->rp[HL]>>8;  Z->r[L]   = Z->rp[HL]&0xFF;
    Z->rp[IX] = rnd();  Z->r[IXh] = Z->rp[IX]>>8;  Z->r[IXl] = Z->rp[IX]&0xFF;
    Z->rp[IY] = rnd();  Z->r[IYh] = Z->rp[IY]>>8;  Z->r[IYl] = Z->rp[IY]&0xFF;
    Z->r_[B] = (UI8)rnd();
    Z->r_[C] = (UI8)rnd();
    Z->r_[D] = (UI8)rnd();
    Z->r_[E] = (UI8)rnd();
    Z->r_[H] = (UI8)rnd();
    Z->r_[L] = (UI8)rnd();
    Z->r_[A] = (UI8)rnd();
    Z->r_[F] = (UI8)rnd();

    // No repeating/block instructions are active (Ie. LDIR)
    Z->rpt = false;

    // No previous instruction
    Z->oldPC = 0x0000;
    Z->ibc   = 0;

    Z->memUpdSz = 0;

    Z->M  = 0;
    Z->M1 = 0;
    Z->T  = 0;

    TIMING(QQ,QQ,3);

    return(true);
  }

// Make sure we have not stepped outside a repeating instr mid-repeat
  if (Z->rpt && (Z->rptPC!=Z->rp[PC]))  Z->rpt = false;

// Initialise return values

  // No memory Update (yet)
  MEMUPD(0,0,0);

  // No register updates (yet)
  // Do NOT use the REGUPD, REGUPD_ or R16UPD macros here! (they use |=, not =)
  Z->rUpd  = 0;
  Z->rUpd_ = 0;
  Z->rpUpd = 0;

  // No flags affected (yet)
  // Do NOT use the FLGUPD macro here! (it uses use |=, not =)
  Z->fUpd  = 0;

  // No instruction encountered (yet)
  // Do NOT use the TIMING macro here! (it uses +=, not =)
  Z->M  = 0;
  Z->M1 = 0;
  Z->T  = 0;

  // Zero the instruction byte-counter
  Z->ibc = 0;

  // No instruction decoded (yet)
  is[0]   = '\0';
  cmnt[0] = '\0';


// Intialise local variables

  // Release intr's from temp hold by EI or NONI
  Z->IHLD = false;

  // If the processor HALTed, force a HALT
  if (Z->HALT)
  {
    ib = 0x76;  // HALT
  }
  else  // Read and decode an instruction from memory
  {
    // Grab the (first byte of the) instruction
    ib = NEXTBYTE;                                                               //-!-m : First Memory Access

    // Search the code for "=-ALUN-=" for more info
    //
    // OK, I'm probably gonna get slapped senseless for this by a bunch of
    // purists... But as I always say: You've got the source code; if you
    // don't like the way I've done it, feel free to recode it ;)
    //
    // -- 10-yyy-zzz : alu[y],r[z]   and   -- 11-yyy-110 : alu[y],n
    // are essentially the same operations with a different source operand.
    // So... This is what I've done:
    // # Patch up  "aly[y],n"  to look like  "alu[y],(HL)"
    //   by clearing bit 6 of the opcode
    // # Set a flag to say we've done it (aka "fAluN")
    // # The switch nest will then hit  "alu[y],r[z]"
    //   ...and check the flag
    // # If the flag is set we know to:
    //   # correct the opcode - probably not necessary, but (imho) polite
    //   # read the NEXTBYTE into 'n' for the source operand
    // Everything should run cleanly form there :)
    if ((ib & mAluN) == pAluN)
    {
      ib    &= ~bAluN;  // clear bit-6
      fAluN  = true;    // say we've done it
    }

    // Check for 0xCB and 0xED prefix bytes
    if ((ib==0xCB)||(ib==0xED))
    {
      pre = ib;
      if (Z->idxHL!=HL)   // 0xDD (IX) or 0xFD (IY) prefix is active
      {
        if (pre==0xCB)    // DD-CB and FD-CB have 'd' before 'ib'
        {
          d.n = NEXTBYTE;                                                        //-!-m
          TWOs(d);
          ib = NEXTBYTE;                                                         //-!-m
        }
        else              // DD-ED and FD-ED reduce to NONI
        {
          ib = 0x00;      // ...force a NONI
        }
      }
      else                // no 0xDD (IX) or 0xFD (IY) prefix
      {
        ib  = NEXTBYTE;                                                          //-!-m
      }
    }
    else
    {
      pre = 0x00;
    }

    // decompose the main instruction to  xx-yyy-zzz
    x  = (ib &0xC0) >>6;
    y  = (ib &0x38) >>3;
    z  = (ib &0x07);

    // yyy -> ppq
    p  = y >>1;
    q  = y &1;

  }//endif(HALT)

  // Maintain a copy of the source address for the instruction
  Z->oldPC = Z->rp[PC];

// Process the instruction
  switch (pre)
  {
    case 0x00:  // No Prefix ................................ -- ?? xx-yyy-zzz
      switch (x)
      {
        case 0: // .......................................... -- ?? 00-yyy-zzz
          switch (z)
          {
            case 0: // Relative jumps and assorted ops ...... -- ?? 00-yyy-000
              switch (y)
              {
                case 0: // .................................. -- 00 00-000-000 : NOP
                  // NOP == "No OPeration"/Do nothing                            //-!-
                  // Registers and Memory are unaffected
                  // Flags are unaffected
                  sprintf(is, MNEMs, "NOP");
                  TIMING(1,1,4);
                  strcpy(cmnt, ".");
                  break;
                case 1: // .................................. -- 08 00-001-000 : EX AF,AF'
                  SWAP(Z->r[A], Z->r_[A]);                                       //-!-
                  SWAP(Z->r[F], Z->r_[F]);                                       //-!-
                  Z->fC = Z->r[F]&mfC?1:0;                                       //-!-
                  Z->fN = Z->r[F]&mfN?1:0;                                       //-!-
                  Z->fP = Z->r[F]&mfP?1:0;  Z->PF = pfEXAF;                      //-!-
                  Z->f3 = Z->r[F]&mf3?1:0;                                       //-!-
                  Z->fH = Z->r[F]&mfH?1:0;                                       //-!-
                  Z->f5 = Z->r[F]&mf5?1:0;                                       //-!-
                  Z->fS = Z->r[F]&mfS?1:0;                                       //-!-
                  Z->fZ = Z->r[F]&mfZ?1:0;                                       //-!-
                  FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  REGUPD_(mA | mF);
                  sprintf(is, MNEMs"%s", "EX", "AF,AF\'");
                  TIMING(1,1,4);
                  strcpy(cmnt, "<-> A,F");
                  break;
                case 2: // .................................. -- 10 00-010-000 : DJNZ d
                case 3: // .................................. -- 18 00-011-000 : JR   d
                case 4: // .................................. -- 20 00-100-000 : JRNZ d
                case 5: // .................................. -- 28 00-101-000 : JRZ  d
                case 6: // .................................. -- 30 00-110-000 : JRNC d
                case 7: // .................................. -- 38 00-111-000 : JRC  d
                {
                  bool    j;   // set true when jump condition is met
                  switch (y)   // again!
                  {
                    case 2: // .............................. -- 10 00-010-000 : DJNZ d
                      Z->r[B]--;                                                 //-!-
                      j = Z->r[B] ? true : false ;                               //-!-
                      REGUPD(mB);
                      sprintf(is, MNEMs, "DJNZ");
                      TIMING(0,0,1);  // DJNZ requires an extra T-state
                      break;
                    case 3: // .............................. -- 18 00-011-000 : JR d
                      j = true;                                                  //-!-
                      sprintf(is, MNEMs, "JR");
                      break;
                    case 4: // .............................. -- 20 00-100-000 : JR NZ,d
                      j = (Z->fZ) ? false : true ;                               //-!-
                      sprintf(is, MNEMs"%s", "JR", "NZ,");
                      break;
                    case 5: // .............................. -- 28 00-101-000 : JR Z,d
                      j = (Z->fZ) ? true : false ;                               //-!-
                      sprintf(is, MNEMs"%s", "JR", "Z,");
                      break;
                    case 6: // .............................. -- 30 00-110-000 : JR NC,d
                      j = (Z->fC) ? false : true ;                               //-!-
                      sprintf(is, MNEMs"%s", "JR", "NC,");
                      break;
                    case 7: // .............................. -- 38 00-111-000 : JR C,d
                      j = (Z->fC) ? true : false ;                               //-!-
                      sprintf(is, MNEMs"%s", "JR", "C,");
                      break;
                  }
                  d.n = NEXTBYTE;                                                //-!-m
                  TWOs(d);
                  strcat(is, label(Z->rp[PC] +d.d +Z->ibc));
                  sprintf(cmnt, "%c$%02X -> %s", d.s?'-':'+', d.u, label(Z->rp[PC] +d.d +Z->ibc));
                  if (j)
                  {
                    Z->rp[PC] += d.d;       // ibc added AFTER main switch()     //-!-
                    R16UPD(mPC);            // DJNZ updated B
                    TIMING(3,1,12);         // DJNZ added an extra T-state
                  }
                  else
                  {
                    // Jump does not occur
                    TIMING(2,1,7);          // DJNZ added an extra T-state
                  }
                  // Flags are unaffected (by all flow-control operations)
                  break;
                }
              }
              break;

            case 1: // 16-bit load-immediate/add ............ -- ?? 00-ppq-001
              switch (q)
              {
                case 0: // .................................. -- ?? 00-pp0-001 : LD rp[p],nn
                  nn = NEXTWORD;                                                 //-!-mm
                  Z->rp[rpXY(p)] = nn;                                           //-!-
                  R16UPD(mROP(rpXY(p)));
                  // Flags are unaffected (by all LD operations *1)
                  sprintf(is, MNEMs"%s,$%04X", "LD", rps[rpXY(p)], nn);
                  TIMING(3,1,10);
                  sprintf(cmnt, "%s = $%04X", rps[rpXY(p)], nn);
                  break;
                case 1: // .................................. -- ?? 00-pp1-001 : ADD HL,rp[p]
                  // Because 16bit ops are handled as 2 8bit ops
                  // (first LSB, then MSB), f3, f5 & fH are from H, not L
                  Z->fC = (Z->rp[rpXY(p)]>(0xFFFF-Z->rp[Z->idxHL]))?1:0;
                  Z->fH = ((Z->rp[rpXY(p)]&0x0FFF) >
                          (0x0FFF-(Z->rp[Z->idxHL]&0x0FFF)))?1:0;
                  Z->rp[Z->idxHL] += Z->rp[rpXY(p)];                             //-!-
                  Z->f3 = BIT3(Z->rp[Z->idxHL]>>8);
                  Z->f5 = BIT5(Z->rp[Z->idxHL]>>8);
                  Z->fN = 0;
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  R16UPD(mROP(Z->idxHL));
                  REGUPD(mF);
                  sprintf(is, MNEMs"%s,%s",
                              "ADD", rps[Z->idxHL], rps[rpXY(p)]);
                  TIMING(3,1,11);                                                // mm ???
                  sprintf(cmnt, "%s += $%04X", rps[Z->idxHL], Z->rp[rpXY(p)]);
                  break;
              }
              break;

            case 2: // Indirect loading ..................... -- ?? 00-yyy-010
              switch (y)
              {
                case 0: // .................................. -- 02 00-000-010 : LD (BC),A
                  sprintf(cmnt, "($%04X)==$%02X", Z->rp[BC], Z->peek8(Z->rp[BC]));
                  Z->poke8(Z->rp[BC],Z->r[A]);                                   //-!-m
                  MEMUPD(Z->rp[BC], BYTE, Z->r[A]);
                  sprintf(is, MNEMs"%s", "LD", "(BC),A");
                  TIMING(2,1,7);
                  sprintf(cmnt, "%s -> $%02X", cmnt, Z->r[A]);
                  break;
                case 1: // .................................. -- 0A 00-001-010 : LD A,(BC)
                  Z->r[A] = Z->peek8(Z->rp[BC]);                                 //-!-m
                  REGUPD(mA);
                  sprintf(is, MNEMs"%s", "LD", "A,(BC)");
                  TIMING(2,1,7);
                  sprintf(cmnt, "($%04X)==$%02X", Z->rp[BC], Z->peek8(Z->rp[BC]));
                  break;
                case 2: // .................................. -- 12 00-010-010 : LD (DE),A
                  sprintf(cmnt, "($%04X)==$%02X", Z->rp[DE], Z->peek8(Z->rp[DE]));
                  Z->poke8(Z->rp[DE],Z->r[A]);                                   //-!-m
                  MEMUPD(Z->rp[DE], BYTE, Z->r[A]);
                  sprintf(is, MNEMs"%s", "LD", "(DE),A");
                  TIMING(2,1,7);
                  sprintf(cmnt, "%s -> $%02X", cmnt, Z->r[A]);
                  break;
                case 3: // .................................. -- 1A 00-011-010 : LD A,(DE)
                  Z->r[A] = Z->peek8(Z->rp[DE]);                                 //-!-m
                  REGUPD(mA);
                  sprintf(is, MNEMs"%s", "LD", "A,(DE)");
                  TIMING(2,1,7);
                  sprintf(cmnt, "($%04X)==$%02X", Z->rp[DE], Z->peek8(Z->rp[DE]));
                  break;

                case 4: // .................................. -- 22 00-100-010 : LD (nn),HL
                  nn = NEXTWORD;                                                 //-!-mm
                  sprintf(cmnt, "($%04X)==$%02X", nn, PEEK16(nn));
                  POKE16(nn,Z->rp[Z->idxHL]);                                    //-!-mm
                  MEMUPD(nn, WORD, Z->rp[Z->idxHL]);
                  sprintf(is, MNEMs"($%04X),%s", "LD", (nn), rps[Z->idxHL]);
                  TIMING(5,1,16);
                  sprintf(cmnt, "%s -> $%04X", cmnt, Z->rp[Z->idxHL]);
                  break;
                case 5: // .................................. -- 2A 00-101-010 : LD HL,(nn)
                  nn = NEXTWORD;                                                 //-!-mm
                  Z->rp[Z->idxHL] = PEEK16(nn);                                  //-!-mm
                  R16UPD(mROP(Z->idxHL));
                  sprintf(is, MNEMs"%s,($%04X)", "LD", rps[Z->idxHL], (nn));
                  TIMING(5,1,16);
                  sprintf(cmnt, "($%04X)==$%04X", nn, PEEK16(nn));
                  break;

                case 6: // .................................. -- 32 00-110-010 : LD (nn),A
                  nn = NEXTWORD;                                                 //-!-mm
                  sprintf(cmnt, "($%04X)==$%02X", nn, Z->peek8(nn));
                  Z->poke8(nn,Z->r[A]);                                          //-!-m
                  MEMUPD(nn, BYTE, Z->r[A]);
                  sprintf(is, MNEMs"($%04X),%s", "LD", (nn), "A");
                  TIMING(4,1,13);
                  sprintf(cmnt, "%s -> $%02X", cmnt, Z->r[A]);
                  break;
                case 7: // .................................. -- 3A 00-111-010 : LD A,(nn)
                  nn = NEXTWORD;                                                 //-!-mm
                  Z->r[A] = Z->peek8(nn);                                        //-!-m
                  REGUPD(mA);
                  sprintf(is, MNEMs"%s,($%04X)", "LD", "A", (nn));
                  TIMING(4,1,13);
                  sprintf(cmnt, "($%04X)==$%04X", nn, Z->peek8(nn));
                  break;
              }
              // Flags are unaffected (by all LD operations *1)
              break;

            case 3: // INC & DEC rp ......................... -- ?? 00-ppq-011
              if (!q)   // .................................. -- ?? 00-pp0-011 : INC rp[p]
              {
                Z->rp[rpXY(p)]++;                                                //-!-
                sprintf(is, MNEMs"%s", "INC", rps[rpXY(p)]);
                sprintf(cmnt, "%s++", rps[rpXY(p)]);
              }
              else      // .................................. -- ?? 00-pp1-011 : DEC rp[p]
              {
                Z->rp[rpXY(p)]--;                                                //-!-
                sprintf(is, MNEMs"%s", "DEC", rps[rpXY(p)]);
                sprintf(cmnt, "%s--", rps[rpXY(p)]);
              }
              R16UPD(mROP(rpXY(p)));
              // Flags are unaffected (by 16-bit INC & DEC)
              TIMING(1,1,6);
              break;

            case 4: // 8-bit INC ............................ -- ?? 00-yyy-100
              if (y==AT_HL) // .............................. -- 34 00-110-100 : INC (HL)
                                                     // ..... DD 34 00-110-100 : INC (IX+d)
                                                     // ..... FD 34 00-110-100 : INC (IY+d)
              {
                XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                   //-!-m
                n = Z->peek8(Z->rp[Z->idxHL]+d.d) +1;                            //-!-m
                Z->poke8(Z->rp[Z->idxHL]+d.d, n);                                //-!-m
                MEMUPD(Z->rp[Z->idxHL]+d.d, BYTE, n);
                TIMING(3,1,11);
              }
              else // ....................................... -- ?? 00-yyy-100 : INC r[y]
              {
                n = ++Z->r[rXY(y)];                                              //-!-
                REGUPD(mROP(rXY(y)));
                TIMING(1,1,4);
                sprintf(cmnt, "%s++", rs[rXY(y)]);
              }
              Z->fZ = ZERO(n);
              Z->fP = ((n-1)^n)>>7;  Z->PF = pfOVERFLOW;
              Z->fH = ZERO(n&0x0F); // If low-nibble now zero, half-overflow!
              Z->fN = 0;
              Z->fS = SIGN(n);
              Z->f3 = BIT3(n);
              Z->f5 = BIT5(n);
              FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mF);
              if ((Z->idxHL!=HL)&&(y==AT_HL))   // 0xDD or 0xFD prefix in play
                sprintf(is, MNEMs"(%s%c$%02X)",
                            "INC", rps[Z->idxHL], d.s?'-':'+', d.u);
              else
                sprintf(is, MNEMs"%s", "INC", rs[rXY(y)]);
              break;

            case 5: // 8-bit DEC ............................ -- ?? 00-yyy-101
              if (y==AT_HL) // .............................. -- 35 00-110-101 : DEC (HL)
                                                     // ..... DD 35 00-110-101 : DEC (IX+d)
                                                     // ..... FD 35 00-110-101 : DEC (IY+d)
              {
                XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                   //-!-m
                n = Z->peek8(Z->rp[Z->idxHL]+d.d) -1;                            //-!-m
                Z->poke8(Z->rp[Z->idxHL]+d.d, n);                                //-!-m
                MEMUPD(Z->rp[Z->idxHL]+d.d, BYTE, n);
                TIMING(3,1,11);
              }
              else // ....................................... -- ?? 00-yyy-101 : DEC r[y]
              {
                n = --Z->r[rXY(y)];                                              //-!-
                REGUPD(mROP(rXY(y)));
                TIMING(1,1,4);
                sprintf(cmnt, "%s--", rs[rXY(y)]);
              }
              Z->fZ = ZERO(n);
              Z->fP = ((n+1)^n)>>7;  Z->PF = pfOVERFLOW;
              Z->fH = ((n&0x0F)==0x0F)?1:0; // If low-nibble now 0x0F, half-overflow!
              Z->fN = 1;
              Z->fS = SIGN(n);
              Z->f3 = BIT3(n);
              Z->f5 = BIT5(n);
              FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mF);
              if ((Z->idxHL!=HL)&&(y==AT_HL))   // 0xDD or 0xFD prefix in play
                sprintf(is, MNEMs"(%s%c$%02X)",
                            "DEC", rps[Z->idxHL], d.s?'-':'+', d.u);
              else
                sprintf(is, MNEMs"%s", "DEC", rs[rXY(y)]);
              break;

            case 6: // 8-bit load immediate ................. -- ?? 00-yyy-110
              if (y==AT_HL) // .............................. -- 36 00-110-110 : LD (HL),n
              {
                XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                   //-!-m
                n = NEXTBYTE;                                                    //-!-m
                Z->poke8(Z->rp[Z->idxHL]+d.d, n);                                //-!-m
                MEMUPD(Z->rp[Z->idxHL]+d.d, BYTE, n);
                TIMING(3,1,10);
              }
              else // ....................................... -- ?? 00-yyy-110 : LD r[y],n
              {
                n = NEXTBYTE;                                                    //-!-m
                Z->r[rXY(y)] = n;                                                //-!-
                REGUPD(mROP(rXY(y)));
                TIMING(2,1,7);
                sprintf(cmnt, "%s = $%02X", rs[rXY(y)], n);
              }
              // Flags are unaffected (by all LD operations *1)
              if ((Z->idxHL!=HL)&&(y==AT_HL))   // 0xDD or 0xFD prefix in play
                sprintf(is, MNEMs"(%s%c$%02X),$%02X",
                            "LD", rps[Z->idxHL], d.s?'-':'+', d.u, n);
              else
                sprintf(is, MNEMs"%s,$%02X", "LD", rs[rXY(y)], n);
              break;

            case 7: // Assorted ops on acc/flags ............ -- ?? 00-yyy-111
              switch (y)
              {
                case 0: // .................................. -- 07 00-000-111 : RLCA
                  Z->r[A] = (Z->r[A]<<1) | (Z->r[A]>>7);                         //-!-
                  Z->fC = Z->r[A]&0x01;     // BOTTOM bit AFTER shift
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "RLCA");
                  TIMING(1,1,4);
                  break;
                case 1: // .................................. -- 0F 00-001-111 : RRCA
                  Z->fC = Z->r[A]&0x01;     // BOTTOM bit BEFORE shift
                  Z->r[A] = (Z->r[A]>>1) | (Z->r[A]<<7);                         //-!-
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "RRCA");
                  TIMING(1,1,4);
                  break;
                case 2: // .................................. -- 17 00-010-111 : RLA
                  b = Z->r[A]>>7;           // TOP bit BEFORE shift
                  Z->r[A] = (Z->r[A]<<1) | Z->fC;                                //-!-
                  Z->fC = b;
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "RLA");
                  TIMING(1,1,4);
                  break;
                case 3: // .................................. -- 1F 00-011-111 : RRA
                  b = Z->r[A]&0x01;         // BOTTOM bit BEFORE shift
                  Z->r[A] = (Z->r[A]>>1) | (Z->fC<<7);                           //-!-
                  Z->fC = b;
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "RRA");
                  TIMING(1,1,4);
                  break;

                case 4: // .................................. -- 27 00-100-111 : DAA
                  // http://www.worldofspectrum.org/faq/reference/z80reference.htm#DAA
                  n = 0;                                                         //-!-
                  if ( (Z->r[A]      >0x99) || Z->fC)  n |= 0x60 ;               //-!-
                  if (((Z->r[A]&0x0F)>0x09) || Z->fH)  n |= 0x06 ;               //-!-
                  Z->fH = (Z->r[A]>>4)&1;           // part 1
                  if (Z->fN)  Z->r[A] -= n;  else  Z->r[A] += n ;                //-!-
                  Z->fH = (Z->fH^(Z->r[A]>>4))&1;   // part 2
                  Z->fC = n>>6;
                  Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  Z->fZ = ZERO(Z->r[A]);
                  Z->fS = SIGN(Z->r[A]);
                  FLGUPD(mfC | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "DAA");
                  TIMING(1,1,4);
                  break;
                case 5: // .................................. -- 2F 00-101-111 : CPL
                  Z->f3 = BIT3(Z->r[A]); // from A BEFORE complement
                  Z->f5 = BIT5(Z->r[A]); // from A BEFORE complement
                  Z->r[A] = ~Z->r[A];                                            //-!-
                  Z->fN = 1;
                  Z->fH = 1;
                  FLGUPD(mfN | mf3 | mfH | mf5);
                  REGUPD(mA | mF);
                  sprintf(is, MNEMs, "CPL");
                  TIMING(1,1,4);
                  break;
                case 6: // .................................. -- 37 00-110-111 : SCF
                  // SET carry-flag
                  Z->fC = 1;                                                     //-!-
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]); // from r[A]
                  Z->f5 = BIT5(Z->r[A]); // from r[A]
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mF);
                  sprintf(is, MNEMs, "SCF");
                  TIMING(1,1,4);
                  break;
                case 7: // .................................. -- 3F 00-111-111 : CCF
                  // COMPLEMENT (not clear) carry-flag
                  Z->fH = Z->fC;         // copy of fC BEFORE clearing
                  Z->fC = Z->fC^1;                                               //-!-
                  Z->fN = 0;
                  Z->f3 = BIT3(Z->r[A]); // from r[A]
                  Z->f5 = BIT5(Z->r[A]); // from r[A]
                  FLGUPD(mfC | mfN | mf3 | mfH | mf5);
                  REGUPD(mF);
                  sprintf(is, MNEMs, "CCF");
                  TIMING(1,1,4);
                  break;
              }
              break;
          }
          break;

        case 1: // 8-bit loading & HALT ..................... -- ?? 01-yyy-zzz
          if ((y==AT_HL) && (z==AT_HL)) // .................. -- 76 01-110-110 : HALT
          // logically this might decode as "LD (HL),(HL)"
          {
            Z->HALT = true;                                                      //-!-
            // Flags are unaffected (by interrupt-control operations)
            // Registers and Memory are unaffected
            sprintf(is, MNEMs, "HALT");
            TIMING(1,1,4);
            strcpy(cmnt, "!!");
          }
          else // ........................................... -- ?? 01-yyy-zzz : LD r[y],r[z]
          {
            char  from[9];
            char  to[9];
            // presume no memory access
            TIMING(1,1,4);
            // from r[z]
            if (z==AT_HL) // ................................ -- ?? 01-yyy-110 : LD ____,(HL)
            {
              XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                     //-!-m
              n = Z->peek8(Z->rp[Z->idxHL]+d.d);                                 //-!-m
              TIMING(1,0,3);
              if (Z->idxHL!=HL)
                sprintf(from, "(%s%c$%02X)", rps[Z->idxHL], d.s?'-':'+', d.u);
              else
                sprintf(from, "%s", rs[z]);
            }
            else // ......................................... -- ?? 01-yyy-zzz : LD ____,r[z]
            {
              if (y==AT_HL)
              {
                n = Z->r[z];                                                     //-!-
                sprintf(from, "%s", rs[z]);
              }
              else
              {
                n = Z->r[rXY(z)];                                                //-!-
                sprintf(from, "%s", rs[rXY(z)]);
              }
            }

            // To r[y]
            if (y==AT_HL) // ................................ -- ?? 01-110-zzz : LD (HL),____
            {
              XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                     //-!-m
              Z->poke8(Z->rp[Z->idxHL]+d.d,n);                                   //-!-m
              MEMUPD(Z->rp[Z->idxHL]+d.d, BYTE, n);
              TIMING(1,0,3);
              if (Z->idxHL!=HL)
                sprintf(to, "(%s%c$%02X)", rps[Z->idxHL], d.s?'-':'+', d.u);
              else
                sprintf(to, "%s", rs[y]);
            }
            else // ......................................... -- ?? 01-yyy-zzz : LD r[y],____
            {
              if (z==AT_HL)
              {
                Z->r[y] = n;                                                     //-!-
                sprintf(to, "%s", rs[y]);
                REGUPD(mROP(y));
              }
              else
              {
                Z->r[rXY(y)] = n;                                                //-!-
                sprintf(to, "%s", rs[rXY(y)]);
                REGUPD(mROP(rXY(y)));
              }
            }
            // Flags are unaffected (by all LD operations *1)
            sprintf(is, MNEMs"%s,%s", "LD", to, from);
            if (y==z)
              strcpy(cmnt, ".");
          }
          break;

        case 2: // Operate on acc & reg/mem-location ........ -- ?? 10-yyy-zzz : alu[y],r[z]
        {
          char  from[9];
          // Search the code for "=-ALUN-=" for more info
          if (fAluN)   // ................................... -- ?? 11-yyy-110 : alu[y],n
          {
            ib |= bAluN;   // Correct the opcode (to what it should be)
            n = NEXTBYTE;                                                        //-!-m
            TIMING(2,1,7);
          }
          else if (z==AT_HL)
          {
            XYGETD;   // If IX/IY then d=NEXTBYTE else d=0                       //-!-m
            n = Z->peek8(Z->rp[Z->idxHL]+d.d);                                   //-!-m
            TIMING(2,1,7);
          }
          else
          {
            n = Z->r[rXY(z)];                                                    //-!-
            TIMING(1,1,4);
          }
          if (fAluN)
            sprintf(from, "$%02X", n);
          else if ((Z->idxHL!=HL)&&(z==AT_HL))   // 0xDD or 0xFD prefix in play
            sprintf(from, "(%s%c$%02X)", rps[Z->idxHL], d.s?'-':'+', d.u);
          else
            sprintf(from, "%s", rs[rXY(z)]);

          switch (y)
          {
            case 0: // ...................................... -- ?? 10-000-zzz : ADD A,r[z]
              Z->fC = (n>(0xFF-Z->r[A]))?1:0;
              Z->fH = ((n&0x0F)>(0x0F-(Z->r[A]&0x0F)))?1:0;
              Z->fP = (Z->r[A]>>7);                             // part 1
              Z->r[A] += n;                                                      //-!-
              Z->fP = Z->fP^(Z->r[A]>>7);  Z->PF = pfOVERFLOW;  // part 2
              Z->fS = SIGN(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fN = 0;
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s,%s", "ADD", "A", from);
              break;
            case 1: // ...................................... -- ?? 10-001-zzz : ADC A,r[z]
              Z->fP = (Z->r[A]>>7);                             // part 1
              if (Z->fC)
              {
                Z->fC = (n >= (0xFF-Z->r[A]))?1:0;
                Z->fH = ((n&0x0F) >= (0x0F-(Z->r[A]&0x0F)))?1:0;
                Z->r[A] = Z->r[A] +n +1;                                         //-!-
              }
              else
              {
                Z->fC = (n > (0xFF-Z->r[A]))?1:0;
                Z->fH = ((n&0x0F) > (0x0F-(Z->r[A]&0x0F)))?1:0;
                Z->r[A] += n;                                                    //-!-
              }
              Z->fP = Z->fP^(Z->r[A]>>7);  Z->PF = pfOVERFLOW;  // part 2
              Z->fS = SIGN(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fN = 0;
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s,%s", "ADC", "A", from);
              break;
            case 2: // ...................................... -- ?? 10-010-zzz : SUB r[z]
              Z->fC = (n>Z->r[A])?1:0;
              Z->fH = ((n&0x0F)>(Z->r[A]&0x0F))?1:0;
              Z->fP = (Z->r[A]>>7);                             // part 1
              Z->r[A] -= n;                                                      //-!-
              Z->fP = Z->fP^(Z->r[A]>>7);  Z->PF = pfOVERFLOW;  // part 2
              Z->fS = SIGN(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fN = 1;
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s,%s", "SUB", "A", from);
              break;
            case 3: // ...................................... -- ?? 10-011-zzz : SBC A,r[z]
              Z->fP = (Z->r[A]>>7);                             // part 1
              if (Z->fC)
              {
                Z->fC = (n >= Z->r[A])?1:0;
                Z->fH = ((n&0x0F) >= (Z->r[A]&0x0F))?1:0;
                Z->r[A] = Z->r[A] -n -1;                                         //-!-
              }
              else
              {
                Z->fC = (n > Z->r[A])?1:0;
                Z->fH = ((n&0x0F) > (Z->r[A]&0x0F))?1:0;
                Z->r[A] -= n;                                                    //-!-
              }
              Z->fP = Z->fP^(Z->r[A]>>7);  Z->PF = pfOVERFLOW;  // part 2
              Z->fS = SIGN(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fN = 1;
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s,%s", "SBC", "A", from);
              break;
            case 4: // ...................................... -- ?? 10-100-zzz : AND r[z]
              Z->r[A] &= n;                                                      //-!-
              Z->fC = 0;
              Z->fN = 0;
              Z->fH = 1;
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
              Z->fS = SIGN(Z->r[A]);
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s", "AND", from);
              break;
            case 5: // ...................................... -- ?? 10-101-zzz : XOR r[z]
              Z->r[A] ^= n;                                                      //-!-
              Z->fC = 0;
              Z->fN = 0;
              Z->fH = 0;
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
              Z->fS = SIGN(Z->r[A]);
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s", "XOR", from);
              break;
            case 6: // ...................................... -- ?? 10-110-zzz : OR r[z]
              Z->r[A] |= n;                                                      //-!-
              Z->fC = 0;
              Z->fN = 0;
              Z->fH = 0;
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
              Z->fS = SIGN(Z->r[A]);
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              sprintf(is, MNEMs"%s", "OR", from);
              break;
            case 7: // ...................................... -- ?? 10-111-zzz : CP r[z]
              Z->fC = (n>Z->r[A])?1:0;
              Z->fH = ((n&0x0F)>(Z->r[A]&0x0F))?1:0;
              n = Z->r[A] -n;         // Perform SUB (discard result)            //-!-
              Z->fP = (Z->r[A]^n)>>7;  Z->PF = pfOVERFLOW;
              Z->fS = SIGN(n);
              Z->fZ = ZERO(n);
              Z->f3 = BIT3(Z->r[z]);  // from operand, NOT result
              Z->f5 = BIT5(Z->r[z]);  // from operand, NOT result
              Z->fN = 1;
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mF);  // CP does not effect A
              sprintf(is, MNEMs"%s", "CP", from);
              break;
          }
          break;
        }

        case 3: // Various .................................. -- ?? 11-yyy-zzz
          switch (z)
          {
            case 0: // ...................................... -- ?? 11-yyy-000 : RET cc[y]
                                                    // ...... -- ?? 11-ppq-000
                              //  (Z->fZ) ? (q) : (!q) ...... -- C0 11-000-000 : RET NZ
                              //  (Z->fZ) ? (q) : (!q) ...... -- C8 11-001-000 : RET  Z
                              //  (Z->fC) ? (q) : (!q) ...... -- D0 11-010-000 : RET NC
                              //  (Z->fC) ? (q) : (!q) ...... -- D8 11-011-000 : RET  C
                              //  (Z->fP) ? (q) : (!q) ...... -- E0 11-100-000 : RET PO
                              //  (Z->fP) ? (q) : (!q) ...... -- E8 11-101-000 : RET PE
                              //  (Z->fS) ? (q) : (!q) ...... -- F0 11-110-000 : RET  P
                              //  (Z->fS) ? (q) : (!q) ...... -- F8 11-111-000 : RET  M
              if (CC_MET(p,q))
              {
                // POP PC
                Z->rp[PC] = PEEK16(Z->rp[SP]);                                   //-!-mm
                Z->rp[SP] += 2;                                                  //-!-
                R16UPD(mPC | mSP);
                TIMING(3,1,11);
              }
              else
              {
                TIMING(1,1,5);
              }
              // Flags are unaffected (by all flow-control operations)
              sprintf(is, MNEMs"%s", "RET", ccs[y]);
              break;

            case 1: // POP & Various ops .................... -- ?? 11-ppq-001
              switch (q)
              {
                case 0: // .................................. -- ?? 11-pp0-001 : POP rp2[p]
                  Z->rp[RP2(rpXY(p))] = PEEK16(Z->rp[SP]);                       //-!-mm
                  Z->rp[SP] += 2;                                                //-!-
                  TIMING(3,1,10);
                  R16UPD(mROP(RP2(rpXY(p))) | mSP);
                  // Flags are unaffected (by all stack operations)
                  // Flags are (IN)DIRECTLY affected by POP AF
                  if (p==3)   // p==3 NOT p==AF !
                  {
                    Z->fC = (Z->rp[AF]>>8)&mfC?1:0;                              //-!-
                    Z->fN = (Z->rp[AF]>>8)&mfN?1:0;                              //-!-
                    Z->fP = (Z->rp[AF]>>8)&mfP?1:0;  Z->PF = pfPOP;              //-!-
                    Z->f3 = (Z->rp[AF]>>8)&mf3?1:0;                              //-!-
                    Z->fH = (Z->rp[AF]>>8)&mfH?1:0;                              //-!-
                    Z->f5 = (Z->rp[AF]>>8)&mf5?1:0;                              //-!-
                    Z->fS = (Z->rp[AF]>>8)&mfS?1:0;                              //-!-
                    Z->fZ = (Z->rp[AF]>>8)&mfZ?1:0;                              //-!-
                    FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  }
                  sprintf(is, MNEMs"%s", "POP", rps[RP2(rpXY(p))]);
                  break;
                case 1: // Various ops ...................... -- ?? 11-pp1-001
                  switch (p)
                  {
                    case 0: // .............................. -- C9 11-001-001 : RET
                      Z->rp[PC] = PEEK16(Z->rp[SP]);                             //-!-mm
                      Z->rp[SP] += 2;                                            //-!-
                      TIMING(3,1,10);
                      R16UPD(mPC | mSP);
                      // Flags are unaffected (by all flow-control operations)
                      sprintf(is, MNEMs, "RET");
                      Z->ibc = 0;
                      break;
                    case 1: // .............................. -- D9 11-011-001 : EXX
                      SWAP(Z->r[B], Z->r_[B]);                                   //-!-
                      SWAP(Z->r[C], Z->r_[C]);                                   //-!-
                      SWAP(Z->r[D], Z->r_[D]);                                   //-!-
                      SWAP(Z->r[E], Z->r_[E]);                                   //-!-
                      SWAP(Z->r[H], Z->r_[H]);                                   //-!-
                      SWAP(Z->r[L], Z->r_[L]);                                   //-!-
                      REGUPD(mB | mC | mD | mE | mH | mL);
                      REGUPD_(mB | mC | mD | mE | mH | mL);
                      TIMING(1,1,4);
                      // Flags are unaffected
                      sprintf(is, MNEMs, "EXX");
                      strcpy(cmnt, "<-> B,C,D,E,H,L");
                      break;
                    case 2: // .............................. -- E9 11-101-001 : JP HL
                                         // commonly, and MISLEADINGLY, called : JP (HL)
                      Z->rp[PC] = Z->rp[Z->idxHL];                               //-!-
                      R16UPD(mPC);
                      TIMING(1,QQ,4);
                      // Flags are unaffected (by all flow-control operations)
                      sprintf(is, MNEMs"%s", "JP", rps[Z->idxHL]);
			Z->ibc = 0;
                      break;
                    case 3: // .............................. -- F9 11-111-001 : LD SP,HL
                      Z->r[SP] = Z->rp[Z->idxHL];                                //-!-
                      R16UPD(mSP);
                      TIMING(1,QQ,6);
                      // Flags are unaffected (by all LD operations *1)
                      sprintf(is, MNEMs"%s,%s", "LD", "SP", rps[Z->idxHL]);
                      break;
                  }
                  break;
              }
              break;

            case 2: // Conditional Jump ..................... -- ?? 11-yyy-010 : JP cc[y],nn
                                                    // ...... -- ?? 11-ppq-010
                              //  (Z->fZ) ? (q) : (!q) ...... -- C2 11-000-010 : JP NZ,nn
                              //  (Z->fZ) ? (q) : (!q) ...... -- CA 11-001-010 : JP  Z,nn
                              //  (Z->fC) ? (q) : (!q) ...... -- D2 11-010-010 : JP NC,nn
                              //  (Z->fC) ? (q) : (!q) ...... -- DA 11-011-010 : JP  C,nn
                              //  (Z->fP) ? (q) : (!q) ...... -- E2 11-100-010 : JP PO,nn
                              //  (Z->fP) ? (q) : (!q) ...... -- EA 11-101-010 : JP PE,nn
                              //  (Z->fS) ? (q) : (!q) ...... -- F2 11-110-010 : JP  P,nn
                              //  (Z->fS) ? (q) : (!q) ...... -- FA 11-111-010 : JP  M,nn
              nn = NEXTWORD;                                                     //-!-mm
              if (CC_MET(p,q))
              {
                Z->rp[PC] = nn;                                                  //-!-
                R16UPD(mPC);
		Z->ibc = 0;
              }
              TIMING(3,1,10); // Whether condition is met or not!
              // Flags are unaffected (by all flow-control operations)
              sprintf(is, MNEMs"%s,%s", "JP", ccs[y], label(nn));
              break;

            case 3: // Assorted operations   ................ -- ?? 11-yyy-011
              switch (y)
              {
                case 0: // .................................. -- C3 11-000-011 : JP nn
                                         // commonly, and MISLEADINGLY, called : JP (nn)
                  nn = NEXTWORD;                                                 //-!-mm
                  Z->rp[PC] = nn;                                                //-!-
                  R16UPD(mPC);
                  TIMING(3,1,10);
                  // Flags are unaffected (by all flow-control operations)
                  sprintf(is, MNEMs"%s", "JP", label(nn));
			Z->ibc = 0;
                  break;
                case 1: // .................................. -- CB 11-001-011 : 0xCB prefix
                  // Prefixes are handled before the main switch()
                  // ...execution should never get here
                  break;
                case 2: // .................................. -- D3 11-010-011 : OUT (n),A
                  n = NEXTBYTE;                                                  //-!-m
                  Z->out8((Z->r[A]<<8)|n, Z->r[A]);  // A15-A8 = r[A]            //-!-m
                  TIMING(3,1,11);
                  // Registers and Memory are unaffected
                  // Flags are unaffected (by all non-block OUT operations)
                  sprintf(is, MNEMs"($%02X),%s", "OUT", (n), "A");
                  break;
                case 3: // .................................. -- DB 11-011-011 : IN A,(n)
                  n = NEXTBYTE;                                                  //-!-m
                  Z->r[A] = Z->in8((Z->r[A]<<8)|n);  // A15-A8 = r[A]            //-!-m
                  TIMING(3,1,11);
                  REGUPD(mA);
                  // Flags are unaffected (by "IN A,(n)" only)
                  sprintf(is, MNEMs"%s,($%02X)", "IN", "A", (n));
                  break;
                case 4: // .................................. -- E3 11-100-011 : EX (SP),HL
                  nn = PEEK16(Z->rp[SP]);                                        //-!-mm
                  SWAP(nn,Z->r[Z->idxHL]);                                       //-!-
                  POKE16(Z->rp[SP], nn);                                         //-!-mm
                  MEMUPD(Z->rp[SP], WORD, nn);
                  R16UPD(mROP(Z->idxHL));
                  TIMING(5,QQ,19);
                  // Flags are unaffected
                  sprintf(is, MNEMs"%s,%s", "EX", "(SP)", rps[Z->idxHL]);
                  break;
                case 5: // .................................. -- EB 11-101-011 : EX DE,HL
                  // Not affected by IDX
                  SWAP(Z->rp[DE],Z->rp[HL]);                                     //-!-
                  R16UPD(mDE | mHL);
                  // Flags are unaffected
                  TIMING(1,QQ,4);
                  sprintf(is, MNEMs"%s", "EX", "DE,HL");
                  break;
                case 6: // .................................. -- F3 11-110-011 : DI
                  Z->IFF1 = 0;   // Disable Interrupts                           //-!-
                  Z->IFF2 = 0;                                                   //-!-
                  // Registers and Memory are unaffected
                  // Flags are unaffected (by interrupt-control operations)
                  TIMING(1,QQ,4);
                  sprintf(is, MNEMs, "DI");
                  break;
                case 7: // .................................. -- FB 11-111-011 : EI
                  Z->IFF1 = 1;     // Enable Interrupts                          //-!-
                  Z->IFF2 = 1;                                                   //-!-
                  Z->IHLD = true;  // Disable interrupts for 1 instr             //-!-
                  // Registers and Memory are unaffected
                  // Flags are unaffected (by interrupt-control operations)
                  TIMING(1,QQ,4);
                  sprintf(is, MNEMs, "EI");
                  break;
              }
              break;

          case 4: // Conditional Call ....................... -- ?? 11-yyy-100 : CALL cc[y],nn
                                                    // ...... -- ?? 11-ppq-100
                              //  (Z->fZ) ? (q) : (!q) ...... -- C4 11-000-100 : CALL NZ,nn
                              //  (Z->fZ) ? (q) : (!q) ...... -- CC 11-001-100 : CALL  Z,nn
                              //  (Z->fC) ? (q) : (!q) ...... -- D4 11-010-100 : CALL NC,nn
                              //  (Z->fC) ? (q) : (!q) ...... -- DC 11-011-100 : CALL  C,nn
                              //  (Z->fP) ? (q) : (!q) ...... -- E4 11-100-100 : CALL PO,nn
                              //  (Z->fP) ? (q) : (!q) ...... -- EC 11-101-100 : CALL PE,nn
                              //  (Z->fS) ? (q) : (!q) ...... -- F4 11-110-100 : CALL  P,nn
                              //  (Z->fS) ? (q) : (!q) ...... -- FC 11-111-100 : CALL  M,nn
              nn = NEXTWORD;                                                     //-!-mm
              if (CC_MET(p,q))
              {
                Z->rp[SP] -= 2;                                                  //-!-
                POKE16(Z->rp[SP], Z->rp[PC]+Z->ibc);                             //-!-mm
                Z->rp[PC] = nn;                                                  //-!-
                R16UPD(mPC | mSP);
                MEMUPD(Z->rp[SP], WORD, Z->rp[PC]+Z->ibc);
                TIMING(5,1,17);
		Z->ibc = 0;
              }
              else
              {
                // Jump does not occur
                TIMING(3,1,10);
              }
              // Flags are unaffected (by all flow-control operations)
              sprintf(is, MNEMs"%s,%s", "CALL", ccs[y], label(nn));
              break;

            case 5: // PUSH, CALL & prefixes ................ -- ?? 11-ppq-101
              switch (q)
              {
                case 0: // .................................. -- ?? 11-pp0-101 : PUSH rp2[p]
                  Z->rp[SP] -= 2;                                                //-!-
                  POKE16(Z->rp[SP], Z->rp[RP2(rpXY(p))]);                        //-!-mm
                  TIMING(3,1,11);
                  R16UPD(mROP(RP2(rpXY(p))) | mSP);
                  MEMUPD(Z->rp[SP], WORD, Z->rp[RP2(rpXY(p))]);
                  // Flags are unaffected (by all stack operations)
                  sprintf(is, MNEMs"%s", "PUSH", rps[RP2(rpXY(p))]);
                  break;
                case 1: // CALL & prefixes .................. -- ?? 11-pp1-101
                  switch (p)
                  {
                    case 0: // .............................. -- CD 11-001-101 : CALL NN
                      nn = NEXTWORD;                                             //-!-mm
                      Z->rp[SP] -= 2;                                            //-!-
                      POKE16(Z->rp[SP], Z->rp[PC]+Z->ibc);                       //-!-mm
                      MEMUPD(Z->rp[SP], WORD, Z->rp[PC]+Z->ibc);
                      Z->rp[PC] = nn;                                            //-!-
                      R16UPD(mPC | mSP);
                      TIMING(5,1,17);
                      // Flags are unaffected (by all flow-control operations)
                      sprintf(is, MNEMs"%s", "CALL", label(nn));
                      Z->ibc = 0;
                      break;
                    case 1: // .............................. -- DD 11-011-101 : 0xDD prefix
                                                     // .....                  : LD IDX,IX
                      n = Z->peek8(Z->rp[PC]+(Z->ibc));
                      if (ixy[n&0xF][n>>4])   // only for effected instr's
                      {
                        Z->idxHL = IX;                                           //-!-
                        Z->idxH  = IXh;                                          //-!-
                        Z->idxL  = IXl;                                          //-!-
                      }
                      Z->IHLD = true;  // Do not check intrs after this instr    //-!-
                      // Registers and Memory are unaffected
                      // Flags are unaffected
                      TIMING(1,1,4);
                      ///sprintf(is, MNEMs"%s,%s", "ld", "idx", "ix");
                      sprintf(is, MNEMs"%s", "NONI", "$DD");
                      break;
                    case 2: // .............................. -- ED 11-101-101 : 0xED prefix
                      // The 0xED prefix is handled before the main switch()
                      // ...execution should never get here
                      break;
                    case 3: // .............................. -- FD 11-111-101 : 0xFD prefix
                                                     // .....                  : LD IDX,IY
                      n = Z->peek8(Z->rp[PC]+(Z->ibc));
                      if (ixy[n&0xF][n>>4])   // only for effected instr's
                      {
                        Z->idxHL = IY;                                           //-!-
                        Z->idxH  = IYh;                                          //-!-
                        Z->idxL  = IYl;                                          //-!-
                      }
                      Z->IHLD = true;  // Do not check intrs after this instr    //-!-
                      // Registers and Memory are unaffected
                      // Flags are unaffected
                      TIMING(1,1,4);
                      ///sprintf(is, MNEMs"%s,%s", "ld", "idx", "iy");
                      sprintf(is, MNEMs"%s", "NONI", "$FD");
                      break;
                  }
                  break;
              }
              break;

            case 6: // Operate on acc & imm-operand ......... -- ?? 11-yyy-110 : alu[y],n
              // Search the code for "=-ALUN-=" for more info
              // ...execution should never get here
              break;

            case 7: // ...................................... -- ?? 11-yyy-111 : RST $(y<<3)
              Z->rp[SP] -= 2;                                                    //-!-
              POKE16(Z->rp[SP], Z->rp[PC]);                                      //-!-mm
              MEMUPD(Z->rp[SP], WORD, Z->rp[PC]);
              Z->rp[PC] = y<<3;                                                  //-!-
              R16UPD(mPC | mSP);
              TIMING(5,1,17);
              // Flags are unaffected (by all flow-control operations)
              sprintf(is, MNEMs"$%02X", "RST", Z->rp[PC]);
              break;
          }
          break;

      } // No prefix, switch(x)
      break;

    case 0xCB:  // Bit twiddling, shifts & rotates .......... CB ?? xx-yyy-zzz
    {
      char  ldop[6];  // I.e. "LD ?,"  [Latin: id est -> that is]
      char  op[4];    // E.g. "RLC"    [Latin: exempli gratia -> for example]

      if (z!=AT_HL)  sprintf(ldop, "LD %s,", rs[z]) ;

      if (Z->idxHL!=HL)
      {
        n = Z->peek8(Z->r[Z->idxHL]+d.d);                                        //-!-m
        TIMING(QQ,QQ,QQ);
      }
      else if (z==AT_HL)
      {
        n = Z->peek8(Z->r[HL]);                                                  //-!-m
        TIMING(3,2,12);       // also see after switch(x)
      }
      else
      {
        n = Z->r[z];
        TIMING(QQ,QQ,QQ);
      }

      switch (x)
      {
        case 0: // .......................................... CB ?? 00-yyy-zzz : rot[y] r[z]
          switch (y)
          {
            case 0: // ...................................... CB ?? 00-000-zzz : RLC r[z]
              n = (n<<1) | (n>>7);                                               //-!-
              Z->fC = n&0x01; // BOTTOM bit AFTER shift
              FLGUPD(mfC);
              strcpy(op,"RLC");
              break;
            case 1: // ...................................... CB ?? 00-001-zzz : RRC r[z]
              Z->fC = n&0x01; // BOTTOM bit BEFORE shift
              FLGUPD(mfC);
              n = (n>>1) | (n<<7);                                               //-!-
              strcpy(op,"RRC");
              break;
            case 2: // ...................................... CB ?? 00-010-zzz : RL  r[z]
              b = n>>7;       // TOP bit BEFORE shift
              n = (n<<1) | Z->fC;                                                //-!-
              Z->fC = b;
              FLGUPD(mfC);
              strcpy(op,"RL");
              break;
            case 3: // ...................................... CB ?? 00-011-zzz : RR  r[z]
              b = n&0x01;     // BOTTOM bit BEFORE shift
              n = (n>>1) | (Z->fC<<7);                                           //-!-
              Z->fC = b;
              FLGUPD(mfC);
              strcpy(op,"RR");
              break;
            case 4: // ...................................... CB ?? 00-100-zzz : SLA r[z]
              Z->fC = n>>7;   // TOP bit BEFORE shift
              FLGUPD(mfC);
              n <<= 1;                                                           //-!-
              strcpy(op,"SLA");
              break;
            case 5: // ...................................... CB ?? 00-101-zzz : SRA r[z]
              Z->fC = n&0x01; // BOTTOM bit BEFORE shift
              FLGUPD(mfC);
              n = (n>>1) | (n&0x80);                                             //-!-
              strcpy(op,"SRA");
              break;
            case 6: // ...................................... CB ?? 00-110-zzz : SLL r[z]
              Z->fC = n>>7;   // TOP bit BEFORE shift
              FLGUPD(mfC);
              n = (n<<1) | 0x01;                                                 //-!-
              strcpy(op,"SLL");
              break;
            case 7: // ...................................... CB ?? 00-111-zzz : SRL r[z]
              Z->fC = n&0x01; // BOTTOM bit BEFORE shift
              FLGUPD(mfC);
              n >>= 1;                                                           //-!-
              strcpy(op,"SRL");
              break;
          }
          Z->fP = PAR8(n);  Z->PF = pfPARITY;
          Z->fS = SIGN(n);
          Z->fZ = ZERO(n);
          Z->fH = 0;
          Z->fN = 0;
          Z->f3 = BIT3(n);
          Z->f5 = BIT5(n);
          FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
          if (Z->idxHL!=HL)
            if (z==AT_HL)
              sprintf(is, MNEMs"(%s%c$%02X)", op, rps[Z->idxHL], d.s?'-':'+', d.u);
            else
              sprintf(is, MNEMs"%s (%s%c$%02X)", ldop, op, rps[Z->idxHL], d.s?'-':'+', d.u);
          else
            sprintf(is, MNEMs"%s", op, rs[z]);
          break;
        case 1: // .......................................... CB ?? 01-yyy-zzz : BIT y,r[z]
          Z->fZ = (n&(1<<y))?0:1;                    // key flag                 //-!-
          Z->fP = Z->fZ;  Z->PF = pfZERO;            // copy of new fZ           //-!-
          Z->fS = ((y==7)&&(n&(1<<7)))?1:0;          // test relevant            //-!-
///*          Z->f3 = ((y==3)&&(n&(1<<3)))?1:0;          // test relevant            //-!-
///*          Z->f5 = ((y==5)&&(n&(1<<5)))?1:0;          // test relevant            //-!-
          Z->f3 = BIT3(Z->r[z]);   ///* unverified   // test relevant             //-!-
          Z->f5 = BIT5(Z->r[z]);   ///* unverified   // test relevant             //-!-
          Z->fN = 0;
          Z->fH = 1;
          FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
          REGUPD(mF);
          if (Z->idxHL!=HL)
            sprintf(is, MNEMs"%d,(%s%c$%02X)", "BIT", y, rps[Z->idxHL], d.s?'-':'+', d.u);
          else
            sprintf(is, MNEMs"%d,%s", "BIT", y, rs[z]);
          break;
        case 2: // .......................................... CB ?? 10-yyy-zzz : RES y,r[z]
          n &= ~(1<<y);                                                          //-!-
          // Flags are unaffected (by all SET/RES operations)
          if (Z->idxHL!=HL)
            if (z==AT_HL)
              sprintf(is, MNEMs"%d,(%s%c$%02X)", "RES", y, rps[Z->idxHL], d.s?'-':'+', d.u);
            else
              sprintf(is, MNEMs"%s %d,(%s%c$%02X)", ldop, "RES", y, rps[Z->idxHL], d.s?'-':'+', d.u);
          else
            sprintf(is, MNEMs"%d,%s", "RES", y, rs[z]);
          break;
        case 3: // .......................................... CB ?? 11-yyy-zzz : SET y,r[z]
          n |= 1<<y;                                                             //-!-
          // Flags are unaffected (by all SET/RES operations)
          if (Z->idxHL!=HL)
            if (z==AT_HL)
              sprintf(is, MNEMs"%d,(%s%c$%02X)", "SET", y, rps[Z->idxHL], d.s?'-':'+', d.u);
            else
              sprintf(is, MNEMs"%s %d,(%s%c$%02X)", ldop, "SET", y, rps[Z->idxHL], d.s?'-':'+', d.u);
          else
            sprintf(is, MNEMs"%d,%s", "SET", y, rs[z]);
          break;
      }//switch(x)

      if (x!=1)   // non-BIT instrs now perform memory &| register writes
      {
        if (Z->idxHL!=HL)   // DD/FD indexing  +  !BIT
        {
          Z->poke8(Z->r[Z->idxHL]+d.d, n);                                       //-!-m
          MEMUPD(Z->r[Z->idxHL]+d.d, BYTE, n);
          TIMING(QQ,QQ,QQ); // added to (3,2,12) from before switch(x)
          if (z!=AT_HL)     // DD/FD indexing  +  !BIT  + r[z]
          {
            Z->r[z] = n;                                                         //-!-
            REGUPD(mROP(z));
          }
        }
        else  // Not indexing  +  !BIT
        {
          if (z==AT_HL)
          {
            Z->poke8(Z->r[HL], n);                                               //-!-m
            TIMING(1,0,3);    // added to (3,2,12) from before switch(x)
            MEMUPD(Z->r[HL], BYTE, n);
          }
          else
          {
            Z->r[z] = n;                                                         //-!-
            REGUPD(mROP(z));
          }
        }
      }

      break;
    }

    case 0xED:  // 16-bit stuff, Interrupts & Others......... ED ?? xx-yyy-zzz
      // An 0xDD and 0xFD prefix will have reduced this 0xED to a NONI
      // by forcing ib=0, which in turn forces the switch (x) to case 0

      switch (x)
      {
        // CAREFUL!  The case values are out of sequence here {1,2,0,3}
        case 1: // .......................................... ED ?? 01-yyy-zzz
          switch (z)
          {
            case 0: // ...................................... ED ?? 01-yyy-000 : IN r[y],(C)
                                                     // .....                  : IN r[y],(BC)
              // if (y==6) (Ie. AT_HL) the inputted byte is lost
              //   this (undoc'd) instr is called "IN F,(C)" or "IN (C)"
              //   or more informatively "IN F,(BC)" or "IN (BC)"
              n = Z->in8(Z->r[BC]);                                              //-!-
              if (y!=AT_HL)   // store inputted byte
              {
                Z->r[y] = n;                                                     //-!-
                REGUPD(mROP(y));
              } // else inputted byte is lost
              Z->fN = 0;
              Z->fH = 0;
              Z->f5 = BIT5(n);
              Z->f3 = BIT3(n);
              Z->fP = PAR8(n);  Z->PF = pfPARITY;
              Z->fS = SIGN(n);
              Z->fZ = ZERO(n);
              FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mF);
              TIMING(3,QQ,12);
              sprintf(is, MNEMs"%s,%s", "IN", (y==AT_HL)?"F":rs[y], "(C)");
              break;
            case 1: // ...................................... ED ?? 01-yyy-001 : OUT (C),r[y]
                                                     // .....                  : OUT (BC),r[y]
              // if (y==6) (Ie. AT_HL) the outputted byte is 0
              //   this instr is called "OUT (C)" or "OUT (C),0"
              //   or more informatively "OUT (BC)" or "OUT (BC),0"
              n = (y==AT_HL) ? 0 : Z->r[y];                                      //-!-
              Z->out8(Z->r[BC], n);                                              //-!-
              // Flags are unaffected (by all non-block OUT operations)
              TIMING(3,QQ,12);
              sprintf(is, MNEMs"%s,%s", "OUT", "(C)", (y==AT_HL)?"0":rs[y]);
              break;
            case 2: // 16-bit ADC/SBC ....................... ED ?? 01-ppq-010
              switch (q)
              {
                case 0: // .................................. ED ?? 01-pp0-010 : SBC HL,rp[p]
                  Z->fP = (Z->rp[HL]>>15);                            // #1
                  if (Z->fC)
                  {
                    Z->fC = (Z->rp[p] >= Z->rp[HL])?1:0;
                    Z->fH = ((Z->rp[p]&0x0FFF) >= (Z->rp[HL]&0x0FFF))?1:0;
                    Z->rp[HL] = Z->rp[HL] -Z->rp[p] -1;                          //-!-
                  }
                  else
                  {
                    Z->fC = (Z->rp[p] > Z->rp[HL])?1:0;
                    Z->fH = ((Z->rp[p]&0x0FFF) > (Z->rp[HL]&0x0FFF))?1:0;
                    Z->rp[HL] -= Z->rp[p];                                       //-!-
                  }
                  Z->fP = Z->fP^(Z->rp[HL]>>15);  Z->PF = pfOVERFLOW; // #2
                  Z->fS = SIGN(Z->rp[HL]>>8);
                  Z->fZ = ZERO(Z->rp[HL]);
                  Z->f3 = BIT3(Z->rp[HL]>>8);
                  Z->f5 = BIT5(Z->rp[HL]>>8);
                  Z->fN = 1;
                  FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  R16UPD(mHL);
                  REGUPD(mF);
                  TIMING(4,QQ,15);
                  sprintf(is, MNEMs"%s,%s", "SBC", "HL", rps[p]);
                  break;
                case 1: // .................................. ED ?? 01-pp1-010 : ADC HL,rp[p]
                  Z->fP = (Z->rp[HL]>>15);                            // #1
                  if (Z->fC)
                  {
                    Z->fC = (Z->rp[p] >= (0xFFFF-Z->rp[HL]))?1:0;
                    Z->fH = ((Z->rp[p]&0x0FFF) >=
                            (0x0FFF-(Z->rp[HL]&0x0FFF)))?1:0;
                    Z->rp[HL] = Z->rp[HL] -Z->rp[p] -1;                          //-!-
                  }
                  else
                  {
                    Z->fC = (Z->rp[p] > (0xFFFF-Z->rp[HL]))?1:0;
                    Z->fH = ((Z->rp[p]&0x0FFF) >
                            (0x0FFF-(Z->rp[HL]&0x0FFF)))?1:0;
                    Z->rp[HL] -= Z->rp[p];                                       //-!-
                  }
                  Z->fP = Z->fP^(Z->rp[HL]>>15);  Z->PF = pfOVERFLOW; // #2
                  Z->fS = SIGN(Z->rp[HL]>>8);
                  Z->fZ = ZERO(Z->rp[HL]);
                  Z->f3 = BIT3(Z->rp[HL]>>8);
                  Z->f5 = BIT5(Z->rp[HL]>>8);
                  Z->fN = 0;
                  FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  R16UPD(mHL);
                  REGUPD(mF);
                  TIMING(4,QQ,15);
                  sprintf(is, MNEMs"%s,%s", "ADC", "HL", rps[p]);
                  break;
              }
              break;
            case 3: // LD rp to/from imm addr ............... ED ?? 01-ppq-011
              switch (q)
              {
                case 0: // .................................. ED ?? 01-pp0-011 : LD (nn),rp[p]
                  nn = NEXTWORD;                                                 //-!-mm
                  POKE16(nn, Z->rp[p]);                                          //-!-mm
                  MEMUPD(nn, WORD, Z->rp[p]);
                  // Flags are unaffected (by all LD operations *1)
                  TIMING(6,QQ,20);
                  sprintf(is, MNEMs"($%04X),%s", "LD", (nn), rps[p]);
                  break;
                case 1: // .................................. ED ?? 01-pp1-011 : LD rp[p],(nn)
                  nn = NEXTWORD;                                                 //-!-mm
                  Z->rp[p] = PEEK16(nn);                                         //-!-mm
                  // Flags are unaffected (by all LD operations *1)
                  R16UPD(mROP(p));
                  TIMING(6,QQ,20);
                  sprintf(is, MNEMs"%s,($%04X)", "LD", rps[p], (nn));
                  break;
              }
              break;
            case 4: // ...................................... ED ?? 01-yyy-100 : NEG
              // For ANY value of y
              Z->fC = Z->r[A]?1:0;  // (Zaks is wrong!)   ///* questionable
              Z->fH = (Z->r[A]&0x0F)?1:0;                 ///* questionable
              Z->r[A] = (~Z->r[A]) +1;  // 2's complement
              Z->fN = 1;
              Z->f3 = BIT3(Z->r[A]);
              Z->f5 = BIT5(Z->r[A]);
              Z->fS = SIGN(Z->r[A]);
              Z->fZ = ZERO(Z->r[A]);
              Z->fP = Z->fS;  Z->PF = pfSIGN;             ///* questionable
              FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
              REGUPD(mA | mF);
              TIMING(2,QQ,8);
              sprintf(is, MNEMs, "NEG");
              break;
            case 5: // RETN/RETI ............................ ED ?? 01-yyy-101
                                                     // ..... ED 4D 01-001-101 : RETI
                                                     // ..... ED ?? 01-yyy-101 : RETN
              // RETI is only unique in as much as the opcode [ED 4D]
              //   is spotted on the databus by the Z80-PIO chip
              Z->IFF1 = Z->IFF2;                                                 //-!-
              // Flags are unaffected (by all flow-control operations)
              TIMING(4,QQ,14);
              sprintf(is, MNEMs, (ib==0x4D)?"RETI":"RETN");
              break;
            case 6: // Set Interrupt Mode ................... ED ?? 01-yyy-110
                                                     // ..... ED 46 01-000-110 : IM 0
                                                     // ..... ED 4E 01-001-110 : IM 0/1
                                                     // ..... ED 56 01-010-110 : IM 1
                                                     // ..... ED 5E 01-011-110 : IM 2
                                                     // ..... ED 66 01-100-110 : IM 0
                                                     // ..... ED 6E 01-101-110 : IM 0/1
                                                     // ..... ED 76 01-110-110 : IM 1
                                                     // ..... ED 7E 01-111-110 : IM 2
              Z->IM = (y&3)?((y&3)-1):0;                                         //-!-
              // Flags are unaffected (by all LD operations *1)
              // Registers and Memory are unaffected
              TIMING(2,QQ,8);
              sprintf(is, MNEMs"%d", "IM", Z->IM);
              if ((ib==0x4E)||(ib==0x6E))  strcat(is, "/1");  ///* unverified
              break;
            case 7: // Various .............................. ED ?? 01-yyy-111
              switch (y)
              {
                case 0: // .................................. ED 47 01-000-111 : LD I,A
                  Z->r[I] = Z->r[A];                                             //-!-
                  // Flags are unaffected (by all LD operations *1)
                  REGUPD(mI);
                  TIMING(2,QQ,9);
                  sprintf(is, MNEMs"%s", "LD", "I,A");
                  break;
                case 1: // .................................. ED 4F 01-001-111 : LD R,A
                  Z->r[R] = Z->r[A];                                             //-!-
                  // Flags are unaffected (by all LD operations *1)
                  REGUPD(mR);
                  TIMING(2,QQ,9);
                  sprintf(is, MNEMs"%s", "LD", "R,A");
                  break;
                case 2: // .................................. ED 57 01-010-111 : LD A,I
                  Z->r[A] = Z->r[I];                                             //-!-
                  // Flags ARE affected by "LD A,I"
                  Z->fP = Z->IFF2;  Z->PF = pfINT;                               //-!-
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  Z->fS = SIGN(Z->r[A]);
                  Z->fZ = ZERO(Z->r[A]);
                  FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  TIMING(2,QQ,9);
                  sprintf(is, MNEMs"%s", "LD", "A,I");
                  break;
                case 3: // .................................. ED 5F 01-011-111 : LD A,R
                  Z->r[A] = Z->r[R];                                             //-!-
                  // Flags ARE affected by "LD A,R"
                  Z->fP = Z->IFF2;  Z->PF = pfINT;                               //-!-
                  Z->fN = 0;
                  Z->fH = 0;
                  Z->f3 = BIT3(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  Z->fS = SIGN(Z->r[A]);
                  Z->fZ = ZERO(Z->r[A]);
                  FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  TIMING(2,QQ,9);
                  sprintf(is, MNEMs"%s", "LD", "A,R");
                  break;
                case 4: // .................................. ED 67 01-100-111 : RRD
                  // Somehow RRD acquires an extra m-state !?                    //-!-m
                  n = Z->peek8(Z->rp[HL]);                                       //-!-m
                  Z->poke8(Z->rp[HL], (n>>4) | (Z->r[A]<<4));                    //-!-m
                  Z->r[A] = (Z->r[A]&0xF0)|(n&0x0F);                             //-!-
                  Z->fS = SIGN(Z->r[A]);
                  Z->fZ = ZERO(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  Z->f3 = BIT3(Z->r[A]);
                  Z->fH = 0;
                  Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
                  Z->fN = 0;
                  FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  TIMING(5,QQ,18);
                  sprintf(is, MNEMs, "RRD");
                  break;
                case 5: // .................................. ED 6F 01-101-111 : RLD
                  // Somehow RLD acquires an extra m-state !?                    //-!-m
                  n = Z->peek8(Z->rp[HL]);                                       //-!-m
                  Z->poke8(Z->rp[HL], (n<<4) | (Z->r[A]&0x0F));                  //-!-m
                  Z->r[A] = (Z->r[A]&0xF0)|(n>>4);                               //-!-
                  Z->fS = SIGN(Z->r[A]);
                  Z->fZ = ZERO(Z->r[A]);
                  Z->f5 = BIT5(Z->r[A]);
                  Z->f3 = BIT3(Z->r[A]);
                  Z->fH = 0;
                  Z->fP = PAR8(Z->r[A]);  Z->PF = pfPARITY;
                  Z->fN = 0;
                  FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                  REGUPD(mA | mF);
                  TIMING(5,QQ,18);
                  sprintf(is, MNEMs, "RLD");
                  break;
                case 6: // .................................. ED 77 01-110-111 : NOP
                  // fallthrough to case 7
                case 7: // .................................. ED 7F 01-111-111 : NOP
                  // NOP == "No OPeration"/Do nothing                            //-!-
                  // Registers and Memory are unaffected
                  // Flags are unaffected
                  sprintf(is, MNEMs, "NOP");
                  TIMING(1,1,4);
                  break;
              }
              break;
          }
          break;
        case 2: // .......................................... ED ?? 10-ppq-zzz
/*----------------------------------------------------------------------------
{ BC--; (DE) = (HL);    HL--; DE--; }                     ... ED A8 10-101-000 : LDD
{ BC--; (DE) = (HL);    HL--; DE--; } until (!BC)         ... ED B8 10-111-000 : LDDR

{ BC--; (DE) = (HL);    HL++; DE++; }                     ... ED A0 10-100-000 : LDI
{ BC--; (DE) = (HL);    HL++; DE++; } until (!BC)         ... ED B0 10-110-000 : LDIR

{ BC--; f* = A-(HL);    HL--;       }                     ... ED A9 10-101-001 : CPD
{ BC--; f* = A-(HL); n=(HL--);      } until (!BC || A==n) ... ED B9 10-111-001 : CPDR

{ BC--; f* = A-(HL);    HL++;       }                     ... ED A1 10-100-001 : CPI
{ BC--; f* = A-(HL); n=(HL++);      } until (!BC || A==n) ... ED B1 10-110-001 : CPIR

{  B--; (HL) = [BC];    HL--;       }                     ... ED AA 10-101-010 : IND
{  B--; (HL) = [BC];    HL--;       } until (!B)          ... ED BA 10-111-010 : INDR

{  B--; (HL) = [BC];    HL++;       }                     ... ED A2 10-100-010 : INI
{  B--; (HL) = [BC];    HL++;       } until (!B)          ... ED B2 10-110-010 : INIR

{  B--; [BC] = (HL);    HL--;       }                     ... ED AB 10-101-011 : OUTD
{  B--; [BC] = (HL);    HL--;       } until (!B)          ... ED BB 10-111-011 : OTDR

{  B--; [BC] = (HL);    HL++;       }                     ... ED A3 10-100-011 : OUTI
{  B--; [BC] = (HL);    HL++;       } until (!B)          ... ED B3 10-110-011 : OTIR
----------------------------------------------------------------------------*/
          if ((p>=2) && (z<=3))
          {
            I8    inc = ((q==0)?+1:-1);
            switch (z) // ................................. ED ?? 10-yyy-zzz
            {
              case 0: // .................................... ED ?? 10-???-000
                                                     // ..... ED A0 10-100-000 : LDI
                                                     // ..... ED A8 10-101-000 : LDD
                                                     // ..... ED B0 10-110-000 : LDIR
                                                     // ..... ED B8 10-111-000 : LDDR
                b = ((--Z->rp[BC])==0) ? false : true ;                          //-!-
                n = Z->peek8(Z->rp[HL]);                                         //-!-m
                Z->poke8(Z->rp[DE], n);                                          //-!-m
                Z->fN = 0;  ///* from z80sflag.htm
                Z->fH = 0;
                Z->fP = Z->rp[BC]?1:0;  Z->PF = pfREPEAT;
                Z->f3 = BIT3(Z->r[A] +n);
                Z->f5 = BIT1(Z->r[A] +n);  // Yes, bit-1
                FLGUPD(mfN | mfP | mf3 | mfH | mf5);
                R16UPD(mBC);
                REGUPD(mF);
                strcpy(is, "LD");
                break;
              case 1: // .................................... ED ?? 10-???-001
                                                     // ..... ED A1 10-100-001 : CPI
                                                     // ..... ED A9 10-101-001 : CPD
                                                     // ..... ED B1 10-110-001 : CPIR
                                                     // ..... ED B9 10-111-001 : CPDR
                // Somehow CP acquires an extra m-state !?                       //-!-m
                b = ((--Z->rp[BC])==0) ? false : true ;                          //-!-
                n = Z->peek8(Z->rp[HL]);                                         //-!-m
                if (Z->r[A]==n)  b = false ; // CP also terminates on a match    //-!-
                Z->fH = ((n&0x0F)>(Z->r[A]&0x0F))?1:0;
                n = Z->r[A] -n;              // Perform SUB (discard result)     //-!-
                Z->fS = SIGN(n);
                Z->fZ = ZERO(n);
                Z->fN = 1;
                Z->fP = Z->rp[BC]?1:0;  Z->PF = pfREPEAT;
                Z->f3 = BIT3(n -Z->fH); // AFTER fH calc
                Z->f5 = BIT1(n -Z->fH); // AFTER fH calc ... Yes, bit-1
                FLGUPD(mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                R16UPD(mBC);
                REGUPD(mF);
                strcpy(is, "CP");
                break;
              case 2: // .................................... ED ?? 10-???-010
                                                     // ..... ED A2 10-100-010 : INI
                                                     // ..... ED AA 10-101-010 : IND
                                                     // ..... ED B2 10-110-010 : INIR
                                                     // ..... ED BA 10-111-010 : INDR
                b = ((--Z->r[B]  )==0) ? false : true;                           //-!-
                Z->fZ = ZERO(Z->r[B]);               // Post decrement
                Z->fS = SIGN(Z->r[B]);               // Post decrement
                Z->f3 = BIT3(Z->r[B]);               // Post decrement
                Z->f5 = BIT5(Z->r[B]);               // Post decrement
                // careful not to use BC v-here-v, as it is not in synch!
                n = Z->in8((Z->r[B]<<8)|Z->r[C]);    // POST decrement           //-!-m
                Z->poke8(Z->rp[HL],n);                                           //-!-m
                Z->fC = (n > (0xFF-((Z->r[C] +inc)&0xFF)))?1:0;
                Z->fH = Z->fC;           // AFTER new fC is calculated
                Z->fN = SIGN(n);
                n = ((n +((Z->r[C] +inc) &0xFF)) &7) ^Z->r[B];
                Z->fP = PAR8(n);  Z->PF = pfPARITY;
                FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                REGUPD(mB | mF);
                strcpy(is, "IN");
                break;
              case 3: // .................................... ED ?? 10-???-011 :
                                                     // ..... ED A3 10-100-011 : OUTI
                                                     // ..... ED AB 10-101-011 : OUTD
                                                     // ..... ED B3 10-110-011 : OTIR
                                                     // ..... ED BB 10-111-011 : OTDR
                // There seems to be a deabte as to whether we should use the
                // pre [z80 undocumented] or post [Rodney Zaks] decrement
                // value of B for A15..A8
                n = Z->peek8(Z->rp[HL]);                                         //-!-m
                Z->out8((Z->r[B]<<8)|Z->r[C],n);    // PRE decrement             //-!-m
                b = ((--Z->r[B]  )==0) ? false : true;                           //-!-
                Z->fZ = ZERO(Z->r[B]);             // Post decrement
                Z->fS = SIGN(Z->r[B]);             // Post decrement
                Z->f3 = BIT3(Z->r[B]);             // Post decrement
                Z->f5 = BIT5(Z->r[B]);             // Post decrement
                Z->fC = (n > (0xFF-(Z->r[L]+inc)))?1:0;
                Z->fH = Z->fC;           // AFTER new fC is calculated
                Z->fN = SIGN(n);
                n = ((n +(Z->r[L]+inc)) &7) ^Z->r[B];
                Z->fP = PAR8(n);  Z->PF = pfPARITY;
                FLGUPD(mfC | mfN | mfP | mf3 | mfH | mf5 | mfS | mfZ);
                REGUPD(mB | mF);
                strcpy(is, (p==2)?"OUT":"OT");
                break;
            }
            strcat(is, (q==1)?"D":"I");   // build the instrcution name
            strcat(is, (p==3)?"R":"");
            sprintf(is, MNEMs, is);

            // INC or DEC  HL (and DE)
            Z->rp[HL] += inc;     // INC (+1)  or  DEC (-1)                      //-!-
            R16UPD(mHL);
            if (z==0)             // inc/dec DE for LD only
            {
              Z->rp[DE] += inc;   // INC (+1)  or  DEC (-1)                      //-!-
              R16UPD(mDE);
            }

            // If p==3 and b=true, then rewind the PC for another iteration
            if ((p==3) && b)
            {
              Z->rpt   = true;
              Z->rptPC = Z->rp[PC]; // ibc is added to PC AFTER main switch()
              Z->rp[PC] -= 2;                                                    //-!-m ?!
              R16UPD(mPC); // hmmm, is this helpful?
              TIMING(5,QQ,21);
            }
            else
            {
              Z->rpt = false;
              TIMING(4,QQ,16);
            }
            break;//switch(z)
          }
          // else fallthrough to case 0  // ................. ED ?? 00-010-zzz : NONI
        case 0: // .......................................... ED ?? 00-000-zzz : NONI
          // fallthrough to case 3
        case 3: // .......................................... ED ?? 11-011-zzz : NONI
          // NOP == "No OPeration"/Do nothing                                    //-!-
//          Z->IHLD = true;  // Do not check interrupts after this instr           //-!-
          // Registers and Memory are unaffected
          // Flags are unaffected
          TIMING(1,1,4);  // 0xED, NONI
          TIMING(1,1,4);  // 0x??, NOP
          sprintf(is, MNEMs"%s,$%02X", "NONI", "$ED", ib);
          break;
      }
      break;

  } // switch (pre)


  if (!is[0])  // No instruction was decoded
  {
    strcpy(is, "<unknown>");
    ret = false;
  }
  else
  {
    // Re-synch the flags register
    if (Z->rUpd&mF)  Z->r[F] = (Z->fC ? mfC : 0) | (Z->fN ? mfN : 0) |
                               (Z->fP ? mfP : 0) | (Z->f3 ? mf3 : 0) |
                               (Z->fH ? mfH : 0) | (Z->f5 ? mf5 : 0) |
                               (Z->fS ? mfS : 0) | (Z->fZ ? mfZ : 0) ;

    // Synchronise 8 & 16 bit registers
    SYNCH(BC,B,C);
    SYNCH(DE,D,E);
    SYNCH(HL,H,L);
    SYNCH(AF,A,F);
    SYNCH(IX,IXh,IXl);
    SYNCH(IY,IYh,IYl);

    // Update PC
    Z->rp[PC] += Z->ibc;

    // Adjust the oldPC & ibc to reflect any DD or FD prefix
    if ( (Z->idxHL!=HL) && ((pre==0xCB) || ((ib&0xDF)!=0xDD)) )
//    if ( (pre==0xCB) || ((Z->idxHL!=HL) && ((ib&0xDF)!=0xDD)) )
    {
     Z->oldPC--;
     Z->ibc++;
    }

    // Return the full instruction
    for (n = 0; n<Z->ibc; n++)  Z->op[n] = Z->peek8(Z->oldPC +n) ;

    ret = true;
  }

  // Reset the (mythical) IDX register : ((ib&0xDF)==0xDD) -> DD or FD prefix
  if ( (pre==0xCB) || ((pre!=0xCB) && ((ib&0xDF)!=0xDD)) )
  {
    Z->idxHL = HL;
    Z->idxH  = H;
    Z->idxL  = L;
  }

  // NMI ...IHLD is set by 0xDD, 0xFD and "EI"
  if ((!Z->IHLD) && Z->NMI)
  {
    // Interrupts unHALT the processor
    if (Z->HALT)
    {
      Z->r[PC]++;                                                                //-!-
      Z->HALT = false;                                                           //-!-
    }

    Z->IFF1 = 0;                                                                 //-!-
    TIMING(0,0,11);

    // ack NMI                                                                   //-!-m
    Z->rp[SP] -= 2;                                                              //-!-
    R16UPD(mSP);
    TIMING(1,0,5);

    POKE16(Z->rp[SP], Z->rp[PC]);                                                //-!-mm
    MEMUPD(Z->rp[SP], WORD, Z->rp[PC]);
    Z->rp[PC] = 0x0066;                                                          //-!-
    R16UPD(mPC);
    TIMING(2,0,6);

    return(true);
  }

  // INT ...IHLD is set by 0xDD, 0xFD and "EI" ...IFF1 is cleared by "DI"
  if ((!Z->IHLD) && (Z->INT) && (Z->IFF1))
  {
    // Interrupts unHALT the processor
    if (Z->HALT)
    {
      Z->r[PC]++;                                                                //-!-
      Z->HALT = false;                                                           //-!-
    }

    Z->IFF2 = 0;
    Z->IFF1 = 0;

    switch (Z->IM)
    {
      case 0:  // IM 0 : 8080 Compat. mode - Execute instruction on data bus
//#error we need some new way to emulate a read directly from the databus
//#error I need to spend a chunk of time working this out
//        if (chkRST(Z->DBUS)
//        {
//          instr=RST xx TIMING(0,0,13);
//        }
        break;
      case 1:  // IM 1 : Execute RST $38
        // ack INT                                                               //-!-m
        Z->rp[SP] -= 2;                                                          //-!-
        POKE16(Z->rp[SP], Z->rp[PC]);                                            //-!-mm
        MEMUPD(Z->rp[SP], WORD, Z->rp[PC]);
        Z->rp[PC] = 0x0066;                                                      //-!-
        R16UPD(mSP | mPC);
        TIMING(3,0,13);
        break;
      case 2:  // IM 2 :
        // ack INT                                                               //-!-m
        Z->rp[SP] -= 2;                                                          //-!-
        POKE16(Z->rp[SP], Z->rp[PC]);                                            //-!-mm
        MEMUPD(Z->rp[SP], WORD, Z->rp[PC]);
        Z->rp[PC] = PEEK16((Z->r[I]<<8) +Z->DBUS);   // NOT (DBUS &0x7F)         //-!-mm
        TIMING(0,0,19);
    }

  }

  return(ret);

} // Z80()


void z80_debug(Z80CPU *z) {
	int i;
#define fprintf(x,...) printf(__VA_ARGS__)


    fprintf(f, "%04X: ", z->oldPC);

    for (i=0; i<4; i++)   if (i<z->ibc)  fprintf(f, "%02X ", z->op[i]);  else  fprintf(f, "   ");

    fprintf(f, " %-20s  ; [%d,%d,%2d] %c%c%c%c%c%c%c%c", is,
                          z->M, z->M1, z->T,
                          (z->fUpd&mfS)?'S':'-', (z->fUpd&mfZ)?'Z':'-',
                          (z->fUpd&mf5)?'5':'-', (z->fUpd&mfH)?'H':'-',
                          (z->fUpd&mf3)?'3':'-', (z->fUpd&mfP)?PFs[z->PF]:'-',
                          (z->fUpd&mfN)?'N':'-', (z->fUpd&mfC)?'C':'-' );

    if (z->memUpdSz==1)
      fprintf(f, " ($%04X=  $%02X)", z->memUpdAddr, z->memUpdVal);
    else if (z->memUpdSz==2)
      fprintf(f, " ($%04X=$%04X)", z->memUpdAddr, z->memUpdVal);
    else
      fprintf(f, " (           )");

    fprintf(f, " %c%c%c%c%c%c%c%c%c%c %c%c%c",   //BCDEHLAFIR SXY
               (z->rpUpd&mBC)&&(!(z->rUpd&mB))?'b':((z->rUpd&mB)?'B':'-'),
               (z->rpUpd&mBC)&&(!(z->rUpd&mC))?'c':((z->rUpd&mC)?'C':'-'),
               (z->rpUpd&mDE)&&(!(z->rUpd&mD))?'d':((z->rUpd&mD)?'D':'-'),
               (z->rpUpd&mDE)&&(!(z->rUpd&mE))?'e':((z->rUpd&mE)?'E':'-'),
               (z->rpUpd&mHL)&&(!(z->rUpd&mH))?'h':((z->rUpd&mH)?'H':'-'),
               (z->rpUpd&mHL)&&(!(z->rUpd&mL))?'l':((z->rUpd&mL)?'L':'-'),
               (z->rpUpd&mAF)&&(!(z->rUpd&mA))?'a':((z->rUpd&mA)?'A':'-'),
               (z->rpUpd&mAF)&&(!(z->rUpd&mF))?'f':((z->rUpd&mF)?'F':'-'),
               (z->rUpd&mI)?'I':'-',
               (z->rUpd&mR)?'R':'-',
               (z->rpUpd&mSP)?'S':'-', (z->rpUpd&mIX)?'X':'-',
               (z->rpUpd&mIY)?'Y':'-' );





    fprintf(f, " %s\n", cmnt);
}

// z80.c [EOF]

//****************************************************************************
//****************************************************************************
//****************************************************************************
//****************************************************************************
//****************************************************************************

