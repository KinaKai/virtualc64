/*!
 * @header      Expert.h
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

#ifndef _EXPERT_INC
#define _EXPERT_INC

#include "Cartridge.h"

class Expert : public Cartridge {
    
    // On-board flipflop
    bool active;
    
public:
    
    Expert(C64 *c64);
    CartridgeType getCartridgeType() { return CRT_EXPERT; }
    
    //
    //! @functiongroup Methods from VirtualComponent
    //
    
    void reset();
    void dump();
    size_t stateSize();
    void didLoadFromBuffer(uint8_t **buffer);
    void didSaveToBuffer(uint8_t **buffer);
    
    //
    //! @functiongroup Methods from Cartridge
    //
    
    void loadChip(unsigned nr, CRTFile *c);
    
    unsigned numButtons() { return 2; }
    const char *getButtonTitle(unsigned nr);
    void pressButton(unsigned nr);
    
    bool hasSwitch() { return true; }
    const char *getSwitchDescription(int8_t pos);
    bool switchInPrgPosition() { return switchIsLeft(); }
    bool switchInOffPosition() { return switchIsNeutral(); }
    bool switchInOnPosition() { return switchIsRight(); }

    void updatePeekPokeLookupTables();
    uint8_t peek(uint16_t addr);
    uint8_t peekIO1(uint16_t addr);
    uint8_t spypeekIO1(uint16_t addr) { return 0; }
    void poke(uint16_t addr, uint8_t value);
    void pokeIO1(uint16_t addr, uint8_t value);
    
    void nmiWillTrigger();
    
    //! @brief    Returns true if cartridge RAM is visible
    bool cartridgeRamIsVisible(uint16_t addr);    

    //! @brief    Returns true if cartridge RAM is write enabled
    bool cartridgeRamIsWritable(uint16_t addr);
};


#endif
