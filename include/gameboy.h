/******************************************************************************
DoGoBoy - Nintendo GameBoy Emulator - ©2009 Douglas Gore (doug@ssonic.co.uk)
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

Header file for defining important values/structures and sharing functions.
******************************************************************************/

#ifndef GAMEBOY_H
#define GAMEBOY_H

#include <SDL.h>

//#define DEBUG_OPCODE_COVERAGE

// Define base addresses for the memory map
#define ADDR_INT_ENABLE				0xFFFF
#define ADDR_MINI_INTERNAL_RAM		0xFF80
#define ADDR_RESERVED2				0xFF4C
#define ADDR_IO_PORTS				0xFF00
#define ADDR_RESERVED1				0xFEA0
#define ADDR_OAM_MEMORY				0xFE00
#define ADDR_INTERNAL_RAM_ECHO		0xE000
#define ADDR_INTERNAL_RAM			0xC000
#define ADDR_S_RAM_BANK				0xA000
#define ADDR_VIDEO_RAM				0x8000
#define ADDR_ROM_BANK_S				0x4000
#define ADDR_ROM_BANK_0				0x0000

#define GB_DISPLAY_WIDTH			160
#define GB_DISPLAY_HEIGHT			144

#define HBLANK_PERIOD 456

#define LCD_MODE0_PERIOD	204		// 48.6uS x 4.2 ticks per microsecond
#define LCD_MODE1_PERIOD	4560    // See VBLANK
#define LCD_MODE2_PERIOD	 80		// 19uS x 4.2
#define LCD_MODE3_PERIOD	172		// 41uS x 4.2

#define CYCLES_PER_FRAME 69905

#define DOGO_LITTLE_ENDIAN
#define bool _Bool

typedef unsigned char byte;
typedef signed char sbyte;
typedef unsigned short word;
typedef signed short sword;
typedef unsigned int dword;

union _REGS
{
#ifdef DOGO_LITTLE_ENDIAN
	struct
	{
		byte C;				// Counter
		byte B;				// GP register
		byte E;				// GP register
		byte D;				// GP register
		byte L;
		byte H;
		byte F;				// Status flags
		byte A;				// Accumulator
		byte PCh;
		byte PCl;
		byte SPh;
		byte SPl;
	} b;
#else
	struct
	{
		byte B;				// GP register
		byte C;				// Counter
		byte D;				// GP register
		byte E;				// GP register
		byte H;
		byte L;
		byte A;				// Accumulator
		byte F;				// Status flags
		byte PCl;
		byte PCh;
		byte SPl;
		byte SPh;
	} b;
#endif
	
	struct
	{
		word BC;
		word DE;
		word HL;
		word AF;
		word PC;			// Program counter
		word SP;			// Stack pointer
	} w;
} REGS;

typedef struct
{
	byte JOYPAD;
	byte SIODATA;
	byte SIOCONT;
	byte DIVIDER;
	byte TIMECNT;
	byte TIMEMOD;
	byte TIMECONT;
	byte IFLAGS;
	byte SNDREG10;
	byte SNDREG11;
	byte SNDREG12;
	byte SNDREG13;
	byte SNDREG14;
	byte SNDREG21;
	byte SNDREG22;
	byte SNDREG23;
	byte SNDREG24;
	byte SNDREG30;
	byte SNDREG31;
	byte SNDREG32;
	byte SNDREG33;
	byte SNDREG34;
	byte SNDREG41;
	byte SNDREG42;
	byte SNDREG43;
	byte SNDREG44;
	byte SNDREG50;
	byte SNDREG51;
	byte SNDREG52;
	byte LCDCONT;
	byte LCDSTAT;
	byte SCROLLX;
	byte SCROLLY;
	byte CURLINE;
	byte CMPLINE;
	byte BGRDPAL;
	byte OBJ0PAL;
	byte OBJ1PAL;
	byte WNDPOSY;
	byte WNDPOSX;
	byte KEY1;					// Prepare speed switch (CGB)
	byte DMACONT;
	byte ISWITCH;
} gbIOstruct;

typedef struct
{
	byte mask;
	byte value;
} logicTest;

typedef enum
{
    ROM_ONLY,
    MBC1,
    MBC2,
    MBC3,
    MBC4
} mbcType;

enum
{
    INT_VBLANK = 0x1,
    INT_LCDC   = 0x2,
    INT_TIMER  = 0x4,
    INT_SERIAL = 0x8,
    INT_HI_LO  = 0x10
} intType;

typedef struct
{
	byte IME;
    byte cpuHalted;
	int lcdMode;
	signed int lcdModePeriod;
	signed int timerPeriod;
	int divideRegister;
    byte bgPal[4];
    byte obj0Pal[4];
    byte obj1Pal[4];
    mbcType mbc;
    byte romBanks;
    byte currentRomBank;
    unsigned int ramSize;
    byte ramMode;
    byte currentRamBank;
    byte batteryBackup;
	byte keysState;
} gbStateStruct;

gbIOstruct gbIO;
gbStateStruct gbState;

word opRecord[256];
int opIndex;

// Functions exported from the processor
void initCPU(void);
unsigned int executeOpcode(void);
void pushWordToStack(word data);

// Functions exported from memory module
byte readByteFromMemory(unsigned int address);
void writeByteToMemory(unsigned int address, byte value);
void initGbMemory(void);
void freeGbMemory(void);
void loadRom(char* filename);

// Functions exported from graphics module
void updateGraphics(SDL_Surface* surface, Uint32 cycles);
void drawTilemap(SDL_Surface* surface);

byte getJoypadState(void);

void writeLog(char* log_message, ...);
void exit_with_debug(void);

int opcode_coverage[0xFF];
int cb_opcode_coverage[0xFF];

#endif  // GAMEBOY_H
