//
// This file is part of VirtualC64 - A cycle accurate Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

class ExportDiskController : UserDialogController {

    @IBOutlet weak var button: NSPopUpButton!
    var type: C64FileType = D64_FILE
    var savePanel: NSSavePanel!
    var selectedURL: URL?
    
    func showSheet(forDrive nr: Int) {
        
        precondition(nr == 1 || nr == 2)
        
        // Create save panel
        savePanel = NSSavePanel()
        savePanel.allowedFileTypes = ["d64"]
        savePanel.prompt = "Export"
        savePanel.title = "Export"
        savePanel.nameFieldLabel = "Export As:"
        savePanel.accessoryView = window?.contentView

        // Run panel as sheet
        if let win = myWindow {
            savePanel.beginSheetModal(for: win, completionHandler: { result in
                if result == .OK {
                    myDocument?.export(drive: nr, to: self.savePanel.url)
                }
            })
        }
    }
    
    @IBAction func selectD64(_ sender: Any!) {
        track()
        savePanel.allowedFileTypes = ["d64"]
        type = D64_FILE
    }

    @IBAction func selectG64(_ sender: Any!) {
        track()
        savePanel.allowedFileTypes = ["g64"]
        type = G64_FILE
    }
    
    @IBAction func selectPRG(_ sender: Any!) {
        track()
        savePanel.allowedFileTypes = ["prg"]
        type = PRG_FILE
    }
    
    @IBAction func selectP00(_ sender: Any!) {
        track()
        savePanel.allowedFileTypes = ["p00"]
        type = P00_FILE
    }
    
    @IBAction func selectT64(_ sender: Any!) {
        track()
        savePanel.allowedFileTypes = ["t64"]
        type = T64_FILE
    }
}
