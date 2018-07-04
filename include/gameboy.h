/******************************************************************************
DoGoBoy - Nintendo GameBoy Emulator
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

union _REGS
{
#ifdef DOGO_LITTLE_ENDIAN
	struct
	{
		uint8_t C;				// Counter
		uint8_t B;				// GP register
		uint8_t E;				// GP register
		uint8_t D;				// GP register
		uint8_t L;
		uint8_t H;
		uint8_t F;				// Status flags
		uint8_t A;				// Accumulator
		uint8_t PCh;
		uint8_t PCl;
		uint8_t SPh;
		uint8_t SPl;
	} b;
#else
	struct
	{
		uint8_t B;				// GP register
		uint8_t C;				// Counter
		uint8_t D;				// GP register
		uint8_t E;				// GP register
		uint8_t H;
		uint8_t L;
		uint8_t A;				// Accumulator
		uint8_t F;				// Status flags
		uint8_t PCl;
		uint8_t PCh;
		uint8_t SPl;
		uint8_t SPh;
	} b;
#endif
	
	struct
	{
		uint16_t BC;
		uint16_t DE;
		uint16_t HL;
		uint16_t AF;
		uint16_t PC;			// Program counter
		uint16_t SP;			// Stack pointer
	} w;
} REGS;

typedef struct
{
	uint8_t JOYPAD;
	uint8_t SIODATA;
	uint8_t SIOCONT;
	uint8_t DIVIDER;
	uint8_t TIMECNT;
	uint8_t TIMEMOD;
	uint8_t TIMECONT;
	uint8_t IFLAGS;
	uint8_t SNDREG10;
	uint8_t SNDREG11;
	uint8_t SNDREG12;
	uint8_t SNDREG13;
	uint8_t SNDREG14;
	uint8_t SNDREG21;
	uint8_t SNDREG22;
	uint8_t SNDREG23;
	uint8_t SNDREG24;
	uint8_t SNDREG30;
	uint8_t SNDREG31;
	uint8_t SNDREG32;
	uint8_t SNDREG33;
	uint8_t SNDREG34;
	uint8_t SNDREG41;
	uint8_t SNDREG42;
	uint8_t SNDREG43;
	uint8_t SNDREG44;
	uint8_t SNDREG50;
	uint8_t SNDREG51;
	uint8_t SNDREG52;
	uint8_t LCDCONT;
	uint8_t LCDSTAT;
	uint8_t SCROLLX;
	uint8_t SCROLLY;
	uint8_t CURLINE;
	uint8_t CMPLINE;
	uint8_t BGRDPAL;
	uint8_t OBJ0PAL;
	uint8_t OBJ1PAL;
	uint8_t WNDPOSY;
	uint8_t WNDPOSX;
	uint8_t KEY1;					// Prepare speed switch (CGB)
	uint8_t DMACONT;
	uint8_t ISWITCH;
} gbIOstruct;

typedef struct
{
	uint8_t mask;
	uint8_t value;
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
	uint8_t IME;
    uint8_t cpuHalted;
	int lcdMode;
	signed int lcdModePeriod;
	signed int timerPeriod;
	int divideRegister;
    uint8_t bgPal[4];
    uint8_t obj0Pal[4];
    uint8_t obj1Pal[4];
    mbcType mbc;
    uint8_t romBanks;
    uint8_t currentRomBank;
    unsigned int ramSize;
    uint8_t ramMode;
    uint8_t currentRamBank;
    uint8_t batteryBackup;
	uint8_t keysState;
} gbStateStruct;

gbIOstruct gbIO;
gbStateStruct gbState;

uint16_t opRecord[256];
int opIndex;

typedef void (*drawCallback)(void);

// Functions exported from the processor
void initCPU(void);
unsigned int executeOpcode(void);
void pushWordToStack(uint16_t data);

// Functions exported from memory module
uint8_t readByteFromMemory(uint16_t address);
uint16_t readWordFromMemory(uint16_t address);
void writeByteToMemory(unsigned int address, uint8_t value);
void initGbMemory(void);
void freeGbMemory(void);
void loadRom(char* filename);

// Functions exported from graphics module
void updateGraphics(SDL_Surface* surface, Uint32 cycles);
void drawTilemap(SDL_Surface* surface);
void setDrawFrameFunction(drawCallback func);

uint8_t getJoypadState(void);

void writeLog(char* log_message, ...);
void exit_with_debug(void);

int opcode_coverage[0xFF];
int cb_opcode_coverage[0xFF];

#endif  // GAMEBOY_H
