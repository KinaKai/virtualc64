/*
 * Written by Dirk Hoffmann based on the original code by A. Carl Douglas.
 *
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

ExpansionPort::ExpansionPort()
{
    setDescription("Expansion port");
    debug(3, "  Creating expansion port at address %p...\n", this);
    
    // Register snapshot items
    SnapshotItem items[] = {
        
        // Internal state
        { &gameLine,          sizeof(gameLine),        KEEP_ON_RESET },
        { &exromLine,         sizeof(exromLine),       KEEP_ON_RESET },
        { NULL,               0,                       0 }};
    
    registerSnapshotItems(items, sizeof(items));
}

ExpansionPort::~ExpansionPort()
{
    debug(3, "  Releasing expansion port...\n");
    detachCartridge();
}

void
ExpansionPort::reset()
{
    VirtualComponent::reset();
    
    if (cartridge) {
        cartridge->reset();
        cartridge->resetCartConfig();
    } else {
        setCartridgeMode(CRT_OFF);
    }
}

void
ExpansionPort::ping()
{
    VirtualComponent::ping();
    c64->putMessage(cartridge ? MSG_CARTRIDGE : MSG_NO_CARTRIDGE);
    c64->putMessage(MSG_CART_SWITCH);
}

size_t
ExpansionPort::stateSize()
{
    return VirtualComponent::stateSize()
    + 2
    + (cartridge ? cartridge->stateSize() : 0);
}

void
ExpansionPort::didLoadFromBuffer(uint8_t **buffer)
{
    // Delete old cartridge (if any)
    if (cartridge != NULL) {
        delete cartridge;
        cartridge = NULL;
    }
    
    // Read cartridge type and cartridge (if any)
    CartridgeType cartridgeType = (CartridgeType)read16(buffer);
    if (cartridgeType != CRT_NONE) {
        cartridge = Cartridge::makeWithType(c64, cartridgeType);
        cartridge->loadFromBuffer(buffer);
    }
}

void
ExpansionPort::didSaveToBuffer(uint8_t **buffer)
{
    // Write cartridge type and data (if any)
    write16(buffer, cartridge ? cartridge->getCartridgeType() : CRT_NONE);
    if (cartridge != NULL)
        cartridge->saveToBuffer(buffer);
}

void
ExpansionPort::dump()
{
    msg("Expansion port\n");
    msg("--------------\n");
    
    msg(" Game line:  %d\n", gameLine);
    msg("Exrom line:  %d\n", exromLine);

    if (cartridge == NULL) {
        msg("No cartridge attached\n");
    } else {
        cartridge->dump();
    }
}

CartridgeType
ExpansionPort::getCartridgeType()
{
    return cartridge ? cartridge->getCartridgeType() : CRT_NONE;
}

uint8_t
ExpansionPort::peek(uint16_t addr)
{
    return cartridge ? cartridge->peek(addr) : 0;
}

uint8_t
ExpansionPort::spypeek(uint16_t addr)
{
    return cartridge ? cartridge->spypeek(addr) : 0;
}

/*
uint8_t
ExpansionPort::peek(uint16_t addr)
{
    assert((addr >= 0x8000 && addr <= 0x9FFF) ||
           (addr >= 0xA000 && addr <= 0xBFFF) ||
           (addr >= 0xE000 && addr <= 0xFFFF));
    
    if (cartridge) {
        if (addr <= 0x9FFF) {
            return cartridge->peekRomLabs(addr);
        } else {
            return cartridge->peekRomHabs(addr);
        }
    }
    return 0;
}
*/

uint8_t
ExpansionPort::peekIO1(uint16_t addr)
{
    /* "Die beiden mit "I/O 1" und "I/O 2" bezeichneten Bereiche
     *  sind für Erweiterungskarten reserviert und normalerweise ebenfalls offen,
     *  ein Lesezugriff liefert auch hier "zufällige" Daten (dass diese Daten gar
     *  nicht so zufällig sind, wird in Kapitel 4 noch ausführlich erklärt. Ein
     *  Lesen von offenen Adressen liefert nämlich auf vielen C64 das zuletzt vom
     *  VIC gelesene Byte zurück!)" [C.B.]
     */
    return cartridge ? cartridge->peekIO1(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::spypeekIO1(uint16_t addr)
{
    return cartridge ? cartridge->spypeekIO1(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::peekIO2(uint16_t addr)
{
    return cartridge ? cartridge->peekIO2(addr) : c64->vic.getDataBusPhi1();
}

uint8_t
ExpansionPort::spypeekIO2(uint16_t addr)
{
    return cartridge ? cartridge->spypeekIO2(addr) : c64->vic.getDataBusPhi1();
}

void
ExpansionPort::poke(uint16_t addr, uint8_t value)
{
    if (cartridge) {
        cartridge->poke(addr, value);
    } else if (!c64->getUltimax()) {
        c64->mem.ram[addr] = value;
    }
}

void
ExpansionPort::pokeIO1(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDE00 && addr <= 0xDEFF);
    
    if (cartridge) cartridge->pokeIO1(addr, value);
}

void
ExpansionPort::pokeIO2(uint16_t addr, uint8_t value)
{
    assert(addr >= 0xDF00 && addr <= 0xDFFF);
    
    if (cartridge) cartridge->pokeIO2(addr, value);
}

void
ExpansionPort::setGameLine(bool value)
{
    gameLine = value;
    c64->vic.setUltimax(!gameLine && exromLine);
    c64->mem.updatePeekPokeLookupTables();
}

void
ExpansionPort::setExromLine(bool value)
{
    exromLine = value;
    c64->vic.setUltimax(!gameLine && exromLine);    
    c64->mem.updatePeekPokeLookupTables();
}

void
ExpansionPort::setGameAndExrom(bool game, bool exrom)
{
    gameLine = game;
    exromLine = exrom;
    c64->vic.setUltimax(!gameLine && exromLine);
    c64->mem.updatePeekPokeLookupTables();
}

CartridgeMode
ExpansionPort::getCartridgeMode()
{
    switch ((exromLine ? 0b10 : 0) | (gameLine ? 0b01 : 0)) {
            
        case 0b00: return CRT_16K;
        case 0b01: return CRT_8K;
        case 0b10: return CRT_ULTIMAX;
        default:   return CRT_OFF;
    }
}

void
ExpansionPort::setCartridgeMode(CartridgeMode mode)
{
    switch (mode) {
        case CRT_16K:     setGameAndExrom(0,0); return;
        case CRT_8K:      setGameAndExrom(1,0); return;
        case CRT_ULTIMAX: setGameAndExrom(0,1); return;
        default:          setGameAndExrom(1,1);
    }
}

void
ExpansionPort::updatePeekPokeLookupTables()
{
    if (cartridge) cartridge->updatePeekPokeLookupTables();
}

void
ExpansionPort::attachCartridge(Cartridge *c)
{
    assert(c != NULL);
    
    // Remove old cartridge (if any) and assign new one
    detachCartridge();
    cartridge = c;
    
    // Reset cartridge to update exrom and game line on the expansion port
    cartridge->reset();
    
    c64->putMessage(MSG_CARTRIDGE);
    if (cartridge->hasSwitch()) c64->putMessage(MSG_CART_SWITCH);
    
    debug(1, "Cartridge attached to expansion port");
    cartridge->dump();
}

bool
ExpansionPort::attachCartridgeAndReset(CRTFile *file)
{
    assert(file != NULL);
    
    Cartridge *cartridge = Cartridge::makeWithCRTFile(c64, file);
    
    if (cartridge) {
        
        suspend();
        attachCartridge(cartridge);
        c64->reset();
        resume();
        return true;
    }
    
    return false;
}

bool
ExpansionPort::attachGeoRamCartridge(uint32_t capacity)
{
    switch (capacity) {
        case 64: case 128: case 256: case 512: case 1024: case 2048: case 4096:
            break;
        default:
            warn("Cannot create GeoRAM cartridge of size %d\n", capacity);
            return false;
    }
    
    Cartridge *geoRAM = Cartridge::makeWithType(c64, CRT_GEO_RAM);
    uint32_t capacityInBytes = capacity * 1024;
    geoRAM->setRamCapacity(capacityInBytes);
    debug("Created GeoRAM cartridge (%d KB)\n", capacity);
    
    attachCartridge(geoRAM);
    return true;
}

void
ExpansionPort::attachIsepicCartridge()
{
    debug("Creating Isepic cartridge\n");
    
    Cartridge *isepic = Cartridge::makeWithType(c64, CRT_ISEPIC);
    return attachCartridge(isepic);
}

void
ExpansionPort::detachCartridge()
{
    if (cartridge) {
        
        suspend();
        
        delete cartridge;
        cartridge = NULL;
        
        setCartridgeMode(CRT_OFF);
        
        debug(1, "Cartridge detached from expansion port");
        c64->putMessage(MSG_NO_CARTRIDGE);
       
        resume();
    }
}

void
ExpansionPort::detachCartridgeAndReset()
{
    suspend();
    detachCartridge();
    c64->reset();
    resume();
}
