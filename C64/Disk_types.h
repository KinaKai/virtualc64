//
//  Disk_types.h
//  V64
//
//  Created by Dirk Hoffmann on 04.06.18.
//

#ifndef DISK_TYPES_H
#define DISK_TYPES_H

/*! @brief    Maximum number of files that can be stored on a single disk
 *  @details  VC1541 DOS stores the directors on track 18 which contains 19 sectors.
 *            Sector 0 is reserved for the BAM. Each of the remaining sectors can
 *            hold up to 8 directory entries, summing um to a total of 144 items.
 */
static const unsigned MAX_FILES_ON_DISK = 144;

/*! @brief    Maximum number of bits stored on a single track
 *  @details  Each track can store a maximum of 7928 bytes. The exact number depends on
 *            the track number (inner tracks contain fewer bytes) and the actual write
 *            speed of a drive.
 */
static const unsigned maxBitsOnTrack = 7928 * 8;

/*
 *
 *                       -----------------------------------------------------------------
 * Track layout:         |  1  | 1.5 |  2  | 2.5 | ... |  35  | 35.5 | ... |  42  | 42.5 |
 *                       -----------------------------------------------------------------
 * Halftrack addressing: |  1  |  2  |  3  |  4  |     |  69  |  70  |     |  83  |  84  |
 * Track addressing:     |  1  |     |  2  |     |     |  35  |      |     |  42  |      |
 *                       -----------------------------------------------------------------
 */

/*! @brief    Data type for addressing half and full tracks on disk
 *  @details  The VC1541 drive head can move between position 1 and 85.
 *            The odd numbers between 1 and 70 mark the 35 tracks that
 *            are used by VC1541 DOS. This means that DOS moves the
 *            drive head always two positions up or down. If programmed
 *            manually, the head can also be position on half tracks
 *            and on tracks beyond 35.
 *  @see      Track
 */
typedef unsigned Halftrack;

/*! @brief    Data type for addressing full tracks on disk
 *  @see      Halftrack
 */
typedef unsigned Track;

/*! @brief    Checks if a given number is a valid halftrack number
 */
inline bool isHalftrackNumber(unsigned nr) { return 1 <= nr && nr <= 84; }

/*! @brief    Checks if a given number is a valid track number
 */
inline bool isTrackNumber(unsigned nr) { return 1 <= nr && nr <= 42; }

//! @brief    Layout information of a single sector
typedef struct {
    int headerBegin;
    int headerEnd;
    int dataBegin;
    int dataEnd;
} SectorInfo;

//! @brief    Information about a single track as gathered by analyzeTrack()
typedef struct {
    uint16_t length; 
    uint8_t data[2 * maxBitsOnTrack]; // Two copies in a row to allow fast wrap arounds
    SectorInfo sectorInfo[22];
} TrackInfo;


#endif
