/******************************************************************************
DoGoBoy - Nintendo GameBoy Emulator - �2009 Douglas Gore (doug@ssonic.co.uk)
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

Memory related emulation functions
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "gameboy.h"

uint8_t WRAMbank0[0x1000];	    // 4KB WRAM bank 0
uint8_t WRAMbank1[0x1000];	    // 4KB WRAM bank 1
uint8_t VRAMbank[0x2000];      	// 8 KB VRAM bank
uint8_t HRAMbank[0x80];			// 128B HRAM
uint8_t OAMbank[0xA0];		    // 160B OAM
uint8_t WFbank[0x10];			// Waveform RAM

uint8_t* cartData = NULL;
uint8_t* switchRAM;
uint8_t* switchRAMPtr;

static void dmaTransfer(uint16_t address);

// Write a byte of data into Game Boy memory
void writeByteToMemory(unsigned int address, uint8_t value)
{
/*
--------------------------- FFFF  | 32kB ROMs are non-switchable and occupy
 I/O ports + internal RAM         | 0000-7FFF are. Bigger ROMs use one of two 
--------------------------- FF00  | different bank switches. The type of a
 Internal RAM                     | bank switch can be determined from the 
--------------------------- C000  | internal info area located at 0100-014F
 8kB switchable RAM bank          | in each cartridge.
--------------------------- A000  |
 16kB VRAM                        | MBC1 (Memory Bank Controller 1):
--------------------------- 8000  | Writing a value into 2000-3FFF area will
 16kB switchable ROM bank         | select an appropriate ROM bank at
--------------------------- 4000  | 4000-7FFF. Writing a value into 4000-5FFF
 16kB ROM bank #0                 | area will select an appropriate RAM bank 
--------------------------- 0000  | at A000-C000.
                                  |
                                  | MBC2 (Memory Bank Controller 2):
                                  | Writing a value into 2100-21FF area will
                                  | select an appropriate ROM bank at
                                  | 4000-7FFF. RAM switching is not provided.
*/

    // This is the interrupt enable register
	if (address == 0xFFFF)
	{
		//printf("Write to interrupt enable register, value: 0x%X\n", value);
		gbIO.ISWITCH = value;
	}
    // High RAM (HRAM) only RAM accessible during the LCD blanking period
	else if ((address >= 0xFF80) && (address < 0xFFFF))
	{
		//printf("Write to high RAM (HRAM), address: 0x%X, value: 0x%X\n", address, value);
		HRAMbank[address - 0xFF80] = value;
	}
    // IO registers
	else if ((address >= ADDR_IO_PORTS) && (address < 0xFF80))
	{
		//printf("Write to I/O port 0x%X value 0x%X\n", address, value);
		
		switch(address)
		{
		case 0xFF00:
			gbIO.JOYPAD = value;
		break;
		
		case 0xFF01:
			gbIO.SIODATA = value;
		break;
		
		case 0xFF02:
			gbIO.SIOCONT = value;
		break;
		
		// Any write to the this register resets it to zero
		case 0xFF04:
			gbIO.DIVIDER = 0;
		break;
		
		case 0xFF05:
			gbIO.TIMECNT = value;
		break;
		
		case 0xFF06:
			gbIO.TIMEMOD = value;
		break;

		case 0xFF07:
			gbIO.TIMECONT = value;
		break;

		case 0xFF0F:
			gbIO.IFLAGS = value;
		break;

		case 0xFF10:
			gbIO.SNDREG10 = value;
		break;

		case 0xFF11:
			gbIO.SNDREG11 = value;
		break;

		case 0xFF12:
			gbIO.SNDREG12 = value;
		break;

		case 0xFF13:
			gbIO.SNDREG13 = value;
		break;

		case 0xFF14:
			gbIO.SNDREG14 = value;
		break;

		case 0xFF16:
			gbIO.SNDREG21 = value;
		break;

		case 0xFF17:
			gbIO.SNDREG22 = value;
		break;

		case 0xFF18:
			gbIO.SNDREG23 = value;
		break;

		case 0xFF19:
			gbIO.SNDREG24 = value;
		break;

		case 0xFF1A:
			gbIO.SNDREG30 = value;
		break;

		case 0xFF1B:
			gbIO.SNDREG31 = value;
		break;

		case 0xFF1C:
			gbIO.SNDREG32 = value;
		break;

		case 0xFF1D:
			gbIO.SNDREG33 = value;
		break;

		case 0xFF1E:
			gbIO.SNDREG34 = value;
		break;

		case 0xFF20:
			gbIO.SNDREG41 = value;
		break;

		case 0xFF21:
			gbIO.SNDREG42 = value;
		break;

		case 0xFF22:
			gbIO.SNDREG43 = value;
		break;
		
		case 0xFF23:
			gbIO.SNDREG44 = value;
		break;
		
		case 0xFF24:
			gbIO.SNDREG50 = value;
		break;
		
		case 0xFF25:
			gbIO.SNDREG51 = value;
		break;
		
		case 0xFF26:
			gbIO.SNDREG52 = value;
		break;

		case 0xFF30:
		case 0xFF31:
		case 0xFF32:
		case 0xFF33:
		case 0xFF34:
		case 0xFF35:
		case 0xFF36:
		case 0xFF37:
		case 0xFF38:
		case 0xFF39:
		case 0xFF3A:
		case 0xFF3B:
		case 0xFF3C:
		case 0xFF3D:
		case 0xFF3E:
		case 0xFF3F:
			WFbank[address - 0xFF30] = value;
		break;

		case 0xFF40:
			gbIO.LCDCONT = value;
		break;
		
		case 0xFF41:
			gbIO.LCDSTAT = value;
		break;
		
		case 0xFF42:
			gbIO.SCROLLY = value;
		break;
		
		case 0xFF43:
			gbIO.SCROLLX = value;
		break;

        // Current scanline, writing to this register resets it
		case 0xFF44:
			gbIO.CURLINE = 0x00;
		break;

		case 0xFF45:
			gbIO.CMPLINE = value;
		break;

		case 0xFF46:
			dmaTransfer(value << 8);
		break;

		case 0xFF47:
			gbIO.BGRDPAL = value;
            gbState.bgPal[0] = gbIO.BGRDPAL & 0x3;
            gbState.bgPal[1] = (gbIO.BGRDPAL >> 2) & 0x3;
            gbState.bgPal[2] = (gbIO.BGRDPAL >> 4) & 0x3;
            gbState.bgPal[3] = (gbIO.BGRDPAL >> 6) & 0x3;
		break;

		case 0xFF48:
			gbIO.OBJ0PAL = value;
            gbState.obj0Pal[0] = gbIO.OBJ0PAL & 0x3;
            gbState.obj0Pal[1] = (gbIO.OBJ0PAL >> 2) & 0x3;
            gbState.obj0Pal[2] = (gbIO.OBJ0PAL >> 4) & 0x3;
            gbState.obj0Pal[3] = (gbIO.OBJ0PAL >> 6) & 0x3;
		break;

		case 0xFF49:
			gbIO.OBJ1PAL = value;
            gbState.obj1Pal[0] = gbIO.OBJ1PAL & 0x3;
            gbState.obj1Pal[1] = (gbIO.OBJ1PAL >> 2) & 0x3;
            gbState.obj1Pal[2] = (gbIO.OBJ1PAL >> 4) & 0x3;
            gbState.obj1Pal[3] = (gbIO.OBJ1PAL >> 6) & 0x3;
		break;

		case 0xFF4A:
			gbIO.WNDPOSY = value;
		break;

		case 0xFF4B:
			gbIO.WNDPOSX = value;
		break;
		
		case 0xFF4D:
			gbIO.KEY1 = value;
		break;

		default:
			printf("I/O port 0x%X not supported\n", address);
			//exit(0);
		}
	}
    // Unusable memory
	else if ((address >= ADDR_RESERVED1) && (address < ADDR_IO_PORTS))
	{
		//printf("Memory address (0x%X) not usable, abort\n", address);
		//exit(0);
	}
    // OAM bank, where sprite attributes are stored
	else if ((address >= ADDR_OAM_MEMORY) && (address < ADDR_RESERVED1))
	{
		//printf("Write to sprite attribute (OAM) address: 0x%X with value: 0x%X\n", address, value);
		OAMbank[address - ADDR_OAM_MEMORY] = value;
	}
    // Echo of WRAM bank 0 and 1
	else if ((address >= ADDR_INTERNAL_RAM_ECHO) && (address < ADDR_OAM_MEMORY))
	{
		//printf("Write to ECHO of 0xC000, address: 0x%X, value: 0x%X\n", address, value);
        if (address < 0xF000)
        {
		    WRAMbank0[address - ADDR_INTERNAL_RAM_ECHO] = value;
        }
        else
        {
            WRAMbank1[address - 0xF000] = value;
        }
    }
    // Work RAM bank 1 (switchable on CGB)
	else if ((address >= 0xD000) && (address < ADDR_INTERNAL_RAM_ECHO))
	{
		//printf("Write to work RAM bank 1, address: 0x%X, value: 0x%X\n", address, value);
		WRAMbank1[address - 0xD000] = value;
	}
    // Work RAM bank 0 (fixed)
	else if ((address >= ADDR_INTERNAL_RAM) && (address < 0xD000))
	{
		//printf("Write to work RAM bank 0, address: 0x%X, value: 0x%X\n", address, value);
		WRAMbank0[address - ADDR_INTERNAL_RAM] = value;
	}
    // External RAM (8KB), if present
	else if ((address >= ADDR_S_RAM_BANK) && (address < ADDR_INTERNAL_RAM))
	{
		//printf("Write to 8kB switchable RAM bank, address: 0x%X, value: 0x%X\n", address, value);

        // If we have no external RAM then don't try to access it
        if (gbState.ramMode == 0x00)
        {
            return;
        }
        // Protect against invalid RAM access in 2K mode
        else if ((gbState.ramMode == 0x01) && (address >= 0xA800))
        {
            return;
        }

        switchRAMPtr[address - ADDR_S_RAM_BANK] = value;
	}
    // Video RAM (switchable on CGB)
	else if ((address >= ADDR_VIDEO_RAM) && (address < ADDR_S_RAM_BANK))
	{
		VRAMbank[address - ADDR_VIDEO_RAM] = value;
	}
    // Switchable ROM bank
	else if ((address >= ADDR_ROM_BANK_S) && (address < ADDR_VIDEO_RAM))
	{
        switch(gbState.mbc)
        {
        case ROM_ONLY:
		    writeLog("Cannot write to 16kb switchable ROM bank\n");
		    //exit_with_debug();
            break;

        default:
            // MBC unhandled
            break;
        }
	}
    // Fixed ROM bank
	else if ((address >= ADDR_ROM_BANK_0) && (address < ADDR_ROM_BANK_S))
	{
        switch(gbState.mbc)
        {
        case ROM_ONLY:
    		writeLog("Cannot write to 16kb ROM bank #0, address: 0x%X, value: 0x%X\n", address, value);
            writeLog("Bank switching not supported in this MBC mode\n");
        break;

        case MBC1:
            if (address >= 0x2000)
            {
                if (0x00 == value)
                {
                    value = 1;
                }

                if (value < gbState.romBanks)
                {
                    gbState.currentRomBank = value;
                }
                else
                {
                    printf("Invalid ROM bank %i (of %i)\n", value, gbState.romBanks);
                    //exit_with_debug();
                }
            }
		break;

        default:
            // MBC unhandled
            break;
        }
	}
	else
	{
		printf("Invalid memory address\n");
		exit(0);
	}
}

uint8_t readByteFromMemory(uint16_t address)
{
/*
--------------------------- FFFF  | 32kB ROMs are non-switchable and occupy
 I/O ports + internal RAM         | 0000-7FFF are. Bigger ROMs use one of two 
--------------------------- FF00  | different bank switches. The type of a
 Internal RAM                     | bank switch can be determined from the 
--------------------------- C000  | internal info area located at 0100-014F
 8kB switchable RAM bank          | in each cartridge.
--------------------------- A000  |
 16kB VRAM                        | MBC1 (Memory Bank Controller 1):
--------------------------- 8000  | Writing a value into 2000-3FFF area will
 16kB switchable ROM bank         | select an appropriate ROM bank at
--------------------------- 4000  | 4000-7FFF. Writing a value into 4000-5FFF
 16kB ROM bank #0                 | area will select an appropriate RAM bank 
--------------------------- 0000  | at A000-C000.
                                  |
                                  | MBC2 (Memory Bank Controller 2):
                                  | Writing a value into 2100-21FF area will
                                  | select an appropriate ROM bank at
                                  | 4000-7FFF. RAM switching is not provided.
*/

	if (address == 0xFFFF)
	{
		return gbIO.ISWITCH;
	}
	else if ((address >= 0xFF80) && (address < 0xFFFF))
	{
		//printf("Read from HRAM, address: 0x%X, value: 0x%X\n", address, HRAMbank[address - 0xFF80]);
		return HRAMbank[address - 0xFF80];
	}
	else if ((address >= ADDR_IO_PORTS) && (address < 0xFF80))
	{
        //printf("Read from I/O port, address: 0x%X\n", address);

		switch(address)
		{
		case 0xFF00:
			return getJoypadState();
		break;
		
		case 0xFF01:
			return gbIO.SIODATA;
		break;
		
		case 0xFF02:
			return gbIO.SIOCONT;
		break;
		
		case 0xFF04:
			return gbIO.DIVIDER;
		break;
		
		case 0xFF05:
			return gbIO.TIMECNT;
		break;
		
		case 0xFF06:
			return gbIO.TIMEMOD;
		break;

		case 0xFF07:
			return gbIO.TIMECONT;
		break;

		case 0xFF0F:
			return gbIO.IFLAGS;
		break;

		case 0xFF10:
			return gbIO.SNDREG10;
		break;

		case 0xFF11:
			return gbIO.SNDREG11;
		break;

		case 0xFF12:
			return gbIO.SNDREG12;
		break;

		case 0xFF13:
			return gbIO.SNDREG13;
		break;

		case 0xFF14:
			return gbIO.SNDREG14;
		break;

		case 0xFF16:
			return gbIO.SNDREG21;
		break;

		case 0xFF17:
			return gbIO.SNDREG22;
		break;

		case 0xFF18:
			return gbIO.SNDREG23;
		break;

		case 0xFF19:
			return gbIO.SNDREG24;
		break;

		case 0xFF1A:
			return gbIO.SNDREG30;
		break;

		case 0xFF1B:
			return gbIO.SNDREG31;
		break;

		case 0xFF1C:
			return gbIO.SNDREG32;
		break;

		case 0xFF1D:
			return gbIO.SNDREG33;
		break;

		case 0xFF1E:
			return gbIO.SNDREG34;
		break;

		case 0xFF20:
			return gbIO.SNDREG41;
		break;

		case 0xFF21:
			return gbIO.SNDREG42;
		break;

		case 0xFF22:
			return gbIO.SNDREG43;
		break;
		
		case 0xFF23:
			return gbIO.SNDREG44;
		break;
		
		case 0xFF24:
			return gbIO.SNDREG50;
		break;
		
		case 0xFF25:
			return gbIO.SNDREG51;
		break;
		
		case 0xFF26:
			return gbIO.SNDREG52;
		break;
		
		case 0xFF40:
			return gbIO.LCDCONT;
		break;
		
		case 0xFF41:
			return gbIO.LCDSTAT;
		break;
		
		case 0xFF42:
			return gbIO.SCROLLY;
		break;
		
		case 0xFF43:
			return gbIO.SCROLLX;
		break;

		case 0xFF44:
			return gbIO.CURLINE;
		break;

		case 0xFF45:
			return gbIO.CMPLINE;
		break;

		case 0xFF46:
			return gbIO.DMACONT;
		break;

		case 0xFF47:
			return gbIO.BGRDPAL;
		break;

		case 0xFF48:
			return gbIO.OBJ0PAL;
		break;

		case 0xFF49:
			return gbIO.OBJ1PAL;
		break;

		case 0xFF4A:
			return gbIO.WNDPOSY;
		break;

		case 0xFF4B:
			return gbIO.WNDPOSX;
		break;
		
		case 0xFF4D:
			return gbIO.KEY1;
		break;

		default:
			printf("I/O port not supported\n");
			return 0;
		}
	}
	else if ((address >= ADDR_RESERVED1) && (address < ADDR_IO_PORTS))
	{
		//printf("Memory address (0x%X) not usable, abort\n", address);
		//exit(0);
		return 0;
	}
	else if ((address >= ADDR_OAM_MEMORY) && (address < ADDR_RESERVED1))
	{
		//printf("Read from Spirte Attribute Table, address: 0x%X, value: 0x%X\n", address, OAMbank[address - ADDR_OAM_MEMORY]);
		return OAMbank[address - ADDR_OAM_MEMORY];
	}
	else if ((address >= ADDR_INTERNAL_RAM_ECHO) && (address < ADDR_OAM_MEMORY))
	{
		//printf("Read from echo WRAM0, address: 0x%X, value: 0x%X\n", address, WRAMbank0[address - ADDR_INTERNAL_RAM_ECHO]);
		return WRAMbank0[address - ADDR_INTERNAL_RAM_ECHO];
	}
	else if ((address >= 0xD000) && (address < ADDR_INTERNAL_RAM_ECHO))
	{
		//printf("Read from WRAM bank 1, address: 0x%X, value: 0x%X\n", address, WRAMbank1[address - 0xD000]);
		return WRAMbank1[address - 0xD000];
	}
	else if ((address >= ADDR_INTERNAL_RAM) && (address < 0xD000))
	{
		//writeLog("Read from WRAM bank 0, address: 0x%X, value: 0x%X\n", address, WRAMbank0[address - ADDR_INTERNAL_RAM]);
		return WRAMbank0[address - ADDR_INTERNAL_RAM];
	}
	else if ((address >= ADDR_S_RAM_BANK) && (address < ADDR_INTERNAL_RAM))
	{
		//printf("Read from 8kB switchable RAM bank\n");

        // If we have no external RAM then don't try to access it
        if (gbState.ramMode == 0x00)
        {
            return 0x00;
        }
        // Protect against invalid RAM access in 2K mode
        else if ((gbState.ramMode == 0x01) && (address >= 0xA800))
        {
            return 0x00;
        }

		return switchRAMPtr[address - ADDR_S_RAM_BANK];
	}
	else if ((address >= ADDR_VIDEO_RAM) && (address < ADDR_S_RAM_BANK))
	{
		//if (address >= 0x9800)
			//printf("Read from VRAM, address: 0x%X, value 0x%X\n", address, VRAMbank[address - ADDR_VIDEO_RAM]);
		return VRAMbank[address - ADDR_VIDEO_RAM];
	}
	else if ((address >= ADDR_ROM_BANK_S) && (address < ADDR_VIDEO_RAM))
	{
        //printf("Read from 16kb switchable ROM bank, address: 0x%X (0x%X)\n", address, (address - ADDR_ROM_BANK_S) + (ADDR_ROM_BANK_S * gbState.currentRomBank));
        return cartData[(address - ADDR_ROM_BANK_S) + (ADDR_ROM_BANK_S * gbState.currentRomBank)];
	}
	else if ((address >= ADDR_ROM_BANK_0) && (address < ADDR_ROM_BANK_S))
	{
		//printf("Read from 16kb ROM bank #0\n");
		return cartData[address];
	}
	else
	{
		printf("Reading from invalid memory address 0x%X\n", address);
		exit(0);
	}
}

// Transfer some data straight into Object Attribute Memory (OAM)
void dmaTransfer(uint16_t address)
{
	int ii;

	for (ii = 0; ii < 0xA0; ii++)
	{
		writeByteToMemory(ADDR_OAM_MEMORY + ii, readByteFromMemory(address + ii));
	}
}

// Load ROM into our system RAM and parse ROM data for system configuration
void loadRom(char* filename)
{
	FILE *fp;
	unsigned int fileLength;

    cartData = NULL;
    switchRAM = NULL;

	fp = fopen(filename, "rb");

	if (fp == NULL)
	{
		printf("Failed to open '%s'\n", filename);
		exit(1);
	}
	
	// obtain file size:
	fseek (fp, 0, SEEK_END);
	fileLength = ftell(fp);
	rewind(fp);

    cartData = malloc(fileLength);

    printf("Cart data located at 0x%X\n", cartData);

	fread(cartData, 1, fileLength, fp);
	fclose(fp);

    printf("Cart length: 0x%X\n", fileLength);
    printf("Cart type: 0x%X\n", cartData[0x147]);

    switch(cartData[0x147])
    {
    case 0x00:
        gbState.mbc = ROM_ONLY;
        break;

    case 0x01:
        gbState.mbc = MBC1;
        break;

    case 0x03:
        gbState.mbc = MBC1;
        gbState.batteryBackup = 1;
        break;

    default:
        printf("ROM type not supported yet!\n");
        exit(0);
    }

    printf("ROM size: %d\n", cartData[0x148]);
    printf("RAM size: %d\n", cartData[0x149]);

    switch(cartData[0x148])
    {
    case 0x00:
        gbState.romBanks = 0;
        break;

    case 0x01:
        gbState.romBanks = 4;
        break;

    case 0x02:
        gbState.romBanks = 8;
        break;

    case 0x03:
        gbState.romBanks = 16;
        break;

    case 0x04:
        gbState.romBanks = 32;
        break;

    default:
        printf("ROM size %i not supported yet!\n", cartData[0x148]);
        exit_with_debug();
    }
    
    switch(cartData[0x149])
    {
    case 0x00:
        gbState.ramMode = 0;
        break;

    case 0x01:
        gbState.ramMode = 1;
        gbState.ramSize = 0x800;        // 2KB
        break;

    case 0x02:
        gbState.ramMode = 2;
        gbState.ramSize = 0x2000;       // 8KB
        break;

    case 0x03:
        gbState.ramMode = 3;
        gbState.ramSize = 0x8000;       // 32KB
        break;

    default:
        printf("ERROR: RAM banks not supported\n");
		exit(0);
    }

    switchRAM = malloc(gbState.ramSize);
    switchRAMPtr = switchRAM;
}

void initGbMemory(void)
{
	memset(WRAMbank0, 0, sizeof(WRAMbank0));
	memset(WRAMbank1, 0, sizeof(WRAMbank1));
}

void freeGbMemory(void)
{
	if (cartData)
	{
		free(cartData);
		cartData = NULL;
	}

    if (switchRAM)
    {
        free(switchRAM);
        switchRAM = NULL;
    }
}
