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

Main program. The main loop exists here along with all the user input handling,
graphics handling, program maintainence, etc.
******************************************************************************/

#include <stdio.h>

#ifdef __WIN32__
#include <Windows.h>
#endif

#include "SDL.h"

#include "gameboy.h"

// Map standard functions to secure functions to stop Visual C++ complaining
#ifdef _MSC_VER
#define snprintf sprintf_s
#define vsnprintf vsprintf_s
#endif 

char msg_log[100][80];
unsigned int msg_offset = 0;

unsigned int frames;
Uint32 last_time;

char window_title[80];

SDL_Surface *screen = NULL;
SDL_Surface *gbSurface = NULL;
SDL_Joystick *joystick = NULL;

static int showTilemap = FALSE;
static int scaleFactor = 2;

// Exit and print out the last 100 debug actions
void exit_with_debug(void)
{
    int ii;

    printf("\nDoGoBoy terminating, last 100 operations...\n");
    
    for (ii = 0; ii < 100; ii++)
    {
        printf("%s", msg_log[(msg_offset + ii) % 100]);
    }

#ifdef DEBUG_OPCODE_COVERAGE
    for (ii = 0; ii < 0x100; ii++)
    {
        printf("Opcode 0x%02X: %8i calls | CB opcode 0x%02X: %8i calls\n", ii, opcode_coverage[ii], ii, cb_opcode_coverage[ii]);
    }
#endif

    //printf("\nSP: 0x%X (value 0x%X)\n", REGS.w.SP, read_word_from_memory(REGS.w.SP));

	freeGbMemory();

    exit(0);
}

// Write a line of debug printf style into an internal circular buffer
void writeLog(char* log_message, ...)
{
    va_list args;

    va_start(args, log_message);

    vsnprintf(msg_log[msg_offset++], 80, log_message, args);

    va_end(args);

    if (msg_offset >= 100) msg_offset = 0;
}

// Blit the output bitmap to the screen and flip the buffer if double buffered
void drawFrame(void)
{
	SDL_Rect dstRect;
	
	dstRect.x = 0;
	dstRect.y = 0;
	dstRect.w = GB_DISPLAY_WIDTH * scaleFactor;
	dstRect.h = GB_DISPLAY_HEIGHT * scaleFactor;

	if (SDL_SoftStretch(gbSurface, NULL, screen, &dstRect) != 0)
	{
		printf("ERROR: Cannot blit surface\n");
		exit(0);
	}
	
    if (SDL_Flip(screen) != 0)
	{
		printf("ERROR: Could not flip framebuffer\n");
	}
    
    frames++;
}

void doInterrupts(void)
{
    // If interrupts are enabled then run our interrupt handler
    if (1 == gbState.IME)
    {
        // Quick check to see if there are an interrupts to service
        if (gbIO.IFLAGS > 0)
        {
			if ((0x2 == gbState.cpuHalted) && ((gbIO.ISWITCH & 0x10) == 0))
			{
				return;
			}

            // Check the V-Blank interrupt
            if ((gbIO.IFLAGS & INT_VBLANK) && (gbIO.ISWITCH & 0x01))
            {
	            //printf(">> V-Blank interrupt <<\n");
    			
	            gbState.IME = 0;				// Disable interrupts
	            gbIO.IFLAGS &= ~INT_VBLANK;		// Clear bit 0 to show we're servicing request
	            pushWordToStack(REGS.w.PC); 	// Put PC on the stack
	            REGS.w.PC = 0x40;	            // Jump to video interrupt code

                gbState.cpuHalted = 0;
            }
            else if ((gbIO.IFLAGS & INT_LCDC) && (gbIO.ISWITCH & 0x02))
            {
	            gbState.IME = 0;				// Disable interrupts
	            gbIO.IFLAGS &= ~INT_LCDC;		// Clear bit 1 to show we're servicing request
	            pushWordToStack(REGS.w.PC); 	// Put PC on the stack
	            REGS.w.PC = 0x48;	            // Jump to LCD status interrupt code

                gbState.cpuHalted = 0;
            }
            else if ((gbIO.IFLAGS & INT_TIMER) && (gbIO.ISWITCH & 0x04))
            {
	            gbState.IME = 0;				// Disable interrupts
	            gbIO.IFLAGS &= ~INT_TIMER;		// Clear bit 2 to show we're servicing request
	            pushWordToStack(REGS.w.PC); 	// Put PC on the stack
	            REGS.w.PC = 0x50;	            // Jump to timer interrupt code

                gbState.cpuHalted = 0;
            }
            else if ((gbIO.IFLAGS & INT_SERIAL) && (gbIO.ISWITCH & 0x08))
            {
	            gbState.IME = 0;				// Disable interrupts
	            gbIO.IFLAGS &= ~INT_SERIAL;		// Clear bit 3 to show we're servicing request
	            pushWordToStack(REGS.w.PC); 	// Put PC on the stack
	            REGS.w.PC = 0x58;	            // Jump to serial interrupt code

                gbState.cpuHalted = 0;
            }
            else if ((gbIO.IFLAGS & INT_HI_LO) && (gbIO.ISWITCH & 0x10))
            {
	            printf(">> Joystick interrupt <<\n");
    			
	            gbState.IME = 0;				// Disable interrupts
	            gbIO.IFLAGS &= ~INT_HI_LO;		// Clear bit 4 to show we're servicing request
	            pushWordToStack(REGS.w.PC);	    // Put PC on the stack
	            REGS.w.PC = 0x60;	            // Jump to joypad interrupt code

                gbState.cpuHalted = 0;
            }
        }
    }
}

void updateTimers(Uint32 cycles)
{
	gbState.divideRegister += cycles;

	// Divide register
	if (gbState.divideRegister >= 0xFF)
	{
		gbState.divideRegister = 0;
		gbIO.DIVIDER++;
	}

	// Is timer running
	if (gbIO.TIMECONT & (1 << 2))
	{
		gbState.timerPeriod -= cycles;

		// Has the timer reached it's trigger point yet?
		if (gbState.timerPeriod <= 0)
		{
			byte freq = gbIO.TIMECONT & 0x03;
			
			// Refresh the timer
			switch (freq)
			{
				case 0: gbState.timerPeriod += 1024 ; break ; // freq 4096
				case 1: gbState.timerPeriod += 16 ; break ;// freq 262144
				case 2: gbState.timerPeriod += 64 ; break ;// freq 65536
				case 3: gbState.timerPeriod += 256 ; break ;// freq 16382
			}

			if (0xFF == gbIO.TIMECNT)
			{
				gbIO.TIMECNT = gbIO.TIMEMOD;
				
				// Set timer interrupt
				gbIO.IFLAGS |= (1 << 2);
			}
			else
			{
				gbIO.TIMECNT++;
			}
		}
	}
}

byte getJoypadState(void)
{
	byte state = 0x0F;

	// Test for direction key check
	if ((gbIO.JOYPAD & (1 << 4)) == 0x00)
	{
		state = (~gbState.keysState & 0x0F);
	}
    // Tes for button key
	else if ((gbIO.JOYPAD & (1 << 5)) == 0x00)
	{
		state = (~(gbState.keysState >> 4) & 0x0F);
	}

	//printf("Key state read, req: 0x%X, pad state: 0x%X, IO value: 0x%X\n", gbIO.JOYPAD, gbState.keysState, state);
    //exit_with_debug();
	return state;
}

void gbKeyPress(int down, int key)
{
	int alreadySet;
	int requestInterrupt = 0;

	alreadySet = (gbState.keysState >> key) & 0x1;

	//printf("gbKeyPress down: %i, key: %i, set %i\n", down, key, alreadySet);

	if (1 == down)
	{
		gbState.keysState |= (1 << key);
	}
	else
	{
		gbState.keysState &= ~(1 << key);
	}

	if (key > 3)
	{
		if (!(gbIO.JOYPAD & (1 << 5)))
		{
			requestInterrupt = 1;
		}
	}
	else
	{
		if (!(gbIO.JOYPAD & (1 << 4)))
		{
			requestInterrupt = 1;
		}
	}

	if ((1 == requestInterrupt) && (0 == alreadySet))
	{
		gbIO.IFLAGS |= (1 << 4);
	}
}

// For some reason we need to do this otherwise Cygwin spits out undefined
// reference to_WinMain@16 errors
#undef main

int main(int argc, char *argv[])
{
    int cycle_count = 0;
    int cyclesExecuted;
	int arg_pos = 1;
	char* romFile = NULL;
	int fullscreen = FALSE;
	Uint32 videoFlags;
    
    Uint32 lastDelayTime;

    int ii;

    SDL_Event sdl_event;

	// Force DirectX
	//SDL_putenv("SDL_VIDEODRIVER=directx");
	
    printf("\nDoGoBoy - Portable Game Boy Emulator - (c)2009-2013 Douglas Gore\n\n");

    if (argc < 2)
    {
        printf("No ROM image specified, please provide a ROM filename on the command line\n");
        return 1;
    }
	
	while(arg_pos < argc)
	{
		if(strcmp(argv[arg_pos], "-f") == 0)
		{
			fullscreen = TRUE;
		}
		else if(strncmp(argv[arg_pos], "-s", 2) == 0)
		{
			scaleFactor = argv[arg_pos][2] - '0';
			
			if (!(scaleFactor > 0) && !(scaleFactor <= 9))
			{
				printf("ERROR: scale factor must be between 1 and 9\n");
				exit(0);
			}
		}
		else
		{
			romFile = argv[arg_pos];
		}
		
		arg_pos++;
	}
	
	// Initialise SDL with modules we need
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) != 0)
    {
        printf("ERROR: Could not initialise SDL\n");
        return 1;
    }
	
	videoFlags = SDL_HWSURFACE | SDL_HWACCEL | SDL_DOUBLEBUF;

	// Set full screen mode flag (no HW acceleration without it)
	if (fullscreen)
	{
		videoFlags |= SDL_FULLSCREEN;
	}
	
	// Configure video output to 160x144 (GB resolution) at 32bpp
    screen = SDL_SetVideoMode(GB_DISPLAY_WIDTH * scaleFactor, GB_DISPLAY_HEIGHT * scaleFactor, 32, videoFlags);
	//screen = SDL_SetVideoMode(800, 600, 32, videoFlags);

	if (!screen)
	{
		printf("ERROR: Failed to create a valid screen buffer\n");
		return 0;
	}
	else
	{
		printf("Display surface in HW: %s\n", (screen->flags & SDL_HWSURFACE) ? "yes" : "no");
		printf("Display HW accelerated: %s\n", (screen->flags & SDL_HWACCEL) ? "yes" : "no");
		printf("Display double buffered: %s\n", (screen->flags & SDL_DOUBLEBUF) ? "yes" : "no");
	}
	
    Uint32 rmask, gmask, bmask, amask;

    /* SDL interprets each pixel as a 32-bit number, so our masks must depend
       on the endianness (byte order) of the machine */
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

	gbSurface = SDL_CreateRGBSurface(0, GB_DISPLAY_WIDTH, GB_DISPLAY_HEIGHT, 32, rmask, gmask, bmask, 0 /*amask*/);
	
	if (!gbSurface)
	{
		printf("ERROR: Failed to create offscreen surface\n");
		return 0;
	}
	
	SDL_JoystickEventState(SDL_ENABLE);
	joystick = SDL_JoystickOpen(0);

    // Load ROM image from disk
    loadRom(romFile);

	// Initialise the CPU to a known state
    initCPU();

	gbState.IME = 0;
    gbState.lcdMode = 0;
    gbState.lcdModePeriod = LCD_MODE0_PERIOD;
    gbState.currentRomBank = 1;
	gbState.keysState = 0;

	// Below is some initialisation values for the GB I read about somewhere
	
	REGS.w.AF = 0x01B0;
	REGS.w.BC = 0x0013;
    REGS.w.DE = 0x00D8;
    REGS.w.HL = 0x014D;
    REGS.w.SP = 0xFFFE;
	
	writeByteToMemory(0xFF05, 0x00);	// TIMA
	writeByteToMemory(0xFF06, 0x00);	// TMA
	writeByteToMemory(0xFF07, 0x00);	// TAC
	writeByteToMemory(0xFF10, 0x80);	// NR10
	writeByteToMemory(0xFF11, 0xBF);	// NR11
	writeByteToMemory(0xFF12, 0xF3);	// NR12
	writeByteToMemory(0xFF14, 0xBF);	// NR14
	writeByteToMemory(0xFF16, 0x3F);	// NR21
	writeByteToMemory(0xFF17, 0x00);	// NR22
	writeByteToMemory(0xFF19, 0xBF);	// NR24
	writeByteToMemory(0xFF1A, 0x7F);	// NR30
	writeByteToMemory(0xFF1B, 0xFF);	// NR31
	writeByteToMemory(0xFF1C, 0x9F);	// NR32
	writeByteToMemory(0xFF1E, 0xBF);	// NR33
	writeByteToMemory(0xFF20, 0xFF);	// NR41
	writeByteToMemory(0xFF21, 0x00);	// NR42
	writeByteToMemory(0xFF22, 0x00);	// NR43
	writeByteToMemory(0xFF23, 0xBF);	// NR30
	writeByteToMemory(0xFF24, 0x77);	// NR50
	writeByteToMemory(0xFF25, 0xF3);	// NR51
	writeByteToMemory(0xFF26, 0xF1);	// NR52
    writeByteToMemory(0xFF40, 0x91);	// LCDC
    writeByteToMemory(0xFF42, 0x00);	// SCY
    writeByteToMemory(0xFF43, 0x00);	// SCX
    writeByteToMemory(0xFF45, 0x00);	// LYC
    writeByteToMemory(0xFF47, 0xFC);	// BGP
    writeByteToMemory(0xFF48, 0xFF);	// OBP0
   	writeByteToMemory(0xFF49, 0xFF);	// OBP1
   	writeByteToMemory(0xFF4A, 0x00);	// WY
   	writeByteToMemory(0xFF4B, 0x00);	// WX
   	writeByteToMemory(0xFFFF, 0x00);	// IE

    opIndex = 0;
    
    for (ii = 0; ii < 0x100; ii++)
    {
        opcode_coverage[ii] = 0;
        cb_opcode_coverage[ii] = 0;
    }

    last_time = SDL_GetTicks();
    lastDelayTime = SDL_GetTicks();

    // Loop forever (for loops are more efficient than while)
    for(;;)
	{
        cycle_count += CYCLES_PER_FRAME;

        // Run our CPU for the duration of one frame of video
        while(cycle_count > 0)
        {
            //writeLog("LCDSTAT: 0x%X, LCDCONT: 0x%X, LCDY: %d\n", gbIO.LCDSTAT, gbIO.LCDCONT, gbIO.CURLINE);

			// Only execute CPU commands while the CPU is active
            if (0x00 == gbState.cpuHalted)
            {
                cyclesExecuted = executeOpcode();
            }
            else
            {
                cyclesExecuted = 4;
            }
    		
            cycle_count -= cyclesExecuted;

            // Run all the hardware functions
			updateTimers(cyclesExecuted);
            updateGraphics(gbSurface, cyclesExecuted);
            doInterrupts();
        }

        if (showTilemap)
		{
			drawTilemap(gbSurface);
		}

        // Update the LCD display
		drawFrame();

        // Process all pending events e.g. keypreses
        while(SDL_PollEvent(&sdl_event))
        {
            switch(sdl_event.type)
            {
            case SDL_KEYDOWN:
                switch (sdl_event.key.keysym.sym)
                {
                case SDLK_ESCAPE:
                    printf("User pressed escape\n");
                    exit_with_debug();
                break;
				
				case SDLK_F1	: showTilemap = (showTilemap == TRUE) ? FALSE : TRUE; break;

                case SDLK_RIGHT : gbKeyPress(1, 0); break;
                case SDLK_LEFT  : gbKeyPress(1, 1); break;
				case SDLK_UP    : gbKeyPress(1, 2); break;
                case SDLK_DOWN  : gbKeyPress(1, 3); break;
				case SDLK_x     : gbKeyPress(1, 4); break;
				case SDLK_z     : gbKeyPress(1, 5); break;
				case SDLK_SPACE : gbKeyPress(1, 6); break;
                case SDLK_RETURN: gbKeyPress(1, 7); break;

                default:
                    // Ignore keypress
                    break;
                }
            break;

			case SDL_KEYUP:
                switch (sdl_event.key.keysym.sym)
                {
                case SDLK_RIGHT : gbKeyPress(0, 0); break;
                case SDLK_LEFT  : gbKeyPress(0, 1); break;
				case SDLK_UP    : gbKeyPress(0, 2); break;
                case SDLK_DOWN  : gbKeyPress(0, 3); break;
				case SDLK_x     : gbKeyPress(0, 4); break;
				case SDLK_z     : gbKeyPress(0, 5); break;
				case SDLK_SPACE : gbKeyPress(0, 6); break;
                case SDLK_RETURN: gbKeyPress(0, 7); break;

                default:
                    // Ignore keypress
                    break;
				}
			break;

			// Capture joystick movements
			case SDL_JOYAXISMOTION:
			if ( ( sdl_event.jaxis.value < -3200 ) || (sdl_event.jaxis.value > 3200 ) ) 
			{
				// Handle X axis
				if( sdl_event.jaxis.axis == 0) 
				{
					if (sdl_event.jaxis.value < 0)
					{
						gbKeyPress(1, 1);
					}
					else
					{
						gbKeyPress(1, 0);
					}
				}
				// Handle Y axis
				else if( sdl_event.jaxis.axis == 1) 
				{
					if (sdl_event.jaxis.value < 0)
					{
						gbKeyPress(1, 2);
					}
					else
					{
						gbKeyPress(1, 3);
					}
				}
			}
			// Joystick is in dead zone so release any active keys
			else
			{
				gbKeyPress(0, 0);
				gbKeyPress(0, 1);
				gbKeyPress(0, 2);
				gbKeyPress(0, 3);
			}
			break;
			
			// Handle joystick button press
		    case SDL_JOYBUTTONDOWN:
			switch ( sdl_event.jbutton.button ) 
			{
				case  1: gbKeyPress(1, 4); break;
				case  2: gbKeyPress(1, 5); break;
				case 11: gbKeyPress(1, 7); break;
				case  8: gbKeyPress(1, 6); break;
			}
			break;
			
			// Handle joystick button release
		    case SDL_JOYBUTTONUP:
			switch ( sdl_event.jbutton.button ) 
			{
				case  1: gbKeyPress(0, 4); break;
				case  2: gbKeyPress(0, 5); break;
				case 11: gbKeyPress(0, 7); break;
				case  8: gbKeyPress(0, 6); break;
			}
			break;

            case SDL_QUIT:
                printf("Terminating application\n");
                goto quit_app;
            break;

            default:
                // Ignore event
                break;
            }
        }

        // Keep the frame rate locked to an upper limit of 60 FPS
        if ((SDL_GetTicks() - lastDelayTime) <= (1000 / 60))
        {
            SDL_Delay((1000 / 60) - (SDL_GetTicks() - lastDelayTime));
        }

        lastDelayTime = SDL_GetTicks();

        // Record the number of frames per second
        if ((SDL_GetTicks() - last_time) >= 1000)
        {
            sprintf(window_title, "DoGoBoy FPS: %d", frames);

            SDL_WM_SetCaption(window_title, NULL);

            frames = 0;
            last_time = SDL_GetTicks();
        }
	}

quit_app:
	freeGbMemory();

    SDL_Quit();

    return 0;
}
