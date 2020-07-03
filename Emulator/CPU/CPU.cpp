/*!
 * @file        CPU.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "C64.h"

CPU::CPU(CPUModel model, Memory *mem)
{
    this->model = model;
    this->mem = mem;
	
    setDescription(model == MOS_6502 ? "CPU(6502)" : "CPU");
	debug(3, "  Creating %s at address %p...\n", getDescription(), this);
	
    // Chip model
    model = MOS_6510;

	// Establish callback for each instruction
	registerInstructions();
		
	// Clear all breakpoint tags
	for (int i = 0; i <  65536; i++) {
		breakpoint[i] = NO_BREAKPOINT;	
	}
    
    // Register snapshot items
    SnapshotItem items[] = {
        
        // Lifetime items
        { &model,              sizeof(model),        KEEP_ON_RESET },

         // Internal state
        { &cycle,              sizeof(cycle),        CLEAR_ON_RESET },
        { &errorState,         sizeof(errorState),   CLEAR_ON_RESET },
        { &next,               sizeof(next),         CLEAR_ON_RESET },

        { &regA,               sizeof(regA),         CLEAR_ON_RESET },
        { &regX,               sizeof(regX),         CLEAR_ON_RESET },
        { &regY,               sizeof(regY),         CLEAR_ON_RESET },
        { &regPC,              sizeof(regPC),        CLEAR_ON_RESET },
        { &regSP,              sizeof(regSP),        CLEAR_ON_RESET },
        { &regP,               sizeof(regP),         CLEAR_ON_RESET },
        { &regADL,             sizeof(regADL),       CLEAR_ON_RESET },
        { &regADH,             sizeof(regADH),       CLEAR_ON_RESET },
        { &regIDL,             sizeof(regIDL),       CLEAR_ON_RESET },
        { &regD,               sizeof(regD),         CLEAR_ON_RESET },
        { &overflow,           sizeof(overflow),     CLEAR_ON_RESET },
        { &pc,                 sizeof(pc),           CLEAR_ON_RESET },
        { &rdyLine,            sizeof(rdyLine),      CLEAR_ON_RESET },
        { &rdyLineUp,          sizeof(rdyLineUp),    CLEAR_ON_RESET },
        { &rdyLineDown,        sizeof(rdyLineDown),  CLEAR_ON_RESET },
        { &nmiLine,            sizeof(nmiLine),      CLEAR_ON_RESET },
        { &irqLine,            sizeof(irqLine),      CLEAR_ON_RESET },
        { &doNmi,              sizeof(doNmi),        CLEAR_ON_RESET },
        { &doIrq,              sizeof(doIrq),        CLEAR_ON_RESET },
        { NULL,                0,                    0 }};
    
    registerSnapshotItems(items, sizeof(items));
}

CPU::~CPU()
{
	debug(3, "  Releasing CPU...\n");
}

void
CPU::reset()
{
    VirtualComponent::reset();

    setB(1);
	rdyLine = true;
	next = fetch;
    levelDetector.clear();
    edgeDetector.clear();
    
    clearTraceBuffer();
}

void 
CPU::dump()
{
    DisassembledInstruction instr = disassemble(true /* hex output */);
    
	msg("CPU:\n");
	msg("----\n\n");
    msg("%s: %s %s %s   %s %s %s %s %s %s\n",
        instr.pc,
        instr.byte1, instr.byte2, instr.byte3,
        instr.a, instr.x, instr.y, instr.sp,
        instr.flags,
        instr.command);
	msg("      Rdy line : %s\n", rdyLine ? "high" : "low");
    msg("      Nmi line : %02X\n", nmiLine);
    msg(" Edge detector : %02X\n", edgeDetector.current());
    msg("         doNmi : %s\n", doNmi ? "yes" : "no");
    msg("      Irq line : %02X\n", irqLine);
    msg("Level detector : %02X\n", levelDetector.current());
    msg("         doIrq : %s\n", doIrq ? "yes" : "no");
	msg("   IRQ routine : %02X%02X\n", mem->spypeek(0xFFFF), mem->spypeek(0xFFFE));
	msg("   NMI routine : %02X%02X\n", mem->spypeek(0xFFFB), mem->spypeek(0xFFFA));
	msg("\n");
    
    c64->processorPort.dump();
}

size_t
CPU::stateSize()
{
    return VirtualComponent::stateSize()
    + levelDetector.stateSize()
    + edgeDetector.stateSize();
}

void
CPU::didLoadFromBuffer(uint8_t **buffer)
{
    levelDetector.loadFromBuffer(buffer);
    edgeDetector.loadFromBuffer(buffer);
}

void
CPU::didSaveToBuffer(uint8_t **buffer)
{
    levelDetector.saveToBuffer(buffer);
    edgeDetector.saveToBuffer(buffer);
}

CPUInfo
CPU::getInfo()
{
    CPUInfo info;
    
    info.cycle = cycle;
    info.pc = pc;
    info.a = regA;
    info.x = regX;
    info.y = regY;
    info.sp = regSP;
    info.nFlag = getN();
    info.vFlag = getV();
    info.bFlag = getB();
    info.dFlag = getD();
    info.iFlag = getI();
    info.zFlag = getZ();
    info.cFlag = getC();
  
    return info;
}

void
CPU::pullDownNmiLine(IntSource bit)
{
    assert(bit != 0);
    
    // Check for falling edge on physical line
    if (!nmiLine) {
        edgeDetector.write(1);
    }
    
    nmiLine |= bit;
}

void
CPU::releaseNmiLine(IntSource source)
{
    nmiLine &= ~source;
}

void
CPU::pullDownIrqLine(IntSource source)
{
	assert(source != 0);
    
	irqLine |= source;
    levelDetector.write(irqLine);
}

void
CPU::releaseIrqLine(IntSource source)
{
    irqLine &= ~source;
    levelDetector.write(irqLine);
}

void
CPU::setRDY(bool value)
{
    if (rdyLine)
    {
        rdyLine = value;
        if (!rdyLine) rdyLineDown = cycle;
    }
    else
    {
        rdyLine = value;
        if (rdyLine) rdyLineUp = cycle;
    }
}

unsigned
CPU::getLengthOfInstruction(uint8_t opcode)
{
    switch(addressingMode[opcode]) {
		case ADDR_IMPLIED:			
		case ADDR_ACCUMULATOR:
			return 1;
		case ADDR_IMMEDIATE:
		case ADDR_ZERO_PAGE:
		case ADDR_ZERO_PAGE_X:
		case ADDR_ZERO_PAGE_Y:
		case ADDR_INDIRECT_X:
		case ADDR_INDIRECT_Y:
		case ADDR_RELATIVE:
			return 2;
		case ADDR_ABSOLUTE:
		case ADDR_ABSOLUTE_X:
		case ADDR_ABSOLUTE_Y:
		case ADDR_DIRECT:
		case ADDR_INDIRECT:
			return 3;
	}
	return 1;
}


void 
CPU::setErrorState(ErrorState state)
{
	if (errorState == state)
        return;

    errorState = state;
    
    switch (errorState) {
        case CPU_OK:
            c64->putMessage(MSG_CPU_OK);
            return;
        case CPU_SOFT_BREAKPOINT_REACHED:
            c64->putMessage(MSG_CPU_SOFT_BREAKPOINT_REACHED);
            return; 
        case CPU_HARD_BREAKPOINT_REACHED:
            c64->putMessage(MSG_CPU_HARD_BREAKPOINT_REACHED);
            return;
        case CPU_ILLEGAL_INSTRUCTION:
            c64->putMessage(MSG_CPU_ILLEGAL_INSTRUCTION);
            return;
        default:
            assert(false);
    }
}

unsigned
CPU::recordedInstructions()
{
    return writePtr >= readPtr ? writePtr - readPtr : traceBufferSize + writePtr - readPtr;
}

void
CPU::recordInstruction()
{
    RecordedInstruction i;
    uint8_t opcode = mem->spypeek(pc);
    unsigned length = getLengthOfInstruction(opcode);
    
    i.cycle = cycle;
    i.pc = pc;
    i.byte1 = opcode;
    i.byte2 = length > 1 ? mem->spypeek(i.pc + 1) : 0;
    i.byte3 = length > 2 ? mem->spypeek(i.pc + 2) : 0;
    i.a = regA;
    i.x = regX;
    i.y = regY;
    i.sp = regSP;
    i.flags = getP();
    
    assert(writePtr < traceBufferSize);

    traceBuffer[writePtr] = i;
    writePtr = (writePtr + 1) % traceBufferSize;
    if (writePtr == readPtr) {
        readPtr = (readPtr + 1) % traceBufferSize;
    }
    // debug("readPtr = %d writePtr = %d size = %d\n", readPtr, writePtr, recordedInstructions());
}

RecordedInstruction
CPU::readRecordedInstruction()
{
    assert(recordedInstructions() != 0);
    assert(readPtr < traceBufferSize);
    
    RecordedInstruction result = traceBuffer[readPtr];
    readPtr = (readPtr + 1) % traceBufferSize;
    
    return result;
}

RecordedInstruction
CPU::readRecordedInstruction(unsigned previous)
{
    // debug("previous = %d recInstr = %d\n",previous, recordedInstructions());
    // assert(previous < recordedInstructions());
    return traceBuffer[(writePtr + traceBufferSize - previous - 1) % traceBufferSize];
}


DisassembledInstruction
CPU::disassemble(RecordedInstruction instr, bool hex)
{
    DisassembledInstruction result;
    
    uint8_t opcode = instr.byte1;
    uint8_t length = getLengthOfInstruction(opcode);
    
    result.addr = instr.pc;
    result.size = length;
    
    // Convert command
    char operand[6];
    switch (addressingMode[opcode]) {
            
        case ADDR_IMMEDIATE:
        case ADDR_ZERO_PAGE:
        case ADDR_ZERO_PAGE_X:
        case ADDR_ZERO_PAGE_Y:
        case ADDR_INDIRECT_X:
        case ADDR_INDIRECT_Y: {
            uint8_t value = mem->spypeek(instr.pc + 1);
            hex ? sprint8x(operand, value) : sprint8d(operand, value);
            break;
        }
        case ADDR_DIRECT:
        case ADDR_INDIRECT:
        case ADDR_ABSOLUTE:
        case ADDR_ABSOLUTE_X:
        case ADDR_ABSOLUTE_Y: {
            uint16_t value = LO_HI(mem->spypeek(instr.pc + 1),mem->spypeek(instr.pc + 2));
            hex ? sprint16x(operand, value) : sprint16d(operand, value);
            break;
        }
        case ADDR_RELATIVE: {
            uint16_t value = instr.pc + 2 + (int8_t)mem->spypeek(instr.pc + 1);
            hex ? sprint16x(operand, value) : sprint16d(operand, value);
            break;
        }
        default:
            break;
    }
    
    switch (addressingMode[opcode]) {
        case ADDR_IMPLIED:
        case ADDR_ACCUMULATOR:
            strcpy(result.command, "xxx");
            break;
        case ADDR_IMMEDIATE:
            strcpy(result.command, hex ? "xxx #hh" : "xxx #ddd");
            memcpy(&result.command[5], operand, hex ? 2 : 3);
            break;
        case ADDR_ZERO_PAGE:
            strcpy(result.command, hex ? "xxx hh" : "xxx ddd");
            memcpy(&result.command[4], operand, hex ? 2 : 3);
            break;
        case ADDR_ZERO_PAGE_X:
            strcpy(result.command, hex ? "xxx hh,X" : "xxx ddd,X");
            memcpy(&result.command[4], operand, hex ? 2 : 3);
            break;
        case ADDR_ZERO_PAGE_Y:
            strcpy(result.command, hex ? "xxx hh,Y" : "xxx ddd,Y");
            memcpy(&result.command[4], operand, hex ? 2 : 3);
            break;
        case ADDR_ABSOLUTE:
        case ADDR_DIRECT:
            strcpy(result.command, hex ? "xxx hhhh" : "xxx ddddd");
            memcpy(&result.command[4], operand, hex ? 4 : 5);
            break;
        case ADDR_ABSOLUTE_X:
            strcpy(result.command, hex ? "xxx hhhh,X" : "xxx ddddd,X");
            memcpy(&result.command[4], operand, hex ? 4 : 5);
            break;
        case ADDR_ABSOLUTE_Y:
            strcpy(result.command, hex ? "xxx hhhh,Y" : "xxx ddddd,Y");
            memcpy(&result.command[4], operand, hex ? 4 : 5);
            break;
        case ADDR_INDIRECT:
            strcpy(result.command, hex ? "xxx (hhhh)" : "xxx (ddddd)");
            memcpy(&result.command[5], operand, hex ? 4 : 5);
            break;
        case ADDR_INDIRECT_X:
            strcpy(result.command, hex ? "xxx (hh,X)" : "xxx (ddd,X)");
            memcpy(&result.command[5], operand, hex ? 2 : 3);
            break;
        case ADDR_INDIRECT_Y:
            strcpy(result.command, hex ? "xxx (hh),Y" : "xxx (ddd),Y");
            memcpy(&result.command[5], operand, hex ? 2 : 3);
            break;
        case ADDR_RELATIVE:
            strcpy(result.command, hex ? "xxx hhhh" : "xxx ddddd");
            memcpy(&result.command[4], operand, hex ? 4 : 5);
            break;
        default:
            strcpy(result.command, "???");
    }
    
    // Copy mnemonic
    strncpy(result.command, mnemonic[opcode], 3);
    
    // Convert register contents to strings
    hex ? sprint16x(result.pc, instr.pc) : sprint16d(result.pc, instr.pc);
    hex ? sprint8x(result.a, instr.a) : sprint8d(result.a, instr.a);
    hex ? sprint8x(result.x, instr.x) : sprint8d(result.x, instr.x);
    hex ? sprint8x(result.y, instr.y) : sprint8d(result.y, instr.y);
    hex ? sprint8x(result.sp, instr.sp) : sprint8d(result.sp, instr.sp);
    
    // Convert memory contents to strings
    if (length >= 1) {
        hex ? sprint8x(result.byte1, instr.byte1) : sprint8d(result.byte1, instr.byte1);
    } else {
        hex ? strcpy(result.byte1, "  ") : strcpy(result.byte1, "   ");
    }
    if (length >= 2) {
        hex ? sprint8x(result.byte2, instr.byte2) : sprint8d(result.byte2, instr.byte2);
    } else {
        hex ? strcpy(result.byte2, "  ") : strcpy(result.byte2, "   ");
    }
    if (length >= 3) {
        hex ? sprint8x(result.byte3, instr.byte3) : sprint8d(result.byte3, instr.byte3);
    } else {
        hex ? strcpy(result.byte3, "  ") : strcpy(result.byte3, "   ");
    }
    
    // Convert flags to a string
    result.flags[0] = (instr.flags & N_FLAG) ? 'N' : 'n';
    result.flags[1] = (instr.flags & V_FLAG) ? 'V' : 'v';
    result.flags[2] = '-';
    result.flags[3] = (instr.flags & B_FLAG) ? 'B' : 'b';
    result.flags[4] = (instr.flags & D_FLAG) ? 'D' : 'd';
    result.flags[5] = (instr.flags & I_FLAG) ? 'I' : 'i';
    result.flags[6] = (instr.flags & Z_FLAG) ? 'Z' : 'z';
    result.flags[7] = (instr.flags & C_FLAG) ? 'C' : 'c';
    result.flags[8] = 0;
    
    return result;
}

DisassembledInstruction
CPU::disassemble(uint16_t addr, bool hex)
{
    RecordedInstruction instr;

    instr.pc = addr;
    instr.byte1 = mem->spypeek(addr);
    instr.byte2 = mem->spypeek(addr + 1);
    instr.byte3 = mem->spypeek(addr + 2);
    instr.a = regA;
    instr.x = regX;
    instr.y = regY;
    instr.sp = regSP;
    instr.flags = getP();
    
    return disassemble(instr, hex);
}


