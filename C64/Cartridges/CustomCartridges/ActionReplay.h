/*!
 * @header      ActionReplay.h
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

#ifndef _ACTIONREPLAY_INC
#define _ACTIONREPLAY_INC

#include "Cartridge.h"

//
// Action Replay (hardware version 3)
//

class ActionReplay3 : public CartridgeWithRegister {
    
public:
    
    ActionReplay3(C64 *c64) : CartridgeWithRegister(c64, "AR3") { };
    CartridgeType getCartridgeType() { return CRT_ACTION_REPLAY3; }
    
    //
    //! @functiongroup Methods from Cartridge
    //
    
    uint8_t peek(uint16_t addr);
    uint8_t peekIO1(uint16_t addr);
    uint8_t peekIO2(uint16_t addr);
    
    void pokeIO1(uint16_t addr, uint8_t value);
    
    unsigned numButtons() { return 2; }
    const char *getButtonTitle(unsigned nr);
    void pressButton(unsigned nr);
    void releaseButton(unsigned nr);
    
    //! @brief   Sets the cartridge's control register
    /*! @details This function triggers all side effects that take place when
     *           the control register value changes.
     */
    void setControlReg(uint8_t value);
    
    unsigned bank() { return control & 0x01; }
    bool game() { return !!(control & 0x02); }
    bool exrom() { return !(control & 0x08); }
    bool disabled() { return !!(control & 0x04); }
};


//
// Action Replay (hardware version 4 and above)
//

class ActionReplay : public CartridgeWithRegister {
    
public:
    
    ActionReplay(C64 *c64);
    CartridgeType getCartridgeType() { return CRT_ACTION_REPLAY; }
    
    //
    //! @functiongroup Methods from VirtualComponent
    //
    
    void reset();
    size_t stateSize() { return Cartridge::stateSize() + 1; }
    void didLoadFromBuffer(uint8_t **buffer) {
        Cartridge::didLoadFromBuffer(buffer); control = read8(buffer); }
    void didSaveToBuffer(uint8_t **buffer) {
        Cartridge::didSaveToBuffer(buffer); write8(buffer, control); }
    
    //
    //! @functiongroup Methods from Cartridge
    //
    
    void resetCartConfig();
    
    uint8_t peek(uint16_t addr);
    uint8_t peekIO1(uint16_t addr);
    uint8_t peekIO2(uint16_t addr);
    
    void poke(uint16_t addr, uint8_t value);
    void pokeIO1(uint16_t addr, uint8_t value);
    void pokeIO2(uint16_t addr, uint8_t value);
    
    unsigned numButtons() { return 2; }
    const char *getButtonTitle(unsigned nr);
    void pressButton(unsigned nr);
    void releaseButton(unsigned nr);
    
    
    //! @brief   Sets the cartridge's control register
    /*! @details This function triggers all side effects that take place when
     *           the control register value changes.
     */
    void setControlReg(uint8_t value);
    
    virtual unsigned bank() { return (control >> 3) & 0x03; }
    virtual bool game() { return (control & 0x01) == 0; }
    virtual bool exrom() { return (control & 0x02) != 0; }
    virtual bool disabled() { return (control & 0x04) != 0; }
    virtual bool resetFreezeMode() { return (control & 0x40) != 0; }
    
    //! @brief  Returns true if the cartridge RAM shows up at addr
    virtual bool ramIsEnabled(uint16_t addr); 
};


//
// Atomic Power (a derivation of the Action Replay cartridge)
//

class AtomicPower : public ActionReplay {
    
public:
    
    AtomicPower(C64 *c64);
    CartridgeType getCartridgeType() { return CRT_ATOMIC_POWER; }
    
    /*! @brief    Indicates if special ROM / RAM config has to be used.
     * @details   In contrast to the Action Replay cartridge, Atomic Power
     *            has the ability to map the on-board RAM to the ROMH area
     *            at $A000 - $BFFF. To enable this special configuration, the
     *            control register has to be configured as follows:
     *            Bit 0b10000000 (Extra ROM)    is 0.
     *            Bit 0b01000000 (Freeze clear) is 0.
     *            Bit 0b00100000 (RAM enable)   is 1.
     *            Bit 0b00000100 (Disable)      is 0.
     *            Bit 0b00000010 (Exrom)        is 1.
     *            Bit 0b00000001 (Game)         is 0.
     */
    bool specialMapping() { return (control & 0b11100111) == 0b00100010; }
    
    bool game();
    bool exrom();
    bool ramIsEnabled(uint16_t addr);
};

#endif
