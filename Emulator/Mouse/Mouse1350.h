/*!
 * @header      Mouse1350.h
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

#ifndef MOUSE1350_H
#define MOUSE1350_H

#include "VirtualComponent.h"

class Mouse1350 : public VirtualComponent {
    
private:
    
    //! @brief    Mouse position
    int64_t mouseX;
    int64_t mouseY;
    
    //! @brief    Mouse button states
    bool leftButton;
    bool rightButton;
    
    //! @brief    Dividers applied to raw coordinates in setXY()
    int dividerX = 64;
    int dividerY = 64;
    
    //! @brief    Latched mouse positions
    int64_t latchedX[3];
    int64_t latchedY[3];
    
    //! @brief    Control port bits
    uint8_t controlPort;
    
public:
    
    //! @brief   Constructor
    Mouse1350();
    
    //! @brief   Destructor
    ~Mouse1350();
    
    //! @brief   Methods from VirtualComponent class
    void reset();
        
    //! @brief   Updates the button state
    void setLeftMouseButton(bool value) { leftButton = value; }
    void setRightMouseButton(bool value) { rightButton = value; }

    //! @brief   Returns the pot X bits as set by the mouse
    uint8_t readPotX();
    
    //! @brief   Returns the pot Y bits as set by the mouse
    uint8_t readPotY();
    
    //! @brief   Returns the control port bits triggered by the mouse
    uint8_t readControlPort();
    
    /*! @brief   Execution function
     *  @details Translates movement deltas into joystick events.
     */
    void execute(int64_t targetX, int64_t targetY);
};

#endif
