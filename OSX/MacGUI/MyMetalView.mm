/*
 * Author: Dirk W. Hoffmann, 2015
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

#import "C64GUI.h"
#import "MyMetalDefs.h"
#import "VirtualC64-Swift.h"

@implementation MyMetalView {
    
    // Local metal objects
    id <MTLCommandBuffer> _commandBuffer;
    id <MTLRenderCommandEncoder> _commandEncoder;
    id <CAMetalDrawable> _drawable;
    
    // All currently supported texture upscalers
    ComputeKernel *bypassUpscaler;
    ComputeKernel *epxUpscaler;
    
    // All currently supported texture filters
    ComputeKernel *bypassFilter;
    ComputeKernel *smoothFilter;
    ComputeKernel *blurFilter;
    ComputeKernel *saturationFilter;
    ComputeKernel *sepiaFilter;
    ComputeKernel *grayscaleFilter;
    ComputeKernel *crtFilter;
}


// -----------------------------------------------------------------------------------------------
//                                           Properties
// -----------------------------------------------------------------------------------------------

@synthesize videoUpscaler;
@synthesize videoFilter;
@synthesize enableMetal;
@synthesize fullscreen;
@synthesize fullscreenKeepAspectRatio;
@synthesize drawC64texture;

//! Adjusts view height by a certain amount
- (void)adjustHeight:(CGFloat)height
{
    NSRect newframe = self.frame;
    newframe.origin.y -= height;
    newframe.size.height += height;
    self.frame = newframe;
}

//! Shrinks view vertically by the height of the status bar
- (void)shrink { [self adjustHeight:-24.0]; }

//! Expand view vertically by the height of the status bar
- (void)expand { [self adjustHeight:24.0]; }

// -----------------------------------------------------------------------------------------------
//                                         Initialization
// -----------------------------------------------------------------------------------------------

- (id)initWithCoder:(NSCoder *)c
{
    NSLog(@"MyMetalView::initWithCoder");
    
    if ((self = [super initWithCoder:c]) == nil) {
        NSLog(@"Error: Can't initiaize MetalView");
    }
    return self;
}

-(void)awakeFromNib
{
    NSLog(@"MyMetalView::awakeFromNib");
    
    // c64 = [c64proxy c64]; // DEPRECATED
    
    // Create semaphore
    _inflightSemaphore = dispatch_semaphore_create(1);
    
    // Set initial scene position and drawing properties
    currentEyeX = targetEyeX = deltaEyeX = 0.0;
    currentEyeY = targetEyeY = deltaEyeY = 0.0;
    currentEyeZ = targetEyeZ = deltaEyeZ = 0.0;
    currentXAngle = targetXAngle = deltaXAngle = 0.0;
    currentYAngle = targetYAngle = deltaYAngle = 0.0;
    currentZAngle = targetZAngle = deltaZAngle = 0.0;
    currentAlpha = targetAlpha = 0.0; deltaAlpha = 0.0;
    
    // Properties
    enableMetal = false;
    fullscreen = false;
    fullscreenKeepAspectRatio = true;
    drawC64texture = false;
    
    // Metal
    layerWidth = 0;
    layerHeight = 0;
    layerIsDirty = YES;
    
    positionBuffer = nil;
    uniformBuffer2D = nil;
    uniformBuffer3D = nil;
    uniformBufferBg = nil;
    
    bgTexture = nil;
    emulatorTexture = nil;
    upscaledTexture = nil;
    filteredTexture = nil;
    framebufferTexture = nil;
    depthTexture = nil;
    
    bypassUpscaler = NULL;
    epxUpscaler = NULL;

    bypassFilter = NULL;
    smoothFilter = NULL;
    blurFilter = NULL;
    saturationFilter = NULL;
    sepiaFilter = NULL;
    grayscaleFilter = NULL;
    crtFilter = NULL;
    
    // Check if machine is capable to run the Metal graphics interface
    if (!MTLCreateSystemDefaultDevice()) {
        NSAlert *alert = [[NSAlert alloc] init];
        [alert setIcon:[NSImage imageNamed:@"metal.png"]];
        [alert setMessageText:@"No suitable GPU hardware found"];
        [alert setInformativeText:@"VirtualC64 can only run on machines supporting the Metal graphics technology (2012 models and above)."];
        [alert addButtonWithTitle:@"Ok"];
        [alert runModal];
        [NSApp terminate:self];
    }
        
    // Register for drag and drop
    [self registerForDraggedTypes:
    [NSArray arrayWithObjects:NSFilenamesPboardType,NSFileContentsPboardType,nil]];
}

- (void)buildKernels
{
    NSLog(@"MyMetalView::buildKernels");
    
    // Build upscalers
    bypassUpscaler = [[BypassUpscaler alloc] initWithDevice:device library:library];
    epxUpscaler = [[EPXUpscaler alloc] initWithDevice:device library:library];

    // Build filters
    bypassFilter = [[BypassFilter alloc] initWithDevice:device library:library];
    smoothFilter = [[SaturationFilter alloc] initWithDevice:device library:library factor:1.0];
    blurFilter = [[BlurFilter alloc] initWithDevice:device library:library radius:2.0];
    saturationFilter = [[SaturationFilter alloc] initWithDevice:device library:library factor:0.5];
    sepiaFilter = [[SepiaFilter alloc] initWithDevice:device library:library];
    crtFilter = [[CrtFilter alloc] initWithDevice:device library:library];
    grayscaleFilter = [[SaturationFilter alloc] initWithDevice:device library:library factor:0.0];
}

- (void) dealloc
{
    [self cleanup];
}

-(void)cleanup
{
    NSLog(@"MyMetalView::cleanup");
}

// -----------------------------------------------------------------------------------------------
//                                           Drawing
// -----------------------------------------------------------------------------------------------

- (void)updateScreenGeometry
{
    if ([c64proxy isPAL]) {
        
        // PAL border will be 36 pixels wide and 34 pixels heigh
        textureXStart = (float)(PAL_LEFT_BORDER_WIDTH - 36.0) / (float)C64_TEXTURE_WIDTH;
        textureXEnd = (float)(PAL_LEFT_BORDER_WIDTH + PAL_CANVAS_WIDTH + 36.0) / (float)C64_TEXTURE_WIDTH;
        textureYStart = (float)(PAL_UPPER_BORDER_HEIGHT - 34.0) / (float)C64_TEXTURE_HEIGHT;
        textureYEnd = (float)(PAL_UPPER_BORDER_HEIGHT + PAL_CANVAS_HEIGHT + 34.0) / (float)C64_TEXTURE_HEIGHT;
        
    } else {
        
        // NTSC border will be 42 pixels wide and 9 pixels heigh
        textureXStart = (float)(NTSC_LEFT_BORDER_WIDTH - 42.0) / (float)C64_TEXTURE_WIDTH;
        textureXEnd = (float)(NTSC_LEFT_BORDER_WIDTH + NTSC_CANVAS_WIDTH + 42.0) / (float)C64_TEXTURE_WIDTH;
        textureYStart = (float)(NTSC_UPPER_BORDER_HEIGHT - 9) / (float)C64_TEXTURE_HEIGHT;
        textureYEnd = (float)(NTSC_UPPER_BORDER_HEIGHT + NTSC_CANVAS_HEIGHT + 9) / (float)C64_TEXTURE_HEIGHT;
    }
    
    // Enable this for debugging (will display the whole texture)
    /*
     textureXStart = 0.0;
     textureXEnd = 1.0;
     textureYStart = 0.0;
     textureYEnd = 1.0;
     */
    
    // Update texture coordinates in vertex buffer
    [self buildVertexBuffer];
}

- (void)updateTexture:(id<MTLCommandBuffer>) cmdBuffer
{
    if (!c64proxy) {
        NSLog(@"Can't access C64");
        return;
    }
    
    void *buf = [[c64proxy vic] screenBuffer];
    assert(buf != NULL);

    NSUInteger pixelSize = 4;
    NSUInteger width = NTSC_PIXELS;
    NSUInteger height = PAL_RASTERLINES;
    NSUInteger rowBytes = width * pixelSize;
    NSUInteger imageBytes = rowBytes * height;
    
    [emulatorTexture replaceRegion:MTLRegionMake2D(0,0,width,height)
                       mipmapLevel:0 slice:0 withBytes:buf
                       bytesPerRow:rowBytes bytesPerImage:imageBytes];
}

- (void)setFrame:(CGRect)frame
{
    // NSLog(@"MyMetalView::setFrame");

    [super setFrame:frame];
    layerIsDirty = YES;
}

- (void)reshapeWithFrame:(CGRect)frame
{
   //  NSLog(@"MetalLayer::reshapeWithFrame");
          
    CGFloat scale = [[NSScreen mainScreen] backingScaleFactor];
    CGSize drawableSize = self.bounds.size;
    
    drawableSize.width *= scale;
    drawableSize.height *= scale;
    
    metalLayer.drawableSize = drawableSize;
    
    [self reshape];
}

- (void)reshape
{
    CGSize drawableSize = [metalLayer drawableSize];

    if (layerWidth == drawableSize.width && layerHeight == drawableSize.height)
        return;

    layerWidth = drawableSize.width;
    layerHeight = drawableSize.height;

    // NSLog(@"MetalLayer::reshape (%f,%f)", drawableSize.width, drawableSize.height);
    
    // Rebuild matrices
    [self buildMatricesBg];
    [self buildMatrices2D];
    [self buildMatrices3D];
    
    // Rebuild depth buffer and tmp drawing buffer
    [self buildDepthBuffer];
}

- (void)buildMatricesBg
{
    matrix_float4x4 model = matrix_identity_float4x4;
    matrix_float4x4 view = matrix_identity_float4x4;
    matrix_float4x4 projection = vc64_matrix_from_perspective_fov_aspectLH(65.0f * (M_PI / 180.0f), fabs(layerWidth / layerHeight), 0.1f, 100.0f);
    
    if (uniformBufferBg) {
        Uniforms *frameData = (Uniforms *)[uniformBufferBg contents];
        frameData->model = model;
        frameData->view = view;
        frameData->projectionView = projection * view * model;
        frameData->alpha = 1.0;
    }
}

- (void)buildMatrices2D
{
    matrix_float4x4 model = matrix_identity_float4x4;
    matrix_float4x4 view = matrix_identity_float4x4;
    matrix_float4x4 projection = matrix_identity_float4x4;

    if (uniformBuffer2D) {
        Uniforms *frameData = (Uniforms *)[uniformBuffer2D contents];
        frameData->model = model;
        frameData->view = view;
        frameData->projectionView = projection * view * model;
        frameData->alpha = 1.0;
    }
}

- (void)buildMatrices3D
{
    float aspectRatio = float(fabs(layerWidth / layerHeight));
    
    // NSLog(@"buildMatrices3D: aspectRatio: %f", aspectRatio);
    
    matrix_float4x4 model = vc64_matrix_from_translation(-currentEyeX, -currentEyeY, currentEyeZ+1.39);
    matrix_float4x4 view = matrix_identity_float4x4;
    matrix_float4x4 projection = vc64_matrix_from_perspective_fov_aspectLH(65.0f * (M_PI / 180.0f), aspectRatio, 0.1f, 100.0f);
    
    if ([self animates]) {
        model = model *
        vc64_matrix_from_rotation(-(currentXAngle / 180.0)*M_PI, 0.5f, 0.0f, 0.0f) *
        vc64_matrix_from_rotation((currentYAngle / 180.0)*M_PI, 0.0f, 0.5f, 0.0f) *
        vc64_matrix_from_rotation((currentZAngle / 180.0)*M_PI, 0.0f, 0.0f, 0.5f);
    }

    if (uniformBuffer3D) {
        Uniforms *frameData = (Uniforms *)[uniformBuffer3D contents];
        frameData->model = model;
        frameData->view = view;
        frameData->projectionView = projection * view * model;
        frameData->alpha = currentAlpha;
    }
}

- (ComputeKernel *)currentUpscaler
{
    switch (videoUpscaler) {
        
        case TEX_UPSCALER_EPX:
        return epxUpscaler;
        
        default:
        return bypassUpscaler;
    }
}

- (ComputeKernel *)currentFilter
{
    switch (videoFilter) {
        case TEX_FILTER_NONE:
        return bypassFilter;
        
        case TEX_FILTER_SMOOTH:
        return smoothFilter;
        
        case TEX_FILTER_BLUR:
        return blurFilter;
        
        case TEX_FILTER_SATURATION:
        return saturationFilter;
        
        case TEX_FILTER_GRAYSCALE:
        return grayscaleFilter;
        
        case TEX_FILTER_SEPIA:
        return sepiaFilter;
        
        case TEX_FILTER_CRT:
        return crtFilter;
        
        default:
        return smoothFilter;
    }
}

- (BOOL)startFrame
{
    static NSInteger width = -1;
    static NSInteger height = -1;
    framebufferTexture = _drawable.texture;

    if (width != framebufferTexture.width) {
        width = framebufferTexture.width;
        // NSLog(@"drawable width = %lu", (unsigned long)framebufferTexture.width);
    }
    if (height != framebufferTexture.height) {
        height = framebufferTexture.height;
        // NSLog(@"drawable height = %lu", (unsigned long)framebufferTexture.height);
    }
    
    NSAssert(framebufferTexture != nil, @"Framebuffer texture must not be nil");
    
    _commandBuffer = [queue commandBuffer];
    NSAssert(_commandBuffer != nil, @"Metal command buffer must not be nil");

    // Upscale C64 texture
    ComputeKernel *upscaler = [self currentUpscaler];
    [upscaler applyWithCommandBuffer:_commandBuffer source:emulatorTexture target:upscaledTexture];
    
    // Post-process C64 texture
    ComputeKernel *filter = [self currentFilter];
    assert (filter != NULL);
    [filter applyWithCommandBuffer:_commandBuffer source:upscaledTexture target:filteredTexture];
    
    // Create render pass
    MTLRenderPassDescriptor *renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    {
        renderPass.colorAttachments[0].texture = framebufferTexture;
        renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0, 0, 0, 1);
        renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        
        renderPass.depthAttachment.texture = depthTexture;
        renderPass.depthAttachment.clearDepth = 1;
        renderPass.depthAttachment.loadAction = MTLLoadActionClear;
        renderPass.depthAttachment.storeAction = MTLStoreActionDontCare;
    }
    
    // Create command encoder
    _commandEncoder = [_commandBuffer renderCommandEncoderWithDescriptor:renderPass];
    {
        [_commandEncoder setRenderPipelineState:pipeline];
        [_commandEncoder setDepthStencilState:depthState];
        [_commandEncoder setFragmentTexture:bgTexture atIndex:0];
        [_commandEncoder setFragmentSamplerState:[filter getsampler] atIndex:0];
        [_commandEncoder setVertexBuffer:positionBuffer offset:0 atIndex:0];
    }
    
    return YES;
}

- (void)drawScene2D
{
    if (![self startFrame])
        return;
    
    // Render quad
    [_commandEncoder setFragmentTexture:filteredTexture atIndex:0];
    [_commandEncoder setVertexBuffer:uniformBuffer2D offset:0 atIndex:1];
    [_commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:42 vertexCount:6 instanceCount:1];
    
    [self endFrame];
}

- (void)drawScene3D
{
    bool animates = [self animates];
    bool drawBackground = !fullscreen && (animates || !drawC64texture);
    
    if (animates) {
        [self updateAngles];
        [self buildMatrices3D];
    }
    
    if (![self startFrame])
        return;
    
    // Make texture transparent if emulator is halted
    Uniforms *frameData = (Uniforms *)[uniformBuffer3D contents];
    frameData->alpha = [c64proxy isHalted] ? 0.5 : currentAlpha;

    // Render background
    if (drawBackground) {
        [_commandEncoder setFragmentTexture:bgTexture atIndex:0];
        [_commandEncoder setVertexBuffer:uniformBufferBg offset:0 atIndex:1];
        [_commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6 instanceCount:1];
    }
    
    // Render cube
    if (drawC64texture) {
        [_commandEncoder setFragmentTexture:filteredTexture atIndex:0];
        [_commandEncoder setVertexBuffer:uniformBuffer3D offset:0 atIndex:1];
        [_commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:6 vertexCount:(animates ? 24 : 6) instanceCount:1];
    }
    
    [self endFrame];
}

- (void)endFrame
{
    [_commandEncoder endEncoding];
    
    __block dispatch_semaphore_t block_sema = _inflightSemaphore;
    [_commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buffer) {
        dispatch_semaphore_signal(block_sema);
    }];

    if (_drawable) {
        [_commandBuffer presentDrawable:_drawable];
        [_commandBuffer commit];
    }
}

- (void)drawRect:(CGRect)rect
{    
    if (!c64proxy || !enableMetal)
        return;
    
    dispatch_semaphore_wait(_inflightSemaphore, DISPATCH_TIME_FOREVER);
    
    // Refresh size dependent items if needed
    if (layerIsDirty) {
        [self reshapeWithFrame:[self frame]];
        layerIsDirty = NO;
    }
    
    // Get drawable from layer
    if (!(_drawable = [metalLayer nextDrawable])) {
        NSLog(@"Metal drawable must not be nil");
        return;
    }
    
    // Draw scene
    [self updateTexture:_commandBuffer];
    if (fullscreen && !fullscreenKeepAspectRatio) {
        [self drawScene2D];
    } else {
        [self drawScene3D];
    }
}

@end
