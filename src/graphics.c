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

Graphics related emulation functions
******************************************************************************/

#include "SDL.h"

#include "gameboy.h"

#define LCDC_LCD_ON						(1 << 7)
#define LCDC_WINDOW_UPPER_TILE_SET		(1 << 6)
#define LCDC_WINDOW_ON					(1 << 5)
#define LCDC_LOWER_TILE_DATA			(1 << 4)
#define LCDC_UPPER_TILE_MAP				(1 << 3)
#define LCDC_8_X_16_SPRITES				(1 << 2)
#define LCDC_SPRITES_ON					(1 << 1)
#define LCDC_BG_WINDOW_ON				(1 << 0)


// Caclulate which shade of grey we need
dword getColour(byte colourIndex, byte palette)
{
    byte colour;
    dword rgb = 0;

    colour = (palette >> (colourIndex * 2)) & 0x03;

    switch(colour)
    {
    // White
    case 0: rgb = 0x00FFFFFF; break;
    case 1: rgb = 0x00CCCCCC; break;
    case 2: rgb = 0x00777777; break;
    case 3: rgb = 0x00000000; break;
    }

    return rgb;
}

void setLcdStatus(void)
{
    byte requestInterrupt = 0;
    byte currentMode;

    // If LCD is not enabled
    if ((gbIO.LCDCONT & LCDC_LCD_ON) == 0)
    {
        gbState.lcdModePeriod = HBLANK_PERIOD;
        gbIO.CURLINE = 0;

        // Clear lower two bits then set V-Blank mode
        gbIO.LCDSTAT &= 0xFC;
        gbIO.LCDSTAT |= 0x01;
    }
    else
    {
        currentMode = gbIO.LCDSTAT & 0x03;

        // We're in V-blank mode
        if (gbIO.CURLINE >= GB_DISPLAY_HEIGHT)
        {
            gbState.lcdMode = 1;

            // Clear lower two bits then set V-Blank mode
            gbIO.LCDSTAT &= 0xFC;
            gbIO.LCDSTAT |= 0x01;

            // Trigger an interrupt if enabled
            if (gbIO.LCDSTAT & 0x10)
            {
                requestInterrupt = 1;
            }
        }
        else
        {
            // Mode 2
            if (gbState.lcdModePeriod >= (HBLANK_PERIOD - 80))
            {
                gbState.lcdMode = 2;

                // Clear lower two bits then set V-Blank mode
                gbIO.LCDSTAT &= 0xFC;
                gbIO.LCDSTAT |= 0x02;

                // Trigger an interrupt if enabled
                if (gbIO.LCDSTAT & 0x20)
                {
                    requestInterrupt = 1;
                }
            }
            // Mode 3
            if (gbState.lcdModePeriod >= (HBLANK_PERIOD - 80 - 172))
            {
                gbState.lcdMode = 3;

                // Clear lower two bits then set V-Blank mode
                gbIO.LCDSTAT &= 0xFC;
                gbIO.LCDSTAT |= 0x03;
            }
            // Mode 0
            else
            {
                gbState.lcdMode = 0;

                // Clear lower two bits then set V-Blank mode
                gbIO.LCDSTAT &= 0xFC;

                // Trigger an interrupt if enabled
                if (gbIO.LCDSTAT & 0x08)
                {
                    requestInterrupt = 1;
                }
            }
        }

        if (requestInterrupt && (gbState.lcdMode != currentMode))
        {
            gbIO.IFLAGS |= 0x02;
        }

        // Set the coincidence flag if lines match
        if (gbIO.CURLINE == gbIO.CMPLINE)
        {
            // If coincidence interrupt enabled then trigger LCDSTAT interrupt
            if (gbIO.LCDSTAT & 0x40)
            {
                // Request interrupt
                gbIO.IFLAGS |= 0x2;
            }

            // Set coincidence flag
            gbIO.LCDSTAT |= 0x4;
        }
        // Clear the flag otherwise
        else
        {
            gbIO.LCDSTAT &= 0xFB;
        }
    }
}

// Draw a scanline of the tile layer to the output bitmap
void drawTiles(SDL_Surface* surface, byte scanline)
{
    const dword tileSizeInBytes = 16;

    byte xx;
	int tx;
    byte colourIndex;
    byte b1, b2;

    dword tileDataTable;
    dword tileMapTable;
    dword tileRow;
	dword tileCol;
    signed int tileNumber;
	dword tileLocation;

	byte xPos, yPos, line;

    dword* video_plane;

    byte unsignedNum = 1;
	byte usingWindow = 0;

	byte windowX = gbIO.WNDPOSX - 7;
	byte windowY = gbIO.WNDPOSY;

    video_plane = &((Uint32*)surface->pixels)[scanline * surface->w];

	// Is the window enabled
    if (gbIO.LCDCONT & LCDC_WINDOW_ON)
    {
		if (windowY <= scanline)
		{
			usingWindow = 1;
		}
    }

    // Check bit 4 for the tile data selection
    if (gbIO.LCDCONT & LCDC_LOWER_TILE_DATA)
    {
        tileDataTable = ADDR_VIDEO_RAM;
    }
    else
    {
        tileDataTable = 0x8800;
        unsignedNum = 0;
    }

	if (0 == usingWindow)
	{
		// Which backgroud memory are we using (bit 3)
		if (gbIO.LCDCONT & LCDC_UPPER_TILE_MAP)
		{
			tileMapTable = 0x9C00;
		}
		else
		{
			tileMapTable = 0x9800;
		}
	}
	else
	{
		// Which window memory are we using (bit 6)
		if (gbIO.LCDCONT & LCDC_WINDOW_UPPER_TILE_SET)
		{
			tileMapTable = 0x9C00;
		}
		else
		{
			tileMapTable = 0x9800;
		}
	}

	if (0 == usingWindow)
	{
		yPos = gbIO.SCROLLY + scanline;
	}
	else
	{
		yPos = scanline - windowY;
	}

	tileRow = (byte)(yPos / 8) * 32;

	for (xx = 0; xx < GB_DISPLAY_WIDTH; xx++)
    {
        xPos = xx + gbIO.SCROLLX;

        if (1 == usingWindow)
        {
            if (xx > windowX)
            {
                xPos = xx - windowX;
            }
        }

		tileCol = xPos / 8;

		tileLocation = tileDataTable;

		if (1 == unsignedNum)
        {
			tileNumber = readByteFromMemory(tileMapTable + tileRow + tileCol);
			tileLocation += tileNumber * tileSizeInBytes;
        }
        else
        {
            tileNumber = (sbyte)readByteFromMemory(tileMapTable + tileRow + tileCol);
			tileLocation += (tileNumber + 128) * tileSizeInBytes;
        }

		line = (yPos % 8) * 2;

        b1 = readByteFromMemory(tileLocation + line);
        b2 = readByteFromMemory(tileLocation + line + 1);

        //printf("Tile col %i, row: %i, number: %i\n", tileCol, tileRow, tileNumber);
        //printf("Reading from loc 0x%X, offset: 0x%X\n", tileLocation, line);

		tx = xPos % 8;

        colourIndex = (((b2 >> (7 - tx)) & 0x1) << 1) | ((b1 >> (7 - tx)) & 0x1);

        video_plane[xx] = getColour(colourIndex, gbIO.BGRDPAL);
    }
}

// Draw a scanline of the sprites layer to the output bitmap
void drawSprites(SDL_Surface* surface, byte scanline)
{
	int use8x16 = 0;
	byte sprite;
	
	byte index;
	byte yPos;
	byte xPos;
	byte tileLocation;
	byte attributes;

	int yFlip;
	int xFlip;

	int ysize;
	int line;

	word dataAddress;
	byte data1;
	byte data2;

	int tilePixel;
	int colourbit;

	int pixel;
	int xPix;

	byte colourNum;
	dword colourValue;

	word colourAddress;

    dword* video_plane;

	video_plane = &((unsigned int*)surface->pixels)[scanline * surface->w];

    // If sprites aren't enabled then get out of here
    if (0x00 == (gbIO.LCDCONT & LCDC_SPRITES_ON))
    {
        return;
    }

    // Determine sprite size, 8x8 or 8x16
	if (gbIO.LCDCONT & LCDC_8_X_16_SPRITES)
	{
		use8x16 = 1;
	}

	for (sprite = 0; sprite < 40; sprite++)
	{
		// sprite occupies 4 bytes in the sprite attributes table
		index        = sprite * 4;
		yPos		 = readByteFromMemory(ADDR_OAM_MEMORY + index) - 16;
		xPos		 = readByteFromMemory(ADDR_OAM_MEMORY + index + 1) - 8;
		tileLocation = readByteFromMemory(ADDR_OAM_MEMORY + index + 2) ;
		attributes	 = readByteFromMemory(ADDR_OAM_MEMORY + index + 3) ;

		yFlip = attributes & (1 << 6);
		xFlip = attributes & (1 << 5);

        // Check to see if the sprite is out of bounds and should be hidden
        if ((0 == yPos) || (yPos >= GB_DISPLAY_WIDTH))
        {
            continue;
        }
        else if ((0 == xPos) || (xPos >= 168))
        {
            continue;
        };

		ysize = 8;

		if (use8x16)
		{
			ysize = 16;
		}

		//printf("Iterate over sprites %d in %d?\n", yPos, scanline);

		// Does this sprite intersect with the scanline?
		if ((scanline >= yPos) && (scanline < (yPos + ysize)))
		{
			line = scanline - yPos;

			// read the sprite in backwards in the y axis
			if (yFlip)
			{
				line -= ysize;
				line *= -1;
			}

			line *= 2; // same as for tiles
			dataAddress = (ADDR_VIDEO_RAM + (tileLocation * 16)) + (word)line;
			data1 = readByteFromMemory(dataAddress);
			data2 = readByteFromMemory(dataAddress + 1);

			// Its easier to read in from right to left as pixel 0 is
			// bit 7 in the colour data, pixel 1 is bit 6 etc...
			for (tilePixel = 7; tilePixel >= 0; tilePixel--)
			{
				colourbit = tilePixel;

				// read the sprite in backwards for the x axis
				if (xFlip)
				{
					colourbit -= 7;
					colourbit *= -1;
				}

				// the rest is the same as for tiles
				colourNum = (data2 >> colourbit) & 0x01;
				colourNum <<= 1;
				colourNum |= (data1 >> colourbit) & 0x01;

                if (attributes & (1 << 4))
				{
					colourAddress = 0xFF49;
				}
				else
				{
					colourAddress = 0xFF48;
				}

                // Skip transparent colours
                if (0 == colourNum)
                {
                    continue;
                }

                switch((readByteFromMemory(colourAddress) >> (colourNum * 2)) & 0x03)
                {
                case 1: colourValue = 0x00CCCCCC; break;
                case 2: colourValue = 0x00777777; break;
                case 3: colourValue = 0x00000000; break;
                case 0:
                default: colourValue = 0x00FFFFFF; break;
                }

                //colourValue = getColour(colourNum, readByteFromMemory(colourAddress));

				xPix = 0 - tilePixel;
				xPix += 7;

				pixel = xPos + xPix;

                // If the background priority is greater than sprite, we need an extra check
                if (attributes & (1 << 7))
                {
                    if (video_plane[pixel] != 0x00FFFFFF)
                    {
                        continue;
                    }
                }

                if (pixel < GB_DISPLAY_WIDTH)
				{
                    video_plane[pixel] = colourValue;
				}

			}
		}
	}
}
void drawTilemap(SDL_Surface* surface)
{
    int xx, yy, tx, ty;
    unsigned int colour;
    byte b1, b2;

    unsigned int tile_table;
    unsigned int bg_offset;

    unsigned int rmask, gmask, bmask, amask;

    unsigned int* video_plane;

    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;

    SDL_LockSurface(surface);

    video_plane = surface->pixels;

    tile_table = ADDR_VIDEO_RAM;
    bg_offset = 0;

    for (yy = 0; yy < 12; yy++)
    {
        for (xx = 0; xx < 32; xx++)
        {
            for (ty = 0; ty < 8; ty++)
            {
                b1 = readByteFromMemory(tile_table++);
                b2 = readByteFromMemory(tile_table++);

                /*
                if (b1 || b2)
                {
                    printf("b1: 0x%X, b2: 0x%X\n", b1, b2);
                    exit(0);
                }
                */

                for (tx = 0; tx < 8; tx++)
                {
                    colour = (((b2 >> (7 - tx)) & 0x1) << 1) | ((b1 >> (7 - tx)) & 0x1);
                    //colour *= 64;

                    video_plane[(((yy * 8) + ty) * 256) + (xx * 8) + tx] = getColour(colour, gbIO.BGRDPAL); //(colour << 16) | (colour << 8) | (colour);
                }
            }
        }
    }

    SDL_UnlockSurface(surface);
}

void updateGraphics(SDL_Surface* surface, Uint32 cycles)
{
    setLcdStatus();

    // If LCD is enabled
    if (gbIO.LCDCONT & LCDC_LCD_ON)
    {
        gbState.lcdModePeriod -= cycles;
     
		// Have we reached the horizontal blanking period?
        if (gbState.lcdModePeriod <= 0)
        {
			// Move to the next scanline
            gbIO.CURLINE++;

			// Reset the countdown
            gbState.lcdModePeriod += HBLANK_PERIOD;

            // Have we reached the V-blank area, if so trigger an interrupt
            if (GB_DISPLAY_HEIGHT == gbIO.CURLINE)
            {
                gbIO.IFLAGS |= 0x01;
            }
            // If gone past scanline 153 reset to 0
            else if (gbIO.CURLINE > 153)
            {
                gbIO.CURLINE = 0;
            }
            // Draw the current scanline
            else if (gbIO.CURLINE < GB_DISPLAY_HEIGHT)
            {
                // Draw tiles then sprites on top
                if (gbIO.LCDCONT & LCDC_BG_WINDOW_ON)
                {
					SDL_LockSurface(surface);
				    drawTiles(surface, gbIO.CURLINE);
					drawSprites(surface, gbIO.CURLINE);
					SDL_UnlockSurface(surface);
                }
            }
        }
    }

    //printf("Line: %i, enable: %i\n", gbIO.CURLINE, gbIO.LCDCONT & 0x80);
}
