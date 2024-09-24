#pragma once

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

// =============================================================================
// scenegraph data structures
// =============================================================================

struct Vertices;
struct R_Material;
struct R_Scene;
struct R_Font;
struct hashmap;

typedef SG_ID R_ID; // negative for R_Components NOT mapped to SG_Components

struct R_Component {
    SG_ID id; // SG_Component this R_Component is mapped to
    SG_ComponentType type;
    std::string name = ""; // TODO move off std::string
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
    GPU_Buffer pull_buffers[SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS];
    WGPUBindGroup pull_bind_group;
    int vertex_count  = -1; // if set, overrides vertex count from vertices
    int indices_count = -1; // if set, overrides index count from indices
    bool pull_bind_group_dirty;

    static void init(R_Geometry* geo);

    static u32 indexCount(R_Geometry* geo);
    static u32 vertexCount(R_Geometry* geo);
    static u32 vertexAttributeCount(R_Geometry* geo);

    static void buildFromVertices(GraphicsContext* gctx, R_Geometry* geo,
                                  Vertices* vertices);

    static void setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo, u32 location,
                                   u32 num_components, f32* data, u32 data_count);
    static void setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo, u32 location,
                                   u32 num_components_per_attrib, void* data,
                                   size_t size);

    // TODO if works, move into cpp
    // TODO move vertexPulling reflection check into state of ck ShaderDesc
    static bool usesVertexPulling(R_Geometry* geo);

    static void rebuildPullBindGroup(GraphicsContext* gctx, R_Geometry* geo,
                                     WGPUBindGroupLayout layout);

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

    // TODO maybe remove texture view
    // eventually migrate to flat_map cached system
    // where each R_Texture has a hashmap : TextureViewDesc --> TextureView
    // and all R_Material have references to TextureViews which are owned by R_Texture.
    // Updating the generation walks the flatmap and recreates all TextureViews
    // for now sticking with awkward situation where some texture bindings create their
    // own TextureViews, and texture_id uses this default gpu_texture_view
    WGPUTextureView gpu_texture_view; // default view of entire gpu_texture + mip chain

    u32 generation = 0;  // incremented every time texture is modified
    SG_TextureDesc desc; // TODO redundant with R_Texture.gpu_texture

    static void init(GraphicsContext* gctx, R_Texture* texture, SG_TextureDesc* desc)
    {
        // free previous
        WGPU_RELEASE_RESOURCE(Texture, texture->gpu_texture);

        // bump generation
        texture->generation++;

        { // validation
            ASSERT(desc->mips >= 1
                   && desc->mips <= G_mipLevels(desc->width, desc->height));
            ASSERT(desc->width > 0 && desc->height > 0 && desc->depth > 0);
        }

        // copy texture info (immutable)
        texture->desc = *desc;

        // init descriptor
        WGPUTextureDescriptor wgpu_texture_desc = {};
        wgpu_texture_desc.label                 = texture->name.c_str();
        wgpu_texture_desc.usage                 = desc->usage;
        wgpu_texture_desc.dimension             = desc->dimension;
        wgpu_texture_desc.size
          = { (u32)desc->width, (u32)desc->height, (u32)desc->depth };
        wgpu_texture_desc.format        = desc->format;
        wgpu_texture_desc.mipLevelCount = desc->mips;
        wgpu_texture_desc.sampleCount   = 1;

        texture->gpu_texture
          = wgpuDeviceCreateTexture(gctx->device, &wgpu_texture_desc);
        ASSERT(texture->gpu_texture);

        // create default texture view for entire mip chain (And 1st array layer)
        // cubemaps are handled differently
        char texture_view_label[256] = {};
        snprintf(texture_view_label, sizeof(texture_view_label), "%s default view",
                 texture->name.c_str());
        WGPUTextureViewDescriptor wgpu_texture_view_desc = {};
        wgpu_texture_view_desc.label                     = texture_view_label;
        wgpu_texture_view_desc.format                    = desc->format;
        wgpu_texture_view_desc.dimension                 = WGPUTextureViewDimension_2D;
        wgpu_texture_view_desc.baseMipLevel              = 0;
        wgpu_texture_view_desc.mipLevelCount             = desc->mips;
        wgpu_texture_view_desc.baseArrayLayer            = 0;
        wgpu_texture_view_desc.arrayLayerCount           = desc->depth;

        texture->gpu_texture_view
          = wgpuTextureCreateView(texture->gpu_texture, &wgpu_texture_view_desc);
        ASSERT(texture->gpu_texture_view);
    }

    // resizes texture and updates generation, clears any previous data
    // used for auto-resizing framebuffer attachments
    static void resize(GraphicsContext* gctx, R_Texture* r_tex, u32 width, u32 height)
    {
        bool needs_resize = r_tex->desc.width != (int) width || r_tex->desc.height != (int) height
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
            source.rowsPerImage = write_desc->height * write_desc->depth;

            WGPUExtent3D size = { (u32)write_desc->width, (u32)write_desc->height,
                                  (u32)write_desc->depth };
            wgpuQueueWriteTexture(gctx->queue, &destination, data, data_size_bytes,
                                  &source, &size);

            wgpuQueueSubmit(gctx->queue, 0, NULL); // schedule transfer immediately
        }
    }

    static void load(GraphicsContext* gctx, R_Texture* texture, const char* filepath,
                     bool flip_vertically, bool gen_mips);
};

void Material_batchUpdatePipelines(GraphicsContext* gctx, FT_Library ft_lib,
                                   R_Font* default_font);

// =============================================================================
// R_Shader
// =============================================================================

struct R_Shader : public R_Component {
    WGPUShaderModule vertex_shader_module;
    WGPUShaderModule fragment_shader_module;
    WGPUVertexFormat vertex_layout[R_GEOMETRY_MAX_VERTEX_ATTRIBUTES];

    WGPUShaderModule compute_shader_module;
    bool lit;

    static void init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                     const char* vertex_filepath, const char* fragment_string,
                     const char* fragment_filepath, WGPUVertexFormat* vertex_layout,
                     int vertex_layout_count, const char* compute_string,
                     const char* compute_filepath, bool lit);

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
    R_BIND_TEXTURE_ID,   // for scenegraph textures
    R_BIND_TEXTURE_VIEW, // default textures (e.g. white pixel)
    R_BIND_STORAGE,
    R_BIND_STORAGE_EXTERNAL,   // pointer to external storage buffer (ref)
    R_BIND_STORAGE_TEXTURE_ID, // for scenegraph textures
};

// TODO can we move R_Binding into .cpp
struct R_Binding {
    R_BindType type;
    size_t size;    // size of data in bytes for UNIFORM and STORAGE types
    u64 generation; // currently only used for textures, track generation so we know
                    // when to rebuild BindGroup
                    // eventually can use to track GPU_Buffer generation
    union {
        SG_ID textureID;
        WGPUTextureView textureView;
        SamplerConfig samplerConfig;
        GPU_Buffer storage_buffer;
        GPU_Buffer* storage_external; // ptr here might be dangerous...
    } as;
};

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

    b32 bind_group_stale; // set if modified by chuck user, need to rebuild bind groups

    R_ID pipelineID = true; // renderpipeline this material belongs to
    bool pipeline_stale;

    // bindgroup state (uniforms, storage buffers, textures, samplers)
    R_Binding bindings[SG_MATERIAL_MAX_UNIFORMS];
    GPU_Buffer uniform_buffer; // maps 1:1 with uniform location, initializesd in
                               // Component_MaterialCreate
    WGPUBindGroup bind_group;

    static void updatePSO(GraphicsContext* gctx, R_Material* mat,
                          SG_MaterialPipelineState* pso);

    // bind group fns --------------------------------------------
    static void rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                 WGPUBindGroupLayout layout);
    static void setBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                           R_BindType type, void* data, size_t bytes);
    static void setUniformBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  void* data, size_t bytes)
    {
        setBinding(gctx, mat, location, R_BIND_UNIFORM, data, bytes);
    }
    static void setSamplerBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  SG_Sampler sampler);
    static void setTextureBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                  SG_ID texture_id);
    static void setTextureViewBinding(GraphicsContext* gctx, R_Material* mat,
                                      u32 location, WGPUTextureView view);
    static void setExternalStorageBinding(GraphicsContext* gctx, R_Material* mat,
                                          u32 location, GPU_Buffer* buffer);

    static void setStorageTextureBinding(GraphicsContext* gctx, R_Material* mat,
                                         u32 location, SG_ID texture_id);

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

struct MaterialToGeometry {
    SG_ID material_id;   // key
    Arena geo_ids;       // value, array of SG_IDs
    hashmap* geo_id_set; // kept in sync with geo_ids, use for quick lookup

    static void addGeometry(MaterialToGeometry* m2g, SG_ID geo_id)
    {
        // first check if already exists
        if (hashmap_get(m2g->geo_id_set, &geo_id) == NULL) {
            *ARENA_PUSH_TYPE(&m2g->geo_ids, SG_ID) = geo_id;
            hashmap_set(m2g->geo_id_set, &geo_id);
        }
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        return ((MaterialToGeometry*)a)->material_id
               - ((MaterialToGeometry*)b)->material_id;
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        MaterialToGeometry* key = (MaterialToGeometry*)item;
        return hashmap_xxhash3(&key->material_id, sizeof(key->material_id), seed0,
                               seed1);
    }

    static void free(void* item)
    {
        MaterialToGeometry* key = (MaterialToGeometry*)item;
        Arena::free(&key->geo_ids);
        hashmap_free(key->geo_id_set);
    }
};

struct GeometryToXformKey {
    SG_ID geo_id;
    SG_ID mat_id;
};

struct GeometryToXforms {
    GeometryToXformKey key;
    Arena xform_ids;       // value, array of SG_IDs
    hashmap* xform_id_set; // kept in sync with xform_ids, use for quick lookup
    WGPUBindGroup xform_bind_group;
    GPU_Buffer xform_storage_buffer;
    bool stale;

    static bool hasXform(GeometryToXforms* g2x, SG_ID xform_id)
    {
        return hashmap_get(g2x->xform_id_set, &xform_id) != NULL;
    }

    static void addXform(GeometryToXforms* g2x, SG_ID xform_id)
    {
        // first check if already exists
        if (hashmap_get(g2x->xform_id_set, &xform_id) == NULL) {
            *ARENA_PUSH_TYPE(&g2x->xform_ids, SG_ID) = xform_id;
            hashmap_set(g2x->xform_id_set, &xform_id);
            g2x->stale = true;
        }
    }

    static void removeXform(GeometryToXforms* g2x, size_t xform_id_index)
    {
        // note: we don't set stale here because removal happens during lazy-deletion,
        // when we are rebuilding a fresh bindgroup

        SG_ID xform_id      = *ARENA_GET_TYPE(&g2x->xform_ids, SG_ID, xform_id_index);
        const void* removed = hashmap_delete(g2x->xform_id_set, &xform_id);
        ASSERT(removed);

        // swap with last element
        *ARENA_GET_TYPE(&g2x->xform_ids, SG_ID, xform_id_index)
          = *ARENA_GET_LAST_TYPE(&g2x->xform_ids, SG_ID);
        ARENA_POP_TYPE(&g2x->xform_ids, SG_ID);
    }

    static int compare(const void* a, const void* b, void* udata)
    {
        GeometryToXforms* ga = (GeometryToXforms*)a;
        GeometryToXforms* gb = (GeometryToXforms*)b;
        return memcmp(&ga->key, &gb->key, sizeof(ga->key));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        GeometryToXforms* g2x = (GeometryToXforms*)item;
        return hashmap_xxhash3(&g2x->key, sizeof(g2x->key), seed0, seed1);
    }

    static void free(void* item)
    {
        GeometryToXforms* g2x = (GeometryToXforms*)item;
        Arena::free(&g2x->xform_ids);
        WGPU_RELEASE_RESOURCE(BindGroup, g2x->xform_bind_group);
        GPU_Buffer::destroy(&g2x->xform_storage_buffer);
        hashmap_free(g2x->xform_id_set);
    }

    static void rebuildBindGroup(GraphicsContext* gctx, R_Scene* scene,
                                 GeometryToXforms* g2x, WGPUBindGroupLayout layout,
                                 Arena* frame_arena);
};

struct R_Scene : R_Transform {
    SG_SceneDesc sg_scene_desc;

    hashmap* pipeline_to_material; // R_ID -> Arena of R_Material ids
    hashmap* material_to_geo;      // SG_ID -> Arena of geo ids
    hashmap* geo_to_xform;         // SG_ID -> Arena of xform ids (for each material)

    hashmap* light_id_set;        // set of SG_IDs
    GPU_Buffer light_info_buffer; // lighting storage buffer

    static void initFromSG(GraphicsContext* gctx, R_Scene* r_scene, SG_ID scene_id,
                           SG_SceneDesc* sg_scene_desc);

    static void removeSubgraphFromRenderState(R_Scene* scene, R_Transform* xform);
    static void addSubgraphToRenderState(R_Scene* scene, R_Transform* xform);

    static void rebuildLightInfoBuffer(GraphicsContext* gctx, R_Scene* scene, u64 fc);

    static i32 numLights(R_Scene* scene)
    {
        return (i32)hashmap_count(scene->light_id_set);
    }

    static void registerMesh(R_Scene* scene, R_Transform* mesh);
    static GeometryToXforms* getPrimitive(R_Scene* scene, SG_ID geo_id, SG_ID mat_id);
    static MaterialToGeometry* getMaterialToGeometry(R_Scene* scene, SG_ID mat_id);

    // static void free(R_Scene* scene);
};

// =============================================================================
// R_RenderPipeline
// =============================================================================

struct R_RenderPipeline /* NOT backed by SG_Component */ {
    R_ID rid;
    // RenderPipeline pipeline;
    WGPURenderPipeline gpu_pipeline;
    SG_MaterialPipelineState pso;
    // ptrdiff_t offset; // acts as an ID, offset in bytes into pipeline Arena

    Arena materialIDs; // array of SG_IDs

    // RenderPass refactor: removing
    // bindgroup now in App->frame_uniforms_map
    // frame uniform buffer now stored per R_Camera
    // keeping these around to handle case of null camera
    // TODO remove after removing null default camera in chugl (and implementing
    // camera controllers)
    WGPUBindGroup frame_group;
    static GPU_Buffer frame_uniform_buffer;

    WGPUBindGroupLayout bind_group_layouts[4];

    /*
    possible optimizations:
    - keep material IDs with nonzero #primitive contiguous
      so we don't waste time iterating over empty materials
    */

    // static void init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
    //                  const SG_MaterialPipelineState* config, ptrdiff_t offset);
    static void init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                     const SG_MaterialPipelineState* config, int msaa_sample_count = 4);

    static void addMaterial(R_RenderPipeline* pipeline, R_Material* material);

    /// @brief Iterator for materials tied to render pipeline
    static size_t numMaterials(R_RenderPipeline* pipeline);
    static bool materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                             R_Material** material);
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
            log_debug("rebuilding framebuffer");
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

        pass->screen_pass_desc                        = {};
        pass->screen_pass_desc.label                  = pass->sg_pass.name;
        pass->screen_pass_desc.colorAttachmentCount   = 1;
        pass->screen_pass_desc.colorAttachments       = pass->screen_color_attachments;
        pass->screen_pass_desc.depthStencilAttachment = NULL;
    }

    // if window size has changed, lazily reconstruct depth/stencil and color
    // targets update DepthStencilAttachment and ColorAttachment params based on
    // ChuGL RenderPass params this should be called right before _R_RenderScene()
    // for the given pass. R_Pass.sg_pass is assumed to be updated (memcopied in the
    // updatePass Command), but not the gpu-specific parameters of R_Pass
    static void updateRenderPassDesc(GraphicsContext* gctx, R_Pass* pass,
                                     u32 window_width, u32 window_height,
                                     int sample_count, WGPUTextureView resolve_view,
                                     WGPUTextureFormat view_format,
                                     glm::vec4 clear_color)
    {
        ASSERT(pass->sg_pass.pass_type == SG_PassType_Render);

        // handle resize
        Framebuffer::rebuild(gctx, &pass->framebuffer, window_width, window_height,
                             sample_count, view_format);

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
            ca->view
              = pass->framebuffer.color_view; // multisampled view, for now lock down
            ca->resolveTarget = resolve_view;

#ifdef __EMSCRIPTEN__
            ca->depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
#endif
            // TODO chugl API to set loadOp
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

struct R_ScreenPassPipeline {
    WGPUTextureFormat format;
    SG_ID shader_id;
    WGPURenderPipeline gpu_pipeline;
    WGPUBindGroupLayout frame_group_layout;
};
R_ScreenPassPipeline R_GetScreenPassPipeline(GraphicsContext* gctx,
                                             WGPUTextureFormat format, SG_ID shader_id);

struct R_ComputePassPipeline {
    SG_ID shader_id;
    WGPUComputePipeline gpu_pipeline;
    WGPUBindGroupLayout bind_group_layout;
};
R_ComputePassPipeline R_GetComputePassPipeline(GraphicsContext* gctx, R_Shader* shader);

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
// Component Manager API
// =============================================================================

R_Transform* Component_CreateTransform();
R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd);

R_Transform* Component_CreateMesh(SG_ID mesh_id, SG_ID geo_id, SG_ID mat_id);
R_Camera* Component_CreateCamera(GraphicsContext* gctx, SG_Command_CameraCreate* cmd);
R_Text* Component_CreateText(GraphicsContext* gctx, FT_Library ft,
                             SG_Command_TextRebuild* cmd);

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

R_Component* Component_GetComponent(SG_ID id);
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

// lazily created on-demand because of many possible shader variations
R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        SG_MaterialPipelineState* pso);
R_RenderPipeline* Component_GetPipeline(R_ID rid);

// component iterators
// bool hashmap_scan(struct hashmap *map, bool (*iter)(const void *item, void
// *udata), void *udata);

// be careful to not delete components while iterating
// returns false upon reachign end of material arena
bool Component_MaterialIter(size_t* i, R_Material** material);
bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline** renderPipeline);
int Component_RenderPipelineCount();

// component manager initialization
void Component_Init(GraphicsContext* gctx);
void Component_Free();

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
