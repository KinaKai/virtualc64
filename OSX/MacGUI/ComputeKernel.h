/*
 * Author: Dirk W. Hoffmann, 2018
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

#import <Foundation/Foundation.h>
#import <MetalKit/MetalKit.h>

@interface ComputeKernel : NSObject
{
    id <MTLComputeCommandEncoder> computeEncoder;
    id <MTLBuffer> uniformBuffer;
    id <MTLComputePipelineState> kernel;
    id <MTLSamplerState> sampler;
    
    MTLSize threadgroupSize;
    MTLSize threadgroupCount;
}

@property (readonly) id <MTLSamplerState> sampler;

//! Create filter
- (instancetype)initWithFunctionName:(NSString *)name
                              device:(id <MTLDevice>)dev
                             library:(id <MTLLibrary>)lib;

- (void)configureComputeCommandEncoder:(id <MTLComputeCommandEncoder>)encoder;

//! Apply filter to a texture
- (void)apply:(id <MTLCommandBuffer>)commandBuffer
           in:(id <MTLTexture>)i
          out:(id <MTLTexture>)o;

@end

