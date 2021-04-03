// -----------------------------------------------------------------------------
// This file is part of VirtualC64
//
// Copyright (C) Dirk W. Hoffmann. www.dirkwhoffmann.de
// Licensed under the GNU General Public License v2
//
// See https://www.gnu.org for license information
// -----------------------------------------------------------------------------

class SplashScreen: Layer {
    
    var bgTexture: MTLTexture! = nil
    var bgRect: Node?
        
    var vertUniforms = VertexUniforms(mvp: matrix_identity_float4x4)
    var fragUniforms = FragmentUniforms(alpha: 1.0,
                                        white: 0.0,
                                        dotMaskWidth: 0,
                                        dotMaskHeight: 0,
                                        scanlineDistance: 0)
    
    override init(renderer: Renderer) {

        super.init(renderer: renderer)

        alpha.set(1.0)
        let img = NSImage.init(named: "background")!
        bgTexture = img.toTexture(device: device, vflip: false)
        
        renderer.metalAssert(bgTexture != nil,
                             "The background texture could not be allocated.")
    }
    
    func buildVertexBuffers() {
    
        bgRect = Node.init(device: device,
                           x: -1.0, y: -1.0, z: 0.99, w: 2.0, h: 2.0,
                           t: NSRect.init(x: 0.0, y: 0.0, width: 1.0, height: 1.0))
    }
    
    func render(encoder: MTLRenderCommandEncoder) {
        
        // Configure vertex shader
        encoder.setVertexBytes(&vertUniforms,
                               length: MemoryLayout<VertexUniforms>.stride,
                               index: 1)
        
        // Configure fragment shader
        encoder.setFragmentTexture(bgTexture, index: 0)
        encoder.setFragmentTexture(bgTexture, index: 1)
        encoder.setFragmentBytes(&fragUniforms,
                                 length: MemoryLayout<FragmentUniforms>.stride,
                                 index: 1)
        
        // Render
        bgRect!.drawPrimitives(encoder)
    }
}
