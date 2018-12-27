/*!
 * @header      Epyx.h
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

#ifndef _EPYX_INC
#define _EPYX_INC

#include "Cartridge.h"

class EpyxFastLoad : public Cartridge {
    
private:
    
    //! @brief    Indicates when the capacitor discharges.
    /*! @details  The Epyx cartridge utilizes a capacitor to switch the ROM on
     *            and off. During normal operation, the capacitor charges
     *            slowly. When it is completely charged, the ROM gets disabled.
     *            When the cartridge is attached, the capacitor is discharged
     *            and the ROM visible. To avoid the ROM to be disabled, the
     *            cartridge can either read from ROML or I/O space 1. Both
     *            operations discharge the capacitor and keep the ROM alive.
     */
    uint64_t cycle = 0;
    
public:
    
    using Cartridge::Cartridge;
    CartridgeType getCartridgeType() { return CRT_EPYX_FASTLOAD; }
    
    //
    //! @functiongroup Methods from VirtualComponent
    //

    void reset();
    size_t stateSize();
    void didLoadFromBuffer(uint8_t **buffer);
    void didSaveToBuffer(uint8_t **buffer);
    
    //
    //! @functiongroup Methods from Cartridge
    //
    
    void resetCartConfig();
    uint8_t peekRomL(uint16_t addr);
    uint8_t spypeekRomL(uint16_t addr) { return Cartridge::peekRomL(addr); }
    uint8_t peekIO1(uint16_t addr);
    uint8_t spypeekIO1(uint16_t addr) { return 0; }
    uint8_t peekIO2(uint16_t addr);
    void execute();

private:
    
    //! @brief    Discharges the cartridge's capacitor
    void dischargeCapacitor();
};

#endif
