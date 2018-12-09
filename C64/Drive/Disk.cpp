/*!
 * @file        Disk.cpp
 * @author      Dirk W. Hoffmann, www.dirkwhoffmann.de
 * @copyright   2015 - 2018 Dirk W. Hoffmann
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

#include "C64.h"

const Disk::TrackDefaults Disk::trackDefaults[43] = {
    
    { 0, 0, 0, 0, 0, 0 }, // Padding
    
    // Speedzone 3 (outer tracks)
    { 21, 3, 7693, 7693 * 8,   0, 0.268956 }, // Track 1
    { 21, 3, 7693, 7693 * 8,  21, 0.724382 }, // Track 2
    { 21, 3, 7693, 7693 * 8,  42, 0.177191 }, // Track 3
    { 21, 3, 7693, 7693 * 8,  63, 0.632698 }, // Track 4
    { 21, 3, 7693, 7693 * 8,  84, 0.088173 }, // Track 5
    { 21, 3, 7693, 7693 * 8, 105, 0.543583 }, // Track 6
    { 21, 3, 7693, 7693 * 8, 126, 0.996409 }, // Track 7
    { 21, 3, 7693, 7693 * 8, 147, 0.451883 }, // Track 8
    { 21, 3, 7693, 7693 * 8, 168, 0.907342 }, // Track 9
    { 21, 3, 7693, 7693 * 8, 289, 0.362768 }, // Track 10
    { 21, 3, 7693, 7693 * 8, 210, 0.815512 }, // Track 11
    { 21, 3, 7693, 7693 * 8, 231, 0.268338 }, // Track 12
    { 21, 3, 7693, 7693 * 8, 252, 0.723813 }, // Track 13
    { 21, 3, 7693, 7693 * 8, 273, 0.179288 }, // Track 14
    { 21, 3, 7693, 7693 * 8, 294, 0.634779 }, // Track 15
    { 21, 3, 7693, 7693 * 8, 315, 0.090253 }, // Track 16
    { 21, 3, 7693, 7693 * 8, 336, 0.545712 }, // Track 17
    
    // Speedzone 2
    { 19, 2, 7143, 7143 * 8, 357, 0.945418 }, // Track 18
    { 19, 2, 7143, 7143 * 8, 376, 0.506081 }, // Track 19
    { 19, 2, 7143, 7143 * 8, 395, 0.066622 }, // Track 20
    { 19, 2, 7143, 7143 * 8, 414, 0.627303 }, // Track 21
    { 19, 2, 7143, 7143 * 8, 433, 0.187862 }, // Track 22
    { 19, 2, 7143, 7143 * 8, 452, 0.748403 }, // Track 23
    { 19, 2, 7143, 7143 * 8, 471, 0.308962 }, // Track 24
    
    // Speedzone 1
    { 18, 1, 6667, 6667 * 8, 490, 0.116926 }, // Track 25
    { 18, 1, 6667, 6667 * 8, 508, 0.788086 }, // Track 26
    { 18, 1, 6667, 6667 * 8, 526, 0.459190 }, // Track 27
    { 18, 1, 6667, 6667 * 8, 544, 0.130238 }, // Track 28
    { 18, 1, 6667, 6667 * 8, 562, 0.801286 }, // Track 29
    { 18, 1, 6667, 6667 * 8, 580, 0.472353 }, // Track 30
    
    // Speedzone 0 (inner tracks)
    { 17, 0, 6250, 6250 * 8, 598, 0.834120 }, // Track 31
    { 17, 0, 6250, 6250 * 8, 615, 0.614880 }, // Track 32
    { 17, 0, 6250, 6250 * 8, 632, 0.395480 }, // Track 33
    { 17, 0, 6250, 6250 * 8, 649, 0.176140 }, // Track 34
    { 17, 0, 6250, 6250 * 8, 666, 0.956800 }, // Track 35
    
    // Speedzone 0 (usually unused tracks)
    { 17, 0, 6250, 6250 * 8, 683, 0.300 }, // Track 36
    { 17, 0, 6250, 6250 * 8, 700, 0.820 }, // Track 37
    { 17, 0, 6250, 6250 * 8, 717, 0.420 }, // Track 38
    { 17, 0, 6250, 6250 * 8, 734, 0.940 }, // Track 39
    { 17, 0, 6250, 6250 * 8, 751, 0.540 }, // Track 40
    { 17, 0, 6250, 6250 * 8, 768, 0.130 }, // Track 41
    { 17, 0, 6250, 6250 * 8, 785, 0.830 }  // Track 42
};


Disk::Disk()
{
    setDescription("Disk");
    
    // Register snapshot items
    SnapshotItem items[] = {        
        { &writeProtected,  sizeof(writeProtected), KEEP_ON_RESET },
        { &modified,        sizeof(modified),       KEEP_ON_RESET },
        { &data,            sizeof(data),           KEEP_ON_RESET },
        { &length,          sizeof(length),         KEEP_ON_RESET | WORD_ARRAY },
        { NULL,             0,                      0 }};
    
    registerSnapshotItems(items, sizeof(items));

    // Create bit expansion table
    // Note that this table expects a LITTLE ENDIAN architecture to work. If you compile
    // the emulator on a BIG ENDIAN architecture, the byte order needs to be reversed.
    for (unsigned i = 0; i < 256; i++) {
        bitExpansion[i] = 0;
        if (i & 0x80) bitExpansion[i] |= 0x0000000000000001;
        if (i & 0x40) bitExpansion[i] |= 0x0000000000000100;
        if (i & 0x20) bitExpansion[i] |= 0x0000000000010000;
        if (i & 0x10) bitExpansion[i] |= 0x0000000001000000;
        if (i & 0x08) bitExpansion[i] |= 0x0000000100000000;
        if (i & 0x04) bitExpansion[i] |= 0x0000010000000000;
        if (i & 0x02) bitExpansion[i] |= 0x0001000000000000;
        if (i & 0x01) bitExpansion[i] |= 0x0100000000000000;
    }
    
    clearDisk();
}

Disk::~Disk()
{
}

void
Disk::dump()
{
    msg("Floppy disk\n");
    msg("-----------\n\n");
    
    for (Halftrack ht = 1; ht <= maxNumberOfHalftracks; ht++) {
        uint16_t length = lengthOfHalftrack(ht);
        msg("Halftrack %2d: %d Bits (%d Bytes)\n", ht, length, length / 8);
    }
    msg("\n");
}

void
Disk::ping()
{
    VirtualComponent::ping();
}

void
Disk::setModified(bool b)
{
    if (b != modified) {
        modified = b;
        c64->drive1.ping();
        c64->drive2.ping();
    }
}


void
Disk::encodeGcr(uint8_t value, Track t, HeadPosition offset)
{
    assert(isTrackNumber(t));
    
    uint8_t nibble1 = bin2gcr(value >> 4);
    uint8_t nibble2 = bin2gcr(value & 0xF);
    
    writeBitToTrack(t, offset++, nibble1 & 0x10);
    writeBitToTrack(t, offset++, nibble1 & 0x08);
    writeBitToTrack(t, offset++, nibble1 & 0x04);
    writeBitToTrack(t, offset++, nibble1 & 0x02);
    writeBitToTrack(t, offset++, nibble1 & 0x01);

    writeBitToTrack(t, offset++, nibble2 & 0x10);
    writeBitToTrack(t, offset++, nibble2 & 0x08);
    writeBitToTrack(t, offset++, nibble2 & 0x04);
    writeBitToTrack(t, offset++, nibble2 & 0x02);
    writeBitToTrack(t, offset++, nibble2 & 0x01);
}

void
Disk::encodeGcr(uint8_t *values, size_t length, Track t, HeadPosition offset)
{
    for (size_t i = 0; i < length; i++, values++, offset += 10) {
        encodeGcr(*values, t, offset);
    }
}

void
Disk::encodeGcr(uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, Track t, unsigned offset)
{
    assert(isTrackNumber(t));
    
    uint8_t buffer[] = { b1, b2, b3, b4 };
    
    encodeGcr(buffer, 4, t, offset);
    /*
    uint64_t shift_reg = 0;
    
    // Shift in
    shift_reg = gcr[b1 >> 4];
    shift_reg = (shift_reg << 5) | gcr[b1 & 0x0F];
    shift_reg = (shift_reg << 5) | gcr[b2 >> 4];
    shift_reg = (shift_reg << 5) | gcr[b2 & 0x0F];
    shift_reg = (shift_reg << 5) | gcr[b3 >> 4];
    shift_reg = (shift_reg << 5) | gcr[b3 & 0x0F];
    shift_reg = (shift_reg << 5) | gcr[b4 >> 4];
    shift_reg = (shift_reg << 5) | gcr[b4 & 0x0F];
    
    // Shift out
    writeByteToTrack(t, offset + 4 * 8, shift_reg & 0xFF); shift_reg >>= 8;
    writeByteToTrack(t, offset + 3 * 8, shift_reg & 0xFF); shift_reg >>= 8;
    writeByteToTrack(t, offset + 2 * 8, shift_reg & 0xFF); shift_reg >>= 8;
    writeByteToTrack(t, offset + 1 * 8, shift_reg & 0xFF); shift_reg >>= 8;
    writeByteToTrack(t, offset, shift_reg & 0xFF);
    */
}

uint8_t
Disk::decodeGcrNibble(uint8_t *gcr)
{
    assert(gcr != NULL);
    
    uint5_t codeword = (gcr[0] << 4) | (gcr[1] << 3) | (gcr[2] << 2) | (gcr[3] << 1) | gcr[4];
    assert(is_uint5_t(codeword));
    
    return invgcr[codeword];
}

uint8_t
Disk::decodeGcr(uint8_t *gcr)
{
    assert(gcr != NULL);
    
    uint4_t nibble1 = decodeGcrNibble(gcr);
    uint4_t nibble2 = decodeGcrNibble(gcr + 5);

    // assert(is_uint4_t(nibble1));
    // assert(is_uint4_t(nibble2));
    
    return (nibble1 << 4) | nibble2;
}

uint64_t
Disk::_bitDelay(Halftrack ht, HeadPosition pos) {
    
    assert(isValidHeadPositon(ht, pos));

    // In the current implementation, we assume that the density bits were
    // set to their correct values when a bit was written to disk. According
    // to this assumption, the returned value is determined solely by the
    // track position of the drive head.
    
    if (ht <= 33)
        return 4 * 10000; // Density bits = 00: 4 * 16/16 * 10^4 1/10 nsec
    if (ht <= 47)
        return 4 * 9375;  // Density bits = 01: 4 * 15/16 * 10^4 1/10 nsec
    if (ht <= 59)
        return 4 * 8750;  // Density bits = 10: 4 * 14/16 * 10^4 1/10 nsec
    
     return 4 * 8125;     // Density bits = 11: 4 * 13/16 * 10^4 1/10 nsec
}

void
Disk::clearHalftrack(Halftrack ht)
{
    memset(&data.halftrack[ht], 0x55, sizeof(data.halftrack[ht]));
    length.halftrack[ht] = sizeof(data.halftrack[ht]) * 8;
}

void
Disk::clearDisk()
{
    // memset(&data, 0x55, sizeof(data));
    for (Halftrack ht = 1; ht <= maxNumberOfHalftracks; ht++) {
        // length.halftrack[ht] = sizeof(data.halftrack[ht]) * 8;
        clearHalftrack(ht);
    }
    writeProtected = false;
    modified = false; 
}

bool
Disk::halftrackIsEmpty(Halftrack ht)
{
    assert(isHalftrackNumber(ht));
    for (unsigned i = 0; i < sizeof(data.halftrack[ht]); i++)
        if (data.halftrack[ht][i] != 0x55) return false;
    return true;
}

bool
Disk::trackIsEmpty(Track t)
{
    assert(isTrackNumber(t));
    return halftrackIsEmpty(2 * t - 1);
}

unsigned
Disk::nonemptyHalftracks()
{
    unsigned result = 0;
    
    for (unsigned ht = 1; ht < 85; ht++) {
        if (!halftrackIsEmpty(ht))
            result++;
    }
    
    return result;
}


//
// Analyzing the disk
//

void
Disk::analyzeHalftrack(Halftrack ht)
{
    assert(isHalftrackNumber(ht));
    
    uint16_t len = length.halftrack[ht];

    errorLog.clear();
    errorStartIndex.clear();
    errorEndIndex.clear();
    
    // The result of the analysis is stored in variable trackInfo.
    memset(&trackInfo, 0, sizeof(trackInfo));
    trackInfo.length = len;
    
    // Setup working buffer (two copies of the track, each bit represented by one byte).
    for (unsigned i = 0; i < maxBytesOnTrack; i++)
        trackInfo.byte[i] = bitExpansion[data.halftrack[ht][i]];
    memcpy(trackInfo.bit + len, trackInfo.bit, len);
    
    // Indicates where the sector headers blocks and the sectors data blocks start.
    uint8_t sync[sizeof(trackInfo.bit)];
    memset(sync, 0, sizeof(sync));
    
    // Scan for SYNC sequences and decode the byte that follows.
    unsigned noOfOnes = 0;
    for (unsigned i = 0; i < 2 * len - 10; i++) {
        
        assert(trackInfo.bit[i] <= 1);
        if (trackInfo.bit[i] == 0 && noOfOnes >= 10) {
            
            // <--- SYNC ---><-- sync[i] -->
            // 11111 .... 1110
            //               ^ <- We are at offset i which is here
            sync[i] = decodeGcr(trackInfo.bit + i);
            
            if (sync[i] == 0x08) {
                debug(2, "Sector header block found at offset %d\n", i);
            } else if (sync[i] == 0x07) {
                debug(2, "Sector data block found at offset %d\n", i);
            } else {
                log(i, 10, "Invalid sector ID %02X at index %d. Should be 0x07 or 0x08.", sync[i], i);
            }
        }
        noOfOnes = trackInfo.bit[i] ? (noOfOnes + 1) : 0;
    }
    
    // Lookup first sector header block
    unsigned startOffset;
    for (startOffset = 0; startOffset < len; startOffset++) {
        if (sync[startOffset] == 0x08) {
            break;
        }
    }
    if (startOffset == len) {
        log(0, len, "Track contains no sector header block.");
        return;
    }
    
    // Compute offsets for all sectors
    uint8_t sector = UINT8_MAX;
    for (unsigned i = startOffset; i < startOffset + len; i++) {
        
        if (sync[i] == 0x08) {
            
            sector = decodeGcr(trackInfo.bit + i + 20);
            
            if (isSectorNumber(sector)) {
                if (trackInfo.sectorInfo[sector].headerEnd != 0)
                    break; // We've seen this sector already, so we are done.
                trackInfo.sectorInfo[sector].headerBegin = i;
                trackInfo.sectorInfo[sector].headerEnd = i + headerBlockSize;
            } else {
                log(i + 20, 10, "Header block at index %d contains an invalid sector number (%d).", i, sector);
            }
        
        } else if (sync[i] == 0x07) {
            
            if (isSectorNumber(sector)) {
                trackInfo.sectorInfo[sector].dataBegin = i;
                trackInfo.sectorInfo[sector].dataEnd = i + dataBlockSize;
            } else {
                log(i + 20, 10, "Data block at index %d contains an invalid sector number (%d).", i, sector);
            }
        }
    }
    
    // Check integrity of all sector blocks
    // For each sector ...
    Track t = (ht + 1) / 2;
    for (Sector s = 0; s < trackDefaults[t].sectors; s++) {
        
        SectorInfo *info = &trackInfo.sectorInfo[s];
        bool hasHeader = info->headerBegin != info->headerEnd;
        bool hasData = info->dataBegin != info->dataEnd;

        if (!hasHeader && !hasData) {
            log(0, 0, "Sector %d not found.\n", s);
            continue;
        }
        
        if (hasHeader) {
            analyzeSectorHeaderBlock(info->headerBegin);
        } else {
            log(0, 0, "Sector %d has no header block.\n", s);
        }
        
        if (hasData) {
            analyzeSectorDataBlock(info->dataBegin);
        } else {
            log(0, 0, "Sector %d has no data block.\n", s);
        }
    }
}

void
Disk::analyzeSectorHeaderBlock(size_t offset)
{
    // The first byte must be 0x08 (indicating a header block)
    assert(decodeGcr(trackInfo.bit + offset) == 0x08);
    offset += 10;
    
    uint8_t s = decodeGcr(trackInfo.bit + offset + 10);
    uint8_t t = decodeGcr(trackInfo.bit + offset + 20);
    uint8_t id2 = decodeGcr(trackInfo.bit + offset + 30);
    uint8_t id1 = decodeGcr(trackInfo.bit + offset + 40);
    uint8_t checksum = id1 ^ id2 ^ t ^ s;

    if (checksum != decodeGcr(trackInfo.bit + offset)) {
        log(offset, 10, "Header block at index %d contains an invalid checksum.\n", offset);
    }
}

void
Disk::analyzeSectorDataBlock(size_t offset)
{
    // The first byte must be 0x07 (indicating a header block)
    assert(decodeGcr(trackInfo.bit + offset) == 0x07);
    offset += 10;
    
    uint8_t checksum = 0;
    for (unsigned i = 0; i < 256; i++, offset += 10) {
        checksum ^= decodeGcr(trackInfo.bit + offset);
    }
    
    if (checksum != decodeGcr(trackInfo.bit + offset)) {
        log(offset, 10, "Data block at index %d contains an invalid checksum.\n", offset);
    }
}

void
Disk::log(size_t begin, size_t length, const char *fmt, ...)
{
    char buf[256];
    
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    errorLog.push_back(std::string(buf));
    errorStartIndex.push_back(begin);
    errorEndIndex.push_back(begin + length);
}

const char *
Disk::diskNameAsString()
{
    analyzeTrack(18);
    
    unsigned i;
    size_t offset = trackInfo.sectorInfo[0].dataBegin + (0x90 * 10);
    
    for (i = 0; i < 255; i++, offset += 10) {
        uint8_t value = decodeGcr(trackInfo.bit + offset);
        if (value == 0xA0)
            break;
        text[i] = value;
    }
    text[i] = 0;
    return text;
}


const char *
Disk::trackDataAsString()
{
    size_t i;
    for (i = 0; i < trackInfo.length; i++) {
        if (trackInfo.bit[i]) {
            text[i] = '1';
        } else {
            text[i] = '0';
        }
    }
    text[i] = 0;
    return text; 
}

const char *
Disk::sectorHeaderAsString(Sector nr)
{
    assert(isSectorNumber(nr));
    size_t begin = trackInfo.sectorInfo[nr].headerBegin;
    size_t end = trackInfo.sectorInfo[nr].headerEnd;
    return (begin == end) ? "" : sectorBytesAsString(trackInfo.bit + begin, 10);
}

const char *
Disk::sectorDataAsString(Sector nr)
{
    assert(isSectorNumber(nr));
    size_t begin = trackInfo.sectorInfo[nr].dataBegin;
    size_t end = trackInfo.sectorInfo[nr].dataEnd;
    return (begin == end) ? "" : sectorBytesAsString(trackInfo.bit + begin, 256);
}

const char *
Disk::sectorBytesAsString(uint8_t *buffer, size_t length)
{
    size_t gcr_offset = 0;
    size_t str_offset = 0;
    
    for (size_t i = 0; i < length; i++, gcr_offset += 10, str_offset += 3) {
        uint8_t value = decodeGcr(buffer + gcr_offset);
        sprint8x(text + str_offset, value);
        text[str_offset + 2] = ' ';
    }
    text[str_offset] = 0;
    return text;
}

//
// Decoding disk data
//

size_t
Disk::decodeDisk(uint8_t *dest)
{
    // Determine highest non-empty track
    Track t = 42;
    while (t > 0 && trackIsEmpty(t)) t--;
    
    // Decode disk 
    if (t <= 35)
        return decodeDisk(dest, 35);
        
    if (t <= 40)
        return decodeDisk(dest, 40);
    
    return decodeDisk(dest, 42);
}

size_t
Disk::decodeDisk(uint8_t *dest, unsigned numTracks)
{
    unsigned numBytes = 0;

    assert(numTracks == 35 || numTracks == 40 || numTracks == 42);

    // For each full track ...
   for (Track t = 1; t <= numTracks; t++) {
        
        if (trackIsEmpty(t))
            break;
        
        debug(2, "Decoding track %d %s\n", t, dest ? "" : "(test run)");
        numBytes += decodeTrack(t, dest + (dest ? numBytes : 0));
    }
    
    return numBytes;
}

size_t
Disk::decodeTrack(Track t, uint8_t *dest)
{
    unsigned numBytes = 0;
    
    // Gather sector information
    analyzeTrack(t);
    
    // For each sector ...
    for (unsigned s = 0; s < 21; s++) {
        
        debug(3, "   Decoding sector %d\n", s);
        SectorInfo info = sectorLayout(s);
        if (info.dataBegin != info.dataEnd) {
            numBytes += decodeSector(info.dataBegin, dest + (dest ? numBytes : 0));
        }
    }
    
    return numBytes;
}

size_t
Disk::decodeSector(size_t offset, uint8_t *dest)
{
    // The first byte must be 0x07 (indicating a data block)
    assert(decodeGcr(trackInfo.bit + offset) == 0x07);
    offset += 10;
    
    if (dest) {
        for (unsigned i = 0; i < 256; i++) {
            dest[i] = decodeGcr(trackInfo.bit + offset);
            offset += 10;
        }
    }
    
    return 256;
}

//
// Encoding disk data
//

void
Disk::encodeArchive(G64File *a)
{
    debug(2, "Encoding G64 archive\n");
    
    assert(a != NULL);
    
    clearDisk();
    for (Halftrack ht = 1; ht <= 84; ht++) {
        
        a->selectHalftrack(ht);
        uint16_t size = a->getSizeOfHalftrack();
        
        if (size == 0) {
            if (ht > 1) {
                // Make this halftrack as long as the previous halftrack
                length.halftrack[ht] = length.halftrack[ht - 1];
            }
            continue;
        }
        
        if (size > 7928) {
            warn("Halftrack %d has %d bytes. Must be less than 7928\n", ht, size);
            continue;
        }
        debug(2, "  Encoding halftrack %d (%d bytes)\n", ht, size);
        length.halftrack[ht] = 8 * size;
        
        for (unsigned i = 0; i < size; i++) {
            int b = a->readHalftrack();
            assert(b != -1);
            data.halftrack[ht][i] = (uint8_t)b;
        }
        assert(a->readHalftrack() == -1 /* EOF */);
    }
}

void
Disk::encodeArchive(D64File *a, bool alignTracks)
{
    assert(a != NULL);
    
    // 64COPY (fails on VICE test drive/skew)
    /*
    int tailGap[4] = { 9, 9, 9, 9 };
    uint16_t trackLength[4] =
    {
        6250 * 8, // Tracks 31 - 35..42 (inner tracks)
        6666 * 8, // Tracks 25 - 30
        7142 * 8, // Tracks 18 - 24
        7692 * 8  // Tracks  1 - 17     (outer tracks)
    };
    */
    
    // Hoxs64 (passes VICE test drive/skew)
    int tailGap[4] = { 9, 12, 17, 8 };
    uint16_t trackLength[4] =
    {
        6250 * 8, // Tracks 31 - 35..42 (inner tracks)
        6667 * 8, // Tracks 25 - 30
        7143 * 8, // Tracks 18 - 24
        7693 * 8  // Tracks  1 - 17     (outer tracks)
    };
    
    // VirtualC64 2.4
    /*
    const int tailGap[4] = { 13, 16, 21, 12 };
    const uint16_t trackLength[4] =
    {
        (uint16_t)(8 * 17 * (354 + tailGap[0])), // Tracks 31 - 35..42 (inner tracks)
        (uint16_t)(8 * 18 * (354 + tailGap[1])), // Tracks 25 - 30
        (uint16_t)(8 * 19 * (354 + tailGap[2])), // Tracks 18 - 24
        (uint16_t)(8 * 21 * (354 + tailGap[3]))  // Tracks  1 - 17     (outer tracks)
    };
    */
    
    size_t encodedBits;
    unsigned numTracks = a->numberOfTracks();

    debug(2, "Encoding D64 archive with %d tracks\n", numTracks);

    // Wipe out track data
    clearDisk();

    // Assign track length
     for (Halftrack ht = 1; ht <= maxNumberOfHalftracks; ht++)
         length.halftrack[ht] = trackLength[speedZoneOfHalftrack(ht)];
    
    // Encode tracks
    HeadPosition start;
    for (Track t = 1; t <= numTracks; t++) {
        
        unsigned zone = speedZoneOfTrack(t);
        if (alignTracks) {
            start = (HeadPosition)(length.track[t][0] * trackDefaults[t].stagger);
        } else {
            start = 0;
        }
        encodedBits = encodeTrack(a, t, tailGap[zone], start);
        debug(2, "Encoded %d bits (%d bytes) for track %d.\n",
              encodedBits, encodedBits / 8, t);
    }

    // Do some consistency checking
    for (Halftrack ht = 1; ht <= maxNumberOfHalftracks; ht++) {
        assert(length.halftrack[ht] <= sizeof(data.halftrack[ht]) * 8);
    }
}

size_t
Disk::encodeTrack(D64File *a, Track t, uint8_t tailGap, HeadPosition start)
{
    assert(isTrackNumber(t));
    debug(3, "Encoding track %d\n", t);

    size_t totalEncodedBits = 0;
    
    // For each sector in this track ...
    for (Sector s = 0; s < trackDefaults[t].sectors; s++) {
        
        size_t encodedBits = encodeSector(a, t, s, start, tailGap);
        start += (HeadPosition)encodedBits;
        totalEncodedBits += encodedBits;
    }
    
    return totalEncodedBits;
}

size_t
Disk::encodeSector(D64File *a, Track t, Sector s, HeadPosition start, int tailGap)
{
    assert(a != NULL);
    assert(isValidTrackSectorPair(t, s));
    
    HeadPosition offset = start;
    uint8_t errorCode = a->errorCode(t, s);
    
    a->selectTrackAndSector(t, s);
    
    debug(4, "  Encoding track/sector %d/%d\n", t, s);
    
    // Get disk id and compute checksum
    uint8_t id1 = a->diskId1();
    uint8_t id2 = a->diskId2();
    uint8_t checksum = id1 ^ id2 ^ t ^ s; // Header checksum byte
    
    // SYNC (0xFF 0xFF 0xFF 0xFF 0xFF)
    if (errorCode == 0x3) {
        writeBitToTrack(t, offset, 0, 40); // NO_SYNC SEQUENCE_ERROR
    } else {
        writeBitToTrack(t, offset, 1, 40);
    }
    offset += 40;
    
    // Header ID
    if (errorCode == 0x2) {
        encodeGcr(0x00, t, offset); // HEADER_BLOCK_NOT_FOUND_ERROR
    } else {
        encodeGcr(0x08, t, offset);
    }
    offset += 10;
 
    // Checksum
    if (errorCode == 0x9) {
        encodeGcr(checksum ^ 0xFF, t, offset); // HEADER_BLOCK_CHECKSUM_ERROR
    } else {
        encodeGcr(checksum, t, offset);
    }
    offset += 10;
    
    // Sector and track number
    encodeGcr(s, t, offset);
    offset += 10;
    encodeGcr(t, t, offset);
    offset += 10;
    
    // Disk ID (two bytes)
     if (errorCode == 0xB) {
         encodeGcr(id2 ^ 0xFF, t, offset); // DISK_ID_MISMATCH_ERROR
         offset += 10;
         encodeGcr(id1 ^ 0xFF, t, offset); // DISK_ID_MISMATCH_ERROR
     } else {
         encodeGcr(id2, t, offset);
         offset += 10;
         encodeGcr(id1, t, offset);
     }
    offset += 10;
    
    // 0x0F, 0x0F
    encodeGcr(0x0F, t, offset);
    offset += 10;
    encodeGcr(0x0F, t, offset);
    offset += 10;

    
    // 0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55 0x55
    writeGapToTrack(t, offset, 9);
    offset += 9 * 8;
    
    // SYNC (0xFF 0xFF 0xFF 0xFF 0xFF)
    if (errorCode == 3) {
        writeBitToTrack(t, offset, 0, 40); // NO_SYNC_SEQUENCE_ERROR
    } else {
        writeBitToTrack(t, offset, 1, 40);
    }
    offset += 40;
    
    // Data ID
    if (errorCode == 0x4) {
        // The error value is important here:
        // (1) If the first GCR bit equals 0, the sector can still be read.
        // (2) If the first GCR bit equals 1, the SYNC sequence continues.
        //     In this case, the bit sequence gets out of sync and the data
        //     can't be read.
        // Hoxs64 and VICE 3.2 write 0x00 which results in option (1)
        encodeGcr(0x00, t, offset); // DATA_BLOCK_NOT_FOUND_ERROR
    } else {
        encodeGcr(0x07, t, offset);
    }
    offset += 10;
    
    // Data bytes
    checksum = 0;
    for (unsigned i = 0; i < 256; i++, offset += 10) {
        uint8_t byte = (uint8_t)a->readTrack();
        checksum ^= byte;
        encodeGcr(byte, t, offset);
    }
    
    // Checksum
    if (errorCode == 0x5) {
        encodeGcr(checksum ^ 0xFF, t, offset); // DATA_BLOCK_CHECKSUM_ERROR
    } else {
        encodeGcr(checksum, t, offset);
    }
    offset += 10;
    
    // 0x00, 0x00
    encodeGcr(0x00, t, offset);
    offset += 10;
    encodeGcr(0x00, t, offset);
    offset += 10;
    
    // Tail gap (0x55 0x55 ... 0x55)
    writeGapToTrack(t, offset, tailGap);
    offset += tailGap * 8;
    
    // Return the number of encoded bits
    return offset - start;
}
