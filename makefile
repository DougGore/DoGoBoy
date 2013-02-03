# makefile for DoGoBoy
#
# Excuse the rubbish makefile, it aims to be cross platform but isn't very well put
# together as I haven't had much time to work on it.

CC = gcc

# Kludge to override paths in environments where sdl-config doesn't work (like Windows)
SDL_PATH := ../SDL-1.2.15/
SDL_INC_PATH := $(SDL_PATH)include/SDL
SDL_LIB_PATH := $(SDL_PATH)lib

CCFLAGS = -I$(SDL_INC_PATH) $(shell sdl-config --cflags) -Wall -D_GNU_SOURCE=1 -Dmain=SDL_main
LDFLAGS = -L$(SDL_LIB_PATH) $(shell sdl-config --libs)

# Cygwin specific flag
ifeq ($(shell uname -o),Cygwin)
	CCFLAGS += -mno-cygwin
endif	
	
TARGET = DoGoBoy

SOURCES = src/main.c src/sharp_LR35902.c src/memory.c src/graphics.c

INCLUDES = -Iinclude
		   
LIBRARIES = \
            -lSDLmain \
            -lSDL \
			-mconsole

all:
	@echo "Compiling, go go DoGoBoy..."
	@$(CC) -mconsole $(SOURCES) $(CCFLAGS) $(INCLUDES) $(LDFLAGS) $(LIBRARIES) -o $(TARGET)
	@echo "Done."
