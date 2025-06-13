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

#include "chugl_defines.h"
#include "graphics.h"
#include "sg_command.h"
#include "sg_component.h"

#include "core/macros.h"
#include "core/memory.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <iostream>
#include <unordered_map>

// freetype font library
#include <ft2build.h>
#include FT_FREETYPE_H

#include <pl/pl_mpeg.h>

// =============================================================================
// scenegraph data structures
// =============================================================================

struct R_Material;
struct R_Scene;
struct R_Font;
struct hashmap;
struct G_DrawCall;
struct G_Graph;

typedef SG_ID R_ID; // negative for R_Components NOT mapped to SG_Components

struct R_Component {
    SG_ID id; // SG_Component this R_Component is mapped to
    SG_ComponentType type;
    char name[64];
};

// priority hiearchy for staleness
enum R_Transform_Staleness {
    R_Transform_STALE_NONE = 0,

    R_Transform_STALE_DESCENDENTS, // at least 1 descendent must recompute
                                   // world matrix

    R_Transform_STALE_WORLD, // world matrix of self and all descendents must
                             // be recomputed

    R_Transform_STALE_LOCAL, // local matrix of self must be recomputed,
                             // AND world matrix of self and all descendents
                             // must be recomputed
    R_Transform_STALE_COUNT
};

enum R_Transform_Type : u8 {
    R_TRANSFORM_NONE = 0,
    R_TRANSFORM_MESH,
    R_TRANSFORM_CAMERA,
    R_TRANSFORM_LIGHT,
    R_TRANSFORM_COUNT
};

struct R_Transform : public R_Component {
    // staleness flag has priority hiearchy, don't set directly!
    // instead use setStale()
    R_Transform_Staleness _stale;

    // transform
    // don't update directly, otherwise staleness will be incorrect
    // use pos(), rot(), sca() instead
    glm::vec3 _pos;
    glm::quat _rot;
    glm::vec3 _sca;

    // world matrix (cached)
    glm::mat4 world;
    glm::mat4 local;

    SG_ID parentID;
    Arena children; // stores list of SG_IDs

    // don't modify directly; use R_Material::addPrimitve() instead
    // Possibly separate this into R_Mesh / R_Camera / R_Light
    // for now holding all type data in the R_Transform struct
    // (maybe middle ground is to use a union { R_Mesh, R_Camera, R_Light })
    SG_ID _geoID;
    SG_ID _matID;

    SG_ID scene_id; // the scene this transform belongs to

    static void init(R_Transform* transform);
    static void initFromSG(R_Transform* r_xform, SG_Command_CreateXform* cmd);

    static void setStale(R_Transform* xform, R_Transform_Staleness stale);

    static glm::mat4 localMatrix(R_Transform* xform);

    /// @brief decompose matrix into transform data
    static void setXformFromMatrix(R_Transform* xform, const glm::mat4& M);

    static void setXform(R_Transform* xform, const glm::vec3& pos, const glm::quat& rot,
                         const glm::vec3& sca);
    static void pos(R_Transform* xform, const glm::vec3& pos);
    static void rot(R_Transform* xform, const glm::quat& rot);
    static void sca(R_Transform* xform, const glm::vec3& sca);

    static void decomposeWorldMatrix(const glm::mat4& m, glm::vec3& pos, glm::quat& rot,
                                     glm::vec3& scale);

    // updates all local/world matrices in the scenegraph
    static void rebuildMatrices(R_Scene* root, Arena* arena);

    // Scenegraph relationships ----------------------------------------------
    // returns if ancestor is somewhere in the parent chain of descendent,
    // including descendent itself
    static bool isAncestor(R_Transform* ancestor, R_Transform* descendent);
    static R_Scene* getScene(R_Transform* xform);
    static void removeChild(R_Transform* parent, R_Transform* child);
    static void removeAllChildren(R_Transform* parent);
    static void addChild(R_Transform* parent, R_Transform* child);
    static u32 numChildren(R_Transform* xform);
    static R_Transform* getChild(R_Transform* xform, u32 index);

    // Transform modification ------------------------------------------------
    static void rotateOnLocalAxis(R_Transform* xform, glm::vec3 axis, f32 deg);
    static void rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg);

    // mesh -------------------------------------------------------------------
    static void updateMesh(R_Transform* xform, SG_ID geo_id, SG_ID mat_id);

    // util -------------------------------------------------------------------
    static void print(R_Transform* xform, u32 depth);
    static void print(R_Transform* xform);
};

#define R_GEOMETRY_MAX_VERTEX_ATTRIBUTES 8
struct R_Geometry : public R_Component {
    GPU_Buffer gpu_vertex_buffers[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES]; // non-interleaved
    GPU_Buffer gpu_index_buffer;
    u8 vertex_attribute_num_components[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    // storage buffers for vertex pulling
    GPU_Buffer pull_buffers[CHUGL_GEOMETRY_MAX_PULLED_VERTEX_BUFFERS];
    int vertex_count  = -1; // if set, overrides vertex count from vertices
    int indices_count = -1; // if set, overrides index count from indices

    static void init(R_Geometry* geo);

    static u32 indexCount(R_Geometry* geo);
    static u32 vertexCount(R_Geometry* geo);
    static u32 vertexAttributeCount(R_Geometry* geo);

    static void setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo, u32 location,
                                   u32 num_components_per_attrib, void* data,
                                   size_t size);

    // TODO if works, move into cpp
    // TODO move vertexPulling reflection check into state of ck ShaderDesc
    static bool usesVertexPulling(R_Geometry* geo);

    static void addPullBindGroupEntries(R_Geometry* geo, G_Graph* graph, G_DrawCall* d);

    static void setPulledVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                         u32 location, void* data, size_t size_bytes);

    static void setIndices(GraphicsContext* gctx, R_Geometry* geo, u32* indices,
                           u32 indices_count);
};

// =============================================================================
// R_Texture
// =============================================================================

struct R_Texture : public R_Component {
    WGPUTexture gpu_texture;
    SG_TextureDesc desc; // TODO redundant with R_Texture.gpu_texture

    static int sizeBytes(R_Texture* texture);

    static void init(GraphicsContext* gctx, R_Texture* texture, SG_TextureDesc* desc)
    {
        // free previous
        WGPU_RELEASE_RESOURCE(Texture, texture->gpu_texture);

        { // validation
            ASSERT(desc->mips >= 1
                   && desc->mips <= G_mipLevels(desc->width, desc->height));
            ASSERT(desc->width > 0 && desc->height > 0 && desc->depth > 0);
        }

        // copy texture info (immutable)
        texture->desc = *desc;

        // init descriptor
        WGPUTextureDescriptor wgpu_texture_desc = {};
        // wgpu_texture_desc.label                 = texture->name.c_str();
        wgpu_texture_desc.label     = texture->name;
        wgpu_texture_desc.usage     = desc->usage;
        wgpu_texture_desc.dimension = desc->dimension;
        wgpu_texture_desc.size
          = { (u32)desc->width, (u32)desc->height, (u32)desc->depth };
        wgpu_texture_desc.format        = desc->format;
        wgpu_texture_desc.mipLevelCount = desc->mips;
        wgpu_texture_desc.sampleCount   = 1;

        texture->gpu_texture
          = wgpuDeviceCreateTexture(gctx->device, &wgpu_texture_desc);
        ASSERT(texture->gpu_texture);
    }

    // resizes texture and updates generation, clears any previous data
    // used for auto-resizing framebuffer attachments
    static void resize(GraphicsContext* gctx, R_Texture* r_tex, u32 width, u32 height)
    {
        bool needs_resize = r_tex->desc.width != (int)width
                            || r_tex->desc.height != (int)height
                            || r_tex->gpu_texture == NULL;

        if (needs_resize) {
            SG_TextureDesc desc = r_tex->desc;
            desc.width          = width;
            desc.height         = height;
            desc.mips           = G_mipLevels(width, height);
            R_Texture::init(gctx, r_tex, &desc);
        }
    }

    static void write(GraphicsContext* gctx, R_Texture* texture,
                      SG_TextureWriteDesc* write_desc, void* data,
                      size_t data_size_bytes)
    {
        // don't need to bump generation here, because we are not recreating the
        // gpu_texture

        ASSERT(texture->gpu_texture);
        ASSERT(wgpuTextureGetUsage(texture->gpu_texture) & WGPUTextureUsage_CopyDst);

        // write gpu_texture data
        {
            WGPUImageCopyTexture destination = {};
            destination.texture              = texture->gpu_texture;
            destination.mipLevel             = write_desc->mip;
            destination.origin               = {
                (u32)write_desc->offset_x,
                (u32)write_desc->offset_y,
                (u32)write_desc->offset_z,
            }; // equivalent of the offset argument of Queue::writeBuffer
            destination.aspect = WGPUTextureAspect_All; // only relevant for
                                                        // depth/Stencil textures

            WGPUTextureDataLayout source = {};
            source.offset = 0; // where to start reading from the cpu buffer
            source.bytesPerRow
              = write_desc->width * G_bytesPerTexel(texture->desc.format);
            // source.rowsPerImage = write_desc->height * write_desc->depth;
            source.rowsPerImage = write_desc->height;

            WGPUExtent3D size = { (u32)write_desc->width, (u32)write_desc->height,
                                  (u32)write_desc->depth };
            wgpuQueueWriteTexture(gctx->queue, &destination, data, data_size_bytes,
                                  &source, &size);
            // wgpuQueueSubmit(gctx->queue, 0, NULL); // schedule transfer immediately
        }
    }

    static void load(GraphicsContext* gctx, R_Texture* texture, const char* filepath,
                     bool flip_vertically, bool gen_mips);

    static void loadCubemap(GraphicsContext* gctx, R_Texture* texture,
                            const char* right_face_path, const char* left_face_path,
                            const char* top_face_path, const char* bottom_face_path,
                            const char* back_face_path, const char* front_face_path,
                            bool flip_y);
};

// =============================================================================
// R_Shader
// =============================================================================

struct R_Shader : public R_Component {
    WGPUShaderModule vertex_shader_module;
    WGPUShaderModule fragment_shader_module;
    WGPUVertexFormat vertex_layout[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    WGPUShaderModule compute_shader_module;
    SG_ShaderIncludes includes;

    static void init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                     const char* vertex_filepath, const char* fragment_string,
                     const char* fragment_filepath, WGPUVertexFormat* vertex_layout,
                     int vertex_layout_count, const char* compute_string,
                     const char* compute_filepath, SG_ShaderIncludes* includes);

    static void free(R_Shader* shader);
};

// =============================================================================
// R_Material
// =============================================================================

enum R_BindType : u32 {
    // ==cleanup== conslidate with SG_BindType
    R_BIND_EMPTY = 0, // empty binding
    R_BIND_UNIFORM,
    R_BIND_SAMPLER,
    R_BIND_TEXTURE,
    // R_BIND_TEXTURE_VIEW, // default textures (e.g. white pixel)
    R_BIND_STORAGE,
    R_BIND_STORAGE_EXTERNAL, // pointer to external storage buffer (ref)
    // R_BIND_STORAGE_TEXTURE_ID, // for scenegraph textures
};

struct R_TextureBinding {
    SG_ID texture_id;
    int base_mip_level;
    int mip_level_count;
};

// TODO can we move R_Binding into .cpp
struct R_Binding {
    R_BindType type;
    size_t size; // size of data in bytes for UNIFORM and STORAGE types
    union {
        R_TextureBinding texture;
        SamplerConfig samplerConfig;
        GPU_Buffer storage_buffer;
        GPU_Buffer* storage_external; // ptr here might be dangerous...
    } as;
};

// currently unused
struct MaterialTextureView {
    // material texture view (not same as wgpu texture view)
    i32 texcoord; // 1 for TEXCOORD_1, etc.
    f32 strength; /* equivalent to strength for occlusion_texture */
    b32 hasTransform;
    // transform
    f32 offset[2];
    f32 rotation;
    f32 scale[2];

    static void init(MaterialTextureView* view);
};

struct R_Material : public R_Component {
    SG_MaterialPipelineState pso;
    // bindgroup state (uniforms, storage buffers, textures, samplers)
    R_Binding bindings[CHUGL_MATERIAL_MAX_BINDINGS];
    WGPUBuffer uniform_buffer;
    // ==optimize== after implementing wgsl reflection layout generator, can cache
    // bindgroup on material?

    // bind group fns --------------------------------------------

    // pushes bind group entries to bind_group arena
    static void createBindGroupEntries(R_Material* mat, int group, G_Graph* graph,
                                       G_DrawCall* drawcall, GraphicsContext* gctx);

    static void setBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                           R_BindType type, void* data, size_t bytes);
    static void setUniformBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  void* data, size_t bytes)
    {
        setBinding(gctx, mat, location, R_BIND_UNIFORM, data, bytes);
    }
    static void setSamplerBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  SG_Sampler sampler);

    static void setExternalStorageBinding(GraphicsContext* gctx, R_Material* mat,
                                          u32 location, GPU_Buffer* buffer);

    static void bindTexture(GraphicsContext* gctx, R_Material* mat, u32 location,
                            R_TextureBinding bind_desc);

    static void removeBinding(R_Material* mat, u32 location)
    {
        ASSERT(false);
        // TODO
    }
};

// =============================================================================
// R_Camera
// =============================================================================

struct R_Camera : public R_Transform {
    SG_CameraParams params;

    GPU_Buffer frame_uniform_buffer;
    u64 frame_uniform_buffer_fc; // frame count of last update, used to make sure buffer
                                 // is only updated once per frame

    static glm::mat4 projectionMatrix(R_Camera* camera, f32 aspect)
    {
        switch (camera->params.camera_type) {
            case SG_CameraType_PERPSECTIVE:
                return glm::perspective(camera->params.fov_radians, aspect,
                                        camera->params.near_plane,
                                        camera->params.far_plane);
            case SG_CameraType_ORTHOGRAPHIC: {
                float width  = camera->params.size * aspect;
                float height = camera->params.size;
                return glm::ortho( // extents in WORLD SPACE units
                  -width / 2.0f, width / 2.0f, -height / 2.0f, height / 2.0f,
                  camera->params.near_plane, camera->params.far_plane);
            }
            default: ASSERT(false); return glm::mat4(1.0f);
        }
    }

    static glm::mat4 viewMatrix(R_Camera* cam)
    {
        ASSERT(cam->_stale == R_Transform_STALE_NONE);
        return glm::inverse(cam->world);
        // accounts for scale
        // return glm::inverse(modelMatrix(entity));

        // optimized version for camera only (doesn't take scale into account)
        // glm::mat4 invT = glm::translate(MAT_IDENTITY, -cam->_pos);
        // glm::mat4 invR = glm::toMat4(glm::conjugate(cam->_rot));
        // return invR * invT;
    }
};

// =============================================================================
// R_Light
// =============================================================================

struct R_Light : public R_Transform {
    SG_LightDesc desc;
};

// =============================================================================
// R_Scene
// =============================================================================

struct R_Scene : R_Transform {
    SG_SceneDesc sg_scene_desc;
    hashmap* geo_to_xform;        // map from (Material, Geometry) to list of xforms
    hashmap* light_id_set;        // set of SG_IDs
    GPU_Buffer light_info_buffer; // lighting storage buffer
    u64 last_fc_updated;          // frame count of last light update

    void update(GraphicsContext* gctx, u64 frame_count, Arena* frame_arena)
    {
        if (frame_count == last_fc_updated) return;
        last_fc_updated = frame_count;

        // Update all transforms
        R_Transform::rebuildMatrices(this, frame_arena);

        // update lights
        R_Scene::rebuildLightInfoBuffer(gctx, this);
    }

    static void initFromSG(GraphicsContext* gctx, R_Scene* r_scene, SG_ID scene_id,
                           SG_SceneDesc* sg_scene_desc);

    static void removeSubgraphFromRenderState(R_Scene* scene, R_Transform* xform);
    static void addSubgraphToRenderState(R_Scene* scene, R_Transform* xform);

    static void rebuildLightInfoBuffer(GraphicsContext* gctx, R_Scene* scene);

    static i32 numLights(R_Scene* scene)
    {
        return (i32)hashmap_count(scene->light_id_set);
    }

    static void registerMesh(R_Scene* scene, R_Transform* mesh);
    static void unregisterMesh(R_Scene* scene, R_Transform* mesh);
    static void markPrimitiveStale(R_Scene* scene, R_Transform* mesh);
    static WGPUBindGroup createPrimitiveBindGroup(GraphicsContext* gctx, R_Scene* scene,
                                                  SG_ID material_id, SG_ID geo_id,
                                                  WGPUBindGroupLayout layout,
                                                  Arena* frame_arena);
    static int numPrimitives(R_Scene* scene, SG_ID material_id, SG_ID geo_id);
};

// =============================================================================
// R_Pass
// =============================================================================

struct Framebuffer {
    u32 width        = 0;
    u32 height       = 0;
    int sample_count = 0;

    WGPUTexture depth_tex;
    WGPUTextureView depth_view;
    WGPUTexture color_tex;
    WGPUTextureView color_view;

    // nvm, resolve target comes from chugl texture
    // WGPUTexture resolve_tex; // sample_count == 1
    // WGPUTextureView resolve_view;

    static void createAttachment(GraphicsContext* gctx, WGPUTextureFormat format,
                                 WGPUTextureUsageFlags usage_flags, u32 width,
                                 u32 height, int sample_count, WGPUTexture* out_tex,
                                 WGPUTextureView* out_view)
    {
        WGPUExtent3D texture_extent = { width, height, 1 };

        // Texture usage flags
        usage_flags |= WGPUTextureUsage_RenderAttachment;

        // Create the texture
        WGPUTextureDescriptor texture_desc = {};
        texture_desc.label                 = NULL;
        texture_desc.size                  = texture_extent;
        texture_desc.mipLevelCount         = 1;
        texture_desc.sampleCount           = sample_count;
        texture_desc.dimension             = WGPUTextureDimension_2D;
        texture_desc.format                = format;
        texture_desc.usage                 = usage_flags;

        WGPU_RELEASE_RESOURCE(Texture, *out_tex);
        *out_tex = wgpuDeviceCreateTexture(gctx->device, &texture_desc);
        ASSERT(*out_tex);

        // Create the texture view
        WGPUTextureViewDescriptor texture_view_desc = {};
        texture_view_desc.label                     = NULL;
        texture_view_desc.dimension                 = WGPUTextureViewDimension_2D;
        texture_view_desc.format                    = texture_desc.format;
        texture_view_desc.baseMipLevel              = 0;
        texture_view_desc.mipLevelCount             = 1;
        texture_view_desc.baseArrayLayer            = 0;
        texture_view_desc.arrayLayerCount           = 1;
        texture_view_desc.aspect                    = WGPUTextureAspect_All;

        WGPU_RELEASE_RESOURCE(TextureView, *out_view);
        *out_view = wgpuTextureCreateView(*out_tex, &texture_view_desc);
        ASSERT(*out_view);
    }

    // rebuilds framebuffer attachment textures
    static void rebuild(GraphicsContext* gctx, Framebuffer* fb, u32 width, u32 height,
                        int sample_count, WGPUTextureFormat color_format)
    {
        bool texture_resized      = (fb->width != width || fb->height != height);
        bool sample_count_changed = fb->sample_count != sample_count;
        // todo support change in resolve target SG_ID
        if (texture_resized || sample_count_changed) {
            log_debug("rebuilding framebuffer, %dx%d, %d samples", width, height,
                      sample_count);
            fb->width        = width;
            fb->height       = height;
            fb->sample_count = sample_count;

            // recreate color target TODO get format and usage from sgid
            // for now locking down to hdr
            Framebuffer::createAttachment(
              gctx, color_format, WGPUTextureUsage_TextureBinding, width, height,
              sample_count, &fb->color_tex, &fb->color_view);

            // recreate depth target
            Framebuffer::createAttachment(
              gctx, WGPUTextureFormat_Depth24PlusStencil8, WGPUTextureUsage_None, width,
              height, sample_count, &fb->depth_tex, &fb->depth_view);

            // TODO create resolve
        }
    }
};

struct R_Pass : public R_Component {
    SG_Pass sg_pass;

    // RenderPass params
    WGPURenderPassColorAttachment color_attachments[1];
    WGPURenderPassDepthStencilAttachment depth_stencil_attachment;
    WGPURenderPassDescriptor render_pass_desc;
    // RenderPass framebuffer
    Framebuffer framebuffer;

    // ScreenPass params (no depth buffer necessary)
    WGPURenderPassColorAttachment screen_color_attachments[1];
    WGPURenderPassDescriptor screen_pass_desc;

    static void updateScreenPassDesc(GraphicsContext* gctx, R_Pass* pass,
                                     WGPUTextureView color_attachment_view)
    {
        ASSERT(pass->sg_pass.pass_type == SG_PassType_Screen);

        WGPURenderPassColorAttachment* ca = &pass->screen_color_attachments[0];
        *ca                               = {};
        ca->view                          = color_attachment_view;
        ca->loadOp                        = WGPULoadOp_Clear;
        ca->storeOp                       = WGPUStoreOp_Store;
        ca->clearValue                    = WGPUColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        ca->depthSlice                    = WGPU_DEPTH_SLICE_UNDEFINED;

        pass->screen_pass_desc                        = {};
        pass->screen_pass_desc.label                  = pass->sg_pass.name;
        pass->screen_pass_desc.colorAttachmentCount   = 1;
        pass->screen_pass_desc.colorAttachments       = pass->screen_color_attachments;
        pass->screen_pass_desc.depthStencilAttachment = NULL;
    }

    // if window size has changed, lazily reconstruct depth/stencil and color
    // targets.
    // update DepthStencilAttachment and ColorAttachment params based on
    // ChuGL RenderPass params.
    // this should be called right before _R_RenderScene()
    // for the given pass. R_Pass.sg_pass is assumed to be updated (memcopied in the
    // updatePass Command), but not the gpu-specific parameters of R_Pass
    static void updateRenderPassDesc(GraphicsContext* gctx, R_Pass* pass,
                                     u32 color_target_width, u32 color_target_height,
                                     int sample_count, WGPUTextureView resolve_view,
                                     WGPUTextureFormat view_format,
                                     glm::vec4 clear_color)
    {
        ASSERT(pass->sg_pass.pass_type == SG_PassType_Render);

        // handle resize
        Framebuffer::rebuild(gctx, &pass->framebuffer, color_target_width,
                             color_target_height, sample_count, view_format);

        // for now, we always set renderpass depth/stencil and color descriptors
        // (even if they haven't changed) to simplify state management
        { // depth
            pass->depth_stencil_attachment.view = pass->framebuffer.depth_view;
            // defaults for render pass depth/stencil attachment
            // The initial value of the depth buffer, meaning "far"
            pass->depth_stencil_attachment.depthClearValue = 1.0f;
            pass->depth_stencil_attachment.depthLoadOp     = WGPULoadOp_Clear;
            pass->depth_stencil_attachment.depthStoreOp    = WGPUStoreOp_Store;
            // we could turn off writing to the depth buffer globally here
            pass->depth_stencil_attachment.depthReadOnly = false;

            // Stencil setup, mandatory but unused
            pass->depth_stencil_attachment.stencilClearValue = 0;
            pass->depth_stencil_attachment.stencilLoadOp     = WGPULoadOp_Clear;
            pass->depth_stencil_attachment.stencilStoreOp    = WGPUStoreOp_Store;
            pass->depth_stencil_attachment.stencilReadOnly   = false;
        }

        { // color
            // defaults for render pass color attachment
            WGPURenderPassColorAttachment* ca = &pass->color_attachments[0];
            *ca                               = {};

            // view and resolve set in GraphicsContext::prepareFrame()
            if (sample_count > 1) {
                // if MSAA, need to set target and resolve separately
                ca->view          = pass->framebuffer.color_view;
                ca->resolveTarget = resolve_view;
            } else {
                // no MSAA, set color attachment to resolve target directly
                ca->view          = resolve_view;
                ca->resolveTarget = NULL;
            }
            ca->depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
            ca->loadOp  = pass->sg_pass.color_target_clear_on_load ? WGPULoadOp_Clear :
                                                                     WGPULoadOp_Load;
            ca->storeOp = WGPUStoreOp_Store;
            ca->clearValue
              = WGPUColor{ clear_color.r, clear_color.g, clear_color.b, clear_color.a };
        }

        { // renderpass desc
            pass->render_pass_desc                      = {};
            pass->render_pass_desc.label                = pass->sg_pass.name;
            pass->render_pass_desc.colorAttachmentCount = 1;
            pass->render_pass_desc.colorAttachments     = pass->color_attachments;
            pass->render_pass_desc.depthStencilAttachment
              = &pass->depth_stencil_attachment;
        }
    }
};

// =============================================================================
// R_Buffer
// =============================================================================

struct R_Buffer : public R_Component {
    GPU_Buffer gpu_buffer;
};

// =============================================================================
// R_Font
// =============================================================================

struct Glyph {
    FT_UInt index;
    i32 bufferIndex;

    i32 curveCount;

    // Important glyph metrics in font units.
    FT_Pos width, height;
    FT_Pos bearingX;
    FT_Pos bearingY;
    FT_Pos advance;
};

struct BufferGlyph {
    i32 start, count; // range of bezier curves belonging to this glyph
};
static_assert(sizeof(BufferGlyph) == (2 * sizeof(i32)), "bufferglyph size");

struct BufferCurve {
    float x0, y0, x1, y1, x2, y2;
};
static_assert(sizeof(BufferCurve) == 6 * sizeof(float), "buffercurve size");

struct BoundingBox {
    float minX, minY, maxX, maxY;
};

struct R_Text : public R_Transform {
    std::string text;
    std::string font_path;
    glm::vec2 control_points;
    float vertical_spacing;
};

struct R_Font {
    std::string font_path;
    FT_Face face; // TODO multiplex faces across R_Font. multiple R_Font with same
                  // font but different text can share the same face

    FT_Int32 loadFlags;
    FT_Kerning_Mode kerningMode;

    // Size of the em square used to convert metrics into em-relative values,
    // which can then be scaled to the worldSize. We do the scaling ourselves in
    // floating point to support arbitrary world sizes (whereas the fixed-point
    // numbers used by FreeType do not have enough resolution if the world size
    // is small).
    // Following the FreeType convention, if hinting (and therefore scaling) is
    // enabled, this value is in 1/64th of a pixel (compatible with 26.6 fixed point
    // numbers). If hinting/scaling is not enabled, this value is in font units.
    float emSize;

    float worldSize = 1.0f;

    GPU_Buffer glyph_buffer;
    GPU_Buffer curve_buffer;

    std::vector<BufferGlyph> bufferGlyphs;
    std::vector<BufferCurve> bufferCurves;
    std::unordered_map<u32, Glyph> glyphs;

    // The glyph quads are expanded by this amount to enable proper
    // anti-aliasing. Value is relative to emSize.
    float dilation = 0.1f;

    // given a text object, updates its geo vertex buffers
    // and material bindgroup
    static void updateText(GraphicsContext* gctx, R_Font* font, R_Text* text);
    static bool init(GraphicsContext* gctx, FT_Library library, R_Font* font,
                     const char* font_path);

    static void free(R_Font* text)
    {
        GPU_Buffer::destroy(&text->glyph_buffer);
        GPU_Buffer::destroy(&text->curve_buffer);
        FT_Done_Face(text->face);
    }

    static void prepareGlyphsForText(GraphicsContext* gctx, R_Font* font,
                                     const char* text);

    // given text and a starting model-space coordinate (x,y)
    // reconstructs the vertex and index buffers for the text
    // (used to batch draw a single GText object)
    static void rebuildVertexBuffers(R_Font* font, const char* mainText, float x,
                                     float y, Arena* positions, Arena* uvs,
                                     Arena* glyph_indices, Arena* indices,
                                     float verticalScale = 1.0f);

    BoundingBox measure(float x, float y, const char* text, float verticalScale = 1.0f);
};

// =============================================================================
// R_Video
// =============================================================================

struct R_Video : public R_Component {
    plm_t* plm;            // plm_destroy(plm) to free
    GraphicsContext* gctx; // messy workaround for plm callbacks
    SG_ID video_texture_rgba_id;
    u8* rgba_data_OWNED; // free with free(rgba_data)
    int rgba_data_size;
    float rate = 1.0f;
};

// =============================================================================
// R_Webcam
// =============================================================================

struct R_Webcam : public R_Component {
    SG_ID webcam_texture_id;
    int device_id;
    u64 last_frame_count; // last webcame frame count, used to detect new frames and
                          // prevent reuploading old frames
    bool freeze;

    static void updateTexture(GraphicsContext* gctx, R_Webcam* webcam);
    static void update(SG_Command_WebcamUpdate* cmd);
};

// =============================================================================
// Component Manager API
// =============================================================================

R_Transform* Component_CreateTransform();
R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd);

R_Transform* Component_CreateMesh(SG_ID mesh_id, SG_ID geo_id, SG_ID mat_id);
R_Camera* Component_CreateCamera(GraphicsContext* gctx, SG_Command_CameraCreate* cmd);

R_Text* Component_CreateText(GraphicsContext* gctx, FT_Library ft,
                             SG_Command_TextRebuild* cmd, R_Font* default_font);

R_Scene* Component_CreateScene(GraphicsContext* gctx, SG_ID scene_id,
                               SG_SceneDesc* sg_scene_desc);

R_Geometry* Component_CreateGeometry();
R_Geometry* Component_CreateGeometry(GraphicsContext* gctx, SG_ID geo_id);

R_Shader* Component_CreateShader(GraphicsContext* gctx, SG_Command_ShaderCreate* cmd);

// R_Material* Component_CreateMaterial(GraphicsContext* gctx, R_MaterialConfig*
// config);
R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd);

R_Texture* Component_CreateTexture();
R_Texture* Component_CreateTexture(GraphicsContext* gctx,
                                   SG_Command_TextureCreate* cmd);
R_Pass* Component_CreatePass(SG_ID pass_id);
R_Buffer* Component_CreateBuffer(SG_ID id);
R_Light* Component_CreateLight(SG_ID id, SG_LightDesc* desc);
R_Video* Component_CreateVideo(GraphicsContext* gctx, SG_ID id, const char* filename,
                               SG_ID rgba_texture_id);
R_Webcam* Component_CreateWebcam(SG_Command_WebcamCreate* cmd);

R_Component* Component_GetComponent(SG_ID id);
WGPUSampler Component_GetSampler(GraphicsContext* gctx, SG_Sampler sampler);
R_Transform* Component_GetXform(SG_ID id);
R_Transform* Component_GetMesh(SG_ID id);
R_Scene* Component_GetScene(SG_ID id);
R_Geometry* Component_GetGeometry(SG_ID id);
R_Shader* Component_GetShader(SG_ID id);
R_Material* Component_GetMaterial(SG_ID id);
R_Texture* Component_GetTexture(SG_ID id);
R_Camera* Component_GetCamera(SG_ID id);
R_Text* Component_GetText(SG_ID id);
R_Font* Component_GetFont(GraphicsContext* gctx, FT_Library library,
                          const char* font_path);
R_Pass* Component_GetPass(SG_ID id);
R_Buffer* Component_GetBuffer(SG_ID id);
R_Light* Component_GetLight(SG_ID id);
R_Video* Component_GetVideo(SG_ID id);
R_Webcam* Component_GetWebcam(SG_ID id);

// be careful to not delete components while iterating
// returns false upon reachign end of material arena
bool Component_MaterialIter(size_t* i, R_Material** material);
bool Component_VideoIter(size_t* i, R_Video** video);
bool Component_WebcamIter(size_t* i, R_Webcam** webcam);

// component manager initialization
void Component_Init(GraphicsContext* gctx);
void Component_Free();

// component garbage collection
void Component_FreeComponent(SG_ID id);

// TODO: add destroy functions. Remember to change offsets after swapping!
// should these live in the components?
// TODO: on xform destroy, set material/geo primitive to stale
// void Component_DestroyXform(u64 id);
/*
Enforcing pointer safety:
- hide all component initialization fns as static within component.cpp
    - only the manager can create/delete components
    - similar to how all memory allocations are routed through realloc
- all component accesses happen via IDs routed through the manager
    - IDs, unlike pointers, are safe to store
    - if the component created by that ID is deleted, the ID lookup will yield
    NULL, and the calling code will likely crash with a NULL pointer dereference
    (easy to debug)
- all deletions / GC are deferred to the VERY END of the frame
    - prevents bug where a component is deleted WHILE it is being used after an
    ID lookup
    - enforce hygiene of never storing / carrying pointers across frame
    boundaries (within is ok)
    - also enables a more controllable GC system
*/

// =============================================================================
// RenderGraph, DrawCall, Cache
// =============================================================================

struct G_DrawCallPipelineDesc {
    SG_ID sg_shader_id;
    WGPUCullMode cull_mode;
    WGPUPrimitiveTopology primitive_topology;
    u32 is_transparent; // TODO support other blend modes (subtrative, additive etc)
};

struct G_CacheComputePipeline {
    WGPUShaderModule key;
    struct {
        WGPUComputePipeline pipeline;
        WGPUBindGroupLayout bind_group_layout; // currently compute pipelines are only
                                               // allowed to use @group(0)
    } val;

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        G_CacheComputePipeline* entry = (G_CacheComputePipeline*)item;
        return hashmap_xxhash3(&entry->key, sizeof(entry->key), seed0, seed1);
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        G_CacheComputePipeline* ga = (G_CacheComputePipeline*)a;
        G_CacheComputePipeline* gb = (G_CacheComputePipeline*)b;
        ASSERT(sizeof(ga->key) == sizeof(WGPUShaderModule));
        return memcmp(&ga->key, &gb->key, sizeof(ga->key));
    }
};

struct G_CacheRenderPipelineKey {
    G_DrawCallPipelineDesc drawcall_pipeline_desc;
    WGPUTextureFormat color_target_format;
    WGPUTextureFormat depth_target_format;

    void print()
    {
        R_Shader* shader = Component_GetShader(drawcall_pipeline_desc.sg_shader_id);
        printf(
          "Shader[%d:%s]\n"
          "Cull_Mode: %s\n"
          "Topology: %s\n"
          "Transparent: %s\n"
          "Color Format %s\n"
          "Depth Format %s\n",
          shader->id, shader->name,
          G_Util::cullModeToString(drawcall_pipeline_desc.cull_mode),
          G_Util::topologyToString(drawcall_pipeline_desc.primitive_topology),
          drawcall_pipeline_desc.is_transparent ? "true" : "false",
          G_Util::textureFormatToString(color_target_format),
          G_Util::textureFormatToString(depth_target_format));
    }
};

struct G_CacheRenderPipelineVal {
    WGPURenderPipeline pipeline;
    WGPUBindGroupLayout
      bind_group_layout_list[CHUGL_MAX_BINDGROUPS]; // TODO free on delete

    // lazy evaluate bindGroups because getting bindGroupLayout of
    // a group that doesn't exist throughs a WGPU validation error
    WGPUBindGroupLayout bindGroupLayout(int index)
    {
        if (bind_group_layout_list[index] == NULL) {
            log_trace("calling wgpuRenderPipelineGetBindGroupLayout(%d)", index);
            bind_group_layout_list[index]
              = wgpuRenderPipelineGetBindGroupLayout(pipeline, index);
        }
        return bind_group_layout_list[index];
    }
};

struct G_CacheRenderPipeline {
    G_CacheRenderPipelineKey key;
    G_CacheRenderPipelineVal val;

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        G_CacheRenderPipeline* entry = (G_CacheRenderPipeline*)item;
        ASSERT(sizeof(entry->key) == sizeof(G_CacheRenderPipelineKey));
        return hashmap_xxhash3(&entry->key, sizeof(entry->key), seed0, seed1);
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        G_CacheRenderPipeline* ga = (G_CacheRenderPipeline*)a;
        G_CacheRenderPipeline* gb = (G_CacheRenderPipeline*)b;
        return memcmp(&ga->key, &gb->key, sizeof(ga->key));
    }
};

struct G_CacheBindGroupEntryBuffer {
    WGPUBuffer buffer;
    u32 offset;
    u32 size;
};

struct G_CacheTextureViewDesc {
    WGPUTexture texture; // NOT refouncted here. instead refcounted by chuck + R_Texture
    int base_mip_level;
    int mip_level_count;
};

struct G_CacheTextureView {
    G_CacheTextureViewDesc key;
    struct {
        WGPUTextureView view;
        int refcount;
    } val;

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        G_CacheTextureView* entry = (G_CacheTextureView*)item;
        ASSERT(sizeof(entry->key) == sizeof(G_CacheTextureViewDesc));
        return hashmap_xxhash3(&entry->key, sizeof(entry->key), seed0, seed1);
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        G_CacheTextureView* ga = (G_CacheTextureView*)a;
        G_CacheTextureView* gb = (G_CacheTextureView*)b;
        return memcmp(&ga->key, &gb->key, sizeof(ga->key));
    }
};

enum G_CacheBindGroupEntryType : u8 {
    G_CacheBindGroupEntryType_None = 0,
    G_CacheBindGroupEntryType_Buffer,
    G_CacheBindGroupEntryType_Sampler,
    G_CacheBindGroupEntryType_TextureView,
};

struct G_CacheBindGroupEntry {
    u8 binding;
    G_CacheBindGroupEntryType type;
    union {
        G_CacheBindGroupEntryBuffer buffer;
        WGPUSampler sampler;
        G_CacheTextureViewDesc texture_view_desc;
    } as;
};

struct G_CacheBindGroupKey {
    WGPUBindGroupLayout layout;
    int bg_entry_count;
    G_CacheBindGroupEntry bg_entry_list[CHUGL_MATERIAL_MAX_BINDINGS];
};

struct G_CacheBindGroupVal {
    WGPUBindGroup bg;
    int frames_till_expired = CHUGL_CACHE_BINDGROUP_FRAMES_TILL_EXPIRED;
};

struct G_CacheBindGroup {
    G_CacheBindGroupKey key;
    G_CacheBindGroupVal val;

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        // ==optimize== cache the hash
        G_CacheBindGroupKey* key = (G_CacheBindGroupKey*)item;
        const size_t base_len    = offsetof(G_CacheBindGroupKey, bg_entry_list);
        return hashmap_xxhash3(
          key, base_len + key->bg_entry_count * sizeof(key->bg_entry_list[0]), seed0,
          seed1);
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        G_CacheBindGroupKey* ga = (G_CacheBindGroupKey*)a;
        G_CacheBindGroupKey* gb = (G_CacheBindGroupKey*)b;
        return memcmp(ga, gb, sizeof(*ga));
    }
};

// eventually this will become more like WGPUTextureDesc
struct G_CacheRenderTargetDesc {
    WGPUTextureFormat view_format;
    u32 width;
    u32 height;
};

struct G_Cache {
    hashmap* render_pipeline_map;
    hashmap* compute_pipeline_map;
    hashmap* bindgroup_map;
    hashmap* texture_view_map;

    Arena deletion_queue;

    void init()
    {
        render_pipeline_map = hashmap_new_simple(sizeof(G_CacheRenderPipeline),
                                                 G_CacheRenderPipeline::hash,
                                                 G_CacheRenderPipeline::compare);

        compute_pipeline_map = hashmap_new_simple(sizeof(G_CacheComputePipeline),
                                                  G_CacheComputePipeline::hash,
                                                  G_CacheComputePipeline::compare);

        bindgroup_map = hashmap_new_simple(
          sizeof(G_CacheBindGroup), G_CacheBindGroup::hash, G_CacheBindGroup::compare);

        texture_view_map
          = hashmap_new_simple(sizeof(G_CacheTextureView), G_CacheTextureView::hash,
                               G_CacheTextureView::compare);
    }

    void free()
    {
        // not implemented because there's only 1 G_Cache (in App)
        // and we let OS do clean up when chugl exits
    }

    G_CacheComputePipeline computePipeline(WGPUShaderModule module, WGPUDevice device,
                                           const char* label)
    {
        G_CacheComputePipeline* result
          = (G_CacheComputePipeline*)hashmap_get(compute_pipeline_map, &module);

        if (result == NULL) { // create new pipeline
            log_trace("Cache miss [ComputePipeline], creating new from %p", module);

            WGPUComputePipelineDescriptor desc = {};
            desc.label                         = label;
            desc.compute.module                = module;
            desc.compute.entryPoint            = CHUGL_COMPUTE_ENTRY_POINT;

            WGPUComputePipeline pipeline
              = wgpuDeviceCreateComputePipeline(device, &desc);
            G_CacheComputePipeline item
              = { module,
                  {
                    pipeline,
                    wgpuComputePipelineGetBindGroupLayout(
                      pipeline, 0) // caching because this wgpu call leaks memory
                  } };

            const void* replaced = hashmap_set(compute_pipeline_map, &item);
            ASSERT(!replaced);

            result = (G_CacheComputePipeline*)hashmap_get(compute_pipeline_map, &item);
            ASSERT(result);
        }

        return *result;
    }

    G_CacheRenderPipeline* renderPipeline(G_CacheRenderPipelineKey key,
                                          WGPUDevice device)
    {
        G_CacheRenderPipeline* result
          = (G_CacheRenderPipeline*)hashmap_get(render_pipeline_map, &key);

        if (result == NULL) {
            // TODO: maybe hash the shader module / shader types to remove dependency on
            // scene graph
            R_Shader* shader
              = Component_GetShader(key.drawcall_pipeline_desc.sg_shader_id);
            ASSERT(shader);

            log_trace("Cache miss [RenderPipeline], creating new from Shader[%d:%s]",
                      key.drawcall_pipeline_desc.sg_shader_id, shader->name);

#if 0 // uncomment to debug pipeline creation
      // print the key
            key.print();

            printf("--------hashmap current contents---------\n");

            size_t pipeline_map_idx_DONT_USE      = 0;
            G_CacheRenderPipeline* cache_pipeline = NULL;
            while (hashmap_iter(render_pipeline_map, &pipeline_map_idx_DONT_USE,
                                (void**)&cache_pipeline)) {
                R_Shader* cache_shader = Component_GetShader(
                  cache_pipeline->key.drawcall_pipeline_desc.sg_shader_id);

                if (cache_shader->id == shader->id) {
                    hexDump("new pipeline key", &key, sizeof(key));
                    hexDump("cached pipeline key", &cache_pipeline->key,
                            sizeof(cache_pipeline->key));
                }

                printf(
                  "Shader[%d:%s]\n"
                  "Equals: %s\n"
                  "Cull_Mode: %s\n"
                  "Topology: %s\n"
                  "Transparent: %s\n"
                  "Color Format %s\n"
                  "Depth Format %s\n",
                  cache_shader->id, cache_shader->name,
                  G_CacheRenderPipeline::compare(&key, &cache_pipeline->key, NULL)
                      == 0 ?
                    "true" :
                    "false",
                  G_Util::cullModeToString(
                    cache_pipeline->key.drawcall_pipeline_desc.cull_mode),
                  G_Util::topologyToString(
                    cache_pipeline->key.drawcall_pipeline_desc.primitive_topology),
                  cache_pipeline->key.drawcall_pipeline_desc.is_transparent ? "true" :
                                                                              "false",
                  G_Util::textureFormatToString(
                    cache_pipeline->key.color_target_format),
                  G_Util::textureFormatToString(
                    cache_pipeline->key.depth_target_format));
                printf("-------\n");
            }
#endif

            ///////////////////////////////////
            // create GPUPipelineDesc from R_PipelineDesc

            bool is_render_pipeline = (shader->vertex_shader_module != NULL);
            // bool is_compute_pipeline = (shader->compute_shader_module != NULL);
            ASSERT(is_render_pipeline);

            // build pipeline desc
            WGPURenderPipelineDescriptor pipeline_desc = {};
            pipeline_desc.layout                       = NULL; // Using layout: auto
            pipeline_desc.primitive.cullMode = key.drawcall_pipeline_desc.cull_mode;
            pipeline_desc.primitive.topology
              = key.drawcall_pipeline_desc.primitive_topology;
            pipeline_desc.primitive.stripIndexFormat
              = G_Util::isStripTopology(key.drawcall_pipeline_desc.primitive_topology) ?
                  WGPUIndexFormat_Uint32 :
                  WGPUIndexFormat_Undefined;

            VertexBufferLayout vertex_layout = {};
            VertexBufferLayout::init(&vertex_layout,
                                     ARRAY_LENGTH(shader->vertex_layout),
                                     shader->vertex_layout);
            pipeline_desc.vertex.bufferCount = vertex_layout.attribute_count;
            pipeline_desc.vertex.buffers     = vertex_layout.layouts;
            pipeline_desc.vertex.module      = shader->vertex_shader_module;
            pipeline_desc.vertex.entryPoint  = VS_ENTRY_POINT;

            // TODO what happens if fragment shader is not defined?
            WGPUBlendState blend_state = G_createBlendState(
              key.drawcall_pipeline_desc.is_transparent); // TODO transparency
            WGPUColorTargetState colorTargetState = {};
            colorTargetState.format               = key.color_target_format;
            colorTargetState.blend                = &blend_state;
            colorTargetState.writeMask            = WGPUColorWriteMask_All;
            WGPUFragmentState fragmentState       = {};
            fragmentState.module                  = shader->fragment_shader_module;
            fragmentState.entryPoint              = FS_ENTRY_POINT;
            fragmentState.targetCount             = 1; // fix 1 color target for now
            fragmentState.targets                 = &colorTargetState;

            pipeline_desc.fragment = &fragmentState;

            WGPUDepthStencilState depth_stencil_state
              = G_createDepthStencilState(key.depth_target_format, true);
            pipeline_desc.depthStencil
              = key.depth_target_format ? &depth_stencil_state : NULL;

            pipeline_desc.multisample = G_createMultisampleState(1);

            char pipeline_label[64] = {};
            snprintf(pipeline_label, sizeof(pipeline_label),
                     "RenderPipeline: Shader[%d] %s", shader->id, shader->name);
            pipeline_desc.label = pipeline_label;

            WGPURenderPipeline pipeline
              = wgpuDeviceCreateRenderPipeline(device, &pipeline_desc);

            // add to cache
            G_CacheRenderPipeline pipeline_item = {};
            pipeline_item.key                   = key;
            pipeline_item.val.pipeline          = pipeline;

            const void* replaced = hashmap_set(render_pipeline_map, &pipeline_item);
            ASSERT(!replaced);

            result = (G_CacheRenderPipeline*)hashmap_get(render_pipeline_map,
                                                         &pipeline_item);
            ASSERT(result);
        }

        return result;
    }

    WGPUTextureView _createTextureViewForBindgroup(G_CacheTextureViewDesc desc)
    {
        // check if present in cache
        G_CacheTextureView* cache_view
          = (G_CacheTextureView*)hashmap_get(texture_view_map, &desc);

        if (cache_view == NULL) {
            log_trace(
              "Cache miss [TextureView], creating new from Texture[%p] mips[%d:%d]",
              (void*)desc.texture, desc.base_mip_level,
              desc.base_mip_level + desc.mip_level_count - 1);

            u32 depth       = wgpuTextureGetDepthOrArrayLayers(desc.texture);
            bool is_cubemap = (depth == 6);
            WGPUTextureViewDescriptor view_desc = {};
            // view_desc.label                     = mip_label; // TODO
            view_desc.format          = wgpuTextureGetFormat(desc.texture);
            view_desc.dimension       = is_cubemap ? WGPUTextureViewDimension_Cube :
                                                     WGPUTextureViewDimension_2D;
            view_desc.baseMipLevel    = desc.base_mip_level;
            view_desc.mipLevelCount   = desc.mip_level_count;
            view_desc.baseArrayLayer  = 0;
            view_desc.arrayLayerCount = depth;

            G_CacheTextureView item = {};
            item.key                = desc;
            item.val.view           = wgpuTextureCreateView(desc.texture, &view_desc);
            ASSERT(item.val.view);
            const void* replaced = hashmap_set(texture_view_map, &item);
            ASSERT(!replaced);

            cache_view = (G_CacheTextureView*)hashmap_get(texture_view_map, &desc);
            ASSERT(cache_view);
        }

        ++cache_view->val.refcount;
        return cache_view->val.view;
    }

    WGPUBindGroup bindGroup(WGPUDevice device, G_CacheBindGroupEntry* bg_entry_list,
                            int bg_entry_count, WGPUBindGroupLayout layout,

                            // debug info
                            int group)
    {
        G_CacheBindGroup item = {};
        ASSERT(bg_entry_count <= ARRAY_LENGTH(item.key.bg_entry_list));
        item.key.layout         = layout;
        item.key.bg_entry_count = bg_entry_count;
        memcpy(item.key.bg_entry_list, bg_entry_list,
               sizeof(*bg_entry_list) * bg_entry_count);

        G_CacheBindGroup* result
          = (G_CacheBindGroup*)hashmap_get(bindgroup_map, &item.key);

        if (result == NULL) {
            log_trace("Cache miss [BindGroup] @group(%d) creating new for layout %p",
                      group, layout);
            static WGPUBindGroupEntry wgpu_bg_entry_list[CHUGL_MATERIAL_MAX_BINDINGS]
              = {};
#if 0
            if (cache_pipeline) cache_pipeline->key.print();
            G_Util::printBindGroupEntryList(bg_entry_list, bg_entry_count);
#endif

            // convert bg_list to wgpu_bg_list
            ZERO_ARRAY(wgpu_bg_entry_list);
            for (int i = 0; i < bg_entry_count; i++) {
                wgpu_bg_entry_list[i].binding = bg_entry_list[i].binding;
                switch (bg_entry_list[i].type) {
                    case G_CacheBindGroupEntryType_Buffer: {
                        wgpu_bg_entry_list[i].buffer
                          = bg_entry_list[i].as.buffer.buffer;
                        wgpu_bg_entry_list[i].offset
                          = bg_entry_list[i].as.buffer.offset;
                        wgpu_bg_entry_list[i].size = bg_entry_list[i].as.buffer.size;
                    } break;
                    case G_CacheBindGroupEntryType_Sampler: {
                        wgpu_bg_entry_list[i].sampler = bg_entry_list[i].as.sampler;
                    } break;
                    case G_CacheBindGroupEntryType_TextureView: {
                        wgpu_bg_entry_list[i].textureView
                          = _createTextureViewForBindgroup(
                            bg_entry_list[i].as.texture_view_desc);
                    } break;
                    default: UNREACHABLE;
                }
            }

            WGPUBindGroupDescriptor desc = {};
            desc.layout                  = layout;
            desc.entryCount              = bg_entry_count;
            desc.entries                 = wgpu_bg_entry_list;

            item.val.bg                  = wgpuDeviceCreateBindGroup(device, &desc);
            item.val.frames_till_expired = CHUGL_CACHE_BINDGROUP_FRAMES_TILL_EXPIRED;
            log_trace("created bind group %p", (void*)item.val.bg);

            const void* replaced = hashmap_set(bindgroup_map, &item);
            ASSERT(!replaced);

            return item.val.bg;
        }

        // reset lifetime counter
        result->val.frames_till_expired = CHUGL_CACHE_BINDGROUP_FRAMES_TILL_EXPIRED;
        return result->val.bg;
    }

    void update()
    {
        ASSERT(deletion_queue.curr == 0);

        // TODO: loop over all pipelines, and if associated R_Shader is destroyed,
        // free the pipeline and WGPU_RELEASE the cached bindgroup layouts

        // Actually, for compute pipelines, the graphics thread GC can explicitly
        // tell the rendergraph to release the compute pipeline and bind group layout!

        // loop over bindgroups. delete expired.
        // intentionally NOT refcounting WGPU resources here under assumption that
        // chuck-side refcounting will handle that for us
        size_t bindgroup_map_idx_DONT_USE = 0;
        G_CacheBindGroup* cache_bg        = NULL;
        while (
          hashmap_iter(bindgroup_map, &bindgroup_map_idx_DONT_USE, (void**)&cache_bg)) {
            if (--cache_bg->val.frames_till_expired <= 0) {
                *ARENA_PUSH_TYPE(&deletion_queue, G_CacheBindGroup) = *cache_bg;
            }
        }
        int num_bg_to_delete = ARENA_LENGTH(&deletion_queue, G_CacheBindGroup);
        for (int i = 0; i < num_bg_to_delete; i++) {
            G_CacheBindGroup* bg_del
              = ARENA_GET_TYPE(&deletion_queue, G_CacheBindGroup, i);
            ASSERT(bg_del->val.frames_till_expired <= 0);
            WGPU_RELEASE_RESOURCE(BindGroup, bg_del->val.bg);

            const void* deleted = hashmap_delete(bindgroup_map, bg_del);
            ASSERT(deleted);
            log_trace("deleting expired bindgroup %p", (void*)bg_del->val.bg);

            // deref all textureview bg entries
            for (int bg_entry_idx = 0; bg_entry_idx < bg_del->key.bg_entry_count;
                 ++bg_entry_idx) {
                G_CacheBindGroupEntry* entry = bg_del->key.bg_entry_list + bg_entry_idx;
                if (entry->type == G_CacheBindGroupEntryType_TextureView) {
                    // deref texview
                    G_CacheTextureView* view = (G_CacheTextureView*)hashmap_get(
                      texture_view_map, &entry->as.texture_view_desc);
                    ASSERT(view);
                    --view->val.refcount;
                    if (view->val.refcount == 0) {
                        log_trace("Cache deleting expired texture view");
                        WGPU_RELEASE_RESOURCE(TextureView, view->val.view);
                        const void* deleted = hashmap_delete(texture_view_map, view);
                        ASSERT(deleted);
                    }
                }
            }

            // G_Util::printBindGroupEntryList(bg_del->key.bg_entry_list,
            //                                 bg_del->key.bg_entry_count);
        }
        Arena::clearZero(&deletion_queue);
    };
};

enum G_RenderingLayer : u8 {
    G_RenderingLayer_World = 0x00,
    G_RenderingLayer_Background
    = 0x10, // draw background first bc world may have translucent objects
    G_RenderingLayer_Overlay
    = 0x20, // actually, would this just be a separate rendergraph node?
    // maybe don't need as many layers because the rendergraph acts as a layering
    // system...
};

/* sort key bit flags
opaque:         0LLLLLLL MMMMMMMM MMMMMMMM MMMMMMMM MMMMMMMM DDDDDDDD DDDDDDDD
DDDDDDDD translucent: :  1LLLLLLL DDDDDDDD DDDDDDDD DDDDDDDD MMMMMMMM MMMMMMMM
MMMMMMMM MMMMMMMM
*/
// ==optimize== store 16 bit offset of Material AND Shader in resource pool
// to minimize number of pipeline switches
struct G_SortKey {
    const static u64 TRANLUCENT_MASK = (1ULL << 63);
    const static u8 LAYER_MASK       = 0b01111111;
    const static u32 MATERIAL_MASK   = 0xFFFFFFFF;
    const static u32 DEPTH_MASK      = 0x00FFFFFF;
    const static u32 MAX_DEPTH       = 1000; // should be far plane of camera

    static u64 create(bool translucent, u8 layer, R_ID material_id, float depth,
                      float camera_far)
    {
        // validate
        ASSERT(layer <= LAYER_MASK);

        u64 sort_key = 0;
        sort_key |= (u64)(layer & LAYER_MASK) << 56ULL;

        // calculate depth
        // Input depth here is: 0 is the closest to the camera, positive values are
        // further away. Negative values (behind camera) are clamped to 0.
        // normalized_depth: 0.0 is closest to camera, 1.0 is farthest from camera.
        // These values are inverted if transparent because we want to render in
        // reverse order (back to front)
        float normalized_depth = CLAMP(depth, 0, camera_far) / camera_far;
        if (translucent) normalized_depth = 1.0 - normalized_depth;
        u32 depth_key = (u32)(normalized_depth * DEPTH_MASK) & DEPTH_MASK;

        if (translucent) {
            sort_key |= TRANLUCENT_MASK;
            sort_key |= ((u64)depth_key << 32ULL);
            sort_key |= material_id;
        } else {
            sort_key |= depth_key;
            sort_key |= (material_id << 24);
        }

        return sort_key;
    }
};

struct G_DrawCall {
    u64 sort_key;
    // name : string     = "";

    u32 vertex_count;
    struct {
        WGPUBuffer buffer;
        u64 offset;
        u64 size;
    } vertex_buffer_list[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    u32 index_count;
    WGPUBuffer index_buffer;
    u64 index_buffer_offset;
    u64 index_buffer_size;

    u32 instance_count;

    struct {
        u32 start, count;
    } bg_list[CHUGL_MAX_BINDGROUPS];

    // no dynamic offsets for now (until we make C port of webgpu-utils shader
    // parser)
    // dynamic_offsets = new Array<Array<number>>(4);

    G_DrawCallPipelineDesc _pipeline_desc;

    void pipelineDesc(SG_ID sg_shader_id, WGPUCullMode cull_mode,
                      WGPUPrimitiveTopology primitive_topology, bool is_transparent)
    {
        _pipeline_desc                    = {};
        _pipeline_desc.sg_shader_id       = sg_shader_id;
        _pipeline_desc.cull_mode          = cull_mode;
        _pipeline_desc.primitive_topology = primitive_topology;

        // doing this to avoid a horrible bug where setting a bool directly
        // didn't zero out all the padded memory
        _pipeline_desc.is_transparent = is_transparent ? 1UL : 0UL;
    }
};

struct G_DrawCallList {
    int drawcall_start_idx;
    int drawcall_count;
    bool sorted;

    // ==optimize== only rebind pipeline if it's actually changed
    // see R_RenderSceneOld for how
    void execute(WGPUDevice device,
                 WGPURenderPassEncoder pass_encoder, // TODO this comes from rendergraph
                 WGPUTextureFormat color_target_format,
                 WGPUTextureFormat depth_target_format, G_Cache* cache,
                 Arena* drawcall_pool, Arena bind_group_list[4])
    {
        G_DrawCall* start
          = ARENA_GET_TYPE(drawcall_pool, G_DrawCall, drawcall_start_idx);

        if (!sorted) {
            qsort(start, drawcall_count, sizeof(G_DrawCall),
                  [](const void* a, const void* b) -> int {
                      u64 sort_key_a = ((G_DrawCall*)a)->sort_key;
                      u64 sort_key_b = ((G_DrawCall*)b)->sort_key;

                      return sort_key_a - sort_key_b; // sorts in ascending order
                  });
            sorted = true;
        }

        // ==optimize== only need to update pipeline/bindgroup if they changed
        // so somehow track curr_pipeline, curr_bindgroup etc
        for (int i = 0; i < drawcall_count; i++) {
            G_DrawCall* d
              = ARENA_GET_TYPE(drawcall_pool, G_DrawCall, drawcall_start_idx + i);
            bool draw_indexed = (d->index_buffer != NULL);

#ifdef CHUGL_DEBUG // drawcall validation
            if (d->instance_count == 0) log_warn("drawcall instance count of 0");
            if (draw_indexed && d->index_count == 0)
                log_warn("drawcall index count of 0");
            if (!draw_indexed && d->vertex_count == 0)
                log_warn("drawcall vertex count of 0");
#endif

            G_CacheRenderPipeline* cached_pipeline = cache->renderPipeline(
              {
                d->_pipeline_desc,
                color_target_format,
                depth_target_format,
              },
              device);

            // set pipeline
            wgpuRenderPassEncoderSetPipeline(pass_encoder,
                                             cached_pipeline->val.pipeline);

            // set bindgroups
            int max_bg = ARRAY_LENGTH(d->bg_list);
            for (int bg_idx = 0; bg_idx < max_bg; bg_idx++) {
                int num_bindings = d->bg_list[bg_idx].count;
                int start        = d->bg_list[bg_idx].start;
                if (num_bindings > 0) {
                    WGPUBindGroup bg = cache->bindGroup(
                      device,
                      ARENA_GET_TYPE(bind_group_list + bg_idx, G_CacheBindGroupEntry,
                                     start),
                      num_bindings, cached_pipeline->val.bindGroupLayout(bg_idx),
                      bg_idx);
                    ASSERT(bg);
                    wgpuRenderPassEncoderSetBindGroup(pass_encoder, bg_idx, bg, 0,
                                                      NULL);
                }
            }

            // set vertex buffer
            for (int vertex_buffer_idx = 0;
                 vertex_buffer_idx < ARRAY_LENGTH(d->vertex_buffer_list);
                 vertex_buffer_idx++) {
                if (d->vertex_buffer_list[vertex_buffer_idx].buffer) {
                    wgpuRenderPassEncoderSetVertexBuffer(
                      pass_encoder, vertex_buffer_idx,
                      d->vertex_buffer_list[vertex_buffer_idx].buffer,
                      d->vertex_buffer_list[vertex_buffer_idx].offset,
                      d->vertex_buffer_list[vertex_buffer_idx].size);
                }
            }

            // set index buffer
            if (draw_indexed) {
                wgpuRenderPassEncoderSetIndexBuffer(
                  pass_encoder, d->index_buffer, WGPUIndexFormat_Uint32,
                  d->index_buffer_offset, d->index_buffer_size);
                wgpuRenderPassEncoderDrawIndexed(pass_encoder, d->index_count,
                                                 d->instance_count, 0, 0, 0);
            } else {
                wgpuRenderPassEncoderDraw(pass_encoder, d->vertex_count,
                                          d->instance_count, 0, 0);
            }
        }
    }
};

enum G_PassType : u8 {
    G_PassType_None = 0,
    G_PassType_Render,
    G_PassType_Compute,
    G_PassType_Count,
};

struct G_RenderTarget {
    WGPUTextureView texture_view;
    WGPUTextureFormat view_format;

    // // aka surface texture
    // G_RenderTarget canvas(bool srgb, GraphicsContext* gctx)
    // {
    //     G_RenderTarget rt = {};
    //     rt.type           = G_RenderTargetType_Canvas,
    //     rt.view_format
    //       = srgb ?
    //       G_Util::textureFormatSrgbVariant(gctx->surface_preferred_format) :
    //                gctx->surface_preferred_format;
    //     return rt;
    // }
};

typedef int G_DrawCallListID;
typedef int G_DrawCallID;

struct G_RenderPassParams {
    G_RenderTarget _color_target;
    WGPUColor clear_color;
    WGPULoadOp color_load_op;
    WGPUStoreOp color_store_op;

    G_RenderTarget _depth_target;

    float viewport_x;
    float viewport_y;
    float viewport_w;
    float viewport_h;
    bool viewport_custom;

    G_DrawCallListID drawcall_list_id;
};

struct G_ComputePassParams {
    WGPUShaderModule module;
    u32 x, y, z;            // workgroup size
    u32 bg_start, bg_count; // bindgroup entries
};

struct G_Pass {
    G_PassType type;
    char name[64];
    union {
        G_RenderPassParams rp;
        G_ComputePassParams cp;
    };
};

struct G_Graph {
    G_Cache cache;

    Arena bind_group_entry_list[CHUGL_MAX_BINDGROUPS]; // type G_CacheBindGroupEntry

    // drawcall pool
    Arena drawcall_pool;
    G_DrawCall* current_draw;

    G_DrawCallList drawcall_list_pool[CHUGL_RENDERGRAPH_MAX_PASSES];
    int drawcall_list_count;

    // pass pool
    G_Pass pass_list[CHUGL_RENDERGRAPH_MAX_PASSES];
    int pass_count;

    void init()
    {
        cache.init();
    }

    // renderpass methods ======================================
    void addRenderPass(const char* name)
    {
        if (pass_count == CHUGL_RENDERGRAPH_MAX_PASSES) {
            log_error("Reached max pass count %d", pass_count);
            return;
        }
        G_Pass* pass = &pass_list[pass_count++];
        pass->type   = G_PassType_Render;
        COPY_STRING(pass->name, name);
    }

    G_DrawCallListID renderPassAddDrawCallList()
    {
        G_Pass* pass = pass_list + (pass_count - 1);
        ASSERT(pass->type == G_PassType_Render);

        drawcall_list_pool[drawcall_list_count].drawcall_start_idx
          = ARENA_LENGTH(&drawcall_pool, G_DrawCall);

        pass->rp.drawcall_list_id = drawcall_list_count;
        return drawcall_list_count++;
    }

    void renderPassColorOp(WGPUColor color, WGPULoadOp load, WGPUStoreOp store)
    {
        G_Pass* pass = pass_list + (pass_count - 1);
        ASSERT(pass->type == G_PassType_Render);

        pass->rp.clear_color    = color;
        pass->rp.color_load_op  = load;
        pass->rp.color_store_op = store;
    }

    void renderPassColorTarget(WGPUTextureView view, WGPUTextureFormat format)
    {
        G_Pass* pass = pass_list + (pass_count - 1);
        ASSERT(pass->type == G_PassType_Render);
        WGPU_REFERENCE_RESOURCE(TextureView, view);
        pass->rp._color_target = { view, format };
    }

    void renderPassDepthTarget(WGPUTextureView view, WGPUTextureFormat format)
    {
        G_Pass* pass = pass_list + (pass_count - 1);
        ASSERT(pass->type == G_PassType_Render);
        ASSERT(G_Util::isDepthTextureFormat(format));
        WGPU_REFERENCE_RESOURCE(TextureView, view);
        pass->rp._depth_target = { view, format };
    }

    G_DrawCall*
    addDraw(G_DrawCallListID dc_list) // adds a draw to the last drawcall list
    {
        ASSERT(drawcall_list_count > 0);
        ASSERT(dc_list == drawcall_list_count - 1);

        ++drawcall_list_pool[dc_list].drawcall_count;
        G_DrawCall* draw = ARENA_PUSH_ZERO_TYPE(&drawcall_pool, G_DrawCall);

        for (int i = 0; i < CHUGL_MAX_BINDGROUPS; i++) {
            draw->bg_list[i].start
              = ARENA_LENGTH(bind_group_entry_list + i, G_CacheBindGroupEntry);
        }

        current_draw = draw;
        return draw;
    }

    void vertexBuffer(G_DrawCall* d, int slot, WGPUBuffer buffer, u64 offset, u64 size)
    {
        ASSERT(d == current_draw);
        d->vertex_buffer_list[slot] = { buffer, offset, size };
    }

    void bindBuffer(G_DrawCall* d, int group, int binding, WGPUBuffer buffer,
                    u32 offset, u32 size)
    {
        ASSERT(d == current_draw);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list + group, G_CacheBindGroupEntry);
        entry->type      = G_CacheBindGroupEntryType_Buffer;
        entry->binding   = binding;
        entry->as.buffer = { buffer, offset, size };

        ++d->bg_list[group].count;
    }

    void bindSampler(G_DrawCall* d, int group, int binding, WGPUSampler sampler)
    {
        ASSERT(d == current_draw);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list + group, G_CacheBindGroupEntry);
        entry->type       = G_CacheBindGroupEntryType_Sampler;
        entry->binding    = binding;
        entry->as.sampler = sampler;

        ++d->bg_list[group].count;
    }

    void bindTexture(G_DrawCall* d, int group, int binding, G_CacheTextureViewDesc desc)
    {
        ASSERT(d == current_draw);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list + group, G_CacheBindGroupEntry);
        entry->type                 = G_CacheBindGroupEntryType_TextureView;
        entry->binding              = binding;
        entry->as.texture_view_desc = desc;

        ++d->bg_list[group].count;
    }

    void computePassBindBuffer(int binding, WGPUBuffer buffer, u32 offset, u32 size)
    {
        ASSERT(pass_list[pass_count - 1].type == G_PassType_Compute);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list, G_CacheBindGroupEntry);
        entry->type      = G_CacheBindGroupEntryType_Buffer;
        entry->binding   = binding;
        entry->as.buffer = { buffer, offset, size };

        ++pass_list[pass_count - 1].cp.bg_count;
    }

    void computePassBindSampler(int binding, WGPUSampler sampler)
    {
        ASSERT(pass_list[pass_count - 1].type == G_PassType_Compute);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list, G_CacheBindGroupEntry);
        entry->type       = G_CacheBindGroupEntryType_Sampler;
        entry->binding    = binding;
        entry->as.sampler = sampler;

        ++pass_list[pass_count - 1].cp.bg_count;
    }

    void computePassBindTexture(int binding, G_CacheTextureViewDesc view)
    {
        ASSERT(pass_list[pass_count - 1].type == G_PassType_Compute);
        G_CacheBindGroupEntry* entry
          = ARENA_PUSH_ZERO_TYPE(bind_group_entry_list, G_CacheBindGroupEntry);
        entry->type                 = G_CacheBindGroupEntryType_TextureView;
        entry->binding              = binding;
        entry->as.texture_view_desc = view;

        ++pass_list[pass_count - 1].cp.bg_count;
    }

    void addComputePass(const char* name, WGPUShaderModule module, u32 x, u32 y, u32 z)
    {
        if (pass_count == CHUGL_RENDERGRAPH_MAX_PASSES) {
            log_error("Reached max pass count %d", pass_count);
            return;
        }
        G_Pass* compute_pass = &pass_list[pass_count++];
        COPY_STRING(compute_pass->name, name);
        compute_pass->type = G_PassType_Compute;
        compute_pass->cp
          = { module,
              x,
              y,
              z,
              (u32)ARENA_LENGTH(&bind_group_entry_list[0], G_CacheBindGroupEntry),
              0 };
    }

    void executeAndReset(WGPUDevice device, WGPUCommandEncoder command_encoder)
    {
        // TODO add debug labels
        // TODO add profiling/timing
        for (int i = 0; i < pass_count; i++) {
            G_Pass* pass = &this->pass_list[i];
            switch (pass->type) {
                case G_PassType_None: break;
                case G_PassType_Render: {
                    // log_trace("Beginning RenderPass: %s", pass->name);
                    // create pass desc
                    WGPURenderPassDescriptor render_pass_desc = {};
                    render_pass_desc.label                    = pass->name;
                    WGPURenderPassColorAttachment ca          = {};
                    WGPURenderPassDepthStencilAttachment ds   = {};

                    bool has_color_target
                      = (pass->rp._color_target.texture_view != NULL);
                    if (has_color_target) {
                        ca.view       = pass->rp._color_target.texture_view;
                        ca.loadOp     = pass->rp.color_load_op;
                        ca.storeOp    = pass->rp.color_store_op;
                        ca.clearValue = pass->rp.clear_color;
                        ca.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

                        render_pass_desc.colorAttachmentCount = 1;
                        render_pass_desc.colorAttachments     = &ca;
                    }

                    bool has_depth_target
                      = (pass->rp._depth_target.texture_view != NULL);
                    if (has_depth_target) {
                        ds.view = pass->rp._depth_target.texture_view;

                        // defaults for render pass depth/stencil attachment
                        // The initial value of the depth buffer, meaning "far"
                        ds.depthClearValue = 1.0f;
                        ds.depthLoadOp     = WGPULoadOp_Clear;
                        ds.depthStoreOp    = WGPUStoreOp_Store;
                        // we could turn off writing to the depth buffer globally
                        // here
                        ds.depthReadOnly = false;

                        // Stencil setup, mandatory but unused
                        ds.stencilClearValue = 0;
                        ds.stencilLoadOp     = WGPULoadOp_Clear;
                        ds.stencilStoreOp    = WGPUStoreOp_Store;
                        ds.stencilReadOnly   = false;

                        render_pass_desc.depthStencilAttachment = &ds;
                    }

                    // render_pass_desc.label = pass->sg_pass.name; // TODO

                    WGPURenderPassEncoder render_pass_encoder
                      = wgpuCommandEncoderBeginRenderPass(command_encoder,
                                                          &render_pass_desc);

                    if (pass->rp.viewport_custom) {
                        wgpuRenderPassEncoderSetViewport(
                          render_pass_encoder, pass->rp.viewport_x, pass->rp.viewport_y,
                          pass->rp.viewport_w, pass->rp.viewport_h, 0.0, 1.0);
                    }

                    ASSERT(pass->rp.drawcall_list_id >= 0
                           && pass->rp.drawcall_list_id < drawcall_list_count);
                    drawcall_list_pool[pass->rp.drawcall_list_id].execute(
                      device, render_pass_encoder, pass->rp._color_target.view_format,
                      pass->rp._depth_target.view_format, &cache, &drawcall_pool,
                      bind_group_entry_list);
                    wgpuRenderPassEncoderEnd(render_pass_encoder);
                    WGPU_RELEASE_RESOURCE(RenderPassEncoder, render_pass_encoder);

                    // release render target texture views
                    WGPU_RELEASE_RESOURCE(TextureView,
                                          pass->rp._color_target.texture_view);
                    WGPU_RELEASE_RESOURCE(TextureView,
                                          pass->rp._depth_target.texture_view);
                } break;
                case G_PassType_Compute: {
                    G_CacheComputePipeline cp
                      = cache.computePipeline(pass->cp.module, device, NULL);
                    WGPUComputePassDescriptor cp_desc = {};
                    cp_desc.label                     = pass->name;
                    WGPUComputePassEncoder compute_pass
                      = wgpuCommandEncoderBeginComputePass(command_encoder, NULL);
                    wgpuComputePassEncoderSetPipeline(compute_pass, cp.val.pipeline);

                    const int compute_pass_binding_location = 0;
                    WGPUBindGroup bg                        = cache.bindGroup(
                      device,
                      ARENA_GET_TYPE(bind_group_entry_list, G_CacheBindGroupEntry,
                                                            pass->cp.bg_start),
                      pass->cp.bg_count, cp.val.bind_group_layout,
                      compute_pass_binding_location);
                    wgpuComputePassEncoderSetBindGroup(
                      compute_pass, compute_pass_binding_location, bg, 0, NULL);

                    // dispatch
                    wgpuComputePassEncoderDispatchWorkgroups(compute_pass, pass->cp.x,
                                                             pass->cp.y, pass->cp.z);

                    // cleanup
                    wgpuComputePassEncoderEnd(compute_pass);
                    WGPU_RELEASE_RESOURCE(ComputePassEncoder, compute_pass);
                } break;
                default: UNREACHABLE
            }
        }

        // reset ----------------------------------------
        current_draw = NULL;

        for (int i = 0; i < ARRAY_LENGTH(bind_group_entry_list); i++) {
            Arena::clear(bind_group_entry_list + i);
        }

        ZERO_ARRAY(pass_list);
        pass_count = 0;

        ZERO_ARRAY(drawcall_list_pool);
        drawcall_list_count = 0;

        // TODO ==optimize== add bindgroup pool
        Arena::clear(&drawcall_pool);

        cache.update();
    }
};
