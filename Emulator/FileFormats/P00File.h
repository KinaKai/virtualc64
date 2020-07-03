/*!
 * @header      P00File.h
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

#ifndef _P00FILE_INC
#define _P00FILE_INC

#include "AnyArchive.h"

/*! @class   P00File
 *  @brief   The P00File class declares the programmatic interface for a file
 *           in P00 format.
 */
class P00File : public AnyArchive {

private:

    //! @brief    Header signature
    static const uint8_t magicBytes[];
    
public:

    //
    //! @functiongroup Class methods
    //
    
    //! @brief    Returns true iff buffer contains a P00 file.
    static bool isP00Buffer(const uint8_t *buffer, size_t length);
    
    //! @brief    Returns true iff the specified file is a P00 file.
    static bool isP00File(const char *filename);
    
    
    //
    //! @functiongroup Creating and destructing
    //
    
    //! @brief    Standard constructor
    P00File();
    
    //! @brief    Factory method
    static P00File *makeWithBuffer(const uint8_t *buffer, size_t length);
    
    //! @brief    Factory method
    static P00File *makeWithFile(const char *path);
    
    /*! @brief    Factory method
     *  @details  otherArchive can be of any archive type
     */
    static P00File *makeWithAnyArchive(AnyArchive *otherArchive);
    
    
    //
    //! @functiongroup Methods from AnyFile
    //
    
    const char *getName();
    C64FileType type() { return P00_FILE; }
    const char *typeAsString() { return "P00"; }
    bool hasSameType(const char *filename) { return isP00File(filename); }
    
    
    //
    //! @functiongroup Methods from AnyArchive
    //
    
    int numberOfItems() { return 1; }
    void selectItem(unsigned item);
    const char *getTypeOfItemAsString() { return "PRG"; }
    const char *getNameOfItem();
    size_t getSizeOfItem() { return size - 0x1C; }
    void seekItem(long offset);
    uint16_t getDestinationAddrOfItem();
};
#endif
