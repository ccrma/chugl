/*----------------------------------------------------------------------------
 ChuGL: Unified Audiovisual Programming in ChucK

 Copyright (c) 2023 Andrew Zhu Aday and Ge Wang. All rights reserved.
   http://chuck.stanford.edu/chugl/
   http://chuck.cs.princeton.edu/chugl/

 MIT License

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
-----------------------------------------------------------------------------*/
#pragma once

#include "core/macros.h"

#include <glfw3webgpu/glfw3webgpu.h>
#include <webgpu/webgpu.h>

#define WGPU_RELEASE_RESOURCE(Type, Name)                                              \
    if (Name) {                                                                        \
        wgpu##Type##Release(Name);                                                     \
        Name = NULL;                                                                   \
    }

// does NOT free array memory
#define WGPU_RELEASE_RESOURCE_ARRAY(Type, Array, Count)                                \
    if (Array) {                                                                       \
        for (u32 i = 0; i < Count; ++i) {                                              \
            if (Array[i]) {                                                            \
                wgpu##Type##Release(Array[i]);                                         \
                Array[i] = NULL;                                                       \
            }                                                                          \
        }                                                                              \
    }

#define WGPU_DESTROY_RESOURCE(Type, Name)                                              \
    if (Name) {                                                                        \
        wgpu##Type##Destroy(Name);                                                     \
    }

#define WGPU_DESTROY_AND_RELEASE_BUFFER(Name)                                          \
    if (Name) {                                                                        \
        wgpuBufferDestroy(Name);                                                       \
        wgpuBufferRelease(Name);                                                       \
        Name = NULL;                                                                   \
    }

// ============================================================================
// Context
// =========================================================================================
// TODO request maximum device feature limits
struct GraphicsContext {
    void* base; //
    // WebGPU API objects --------
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;
    WGPUSurfaceTexture surface_texture;
    WGPUTextureFormat surface_format;

    WGPUTexture multisampled_texture;
    WGPUTextureView multisampled_texture_view;

    // Per frame resources --------
    WGPUTextureView backbufferView;
    WGPUCommandEncoder commandEncoder;
    WGPURenderPassDepthStencilAttachment depthStencilAttachment;
    WGPUCommandBuffer commandBuffer;

    // Window --------
    bool window_minimized;

    // Device limits --------
    WGPULimits limits;

    // Methods --------
    static bool init(GraphicsContext* context, GLFWwindow* window);
    static bool prepareFrame(GraphicsContext* ctx);
    static void presentFrame(GraphicsContext* ctx);
    static void resize(GraphicsContext* ctx, u32 width, u32 height);
    static void release(GraphicsContext* ctx);
};

// ============================================================================
// Buffers
// =============================================================================

// grows buffer to new size, copying old data
struct GPU_Buffer {
    WGPUBuffer buf;
    WGPUBufferUsageFlags usage;
    u64 capacity; // total size in bytes
    u64 size;     // current size in bytes

    // resizes buffer, does NOT copy old data
    // returns true if buffer was recreated
    static bool resizeNoCopy(GraphicsContext* gctx, GPU_Buffer* gpu_buffer,
                             u64 new_size, WGPUBufferUsageFlags usage_flags);

    // optional, initialize size
    static void init(GraphicsContext* gctx, GPU_Buffer* gpu_buffer,
                     WGPUBufferUsageFlags usage_flags, u64 new_capacity)
    {
        ASSERT(!gpu_buffer->buf);

        WGPUBufferDescriptor desc = {};
        desc.usage                = usage_flags | WGPUBufferUsage_CopyDst;
        desc.size                 = NEXT_MULT(new_capacity, 4);

        WGPUBuffer new_buf = wgpuDeviceCreateBuffer(gctx->device, &desc);

        // update buffer
        gpu_buffer->buf      = new_buf;
        gpu_buffer->capacity = desc.size;
        gpu_buffer->usage    = desc.usage;
    }

    // returns true if buffer was recreated (because of capacity or usage flags)
    static bool write(GraphicsContext* gctx, GPU_Buffer* gpu_buffer,
                      WGPUBufferUsageFlags usage_flags, u64 offset, const void* data,
                      u64 size)
    {
        bool recreated = false;

        // if (size == 0) return recreated;

        bool needs_new_capacity    = size + offset > gpu_buffer->capacity;
        bool needs_new_permissions = (usage_flags & gpu_buffer->usage) != usage_flags;
        bool needs_initialization  = gpu_buffer->buf == NULL;
        if (needs_new_capacity || needs_new_permissions || needs_initialization
            // offset + size > gpu_buffer->capacity
            // || (usage_flags & gpu_buffer->usage) != usage_flags
        ) {

            recreated = true;

            // recreating buffer with non-zero offset is bug
            // because we do NOT copy the data from the old buffer
            ASSERT(offset == 0);

            // grow buffer
            u64 new_capacity = MAX(gpu_buffer->capacity * 2, size + offset);
            new_capacity     = NEXT_MULT(new_capacity, 4); // align to 4 bytes
            ASSERT(new_capacity % 4 == 0);

            // TODO: how to copy data between buffers?
            // what is going on with mapping?
            // is copying a synchronous or asynchronous operation?
            // when is it safe to release the old buffer?

            WGPUBufferDescriptor desc = {};
            desc.usage                = usage_flags | WGPUBufferUsage_CopyDst;
            desc.size                 = new_capacity;

            WGPUBuffer new_buf = wgpuDeviceCreateBuffer(gctx->device, &desc);

            // release old buffer
            WGPU_DESTROY_AND_RELEASE_BUFFER(gpu_buffer->buf);

            // update buffer
            gpu_buffer->buf      = new_buf;
            gpu_buffer->capacity = desc.size;
            gpu_buffer->usage    = desc.usage;
        }

        wgpuQueueWriteBuffer(gctx->queue, gpu_buffer->buf, offset, data, size);
        gpu_buffer->size = offset + size;
        return recreated;
    }

    // returns true if buffer was recreated (because of capacity or usage flags)
    static bool write(GraphicsContext* gctx, GPU_Buffer* gpu_buffer,
                      WGPUBufferUsageFlags usage_flags, const void* data, u64 size)
    {
        return write(gctx, gpu_buffer, usage_flags, 0, data, size);
    }

    static void destroy(GPU_Buffer* buffer)
    {
        WGPU_DESTROY_AND_RELEASE_BUFFER(buffer->buf);
    }
};

// ============================================================================
// Attributes
// ============================================================================

#define VERTEX_BUFFER_LAYOUT_MAX_ENTRIES 8
// TODO request this in device limits
// force de-interleaved data. i.e. each attribute has its own buffer
struct VertexBufferLayout {
    WGPUVertexBufferLayout layouts[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    WGPUVertexAttribute attributes[VERTEX_BUFFER_LAYOUT_MAX_ENTRIES];
    u8 attribute_count;

    static void init(VertexBufferLayout* layout, u8 attribute_count,
                     u32* attribute_strides // stride in count NOT bytes
    );

    static void init(VertexBufferLayout* layout, u8 format_count,
                     WGPUVertexFormat* formats // stride in count NOT bytes
    );
};

// ============================================================================
// Shaders
// ============================================================================

struct ShaderModule {
    WGPUShaderModule module;
    WGPUShaderModuleDescriptor desc;

    // only support wgsl for now (can change into union later)
    WGPUShaderModuleWGSLDescriptor wgsl_desc;

    static void init(GraphicsContext* ctx, ShaderModule* module, const char* code,
                     const char* label);

    static void release(ShaderModule* module);
};

// ============================================================================
// Bind Group
// ============================================================================

struct BindGroup {
    WGPUBindGroup bindGroup;
    WGPUBindGroupDescriptor desc;
    WGPUBuffer uniformBuffer;

    // unsupported:
    // WGPUSampler sampler;
    // WGPUTextureView textureView;

    static void init(GraphicsContext* ctx, BindGroup* bindGroup,
                     WGPUBindGroupLayout layout, u64 bufferSize);
};

// ============================================================================
// Depth Texture
// ============================================================================

// TODO: unused, remove?

struct DepthTexture {
    WGPUTexture texture;
    WGPUTextureView view;
    WGPUTextureFormat format;

    static void init(GraphicsContext* ctx, DepthTexture* depthTexture,
                     WGPUTextureFormat format);

    static void release(DepthTexture* depthTexture);
};

// ============================================================================
// Texture
// ============================================================================

struct Texture {
    u32 width;
    u32 height;
    u32 depth;
    u32 mip_level_count;
    u32 sample_count;

    WGPUTextureFormat format;
    WGPUTextureDimension dimension;
    WGPUTextureUsageFlags usage;

    WGPUTexture texture;
    WGPUTextureView view;
    // WGPUSampler sampler;

    static void init(GraphicsContext* gctx, Texture* texture, u32 width, u32 height,
                     u32 depth, bool gen_mipmaps, const char* label,
                     WGPUTextureFormat format, WGPUTextureUsageFlags usage,
                     WGPUTextureDimension dimension);

    static void initFromFile(GraphicsContext* ctx, Texture* texture,
                             const char* filename, bool genMipMaps,
                             WGPUTextureUsageFlags usage_flags);

    // initializes from image data read into memory, NOT raw buffer
    static void initFromBuff(GraphicsContext* ctx, Texture* texture, const u8* data,
                             u64 dataLen);

    static void initSinglePixel(GraphicsContext* ctx, Texture* texture,
                                u8 pixelData[4]);

    static void initFromPixelData(GraphicsContext* ctx, Texture* gpu_texture,
                                  const char* label, const void* pixelData,
                                  int pixel_width, int pixel_height, bool genMipMaps,
                                  WGPUTextureUsageFlags usage_flags,
                                  int component_size);

    static void release(Texture* texture);
};

// ============================================================================
// Sampler
// ============================================================================
// Graphics.cpp stores a static sampler manager that creates and caches
// samplers according to the sampler descriptor.
// TODO: figure out if lodMaxClamp always 32 is ok

/*
    WGPUAddressMode_Repeat = 0x00000000,
    WGPUAddressMode_MirrorRepeat = 0x00000001,
    WGPUAddressMode_ClampToEdge = 0x00000002,
*/

enum SamplerWrapMode : u8 {
    SAMPLER_WRAP_REPEAT        = 0,
    SAMPLER_WRAP_MIRROR_REPEAT = 1,
    SAMPLER_WRAP_CLAMP_TO_EDGE = 2,
};

enum SamplerFilterMode : u8 {
    SAMPLER_FILTER_NEAREST = 0,
    SAMPLER_FILTER_LINEAR  = 1,
};

struct SamplerConfig {
    SamplerWrapMode wrapU : 2;
    SamplerWrapMode wrapV : 2;
    SamplerWrapMode wrapW : 2;
    SamplerFilterMode filterMin : 1;
    SamplerFilterMode filterMag : 1;
    SamplerFilterMode filterMip : 1;

    static SamplerConfig Default();
};

/// @brief returns a WGPU sampler object for the given configuration.
/// lazily creates a new one and stores in array if not found.
WGPUSampler Graphics_GetSampler(GraphicsContext* gctx, SamplerConfig config);
SamplerConfig Graphics_SamplerConfigFromDesciptor(WGPUSamplerDescriptor* desc);

// ============================================================================
// Material TODO: delete this
// ============================================================================

/// @brief Material instance provides uniforms/textures/etc
/// and bindGroup for a given render pipeline.
/// Each render pipeline is associated with particular material type
// struct Material {
//     WGPUBindGroup bindGroup;
//     WGPUBuffer uniformBuffer;
//     // glm::vec4 color;
//     Texture* texture; // multiple materials can share same texture
//     WGPUSampler _sampler;

//     // bind group entries
//     WGPUBindGroupEntry entries[3]; // uniforms, texture, sampler
//     WGPUBindGroupDescriptor desc;

//     // TODO: store MaterialUniforms struct (struct inheritance, like Obj)

//     static void init(GraphicsContext* ctx, Material* material,
//                      RenderPipeline* pipeline, Texture* texture);

//     static void setTexture(GraphicsContext* ctx, Material* material,
//                            Texture* texture);

//     static void release(Material* material);
// };

// ============================================================================
// MipMapGenerator
// ============================================================================

void MipMapGenerator_init(GraphicsContext* ctx);
void MipMapGenerator_release();
void MipMapGenerator_generate(GraphicsContext* ctx, WGPUTexture texture,
                              const char* label);

// ============================================================================
// Pipeline State Helpers (blend, depth/stencil, multisample)
// ============================================================================

WGPUDepthStencilState G_createDepthStencilState(WGPUTextureFormat format,
                                                bool enableDepthWrite);

WGPUMultisampleState G_createMultisampleState(u8 sample_count);

WGPUShaderModule G_createShaderModule(GraphicsContext* gctx, const char* code,
                                      const char* label);

WGPUBlendState G_createBlendState(bool enableBlend);

struct DepthStencilTextureResult {
    WGPUTexture tex;
    WGPUTextureView view;
};
DepthStencilTextureResult G_createDepthStencilTexture(GraphicsContext* gctx,
                                                      u32 sample_count, u32 width,
                                                      u32 height);

u32 G_mipLevels(int width, int height);
u32 G_mipLevelsLimit(u32 w, u32 h, u32 downscale_limit);

// calculate mip size from original size
struct G_MipSize {
    u32 width;
    u32 height;
};
G_MipSize G_mipLevelSize(int width, int height, u32 mip_level);

// creates a texture view for a single mip level
WGPUTextureView G_createTextureViewAtMipLevel(WGPUTexture texture, u32 base_mip_level,
                                              const char* label);

int G_componentsPerTexel(WGPUTextureFormat format);
int G_bytesPerTexel(WGPUTextureFormat format);
