/******************************************************************************
DoGoBoy - Sharp LR25902 CPU emulator
*******************************************************************************
Copyright (c) 2009-2013, Douglas Gore (doug@ssonic.co.uk)
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Douglas Gore nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DOUGLAS GORE BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************
Purpose:

This module provides the emulation of the Game Boy CPU core, a Sharp LR35902.
It is Z80 like with some registers, opcode modes and CPU flags removed and some
custom instructions added.
******************************************************************************/

#include <stdio.h>
#include <string.h>

#include "gameboy.h"
#include "opcodes.h"

// Turn this on if you want to record all the CPU opcodes executed and the
// register state
//#define GAMEBOY_DEBUG

// Define our CPU flags and their bit positions
#define FLAG_Z      7
#define FLAG_N      6
#define FLAG_H      5
#define FLAG_C      4

#ifdef GAMEBOY_DEBUG
int breakpoint = 0x6A0D;
int bp_count = 0x0;

char opcodeBuffer[32];
#endif

// Reads a word from memory by making two byte read requests
word __inline readWordFromMemory(unsigned int address)
{
	//return ((readByteFromMemory(address) << 8) | readByteFromMemory(address + 1));
    return ((readByteFromMemory(address + 1) << 8) | readByteFromMemory(address));
}

// Writes a word to memory using two byte writes
void __inline writeWordToMemory(unsigned int address, word value)
{
    writeByteToMemory(address, value >> 8);
    writeByteToMemory(address + 1, value & 0xFF);
}

// Push a word onto the stack
void __inline pushWordToStack(word data)
{
    REGS.w.SP--;
    writeByteToMemory(REGS.w.SP, (data >> 8) & 0xFF);
    REGS.w.SP--;
    writeByteToMemory(REGS.w.SP, data & 0xFF);
}

// Pop a word from the stack
static __inline word popWordFromStack(void)
{
    word temp;

    temp = readByteFromMemory(REGS.w.SP);
    REGS.w.SP++;
    temp |= readByteFromMemory(REGS.w.SP) << 8;
    REGS.w.SP++;
	
    return temp;
}

static __inline void setFlagZ(byte value)
{
	// Set the Z zero flag if new value is 0
	if (0x01 == value)
	{
		REGS.b.F |= 0x80;
	}
	else
	{
		REGS.b.F &= ~0x80;
	}
}

static __inline byte getFlagZ(void)
{
	return (REGS.b.F & 0x80);
}

static __inline void setFlagC(byte value)
{
	// Set the C carry flag is the result exceeds max register value
	if (0x01 == value)
	{
		REGS.b.F |= 0x10;
	}
	else
	{
		REGS.b.F &= ~0x10;
	}
}

static __inline byte getFlagC(void)
{
	return (REGS.b.F & 0x10);
}

static __inline void setFlagH(byte value)
{
	// Set H flag
	if (0x01 == value)
	{
		REGS.b.F |= 0x20;
	}
	else
	{
		REGS.b.F &= ~0x20;
	}
}

static void setFlagN(byte value)
{
	// Set N subtract flag is the new value is less than the old value
	if (0x01 == value)
	{
		REGS.b.F |= 0x40;
	}
	else
	{
		REGS.b.F &= ~0x40;
	}
}

static __inline byte getFlagN(void)
{
	return (REGS.b.F & 0x40);
}

static __inline void setZonZero(byte toCheck)
{
    if (0x00 == toCheck)
    {
        setFlagZ(1);
    }
    else
    {
        setFlagZ(0);
    }
}

// Sets the CPU to an initial state
void initCPU(void)
{
	REGS.w.PC = 0x100;
	REGS.w.SP = 0;
}

// DECrement a register
static __inline byte cpuDEC(register byte reg)
{
	byte before;

	before = reg;

    // Decrement register
    reg--;

    // Set zero flag register
    setZonZero(reg);

    // This was a negative operation so set subtract
    setFlagN(1);

	if ((before & 0x0F) == 0x00)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}
	
	// C flag is not affected

	return reg;
}

// INCrement a register
static __inline byte cpuINC(register byte reg)
{
	byte before;

	before = reg;

    // Increment register
    reg++;
	
	// Set zero flag register
    setZonZero(reg);

    setFlagN(0);

	if ((before & 0x0F) == 0x0F)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}

	// C flag is not set

	return reg;
}

// ComPare a value to that in the accumulator
static __inline void cpuCP(register byte compare_value)
{
	byte before;
    byte reg;
    sword hTest;
    
    reg = REGS.b.A;
    before = REGS.b.A;

    reg -= compare_value;

    setZonZero(reg);
    
    if (before < compare_value)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

    setFlagN(1);

    hTest = before & 0x0F;
    hTest -= compare_value & 0x0F;

    if (hTest < 0)
    {
        setFlagH(1);
    }
    else
    {
        setFlagH(0);
    }
}

// ComPLement the accumulator (flip the bits)
static __inline void cpuCPL(void)
{
    REGS.b.A = ~REGS.b.A;

    setFlagN(1);
    setFlagH(1);
	// Z and C flags not affected
}

// Set Carry Flag
static __inline void cpuSCF(void)
{
	// Z flag not affected
    setFlagN(0);
    setFlagH(0);
    setFlagC(1);
}

// Complement Carry Flag
static __inline void cpuCCF(void)
{
    // Invert the Carry flag
    if (getFlagC())
    {
        setFlagC(0);
    }
    else
    {
        setFlagC(1);
    }

	setFlagN(0);
	setFlagH(0);
	// Z flag not affected
}

// Decimal Adjust register A (accumulator)
static __inline void cpuDAA(void)
{
	if (getFlagN())
	{
		if (((REGS.b.A & 0x0F) > 0x09) || (REGS.b.F & 0x20))
		{
			REGS.b.A -= 0x06;

			if ((REGS.b.A & 0xF0) == 0xF0)
			{
				setFlagC(1);
			}
			else
			{
				setFlagC(0);
			}
		}

		if (((REGS.b.A & 0xF0) > 0x90) || (REGS.b.F & 0x10))
		{
			REGS.b.A -= 0x60;
		}
	}
	else
	{
		if (((REGS.b.A & 0x0F) > 0x09) || (REGS.b.F & 0x20))
		{
			REGS.b.A += 0x06;

			if ((REGS.b.A & 0xF0) == 0x00)
			{
				setFlagC(1);
			}
			else
			{
				setFlagC(0);
			}

			if (((REGS.b.A & 0xF0) > 0x90) || (REGS.b.F & 0x10))
			{
				REGS.b.A += 0x60;
			}
		}
	}

    setZonZero(REGS.b.A);
}

// AND the accumulator
static __inline void cpuAND(register byte value)
{
	REGS.b.A &= value;

    setZonZero(REGS.b.A);

	setFlagN(0);
	setFlagH(1);
	setFlagC(0);
}

// OR the accumulator
static __inline void cpuOR(register byte value)
{
    REGS.b.A |= value;

    setZonZero(REGS.b.A);
    
    setFlagN(0);
    setFlagH(0);
    setFlagC(0);
}

// XOR the accumulator
static __inline void cpuXOR(register byte value)
{
    REGS.b.A ^= value;

    setZonZero(REGS.b.A);
  
    setFlagN(0);
    setFlagH(0);
    setFlagC(0);
}

// SUBtractor from the accumulator
static __inline void cpuSUB(register byte value)
{
	byte before;
	sword hTest;

	before = REGS.b.A;

    REGS.b.A -= value;
	
    setZonZero(REGS.b.A);

    // Set if no borrow
	if (before < value)
	{
		setFlagC(1);
	}
	else
	{
		setFlagC(0);
	}

    setFlagN(1);

	hTest = (before & 0xF);
	hTest -= (value & 0xF);

	if (hTest < 0)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}
}

// ADD to the accumulator
static __inline void cpuADD(register byte adding)
{
	byte before = REGS.b.A;
	sword hTest;

	REGS.b.A += adding;

    setZonZero(REGS.b.A);

    if ((before + adding) > 0xFF)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

	setFlagN(0);

	hTest = (before & 0xF);
	hTest += (adding & 0xF);

	if (hTest > 0xF)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}
}

static __inline word cpuADDw(register word reg, word adding)
{
	word before = reg;

	reg += adding;

    if ((before + adding) > 0xFFFF)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

	setFlagN(0);

	if (((before & 0xFF00) & 0xF) + ((adding >> 8) & 0xF))
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}

	// Z flag not affected

	return reg;
}

static __inline void cpuADDSP(sbyte offset)
{
    word before;

    before = REGS.w.SP;

    REGS.w.SP += offset & 0xFFFF;

    if ((before + offset) > 0xFFFF)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

	if (((before & 0xFF00) & 0xF) + ((offset >> 8) & 0xF))
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}

	setFlagZ(0);
    setFlagN(0);
}

// ADd and Carry to accumulator
static __inline void cpuADC(register byte adding)
{
	byte before = REGS.b.A;
	sword hTest;

    // Check carry flag
    if (getFlagC())
    {
        adding++;
    }

	REGS.b.A += adding;

    setZonZero(REGS.b.A);

    if ((before + adding) > 0xFF)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

	setFlagN(0);

	hTest = (before & 0xF);
	hTest += (adding & 0xF);

	if (hTest > 0xF)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}
}

// SuBtract and Carry from accumulator
static __inline void cpuSBC(register byte value)
{
	byte before;
	sword hTest;

	before = REGS.b.A;

	// Is carry set?
	if (getFlagC())
	{
		value++;
	}

    REGS.b.A -= value;
	
    setZonZero(REGS.b.A);

	// Set if no borrow
	if (before < value)
	{
		setFlagC(1);
	}
	else
	{
		setFlagC(0);
	}

	hTest = (before & 0xF);
	hTest -= (value & 0xF);

	if (hTest < 0)
	{
		setFlagH(1);
	}
	else
	{
		setFlagH(0);
	}

	setFlagN(1);
}

// Test BIT in register
static __inline void cpuBIT(byte bit, byte value)
{
	if ((value & (1 << bit)) == 0)
	{
		setFlagZ(1);
	}
	else
	{
		setFlagZ(0);
	}

    setFlagN(0);
    setFlagH(1);
	// C flag not affected
}

static __inline byte cpuRES(byte bit, byte value)
{
	// No flags affected
	return value & ~(1 << bit);
}

static __inline byte cpuSET(byte bit, byte value)
{
	// No flags affected
	return value | (1 << bit);
}

// CALL operation, push address to stack then jump
static __inline void cpuCALL(byte useCondition, byte flag, byte set)
{
	word jumpAddress;
	
	jumpAddress = readWordFromMemory(REGS.w.PC);
	REGS.w.PC += 2;

	// Unconditional jump
	if (0 == useCondition)
	{
		pushWordToStack(REGS.w.PC);
		REGS.w.PC = jumpAddress;
	}
	else
	{
		// If request flag condition is met then jump
		if ((REGS.b.F & (1 << flag)) == (set << flag))
		{
			pushWordToStack(REGS.w.PC);
			REGS.w.PC = jumpAddress;
		}
	}
}

// RETurn - pop word from stack and jump to address
static __inline void cpuRET(byte flag, byte set)
{
	// Is Z flag is not set then jump
	if ((REGS.b.F & (1 << flag)) == (set << flag))
	{
        REGS.w.PC = popWordFromStack();
	}
}

// JumP to address (may be conditional)
static __inline void cpuJP(byte useCondition, byte flag, byte set)
{
	word jumpAddress;

	jumpAddress = readWordFromMemory(REGS.w.PC);
	REGS.w.PC += 2;

	if (0x00 == useCondition)
	{
		REGS.w.PC = jumpAddress;
	}
	else
	{
		// Is Z flag is not set then jump
		if ((REGS.b.F & (1 << flag)) == (set << flag))
		{
			REGS.w.PC = jumpAddress;
		}
	}
}

// Jump Relative to current address
static __inline void cpuJR(byte flag, byte set)
{
	sbyte index = readByteFromMemory(REGS.w.PC++);
	
	// Is Z flag is not set then jump
	if ((REGS.b.F & (1 << flag)) == (set << flag))
	{
		REGS.w.PC += index;
	}
}

// SWAP upper and lower nibbles
static __inline byte cpuSWAP(register byte reg)
{
    reg = ((reg & 0xF0) >> 4) | ((reg & 0x0F) << 4);

    setZonZero(reg);

	setFlagN(0);
	setFlagH(0);
	setFlagC(0);

	return reg;
}

// Shift Left into carry (LSB set to 0)
static __inline byte cpuSLA(register byte reg)
{
    byte msb = reg & 0x80;

	reg <<= 1;

	if (msb != 0)
	{
		setFlagC(1);
	}
	else
	{
		setFlagC(0);
	}

    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);
	
	return reg;
}

// Shift Right into carry (MSB set to 0)
static __inline byte cpuSRL(register byte reg)
{
    byte lsb;

    lsb = reg & 0x01;
	
    reg >>= 1;

    if (lsb)
	{
		setFlagC(1);
	}
	else
	{
		setFlagC(0);
	}
	
    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);

    return reg;
}

// Shift Right into carry (MSB unaffected)
static __inline byte cpuSRA(register byte reg)
{
    byte lsb;
    byte msb;

    lsb = reg & 0x01;
    msb = reg & 0x80;
	
    reg >>= 1;

    if (msb)
    {
        reg |= 0x80;
    }

    if (lsb)
	{
		setFlagC(1);
	}
	else
	{
		setFlagC(0);
	}
	
    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);

    return reg;
}

// Rotate Left through carry
static __inline byte cpuRL(register byte reg)
{
    byte msb;

    msb = reg & 0x80;
    
    reg <<= 1;

    // Move carry flag into bit 0
    if (getFlagC())
    {
        reg |= 0x1;
    }
    
    if (msb)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);

    return reg;
}

// Rotate Left (old bit 7 to Carry)
static __inline byte cpuRLC(register byte reg)
{
    byte msb = reg & 0x80;

	reg <<= 1;
	
	if (msb)
	{
		setFlagC(1);
        reg |= 0x01;
	}
	else
	{
		setFlagC(0);
	}

    setZonZero(reg);

	setFlagN(0);
	setFlagH(0);

	return reg;
}

// Rotate Right through carry
static __inline byte cpuRR(register byte reg)
{
	byte carry = getFlagC();
    byte lsb = reg & 0x01;

    reg >>= 1;

    if (lsb)
    {
        setFlagC(1);
    }
    else
    {
        setFlagC(0);
    }

    // If set move carry into the MSB
    if (carry)
    {
        reg |= (1 << 7);
    }
	
    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);

	return reg;
}

// Rotate Right through Carry (old bit 0 to carry)
static __inline byte cpuRRC(register byte reg)
{
    byte lsb = reg & 0x01;

    reg >>= 1;

    if (lsb)
    {
        setFlagC(1);
		reg |= (1<<7);	// Set bit 7
    }
    else
    {
        setFlagC(0);
    }

    setZonZero(reg);

    setFlagN(0);
    setFlagH(0);

	return reg;
}

// ReSarT - push current address to stack and jump to address
static __inline void cpuRST(byte address)
{
    pushWordToStack(REGS.w.PC);
    REGS.w.PC = address;
}

// LoaD HL, put SP + offset into HL
static __inline void cpuLDHL(sbyte offset)
{
	REGS.w.HL = (REGS.w.SP + offset) & 0xFFFF;

	if((REGS.w.SP + offset) > 0xFFFF)
    {
		setFlagC(1);
    }
	else
    {
		setFlagC(0);
    }

	if(((REGS.w.SP & 0x0F) + (offset & 0x0F)) > 0x0F)
    {
		setFlagH(1);
    }
	else
    {
		setFlagH(0);
    }

	setFlagZ(0);
    setFlagN(0);
}

static __inline unsigned int executeExetendedOpcode(void)
{
    unsigned int cycles_executed;
	byte opcode = readByteFromMemory(REGS.w.PC++);

#ifdef GAMEBOY_DEBUG
	writeLog("0x%04X: %s\n", REGS.w.PC - 1, cb_opcodes[opcode].text);
#endif

    cb_opcode_coverage[opcode]++;

	cycles_executed = cb_opcode_cycles[opcode];

    switch(opcode)
    {
	case 0x00: REGS.b.B = cpuRLC(REGS.b.B); break;
	case 0x01: REGS.b.C = cpuRLC(REGS.b.C); break;
	case 0x02: REGS.b.D = cpuRLC(REGS.b.D); break;
	case 0x03: REGS.b.E = cpuRLC(REGS.b.E); break;
	case 0x04: REGS.b.H = cpuRLC(REGS.b.H); break;
	case 0x05: REGS.b.L = cpuRLC(REGS.b.L); break;
	case 0x06: writeByteToMemory(REGS.w.HL, cpuRLC(readByteFromMemory(REGS.w.HL))); break;
	case 0x07: REGS.b.A = cpuRLC(REGS.b.A); break;

	case 0x08: REGS.b.B = cpuRRC(REGS.b.B); break;
	case 0x09: REGS.b.C = cpuRRC(REGS.b.C); break;
	case 0x0A: REGS.b.D = cpuRRC(REGS.b.D); break;
	case 0x0B: REGS.b.E = cpuRRC(REGS.b.E); break;
	case 0x0C: REGS.b.H = cpuRRC(REGS.b.H); break;
	case 0x0D: REGS.b.L = cpuRRC(REGS.b.L); break;
	case 0x0E: writeByteToMemory(REGS.w.HL, cpuRRC(readByteFromMemory(REGS.w.HL))); break;
	case 0x0F: REGS.b.A = cpuRRC(REGS.b.A); break;

	case 0x10: REGS.b.B = cpuRL(REGS.b.B); break;
	case 0x11: REGS.b.C = cpuRL(REGS.b.C); break;
	case 0x12: REGS.b.D = cpuRL(REGS.b.D); break;
	case 0x13: REGS.b.E = cpuRL(REGS.b.E); break;
	case 0x14: REGS.b.H = cpuRL(REGS.b.H); break;
	case 0x15: REGS.b.L = cpuRL(REGS.b.L); break;
	case 0x16: writeByteToMemory(REGS.w.HL, cpuRL(readByteFromMemory(REGS.w.HL))); break;
	case 0x17: REGS.b.A = cpuRL(REGS.b.A); break;

	case 0x18: REGS.b.B = cpuRR(REGS.b.B); break;
	case 0x19: REGS.b.C = cpuRR(REGS.b.C); break;
	case 0x1A: REGS.b.D = cpuRR(REGS.b.D); break;
	case 0x1B: REGS.b.E = cpuRR(REGS.b.E); break;
	case 0x1C: REGS.b.H = cpuRR(REGS.b.H); break;
	case 0x1D: REGS.b.L = cpuRR(REGS.b.L); break;
	case 0x1E: writeByteToMemory(REGS.w.HL, cpuRR(readByteFromMemory(REGS.w.HL))); break;
	case 0x1F: REGS.b.A = cpuRR(REGS.b.A); break;

	case 0x20: REGS.b.B = cpuSLA(REGS.b.B); break;
	case 0x21: REGS.b.C = cpuSLA(REGS.b.C); break;
	case 0x22: REGS.b.D = cpuSLA(REGS.b.D); break;
	case 0x23: REGS.b.E = cpuSLA(REGS.b.E); break;
	case 0x24: REGS.b.H = cpuSLA(REGS.b.H); break;
	case 0x25: REGS.b.L = cpuSLA(REGS.b.L); break;
	case 0x26: writeByteToMemory(REGS.w.HL, cpuSLA(readByteFromMemory(REGS.w.HL))); break;
	case 0x27: REGS.b.A = cpuSLA(REGS.b.A); break;

	case 0x28: REGS.b.B = cpuSRA(REGS.b.B); break;
	case 0x29: REGS.b.C = cpuSRA(REGS.b.C); break;
	case 0x2A: REGS.b.D = cpuSRA(REGS.b.D); break;
	case 0x2B: REGS.b.E = cpuSRA(REGS.b.E); break;
	case 0x2C: REGS.b.H = cpuSRA(REGS.b.H); break;
	case 0x2D: REGS.b.L = cpuSRA(REGS.b.L); break;
	case 0x2E: writeByteToMemory(REGS.w.HL, cpuSRA(readByteFromMemory(REGS.w.HL))); break;
	case 0x2F: REGS.b.A = cpuSRA(REGS.b.A); break;

    case 0x30: REGS.b.B = cpuSWAP(REGS.b.B); break;
	case 0x31: REGS.b.C = cpuSWAP(REGS.b.C); break;
	case 0x32: REGS.b.D = cpuSWAP(REGS.b.D); break;
	case 0x33: REGS.b.E = cpuSWAP(REGS.b.E); break;
	case 0x34: REGS.b.H = cpuSWAP(REGS.b.H); break;
	case 0x35: REGS.b.L = cpuSWAP(REGS.b.L); break;
	case 0x36: writeByteToMemory(REGS.w.HL, cpuSWAP(readByteFromMemory(REGS.w.HL))); break;
	case 0x37: REGS.b.A = cpuSWAP(REGS.b.A); break;

	case 0x38: REGS.b.B = cpuSRL(REGS.b.B); break;
	case 0x39: REGS.b.C = cpuSRL(REGS.b.C); break;
	case 0x3A: REGS.b.D = cpuSRL(REGS.b.D); break;
	case 0x3B: REGS.b.E = cpuSRL(REGS.b.E); break;
	case 0x3C: REGS.b.H = cpuSRL(REGS.b.H); break;
	case 0x3D: REGS.b.L = cpuSRL(REGS.b.L); break;
	case 0x3E: writeByteToMemory(REGS.w.HL, cpuSRL(readByteFromMemory(REGS.w.HL))); break;
	case 0x3F: REGS.b.A = cpuSRL(REGS.b.A); break;

	case 0x40: cpuBIT(0, REGS.b.B); break;
	case 0x41: cpuBIT(0, REGS.b.C); break;
	case 0x42: cpuBIT(0, REGS.b.D); break;
	case 0x43: cpuBIT(0, REGS.b.E); break;
	case 0x44: cpuBIT(0, REGS.b.H); break;
	case 0x45: cpuBIT(0, REGS.b.L); break;
	case 0x46: cpuBIT(0, readByteFromMemory(REGS.w.HL)); break;
	case 0x47: cpuBIT(0, REGS.b.A); break;

	case 0x48: cpuBIT(1, REGS.b.B); break;
	case 0x49: cpuBIT(1, REGS.b.C); break;
	case 0x4A: cpuBIT(1, REGS.b.D); break;
	case 0x4B: cpuBIT(1, REGS.b.E); break;
	case 0x4C: cpuBIT(1, REGS.b.H); break;
	case 0x4D: cpuBIT(1, REGS.b.L); break;
	case 0x4E: cpuBIT(1, readByteFromMemory(REGS.w.HL)); break;
	case 0x4F: cpuBIT(1, REGS.b.A); break;

	case 0x50: cpuBIT(2, REGS.b.B); break;
	case 0x51: cpuBIT(2, REGS.b.C); break;
	case 0x52: cpuBIT(2, REGS.b.D); break;
	case 0x53: cpuBIT(2, REGS.b.E); break;
	case 0x54: cpuBIT(2, REGS.b.H); break;
	case 0x55: cpuBIT(2, REGS.b.L); break;
	case 0x56: cpuBIT(2, readByteFromMemory(REGS.w.HL)); break;
	case 0x57: cpuBIT(2, REGS.b.A); break;

	case 0x58: cpuBIT(3, REGS.b.B); break;
	case 0x59: cpuBIT(3, REGS.b.C); break;
	case 0x5A: cpuBIT(3, REGS.b.D); break;
	case 0x5B: cpuBIT(3, REGS.b.E); break;
	case 0x5C: cpuBIT(3, REGS.b.H); break;
	case 0x5D: cpuBIT(3, REGS.b.L); break;
	case 0x5E: cpuBIT(3, readByteFromMemory(REGS.w.HL)); break;
	case 0x5F: cpuBIT(3, REGS.b.A); break;

	case 0x60: cpuBIT(4, REGS.b.B); break;
	case 0x61: cpuBIT(4, REGS.b.C); break;
	case 0x62: cpuBIT(4, REGS.b.D); break;
	case 0x63: cpuBIT(4, REGS.b.E); break;
	case 0x64: cpuBIT(4, REGS.b.H); break;
	case 0x65: cpuBIT(4, REGS.b.L); break;
	case 0x66: cpuBIT(4, readByteFromMemory(REGS.w.HL)); break;
	case 0x67: cpuBIT(4, REGS.b.A); break;

	case 0x68: cpuBIT(5, REGS.b.B); break;
	case 0x69: cpuBIT(5, REGS.b.C); break;
	case 0x6A: cpuBIT(5, REGS.b.D); break;
	case 0x6B: cpuBIT(5, REGS.b.E); break;
	case 0x6C: cpuBIT(5, REGS.b.H); break;
	case 0x6D: cpuBIT(5, REGS.b.L); break;
	case 0x6E: cpuBIT(5, readByteFromMemory(REGS.w.HL)); break;
	case 0x6F: cpuBIT(5, REGS.b.A); break;

	case 0x70: cpuBIT(6, REGS.b.B); break;
	case 0x71: cpuBIT(6, REGS.b.C); break;
	case 0x72: cpuBIT(6, REGS.b.D); break;
	case 0x73: cpuBIT(6, REGS.b.E); break;
	case 0x74: cpuBIT(6, REGS.b.H); break;
	case 0x75: cpuBIT(6, REGS.b.L); break;
	case 0x76: cpuBIT(6, readByteFromMemory(REGS.w.HL)); break;
	case 0x77: cpuBIT(6, REGS.b.A); break;

	case 0x78: cpuBIT(7, REGS.b.B); break;
	case 0x79: cpuBIT(7, REGS.b.C); break;
	case 0x7A: cpuBIT(7, REGS.b.D); break;
	case 0x7B: cpuBIT(7, REGS.b.E); break;
	case 0x7C: cpuBIT(7, REGS.b.H); break;
	case 0x7D: cpuBIT(7, REGS.b.L); break;
	case 0x7E: cpuBIT(7, readByteFromMemory(REGS.w.HL)); break;
	case 0x7F: cpuBIT(7, REGS.b.A); break;

	case 0x80: REGS.b.B = cpuRES(0, REGS.b.B); break;
	case 0x81: REGS.b.C = cpuRES(0, REGS.b.C); break;
	case 0x82: REGS.b.D = cpuRES(0, REGS.b.D); break;
	case 0x83: REGS.b.E = cpuRES(0, REGS.b.E); break;
	case 0x84: REGS.b.H = cpuRES(0, REGS.b.H); break;
	case 0x85: REGS.b.L = cpuRES(0, REGS.b.L); break;
	case 0x86: writeByteToMemory(REGS.w.HL, cpuRES(0, readByteFromMemory(REGS.w.HL))); break;
	case 0x87: REGS.b.A = cpuRES(0, REGS.b.A); break;

	case 0x88: REGS.b.B = cpuRES(1, REGS.b.B); break;
	case 0x89: REGS.b.C = cpuRES(1, REGS.b.C); break;
	case 0x8A: REGS.b.D = cpuRES(1, REGS.b.D); break;
	case 0x8B: REGS.b.E = cpuRES(1, REGS.b.E); break;
	case 0x8C: REGS.b.H = cpuRES(1, REGS.b.H); break;
	case 0x8D: REGS.b.L = cpuRES(1, REGS.b.L); break;
	case 0x8E: writeByteToMemory(REGS.w.HL, cpuRES(1, readByteFromMemory(REGS.w.HL))); break;
	case 0x8F: REGS.b.A = cpuRES(1, REGS.b.A); break;

	case 0x90: REGS.b.B = cpuRES(2, REGS.b.B); break;
	case 0x91: REGS.b.C = cpuRES(2, REGS.b.C); break;
	case 0x92: REGS.b.D = cpuRES(2, REGS.b.D); break;
	case 0x93: REGS.b.E = cpuRES(2, REGS.b.E); break;
	case 0x94: REGS.b.H = cpuRES(2, REGS.b.H); break;
	case 0x95: REGS.b.L = cpuRES(2, REGS.b.L); break;
	case 0x96: writeByteToMemory(REGS.w.HL, cpuRES(2, readByteFromMemory(REGS.w.HL))); break;
	case 0x97: REGS.b.A = cpuRES(2, REGS.b.A); break;

	case 0x98: REGS.b.B = cpuRES(3, REGS.b.B); break;
	case 0x99: REGS.b.C = cpuRES(3, REGS.b.C); break;
	case 0x9A: REGS.b.D = cpuRES(3, REGS.b.D); break;
	case 0x9B: REGS.b.E = cpuRES(3, REGS.b.E); break;
	case 0x9C: REGS.b.H = cpuRES(3, REGS.b.H); break;
	case 0x9D: REGS.b.L = cpuRES(3, REGS.b.L); break;
	case 0x9E: writeByteToMemory(REGS.w.HL, cpuRES(3, readByteFromMemory(REGS.w.HL))); break;
	case 0x9F: REGS.b.A = cpuRES(3, REGS.b.A); break;

	case 0xA0: REGS.b.B = cpuRES(4, REGS.b.B); break;
	case 0xA1: REGS.b.C = cpuRES(4, REGS.b.C); break;
	case 0xA2: REGS.b.D = cpuRES(4, REGS.b.D); break;
	case 0xA3: REGS.b.E = cpuRES(4, REGS.b.E); break;
	case 0xA4: REGS.b.H = cpuRES(4, REGS.b.H); break;
	case 0xA5: REGS.b.L = cpuRES(4, REGS.b.L); break;
	case 0xA6: writeByteToMemory(REGS.w.HL, cpuRES(4, readByteFromMemory(REGS.w.HL))); break;
	case 0xA7: REGS.b.A = cpuRES(4, REGS.b.A); break;

	case 0xA8: REGS.b.B = cpuRES(5, REGS.b.B); break;
	case 0xA9: REGS.b.C = cpuRES(5, REGS.b.C); break;
	case 0xAA: REGS.b.D = cpuRES(5, REGS.b.D); break;
	case 0xAB: REGS.b.E = cpuRES(5, REGS.b.E); break;
	case 0xAC: REGS.b.H = cpuRES(5, REGS.b.H); break;
	case 0xAD: REGS.b.L = cpuRES(5, REGS.b.L); break;
	case 0xAE: writeByteToMemory(REGS.w.HL, cpuRES(5, readByteFromMemory(REGS.w.HL))); break;
	case 0xAF: REGS.b.A = cpuRES(5, REGS.b.A); break;

	case 0xB0: REGS.b.B = cpuRES(6, REGS.b.B); break;
	case 0xB1: REGS.b.C = cpuRES(6, REGS.b.C); break;
	case 0xB2: REGS.b.D = cpuRES(6, REGS.b.D); break;
	case 0xB3: REGS.b.E = cpuRES(6, REGS.b.E); break;
	case 0xB4: REGS.b.H = cpuRES(6, REGS.b.H); break;
	case 0xB5: REGS.b.L = cpuRES(6, REGS.b.L); break;
	case 0xB6: writeByteToMemory(REGS.w.HL, cpuRES(6, readByteFromMemory(REGS.w.HL))); break;
	case 0xB7: REGS.b.A = cpuRES(6, REGS.b.A); break;

	case 0xB8: REGS.b.B = cpuRES(7, REGS.b.B); break;
	case 0xB9: REGS.b.C = cpuRES(7, REGS.b.C); break;
	case 0xBA: REGS.b.D = cpuRES(7, REGS.b.D); break;
	case 0xBB: REGS.b.E = cpuRES(7, REGS.b.E); break;
	case 0xBC: REGS.b.H = cpuRES(7, REGS.b.H); break;
	case 0xBD: REGS.b.L = cpuRES(7, REGS.b.L); break;
	case 0xBE: writeByteToMemory(REGS.w.HL, cpuRES(7, readByteFromMemory(REGS.w.HL))); break;
	case 0xBF: REGS.b.A = cpuRES(7, REGS.b.A); break;

	case 0xC0: REGS.b.B = cpuSET(0, REGS.b.B); break;
	case 0xC1: REGS.b.C = cpuSET(0, REGS.b.C); break;
	case 0xC2: REGS.b.D = cpuSET(0, REGS.b.D); break;
	case 0xC3: REGS.b.E = cpuSET(0, REGS.b.E); break;
	case 0xC4: REGS.b.H = cpuSET(0, REGS.b.H); break;
	case 0xC5: REGS.b.L = cpuSET(0, REGS.b.L); break;
	case 0xC6: writeByteToMemory(REGS.w.HL, cpuSET(0, readByteFromMemory(REGS.w.HL))); break;
	case 0xC7: REGS.b.A = cpuSET(0, REGS.b.A); break;

	case 0xC8: REGS.b.B = cpuSET(1, REGS.b.B); break;
	case 0xC9: REGS.b.C = cpuSET(1, REGS.b.C); break;
	case 0xCA: REGS.b.D = cpuSET(1, REGS.b.D); break;
	case 0xCB: REGS.b.E = cpuSET(1, REGS.b.E); break;
	case 0xCC: REGS.b.H = cpuSET(1, REGS.b.H); break;
	case 0xCD: REGS.b.L = cpuSET(1, REGS.b.L); break;
	case 0xCE: writeByteToMemory(REGS.w.HL, cpuSET(1, readByteFromMemory(REGS.w.HL))); break;
	case 0xCF: REGS.b.A = cpuSET(1, REGS.b.A); break;

	case 0xD0: REGS.b.B = cpuSET(2, REGS.b.B); break;
	case 0xD1: REGS.b.C = cpuSET(2, REGS.b.C); break;
	case 0xD2: REGS.b.D = cpuSET(2, REGS.b.D); break;
	case 0xD3: REGS.b.E = cpuSET(2, REGS.b.E); break;
	case 0xD4: REGS.b.H = cpuSET(2, REGS.b.H); break;
	case 0xD5: REGS.b.L = cpuSET(2, REGS.b.L); break;
	case 0xD6: writeByteToMemory(REGS.w.HL, cpuSET(2, readByteFromMemory(REGS.w.HL))); break;
	case 0xD7: REGS.b.A = cpuSET(2, REGS.b.A); break;

	case 0xD8: REGS.b.B = cpuSET(3, REGS.b.B); break;
	case 0xD9: REGS.b.C = cpuSET(3, REGS.b.C); break;
	case 0xDA: REGS.b.D = cpuSET(3, REGS.b.D); break;
	case 0xDB: REGS.b.E = cpuSET(3, REGS.b.E); break;
	case 0xDC: REGS.b.H = cpuSET(3, REGS.b.H); break;
	case 0xDD: REGS.b.L = cpuSET(3, REGS.b.L); break;
	case 0xDE: writeByteToMemory(REGS.w.HL, cpuSET(3, readByteFromMemory(REGS.w.HL))); break;
	case 0xDF: REGS.b.A = cpuSET(3, REGS.b.A); break;

	case 0xE0: REGS.b.B = cpuSET(4, REGS.b.B); break;
	case 0xE1: REGS.b.C = cpuSET(4, REGS.b.C); break;
	case 0xE2: REGS.b.D = cpuSET(4, REGS.b.D); break;
	case 0xE3: REGS.b.E = cpuSET(4, REGS.b.E); break;
	case 0xE4: REGS.b.H = cpuSET(4, REGS.b.H); break;
	case 0xE5: REGS.b.L = cpuSET(4, REGS.b.L); break;
	case 0xE6: writeByteToMemory(REGS.w.HL, cpuSET(4, readByteFromMemory(REGS.w.HL))); break;
	case 0xE7: REGS.b.A = cpuSET(4, REGS.b.A); break;

	case 0xE8: REGS.b.B = cpuSET(5, REGS.b.B); break;
	case 0xE9: REGS.b.C = cpuSET(5, REGS.b.C); break;
	case 0xEA: REGS.b.D = cpuSET(5, REGS.b.D); break;
	case 0xEB: REGS.b.E = cpuSET(5, REGS.b.E); break;
	case 0xEC: REGS.b.H = cpuSET(5, REGS.b.H); break;
	case 0xED: REGS.b.L = cpuSET(5, REGS.b.L); break;
	case 0xEE: writeByteToMemory(REGS.w.HL, cpuSET(5, readByteFromMemory(REGS.w.HL))); break;
	case 0xEF: REGS.b.A = cpuSET(5, REGS.b.A); break;

	case 0xF0: REGS.b.B = cpuSET(6, REGS.b.B); break;
	case 0xF1: REGS.b.C = cpuSET(6, REGS.b.C); break;
	case 0xF2: REGS.b.D = cpuSET(6, REGS.b.D); break;
	case 0xF3: REGS.b.E = cpuSET(6, REGS.b.E); break;
	case 0xF4: REGS.b.H = cpuSET(6, REGS.b.H); break;
	case 0xF5: REGS.b.L = cpuSET(6, REGS.b.L); break;
	case 0xF6: writeByteToMemory(REGS.w.HL, cpuSET(6, readByteFromMemory(REGS.w.HL))); break;
	case 0xF7: REGS.b.A = cpuSET(6, REGS.b.A); break;

	case 0xF8: REGS.b.B = cpuSET(7, REGS.b.B); break;
	case 0xF9: REGS.b.C = cpuSET(7, REGS.b.C); break;
	case 0xFA: REGS.b.D = cpuSET(7, REGS.b.D); break;
	case 0xFB: REGS.b.E = cpuSET(7, REGS.b.E); break;
	case 0xFC: REGS.b.H = cpuSET(7, REGS.b.H); break;
	case 0xFD: REGS.b.L = cpuSET(7, REGS.b.L); break;
	case 0xFE: writeByteToMemory(REGS.w.HL, cpuSET(7, readByteFromMemory(REGS.w.HL))); break;
	case 0xFF: REGS.b.A = cpuSET(7, REGS.b.A); break;

	default:
		printf("CB opcode 0x%X not supported yet!\n", opcode);
		exit_with_debug();
	}

	return cycles_executed;
}

// Function to emulate the fetch, decode and execute cycle
unsigned int executeOpcode(void)
{
    unsigned int cycles_executed;
	
	// Fetch
	byte opcode = readByteFromMemory(REGS.w.PC++);
    
#ifdef GAMEBOY_DEBUG
    switch(opcode_params[opcode])
    {
    case PARAM_NONE:
        sprintf(opcodeBuffer, opcode_labels[opcode]);
        break;

    case PARAM_BYTE:
        sprintf(opcodeBuffer, opcode_labels[opcode], readByteFromMemory(REGS.w.PC));
        break;

    case PARAM_WORD:
        sprintf(opcodeBuffer, opcode_labels[opcode], readWordFromMemory(REGS.w.PC));
        break;

    case PARAM_JUMP:
        sprintf(opcodeBuffer, opcode_labels[opcode], REGS.w.PC + (sbyte)readByteFromMemory(REGS.w.PC) + 1);
        break;
    }

    writeLog("0x%04X: %s\n", REGS.w.PC - 1, opcodeBuffer);
	//writeLog("0x%04X: %s\n", REGS.w.PC - 1, opcode_labels[opcode], readWordFromMemory(REGS.w.PC));
#endif

    //opRecord[opIndex++] = pcOffset;

    opcode_coverage[opcode]++;

#ifdef GAMEBOY_DEBUG
	if (REGS.w.PC == breakpoint)
	{
		if (bp_count-- == 0x00)
		{
			printf("Breakpoint reached, terminating\n");
			exit_with_debug();
		}
	}
#endif

	cycles_executed = opcode_cycles[opcode];

	// Decode then execute
    switch(opcode)
    {
	// Execute an extended CB opcode
	case 0xCB: cycles_executed += executeExetendedOpcode(); break;

    // NOPs
    case 0x00:
	case 0xD3:	// Non-Z80
	case 0xDB:	// Non-Z80
	case 0xDD:	// Non-Z80
	case 0xE3:	// Non-Z80
	case 0xE4:	// Non-Z80
	case 0xEB:	// Non-Z80
	case 0xEC:	// Non-Z80
    case 0xF4:	// Non-Z80
    case 0xFC:	// Non-Z80
    case 0xFD:	// Non-Z80
        // Do nothing
    break;

    // Load immediate, LD rr, n
    case 0x06: REGS.b.B = readByteFromMemory(REGS.w.PC++); break;
    case 0x0E: REGS.b.C = readByteFromMemory(REGS.w.PC++); break;
    case 0x16: REGS.b.D = readByteFromMemory(REGS.w.PC++); break;
    case 0x1E: REGS.b.E = readByteFromMemory(REGS.w.PC++); break;
    case 0x26: REGS.b.H = readByteFromMemory(REGS.w.PC++); break;
    case 0x2E: REGS.b.L = readByteFromMemory(REGS.w.PC++); break;
    case 0x36: writeByteToMemory(REGS.w.HL, readByteFromMemory(REGS.w.PC++)); break;
    case 0x3E: REGS.b.A = readByteFromMemory(REGS.w.PC++); break;

    // Load register, LD r, r'
    case 0x40: REGS.b.B = REGS.b.B; break;
    case 0x41: REGS.b.B = REGS.b.C; break;
    case 0x42: REGS.b.B = REGS.b.D; break;
    case 0x43: REGS.b.B = REGS.b.E; break;
    case 0x44: REGS.b.B = REGS.b.H; break;
    case 0x45: REGS.b.B = REGS.b.L; break;
    case 0x46: REGS.b.B = readByteFromMemory(REGS.w.HL); break;
    case 0x47: REGS.b.B = REGS.b.A; break;
    case 0x48: REGS.b.C = REGS.b.B; break;
    case 0x49: REGS.b.C = REGS.b.C; break;
    case 0x4A: REGS.b.C = REGS.b.D; break;
    case 0x4B: REGS.b.C = REGS.b.E; break;
    case 0x4C: REGS.b.C = REGS.b.H; break;
    case 0x4D: REGS.b.C = REGS.b.L; break;
    case 0x4E: REGS.b.C = readByteFromMemory(REGS.w.HL); break;
    case 0x4F: REGS.b.C = REGS.b.A; break;
    case 0x50: REGS.b.D = REGS.b.B; break;
    case 0x51: REGS.b.D = REGS.b.C; break;
    case 0x52: REGS.b.D = REGS.b.D; break;
    case 0x53: REGS.b.D = REGS.b.E; break;
    case 0x54: REGS.b.D = REGS.b.H; break;
    case 0x55: REGS.b.D = REGS.b.L; break;
    case 0x56: REGS.b.D = readByteFromMemory(REGS.w.HL); break;
    case 0x57: REGS.b.D = REGS.b.A; break;
    case 0x58: REGS.b.E = REGS.b.B; break;
    case 0x59: REGS.b.E = REGS.b.C; break;
    case 0x5A: REGS.b.E = REGS.b.D; break;
    case 0x5B: REGS.b.E = REGS.b.E; break;
    case 0x5C: REGS.b.E = REGS.b.H; break;
    case 0x5D: REGS.b.E = REGS.b.L; break;
    case 0x5E: REGS.b.E = readByteFromMemory(REGS.w.HL); break;
    case 0x5F: REGS.b.E = REGS.b.A; break;
    case 0x60: REGS.b.H = REGS.b.B; break;
    case 0x61: REGS.b.H = REGS.b.C; break;
    case 0x62: REGS.b.H = REGS.b.D; break;
    case 0x63: REGS.b.H = REGS.b.E; break;
    case 0x64: REGS.b.H = REGS.b.H; break;
    case 0x65: REGS.b.H = REGS.b.L; break;
    case 0x66: REGS.b.H = readByteFromMemory(REGS.w.HL); break;
    case 0x67: REGS.b.H = REGS.b.A; break;
    case 0x68: REGS.b.L = REGS.b.B; break;
    case 0x69: REGS.b.L = REGS.b.C; break;
    case 0x6A: REGS.b.L = REGS.b.D; break;
    case 0x6B: REGS.b.L = REGS.b.E; break;
    case 0x6C: REGS.b.L = REGS.b.H; break;
    case 0x6D: REGS.b.L = REGS.b.L; break;
    case 0x6E: REGS.b.L = readByteFromMemory(REGS.w.HL); break;
    case 0x6F: REGS.b.L = REGS.b.A; break;
    case 0x70: writeByteToMemory(REGS.w.HL, REGS.b.B); break;
    case 0x71: writeByteToMemory(REGS.w.HL, REGS.b.C); break;
    case 0x72: writeByteToMemory(REGS.w.HL, REGS.b.D); break;
    case 0x73: writeByteToMemory(REGS.w.HL, REGS.b.E); break;
    case 0x74: writeByteToMemory(REGS.w.HL, REGS.b.H); break;
    case 0x75: writeByteToMemory(REGS.w.HL, REGS.b.L); break;
    case 0x77: writeByteToMemory(REGS.w.HL, REGS.b.A); break;
    case 0x78: REGS.b.A = REGS.b.B; break;
    case 0x79: REGS.b.A = REGS.b.C; break;
    case 0x7A: REGS.b.A = REGS.b.D; break;
    case 0x7B: REGS.b.A = REGS.b.E; break;
    case 0x7C: REGS.b.A = REGS.b.H; break;
    case 0x7D: REGS.b.A = REGS.b.L; break;
    case 0x7E: REGS.b.A = readByteFromMemory(REGS.w.HL); break;
    case 0x7F: REGS.b.A = REGS.b.A; break;

    // LD (HLI), A (non-Z80)
    case 0x22: writeByteToMemory(REGS.w.HL, REGS.b.A); REGS.w.HL++; break;

	// LD (HLD), A (Non Z-80)
	case 0x32: writeByteToMemory(REGS.w.HL, REGS.b.A); REGS.w.HL--; break;

    // LD A, (HLI)
	case 0x2A: REGS.b.A = readByteFromMemory(REGS.w.HL); REGS.w.HL++; break;

    // LD A, (HLD)
    case 0x3A: REGS.b.A = readByteFromMemory(REGS.w.HL); REGS.w.HL--; break;

    // LD ($FF00+n), A (non-Z80)
	case 0xE0: writeByteToMemory(0xFF00 + readByteFromMemory(REGS.w.PC++), REGS.b.A); break;

    // LD A, ($FF00+n) (non-Z80)
	case 0xF0: REGS.b.A = readByteFromMemory(0xFF00 + readByteFromMemory(REGS.w.PC++)); break;

	//  LD ($FF00+C), A
	case 0xE2: writeByteToMemory(0xFF00 + REGS.b.C, REGS.b.A); break;

	//  LD A, ($FF00+C)
	case 0xF2: REGS.b.A = readByteFromMemory(0xFF00 + REGS.b.C); break;

    // 16-bit loads (LD rr, nn)
    case 0x01: REGS.w.BC = readWordFromMemory(REGS.w.PC); REGS.w.PC += 2; break;
    case 0x11: REGS.w.DE = readWordFromMemory(REGS.w.PC); REGS.w.PC += 2; break;
    case 0x21: REGS.w.HL = readWordFromMemory(REGS.w.PC); REGS.w.PC += 2; break;
    case 0x31: REGS.w.SP = readWordFromMemory(REGS.w.PC); REGS.w.PC += 2; break;

    // LD (rr), A
	case 0x02: writeByteToMemory(REGS.w.BC, REGS.b.A); break;
	case 0x12: writeByteToMemory(REGS.w.DE, REGS.b.A); break;

    // LD nn, A (non-Z80)
    case 0xEA: writeByteToMemory(readWordFromMemory(REGS.w.PC), REGS.b.A); REGS.w.PC += 2; break;

	// LD A, (nn)
    case 0xFA: REGS.b.A = readByteFromMemory(readWordFromMemory(REGS.w.PC)); REGS.w.PC += 2; break;

	// LD A, (rr)
	case 0x0A: REGS.b.A = readByteFromMemory(REGS.w.BC); break;
	case 0x1A: REGS.b.A = readByteFromMemory(REGS.w.DE); break;

	// LD (nn), SP (non-Z80)
	case 0x08: writeWordToMemory(readWordFromMemory(REGS.w.PC), REGS.w.SP); REGS.w.PC += 2; break;

	// LDHL SP, d
	case 0xF8: cpuLDHL((sbyte)readByteFromMemory(REGS.w.PC++)); break;

	// LD SP, HL
    case 0xF9: REGS.w.SP = REGS.w.HL; break;

    // PUSH
    case 0xC5: pushWordToStack(REGS.w.BC); break;
    case 0xD5: pushWordToStack(REGS.w.DE); break;
    case 0xE5: pushWordToStack(REGS.w.HL); break;
    case 0xF5: pushWordToStack(REGS.w.AF); break;

    // POP
    case 0xC1: REGS.w.BC = popWordFromStack(); break;
    case 0xD1: REGS.w.DE = popWordFromStack(); break;
    case 0xE1: REGS.w.HL = popWordFromStack(); break;
    case 0xF1: REGS.w.AF = popWordFromStack(); break;

    // INC 16-bit
    case 0x03: REGS.w.BC++; break;
    case 0x13: REGS.w.DE++; break;
    case 0x23: REGS.w.HL++; break;
    case 0x33: REGS.w.SP++; break;

    // DEC 16-bit
    case 0x0B: REGS.w.BC--; break;
    case 0x1B: REGS.w.DE--; break;
    case 0x2B: REGS.w.HL--; break;
    case 0x3B: REGS.w.SP--; break;

    // INC 8-bit
    case 0x04: REGS.b.B = cpuINC(REGS.b.B); break;
    case 0x0C: REGS.b.C = cpuINC(REGS.b.C); break;
    case 0x14: REGS.b.D = cpuINC(REGS.b.D); break;
    case 0x1C: REGS.b.E = cpuINC(REGS.b.E); break;
    case 0x24: REGS.b.H = cpuINC(REGS.b.H); break;
    case 0x2C: REGS.b.L = cpuINC(REGS.b.L); break;
    case 0x34: writeByteToMemory(REGS.w.HL, cpuINC(readByteFromMemory(REGS.w.HL))); break;
    case 0x3C: REGS.b.A = cpuINC(REGS.b.A); break;

    // DEC 8-bit
    case 0x05: REGS.b.B = cpuDEC(REGS.b.B); break;
    case 0x0D: REGS.b.C = cpuDEC(REGS.b.C); break;
    case 0x15: REGS.b.D = cpuDEC(REGS.b.D); break;
    case 0x1D: REGS.b.E = cpuDEC(REGS.b.E); break;
    case 0x25: REGS.b.H = cpuDEC(REGS.b.H); break;
    case 0x2D: REGS.b.L = cpuDEC(REGS.b.L); break;
    case 0x35: writeByteToMemory(REGS.w.HL, cpuDEC(readByteFromMemory(REGS.w.HL))); break;
    case 0x3D: REGS.b.A = cpuDEC(REGS.b.A); break;

    // ADD r
    case 0x80: cpuADD(REGS.b.B); break;
    case 0x81: cpuADD(REGS.b.C); break;
    case 0x82: cpuADD(REGS.b.D); break;
    case 0x83: cpuADD(REGS.b.E); break;
    case 0x84: cpuADD(REGS.b.H); break;
    case 0x85: cpuADD(REGS.b.L); break;
    case 0x86: cpuADD(readByteFromMemory(REGS.w.HL)); break;
    case 0x87: cpuADD(REGS.b.A); break;

    // ADC r
    case 0x88: cpuADC(REGS.b.B); break;
    case 0x89: cpuADC(REGS.b.C); break;
    case 0x8A: cpuADC(REGS.b.D); break;
    case 0x8B: cpuADC(REGS.b.E); break;
    case 0x8C: cpuADC(REGS.b.H); break;
    case 0x8D: cpuADC(REGS.b.L); break;
    case 0x8E: cpuADC(readByteFromMemory(REGS.w.HL)); break;
    case 0x8F: cpuADC(REGS.b.A); break;

	// SUB r
    case 0x90: cpuSUB(REGS.b.B); break;
    case 0x91: cpuSUB(REGS.b.C); break;
    case 0x92: cpuSUB(REGS.b.D); break;
    case 0x93: cpuSUB(REGS.b.E); break;
    case 0x94: cpuSUB(REGS.b.H); break;
    case 0x95: cpuSUB(REGS.b.L); break;
    case 0x96: cpuSUB(readByteFromMemory(REGS.w.HL)); break;
    case 0x97: cpuSUB(REGS.b.A); break;

    // SBC r
    case 0x98: cpuSBC(REGS.b.B); break;
    case 0x99: cpuSBC(REGS.b.C); break;
    case 0x9A: cpuSBC(REGS.b.D); break;
    case 0x9B: cpuSBC(REGS.b.E); break;
    case 0x9C: cpuSBC(REGS.b.H); break;
    case 0x9D: cpuSBC(REGS.b.L); break;
    case 0x9E: cpuSBC(readByteFromMemory(REGS.w.HL)); break;
    case 0x9F: cpuSBC(REGS.b.A); break;

	// AND r
    case 0xA0: cpuAND(REGS.b.B); break;
    case 0xA1: cpuAND(REGS.b.C); break;
    case 0xA2: cpuAND(REGS.b.D); break;
    case 0xA3: cpuAND(REGS.b.E); break;
    case 0xA4: cpuAND(REGS.b.H); break;
    case 0xA5: cpuAND(REGS.b.L); break;
    case 0xA6: cpuAND(readByteFromMemory(REGS.w.HL)); break;
    case 0xA7: cpuAND(REGS.b.A); break;

    // XOR r
    case 0xA8: cpuXOR(REGS.b.B); break;
    case 0xA9: cpuXOR(REGS.b.C); break;
    case 0xAA: cpuXOR(REGS.b.D); break;
    case 0xAB: cpuXOR(REGS.b.E); break;
    case 0xAC: cpuXOR(REGS.b.H); break;
    case 0xAD: cpuXOR(REGS.b.L); break;
    case 0xAE: cpuXOR(readByteFromMemory(REGS.w.HL)); break;
    case 0xAF: cpuXOR(REGS.b.A); break;

    // OR r
    case 0xB0: cpuOR(REGS.b.B); break;
    case 0xB1: cpuOR(REGS.b.C); break;
    case 0xB2: cpuOR(REGS.b.D); break;
    case 0xB3: cpuOR(REGS.b.E); break;
    case 0xB4: cpuOR(REGS.b.H); break;
    case 0xB5: cpuOR(REGS.b.L); break;
    case 0xB6: cpuOR(readByteFromMemory(REGS.w.HL)); break;
    case 0xB7: cpuOR(REGS.b.A); break;
  
    // CP r
    case 0xB8: cpuCP(REGS.b.B); break;
    case 0xB9: cpuCP(REGS.b.C); break;
    case 0xBA: cpuCP(REGS.b.D); break;
    case 0xBB: cpuCP(REGS.b.E); break;
    case 0xBC: cpuCP(REGS.b.H); break;
    case 0xBD: cpuCP(REGS.b.L); break;
    case 0xBE: cpuCP(readByteFromMemory(REGS.w.HL)); break;
    case 0xBF: cpuCP(REGS.b.A); break;

    // AND n
    case 0xC6: cpuADD(readByteFromMemory(REGS.w.PC++)); break;
    case 0xCE: cpuADC(readByteFromMemory(REGS.w.PC++)); break;
    case 0xD6: cpuSUB(readByteFromMemory(REGS.w.PC++)); break;
    case 0xDE: cpuSBC(readByteFromMemory(REGS.w.PC++)); break;
    case 0xE6: cpuAND(readByteFromMemory(REGS.w.PC++)); break;
    case 0xEE: cpuXOR(readByteFromMemory(REGS.w.PC++)); break;
    case 0xF6: cpuOR(readByteFromMemory(REGS.w.PC++)); break;
    case 0xFE: cpuCP(readByteFromMemory(REGS.w.PC++)); break;

	// 16-bit ADD
	case 0x09: REGS.w.HL = cpuADDw(REGS.w.HL, REGS.w.BC); break;
	case 0x19: REGS.w.HL = cpuADDw(REGS.w.HL, REGS.w.DE); break;
	case 0x29: REGS.w.HL = cpuADDw(REGS.w.HL, REGS.w.HL); break;
	case 0x39: REGS.w.HL = cpuADDw(REGS.w.HL, REGS.w.SP); break;

	// ADD SP, d (non-Z80)
	case 0xE8: cpuADDSP((sbyte)readByteFromMemory(REGS.w.PC++)); break;

    // JUMPs
   
    // JR e
    case 0x18: REGS.w.PC += (sbyte)readByteFromMemory(REGS.w.PC); REGS.w.PC++; break;

    // JR NZ, e
    case 0x20: cpuJR(FLAG_Z, 0); break;
    case 0x28: cpuJR(FLAG_Z, 1); break;
    case 0x30: cpuJR(FLAG_C, 0); break;
    case 0x38: cpuJR(FLAG_C, 1); break;

	case 0xC2: cpuJP(1, FLAG_Z, 0); break;
	case 0xCA: cpuJP(1, FLAG_Z, 1); break;
	case 0xD2: cpuJP(1, FLAG_C, 0); break;
	case 0xDA: cpuJP(1, FLAG_C, 1); break;
    case 0xC3: cpuJP(0, 0, 0); break;           // JP
    case 0xE9: REGS.w.PC = REGS.w.HL; break;    // JP (HL)

    // CALL
    case 0xC4: cpuCALL(1, FLAG_Z, 0); break;
    case 0xCC: cpuCALL(1, FLAG_Z, 1); break;
    case 0xD4: cpuCALL(1, FLAG_C, 0); break;
    case 0xDC: cpuCALL(1, FLAG_C, 1); break;
    case 0xCD: cpuCALL(0, 0, 0); break;

    // RET
    case 0xC0: cpuRET(FLAG_Z, 0); break;
    case 0xC8: cpuRET(FLAG_Z, 1); break;
    case 0xD0: cpuRET(FLAG_C, 0); break;
    case 0xD8: cpuRET(FLAG_C, 1); break;

    // RET
    case 0xC9: REGS.w.PC = popWordFromStack(); break;

    // RETI
    case 0xD9: REGS.w.PC = popWordFromStack(); gbState.IME = 1; break;

    // Interrupts
    case 0xF3: gbState.IME = 0; break;  // DI
    case 0xFB: gbState.IME = 1; break;  // EI

    // Restarts
    case 0xC7: cpuRST(0x00); break;
    case 0xCF: cpuRST(0x08); break;
    case 0xD7: cpuRST(0x10); break;
    case 0xDF: cpuRST(0x18); break;
    case 0xE7: cpuRST(0x20); break;
    case 0xEF: cpuRST(0x28); break;
    case 0xF7: cpuRST(0x30); break;
    case 0xFF: cpuRST(0x38); break;

	// HALT
    case 0x76: gbState.cpuHalted = 1; break;

	// STOP (non-Z80)
    case 0x10: gbState.cpuHalted = 2; break;

	// RLCA
	case 0x07: REGS.b.A = cpuRLC(REGS.b.A); break;

	// RRCA
	case 0x0F: REGS.b.A = cpuRRC(REGS.b.A); break;

	// RLA
	case 0x17: REGS.b.A = cpuRL(REGS.b.A); break;

	// RRA
    case 0x1F: REGS.b.A = cpuRR(REGS.b.A); break;

	// DAA
	case 0x27: cpuDAA(); break;

	// CPL
	case 0x2F: cpuCPL(); break;

	// SCF
    case 0x37: cpuSCF(); break;

	// CCF
	case 0x3F: cpuCCF(); break;

	default:
        printf("Opcode 0x%X at address 0x%X not supported yet!\n", opcode, REGS.w.PC - 1);
        exit_with_debug();
    }

#ifdef GAMEBOY_DEBUG
    writeLog("AF: 0x%04X BC: 0x%04X DE: 0x%04X HL: 0x%04X PC: 0x%04X SP: 0x%04X B: %X\n", REGS.w.AF, REGS.w.BC, REGS.w.DE, REGS.w.HL, REGS.w.PC, REGS.w.SP, gbState.currentRomBank);
#endif

    return cycles_executed;
}
