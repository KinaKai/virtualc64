// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

#pragma once

#include "C64Object.h"
#include "FileTypes.h"
#include "PETName.h"

/* All media files are organized in the class hierarchy displayed below. Two
 * abstract classes are involed: AnyFile and AnyCollection. AnyFiles provided
 * basic functionality for reading and writing files, streams, and buffers.
 * AnyCollection provides an abstract interface for accessing single files.
 * This interface is implemented by those file classes that provide easy
 * access to single files.
 *
 *     ---------
 *    | AnyFile |
 *     ---------
 *         |
 *         |------------------------------------------------------------
 *         |         |           |            |            |            |
 *         |     ---------   ---------    ---------    ---------    ---------
 *         |    | ROMFile | |Snapshot |  | TAPFile |  | CRTFile |  | G64File |
 *         |     ---------   ---------    ---------    ---------    ---------
 *         |
 *  ---------------
 * | AnyCollection |
 *  ---------------
 *         |
 *         |-----------------------------------------------
 *                   |           |            |            |
 *               ---------   ---------    ---------    ---------
 *              | D64File | | T64File |  | PRGFile |  | P00File |
 *               ---------   ---------    ---------    ---------
 */
  
class AnyFile : public C64Object {
    
public:
	     
    // Physical location of this file
    string path = "";
    
    // The raw data of this file
    u8 *data = nullptr;
    
    // The size of this file in bytes
    usize size = 0;
    

    //
    // Generating
    //
    
public:
    
    template <class T> static T *make(std::istream &stream)
    {
        if (!T::isCompatibleStream(stream)) throw Error(ERROR_INVALID_TYPE);
        
        T *obj = new T();
        
        try { obj->readFromStream(stream); } catch (Error &err) {
            delete obj;
            throw err;
        }
        return obj;
    }

    template <class T> static T *make(const u8 *buf, usize len)
    {
        std::stringstream stream;
        stream.write((const char *)buf, len);
        return make <T> (stream);
    }
    
    template <class T> static T *make(const char *path)
    {
        std::ifstream stream(path);
        if (!stream.is_open()) throw Error(ERROR_FILE_NOT_FOUND);
        return make <T> (stream);
    }

    
    //
    // Initializing
    //
    
public:
    
    AnyFile() { };
    AnyFile(usize capacity);
    virtual ~AnyFile();

    
    //
    // Accessing
    //
    
public:
    
    // Returns the logical name of this file
    virtual PETName<16> getName();

    // Returns the media type of this file
    virtual FileType type() { return FILETYPE_UNKNOWN; }
     
    // Returns a unique fingerprint
    u64 fnv();
    
    
    //
    // Flashing data
    //

    // Copies the file contents into a buffer starting at the provided offset
    void flash(u8 *buffer, usize offset = 0);

    
    //
    // Serializing
    //
    
protected:

    usize readFromFile(const char *path) throws;
    usize readFromBuffer(const u8 *buf, usize len) throws;
    virtual usize readFromStream(std::istream &stream) throws;

public:
    
    usize writeToFile(const char *path) throws;
    usize writeToBuffer(u8 *buf) throws;
    virtual usize writeToStream(std::ostream &stream) throws;
    

    //
    // Repairing
    //
    
public:
    
    /* This function is called in the default implementation of readFromStream.
     * It is overwritten by some subclasses to fix known inconsistencies in
     * certain media files.
     */
    virtual void repair() { };    
};
