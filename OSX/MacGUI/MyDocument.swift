/*
 * (C) 2018 Dirk W. Hoffmann. All rights reserved.
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

import Foundation

class MyDocument : NSDocument {
    
    /**
     Emulator proxy object. This object is an Objective-C bridge between
     the GUI (written in Swift) an the core emulator (written in C++).
     */
    var c64: C64Proxy!
    
    /**
     When the GUI receives the READY_TO_RUN message from the emulator, it
     checks this variable. If an attachment is present, e.g., a T64 archive,
     is displays a user dialog. The user can then choose to mount the archive
     as a disk or to flash a single file into memory. If the attachment is a
     snapshot, it is read into the emulator without asking the user.
     */
    var attachment: ContainerProxy? = nil
    
    /// The list of recent-disk URLs.
    var recentDiskURLs: [URL] = []
    
    /// The maximum number of items stored in recentDiskURLs
    var maximumRecentDiskCount = 10

    
    override init() {
        
        track()
        super.init()
        
        // Register standard user defaults
        MyController.registerUserDefaults()
        
        // Create emulator instance
        c64 = C64Proxy()
        
        // Try to load ROMs
        let defaults = UserDefaults.standard
        loadRom(defaults.url(forKey: VC64Keys.basicRom))
        loadRom(defaults.url(forKey: VC64Keys.charRom))
        loadRom(defaults.url(forKey: VC64Keys.kernalRom))
        loadRom(defaults.url(forKey: VC64Keys.vc1541Rom))
        
        // Try to run. The emulator will either run (if all ROMs were found)
        // or writes a MISSING_ROM message into the message queue.
        c64.run()
    }
 
    override open func makeWindowControllers() {
        
        track()
        
        let nibName = NSNib.Name(rawValue: "MyDocument")
        let controller = MyController.init(windowNibName: nibName)
        controller.c64 = c64
        self.addWindowController(controller)
    }
    
    //
    // Handling the list of recently used files
    //
    
    func noteNewRecentDiskURL(url: URL) {
        
        if !recentDiskURLs.contains(url) {
            if recentDiskURLs.count == maximumRecentDiskCount {
                recentDiskURLs.remove(at: maximumRecentDiskCount - 1)
            }
            recentDiskURLs.insert(url, at: 0)
        }
    }
    
    
    //
    // Loading
    //
    
    /// Creates an attachment from a URL
    func createAttachment(from url: URL) throws {
    
        track("Trying to create attachment for file \(url.lastPathComponent).")

        // Try to create the attachment
        let fileWrapper = try FileWrapper.init(url: url)
        try createAttachment(from: fileWrapper, ofType: url.pathExtension)
        
        // Add URL to list of recently used files
        switch (url.pathExtension.uppercased()) {
        case "T64", "PRG", "D64", "P00", "G64", "NIB":
            noteNewRecentDiskURL(url: url)
        default:
            break
        }
    }
    
    /// Creates an attachment from a file wrapper
    func createAttachment(from fileWrapper: FileWrapper, ofType typeName: String) throws {
        
        guard let filename = fileWrapper.filename else {
            throw NSError(domain: "VirtualC64", code: 0, userInfo: nil)
        }
        guard let data = fileWrapper.regularFileContents else {
            throw NSError(domain: "VirtualC64", code: 0, userInfo: nil)
        }
        
        let buffer = (data as NSData).bytes
        let length = data.count

        track("Reading \(length) bytes for file \(filename) at \(buffer).")
        
        switch (typeName.uppercased()) {
        case "VC64":
            if SnapshotProxy.isUnsupportedSnapshot(buffer, length: length) {
                throw NSError.snapshotVersionError(filename: filename)
            }
            attachment = SnapshotProxy.make(withBuffer: buffer, length: length)
            break
        case "CRT":
            if CRTProxy.isUnsupportedCRTBuffer(buffer, length: length) {
                let type = CRTProxy.typeName(ofCRTBuffer: buffer, length: length)!
                throw NSError.unsupportedCartridgeError(filename: filename, type: type)
            }
            attachment = CRTProxy.make(withBuffer: buffer, length: length)
            break
        case "TAP":
            attachment = TAPProxy.make(withBuffer: buffer, length: length)
            break
        case "T64":
            attachment = T64Proxy.make(withBuffer: buffer, length: length)
            break
        case "PRG":
            attachment = PRGProxy.make(withBuffer: buffer, length: length)
            break
        case "D64":
            attachment = D64Proxy.make(withBuffer: buffer, length: length)
            break
        case "P00":
            attachment = P00Proxy.make(withBuffer: buffer, length: length)
            break
        case "G64":
            attachment = G64Proxy.make(withBuffer: buffer, length: length)
            break
        case "NIB":
            attachment = NIBProxy.make(withBuffer: buffer, length: length)
            break
        default:
            throw NSError.unsupportedFormatError(filename: filename)
        }
        
        if attachment == nil {
            throw NSError.corruptedFileError(filename: filename)
        }
    }
    
    /**
     Reads in the document's attachment. Snapshots will be flashed, cartridges
     attached and archives mounted as disk. Depending on the attachment type,
     user dialogs show up.
     
     - parameter warnAboutUnsafedDisk: Asks the user for permission to proceed,
     if the currently inserted disk contains unsaved data.
     
     - parameter showMountDialog: Pops up the mount dialog if the attachment
     contains an archive that can be mounted as a disk.
     */
    func readFromAttachment(warnAboutUnsafedDisk: Bool, showMountDialog: Bool) {
        
        if attachment == nil { return }
        
        let parent = windowForSheet!.windowController as! MyController
        
        switch attachment!.type() {
            
        case V64_CONTAINER:
            if !warnAboutUnsafedDisk || proceedWithUnsavedDisk() {
                c64.load(fromSnapshot: attachment as! SnapshotProxy)
            }
            return
            
        case CRT_CONTAINER:
            let nibName = NSNib.Name(rawValue: "CartridgeMountDialog")
            let controller = CartridgeMountController.init(windowNibName: nibName)
            controller.showSheet(withParent: parent)
            return
            
        case TAP_CONTAINER:
            let nibName = NSNib.Name(rawValue: "TapeMountDialog")
            let controller = TapeMountController.init(windowNibName: nibName)
            controller.showSheet(withParent: parent)
            return
            
        case T64_CONTAINER, PRG_CONTAINER, P00_CONTAINER, D64_CONTAINER:
            
            if !warnAboutUnsafedDisk || proceedWithUnsavedDisk() {
                if !showMountDialog {
                    c64.insertDisk(attachment as! ArchiveProxy)
                } else {
                    let nibName = NSNib.Name(rawValue: "ArchiveMountDialog")
                    let controller = ArchiveMountController.init(windowNibName: nibName)
                    controller.showSheet(withParent: parent)
                }
            }
            return
            
        case G64_CONTAINER, NIB_CONTAINER:
            
            if !warnAboutUnsafedDisk || proceedWithUnsavedDisk() {
                if !showMountDialog {
                    c64.insertDisk(attachment as! ArchiveProxy)
                } else {
                    let nibName = NSNib.Name(rawValue: "DiskMountDialog")
                    let controller = DiskMountController.init(windowNibName: nibName)
                    controller.showSheet(withParent: parent)
                }
            }
            return
            
        default:
            track("Unknown attachment type")
            fatalError()
        }
    }
    
    override open func read(from url: URL, ofType typeName: String) throws {
        
        try createAttachment(from: url)
    }
    
    /// Loads a ROM image file into the emulator and stores the URL in the
    /// the user defaults.
    @discardableResult
    func loadRom(_ url: URL?) -> Bool {
        
        if (url == nil) {
            return false
        }
        
        let defaults = UserDefaults.standard
        
        if c64.loadBasicRom(url!) {
            track("Basic ROM:  \(url!)")
            defaults.set(url, forKey: VC64Keys.basicRom)
            return true
        }
        if c64.loadCharRom(url!) {
            track("Char ROM:   \(url!)")
            defaults.set(url, forKey: VC64Keys.charRom)
            return true
        }
        if c64.loadKernalRom(url!) {
            track("Kernal ROM: \(url!)")
            defaults.set(url, forKey: VC64Keys.kernalRom)
            return true
        }
        if c64.loadVC1541Rom(url!) {
            track("VC1541 ROM: \(url!)")
            defaults.set(url, forKey: VC64Keys.vc1541Rom)
            return true
        }
        
        track("ROM file \(url!) not found")
        return false
    }

    /// Restores the emulator state from a snapshot.
    @discardableResult
    func loadSnapshot(_ snapshot: SnapshotProxy) -> Bool {
                
        if proceedWithUnsavedDisk() {
            c64.load(fromSnapshot: snapshot)
            return true
        } else {
            return false
        }
    }
    
    
    //
    // Saving
    //
    
    override open func data(ofType typeName: String) throws -> Data {
        
        track("Trying to write \(typeName) file.")
        
        if typeName == "VC64" {

            NSLog("Type is VC64")
            
            // Take snapshot
            if let snapshot = SnapshotProxy.make(withC64: c64) {

                // Write to data buffer
                if let data = NSMutableData.init(length: snapshot.sizeOnDisk()) {
                    snapshot.write(toBuffer: data.mutableBytes)
                    return data as Data
                }
            }
        }
        
        throw NSError(domain: NSOSStatusErrorDomain, code: unimpErr, userInfo: nil)
    }
    
    open override func removeWindowController(_ windowController: NSWindowController) {
        
        NSLog("MyDocument:\(#function)")

        super.removeWindowController(windowController)
        
        // Shut down the emulator.
        // Note that all GUI elements need to be inactive when we set the proxy
        // to nil. Hence, the emulator should be shut down as late as possible.
        c64.kill()
    }
    
}

