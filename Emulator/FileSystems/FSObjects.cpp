// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#include "FSObjects.h"

FSName::FSName(const u8 *_pet)
{
    assert(_pet);
    
    memset(pet, 0xA0, sizeof(pet));
    memset(str, 0x00, sizeof(str));
    
    for (int i = 0; i < 16 && _pet[i] != 0xA0; i++) {
        
        str[i] = _pet[i];
        pet[i] = petscii2printable(_pet[i], '_');
    }    
}

bool
FSName::operator== (FSName &rhs)
{
    int i = 0;
    
    while (pet[i] != 0xA0 || rhs.pet[i] != 0xA0) {
        if (pet[i] != rhs.pet[i]) return false;
        i++;
    }
    return true;
}

void
FSName::write(u8 *p)
{
    assert(p);
    for (int i = 0; i < 16; i++) p[i] = pet[i];
}