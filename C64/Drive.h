/*!
 * @header      Drive.h
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

/*
 * This implementation is based on the following two documents written
 * by Ruud Baltissen. Ruud, thank you for this excellent work!
 *
 * Description: http://www.baltissen.org/newhtm/1541a.htm
 * Schematics:  http://www.baltissen.org/images/1540.gif
 */
 
#ifndef _VC1541_INC
#define _VC1541_INC

#include "VIA.h"
#include "Disk.h"

/*!
 * @brief    A Commodore VC 1541 disk drive
 */
class VC1541 : public VirtualComponent {

    //
    // Constants
    //

    /*! @brief    Time between two carry pulses of UE7 in 1/10 nano seconds
     *  @details  The VC1541 drive is clocked by 16 Mhz. The base frequency is
     *            divided by N where N ranges from 13 (density bits = 11) to 16
     *            (density bits = 00). On the logic board, this is done with
     *            a 4-bit counter of type 74SL193 whose reset value bits are
     *            connected to the two density bits (PB5 and PB6 of VIA2). It
     *            follows that a single bit is ready after approx. 3.25 CPU
     *            cycles in the fastest zone and approx. 4 CPU cycles in the
     *            slowest zone.
     */
    const uint64_t delayBetweenTwoCarryPulses[4] = {
        10000, // Density bits = 00: Carry pulse every 16/16 * 10^4 1/10 nsec
        9375,  // Density bits = 01: Carry pulse every 15/16 * 10^4 1/10 nsec
        8750,  // Density bits = 10: Carry pulse every 14/16 * 10^4 1/10 nsec
        8125   // Density bits = 11: Carry pulse every 13/16 * 10^4 1/10 nsec
    };
    
    
    //
    // Sub components
    //
    
public:
    
	//! @brief    Memory of the virtual drive
	VC1541Memory mem = VC1541Memory(this);

    //! @brief    The drive's CPU
    CPU cpu = CPU(MOS_6502, &mem);

	//! @brief    VIA6522 connecting the drive CPU with the IEC bus
    VIA1 via1 = VIA1(this);

    //! @brief    VIA6522 connecting the drive CPU with the read/write head
    VIA2 via2 = VIA2(this);

    //! @brief    A single sided 5,25" floppy disk
    Disk disk;
    
    
    //
    // Drive status
    //
    
private:
    
    //! @brief    Internal number of this drive
    /*! @details  The first drive has number 1, the second drive number 2, etc.
     *            The number is set once and never changed afterwards.
     *            When the drive writes a message into the message queue, the
     *            drive number is provided in the data field to let the GUI
     *            know about the sender.
     */
    unsigned deviceNr;
    
    //! @brief    Indicates whether the disk drive is powered on.
    bool poweredOn;

    //! @brief    Indicates whether the disk is rotating.
    bool spinning;
    
    //! @brief    Indicates whether the red LED is on.
    bool redLED;
    
    //! @brief    Indicates if or how a disk is inserted.
    DiskInsertionStatus insertionStatus;
    
    //! @brief    Indicates whether the drive shall send sound notifications.
    bool sendSoundMessages;
    
    
    //
    // Clocking logic
    //
    
    //! @brief    Elapsed time since power up in 1/10 nano seconds
    uint64_t elapsedTime;
    
    //! @brief    Duration of a single CPU clock cycle in 1/10 nano seconds
    uint64_t durationOfOneCpuCycle;
    
    /*! @brief    Indicates when the next drive clock cycle occurs.
     *  @details  The VC1541 drive is clocked by 16 MHz. The clock signal is
     *            fed into a counter which serves as a frequency divider. It's
     *            output is used to clock the drive's CPU and the two VIA chips.
     */
    int64_t nextClock;
     
    /*! @brief    Indicates when the next carry output pulse occurs on UE7.
     *  @details  The 16 MHz signal is also fed into UE7, a 74SL193 4-bit
     *            couter, which generates a carry output signal on overflow.
     *            The pre-load inputs of this counter are connected to PB5 and
     *            PB6 of VIA2 (the 'density bits'). This means that a carry
     *            signal is generated every 13th cycle (from the 16 Mhz clock)
     *            when both density bits are 0 and every 16th cycle when both
     *            density bits are 1. The carry signal drives uf4, a counter of
     *            the same type.
     */
    int64_t nextCarry;
    
public:
    
    /*! @brief    Counts the number of carry pulses from UE7.
     *  @details  In a perfect setting, a new bit is read from or written to the
     *            drive after four carry pulses.
     */
    int64_t carryCounter;
    
    /*! @brief    The second 74SL193 4-bit counter on the logic board.
     *  @details  This counter is driven by the carry output of UE7. It has
     *            four outputs QA, QB, QC, and QD. QA and QB are used to clock
     *            most of the other components. QC and QD are fed into a
     *            NOR gate whose output is connected to the serial input pin of
     *            the input shift register.
     */
    uint4_t counterUF4;
    
    
    //
    // Read/Write logic
    //
    
    //! @brief    The next bit will be ready after this number of cycles.
    int16_t bitReadyTimer;
    
    /*! @brief    Byte ready counter (UE3)
     *  @details  The VC1540 logic board contains a 4-bit-counter of type
     *            72LS191 which is advanced whenever a bit is ready. By reaching
     *            7, the counter signals that a byte is ready. In that case,
     *            the write shift register is loaded with new data and pin CA1
     *            of VIA2 changes state. This state change causes the current
     *            contents of the read shift register to be latched into the
     *            input register of VIA2.
     */
    uint8_t byteReadyCounter;
    
    //! @brief    Halftrack position of the read/write head
    Halftrack halftrack;
    
    //! @brief    Position of the drive head inside the current track
    HeadPosition offset;
    
    /*! @brief    Current disk zone
     *  @details  Each track belongs to one of four zones. Whenever the drive
     *            moves the r/w head, it computes the new number and writes into
     *            PB5 and PB6 of via2. These bits are hard-wired to a 74LS193
     *            counter on the logic board that breaks down the 16 Mhz base
     *            frequency. This mechanism is used to slow down the read/write
     *            process on inner tracks.
     */
    uint8_t zone;
    
    /*! @brief    The 74LS164 serial to parallel shift register
     *  @details  In read mode, this register is fed by the drive head with data.
     */
    uint16_t readShiftreg;
    
    /*! @brief    The 74LS165 parallel to serial shift register
     *  @details  In write mode, this register feeds the drive head with data.
     */
    uint8_t writeShiftreg;
    
    /*! @brief    Current value of the SYNC line
     *  @details  The SYNC signal plays an important role for timing
     *            synchronization. It becomes true when the beginning of a SYNC
     *            is detected. On the logic board, the SYNC signal is computed
     *            by a NAND gate that combines the 10 previously read bits from
     *            the input shift register and VIA2::CB2 (the r/w mode pin).
     *            Connecting CB2 to the NAND gates ensures that SYNC can only be
     *            true in read mode. When SYNC becomes false (meaning that a 0
     *            was pushed into the shift register), the byteReadyCounter is
     *            reset.
     */
    bool sync;
    
    /*! @brief    Current value of the ByteReady line
     *  @details  This signal goes low when a byte has been processed.
     */
    bool byteReady;
    
    public:

    //
    //! @functiongroup Creating and destructing
    //
    
    //! @brief    Custom Constructor
    /*! @param    deviceNr must be 1 (first drive) or 2 (second drive).
     */
    VC1541(unsigned deviceNr);
    
    //! @brief    Standard destructor
    ~VC1541();

    
    //
    //! @functiongroup Methods from VirtualComponent
    //

    void reset();
    void ping();
    void dump();
    void setClockFrequency(uint32_t frequency);

    /*! @brief    Resets all disk related properties
     *  @note     This method is needed, because reset() keeps the disk alive.
     */
    void resetDisk();
    
    
    //
    //! @functiongroup Configuring the device
    //
    
    //! @brief    Returns true if sound messages are sent to the GUI.
    bool soundMessagesEnabled() { return sendSoundMessages; }

    //! @brief    Enables or disables sending of sound messages.
    void setSendSoundMessages(bool b) { sendSoundMessages = b; }

    
    //
    //! @functiongroup Working with the drive
    //

    //! @brief    Returns the device number
    /*! @return   0 for the first drive, 1 for the second.
     */
    unsigned getDeviceNr() { return deviceNr; }
    
    //! @brief    Returns true iff the drive is powered on.
    bool isPoweredOn() { return poweredOn; }

    //! @brief    Returns true iff the drive is powered off.
    bool isPoweredOff() { return !poweredOn; }

    //! @brief    Powers the drive on
    void powerOn();
    
    //! @brief    Powers the drive off
    void powerOff();

    //! @brief    Toggle between power on and power off
    void togglePowerSwitch() { isPoweredOn() ? powerOff() : powerOn(); }
    
    //! @brief    Returns true iff the red drive LED is on.
    bool getRedLED() { return redLED; };

    //! @brief    Turns red drive LED on or off.
    void setRedLED(bool b);

    //! @brief    Returns true iff the drive engine is on.
    bool isRotating() { return spinning; };

    //! @brief    Turns the drive engine on or off.
    void setRotating(bool b);
    
    
    //
    //! @functiongroup Handling virtual disks
    //

    //! @brief    Returns true if a disk is partially inserted.
    bool hasPartiallyInsertedDisk() { return insertionStatus == PARTIALLY_INSERTED; }
    
    //! @brief    Returns true if a disk is fully inserted.
    bool hasDisk() { return insertionStatus == FULLY_INSERTED; }

    //! @brief    Returns true if a modified disk is fully inserted.
    bool hasModifiedDisk() { return hasDisk() && disk.isModified(); }

    //! @brief    Sets or clears the modification flags of a disk
    void setModifiedDisk(bool value);

    //! @brief    Returns true if a write protected disk is fully inserted.
    bool hasWriteProtectedDisk() { return hasDisk() && disk.isWriteProtected(); }

    /*! @brief    Prepares to insert a disk
     *  @details  This functions puts a disk partially into the drive.
     *            disk. As a result, the light barrier is blocked.
     *  @warning  Only call this functions if no disk is inserted.
     */
    void prepareToInsert();
    
    /*! @brief    Inserts an archive as a virtual disk.
     *  @warning  Make sure to eject a previously inserted disk before calling
     *            this function.
     *  @note     Inserting an archive as a disk is a time consuming task
     *            because variouls conversion have to take place. E.g., if you
     *            provide a T64 archive, it is first converted to a D64 archive.
     *            After that, all tracks will be GCR-encoded and written to a
     *            new disk.
     */
    void insertDisk(AnyArchive *a);

    /*! @brief    Returns the current state of the write protection barrier
     *  @details  If the light barrier is blocked, the drive head is unable to
     *            modify bits on disk.
     *  @note     We block the write barrier on power up for about 1.5 sec,
     *            because the drive enters write mode during the power up phase.
     *            I'm unsure if this is normal drive behavior or an emulator
     *            bug. Any hint on this is very welcome!
     */
    bool getLightBarrier() {
        return
        (cpu.cycle < 1500000)
        || hasPartiallyInsertedDisk()
        || disk.isWriteProtected();
    }

    /*! @brief    Prepares to eject a disk
     *  @details  This functions opens the drive lid and partially removes the
     *            disk. As a result, no data can be read any more and the light
     *            barrier is blocked.
     *  @warning  Only call this functions if a disk is inserted.
     */
    void prepareToEject();
    
    /*! @brief    Finishes the ejection of a disk
     *  @details  This function assumes that the drive lid is already open.
     *            It fully removes the disk and frees the light barrier.
     *  @note     To eject a disk in the right way, make sure that some time
     *            elapsed between the two calls to prepareToEject() and
     *            ejectDisk(). Otherwise, the VC1541 DOS does not recognize
     *            the ejection.
     */
    void ejectDisk();
   
    
    //
    //! @functiongroup Running the device
    //
    
    //! @brief    Cold starts the floppy drive
    /*! @details  Mimics the effect of switching the drive off and on again.
     */
    void powerUp();

    //! @brief    Executes all pending cycles of the virtual drive
    /*! @details  The number of cycles is determined by the target time
     *            which is elapsedTime + duration.
     */
    bool execute(uint64_t duration);

private:
    
    //! @brief   Emulates a trigger event on the carry output pin of UE7.
    void executeUF4();
    
public:

    /*! @brief    Returns true iff drive is in read mode
     *  @details  The drive is in read mode iff port pin VIA2::CB2 equals 1.
     */
    bool readMode() { return via2.getCB2(); }
    
    //! @brief    Returns true iff drive is in write mode
    bool writeMode() { return !readMode(); }
    
    //! @brief    Returns the halftrack under the drive head
    Halftrack getHalftrack() { return halftrack; }

    //! @brief    Moves the drive head to the specified track
    void setTrack(Track t) { assert(isTrackNumber(t)); halftrack = 2 * t - 1; }

    //! @brief    Moves the drive head to the specified halftrack
    void setHalftrack(Halftrack ht) { assert(isHalftrackNumber(ht)); halftrack = ht; }
    
    //! @brief    Returns the number of bits in the current halftrack
    uint16_t sizeOfCurrentHalftrack() {
        return hasDisk() ? disk.lengthOfHalftrack(halftrack) : 0; }

    //! @brief    Returns the position of the drive head inside the current track
    HeadPosition getOffset() { return offset; }

    //! @brief    Sets the position of the drive head inside the current track
    void setOffset(HeadPosition pos) {
        if (hasDisk() && disk.isValidHeadPositon(halftrack, pos)) offset = pos;
    }

    //! @brief    Moves head one halftrack up
    void moveHeadUp();
    
    //! @brief    Moves head one halftrack down
    void moveHeadDown();

    //! @brief    Returns the current value of the sync signal
    bool getSync() { return sync; }
    
    //! @brief    Updates the value on the byte ready line
    /*! @note     The byte ready line is connected to the CA1 pin of VIA2.
     *            Pulling this signal low causes important side effects.
     *            Firstly, the contents of the read shift register is latched
     *            into the VIA chip. Secondly, the V flag is set inside the CPU.
     *  @seealso  CA1action()
     */
    void updateByteReady();
    
    //! @brief    Raises the byte ready line
    void raiseByteReady();
    
    //! @brief    Returns the current track zone (0 to 3)
    bool getZone() { return zone; }

    /*! @brief    Sets the current track zone
     *  @param    z drive zone (0 to 3)
     */
    void setZone(uint2_t value);

    /*! @brief    Reads a single bit from the disk head
     *  @result   0 or 1
     */
    uint8_t readBitFromHead() { return disk.readBitFromHalftrack(halftrack, offset); }
    
    //! @brief Writes a single bit to the disk head
    void writeBitToHead(uint8_t bit) {
        disk.writeBitToHalftrack(halftrack, offset, bit); }
    
    //! @brief  Advances drive head position by one bit
    void rotateDisk() { if (++offset >= disk.lengthOfHalftrack(halftrack)) offset = 0; }

    //! @brief  Moves drive head position back by one bit
    void rotateBack() { if (--offset < 0) offset = disk.lengthOfHalftrack(halftrack) - 1; }

private:
    
    //! @brief    Advances drive head position by eight bits
    void rotateDiskByOneByte() { for (unsigned i = 0; i < 8; i++) rotateDisk(); }

    //! @brief    Moves drive head position back by eight bits
    void rotateBackByOneByte() { for (unsigned i = 0; i < 8; i++) rotateBack(); }
};

#endif
