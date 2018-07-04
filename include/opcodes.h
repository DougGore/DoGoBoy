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

Opcode data to aid with emulating and labels to assist with disassembly.
******************************************************************************/

#ifndef OPCODES_H
#define OPCODES_H

typedef enum
{
    PARAM_NONE,
    PARAM_BYTE,
    PARAM_WORD,
    PARAM_JUMP
} opcodeParams;

typedef struct
{
    char* text;
    uint8_t m_cycles;
} opcodeStruct;

uint8_t opcode_cycles[] =
{
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
	 4, 12,  8,  8,  4,  4,  8,  4, 20,  8,  8,  8,  4,  4,  8,  4,		// 0x00 - 0x0F
     4, 12,  8,  8,  4,  4,  8,  4, 12,  8,  8,  8,  4,  4,  8,  4,		// 0x10 - 0x1F
	 8, 12,  8,  8,  4,  4,  8,  4,  8,  8,  8,  8,  4,  4,  8,  4,		// 0x20 - 0x2F
	 8, 12,  8,  8, 12, 12, 12,  4,  8,  8,  8,  8,  4,  4,  8,  4,		// 0x30 - 0x3F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x40 - 0x4F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x50 - 0x5F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x60 - 0x6F
	 8,  8,  8,  8,  8,  8,  4,  8,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x70 - 0x7F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x80 - 0x8F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0x90 - 0x9F
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0xA0 - 0xAF
	 4,  4,  4,  4,  4,  4,  8,  4,  4,  4,  4,  4,  4,  4,  8,  4,		// 0xB0 - 0xBF
	 8, 12, 12, 16, 12, 16,  8, 16,  8, 16, 12,  0, 12, 24,  8, 16,		// 0xC0 - 0xCF
	 8, 12, 12,  4, 12, 16,  8, 16,  8, 16, 12,  4, 12,  4,  8, 16,		// 0xD0 - 0xDF
	12, 12,  8,  4,  4, 16,  8, 16, 16,  4, 16,  4,  4,  4,  8, 16,		// 0xE0 - 0xEF
	12, 12,  8,  4,  4, 16,  8, 16, 12,  8, 16,  4,  4,  4,  8, 16		// 0xF0 - 0xFF
};

uint8_t cb_opcode_cycles[] =
{
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	 8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	 8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	 8,  8,  8,  8,  8,  8, 12,  8,  8,  8,  8,  8,  8,  8, 12,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8,
	 8,  8,  8,  8,  8,  8, 16,  8,  8,  8,  8,  8,  8,  8, 16,  8
};


opcodeParams opcode_params[] =
{
//  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F
	PARAM_NONE, PARAM_WORD, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_BYTE,  PARAM_NONE,        // 0x00 - 0x07
    PARAM_WORD, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x08 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x10 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x18 - 0x0F
	PARAM_JUMP, PARAM_WORD, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x20 - 0x07
    PARAM_JUMP, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x28 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x30 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x38 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x40 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x48 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x50 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x58 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x60 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x68 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x70 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x78 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x80 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x88 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0x90 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0x98 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0xA0 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0xA8 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0xB0 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0xB8 - 0x0F
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0xC0 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_WORD, PARAM_NONE,  PARAM_NONE,		// 0xC8 - 0xCF
	PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_BYTE,  PARAM_NONE,        // 0xD0 - 0xD7
    PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,		// 0xD8 - 0x0F
	PARAM_BYTE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_BYTE,  PARAM_NONE,        // 0xE0 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_WORD, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_BYTE,  PARAM_NONE,		// 0xE8 - 0x0F
	PARAM_BYTE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_NONE,  PARAM_NONE,        // 0xF0 - 0x07
    PARAM_NONE, PARAM_NONE, PARAM_WORD, PARAM_NONE, PARAM_NONE, PARAM_NONE, PARAM_BYTE,  PARAM_NONE,		// 0xF8 - 0x0F
};

char* opcode_labels[] =
{
    "NOP",                      // 0x00
    "LD  BC, ($X)",             // 0x01
    "LD  (BC), A",              // 0x02
    "INC BC",
    "INC B",
    "DEC B",                    // 0x05
    "LD  B, $%X",               // 0x06
    "RLCA",
    "LD  $%X, SP (non-Z80)",    // 0x08
    "ADD HL, BC",
    "LD  A, (BC)",              // 0x0A
    "DEC BC",                   // 0x0B
    "INC C",                    // 0x0C
    "DEC C",
    "LD  C, n",
    "RRCA",
    "STOP (non-Z80)",           // 0x10
    "LD  DE, nn",               // 0x11
    "LD  (DE), A",              // 0x12
    "INC DE",                   // 0x13
    "INC D",
    "DEC D",
    "LD  D, n",
    "RLA",
    "JR  e",
    "ADD HL, DE",
    "LD  A, (DE)",
    "DEC DE",
    "INC E",
    "DEC E",
    "LD  E, n",
    "RRA",
    "JR  NZ, $%04X",            // 0x20
    "LD  HL, $%X",              // 0x21
    "LD (HLI), A (non-Z80)",    // 0x22
    "INC HL",
    "INC H",
    "DEC H",
    "LD  H, n",
    "DAA",
    "JR  Z, $%04X",             // 0x28
    "ADD HL, HL",               // 0x29
    "LD A, (HLI) (non-Z80)",    // 0x2A
    "DEC HL",
    "INC L",
    "DEC L",                    // 0x32
    "LD  L, n",
    "CPL",
    "JR  NC, e",                // 0x30
    "LD  SP, nn",
    "LD  (HLD), A (non-Z80)",
    "INC SP",
    "INC (HL)",
    "DEC (HL)",
    "LD  (HL), n",
    "SCF",
    "JR  C, e",                 // 0x38
    "DEC SP",
    "LD  A, (HLD) (non-Z80)",
    "ADD HL, SP",
    "INC A",
    "DEC A",
    "LD  A, n",                 // 0x3E
    "CCF",                      // 0x3F
    "LD  B, B",                 // 0x40
    "LD  B, C",
    "LD  B, D",
    "LD  B, E",
    "LD  B, H",
    "LD  B, L",
    "LD  B, (HL)",
    "LD  B, A",
    "LD  C, B",
    "LD  C, C",
    "LD  C, D",
    "LD  C, E",
    "LD  C, H",
    "LD  C, L",
    "LD  C, (HL)",
    "LD  C, A",
    "LD  D, B",      // 0x50
    "LD  D, C",
    "LD  D, D",
    "LD  D, E",
    "LD  D, H",
    "LD  D, L",
    "LD  D, (HL)",      // 0x56
    "LD  D, A",         // 0x57
    "LD  E, B",
    "LD  E, C",
    "LD  E, D",
    "LD  E, E",
    "LD  E, H",
    "LD  E, L",
    "LD  E, (HL)",
    "LD  E, A",             // 0x5E
    "LD  H, B",                 // 0x60
    "LD  H, C",
    "LD  H, D",
    "LD  H, E",
    "LD  H, H",
    "LD  H, L",
    "LD  H, (HL)",
    "LD  H, A",
    "LD  L, B",
    "LD  L, C",
    "LD  L, D",
    "LD  L, E",
    "LD  L, H",
    "LD  L, L",
    "LD  L, (HL)",
    "LD  L, A",
    "LD  (HL), B",              // 0x70
    "LD  (HL), C",
    "LD  (HL), D",
    "LD  (HL), E",
    "LD  (HL), H",
    "LD  (HL), L",
    "HALT",
    "LD  (HL), A",
    "LD  A, B",                 // 0x78
    "LD  A, C",                 // 0x79
    "LD  A, D",
    "LD  A, E",
    "LD  A, H",
    "LD  A, L",
    "LD  A, (HL)",
    "LD  A, A",                 // 0x7E
    "ADD A, B",                 // 0x80
    "ADD A, C",
    "ADD A, D",
    "ADD A, E",
    "ADD A, H",
    "ADD A, L",
    "ADD A, (HL)",
    "ADD A, A",
    "ADC A, B",
    "ADC A, C",
    "ADC A, D",
    "ADC A, E",
    "ADC A, H",
    "ADC A, L",  // ADC A, H - 4
    "ADC A, (HL)",
    "ADC A, A",
    "SUB B",      // 0x90
    "SUB C",
    "SUB D",
    "SUB E",
    "SUB H",
    "SUB L",
    "SUB (HL)",
    "SUB A",
    "SBC B",
    "SBC C",
    "SBC D",
    "SBC E",
    "SBC H",
    "SBC L",
    "SBC (HL)",
    "SBC A, A",
    "AND B",      // 0xA0
    "AND C",
    "AND D",
    "AND E",
    "AND H",
    "AND L",
    "AND (HL)",
    "AND A",
    "XOR B",
    "XOR C",
    "XOR D",
    "XOR E",
    "XOR H",
    "XOR L",
    "XOR (HL)",
    "XOR A",
    "OR  B",                    // 0xB0
    "OR  C",                    // 0xB1
    "OR  D",
    "OR  E",
    "OR  H",
    "OR  L",
    "OR  (HL)",
    "OR  A",
    "CP  B",
    "CP  C",
    "CP  D",
    "CP  E",
    "CP  H",
    "CP  L",
    "CP  (HL)",
    "CP  A",
    "RET NZ",           // 0xC0
    "POP BC",               // 0xC1
    "JP  NZ, nn",
    "JP  nn",               // 0xC3
    "CALL NZ, nn",
    "PUSH BC",              // 0xC5
    "ADD A, n",
    "RST 00H",
    "RET Z",
    "RET",              // 0xC9
    "JP  Z, nn",
    "#CB",              // 0xCB
    "CALL Z, nn",
    "CALL $%X",                 // 0xCD
    "ADC A, n",
    "RST 08H",
    "RET NC",                   // 0xD0
    "POP DE",
    "JZ NC, nn",
    "NOP (non-Z80)",
    "CALL NC",
    "PUSH DE",
    "SUB $%02X",                // 0xD6
    "RST 10H",
    "RET C",
    "RETI (non-Z80)",
    "JP  C, nn",
    "NOP (non-Z80)",
    "CALL C, nn",
    "NOP (non-Z80)",
    "SBC n",
    "RST 18H",
    "LD  $FF%02X, A (non-Z80)",     // 0xE0
    "POP HL",                       // 0xE1
    "LD  (C), A (non-Z80)",
    "NOP (non-Z80)",
    "NOP (non-Z80)",
    "PUSH HL",                      // 0xE5
    "AND $%02X",                    // 0xE6
    "RST 20H",
    "ADD SP, n (non-Z80)",
    "JP  (HL)",
    "LD  $%X, A (non-Z80)",          // 0xEA
    "NOP (non-Z80)",
    "NOP (non-Z80)",
    "",
    "XOR $%X",
    "RST 28H",
    "LD  A, $FF%02X (non-Z80)",     // 0xF0
    "POP AF",                       // 0xF1
    "NOP (non-Z80)",
    "DI",
    "NOP (non-Z80)",
    "PUSH AF",                      // 0xF5
    "OR  n",
    "RST 30H",
    "LDHL SP, n (non-Z80)",
    "LD  SP, HL",
    "LD  A, ($%X) (non-Z80)",       // 0xFA
    "EI",
    "NOP (non-Z80)",
    "NOP (non-Z80)",
    "CP  $%X",                      // 0xFE
    "RST 38H"                       // 0xFF
};

opcodeStruct cb_opcodes[] =
{
    { "RLC B", 8 },      // 0x00
    { "RLC C", 8 },
    { "RLC D", 8 },
    { "RLC E", 8 },
    { "RLC H", 8 },
    { "RLC L", 8 },
    { "RLC (HL)", 16 },
    { "RLC A", 8 },
    { "RRC B", 8 },
    { "RRC C", 8 },
    { "RRC D", 8 },
    { "RRC E", 8 },
    { "RRC H", 8 },
    { "RRC L", 8 },
    { "RRC (HL)", 16 },
    { "RRC A", 8 },
    { "RL  B", 8 },      // 0x10
    { "RL  C", 8 },
    { "RL  D", 8 },
    { "RL  E", 8 },
    { "RL  H", 8 },
    { "RL  L", 8 },
    { "RL  (HL)", 15 },
    { "RL  A", 8 },
    { "RR B", 8 },
    { "RR C", 8 },
    { "RR D", 8 },
    { "RR E", 8 },
    { "RR H", 8 },
    { "RR L", 8 },
    { "RR (HL)", 15 },
    { "RR A", 8 },
    { "SLA B", 8 },      // 0x20
    { "SLA C", 8 },
    { "SLA D", 8 },
    { "SLA E", 8 },
    { "SLA H", 8 },
    { "SLA L", 8 },
    { "SLA (HL)", 12 },
    { "SLA A", 8 },     // 0x27
    { "SRA B", 8 },
    { "SRA C", 8 },
    { "SRA D", 8 },
    { "SRA E", 8 },
    { "SRA H", 8 },
    { "SRA L", 8 },
    { "SRA (HL)", 12 },
    { "SRA A", 8 },
    { "SWAP B", 8 },      // 0x30
    { "SWAP C", 8 },
    { "SWAP D", 8 },
    { "SWAP E", 8 },
    { "SWAP H", 8 },
    { "SWAP L", 8 },
    { "SWAP (HL)", 16 },
    { "SWAP A", 8 },
    { "SRL B", 8 },
    { "SRL C", 8 },
    { "SRL D", 8 },
    { "SRL E", 8 },
    { "SRL H", 8 },
    { "SRL L", 8 },
    { "SRL (HL)", 16 },
    { "SRL A", 8 },
    { "BIT 0, B", 8 },      // 0x40
    { "BIT 0, C", 8 },
    { "BIT 0, D", 8 },
    { "BIT 0, E", 8 },
    { "BIT 0, H", 8 },
    { "BIT 0, L", 8 },
    { "BIT 0, (HL)", 12 },
    { "BIT 0, A", 8 },
    { "BIT 1, B", 8 },
	{ "BIT 1, C", 8 },
    { "BIT 1, D", 8 },
    { "BIT 1, E", 8 },
    { "BIT 1, H", 8 },
    { "BIT 1, L", 8 },
    { "BIT 1, (HL)", 12 },
    { "BIT 1, A", 8 },
    { "BIT 2, B", 8 },      // 0x50
    { "BIT 2, C", 8 },
    { "BIT 2, D", 8 },
    { "BIT 2, E", 8 },
    { "BIT 2, H", 8 },
    { "BIT 2, L", 8 },
    { "BIT 2, (HL)", 12 },
    { "BIT 2, A", 8 },
    { "BIT 3, B", 8 },
	{ "BIT 3, C", 8 },
    { "BIT 3, D", 8 },
    { "BIT 3, E", 8 },
    { "BIT 3, H", 8 },
    { "BIT 3, L", 8 },
    { "BIT 3, (HL)", 12 },
    { "BIT 3, A", 8 },
    { "BIT 4, B", 8 },      // 0x60
    { "BIT 4, C", 8 },
    { "BIT 4, D", 8 },
    { "BIT 4, E", 8 },
    { "BIT 4, H", 8 },
    { "BIT 4, L", 8 },
    { "BIT 4, (HL)", 12 },  // 0x66
    { "BIT 4, A", 8 },
    { "BIT 5, B", 8 },
	{ "BIT 5, C", 8 },
    { "BIT 5, D", 8 },
    { "BIT 5, E", 8 },
    { "BIT 5, H", 8 },
    { "BIT 5, L", 8 },
    { "BIT 5, (HL)", 12 },
    { "BIT 5, A", 8 },
    { "BIT 6, B", 8 },      // 0x70
    { "BIT 6, C", 8 },
    { "BIT 6, D", 8 },
    { "BIT 6, E", 8 },
    { "BIT 6, H", 8 },
    { "BIT 6, L", 8 },
    { "BIT 6, (HL)", 12 },  // 0x76
    { "BIT 6, A", 8 },
    { "BIT 7, B", 8 },
	{ "BIT 7, C", 8 },
    { "BIT 7, D", 8 },
    { "BIT 7, E", 8 },
    { "BIT 7, H", 8 },
    { "BIT 7, L", 8 },
    { "BIT 7, (HL)", 12 },
    { "BIT 7, A", 8 },      // 0x7E
    { "RES 0, B", 8 },      // 0x80
    { "RES 0, C", 8 },
    { "RES 0, D", 8 },
    { "RES 0, E", 8 },
    { "RES 0, H", 8 },
    { "RES 0, L", 8 },
    { "RES 0, (HL)", 12 },
    { "RES 0, A", 8 },
    { "RES 1, B", 8 },
    { "RES 1, C", 8 },
    { "RES 1, D", 8 },
    { "RES 1, E", 8 },
    { "RES 1, H", 8 },
    { "RES 1, L", 8 },
    { "RES 1, (HL)", 12 },
    { "RES 1, A", 8 },
    { "RES 2, B", 8 },      // 0x90
    { "RES 2, C", 8 },
    { "RES 2, D", 8 },
    { "RES 2, E", 8 },
    { "RES 2, H", 8 },
    { "RES 2, L", 8 },
    { "RES 2, (HL)", 12 },
    { "RES 2, A", 8 },
    { "RES 3, B", 8 },
    { "RES 3, C", 8 },
    { "RES 3, D", 8 },
    { "RES 3, E", 8 },
    { "RES 3, H", 8 },
    { "RES 3, L", 8 },
    { "RES 3, (HL)", 12 },
    { "RES 3, A", 8 },
    { "RES 4, B", 8 },      // 0xA0
    { "RES 4, C", 8 },
    { "RES 4, D", 8 },
    { "RES 4, E", 8 },
    { "RES 4, H", 8 },
    { "RES 4, L", 8 },
    { "RES 4, (HL)", 12 },  // 0xA6
    { "RES 4, A", 8 },
    { "RES 5, B", 8 },
    { "RES 5, C", 8 },
    { "RES 5, D", 8 },
    { "RES 5, E", 8 },
    { "RES 5, H", 8 },
    { "RES 5, L", 8 },
    { "RES 5, (HL)", 12 },
    { "RES 5, A", 8 },
    { "RES 6, B", 8 },      // 0xB0
    { "RES 6, C", 8 },
    { "RES 6, D", 8 },
    { "RES 6, E", 8 },
    { "RES 6, H", 8 },
    { "RES 6, L", 8 },
    { "RES 6, (HL)", 12 },
    { "RES 6, A", 8 },
    { "RES 7, B", 8 },
    { "RES 7, C", 8 },
    { "RES 7, D", 8 },
    { "RES 7, E", 8 },
    { "RES 7, H", 8 },
    { "RES 7, L", 8 },
    { "RES 7, (HL)", 12 },
    { "RES 7, A", 8 },
    { "SET 0, B", 8 },		// 0xC0
    { "SET 0, C", 8 },
    { "SET 0, D", 8 },
    { "SET 0, E", 8 },
    { "SET 0, H", 8 },
    { "SET 0, L", 8 },
    { "SET 0, (HL)", 15 },
    { "SET 0, A", 8 },
    { "SET 1, B", 8 },
    { "SET 1, C", 8 },
    { "SET 1, D", 8 },
    { "SET 1, E", 8 },
    { "SET 1, H", 8 },
    { "SET 1, L", 8 },
    { "SET 1, (HL)", 15 },
    { "SET 1, A", 8 },
    { "SET 2, B", 8 },		// 0xD0
    { "SET 2, C", 8 },
    { "SET 2, D", 8 },
    { "SET 2, E", 8 },
    { "SET 2, H", 8 },
    { "SET 2, L", 8 },
    { "SET 2, (HL)", 15 },
    { "SET 2, A", 8 },
    { "SET 3, B", 8 },
    { "SET 3, C", 8 },
    { "SET 3, D", 8 },
    { "SET 3, E", 8 },
    { "SET 3, H", 8 },
    { "SET 3, L", 8 },
    { "SET 3, (HL)", 15 },
    { "SET 3, A", 8 },
    { "SET 4, B", 8 },		// 0xE0
    { "SET 4, C", 8 },
    { "SET 4, D", 8 },
    { "SET 4, E", 8 },
    { "SET 4, H", 8 },
    { "SET 4, L", 8 },
    { "SET 4, (HL)", 15 },  // 0xE6
    { "SET 4, A", 8 },
    { "SET 5, B", 8 },
    { "SET 5, C", 8 },
    { "SET 5, D", 8 },
    { "SET 5, E", 8 },
    { "SET 5, H", 8 },
    { "SET 5, L", 8 },
    { "SET 5, (HL)", 15 },
    { "SET 5, A", 8 },
    { "SET 6, B", 8 },		// 0xF0
    { "SET 6, C", 8 },
    { "SET 6, D", 8 },
    { "SET 6, E", 8 },
    { "SET 6, H", 8 },
    { "SET 6, L", 8 },
    { "SET 6, (HL)", 15 },
    { "SET 6, A", 8 },
    { "SET 7, B", 8 },
    { "SET 7, C", 8 },
    { "SET 7, D", 8 },
    { "SET 7, E", 8 },
    { "SET 7, H", 8 },
    { "SET 7, L", 8 },
    { "SET 7, (HL)", 15 },  // 0xFE
    { "SET 7, A", 8 },      // 0xFF
};

#endif  // OPCODES_H
