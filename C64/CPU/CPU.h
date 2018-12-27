/*!
 * @header      CPU.h
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

#ifndef _CPU_INC
#define _CPU_INC

#include "CPU_types.h"
#include "CPUInstructions.h"
#include "TimeDelayed.h"

class Memory;

/*! @class  The virtual 6502 / 6510 processor
 */
class CPU : public VirtualComponent {
    
    //
    // Types
    //
    
    public:
    
    //! @brief    Bit positions of all 7 CPU flags
    typedef enum : uint8_t {
        C_FLAG = 0x01,
        Z_FLAG = 0x02,
        I_FLAG = 0x04,
        D_FLAG = 0x08,
        B_FLAG = 0x10,
        V_FLAG = 0x40,
        N_FLAG = 0x80
    } Flag;
    
    //! @brief    Possible interrupt sources
    typedef enum : uint8_t {
        INTSRC_CIA = 0x01,
        INTSRC_VIC = 0x02,
        INTSRC_VIA1 = 0x04,
        INTSRC_VIA2 = 0x08,
        INTSRC_EXPANSION = 0x10,
        INTSRC_KEYBOARD = 0x20
    } IntSource;
    
    
    //
    // References to other components
    //
    
    private:
    
    //! @brief    Reference to the connected virtual memory
    Memory *mem;


    //
    // Chip properties
    //

    /*! @brief    Selected model
     *  @details  Right now, this atrribute is only used to distinguish the
     *            C64 CPU (MOS6510) from the VC1541 CPU (MOS6502). Hardware
     *            differences between both models are not emulated.
     */
    CPUModel model;
    
    
    //
    // Lookup tables
    //
    
    //! @brief    Mapping from opcodes to microinstructions
    /*! @details  The mapped microinstruction is the first microinstruction to
     *            be executed after the fetch phase (second microcycle).
     */
    MicroInstruction actionFunc[256];
    
    /*! @brief    Textual representation for each opcode
     *  @note     Used by the disassembler, only.
     */
    const char *mnemonic[256];
    
    /*! @brief    Adressing mode of each opcode
     *  @note     Used by the disassembler, only.
     */
    AddressingMode addressingMode[256];
    
    //! @brief    Breakpoint tag for each memory cell
    uint8_t breakpoint[65536];

    
    //
    // Internal state
    //
    
    public:
    
    //! @brief    Elapsed C64 clock cycles since power up
    uint64_t cycle;
    
    //! @brief    Current error state
    ErrorState errorState;
    
    private:

    //! @brief    Next microinstruction to be executed
    /*! @see      executeOneCycle()
     */
    MicroInstruction next;
    
    
    //
    // Registers
    //
    
    public:
    
	//! @brief    Accumulator
	uint8_t regA;
    
	//! @brief    X register
	uint8_t regX;
    
	//! @brief    Y register
	uint8_t regY;
    
    //! @brief    Program counter
    uint16_t regPC;

    //! @brief    Stack pointer
    uint8_t regSP;

    private:
    
    /*! @brief     Processor status register (flags)
     *  @details   7 6 5 4 3 2 1 0
     *             N O - B D I Z C
     */
    uint8_t regP;
    
	//! @brief    Address data (low byte)
	uint8_t regADL;
    
	//! @brief    Address data (high byte)
	uint8_t regADH;
    
	//! @brief    Input data latch (indirect addressing modes)
	uint8_t regIDL;
    
    //! @brief    Data buffer
    uint8_t regD;
    
	/*! @brief    Address overflow indicater
	 *  @details  Indicates when the page boundary has been crossed.
     */
	bool overflow;
    
    /*! @brief    Memory location of the currently executed command.
     *  @details  This variable is overwritten with the current value of the
     *            program counter (regPC) when the CPU executes the fetch phase
     *            of an instruction. Hence, this value always contains the start
     *            address of the currently executed command, even if some
     *            microcycles of the command have already been computed.
     */
    uint16_t pc;
    
  
    //
    // Port lines
    //
    
    public:
    
	/*! @brief    Ready line (RDY)
	 *  @details  If this line is low, the CPU freezes on the next read access.
     *            RDY is pulled down by VICII to perform longer lasting read
     *            operations.
     */
	bool rdyLine;
    
    private:
    
    //! @brief    Cycle of the most recent rising edge of the rdyLine
    uint64_t rdyLineUp;
    
    //! @brief    Cycle of the most recent falling edge of the rdyLine
    uint64_t rdyLineDown;
    
    public:
    
    /*! @brief    NMI line (non maskable interrupts)
     *  @details  This variable is usually set to 0 which means that the NMI
     *            line is in high state. When an external component requests an
     *            NMI nterrupt, this line is pulled low. In that case, this
     *            variable has a positive value and the set bits indicate the
     *            interrupt source.
     */
    uint8_t nmiLine;
    
    
    /*! @brief    IRQ line (maskable interrupts)
     *  @details  This variable is usually set to 0 which means that the IRQ
     *            line is in high state. When an external component requests an
     *            IRQ nterrupt, this line is pulled low. In that case, this
     *            variable has a positive value and the set bits indicate the
     *            interrupt source.
     */
	uint8_t irqLine;

    private:
    
	/*! @brief    Edge detector of NMI line
     *  @details  https://wiki.nesdev.com/w/index.php/CPU_interrupts
     *            "The NMI input is connected to an edge detector. This edge
     *             detector polls the status of the NMI line during φ2 of each
     *             CPU cycle (i.e., during the second half of each cycle) and
     *             raises an internal signal if the input goes from being high
     *             during one cycle to being low during the next. The internal
     *             signal goes high during φ1 of the cycle that follows the one
     *             where the edge is detected, and stays high until the NMI has
     *             been handled."
     */
    TimeDelayed<uint8_t> edgeDetector = TimeDelayed<uint8_t>(1, &cycle);
    
    /*! @brief    Level detector of IRQ line
     *  @details  https://wiki.nesdev.com/w/index.php/CPU_interrupts
     *            "The IRQ input is connected to a level detector. If a low
     *             level is detected on the IRQ input during φ2 of a cycle, an
     *             internal signal is raised during φ1 the following cycle,
     *             remaining high for that cycle only (or put another way,
     *             remaining high as long as the IRQ input is low during the
     *             preceding cycle's φ2).
     */
    TimeDelayed<uint8_t> levelDetector = TimeDelayed<uint8_t>(1, &cycle);
    
    //! @brief    Result of the edge detector polling operation
    /*! @details  https://wiki.nesdev.com/w/index.php/CPU_interrupts
     *            "The output from the edge detector and level detector are
     *             polled at certain points to detect pending interrupts. For
     *             most instructions, this polling happens during the final
     *             cycle of the instruction, before the opcode fetch for the
     *             next instruction. If the polling operation detects that an
     *             interrupt has been asserted, the next "instruction" executed
     *             is the interrupt sequence. Many references will claim that
     *             interrupts are polled during the last cycle of an
     *             instruction, but this is true only when talking about the
     *             output from the edge and level detectors."
     *            Variable is set in macro POLL_INTS (CPUInstructions.h)
     */
    bool doNmi;
    
    //! @brief    Result of the level detector polling operation
    /*! details   https://wiki.nesdev.com/w/index.php/CPU_interrupts
     *  @note     "If both an NMI and an IRQ are pending at the end of an
     *             instruction, the NMI will be handled and the pending status
     *             of the IRQ forgotten (though it's likely to be detected again
     *             during later polling)."
     *            Variable is set in macro POLL_INTS (CPUInstructions.h)
     */
    bool doIrq;
    
    
    //
    // Trace buffer
    //
    
    //! @brief  Trace buffer size
    static const unsigned traceBufferSize = 1024;
    
    //! @brief  Ring buffer for storing the CPU state
    RecordedInstruction traceBuffer[traceBufferSize];
    
    //! @brief  Trace buffer read pointer
    unsigned readPtr;
    
    //! @brief  Trace buffer write pointer
    unsigned writePtr;

    
    //
    //! @functiongroup Constructing and destructing
    //

    public:

    //! @brief    Constructor
    CPU(CPUModel model, Memory *mem);

	//! @brief    Destructor
	~CPU();

    //! @brief    Returns true if this is the C64's CPU
    bool isC64CPU() { return model == MOS_6510; }
    
    private:
    
    //! @brief    Registers a single opcode
    /*! @details  Initializes all lookup table entries for this opcode.
     */
    void registerCallback(uint8_t opcode, const char *mnemonic,
                          AddressingMode mode, MicroInstruction mInstr);
    
    //! @brief    Registers the complete instruction set.
    void registerInstructions();
    
    //! @brief    Registers the legal instruction set, only.
    void registerLegalInstructions();
    
    //! @brief    Registers the illegal instructions set, only.
    void registerIllegalInstructions();
    
    
    //
	//! @functiongroup Methods from VirtualComponent
    //
    
    public:

	void reset();
	void dump();	
    size_t stateSize();
    void didLoadFromBuffer(uint8_t **buffer);
    void didSaveToBuffer(uint8_t **buffer);
    
    
    //
    //! @functiongroup Gathering debug information
    //
    
    //! @brief    Returns the internal state.
    CPUInfo getInfo();
    
    
    //
    //! @functiongroup Handling registers and flags
    //

	/*! @brief    Returns the frozen program counter.
     *  @note     This function returns variable pc that always points to the
     *            beginning of the currently executed command. If execution is
     *            not in microcycle 0 (fetch phase) and the currently executed
     *            command spans over multiple bytes in memory, the program
     *            counter (regPC) may already have been incremented.
     */
    uint16_t getPC() { return pc; }
    
    //! @brief    Redirects the CPU to a new instruction in memory.
    void jumpToAddress(uint16_t addr) { pc = regPC = addr; next = fetch; }

	//! @brief    Returns N_FLAG, if Negative flag is set, 0 otherwise.
    uint8_t getN() { return regP & N_FLAG; }
    
    //! @brief    0: Negative-flag is cleared, any other value: flag is set.
    void setN(uint8_t bit) { bit ? regP |= N_FLAG : regP &= ~N_FLAG; }
    
	//! @brief    Returns V_FLAG, if Overflow flag is set, 0 otherwise.
    uint8_t getV() { return regP & V_FLAG; }
    
    //! @brief    0: Overflow-flag is cleared, any other value: flag is set.
    void setV(uint8_t bit) { bit ? regP |= V_FLAG : regP &= ~V_FLAG; }

	//! @brief    Returns B_FLAG, if Break flag is set, 0 otherwise.
    uint8_t getB() { return regP & B_FLAG; }
    
    //! @brief    0: Break-flag is cleared, any other value: flag is set.
    void setB(uint8_t bit) { bit ? regP |= B_FLAG : regP &= ~B_FLAG; }
    
	//! @brief    Returns D_FLAG, if Decimal flag is set, 0 otherwise.
    uint8_t getD() { return regP & D_FLAG; }
    
    //! @brief    0: Decimal-flag is cleared, any other value: flag is set.
    void setD(uint8_t bit) { bit ? regP |= D_FLAG : regP &= ~D_FLAG; }

	//! @brief    Returns I_FLAG, if Interrupt flag is set, 0 otherwise.
    uint8_t getI() { return regP & I_FLAG; }
    
    //! @brief    0: Interrupt-flag is cleared, any other value: flag is set.
    void setI(uint8_t bit) { bit ? regP |= I_FLAG : regP &= ~I_FLAG; }
    
	//! @brief    Returns Z_FLAG, if Zero flag is set, 0 otherwise.
    uint8_t getZ() { return regP & Z_FLAG; }
    
    //! @brief    0: Zero-flag is cleared, any other value: flag is set.
    void setZ(uint8_t bit) { bit ? regP |= Z_FLAG : regP &= ~Z_FLAG; }
    
	//! @brief    Returns C_FLAG, if Carry flag is set, 0 otherwise.
    uint8_t getC() { return regP & C_FLAG; }
    
    //! @brief    0: Carry-flag is cleared, any other value: flag is set.
    void setC(uint8_t bit) { bit ? regP |= C_FLAG : regP &= ~C_FLAG; }

    private:
    
	/*! @brief    Returns the contents of the status register
	 *  @details  Each bit in the status register corresponds to the value of
     *            a single flag, except bit 5 which is always set.
     */
    uint8_t getP() { return regP | 0b00100000; }

	/*! @brief    Returns the status register without the B flag
	 *  @details  The bit position of the B flag is always 0. This function is
     *            needed for proper interrupt handling. When an IRQ or NMI is
     *            triggered internally, the status register is pushed on the
     *            stack with the B-flag cleared.
     */
    uint8_t getPWithClearedB() { return getP() & 0b11101111; }
    
    //! @brief    Writes a value to the status register.
    void setP(uint8_t p) { regP = p; }
    
    //! @brief    Writes a value to the status register without overwriting B.
    void setPWithoutB(uint8_t p) { regP = (p & 0b11101111) | (regP & 0b00010000); }
    
	//! @brief    Changes low byte of the program counter only.
    void setPCL(uint8_t lo) { regPC = (regPC & 0xff00) | lo; }
    
	//! @brief    Changes high byte of the program counter only.
    void setPCH(uint8_t hi) { regPC = (regPC & 0x00ff) | ((uint16_t)hi << 8); }
    
	//! @brief    Increments the program counter by the specified amount.
    void incPC(uint8_t offset = 1) { regPC += offset; }
    
	/*! @brief    Increments the program counter's low byte.
     *  @note     The high byte does not change.
     */
    void incPCL(uint8_t offset = 1) { setPCL(LO_BYTE(regPC) + offset); }
    
	/*! @brief    Increments the program counter's high byte.
     *  @note     The low byte does not change.
     */
    void incPCH(uint8_t offset = 1) { setPCH(HI_BYTE(regPC) + offset); }
	
	//! @brief    Loads the accumulator. The Z- and N-flag may change.
    void loadA(uint8_t a) { regA = a; setN(a & 0x80); setZ(a == 0); }

	//! @brief    Loads the X register. The Z- and N-flag may change.
    void loadX(uint8_t x) { regX = x; setN(x & 0x80); setZ(x == 0); }
    
	//! @brief    Loads the Y register. The Z- and N-flag may change.
    void loadY(uint8_t y) { regY = y; setN(y & 0x80); setZ(y == 0); }
    
    
    //
    //! @functiongroup Performing ALU operations (CPUInstructions.cpp)
    //
    
    void adc(uint8_t op);
    void adc_binary(uint8_t op);
    void adc_bcd(uint8_t op);
    void sbc(uint8_t op);
    void sbc_binary(uint8_t op);
    void sbc_bcd(uint8_t op);
    void branch(int8_t offset);
    void cmp(uint8_t op1, uint8_t op2);
    uint8_t ror(uint8_t op);
    uint8_t rol(uint8_t op);
    
    
    //
    //! @functiongroup Handling interrupts
    //
    
    public:
    
    /*! @brief    Pulls down the NMI line.
     *  @details  Pulling down the NMI line requests the CPU to interrupt.
     */
    void pullDownNmiLine(IntSource source);
    
    /*! @brief    Releases the NMI line.
     *  @note     Other sources might still hold the line down.
     */
    void releaseNmiLine(IntSource source);
    
    /*! @brief    Pulls down the IRQ line.
     *  @details  Pulling down the IRQ line requests the CPU to interrupt.
     */
    void pullDownIrqLine(IntSource source);
    
    /*! @brief    Releases the IRQ line.
     *  @note     Other sources might still hold the line down.
     */
    void releaseIrqLine(IntSource source);
    
	//! @brief    Sets the RDY line.
    void setRDY(bool value);
		
    
    //
    //! @functiongroup Examining the currently executed instruction
    //
        
	/*! @brief    Returns the length of an instruction in bytes.
	 *  @result   Integer value between 1 and 3.
     */
	unsigned getLengthOfInstruction(uint8_t opcode);
    
	/*! @brief    Returns the length of instruction in bytes.
     *  @result   Integer value between 1 and 3.
     */
    unsigned getLengthOfInstructionAtAddress(uint16_t addr) {
        return getLengthOfInstruction(mem->spypeek(addr)); }
    
	/*! @brief    Returns the length of the currently executed instruction.
     *  @result   Integer value between 1 and 3.
     */
    unsigned getLengthOfCurrentInstruction() {
        return getLengthOfInstructionAtAddress(pc); }
    
	/*! @brief    Returns the address of the next instruction to execute.
     *  @result   Integer value between 1 and 3.
     */
    uint16_t getAddressOfNextInstruction() {
        return pc + getLengthOfCurrentInstruction(); }
    
	/*! @brief    Returns true if the next microcycle is the fetch cycle.
     *  @details  The fetch cycle is the first microinstruction of each command.
     */
    bool inFetchPhase() { return next == fetch; }
	
    
    //
    //! @functiongroup Executing the device
    //
    
	/*! @brief    Executes the next micro instruction.
	 *  @return   true, if the micro instruction was processed successfully.
     *            false, if the CPU was halted, e.g., by reaching a breakpoint.
     */
    bool executeOneCycle();
    
	//! @brief    Returns the current error state.
    ErrorState getErrorState() { return errorState; }
    
	//! @brief    Sets the error state.
    void setErrorState(ErrorState state);
    
	//! @brief    Sets the error state back to normal.
    void clearErrorState() { setErrorState(CPU_OK); }
    
    
    //
    //! @functiongroup Handling breakpoints
    //
    
    //! @brief    Checks if a hard breakpoint is set at the provided address.
    bool hardBreakpoint(uint16_t addr) { return (breakpoint[addr] & HARD_BREAKPOINT) != 0; }
    
	//! @brief    Sets a hard breakpoint at the provided address.
    void setHardBreakpoint(uint16_t addr) { breakpoint[addr] |= HARD_BREAKPOINT; }
	
	//! @brief    Deletes a hard breakpoint at the provided address.
	void deleteHardBreakpoint(uint16_t addr) { breakpoint[addr] &= ~HARD_BREAKPOINT; }
	
	//! @brief    Sets or deletes a hard breakpoint at the provided address.
	void toggleHardBreakpoint(uint16_t addr) { breakpoint[addr] ^= HARD_BREAKPOINT; }
    
    //! @brief    Checks if a soft breakpoint is set at the provided address.
    bool softBreakpoint(uint16_t addr) { return (breakpoint[addr] & SOFT_BREAKPOINT) != 0; }

	//! @brief    Sets a soft breakpoint at the provided address.
	void setSoftBreakpoint(uint16_t addr) { breakpoint[addr] |= SOFT_BREAKPOINT; }
    
	//! @brief    Deletes a soft breakpoint at the specified address.
	void deleteSoftBreakpoint(uint16_t addr) { breakpoint[addr] &= ~SOFT_BREAKPOINT; }
    
	//! @brief    Sets or deletes a hard breakpoint at the specified address.
	void toggleSoftBreakpoint(uint16_t addr) { breakpoint[addr] ^= SOFT_BREAKPOINT; }
    
    
    //
    //! @functiongroup Tracing the program execution
    //
    
    //! @brief  Clears the trace buffer.
    void clearTraceBuffer() { readPtr = writePtr = 0; }
    
    //! @brief   Returns the number of recorded instructions.
    unsigned recordedInstructions(); 
    
    //! @brief   Records an instruction.
    void recordInstruction();
    
    /*! @brief   Reads and removes a recorded instruction from the trace buffer.
     *  @note    The trace buffer must not be empty.
     */
    RecordedInstruction readRecordedInstruction();

    /*! @brief   Reads a recorded instruction from the trace buffer.
     *  @note    'previous' must be smaller than the number of recorded
     *           instructions.
     */
    RecordedInstruction readRecordedInstruction(unsigned previous);

    
    //
    //! @functiongroup Disassembling instructions
    //
    
    //! @brief    Disassembles a previously recorded instruction
    DisassembledInstruction disassemble(RecordedInstruction instr, bool hex);

    //! @brief    Disassembles an instruction at the specified memory location
    DisassembledInstruction disassemble(uint16_t addr, bool hex);
    
    //! @brief    Disassembles the current instruction.
    DisassembledInstruction disassemble(bool hex) { return disassemble(pc, hex); }

};

#endif
