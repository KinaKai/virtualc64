/*!
 * @file        AnyArchive.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   Dirk W. Hoffmann. All rights reserved.
 */
/* This program is free software; you can redistribute it and/or modify
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

#include "T64File.h"
#include "D64File.h"
#include "PRGFile.h"
#include "P00File.h"
#include "G64File.h"

AnyArchive *
AnyArchive::makeWithFile(const char *path)
{
    assert(path != NULL);
    
    if (T64File::isT64File(path)) {
        return T64File::makeWithFile(path);
    }
    if (D64File::isD64File(path)) {
        return D64File::makeWithFile(path);
    }
    if (PRGFile::isPRGFile(path)) {
        return PRGFile::makeWithFile(path);
    }
    if (P00File::isP00File(path)) {
        return P00File::makeWithFile(path);
    }
    if (G64File::isG64File(path)) {
        return G64File::makeWithFile(path);
    }
    return NULL;
}

const unsigned short *
AnyArchive::getUnicodeNameOfItem()
{
    translateToUnicode(getNameOfItem(), unicode, 0xE000, sizeof(unicode) / 2);
    return unicode;
}

size_t
AnyArchive::getSizeOfItem()
{
    int size = 0;
    
    seekItem(0);
    while (readItem() != EOF)
        size++;

    seekItem(0);
    return size;
}

int
AnyArchive::readItem()
{
    int result;
    
    assert(iEof <= size);
    
    if (iFp < 0)
        return -1;
    
    // Get byte
    result = data[iFp++];
    
    // Check for end of file
    if (iFp == iEof)
        iFp = -1;
    
    return result;
}

const char *
AnyArchive::readItemHex(size_t num)
{
    assert(sizeof(name) > 3 * num);
    
    for (unsigned i = 0; i < num; i++) {
        
        int byte = readItem();
        if (byte == EOF) break;
        sprintf(name + (3 * i), "%02X ", byte);
    }
    
    return name;
}

void
AnyArchive::flashItem(uint8_t *buffer)
{
    int byte;
    assert(buffer != NULL);
    
    size_t offset = getDestinationAddrOfItem();
    
    seekItem(0);
    
    while ((byte = readItem()) != EOF) {
        if (offset <= 0xFFFF) {
            buffer[offset++] = (uint8_t)byte;
        } else {
            break;
        }
    }
}

void
AnyArchive::dumpDirectory()
{
    int numItems = numberOfItems();
    
    msg("Archive:           %s\n", getName());
    msg("-------\n");
    msg("  Path:            %s\n", getPath());
    msg("  Items:           %d\n", numItems);

    for (unsigned i = 0; i < numItems; i++) {
        
        selectItem(i);
        msg("  Item %2d:      %s (%d bytes, load address: %d)\n",
                i, getNameOfItem(), getSizeOfItem(), getDestinationAddrOfItem());
        msg("                 ");
        selectItem(i);
        for (unsigned j = 0; j < 8; j++) {
            int byte = readItem();
            if (byte != -1)
                msg("%02X ", byte);
        }
        msg("\n");
    }
}
