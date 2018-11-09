/*!
 * @header      TOD.h
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

#ifndef _TOD_INC
#define _TOD_INC

#include "VirtualComponent.h"
#include "TOD_types.h"

class CIA;

//! @brief    Increments a BCD number by one.
inline uint8_t incBCD(uint8_t bcd) {
    return ((bcd & 0x0F) == 0x09) ? (bcd & 0xF0) + 0x10 : (bcd & 0xF0) + ((bcd + 0x01) & 0x0F);
}

/*! @brief    Time of day clock (TOD)
 *  @details  Each CIA chip contains a time of day clock, counting hours,
 *            minutes, seconds and tenth of a second. Every TOD clock features
 *            an alarm mechanism. When the alarm time is reached, an interrupt
 *            is initiated.
 */
class TOD : public VirtualComponent {
    
    friend CIA;

private:
    
    //! @brief    Reference to the connected CIA
    CIA *cia;
    
    //! @brief    Time of day clock
	TimeOfDay tod;

    //! @brief    Time of day clock latch
    TimeOfDay latch;

    //! @brief    Alarm time
	TimeOfDay alarm;
	
	/*! @brief    Indicates if the TOD registers are frozen
	 *  @details  The CIA chip freezes the registers when the hours-part is read
     *            and reactivates them, when the 1/10th part is read. Although
     *            the values stay constant, the internal clock continues to run.
     *            Purpose: If you start reading with the hours-part, the clock
     *            won't change until you have read the whole time.
     */
	bool frozen;
	
	/*! @brief    Indicates if the TOD clock is halted.
	 *  @details  The CIA chip stops the TOD clock when the hours-part is
     *            written and restarts it, when the 1/10th part is written.
     *            Purpose: The clock will only start running when the time is
     *            completely set.
     */
	bool stopped;
	
    /*! @brief    Indicates if tod time matches the alarm time
     *  @details  This value is read in checkForInterrupt() for edge detection.
     */
    bool matching;
    
    /*! @brief    Indicates if TOD is driven by a 50 Hz or 60 Hz signal
     *  @details  Valid values are 5 (50 Hz mode) and 6 (60 Hz mode)
     */
    uint8_t hz;
    
    /*! @brief    Frequency counter
     *  @details  This counter is driven by the A/C power frequency and
     *            determines when TOD should increment. This variable is
     *            incremented in function increment() which is called in
     *            endFrame(). Hence, frequencyCounter is a 50 Hz signal in PAL
     *            mode and a 60 Hz signal in NTSC mode.
     */
    uint64_t frequencyCounter;
    
public:
    
    //
    //! @functiongroup Creating and destructing
    //
    
	//! @brief    Constructor
	TOD(CIA *cia);
	
    
    //
    //! @functiongroup Methods from VirtualComponent
    //

	void reset();
	void dump();	

    
    //
    //! @functiongroup Configuring the component
    //
    
    //! @brief    Sets the frequency of the driving clock.
    void setHz(uint8_t value) { assert(value == 5 || value == 6); hz = value; }

    //! @brief    Returns the current configuration.
    TODInfo getInfo();
    

    //
    //! @functiongroup Running the component
    //
    
private:
    
    //! @brief    Freezes the time of day clock.
    void freeze() { if (!frozen) { latch.value = tod.value; frozen = true; } }
    
    //! @brief    Unfreezes the time of day clock.
    void defreeze() { frozen = false; }
    
    //! @brief    Stops the time of day clock.
    void stop() { frequencyCounter = 0; stopped = true; }
    
    //! @brief    Starts the time of day clock.
    void cont() { stopped = false; }

    //! @brief    Returns the hours digits of the time of day clock.
    uint8_t getTodHours() { return (frozen ? latch.hours : tod.hours) & 0x9F; }

    //! @brief    Returns the minutes digits of the time of day clock.
    uint8_t getTodMinutes() { return (frozen ? latch.minutes : tod.minutes) & 0x7F; }

    //! @brief    Returns the seconds digits of the time of day clock.
    uint8_t getTodSeconds() { return (frozen ? latch.seconds : tod.seconds) & 0x7F; }

    //! @brief    Returns the tenth-of-a-second digits of the time of day clock.
    uint8_t getTodTenth() { return (frozen ? latch.tenth : tod.tenth) & 0x0F; }

    //! @brief    Returns the hours digits of the alarm time.
    uint8_t getAlarmHours() { return alarm.hours & 0x9F; }

    //! @brief    Returns the minutes digits of the alarm time.
    uint8_t getAlarmMinutes() { return alarm.minutes & 0x7F; }

    //! @brief    Returns the seconds digits of the alarm time.
    uint8_t getAlarmSeconds() { return alarm.seconds & 0x7F; }

    //! @brief    Returns the tenth-of-a-second digits of the alarm time.
    uint8_t getAlarmTenth() { return alarm.tenth & 0x0F; }
    

	//! @brief    Sets the hours digits of the time of day clock.
    void setTodHours(uint8_t value) { tod.hours = value & 0x9F; checkForInterrupt(); }
	
	//! @brief    Sets the minutes digits of the time of day clock.
    void setTodMinutes(uint8_t value) {
        tod.minutes = value & 0x7F; checkForInterrupt(); }
	
	//! @brief    Sets the seconds digits of the time of day clock.
    void setTodSeconds(uint8_t value) {
        tod.seconds = value & 0x7F; checkForInterrupt(); }
	
	//! @brief    Sets the tenth-of-a-second digits of the time of day clock.
	void setTodTenth(uint8_t value) {
        tod.tenth = value & 0x0F; checkForInterrupt(); }
	
	//! @brief    Sets the hours digits of the alarm time.
    void setAlarmHours(uint8_t value) {
        alarm.hours = value & 0x9F; checkForInterrupt(); }
	
	//! @brief    Sets the minutes digits of the alarm time.
    void setAlarmMinutes(uint8_t value) {
        alarm.minutes = value & 0x7F; checkForInterrupt(); }
	
	//! @brief    Sets the seconds digits of the alarm time.
    void setAlarmSeconds(uint8_t value) {
        alarm.seconds = value & 0x7F; checkForInterrupt(); }
	
	//! @brief    Sets the tenth-of-a-second digits of the time of day clock.
    void setAlarmTenth(uint8_t value) {
        alarm.tenth = value & 0x0F; checkForInterrupt(); }
	
	/*! @brief    Increments the TOD clock by one tenth of a second.
     *  @see      C64::endFrame()
     */
	void increment();

    /*! @brief    Updates variable 'matching'
     *  @details  If a positive edge occurs, the connected CIA will be requested
     *            to trigger an interrupt.
     */
    void checkForInterrupt();
};

#endif


