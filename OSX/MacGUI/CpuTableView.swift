//
// This file is part of VirtualC64 - A cycle accurate Commodore 64 emulator
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v3
//
// See https://www.gnu.org for license information
//

import Foundation

class CpuTableView : NSTableView {
    
    var c : MyController? = nil
    
    var instructionAtRow : [Int:DisassembledInstruction] = [:]
    var rowForAddress : [UInt16:Int] = [:]
    var hex = true
    
    override func awakeFromNib() {
        
        delegate = self
        dataSource = self
        target = self
        doubleAction = #selector(doubleClickAction(_:))
    }
    
    @IBAction func doubleClickAction(_ sender: Any!) {
    
        let sender = sender as! NSTableView
        let row = sender.selectedRow
        
        if let instr = instructionAtRow[row] {
            track("Toggling breakpoint at \(instr.addr)")
            c?.c64.cpu.toggleBreakpoint(instr.addr)
            reloadData()
        }
    }

    func setHex(_ value: Bool) {
        
        hex = value
        updateDisplayedAddresses()
    }
    
    func updateDisplayedAddresses(startAddr: UInt16) {

        if c == nil { return }
        
        var addr = Int(startAddr)
        rowForAddress = [:]
        
        for i in 0...255 {
            if (addr <= 0xFFFF) {
                instructionAtRow[i] = c!.c64.cpu.disassemble(UInt16(addr), hex: hex)
                rowForAddress[UInt16(addr)] = i
                addr += Int(instructionAtRow[i]!.size)
            } else {
                instructionAtRow[i] = nil;
            }
        }
        
        reloadData()
    }

    func updateDisplayedAddresses() {
        
        if c == nil { return }
        
        updateDisplayedAddresses(startAddr: c!.c64.cpu.pc())
    }
    
    func refresh() {
    
        if c == nil { return }
        
        if let row = rowForAddress[c!.c64.cpu.pc()] {
            
            // If PC points to an address which is already displayed,
            // we simply select the corresponding row.
            scrollRowToVisible(row)
            selectRowIndexes([row], byExtendingSelection: false)
            
        } else {
            
            // If PC points to an address that is not displayed,
            // we update the whole view and display PC in the first row.
            updateDisplayedAddresses()
            scrollRowToVisible(0)
            selectRowIndexes([0], byExtendingSelection: false)
        }
    }
}

extension CpuTableView : NSTableViewDataSource {
   
    func numberOfRows(in tableView: NSTableView) -> Int {
        return 256;
    }
        
    func tableView(_ tableView: NSTableView, objectValueFor tableColumn: NSTableColumn?, row: Int) -> Any? {
        
        if var instr = instructionAtRow[row] {
            
            switch(tableColumn?.identifier.rawValue) {

            case "break":
                if (c?.c64.cpu.breakpoint(instr.addr))! {
                    return "⛔"
                } else {
                    return " "
                }
            case "addr":
                return String.init(utf8String:&instr.pc.0)
            case "data01":
                return String.init(utf8String:&instr.byte1.0)
            case "data02":
                return String.init(utf8String:&instr.byte2.0)
            case "data03":
                return String.init(utf8String:&instr.byte3.0)
            case "ascii":
                return String.init(utf8String:&instr.command.0)
            default:
                return "?"
            }
        }
        return ""
    }
}

extension CpuTableView : NSTableViewDelegate {
    
    func tableView(_ tableView: NSTableView, willDisplayCell cell: Any, for tableColumn: NSTableColumn?, row: Int) {
        
        let cell = cell as! NSTextFieldCell
        
        if  let instr = instructionAtRow[row] {
            
            if (c?.c64.cpu.breakpoint(instr.addr))! {
                cell.textColor = NSColor.systemRed
            } else {
                cell.textColor = NSColor.textColor
            }
        }
    }
}
