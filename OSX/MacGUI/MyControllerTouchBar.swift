//
// This file is part of VirtualC64 - A cycle accurate Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

@available(OSX 10.12.2, *)
extension NSTouchBarItem.Identifier {
    
    static let commodore  = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.commodore")
    static let runstop    = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.runstop")
    static let home       = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.home")
    static let del        = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.del")
    static let restore    = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.restore")
    static let rewind     = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.rewind")
    static let snap       = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.snap")
    static let revert     = NSTouchBarItem.Identifier("com.virtualc64.TouchBarItem.revert")
}

@available(OSX 10.12.2, *)
extension MyController : NSTouchBarDelegate
 {
    @objc func TouchBarHomeKeyAction() {
        if (modifierFlags.contains(NSEvent.ModifierFlags.shift)) {
            clearKeyAction(self)
        } else {
            homeKeyAction(self)
        }
    }

    @objc func TouchBarDelKeyAction() {
        if (modifierFlags.contains(NSEvent.ModifierFlags.shift)) {
            insertKeyAction(self)
        } else {
            deleteKeyAction(self)
        }
    }
    
    // NSTouchBarDelegate

    override open func makeTouchBar() -> NSTouchBar? {
 
        track()
        
        if (c64 == nil) {
            track("Cannot create touch bar (no C64 proxy).")
            return nil
        }

        let touchBar = NSTouchBar()
        touchBar.delegate = self

        // Configure items
        touchBar.defaultItemIdentifiers = [
            .commodore,
            .runstop,
            .home,
            .del,
            // .restore,
            .rewind,
            .snap,
            .revert
        ]
        
        // Make touchbar customizable
        touchBar.customizationIdentifier = NSTouchBar.CustomizationIdentifier("com.virtualc64.touchbar")
        touchBar.customizationAllowedItemIdentifiers = [
            .commodore,
            .runstop,
            .home,
            .del,
            .restore,
            .rewind,
            .snap,
            .revert
        ]
        
        return touchBar
    }
    
    public func touchBar(_ touchBar: NSTouchBar, makeItemForIdentifier identifier: NSTouchBarItem.Identifier) -> NSTouchBarItem? {
        
        switch identifier {
            
        case NSTouchBarItem.Identifier.commodore:
            let item = NSCustomTouchBarItem(identifier: identifier)
            item.customizationLabel = "Commodore key"
            item.view = NSButton(image: NSImage(named: NSImage.Name("commodore"))!,
                                 target: self,
                                 action: #selector(commodoreKeyAction))
            return item

        case NSTouchBarItem.Identifier.runstop:
            let item = NSCustomTouchBarItem(identifier: identifier)
            item.customizationLabel = "Runstop key"
            item.view = NSButton(image:  NSImage(named: NSImage.Name("runstop"))!,
                                 target: self,
                                 action: #selector(runstopAction))
            
            return item

        case NSTouchBarItem.Identifier.home:
            let item = NSCustomTouchBarItem(identifier: identifier)
            item.customizationLabel = "Home and Clear key"
            item.view = NSButton(image:  NSImage(named: NSImage.Name("home"))!,
                                 target: self,
                                 action: #selector(TouchBarHomeKeyAction))
            return item

        case NSTouchBarItem.Identifier.del:
            let item = NSCustomTouchBarItem(identifier: identifier)
            let icon = NSImage(named: NSImage.Name("del"))!
            // let resizedIcon = icon.resizeImage(width: 24, height: 24)
            item.customizationLabel = "Delete and Insert key"
            item.view = NSButton(image:  icon,
                                 target: self,
                                 action: #selector(TouchBarDelKeyAction))
            return item

        case NSTouchBarItem.Identifier.restore:
            let item = NSCustomTouchBarItem(identifier: identifier)
            let icon = NSImage(named: NSImage.Name("restore"))!
            // let resizedIcon = icon.resizeImage(width: 24, height: 24)
            item.customizationLabel = "Restore key"
            item.view = NSButton(image:  icon,
                                 target: self,
                                 action: #selector(restoreAction))
            return item
         
        case NSTouchBarItem.Identifier.rewind:
            let item = NSCustomTouchBarItem(identifier: identifier)
            let icon = NSImage(named: NSImage.Name("ttRewindTemplate"))!
            let resizedIcon = icon.resizeImage(width: 24, height: 24)
            item.customizationLabel = "Rewind"
            item.view = NSButton(image: resizedIcon,
                                 target: self,
                                 action: #selector(restoreLatestAutoSnapshotAction(_:)))
            return item
            
        case NSTouchBarItem.Identifier.snap:
            let item = NSCustomTouchBarItem(identifier: identifier)
            let icon = NSImage(named: NSImage.Name("ttStoreTemplate"))!
            let resizedIcon = icon.resizeImage(width: 24, height: 24)
            item.customizationLabel = "Snap"
            item.view = NSButton(image: resizedIcon,
                                 target: self,
                                 action: #selector(takeSnapshot(_:)))
            return item
        
        case NSTouchBarItem.Identifier.revert:
            let item = NSCustomTouchBarItem(identifier: identifier)
            let icon = NSImage(named: NSImage.Name("ttRestoreTemplate"))!
            let resizedIcon = icon.resizeImage(width: 24, height: 24)
            item.customizationLabel = "Revert"
            item.view = NSButton(image: resizedIcon,
                                 target: self,
                                 action: #selector(restoreLatestUserSnapshotAction(_:)))
            return item
            
        default:
            return nil
        }
    }
}

