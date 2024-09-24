#include "r_component.h"
#include "core/hashmap.h"
#include "geometry.h"
#include "graphics.h"
#include "shaders.h"

#include "compressed_fonts.h"

#include "core/file.h"
#include "core/log.h"

#include <stb/stb_image.h>

#include <glm/gtx/matrix_decompose.hpp>

static int compareSGIDs(const void* a, const void* b, void* udata)
{
    return *(SG_ID*)a - *(SG_ID*)b;
}

static uint64_t hashSGID(const void* item, uint64_t seed0, uint64_t seed1)
{
    return hashmap_xxhash3(item, sizeof(SG_ID), seed0, seed1);
}

/*
XForm system:
- each XForm component is marked with a stale flag NONE/DESCENDENTS/WORLD/LOCAL
- when an XForm is reparented or moved, it marks itself as stale WORLD/LOCAL and
all its parents as DESCENDENTS
    - DESCENDENTS means my own transform is fine, but at least 1 descendent
needs to be recomputed
    - WORLD means my local matrix is fine, but my world matrix needs to be
recomputed
    - LOCAL means both local and world matrices need to be recomputed
- at the start of each render, after all updates, the graphics thread needs to
call R_Transform::rebuildMatrices(root) to update all world matrices
    - the staleness flags optimizes this process by ignoring all branches that
don't require updating
    - during this rebuild, transforms that have been marked as WORLD or LOCAL
will also mark their geometry as stale

Geometry system:
- all draws are instanced. All xforms bound to a certain geometry and material
will be rendered in a single draw call
- each geometry component has a storage buffer that holds all the world matrices
of its associated xform instances
- if a xform is moved or reparented, the geometry component will be marked
stale, and the storage buffer will be rebuilt via R_Geometry::rebuildBindGroup

Component Manager:
- handles all creation and deletion of components
- components (Xforms, Geos ...) are stored in contiguous Arena memory
- deletions are handled by swapping the deleted component with the last
component
- each component has a unique ID
- a hashmap stores the ID to offset mapping
  - store offset and not pointer because pointers can be invalidated if the
arena grows, or elements within are deleted and swapped

Note: the stale flag scenegraph system allows for a nice programming pattern
in ChucK:

// put all objects that can move under here
GGen dynamic --> GG.scene();
// put all immobile objects here
GGen static --> GG.scene();

all static GGens are grouped under a single XForm, and this entire branch of the
scenegraph will be skipped every frame!

*/

// ============================================================================
// Forward Declarations
// ============================================================================
//
// set for materials to create pipelines for
static hashmap* materials_with_new_pso = NULL;

static SG_ID _componentIDCounter = 1; // reserve 0 for NULL

// All R_IDs are negative to avoid conflict with positive SG_IDs
static R_ID _R_IDCounter = -1; // reserve 0 for NULL.

static SG_ID getNewComponentID()
{
    return _componentIDCounter++;
}

static R_ID getNewRID()
{
    return _R_IDCounter--;
}

// ============================================================================
// Transform Component
// ============================================================================

static void R_Transform_init(R_Transform* xform, SG_ID id, SG_ComponentType comp_type)
{
    *xform = {};

    xform->id   = id;
    xform->type = comp_type;

    xform->_pos = glm::vec3(0.0f);
    xform->_rot = QUAT_IDENTITY;
    xform->_sca = glm::vec3(1.0f);

    xform->world  = MAT_IDENTITY;
    xform->local  = MAT_IDENTITY;
    xform->_stale = R_Transform_STALE_LOCAL;

    xform->parentID = 0;

    // initialize children array for 8 children
    Arena::init(&xform->children, sizeof(SG_ID) * 8);
}

void R_Transform::init(R_Transform* transform)
{
    ASSERT(transform->id == 0); // ensure not initialized twice
    *transform = {};

    transform->id   = getNewComponentID();
    transform->type = SG_COMPONENT_TRANSFORM;

    transform->_pos = glm::vec3(0.0f);
    transform->_rot = QUAT_IDENTITY;
    transform->_sca = glm::vec3(1.0f);

    transform->world  = MAT_IDENTITY;
    transform->local  = MAT_IDENTITY;
    transform->_stale = R_Transform_STALE_NONE;

    transform->parentID = 0;
    // initialize children array for 8 children
    Arena::init(&transform->children, sizeof(SG_ID) * 8);
}

void R_Transform::initFromSG(R_Transform* r_xform, SG_Command_CreateXform* cmd)
{
    ASSERT(r_xform->id == 0); // ensure not initialized twice
    *r_xform = {};

    // copy base component data
    // TODO have a separate R_ComponentType enum?
    r_xform->id   = cmd->sg_id;
    r_xform->type = SG_COMPONENT_TRANSFORM;

    // copy xform
    r_xform->_pos   = cmd->pos;
    r_xform->_rot   = cmd->rot;
    r_xform->_sca   = cmd->sca;
    r_xform->_stale = R_Transform_STALE_LOCAL;

    // initialize children array for 8 children
    Arena::init(&r_xform->children, sizeof(SG_ID) * 8);
}

void R_Transform::setStale(R_Transform* xform, R_Transform_Staleness stale)
{
    // only set if new staleness is higher priority
    if (stale > xform->_stale) xform->_stale = stale;

    // propagate staleness to parent
    // it is assumed that if a parent has staleness, all its parents will
    // also have been marked with prio at least R_Transform_STALE_DESCENDENTS
    while (xform->parentID != 0) {
        R_Transform* parent = Component_GetXform(xform->parentID);
        // upwards stale chain already established. skip
        if (parent->_stale > R_Transform_STALE_NONE) break;
        // otherwise set parent staleness and continue propagating
        parent->_stale = R_Transform_STALE_DESCENDENTS;
        xform          = parent;
    }
}

R_Scene* R_Transform::getScene(R_Transform* xform)
{
    // walk up parent chain until scene is found
    while (xform) {
        if (xform->type == SG_COMPONENT_SCENE) return (R_Scene*)xform;
        xform = Component_GetXform(xform->parentID);
    }

    return NULL;
}

bool R_Transform::isAncestor(R_Transform* ancestor, R_Transform* descendent)
{
    while (descendent != NULL) {
        if (descendent == ancestor) return true;
        descendent = Component_GetXform(descendent->parentID);
    }
    return false;
}

void R_Transform::removeChild(R_Transform* parent, R_Transform* child)
{
    if (child->parentID != parent->id) {
        log_error("cannot remove a child who does not belong to parent");
        return;
    }

    size_t numChildren = ARENA_LENGTH(&parent->children, SG_ID);
    SG_ID* children    = (SG_ID*)parent->children.base;

    // remove child's parent reference
    child->parentID = 0;

    // remove from parent
    for (size_t i = 0; i < numChildren; ++i) {
        if (children[i] == child->id) {
            // swap with last element
            children[i] = children[numChildren - 1];
            // pop last element
            Arena::pop(&parent->children, sizeof(SG_ID));
            break;
        }
    }

    // remove child subgraph from scene render state
    R_Scene* scene = R_Transform::getScene(parent);
    R_Scene::removeSubgraphFromRenderState(scene, child);
}

void R_Transform::removeAllChildren(R_Transform* parent)
{
    if (!parent) return;

    R_Scene* scene     = R_Transform::getScene(parent);
    size_t numChildren = ARENA_LENGTH(&parent->children, SG_ID);
    SG_ID* children    = (SG_ID*)parent->children.base;

    for (size_t i = 0; i < numChildren; ++i)
        R_Scene::removeSubgraphFromRenderState(scene, Component_GetXform(children[i]));

    Arena::clear(&parent->children);
}

void R_Transform::addChild(R_Transform* parent, R_Transform* child)
{
    if (R_Transform::isAncestor(child, parent)) {
        log_error("No cycles in scenegraph; cannot add parent as child of descendent");
        return;
    }

    if (parent == NULL || child == NULL) {
        log_error("Cannot add NULL parent or child to scenegraph");
        return;
    }

    // relationship already in place, do noting
    if (child->parentID == parent->id) return;

    // remove child from previous parent
    if (child->parentID != 0) {
        R_Transform* prevParent = Component_GetXform(child->parentID);
        R_Transform::removeChild(prevParent, child);
    }

    // set parent of child
    child->parentID = parent->id;

    // add child to parent
    SG_ID* xformID = ARENA_PUSH_ZERO_TYPE(&parent->children, SG_ID);
    *xformID       = child->id;

    R_Transform::setStale(child, R_Transform_STALE_WORLD);

    // add child subgraph to scene render state
    R_Scene* scene = R_Transform::getScene(parent);
    R_Scene::addSubgraphToRenderState(scene, child);
}

glm::mat4 R_Transform::localMatrix(R_Transform* xform)
{
    glm::mat4 M = glm::mat4(1.0);
    M           = glm::translate(M, xform->_pos);
    M           = M * glm::toMat4(xform->_rot);
    M           = glm::scale(M, xform->_sca);
    return M;
}

/// @brief decompose matrix into transform data
void R_Transform::setXformFromMatrix(R_Transform* xform, const glm::mat4& M)
{
    log_trace("decomposing matrix");
    xform->_pos  = glm::vec3(M[3]);
    xform->_rot  = glm::quat_cast(M);
    xform->_sca  = glm::vec3(glm::length(M[0]), glm::length(M[1]), glm::length(M[2]));
    xform->local = M;

    // log_trace("pos: %s", glm::to_string(xform->_pos).c_str());
    // log_trace("rot: %s", glm::to_string(xform->_rot).c_str());
    // log_trace("sca: %s", glm::to_string(xform->_sca).c_str());

    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::setXform(R_Transform* xform, const glm::vec3& pos,
                           const glm::quat& rot, const glm::vec3& sca)
{
    xform->_pos = pos;
    xform->_rot = rot;
    xform->_sca = sca;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::pos(R_Transform* xform, const glm::vec3& pos)
{
    xform->_pos = pos;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::rot(R_Transform* xform, const glm::quat& rot)
{
    xform->_rot = rot;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::sca(R_Transform* xform, const glm::vec3& sca)
{
    xform->_sca = sca;
    R_Transform::setStale(xform, R_Transform_STALE_LOCAL);
}

void R_Transform::decomposeWorldMatrix(const glm::mat4& m, glm::vec3& pos,
                                       glm::quat& rot, glm::vec3& scale)
{
    // pos = m[3];
    // for (int i = 0; i < 3; i++) scale[i] = glm::length(glm::vec3(m[i]));
    // const glm::mat3 rotMtx(glm::vec3(m[0]) / scale[0], glm::vec3(m[1]) / scale[1],
    //                        glm::vec3(m[2]) / scale[2]);
    // rot = glm::quat_cast(rotMtx);
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::decompose(m, scale, rot, pos, skew, perspective);
}

/// @brief recursive helper to regen matrices for xform and all descendents
static void _Transform_RebuildDescendants(R_Scene* scene, R_Transform* xform,
                                          const glm::mat4* parentWorld)
{
    // mark primitive as stale since world matrix will change
    if (xform->_geoID && xform->_matID) {
        ASSERT(xform->type == SG_COMPONENT_MESH || xform->type == SG_COMPONENT_TEXT);
        R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID)->stale = true;
    }

    // TODO ==optimize==: this is where we would mark lights as stale
    // For now we rebuild the light storage buffer every frame, no memoization

    // rebuild local mat
    if (xform->_stale == R_Transform_STALE_LOCAL)
        xform->local = R_Transform::localMatrix(xform);

    // always rebuild world mat
    xform->world = (*parentWorld) * xform->local;

    // set fresh
    xform->_stale = R_Transform_STALE_NONE;

    // rebuild all children
    for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
        R_Transform* child
          = Component_GetXform(*ARENA_GET_TYPE(&xform->children, SG_ID, i));
        ASSERT(child != NULL);
        _Transform_RebuildDescendants(scene, child, &xform->world);
    }
}

void R_Transform::rebuildMatrices(R_Scene* root, Arena* arena)
{
    // push root onto stack
    SG_ID* base = ARENA_PUSH_ZERO_TYPE(arena, SG_ID);
    *base       = root->id;
    SG_ID* top  = (SG_ID*)Arena::top(arena);

    ASSERT(base + 1 == top);

    glm::mat4 identityMat = MAT_IDENTITY;

    // while stack is not empty
    while (top != base) {
        // pop id from stack
        SG_ID xformID = top[-1];
        Arena::pop(arena, sizeof(SG_ID));
        R_Transform* xform = Component_GetXform(xformID);
        ASSERT(xform != NULL);

        switch (xform->_stale) {
            case R_Transform_STALE_NONE: break;
            case R_Transform_STALE_DESCENDENTS: {
                // add to stack
                SG_ID* children = (SG_ID*)xform->children.base;
                for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
                    SG_ID* childID = ARENA_PUSH_ZERO_TYPE(arena, SG_ID);
                    *childID       = children[i];
                }
                break;
            }
            case R_Transform_STALE_WORLD:
            case R_Transform_STALE_LOCAL: {
                // get parent world matrix
                R_Transform* parent = Component_GetXform(xform->parentID);
                _Transform_RebuildDescendants(root, xform,
                                              parent ? &parent->world : &identityMat);
                break;
            }
            default: log_error("unhandled staleness %d", xform->_stale); break;
        }

        // always set fresh
        xform->_stale = R_Transform_STALE_NONE;

        // update stack top
        top = (SG_ID*)Arena::top(arena);
    }
}

u32 R_Transform::numChildren(R_Transform* xform)
{
    return ARENA_LENGTH(&xform->children, SG_ID);
}

R_Transform* R_Transform::getChild(R_Transform* xform, u32 index)
{
    return Component_GetXform(*ARENA_GET_TYPE(&xform->children, SG_ID, index));
}

void R_Transform::rotateOnLocalAxis(R_Transform* xform, glm::vec3 axis, f32 deg)
{
    R_Transform::rot(xform, xform->_rot * glm::angleAxis(deg, glm::normalize(axis)));
}

void R_Transform::rotateOnWorldAxis(R_Transform* xform, glm::vec3 axis, f32 deg)
{
    R_Transform::rot(xform, glm::angleAxis(deg, glm::normalize(axis)) * xform->_rot);
}

void R_Transform::updateMesh(R_Transform* xform, SG_ID geo_id, SG_ID mat_id)
{
    R_Scene* scene = Component_GetScene(xform->scene_id);
    if (scene) {
        // mark previous primitives as stale
        R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID)->stale = true;
    }

    xform->_geoID = geo_id;
    xform->_matID = mat_id;

    // add new primitive
    if (scene) R_Scene::registerMesh(scene, xform);
}

void R_Transform::print(R_Transform* xform)
{
    R_Transform::print(xform, 0);
}

void R_Transform::print(R_Transform* xform, u32 depth)
{
    for (u32 i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("%s\n", xform->name.c_str());
    // print position
    for (u32 i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("  pos: %f %f %f\n", xform->_pos.x, xform->_pos.y, xform->_pos.z);

    u32 childrenCount = R_Transform::numChildren(xform);
    for (u32 i = 0; i < childrenCount; ++i) {
        print(R_Transform::getChild(xform, i), depth + 1);
    }
}

// ============================================================================
// Geometry Component
// ============================================================================
void R_Geometry::init(R_Geometry* geo)
{
    ASSERT(geo->id == 0);
    *geo = {};

    geo->id   = getNewComponentID();
    geo->type = SG_COMPONENT_GEOMETRY;
}

u32 R_Geometry::indexCount(R_Geometry* geo)
{
    return geo->gpu_index_buffer.size / sizeof(u32);
}

u32 R_Geometry::vertexCount(R_Geometry* geo)
{
    if (geo->vertex_attribute_num_components[0] == 0) return 0;

    return geo->gpu_vertex_buffers[0].size
           / (sizeof(f32) * geo->vertex_attribute_num_components[0]);
}

// returns # of contiguous non-zero vertex attributes
u32 R_Geometry::vertexAttributeCount(R_Geometry* geo)
{
    for (int i = 0; i < ARRAY_LENGTH(geo->vertex_attribute_num_components); ++i) {
        if (geo->vertex_attribute_num_components[i] == 0) return i;
    }
    return ARRAY_LENGTH(geo->vertex_attribute_num_components);
}

void R_Geometry::setVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                    u32 location, u32 num_components_per_attrib,
                                    void* data, size_t size)
{
    ASSERT(location >= 0
           && location < ARRAY_LENGTH(geo->vertex_attribute_num_components));

    geo->vertex_attribute_num_components[location] = num_components_per_attrib;
    GPU_Buffer::write(gctx, &geo->gpu_vertex_buffers[location],
                      (WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst), data, size);
}

void R_Geometry::setIndices(GraphicsContext* gctx, R_Geometry* geo, u32* indices,
                            u32 indices_count)
{
    GPU_Buffer::write(gctx, &geo->gpu_index_buffer,
                      (WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst), indices,
                      indices_count * sizeof(*indices));
}

bool R_Geometry::usesVertexPulling(R_Geometry* geo)
{
    for (int i = 0; i < ARRAY_LENGTH(geo->pull_buffers); ++i) {
        if (geo->pull_buffers[i].buf) return true;
    }
    return false;
}
void R_Geometry::rebuildPullBindGroup(GraphicsContext* gctx, R_Geometry* geo,
                                      WGPUBindGroupLayout layout)
{
    if (!geo->pull_bind_group_dirty) {
        return;
    }

    geo->pull_bind_group_dirty = false;

    WGPUBindGroupEntry entries[SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS];
    int num_entries = 0;
    for (u32 i = 0; i < SG_GEOMETRY_MAX_VERTEX_PULL_BUFFERS; i++) {
        if (geo->pull_buffers[i].buf == NULL) {
            continue;
        }

        WGPUBindGroupEntry* entry = &entries[num_entries++];
        entry->binding            = i;
        entry->buffer             = geo->pull_buffers[i].buf;
        entry->offset             = 0;
        entry->size               = geo->pull_buffers[i].size;
    }

    WGPUBindGroupDescriptor desc = {};
    desc.layout                  = layout;
    desc.entryCount              = num_entries;
    desc.entries                 = entries;

    WGPU_RELEASE_RESOURCE(BindGroup, geo->pull_bind_group);

    geo->pull_bind_group = wgpuDeviceCreateBindGroup(gctx->device, &desc);
    ASSERT(geo->pull_bind_group);
}

void R_Geometry::setPulledVertexAttribute(GraphicsContext* gctx, R_Geometry* geo,
                                          u32 location, void* data, size_t size_bytes)
{
    int prev_size  = geo->pull_buffers[location].size;
    bool recreated = GPU_Buffer::write(gctx, &geo->pull_buffers[location],
                                       WGPUBufferUsage_Storage, data, size_bytes);
    if (recreated || prev_size != size_bytes) {
        // need to update bindgroup size
        geo->pull_bind_group_dirty = true;
    }
}

// ============================================================================
// R_Texture
// ============================================================================

void R_Texture::load(GraphicsContext* gctx, R_Texture* texture, const char* filepath,
                     bool flip_vertically, bool gen_mips)
{
    i32 width = 0, height = 0;
    // Force loading 3 channel images to 4 channel by stb becasue Dawn
    // doesn't support 3 channel formats currently. The group is discussing
    // on whether webgpu shoud support 3 channel format.
    // https://github.com/gpuweb/gpuweb/issues/66#issuecomment-410021505
    i32 read_comps    = 0;
    i32 desired_comps = STBI_rgb_alpha; // force 4 channels

    stbi_set_flip_vertically_on_load(flip_vertically);

    // determine if we should load ldr or hdr
    void* pixelData = NULL;
    // free pixel data
    defer(if (pixelData) stbi_image_free(pixelData););

    if (texture->desc.format == WGPUTextureFormat_RGBA16Float) {
        log_error(
          "WARNING trying to load texture as RGBA16Float, but this format is not "
          "supported by chugl image loader\n. Use RGBA32Float or RGBA8Unorm instead");
    }

    bool is_hdr = false;
    if (texture->desc.format == WGPUTextureFormat_RGBA32Float) {
        pixelData = stbi_loadf(filepath,     //
                               &width,       //
                               &height,      //
                               &read_comps,  //
                               desired_comps //
        );
        is_hdr    = true;
    } else if (texture->desc.format == WGPUTextureFormat_RGBA8Unorm) {
        pixelData = stbi_load(filepath,     //
                              &width,       //
                              &height,      //
                              &read_comps,  //
                              desired_comps //
        );
        is_hdr    = false;
    } else {
        log_error("Unsupported texture format %d\n", texture->desc.format);
        return;
    }

    if (pixelData == NULL) {
        log_error("Couldn't load '%s'\n. Reason: %s", filepath, stbi_failure_reason());
        return;
    } else {
        log_info("Loaded %s image %s (%d, %d, %d / %d)\n", is_hdr ? "HDR" : "LDR",
                 filepath, width, height, read_comps, desired_comps);
    }

    SG_TextureWriteDesc write_desc = {};
    write_desc.width               = width;
    write_desc.height              = height;
    R_Texture::write(gctx, texture, &write_desc, pixelData,
                     width * height * G_bytesPerTexel(texture->desc.format));

    if (gen_mips) {
        MipMapGenerator_generate(gctx, texture->gpu_texture, texture->name.c_str());
    }
}

// ============================================================================
// R_Material
// ============================================================================

void MaterialTextureView::init(MaterialTextureView* view)
{
    *view          = {};
    view->strength = 1.0f;
    view->scale[0] = 1.0f;
    view->scale[1] = 1.0f;
}

void R_Material::updatePSO(GraphicsContext* gctx, R_Material* mat,
                           SG_MaterialPipelineState* pso)
{
    mat->pso = *pso;
    hashmap_set(materials_with_new_pso, &mat->id);
    mat->pipeline_stale = true;
}

void R_Material::rebuildBindGroup(R_Material* mat, GraphicsContext* gctx,
                                  WGPUBindGroupLayout layout)
{
    // TODO: can we improve this? maybe assume bindings are consecutive.
    // at first R_BIND_EMPTY can early-out
    // check in chugl example if we can skip @binding() numbers
    // unfortunately wgsl still compiles if there are skips/holes in bindgroup numbering
    if (!mat->bind_group_stale) {
        // check all texture bindings and see if generation# changed
        // ==optimize== have textures track an arena of bound mat IDs, and mark as stale
        // upon change or have a "bindgroup" manager class do the same
        // this lazy check is the quickest to implement, and requires the least
        // additional state
        bool needs_rebuild = false;
        for (u32 i = 0; i < ARRAY_LENGTH(mat->bindings); ++i) {
            R_Binding* binding = &mat->bindings[i];
            if (binding->type == R_BIND_TEXTURE_ID) {
                R_Texture* tex = Component_GetTexture(binding->as.textureID);
                if (tex->generation != binding->generation) {
                    ASSERT(tex->generation > binding->generation);
                    needs_rebuild = true;
                    break;
                }
            }
        }
        if (!needs_rebuild) return;
    }
    mat->bind_group_stale = false;

    // log_info("rebuilding bind group\n");

    // create bindgroups for all bindings
    WGPUBindGroupEntry new_bind_group_entries[SG_MATERIAL_MAX_UNIFORMS] = {};
    int bind_group_index                                                = 0;
    ASSERT(SG_MATERIAL_MAX_UNIFORMS == ARRAY_LENGTH(mat->bindings));

    for (u32 i = 0; i < SG_MATERIAL_MAX_UNIFORMS; ++i) {
        R_Binding* binding = &mat->bindings[i];
        if (binding->type == R_BIND_EMPTY) continue;

        WGPUBindGroupEntry* bind_group_entry
          = &new_bind_group_entries[bind_group_index++];
        *bind_group_entry = {};

        bind_group_entry->binding = i; // binding location

        switch (binding->type) {
            case R_BIND_UNIFORM: {
                bind_group_entry->offset
                  = MAX(gctx->limits.minUniformBufferOffsetAlignment,
                        sizeof(SG_MaterialUniformData))
                    * i;
                bind_group_entry->size   = sizeof(SG_MaterialUniformData);
                bind_group_entry->buffer = mat->uniform_buffer.buf;
            } break;
            case R_BIND_STORAGE: {
                bind_group_entry->offset = 0;
                bind_group_entry->size   = binding->size;
                bind_group_entry->buffer = binding->as.storage_buffer.buf;
            } break;
            case R_BIND_SAMPLER: {
                bind_group_entry->sampler
                  = Graphics_GetSampler(gctx, binding->as.samplerConfig);
            } break;
            case R_BIND_TEXTURE_ID: {
                R_Texture* rTexture = Component_GetTexture(binding->as.textureID);
                bind_group_entry->textureView = rTexture->gpu_texture_view;
                ASSERT(bind_group_entry->textureView);
                ASSERT(binding->size == sizeof(SG_ID));
                binding->generation = rTexture->generation;
            } break;
            case R_BIND_STORAGE_EXTERNAL: {
                bind_group_entry->offset = 0;
                bind_group_entry->size   = binding->size;
                bind_group_entry->buffer = binding->as.storage_external->buf;
            } break;
            case R_BIND_TEXTURE_VIEW: {
                ASSERT(binding->as.textureView);
                bind_group_entry->textureView = binding->as.textureView;
            } break;
            case R_BIND_STORAGE_TEXTURE_ID: {
                ASSERT(binding->as.textureView);
                bind_group_entry->textureView = binding->as.textureView;
            } break;
            default: ASSERT(false);
        }
    }

    // release previous bindgroup
    WGPU_RELEASE_RESOURCE(BindGroup, mat->bind_group);

    // create new bindgroup
    WGPUBindGroupDescriptor bg_desc = {};
    bg_desc.layout                  = layout;
    bg_desc.entryCount              = bind_group_index;
    bg_desc.entries                 = new_bind_group_entries;
    mat->bind_group                 = wgpuDeviceCreateBindGroup(gctx->device, &bg_desc);
    ASSERT(mat->bind_group);
}

static SamplerConfig samplerConfigFromSGSampler(SG_Sampler sg_sampler)
{
    // TODO make SamplerConfig and SG_Sampler just use WGPU types
    SamplerConfig sampler = {};
    sampler.wrapU         = (SamplerWrapMode)sg_sampler.wrapU;
    sampler.wrapV         = (SamplerWrapMode)sg_sampler.wrapV;
    sampler.wrapW         = (SamplerWrapMode)sg_sampler.wrapW;
    sampler.filterMin     = (SamplerFilterMode)sg_sampler.filterMin;
    sampler.filterMag     = (SamplerFilterMode)sg_sampler.filterMag;
    sampler.filterMip     = (SamplerFilterMode)sg_sampler.filterMip;
    return sampler;
}

void R_Material::setSamplerBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                   SG_Sampler sampler)
{
    SamplerConfig sampler_config = samplerConfigFromSGSampler(sampler);
    R_Material::setBinding(gctx, mat, location, R_BIND_SAMPLER, &sampler_config,
                           sizeof(sampler_config));
}

void R_Material::setTextureBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                                   SG_ID texture_id)
{
    R_Material::setBinding(gctx, mat, location, R_BIND_TEXTURE_ID, &texture_id,
                           sizeof(texture_id));
}

void R_Material::setTextureViewBinding(GraphicsContext* gctx, R_Material* mat,
                                       u32 location, WGPUTextureView view)
{
    ASSERT(view);
    R_Material::setBinding(gctx, mat, location, R_BIND_TEXTURE_VIEW, &view,
                           sizeof(WGPUTextureView));
}

void R_Material::setExternalStorageBinding(GraphicsContext* gctx, R_Material* mat,
                                           u32 location, GPU_Buffer* buffer)
{
    ASSERT(buffer->usage & WGPUBufferUsage_Storage);
    R_Material::setBinding(gctx, mat, location, R_BIND_STORAGE_EXTERNAL, buffer,
                           buffer->size);
}

void R_Material::setStorageTextureBinding(GraphicsContext* gctx, R_Material* mat,
                                          u32 location, SG_ID texture_id)
{
    R_Material::setBinding(gctx, mat, location, R_BIND_STORAGE_TEXTURE_ID, &texture_id,
                           sizeof(texture_id));
}

void R_Material::setBinding(GraphicsContext* gctx, R_Material* mat, u32 location,
                            R_BindType type, void* data, size_t bytes)
{
    R_Binding* binding = &mat->bindings[location];

    { // logic for setting bind group stale (only check if it's not already set to
      // stale)
        if (!mat->bind_group_stale) {
            bool same_bind_type = (binding->type == type);
            // clang-format off
            if (same_bind_type && 
            (
                (type == R_BIND_UNIFORM) // uniform buffer is never recreated
                ||
                (type == R_BIND_STORAGE && binding->size == bytes) // local storage binding same size
                ||
                (type == R_BIND_SAMPLER && memcmp(&binding->as.samplerConfig, data, bytes) == 0) // sampler config the same
            )) {
                // in this case do nothing
                // DO NOT set bind_group_stale = false because that overwrites previous 
                // times this frame when bind_group_stale was set to true correctly
            } else {
                mat->bind_group_stale = true;
            }
        }
    }
    // clang-format on

    R_BindType prev_bind_type = binding->type;
    binding->type             = type;
    binding->size             = bytes;

    // cleanup previous
    if (prev_bind_type == R_BIND_STORAGE_TEXTURE_ID) {
        // free previous storage texture view
        WGPU_RELEASE_RESOURCE(TextureView, binding->as.textureView);
    }

    // create new binding
    switch (type) {
        case R_BIND_UNIFORM: {
            size_t offset = MAX(gctx->limits.minUniformBufferOffsetAlignment,
                                sizeof(SG_MaterialUniformData))
                            * location;
            // write unfiform data to corresponding GPU location
            GPU_Buffer::write(gctx, &mat->uniform_buffer, WGPUBufferUsage_Uniform,
                              offset, data, bytes);
        } break;
        case R_BIND_TEXTURE_ID: {
            ASSERT(bytes == sizeof(SG_ID));
            R_Texture* tex        = Component_GetTexture(*(SG_ID*)data);
            binding->as.textureID = tex->id;
            binding->generation   = tex->generation;
        } break;
        case R_BIND_TEXTURE_VIEW: {
            ASSERT(bytes == sizeof(WGPUTextureView));
            binding->as.textureView = *(WGPUTextureView*)data;
            break;
        }
        case R_BIND_SAMPLER: {
            ASSERT(bytes == sizeof(SamplerConfig));
            binding->as.samplerConfig = *(SamplerConfig*)data;
        } break;
        case R_BIND_STORAGE: {
            GPU_Buffer::write(gctx, &binding->as.storage_buffer,
                              WGPUBufferUsage_Storage, data, bytes);
        } break;
        case R_BIND_STORAGE_EXTERNAL: {
            // external storage buffer
            binding->as.storage_external = (GPU_Buffer*)data;
        } break;
        case R_BIND_STORAGE_TEXTURE_ID: {
            // TODO: allow creating storage texture at other mip levels
            binding->as.textureView = G_createTextureViewAtMipLevel(
              Component_GetTexture(*(SG_ID*)data)->gpu_texture, 0, "storage texture");
        } break;
        default:
            // if the new binding is also STORAGE reuse the memory, don't
            // free
            log_error("unsupported binding type %d", type);
            ASSERT(false);
            break;
    }
}

// ============================================================================
// R_Scene
// ============================================================================

void GeometryToXforms::rebuildBindGroup(GraphicsContext* gctx, R_Scene* scene,
                                        GeometryToXforms* g2x,
                                        WGPUBindGroupLayout layout, Arena* frame_arena)
{
    if (!g2x->stale) return;
    defer(g2x->stale = false);

    // build new array of matrices on CPU
    u64 model_matrices_offset = frame_arena->curr;

    int numInstances = ARENA_LENGTH(&g2x->xform_ids, SG_ID);
    SG_ID* xformIDs  = (SG_ID*)g2x->xform_ids.base;
    // delete and swap any destroyed xforms
    for (size_t i = 0; i < numInstances; ++i) {
        R_Transform* xform = Component_GetXform(xformIDs[i]);
        // remove NULL xforms and xforms that have been reassigned new mesh params

        // TODO: impl GMesh.geo() and GMesh.mat() to change geo and mat of mesh
        // - impl needs to set GeometryToXforms.stale = true
        // - but does NOT need to linear search the xformIDs arena. because lazy
        // deletion happens right here

        bool xform_destroyed = (xform == NULL);
        bool xform_changed_mesh
          = (xform->_geoID != g2x->key.geo_id || xform->_matID != g2x->key.mat_id);
        bool xform_detached_from_scene = (xform->scene_id != scene->id);
        // TODO use arena macro instead
        if (xform_destroyed || xform_changed_mesh || xform_detached_from_scene) {
            GeometryToXforms::removeXform(g2x, i);
            // decrement to reprocess this index
            --i;
            --numInstances;
            continue;
        }
        // assert his xform belongs to this material and geometry
        ASSERT(xform->_geoID == g2x->key.geo_id);
        ASSERT(xform->_matID == g2x->key.mat_id);
        // else add xform matrix to arena
        // world matrix should already have been computed by now
        ASSERT(xform->_stale == R_Transform_STALE_NONE);

        DrawUniforms* draw_uniforms = ARENA_PUSH_TYPE(frame_arena, DrawUniforms);
        draw_uniforms->model        = xform->world;
        draw_uniforms->id           = xform->id;
    }
    // sanity check that we have the correct number of matrices
    ASSERT(numInstances == ARENA_LENGTH(&g2x->xform_ids, SG_ID));

    u64 write_size = frame_arena->curr - model_matrices_offset;
    GPU_Buffer::write(gctx, &g2x->xform_storage_buffer, WGPUBufferUsage_Storage,
                      Arena::get(frame_arena, model_matrices_offset), write_size);

    // pop arena after copying data to GPU
    Arena::pop(frame_arena, write_size);
    ASSERT(model_matrices_offset == frame_arena->curr);

    // recreate bindgroup
    WGPUBindGroupEntry entry = {};
    entry.binding            = 0;
    entry.buffer             = g2x->xform_storage_buffer.buf;
    entry.offset             = 0;
    entry.size               = g2x->xform_storage_buffer.size;

    WGPUBindGroupDescriptor desc = {};
    desc.layout                  = layout;
    desc.entryCount              = 1;
    desc.entries                 = &entry;

    WGPU_RELEASE_RESOURCE(BindGroup, g2x->xform_bind_group);

    g2x->xform_bind_group = wgpuDeviceCreateBindGroup(gctx->device, &desc);
    ASSERT(g2x->xform_bind_group);
}

void R_Scene::removeSubgraphFromRenderState(R_Scene* scene, R_Transform* root)
{
    if (!scene || !root) return;

    ASSERT(scene->id == root->scene_id);

    // find all meshes in child subgraph
    static Arena arena{};
    defer(Arena::clear(&arena));

    SG_ID* sg_id = ARENA_PUSH_TYPE(&arena, SG_ID);
    *sg_id       = root->id;
    while (ARENA_LENGTH(&arena, SG_ID)) {
        SG_ID xformID = *ARENA_GET_LAST_TYPE(&arena, SG_ID);
        ARENA_POP_TYPE(&arena, SG_ID);
        R_Transform* xform = Component_GetXform(xformID);

        // add children to queue
        for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
            *ARENA_PUSH_ZERO_TYPE(&arena, SG_ID)
              = *ARENA_GET_TYPE(&xform->children, SG_ID, i);
        }

        // remove scene from xform
        ASSERT(xform->scene_id == scene->id);
        xform->scene_id = 0;

        if (xform->type == SG_COMPONENT_LIGHT) {
            // remove from light set
            const void* prev_item = hashmap_delete(scene->light_id_set, &xform->id);
            ASSERT(prev_item);
        } else if (xform->type == SG_COMPONENT_MESH) {
            // get xforms from geometry
            GeometryToXforms* g2x
              = R_Scene::getPrimitive(scene, xform->_geoID, xform->_matID);

            // don't need to remove xform from geometry
            // just mark the g2x entry as stale; will be lazily deleted in
            // GeometryToXforms::rebuildBindGroup();
            g2x->stale = true;
            ASSERT(GeometryToXforms::hasXform(g2x, xform->id));

            // TODO: deletions must cascade upwards to geo-->material-->pipeline
            // we can only remove an SG_ID from a higher level IF that SG_ID
            // at the lower level has an empty arena
            // can implement this later. for now deletions will leave a bunch of empty
            // arenas in the 3 hashmaps.
            // better yet: batch all deletions and prune the render state in one go
            // in a separate function like GG.gc()
        }
    }
}

void R_Scene::addSubgraphToRenderState(R_Scene* scene, R_Transform* root)
{
    ASSERT(scene == R_Transform::getScene(root));

    if (!scene) return;

    // find all meshes in child subgraph
    static Arena arena{};
    defer(Arena::clear(&arena));

    *ARENA_PUSH_TYPE(&arena, SG_ID) = root->id;
    while (ARENA_LENGTH(&arena, SG_ID)) {
        R_Transform* xform = Component_GetXform(*ARENA_GET_LAST_TYPE(&arena, SG_ID));
        ARENA_POP_TYPE(&arena, SG_ID);

        // add children to queue
        for (u32 i = 0; i < ARENA_LENGTH(&xform->children, SG_ID); ++i) {
            *ARENA_PUSH_ZERO_TYPE(&arena, SG_ID)
              = *ARENA_GET_TYPE(&xform->children, SG_ID, i);
        }

        // add scene to transform state
        ASSERT(xform->scene_id != scene->id);
        xform->scene_id = scene->id;

        // lighting logic
        if (xform->type == SG_COMPONENT_LIGHT) {
            // add light to scene
            const void* replaced = hashmap_set(scene->light_id_set, &xform->id);
            // light should not already be in scene
            ASSERT(!replaced);
        } else if (xform->_geoID != 0 && xform->_matID != 0) { // for all renderables
            ASSERT(xform->type == SG_COMPONENT_MESH
                   || xform->type == SG_COMPONENT_TEXT);

            // try adding to bottom level GeometryToXform
            // if its not present, build up entire chain:
            // g2x (geo, mat) --> xforms
            // m2g mat --> geo
            // p2g pso --> mat
            // maintain invariant that if a (geo, mat) is present in g2x,
            // then all higher order entries are also present.
            // if g2x entry is not found, create it and all corresponding.
            // invariant holds assuming the deletion in removeSubgraphFromRenderState()
            // does not delete entire hashmap entries, ONLY pops individual SG_IDs from
            // the entry arena
            // that is, if a <geo_id, mat_id> entry is present in g2m hashmap,
            // even if it has 0 xform ids, all upwards mat_id --> geo_ids and
            // pipeline_id --> mat_ids entries also exist
            // this avoids us having to do O(n) search on each xform added/removed
            // from scenegraph render state

            R_Scene::registerMesh(scene, xform);
        }
    }
}

void R_Scene::rebuildLightInfoBuffer(GraphicsContext* gctx, R_Scene* scene, u64 fc)
{
    static u64 last_fc_updated{ 0 };
    if (last_fc_updated == fc) return;
    last_fc_updated = fc;

    static Arena light_info_arena{};
    ASSERT(light_info_arena.curr == 0);
    defer(Arena::clear(&light_info_arena));

    // allocate cpu memory for light uniform data
    int num_lights = R_Scene::numLights(scene);
    UNUSED_VAR(num_lights);
    LightUniforms* light_uniforms
      = ARENA_PUSH_COUNT(&light_info_arena, LightUniforms, R_Scene::numLights(scene));

    size_t hashmap_idx_DONT_USE = 0;
    SG_ID* light_id             = NULL;
    int light_idx               = 0;

    while (
      hashmap_iter(scene->light_id_set, &hashmap_idx_DONT_USE, (void**)&light_id)) {
        LightUniforms* light_uniform = &light_uniforms[light_idx++];
        *light_uniform               = {};

        R_Light* light = Component_GetLight(*light_id);
        ASSERT(light);
        ASSERT(light->_stale == R_Transform_STALE_NONE);
        // get light transform data
        glm::vec3 pos, sca;
        glm::quat rot;
        R_Transform::decomposeWorldMatrix(light->world, pos, rot, sca);
        glm::vec3 forward
          = glm::normalize(glm::rotate(rot, glm::vec3(0.0f, 0.0f, -1.0f)));

        { // initialize uniform struct
            light_uniform->color      = light->desc.intensity * light->desc.color;
            light_uniform->light_type = (i32)light->desc.type;
            light_uniform->position   = pos;
            light_uniform->direction  = forward;

            switch (light->desc.type) {
                case SG_LightType_None: break;
                case SG_LightType_Directional: {
                } break;
                case SG_LightType_Point: {
                    light_uniform->point_radius  = light->desc.point_radius;
                    light_uniform->point_falloff = light->desc.point_falloff;
                } break;
                default: {
                    // not impl
                    ASSERT(false);
                } break;
            }
        }
    }

    // upload to gpu
    GPU_Buffer::write(gctx, &scene->light_info_buffer, WGPUBufferUsage_Storage,
                      light_info_arena.base, light_info_arena.curr);
}

void R_Scene::registerMesh(R_Scene* scene, R_Transform* mesh)
{
    if (!scene || !mesh) return;

    if (mesh->_geoID == 0 || mesh->_matID == 0) return;

    ASSERT(mesh->type == SG_COMPONENT_MESH || mesh->type == SG_COMPONENT_TEXT);
    GeometryToXforms* g2x = R_Scene::getPrimitive(scene, mesh->_geoID, mesh->_matID);
    GeometryToXforms::addXform(g2x, mesh->id);

    MaterialToGeometry* m2g = R_Scene::getMaterialToGeometry(scene, mesh->_matID);
    MaterialToGeometry::addGeometry(m2g, mesh->_geoID);
}

GeometryToXforms* R_Scene::getPrimitive(R_Scene* scene, SG_ID geo_id, SG_ID mat_id)
{
    GeometryToXformKey key = {};
    key.geo_id             = geo_id;
    key.mat_id             = mat_id;
    GeometryToXforms* g2x  = (GeometryToXforms*)hashmap_get(scene->geo_to_xform, &key);

    if (!g2x) {
        // create new one
        GeometryToXforms new_g2x = {};
        new_g2x.key.geo_id       = geo_id;
        new_g2x.key.mat_id       = mat_id;
        Arena::init(&new_g2x.xform_ids, sizeof(SG_ID) * 8);
        u64 seed             = time(NULL);
        new_g2x.xform_id_set = hashmap_new(sizeof(SG_ID), 0, seed, seed, hashSGID,
                                           compareSGIDs, NULL, NULL);
        hashmap_set(scene->geo_to_xform, &new_g2x);
        g2x = (GeometryToXforms*)hashmap_get(scene->geo_to_xform, &key);
    }
    ASSERT(g2x);
    return g2x;
}

MaterialToGeometry* R_Scene::getMaterialToGeometry(R_Scene* scene, SG_ID mat_id)
{
    MaterialToGeometry* m2g
      = (MaterialToGeometry*)hashmap_get(scene->material_to_geo, &mat_id);

    if (!m2g) {
        MaterialToGeometry new_m2g = {};
        new_m2g.material_id        = mat_id;
        Arena::init(&new_m2g.geo_ids, sizeof(SG_ID) * 8);
        u64 seed           = time(NULL);
        new_m2g.geo_id_set = hashmap_new(sizeof(SG_ID), 0, seed, seed, hashSGID,
                                         compareSGIDs, NULL, NULL);
        hashmap_set(scene->material_to_geo, &new_m2g);

        m2g = (MaterialToGeometry*)hashmap_get(scene->material_to_geo, &mat_id);
    }
    ASSERT(m2g);
    return m2g;
}

void R_Scene::initFromSG(GraphicsContext* gctx, R_Scene* r_scene, SG_ID scene_id,
                         SG_SceneDesc* sg_scene_desc)
{
    ASSERT(r_scene->id == 0); // ensure not initialized twice
    *r_scene = {};

    // copy base component data
    // TODO have a separate R_ComponentType enum?
    r_scene->id   = scene_id;
    r_scene->type = SG_COMPONENT_SCENE;

    // copy xform
    r_scene->_pos = VEC_ORIGIN;
    r_scene->_rot = QUAT_IDENTITY;
    r_scene->_sca = VEC_ONES;

    // set stale to force rebuild of matrices
    r_scene->_stale = R_Transform_STALE_LOCAL;

    // copy scene desc
    r_scene->sg_scene_desc = *sg_scene_desc;

    // initialize arenas
    int seed = time(NULL);
    r_scene->material_to_geo
      = hashmap_new(sizeof(MaterialToGeometry), 0, seed, seed, MaterialToGeometry::hash,
                    MaterialToGeometry::compare, MaterialToGeometry::free, NULL);
    r_scene->geo_to_xform
      = hashmap_new(sizeof(GeometryToXforms), 0, seed, seed, GeometryToXforms::hash,
                    GeometryToXforms::compare, GeometryToXforms::free, NULL);
    r_scene->light_id_set
      = hashmap_new(sizeof(SG_ID), 0, seed, seed, hashSGID, compareSGIDs, NULL, NULL);

    GPU_Buffer::init(gctx, &r_scene->light_info_buffer, WGPUBufferUsage_Uniform,
                     sizeof(LightUniforms) * 16);

    // initialize children array for 8 children
    Arena::init(&r_scene->children, sizeof(SG_ID) * 8);
}

// ============================================================================
// Render Pipeline Definitions
// ============================================================================

GPU_Buffer R_RenderPipeline::frame_uniform_buffer = {};

void R_RenderPipeline::init(GraphicsContext* gctx, R_RenderPipeline* pipeline,
                            const SG_MaterialPipelineState* config,
                            int msaa_sample_count)
{
    ASSERT(pipeline->gpu_pipeline == NULL);
    ASSERT(pipeline->rid == 0);

    pipeline->rid = getNewRID();
    ASSERT(pipeline->rid < 0);

    pipeline->pso = *config;

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = config->primitive_topology;
    primitiveState.stripIndexFormat
      = (primitiveState.topology == WGPUPrimitiveTopology_TriangleStrip
         || primitiveState.topology == WGPUPrimitiveTopology_LineStrip) ?
          WGPUIndexFormat_Uint32 :
          WGPUIndexFormat_Undefined;
    primitiveState.frontFace = WGPUFrontFace_CCW;
    primitiveState.cullMode  = config->cull_mode;

    // TODO transparency (dissallow partial transparency, see if fragment
    // discard writes to the depth buffer)
    WGPUBlendState blendState = G_createBlendState(true);

    WGPUColorTargetState colorTargetState = {};
    // colorTargetState.format               = gctx->swapChainFormat;
    colorTargetState.format    = WGPUTextureFormat_RGBA16Float; // for now force HDR
    colorTargetState.blend     = &blendState;
    colorTargetState.writeMask = WGPUColorWriteMask_All;

    WGPUDepthStencilState depth_stencil_state
      = G_createDepthStencilState(WGPUTextureFormat_Depth24PlusStencil8, true);

    // Setup shader module
    R_Shader* shader = Component_GetShader(config->sg_shader_id);
    if (!shader) {
        log_error(
          "Error: failed creating render pipeline from material with shader id = %llu",
          config->sg_shader_id);
    }
    ASSERT(shader);

    VertexBufferLayout vertexBufferLayout = {};
    VertexBufferLayout::init(&vertexBufferLayout, ARRAY_LENGTH(shader->vertex_layout),
                             shader->vertex_layout);

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.bufferCount     = vertexBufferLayout.attribute_count;
    vertexState.buffers         = vertexBufferLayout.layouts;
    vertexState.module          = shader->vertex_shader_module;
    vertexState.entryPoint      = VS_ENTRY_POINT;

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = shader->fragment_shader_module;
    fragmentState.entryPoint        = FS_ENTRY_POINT;
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState = G_createMultisampleState(msaa_sample_count);

    char pipeline_label[64] = {};
    snprintf(pipeline_label, sizeof(pipeline_label), "RenderPipeline %lld %s",
             (i64)shader->id, shader->name.c_str());
    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.label                        = pipeline_label;
    pipeline_desc.layout                       = NULL; // Using layout: auto
    pipeline_desc.primitive                    = primitiveState;
    pipeline_desc.vertex                       = vertexState;
    pipeline_desc.fragment                     = &fragmentState;
    pipeline_desc.depthStencil                 = &depth_stencil_state;
    pipeline_desc.multisample                  = multisampleState;

    pipeline->gpu_pipeline
      = wgpuDeviceCreateRenderPipeline(gctx->device, &pipeline_desc);
    ASSERT(pipeline->gpu_pipeline);

    Arena::init(&pipeline->materialIDs, sizeof(SG_ID) * 8);

    // cache the bind group layouts because apparently
    // wgpuRenderPipelineGetBindGroupLayout freaking leaks...

    for (int i = 0; i < 3; i++) {
        pipeline->bind_group_layouts[i]
          = wgpuRenderPipelineGetBindGroupLayout(pipeline->gpu_pipeline, i);
        // note: we don't compute the pull bind group (@group(3)) here
        // because not all pipelines use it, and calling this function with
        // an index of 3 will crash if the pipeline doesn't have a group(3)
        // instead we lazily evaluate and cache during the renderloop
    }
}

/*
Tricky: a material can only ever be bound to a single pipeline,
BUT it might switch pipelines over the course of its lifetime.
E.g. the user switches a material from being single-sided to double-sided,
or from being opaque to transparent.
How to support this?

Add a function R_MaterialConfig GetRenderPipelineConfig() to R_Material

Option 1: before rendering, do a pass over all materials in material arena.
Get its renderPipelineConfig, and see if its tied to the correct pipeline.
If not, remove from the current and add to the new.
- cons: requires a second pass over all materials, which is not cache
efficient

Option 2: check the material WHEN doing the render pass over all
RenderPipeline. Same logic, GetRenderPipelineConfig() and if its different,
add to correct pipeline.
- cons: if the material gets added to a pipeline that was ALREADY processed,
it will be missed and only rendered on the NEXT frame. (that might be fine)

Going with option 2 for now

*/

void R_RenderPipeline::addMaterial(R_RenderPipeline* pipeline, R_Material* material)
{
    if (material->pipelineID == pipeline->rid) {
        ASSERT(ARENA_CONTAINS(&pipeline->materialIDs, material->id));
        return;
    }

    // remove material from existing pipeline
    // Lazy deletion. during render loop, pipeline checks if the material
    // ID matches this current pipeline. If not, we know we've switched.
    // the render pipeline then removes the material from its list of
    // material IDs. This avoids the cost of doing a linear search over all
    // materials for deletion. We postpone the deletion until we're already
    // iterating over the materials anyways

    // add material to new pipeline
    ASSERT(!ARENA_CONTAINS(&pipeline->materialIDs, material->id));
    *ARENA_PUSH_TYPE(&pipeline->materialIDs, SG_ID) = material->id;
    material->pipelineID                            = pipeline->rid;
}

size_t R_RenderPipeline::numMaterials(R_RenderPipeline* pipeline)
{
    return ARENA_LENGTH(&pipeline->materialIDs, SG_ID);
}

// While iterating, will lazy delete NULL material IDs and materials
// whose config doesn't match the pipeline (e.g. material has been assigned
// elsewhere) swap NULL with last element and pop
// Can do away with this lazy deletion if we augment the SG_ID arena with a
// hashmap for O(1) material ID lookup and deletion
bool R_RenderPipeline::materialIter(R_RenderPipeline* pipeline, size_t* indexPtr,
                                    R_Material** material)
{
    size_t numMaterials = R_RenderPipeline::numMaterials(pipeline);

    if (*indexPtr >= numMaterials) {
        *material = NULL;
        return false;
    }

    SG_ID* materialID = ARENA_GET_TYPE(&pipeline->materialIDs, SG_ID, *indexPtr);
    *material         = Component_GetMaterial(*materialID);

    // if null or reassigned to different pipeline, swap with last element
    // and try again
    if (*material == NULL || (*material)->pipelineID != pipeline->rid) {
        SG_ID* lastMatID
          = ARENA_GET_TYPE(&pipeline->materialIDs, SG_ID, numMaterials - 1);

        // swap
        *materialID = *lastMatID;
        // pop last element
        Arena::pop(&pipeline->materialIDs, sizeof(SG_ID));
        // try again with same index
        return materialIter(pipeline, indexPtr, material);
    }

    // else return normally
    ++(*indexPtr);
    return true;
}

// ============================================================================
// Component Manager Definitions
// ============================================================================

// storage arenas
static Arena xformArena;
static Arena sceneArena;
static Arena geoArena;
static Arena shaderArena;
static Arena materialArena;
static Arena textureArena;
static Arena passArena;
static Arena bufferArena;
static Arena _RenderPipelineArena;
static Arena cameraArena;
static Arena textArena;
static Arena lightArena;

// default textures
static Texture opaqueWhitePixel      = {};
static Texture transparentBlackPixel = {};
static Texture defaultNormalPixel    = {};

// maps from id --> offset
static hashmap* r_locator                 = NULL;
static hashmap* render_pipeline_pso_table = NULL; // lookup by pso
static hashmap* _RenderPipelineMap;               // lookup by rid

// fonts
// each font is 600bytes, 128 fonts is 76.8KB
static R_Font component_fonts[128];
static int component_font_count = 0;

struct R_Location {
    SG_ID id;     // key
    u64 offset;   // value (byte offset into arena)
    Arena* arena; // where to find
};

struct RenderPipelinePSOTableItem {
    SG_MaterialPipelineState pso; // key
    u64 pipeline_offset;          // item

    static int compare(const void* a, const void* b, void* udata)
    {
        RenderPipelinePSOTableItem* ga = (RenderPipelinePSOTableItem*)a;
        RenderPipelinePSOTableItem* gb = (RenderPipelinePSOTableItem*)b;
        return memcmp(&ga->pso, &gb->pso, sizeof(ga->pso));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        RenderPipelinePSOTableItem* g2x = (RenderPipelinePSOTableItem*)item;
        return hashmap_xxhash3(&g2x->pso, sizeof(g2x->pso), seed0, seed1);
    }
};

struct RenderPipelineIDTableItem {
    R_ID id;             // key
    u64 pipeline_offset; // item

    static int compare(const void* a, const void* b, void* udata)
    {
        RenderPipelineIDTableItem* ga = (RenderPipelineIDTableItem*)a;
        RenderPipelineIDTableItem* gb = (RenderPipelineIDTableItem*)b;
        return memcmp(&ga->id, &gb->id, sizeof(ga->id));
    }

    static u64 hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        RenderPipelineIDTableItem* g2x = (RenderPipelineIDTableItem*)item;
        return hashmap_xxhash3(&g2x->id, sizeof(g2x->id), seed0, seed1);
    }
};

static int R_CompareLocation(const void* a, const void* b, void* udata)
{
    R_Location* locA = (R_Location*)a;
    R_Location* locB = (R_Location*)b;
    return locA->id - locB->id;
}

static u64 R_HashLocation(const void* item, uint64_t seed0, uint64_t seed1)
{
    // tested xxhash3 is best
    R_Location* key = (R_Location*)item;
    return hashmap_xxhash3(&key->id, sizeof(SG_ID), seed0, seed1);
    // return hashmap_sip(item, sizeof(int), seed0, seed1);
    // return hashmap_murmur(item, sizeof(int), seed0, seed1);
}

void Component_Init(GraphicsContext* gctx)
{
    // initialize arena memory
    Arena::init(&xformArena, sizeof(R_Transform) * 128);
    Arena::init(&sceneArena, sizeof(R_Scene) * 128);
    Arena::init(&geoArena, sizeof(R_Geometry) * 128);
    Arena::init(&shaderArena, sizeof(R_Shader) * 64);
    Arena::init(&materialArena, sizeof(R_Material) * 64);
    Arena::init(&_RenderPipelineArena, sizeof(R_RenderPipeline) * 8);
    Arena::init(&textureArena, sizeof(R_Texture) * 64);
    Arena::init(&cameraArena, sizeof(R_Camera) * 4);
    Arena::init(&textArena, sizeof(R_Text) * 64);
    Arena::init(&passArena, sizeof(R_Pass) * 16);
    Arena::init(&bufferArena, sizeof(R_Buffer) * 64);
    Arena::init(&lightArena, sizeof(R_Light) * 16);

    // initialize default textures
    static u8 white[4]  = { 255, 255, 255, 255 };
    static u8 black[4]  = { 0, 0, 0, 0 };
    static u8 normal[4] = { 128, 128, 255, 255 };
    Texture::initSinglePixel(gctx, &opaqueWhitePixel, white);
    Texture::initSinglePixel(gctx, &transparentBlackPixel, black);
    Texture::initSinglePixel(gctx, &defaultNormalPixel, normal);

    // init locator
    int seed = time(NULL);
    srand(seed);
    r_locator = hashmap_new(sizeof(R_Location), 0, seed, seed, R_HashLocation,
                            R_CompareLocation, NULL, NULL);
    render_pipeline_pso_table
      = hashmap_new(sizeof(RenderPipelinePSOTableItem), 0, seed, seed,
                    RenderPipelinePSOTableItem::hash,
                    RenderPipelinePSOTableItem::compare, NULL, NULL);

    _RenderPipelineMap = hashmap_new(sizeof(RenderPipelineIDTableItem), 0, seed, seed,
                                     RenderPipelineIDTableItem::hash,
                                     RenderPipelineIDTableItem::compare, NULL, NULL);
    materials_with_new_pso
      = hashmap_new(sizeof(SG_ID), 0, seed, seed, hashSGID, compareSGIDs, NULL, NULL);

    // init frame uniform buffer
    FrameUniforms frame_uniforms = {};
    GPU_Buffer::write(gctx, &R_RenderPipeline::frame_uniform_buffer,
                      WGPUBufferUsage_Uniform, &frame_uniforms, sizeof(frame_uniforms));
}

void Component_Free()
{
    // TODO: should we also free the individual components?

    // free arena memory
    Arena::free(&xformArena);
    Arena::free(&sceneArena);
    Arena::free(&geoArena);
    Arena::free(&shaderArena);
    Arena::free(&materialArena);
    Arena::free(&_RenderPipelineArena);
    Arena::free(&textureArena);
    Arena::free(&cameraArena);
    Arena::free(&textArena);
    Arena::free(&passArena);
    // Arena::free(&bufferArena);

    // free default textures
    Texture::release(&opaqueWhitePixel);
    Texture::release(&transparentBlackPixel);
    Texture::release(&defaultNormalPixel);

    // free locator
    hashmap_free(r_locator);
    r_locator = NULL;
}

R_Transform* Component_CreateTransform()
{
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);
    R_Transform::init(xform);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Transform* Component_CreateTransform(SG_Command_CreateXform* cmd)
{
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);
    R_Transform::initFromSG(xform, cmd);

    ASSERT(xform->id != 0);                        // ensure id is set
    ASSERT(xform->type == SG_COMPONENT_TRANSFORM); // ensure type is set

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Transform* Component_CreateMesh(SG_ID mesh_id, SG_ID geo_id, SG_ID mat_id)
{
    R_Transform* xform = ARENA_PUSH_ZERO_TYPE(&xformArena, R_Transform);

    R_Transform_init(xform, mesh_id, SG_COMPONENT_MESH);

    // init mesh
    xform->_geoID = geo_id;
    xform->_matID = mat_id;

    // store offset
    R_Location loc = { xform->id, Arena::offsetOf(&xformArena, xform), &xformArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return xform;
}

R_Camera* Component_CreateCamera(GraphicsContext* gctx, SG_Command_CameraCreate* cmd)
{
    R_Camera* cam = ARENA_PUSH_ZERO_TYPE(&cameraArena, R_Camera);

    R_Transform_init(cam, cmd->camera.id, SG_COMPONENT_CAMERA);

    { // camera init
        // copy camera params
        cam->params = cmd->camera.params;

        // initialize frame uniform buffer
        // kept here because the camera xform is the only part that differs between
        // RenderPasses
        FrameUniforms frame_uniforms = {};
        GPU_Buffer::write(gctx, &cam->frame_uniform_buffer, WGPUBufferUsage_Uniform,
                          &frame_uniforms, sizeof(frame_uniforms));
    }

    // store offset
    R_Location loc     = { cam->id, Arena::offsetOf(&cameraArena, cam), &cameraArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return cam;
}

static void R_Text_updateFromCommand(R_Text* text, SG_Command_TextRebuild* cmd)
{
    // TODO other fields (control point, etc)
    text->text
      = std::string((const char*)CQ_ReadCommandGetOffset(cmd->text_str_offset));
    text->font_path
      = std::string((const char*)CQ_ReadCommandGetOffset(cmd->font_path_str_offset));
    text->vertical_spacing = cmd->vertical_spacing;
    text->control_points   = cmd->control_point;

    // defer font construction to end of frame (TODO change name...)
    hashmap_set(materials_with_new_pso, &text->id);
}

// ==optimize== group materials by font
// GText which share the same font_path can share the same material
// actually we can't: consider two GTexts same fonts but different color.
R_Text* Component_CreateText(GraphicsContext* gctx, FT_Library ft,
                             SG_Command_TextRebuild* cmd)
{
    // see if text is already created
    R_Text* text = Component_GetText(cmd->text_id);
    if (text) {
        R_Text_updateFromCommand(text, cmd);
        return text;
    }

    // else lazily create

    // OOP is creeping in here....
    // can factor these out into separate fns: createMaterialImpl, createGeometryImpl,
    // createMeshImpl

    // initialize material
    R_Material* mat = Component_GetMaterial(cmd->material_id);
    ASSERT(mat);

    // init geometry
    R_Geometry* geo = ARENA_PUSH_TYPE(&geoArena, R_Geometry);
    {
        *geo              = {};
        geo->id           = getNewRID();
        geo->type         = SG_COMPONENT_GEOMETRY;
        geo->vertex_count = -1; // -1 means draw all vertices

        // store offset
        R_Location loc     = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
        const void* result = hashmap_set(r_locator, &loc);
        ASSERT(result == NULL); // ensure id is unique
    }

    // init text
    text = ARENA_PUSH_ZERO_TYPE(&textArena,
                                R_Text); // can also add void* udata to R_Transform to
                                         // support these kinds of renderables
    {
        R_Transform_init(text, cmd->text_id, SG_COMPONENT_TEXT); // text or mesh type?

        // init mesh
        text->_geoID = geo->id;
        text->_matID = mat->id;

        // store offset
        R_Location loc = { text->id, Arena::offsetOf(&textArena, text), &textArena };
        const void* result = hashmap_set(r_locator, &loc);
        ASSERT(result == NULL); // ensure id is unique

        // make sure these are internal
        ASSERT(text->_geoID < 0);

        R_Text_updateFromCommand(text, cmd);
    }

    return text;
}

R_Scene* Component_CreateScene(GraphicsContext* gctx, SG_ID scene_id,
                               SG_SceneDesc* sg_scene_desc)
{
    Arena* arena     = &sceneArena;
    R_Scene* r_scene = ARENA_PUSH_ZERO_TYPE(arena, R_Scene);
    R_Scene::initFromSG(gctx, r_scene, scene_id, sg_scene_desc);

    ASSERT(r_scene->id != 0);                    // ensure id is set
    ASSERT(r_scene->type == SG_COMPONENT_SCENE); // ensure type is set

    // store offset
    R_Location loc     = { r_scene->id, Arena::offsetOf(arena, r_scene), arena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return r_scene;
}

R_Geometry* Component_CreateGeometry()
{
    R_Geometry* geo = ARENA_PUSH_ZERO_TYPE(&geoArena, R_Geometry);
    R_Geometry::init(geo);

    ASSERT(geo->id != 0);                       // ensure id is set
    ASSERT(geo->type == SG_COMPONENT_GEOMETRY); // ensure type is set

    // store offset
    R_Location loc     = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Geometry* Component_CreateGeometry(GraphicsContext* gctx, SG_ID geo_id)
{
    R_Geometry* geo = ARENA_PUSH_ZERO_TYPE(&geoArena, R_Geometry);

    geo->id           = geo_id;
    geo->type         = SG_COMPONENT_GEOMETRY;
    geo->vertex_count = -1; // -1 means draw all vertices

    // for now not storing geo_type (cube, sphere, custom etc.)
    // we only store the GPU vertex data, and don't care about semantics

    // store offset
    R_Location loc     = { geo->id, Arena::offsetOf(&geoArena, geo), &geoArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return geo;
}

R_Shader* Component_CreateShader(GraphicsContext* gctx, SG_Command_ShaderCreate* cmd)
{
    R_Shader* shader = ARENA_PUSH_ZERO_TYPE(&shaderArena, R_Shader);

    shader->id   = cmd->sg_id;
    shader->type = SG_COMPONENT_SHADER;

    // shader member vars
    const char* vertex_string
      = (const char*)CQ_ReadCommandGetOffset(cmd->vertex_string_offset);
    const char* fragment_string
      = (const char*)CQ_ReadCommandGetOffset(cmd->fragment_string_offset);
    const char* vertex_filepath
      = (const char*)CQ_ReadCommandGetOffset(cmd->vertex_filepath_offset);
    const char* fragment_filepath
      = (const char*)CQ_ReadCommandGetOffset(cmd->fragment_filepath_offset);
    const char* compute_filepath
      = (const char*)CQ_ReadCommandGetOffset(cmd->compute_filepath_offset);
    const char* compute_string
      = (const char*)CQ_ReadCommandGetOffset(cmd->compute_string_offset);

    ASSERT(sizeof(cmd->vertex_layout) == sizeof(shader->vertex_layout));
    R_Shader::init(gctx, shader, vertex_string, vertex_filepath, fragment_string,
                   fragment_filepath, cmd->vertex_layout,
                   ARRAY_LENGTH(cmd->vertex_layout), compute_string, compute_filepath,
                   cmd->lit);

    // store offset
    R_Location loc
      = { shader->id, Arena::offsetOf(&shaderArena, shader), &shaderArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return shader;
}

R_Material* Component_CreateMaterial(GraphicsContext* gctx,
                                     SG_Command_MaterialCreate* cmd)
{
    R_Material* mat = ARENA_PUSH_ZERO_TYPE(&materialArena, R_Material);

    // initialize
    {
        *mat                  = {};
        mat->id               = cmd->sg_id;
        mat->type             = SG_COMPONENT_MATERIAL;
        mat->bind_group_stale = true;

        // init uniform buffer
        GPU_Buffer::init(gctx, &mat->uniform_buffer, WGPUBufferUsage_Uniform,
                         MAX(sizeof(SG_MaterialUniformData),
                             gctx->limits.minUniformBufferOffsetAlignment)
                           * ARRAY_LENGTH(mat->bindings));

        R_Material::updatePSO(gctx, mat, &cmd->pso);
    }

    // store offset
    R_Location loc = { mat->id, Arena::offsetOf(&materialArena, mat), &materialArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return mat;
}

// called by renderer after flushing all commands
void Material_batchUpdatePipelines(GraphicsContext* gctx, FT_Library ft_lib,
                                   R_Font* default_font)
{
    // TODO:
    // handle xforms changing geo/mat by marking g2m as stale in add subgraph to scene
    // fn
    size_t hashmap_index_DONT_USE = 0;
    SG_ID* sg_id;
    while (
      hashmap_iter(materials_with_new_pso, &hashmap_index_DONT_USE, (void**)&sg_id)) {
        R_Component* comp = Component_GetComponent(*sg_id);
        switch (comp->type) {
            case SG_COMPONENT_MATERIAL: {
                R_Material* mat = (R_Material*)comp;

                // don't process ScreenPass materials in the RenderPass
                if (mat->pso.exclude_from_render_pass) continue;

                R_RenderPipeline* pipeline = Component_GetPipeline(gctx, &mat->pso);

                ASSERT(mat->pipeline_stale);
                mat->pipeline_stale = false;

                // add material to pipeline
                R_RenderPipeline::addMaterial(pipeline, mat);
                ASSERT(mat->pipelineID == pipeline->rid);
            } break;
            case SG_COMPONENT_TEXT: {
                R_Text* rtext = (R_Text*)comp;
                R_Font* font
                  = Component_GetFont(gctx, ft_lib, rtext->font_path.c_str());
                if (!font) font = default_font;
                R_Font::updateText(gctx, font, rtext);
            } break;
            default: ASSERT(false);
        }
    }

    // clear hashmap
    hashmap_clear(materials_with_new_pso, false);
}

R_Texture* Component_CreateTexture(GraphicsContext* gctx, SG_Command_TextureCreate* cmd)
{
    R_Texture* tex = ARENA_PUSH_TYPE(&textureArena, R_Texture);
    *tex           = {};

    // R_Component init
    tex->id   = cmd->sg_id;
    tex->type = SG_COMPONENT_TEXTURE;

    // R_Texture init
    R_Texture::init(gctx, tex, &cmd->desc);

    // store offset
    R_Location loc = { tex->id, Arena::offsetOf(&textureArena, tex), &textureArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return tex;
}

R_Pass* Component_CreatePass(SG_ID pass_id)
{
    Arena* arena = &passArena;
    R_Pass* pass = ARENA_PUSH_TYPE(arena, R_Pass);
    *pass        = {};

    // SG_Component init
    pass->id   = pass_id;
    pass->type = SG_COMPONENT_PASS;

    // store offset
    R_Location loc     = { pass->id, Arena::offsetOf(arena, pass), arena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return pass;
}

R_Buffer* Component_CreateBuffer(SG_ID id)
{
    Arena* arena     = &bufferArena;
    R_Buffer* buffer = ARENA_PUSH_TYPE(arena, R_Buffer);
    *buffer          = {};

    // SG_Component init
    buffer->id   = id;
    buffer->type = SG_COMPONENT_BUFFER;

    // store offset
    R_Location loc     = { buffer->id, Arena::offsetOf(arena, buffer), arena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return buffer;
}

R_Light* Component_CreateLight(SG_ID id, SG_LightDesc* desc)
{
    R_Light* light = ARENA_PUSH_ZERO_TYPE(&lightArena, R_Light);

    R_Transform_init(light, id, SG_COMPONENT_LIGHT);

    // light init
    light->desc = *desc;

    // store offset
    R_Location loc = { light->id, Arena::offsetOf(&lightArena, light), &lightArena };
    const void* result = hashmap_set(r_locator, &loc);
    ASSERT(result == NULL); // ensure id is unique

    return light;
}

// linear search by font path, lazily creates if not found
R_Font* Component_GetFont(GraphicsContext* gctx, FT_Library library,
                          const char* font_path)
{
    if (font_path == NULL || strlen(font_path) == 0) return NULL;

    for (int i = 0; i < component_font_count; ++i) {
        // this lookup won't work for loading fonts from memory
        // actually it will if we give builtin fonts special names
        if (component_fonts[i].font_path == font_path) {
            return &component_fonts[i];
        }
    }

    R_Font* font = &component_fonts[component_font_count];
    if (R_Font::init(gctx, library, font, font_path)) {
        component_font_count++;
        return font;
    }
    return NULL;
}

R_Component* Component_GetComponent(SG_ID id)
{
    R_Location loc     = { id, 0, NULL };
    R_Location* result = (R_Location*)hashmap_get(r_locator, &loc);
    return result ? (R_Component*)Arena::get(result->arena, result->offset) : NULL;
}

R_Transform* Component_GetXform(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    if (comp) {
        ASSERT(comp->type == SG_COMPONENT_TRANSFORM || comp->type == SG_COMPONENT_SCENE
               || comp->type == SG_COMPONENT_MESH || comp->type == SG_COMPONENT_CAMERA
               || comp->type == SG_COMPONENT_TEXT || comp->type == SG_COMPONENT_LIGHT);
    }
    return (R_Transform*)comp;
}

R_Transform* Component_GetMesh(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_MESH);
    return (R_Transform*)comp;
}

R_Scene* Component_GetScene(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_SCENE);
    return (R_Scene*)comp;
}

R_Geometry* Component_GetGeometry(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_GEOMETRY);
    return (R_Geometry*)comp;
}

R_Shader* Component_GetShader(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_SHADER);
    return (R_Shader*)comp;
}

R_Material* Component_GetMaterial(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_MATERIAL);
    return (R_Material*)comp;
}

R_Texture* Component_GetTexture(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_TEXTURE);
    return (R_Texture*)comp;
}

R_Camera* Component_GetCamera(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_CAMERA);
    return (R_Camera*)comp;
}

R_Text* Component_GetText(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_TEXT);
    return (R_Text*)comp;
}

R_Pass* Component_GetPass(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_PASS);
    return (R_Pass*)comp;
}

R_Buffer* Component_GetBuffer(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_BUFFER);
    return (R_Buffer*)comp;
}

R_Light* Component_GetLight(SG_ID id)
{
    R_Component* comp = Component_GetComponent(id);
    ASSERT(comp == NULL || comp->type == SG_COMPONENT_LIGHT);
    return (R_Light*)comp;
}

bool Component_MaterialIter(size_t* i, R_Material** material)
{
    if (*i >= ARENA_LENGTH(&materialArena, R_Material)) {
        *material = NULL;
        return false;
    }

    *material = ARENA_GET_TYPE(&materialArena, R_Material, *i);
    ++(*i);
    return true;
}

bool Component_RenderPipelineIter(size_t* i, R_RenderPipeline** renderPipeline)
{
    if (*i >= ARENA_LENGTH(&_RenderPipelineArena, R_RenderPipeline)) {
        *renderPipeline = NULL;
        return false;
    }

    // Possible optimization: pack nonempty pipelines at start, swap empty
    // pipelines to end
    *renderPipeline = ARENA_GET_TYPE(&_RenderPipelineArena, R_RenderPipeline, *i);
    ++(*i);
    return true;
}

int Component_RenderPipelineCount()
{
    return ARENA_LENGTH(&_RenderPipelineArena, R_RenderPipeline);
}

R_RenderPipeline* Component_GetPipeline(GraphicsContext* gctx,
                                        SG_MaterialPipelineState* pso)
{
    RenderPipelinePSOTableItem* rp_item
      = (RenderPipelinePSOTableItem*)hashmap_get(render_pipeline_pso_table, pso);
    if (rp_item)
        return (R_RenderPipeline*)Arena::get(&_RenderPipelineArena,
                                             rp_item->pipeline_offset);

    // else create a new one
    R_RenderPipeline* rPipeline
      = ARENA_PUSH_ZERO_TYPE(&_RenderPipelineArena, R_RenderPipeline);
    u64 pipelineOffset = Arena::offsetOf(&_RenderPipelineArena, rPipeline);
    R_RenderPipeline::init(gctx, rPipeline, pso);

    ASSERT(!hashmap_get(_RenderPipelineMap, &rPipeline->rid))
    RenderPipelineIDTableItem new_ri_item = { rPipeline->rid, pipelineOffset };
    hashmap_set(_RenderPipelineMap, &new_ri_item);

    RenderPipelinePSOTableItem new_rp_item = { *pso, pipelineOffset };
    hashmap_set(render_pipeline_pso_table, &new_rp_item);

    return rPipeline;
}

R_RenderPipeline* Component_GetPipeline(R_ID rid)
{
    return (R_RenderPipeline*)hashmap_get(_RenderPipelineMap, &rid);
}

// =============================================================================
// R_Shader
// =============================================================================

void R_Shader::init(GraphicsContext* gctx, R_Shader* shader, const char* vertex_string,
                    const char* vertex_filepath, const char* fragment_string,
                    const char* fragment_filepath, WGPUVertexFormat* vertex_layout,
                    int vertex_layout_count, const char* compute_string,
                    const char* compute_filepath, bool lit)
{
    shader->lit = lit;

    char vertex_shader_label[32] = {};
    snprintf(vertex_shader_label, sizeof(vertex_shader_label), "vertex shader %d",
             (int)shader->id);
    char fragment_shader_label[32] = {};
    snprintf(fragment_shader_label, sizeof(fragment_shader_label), "fragment shader %d",
             (int)shader->id);

    if (vertex_string && strlen(vertex_string) > 0) {
        shader->vertex_shader_module
          = G_createShaderModule(gctx, vertex_string, vertex_shader_label);
    } else if (vertex_filepath && strlen(vertex_filepath) > 0) {
        // read entire file contents
        FileReadResult vertex_file = File_read(vertex_filepath, true);
        if (vertex_file.data_owned) {
            shader->vertex_shader_module = G_createShaderModule(
              gctx, (const char*)vertex_file.data_owned, vertex_shader_label);
            FREE(vertex_file.data_owned);
        } else {
            log_error("failed to read vertex shader file %s", vertex_filepath);
        }
    }

    if (fragment_string && strlen(fragment_string) > 0) {
        shader->fragment_shader_module
          = G_createShaderModule(gctx, fragment_string, fragment_shader_label);
    } else if (fragment_filepath && strlen(fragment_filepath) > 0) {
        // read entire file contents
        FileReadResult fragment_file = File_read(fragment_filepath, true);
        if (fragment_file.data_owned) {
            shader->fragment_shader_module = G_createShaderModule(
              gctx, (const char*)fragment_file.data_owned, fragment_shader_label);
            FREE(fragment_file.data_owned);
        } else {
            log_error("failed to read fragment shader file %s", fragment_filepath);
        }
    }

    // copy vertex layout
    ASSERT(sizeof(*shader->vertex_layout) == sizeof(*vertex_layout));
    memcpy(shader->vertex_layout, vertex_layout,
           sizeof(*vertex_layout) * vertex_layout_count);

    // compute shaders
    char compute_shader_label[32] = {};
    snprintf(compute_shader_label, sizeof(compute_shader_label), "compute shader %d",
             (int)shader->id);
    if (compute_string && strlen(compute_string) > 0) {
        shader->compute_shader_module
          = G_createShaderModule(gctx, compute_string, compute_shader_label);
    } else if (compute_filepath && strlen(compute_filepath) > 0) {
        // read entire file contents
        FileReadResult compute_file = File_read(compute_filepath, true);
        if (compute_file.data_owned) {
            shader->compute_shader_module = G_createShaderModule(
              gctx, (const char*)compute_file.data_owned, compute_shader_label);
            FREE(compute_file.data_owned);
        } else {
            log_error("failed to read compute shader file %s", compute_filepath);
        }
    }
}

void R_Shader::free(R_Shader* shader)
{
    WGPU_RELEASE_RESOURCE(ShaderModule, shader->vertex_shader_module);
    WGPU_RELEASE_RESOURCE(ShaderModule, shader->fragment_shader_module);
}

// =============================================================================
// R_Font
// =============================================================================

// Decodes the first Unicode code point from the null-terminated UTF-8 string *text and
// advances *text to point at the next code point. If the encoding is invalid, advances
// *text by one byte and returns 0. *text should not be empty, because it will be
// advanced past the null terminator.
static u32 R_Font_decodeCharcode(char** text)
{
    uint8_t first = static_cast<uint8_t>((*text)[0]);

    // Fast-path for ASCII.
    if (first < 128) {
        (*text)++;
        return static_cast<uint32_t>(first);
    }

    // This could probably be optimized a bit.
    uint32_t result;
    int size;
    if ((first & 0xE0) == 0xC0) { // 110xxxxx
        result = first & 0x1F;
        size   = 2;
    } else if ((first & 0xF0) == 0xE0) { // 1110xxxx
        result = first & 0x0F;
        size   = 3;
    } else if ((first & 0xF8) == 0xF0) { // 11110xxx
        result = first & 0x07;
        size   = 4;
    } else {
        // Invalid encoding.
        (*text)++;
        return 0;
    }

    for (int i = 1; i < size; i++) {
        uint8_t value = static_cast<uint8_t>((*text)[i]);
        // Invalid encoding (also catches a null terminator in the middle of a code
        // point).
        if ((value & 0xC0) != 0x80) { // 10xxxxxx
            (*text)++;
            return 0;
        }
        result = (result << 6) | (value & 0x3F);
    }

    (*text) += size;
    return result;
}

FT_Face R_Font_loadFace(FT_Library library, const char* filename)
{
    FT_Face face = NULL;

    FT_Error ftError = FT_New_Face(library, filename, 0, &face);
    if (ftError) {
        const char* ftErrorStr = FT_Error_String(ftError);
        log_error("Error loading font face [error number %d] for file %s: %s", ftError,
                  filename, ftErrorStr);
        return NULL;
    }

    if (!(face->face_flags & FT_FACE_FLAG_SCALABLE)) {
        log_error("non-scalable fonts are not supported. Font file: %s", filename);
        FT_Done_Face(face);
        return NULL;
    }

    return face;
}

void R_Font::rebuildVertexBuffers(R_Font* font, const char* mainText, float x, float y,
                                  Arena* positions, Arena* uvs, Arena* glyph_indices,
                                  Arena* indices, float verticalScale)
{
    float originalX = x;

    FT_UInt previous = 0;
    char* textIt     = (char*)mainText;
    while (*textIt != '\0') {
        uint32_t charcode = R_Font_decodeCharcode(&textIt);

        if (charcode == '\r') continue;

        if (charcode == '\n') {
            x = originalX;
            y -= verticalScale
                 * ((float)font->face->height / (float)font->face->units_per_EM
                    * font->worldSize);
            continue;
        }

        auto glyphIt = font->glyphs.find(charcode);
        Glyph& glyph
          = (glyphIt == font->glyphs.end()) ? font->glyphs[0] : glyphIt->second;

        if (previous != 0 && glyph.index != 0) {
            FT_Vector kerning;
            FT_Error error = FT_Get_Kerning(font->face, previous, glyph.index,
                                            font->kerningMode, &kerning);
            if (!error) {
                x += (float)kerning.x / font->emSize * font->worldSize;
            }
        }

        // Do not emit quad for empty glyphs (whitespace).
        if (glyph.curveCount) {
            FT_Pos d = (FT_Pos)(font->emSize * font->dilation);

            float u0 = (float)(glyph.bearingX - d) / font->emSize;
            float v0 = (float)(glyph.bearingY - glyph.height - d) / font->emSize;
            float u1 = (float)(glyph.bearingX + glyph.width + d) / font->emSize;
            float v1 = (float)(glyph.bearingY + d) / font->emSize;

            float x0 = x + u0 * font->worldSize;
            float y0 = y + v0 * font->worldSize;
            float x1 = x + u1 * font->worldSize;
            float y1 = y + v1 * font->worldSize;

            u32 base                               = ARENA_LENGTH(positions, glm::vec2);
            *ARENA_PUSH_TYPE(positions, glm::vec2) = glm::vec2(x0, y0);
            *ARENA_PUSH_TYPE(positions, glm::vec2) = glm::vec2(x1, y0);
            *ARENA_PUSH_TYPE(positions, glm::vec2) = glm::vec2(x1, y1);
            *ARENA_PUSH_TYPE(positions, glm::vec2) = glm::vec2(x0, y1);

            *ARENA_PUSH_TYPE(uvs, glm::vec2) = glm::vec2(u0, v0);
            *ARENA_PUSH_TYPE(uvs, glm::vec2) = glm::vec2(u1, v0);
            *ARENA_PUSH_TYPE(uvs, glm::vec2) = glm::vec2(u1, v1);
            *ARENA_PUSH_TYPE(uvs, glm::vec2) = glm::vec2(u0, v1);

            *ARENA_PUSH_TYPE(glyph_indices, u32) = glyph.bufferIndex;
            *ARENA_PUSH_TYPE(glyph_indices, u32) = glyph.bufferIndex;
            *ARENA_PUSH_TYPE(glyph_indices, u32) = glyph.bufferIndex;
            *ARENA_PUSH_TYPE(glyph_indices, u32) = glyph.bufferIndex;

            *ARENA_PUSH_TYPE(indices, u32) = base;
            *ARENA_PUSH_TYPE(indices, u32) = base + 1;
            *ARENA_PUSH_TYPE(indices, u32) = base + 2;
            *ARENA_PUSH_TYPE(indices, u32) = base + 2;
            *ARENA_PUSH_TYPE(indices, u32) = base + 3;
            *ARENA_PUSH_TYPE(indices, u32) = base;
        }

        x += (float)glyph.advance / font->emSize * font->worldSize;
        previous = glyph.index;
    }
}

// ==optimize== consolidate with R_Font::rebuildBuffers
// calculate vertices assuming control point 0,0, and determine BB at same time
// after calculating bb, apply translation to vertices.
BoundingBox R_Font::measure(float x, float y, const char* text, float verticalScale)
{
    BoundingBox bb = {};
    bb.minX        = +std::numeric_limits<float>::infinity();
    bb.minY        = +std::numeric_limits<float>::infinity();
    bb.maxX        = -std::numeric_limits<float>::infinity();
    bb.maxY        = -std::numeric_limits<float>::infinity();

    float originalX = x;

    FT_UInt previous = 0;
    char* textIt     = (char*)text;
    while (*textIt != '\0') {
        uint32_t charcode = R_Font_decodeCharcode(&textIt);

        if (charcode == '\r') continue;

        if (charcode == '\n') {
            x = originalX;
            y -= verticalScale
                 * ((float)face->height / (float)face->units_per_EM * worldSize);
            continue;
        }

        auto glyphIt = glyphs.find(charcode);
        Glyph& glyph = (glyphIt == glyphs.end()) ? glyphs[0] : glyphIt->second;

        if (previous != 0 && glyph.index != 0) {
            FT_Vector kerning;
            FT_Error error
              = FT_Get_Kerning(face, previous, glyph.index, kerningMode, &kerning);
            if (!error) {
                x += (float)kerning.x / emSize * worldSize;
            }
        }

        // Note: Do not apply dilation here, we want to calculate exact bounds.
        float u0 = (float)(glyph.bearingX) / emSize;
        float v0 = (float)(glyph.bearingY - glyph.height) / emSize;
        float u1 = (float)(glyph.bearingX + glyph.width) / emSize;
        float v1 = (float)(glyph.bearingY) / emSize;

        float x0 = x + u0 * worldSize;
        float y0 = y + v0 * worldSize;
        float x1 = x + u1 * worldSize;
        float y1 = y + v1 * worldSize;

        if (x0 < bb.minX) bb.minX = x0;
        if (y0 < bb.minY) bb.minY = y0;
        if (x1 > bb.maxX) bb.maxX = x1;
        if (y1 > bb.maxY) bb.maxY = y1;

        x += (float)glyph.advance / emSize * worldSize;
        previous = glyph.index;
    }

    return bb;
}

// This function takes a single contour (defined by firstIndex and
// lastIndex, both inclusive) from outline and converts it into individual
// quadratic bezier curves, which are added to the curves vector.
static void convertContour(std::vector<BufferCurve>& curves, const FT_Outline* outline,
                           short firstIndex, short lastIndex, float emSize)
{
    // See https://freetype.org/freetype2/docs/glyphs/glyphs-6.html
    // for a detailed description of the outline format.
    //
    // In short, a contour is a list of points describing line segments
    // and quadratic or cubic bezier curves that form a closed shape.
    //
    // TrueType fonts only contain quadratic bezier curves. OpenType fonts
    // may contain outline data in TrueType format or in Compact Font
    // Format, which also allows cubic beziers. However, in FreeType it is
    // (theoretically) possible to mix the two types of bezier curves, so
    // we handle both at the same time.
    //
    // Each point in the contour has a tag specifying its type
    // (FT_CURVE_TAG_ON, FT_CURVE_TAG_CONIC or FT_CURVE_TAG_CUBIC).
    // FT_CURVE_TAG_ON points sit exactly on the outline, whereas the
    // other types are control points for quadratic/conic bezier curves,
    // which in general do not sit exactly on the outline and are also
    // called off points.
    //
    // Some examples of the basic segments:
    // ON - ON ... line segment
    // ON - CONIC - ON ... quadratic bezier curve
    // ON - CUBIC - CUBIC - ON ... cubic bezier curve
    //
    // Cubic bezier curves must always be described by two CUBIC points
    // inbetween two ON points. For the points used in the TrueType format
    // (ON, CONIC) there is a special rule, that two consecutive points of
    // the same type imply a virtual point of the opposite type at their
    // exact midpoint.
    //
    // For example the sequence ON - CONIC - CONIC - ON describes two
    // quadratic bezier curves where the virtual point forms the joining
    // end point of the two curves: ON - CONIC - [ON] - CONIC - ON.
    //
    // Similarly the sequence ON - ON can be thought of as a line segment
    // or a quadratic bezier curve (ON - [CONIC] - ON). Because the
    // virtual point is at the exact middle of the two endpoints, the
    // bezier curve is identical to the line segment.
    //
    // The font shader only supports quadratic bezier curves, so we use
    // this virtual point rule to represent line segments as quadratic
    // bezier curves.
    //
    // Cubic bezier curves are slightly more difficult, since they have a
    // higher degree than the shader supports. Each cubic curve is
    // approximated by two quadratic curves according to the following
    // paper. This preserves C1-continuity (location of and tangents at
    // the end points of the cubic curve) and the paper even proves that
    // splitting at the parametric center minimizes the error due to the
    // degree reduction. One could also analyze the approximation error
    // and split the cubic curve, if the error is too large. However,
    // almost all fonts use "nice" cubic curves, resulting in very small
    // errors already (see also the section on Font Design in the paper).
    //
    // Quadratic Approximation of Cubic Curves
    // Nghia Truong, Cem Yuksel, Larry Seiler
    // https://ttnghia.github.io/pdf/QuadraticApproximation.pdf
    // https://doi.org/10.1145/3406178

    if (firstIndex == lastIndex) return;

    short dIndex = 1;
    if (outline->flags & FT_OUTLINE_REVERSE_FILL) {
        short tmpIndex = lastIndex;
        lastIndex      = firstIndex;
        firstIndex     = tmpIndex;
        dIndex         = -1;
    }

    auto convert = [emSize](const FT_Vector& v) {
        return glm::vec2((float)v.x / emSize, (float)v.y / emSize);
    };

    auto makeMidpoint
      = [](const glm::vec2& a, const glm::vec2& b) { return 0.5f * (a + b); };

    auto makeCurve = [](const glm::vec2& p0, const glm::vec2& p1, const glm::vec2& p2) {
        BufferCurve result;
        result.x0 = p0.x;
        result.y0 = p0.y;
        result.x1 = p1.x;
        result.y1 = p1.y;
        result.x2 = p2.x;
        result.y2 = p2.y;
        return result;
    };

    // Find a point that is on the curve and remove it from the list.
    glm::vec2 first;
    bool firstOnCurve = (outline->tags[firstIndex] & FT_CURVE_TAG_ON);
    if (firstOnCurve) {
        first = convert(outline->points[firstIndex]);
        firstIndex += dIndex;
    } else {
        bool lastOnCurve = (outline->tags[lastIndex] & FT_CURVE_TAG_ON);
        if (lastOnCurve) {
            first = convert(outline->points[lastIndex]);
            lastIndex -= dIndex;
        } else {
            first = makeMidpoint(convert(outline->points[firstIndex]),
                                 convert(outline->points[lastIndex]));
            // This is a virtual point, so we don't have to remove it.
        }
    }

    glm::vec2 start    = first;
    glm::vec2 control  = first;
    glm::vec2 previous = first;
    char previousTag   = FT_CURVE_TAG_ON;
    for (short index = firstIndex; index != lastIndex + dIndex; index += dIndex) {
        glm::vec2 current = convert(outline->points[index]);
        char currentTag   = FT_CURVE_TAG(outline->tags[index]);
        if (currentTag == FT_CURVE_TAG_CUBIC) {
            // No-op, wait for more points.
            control = previous;
        } else if (currentTag == FT_CURVE_TAG_ON) {
            if (previousTag == FT_CURVE_TAG_CUBIC) {
                glm::vec2& b0 = start;
                glm::vec2& b1 = control;
                glm::vec2& b2 = previous;
                glm::vec2& b3 = current;

                glm::vec2 c0 = b0 + 0.75f * (b1 - b0);
                glm::vec2 c1 = b3 + 0.75f * (b2 - b3);

                glm::vec2 d = makeMidpoint(c0, c1);

                curves.push_back(makeCurve(b0, c0, d));
                curves.push_back(makeCurve(d, c1, b3));
            } else if (previousTag == FT_CURVE_TAG_ON) {
                // Linear segment.
                curves.push_back(
                  makeCurve(previous, makeMidpoint(previous, current), current));
            } else {
                // Regular bezier curve.
                curves.push_back(makeCurve(start, previous, current));
            }
            start   = current;
            control = current;
        } else /* currentTag == FT_CURVE_TAG_CONIC */
        {
            if (previousTag == FT_CURVE_TAG_ON) {
                // No-op, wait for third point.
            } else {
                // Create virtual on point.
                glm::vec2 mid = makeMidpoint(previous, current);
                curves.push_back(makeCurve(start, previous, mid));
                start   = mid;
                control = mid;
            }
        }
        previous    = current;
        previousTag = currentTag;
    }

    // Close the contour.
    if (previousTag == FT_CURVE_TAG_CUBIC) {
        glm::vec2& b0 = start;
        glm::vec2& b1 = control;
        glm::vec2& b2 = previous;
        glm::vec2& b3 = first;

        glm::vec2 c0 = b0 + 0.75f * (b1 - b0);
        glm::vec2 c1 = b3 + 0.75f * (b2 - b3);

        glm::vec2 d = makeMidpoint(c0, c1);

        curves.push_back(makeCurve(b0, c0, d));
        curves.push_back(makeCurve(d, c1, b3));
    } else if (previousTag == FT_CURVE_TAG_ON) {
        // Linear segment.
        curves.push_back(makeCurve(previous, makeMidpoint(previous, first), first));
    } else {
        curves.push_back(makeCurve(start, previous, first));
    }
}

static void R_Font_buildGlyph(R_Font* font, u32 charcode, FT_UInt glyphIndex)
{
    BufferGlyph bufferGlyph;
    bufferGlyph.start = (i32)font->bufferCurves.size();

    short start = 0;
    for (int i = 0; i < font->face->glyph->outline.n_contours; i++) {
        // Note: The end indices in face->glyph->outline.contours are inclusive.
        convertContour(font->bufferCurves, &font->face->glyph->outline, start,
                       font->face->glyph->outline.contours[i], font->emSize);
        start = font->face->glyph->outline.contours[i] + 1;
    }

    bufferGlyph.count = (i32)font->bufferCurves.size() - bufferGlyph.start;

    i32 bufferIndex = (i32)font->bufferGlyphs.size();
    font->bufferGlyphs.push_back(bufferGlyph);

    Glyph glyph;
    glyph.index            = glyphIndex;
    glyph.bufferIndex      = bufferIndex;
    glyph.curveCount       = bufferGlyph.count;
    glyph.width            = font->face->glyph->metrics.width;
    glyph.height           = font->face->glyph->metrics.height;
    glyph.bearingX         = font->face->glyph->metrics.horiBearingX;
    glyph.bearingY         = font->face->glyph->metrics.horiBearingY;
    glyph.advance          = font->face->glyph->metrics.horiAdvance;
    font->glyphs[charcode] = glyph;
}

// updates gpu buffers with new glyph/curve data
static void R_Font_uploadBuffers(GraphicsContext* gctx, R_Font* font)
{
    // could only write new data (can calculate offset from bufferGlyphs.size - gpu
    // buffer curr size)
    // check limits->minStorageBufferOffsetAlignment first
    // but not needed as these are small buffers relative to PCIe bandwidth

    // make sure we only rewrite on getting new glyph data
    ASSERT(sizeof(BufferGlyph) * font->bufferGlyphs.size() > font->glyph_buffer.size);
    ASSERT(sizeof(BufferCurve) * font->bufferCurves.size() > font->curve_buffer.size);

    GPU_Buffer::write(gctx, &font->glyph_buffer, WGPUBufferUsage_Storage,
                      font->bufferGlyphs.data(),
                      sizeof(BufferGlyph) * font->bufferGlyphs.size());
    GPU_Buffer::write(gctx, &font->curve_buffer, WGPUBufferUsage_Storage,
                      font->bufferCurves.data(),
                      sizeof(BufferCurve) * font->bufferCurves.size());
}

// impl in imgui_draw.cpp
unsigned char* imgui_decompressBase85TTF(const char* compressed_ttf_data_base85,
                                         int* out_size);

bool R_Font::init(GraphicsContext* gctx, FT_Library library, R_Font* font,
                  const char* font_path)
{
    ASSERT(font->face == NULL);
    *font           = {}; // init defaults
    font->font_path = std::string(font_path);
    ASSERT(font->worldSize > 0.0f);

    log_debug("Creating new R_Font with font path: %s", font_path);

    // DEFAULT FONTS
    // if font path starts with chugl:
    // then it is a builtin font we load from memory
    if (strncmp(font_path, "chugl:", 6) == 0) {
        unsigned char* font_data = NULL;
        int font_memory_size     = 0;
        if (strncmp(&font_path[6], "cousine-regular", 15) == 0) {
            font_data = imgui_decompressBase85TTF(
              cousine_regular_compressed_data_base85, &font_memory_size);
            ASSERT(font_memory_size == 43912);
        } else if (strncmp(&font_path[6], "karla-regular", 13) == 0) {
            font_data = imgui_decompressBase85TTF(karla_regular_compressed_data_base85,
                                                  &font_memory_size);
            ASSERT(font_memory_size == 16848);
        } else if (strncmp(&font_path[6], "proggy-tiny", 11) == 0) {
            font_data = imgui_decompressBase85TTF(proggy_tiny_compressed_data_base85,
                                                  &font_memory_size);
            ASSERT(font_memory_size == 35656);
        } else if (strncmp(&font_path[6], "proggy-clean", 12) == 0) {
            font_data = imgui_decompressBase85TTF(proggy_clean_compressed_data_base85,
                                                  &font_memory_size);
            ASSERT(font_memory_size == 41208);
        }

        FT_Error error = FT_New_Memory_Face(library, (const FT_Byte*)font_data,
                                            font_memory_size, 0, &font->face);

        if (error) {
            log_error("error while loading font face from memory: %d", error);
            return false;
        }
    } else {
        font->face = R_Font_loadFace(library, font_path);
    }

    FT_Face face = font->face;

    font->loadFlags   = FT_LOAD_NO_SCALE | FT_LOAD_NO_HINTING | FT_LOAD_NO_BITMAP;
    font->kerningMode = FT_KERNING_UNSCALED;
    font->emSize      = font->face->units_per_EM;

    { // build undefined glyph
        uint32_t charcode  = 0;
        FT_UInt glyphIndex = 0;
        FT_Error error     = FT_Load_Glyph(font->face, glyphIndex, font->loadFlags);
        if (error) {
            log_error("error while loading undefined glyph: %d", error);
            return false;
            // Continue, because we always want an entry for the undefined glyph in our
            // glyphs map!
        }

        R_Font_buildGlyph(font, charcode, glyphIndex);
    }

    // build glyphs for ASCII characters
    // 32-127 are printable ASCII characters
    for (uint32_t charcode = 32; charcode < 128; charcode++) {
        FT_UInt glyphIndex = FT_Get_Char_Index(face, charcode);
        if (!glyphIndex) continue;

        FT_Error error = FT_Load_Glyph(face, glyphIndex, font->loadFlags);
        if (error) {
            log_error("error while loading glyph for character %d: %d", charcode,
                      error);
            continue;
        }

        R_Font_buildGlyph(font, charcode, glyphIndex);
    }

    R_Font_uploadBuffers(gctx, font);
    font->font_path = std::string(font_path);
    return true;
}

void R_Font::updateText(GraphicsContext* gctx, R_Font* font, R_Text* text)
{
    static Arena positions;
    static Arena uvs;
    static Arena glyph_indices;
    static Arena indices;

    // clear the buffers
    Arena::clear(&positions);
    Arena::clear(&uvs);
    Arena::clear(&glyph_indices);
    Arena::clear(&indices);

    // generate new glyps for this font
    R_Font::prepareGlyphsForText(gctx, font, text->text.c_str());

    // compute new bounding box
    BoundingBox bb = font->measure(0, 0, text->text.c_str(), text->vertical_spacing);

    // update material bindgroup
    R_Material* mat = Component_GetMaterial(text->_matID);
    R_Material::setExternalStorageBinding(gctx, mat, 0, &font->glyph_buffer);
    R_Material::setExternalStorageBinding(gctx, mat, 1, &font->curve_buffer);

    float cx = bb.minX + text->control_points.x * (bb.maxX - bb.minX);
    float cy = bb.minY + text->control_points.y * (bb.maxY - bb.minY);

    R_Font::rebuildVertexBuffers(font, text->text.c_str(), -cx, -cy, &positions, &uvs,
                                 &glyph_indices, &indices, text->vertical_spacing);

    // write buffer data to geometry
    R_Geometry* geo = Component_GetGeometry(text->_geoID);
    R_Geometry::setVertexAttribute(gctx, geo, 0, 2, positions.base, positions.curr);
    R_Geometry::setVertexAttribute(gctx, geo, 1, 2, uvs.base, uvs.curr);
    R_Geometry::setVertexAttribute(gctx, geo, 2, 1, glyph_indices.base,
                                   glyph_indices.curr);
    R_Geometry::setIndices(gctx, geo, (u32*)indices.base, ARENA_LENGTH(&indices, u32));

    // set internal uniforms
    // recompute bb adjusted by control points
    BoundingBox adjust_bb = { bb.minX - cx, bb.minY - cy, bb.maxX - cx, bb.maxY - cy };
    R_Material::setUniformBinding(gctx, mat, 5, &adjust_bb, sizeof(adjust_bb));

    // leq because whitespaces are skipped
    ASSERT(ARENA_LENGTH(&indices, u32) <= text->text.length() * 6);
}

// build new glyphs if text has unseen characters
void R_Font::prepareGlyphsForText(GraphicsContext* gctx, R_Font* font, const char* text)
{
    bool changed = false;

    char* textIt = (char*)text;
    while (*textIt != '\0') {
        uint32_t charcode = R_Font_decodeCharcode(&textIt);

        if (charcode == '\r' || charcode == '\n') continue;
        if (font->glyphs.count(charcode) != 0) continue; // if already exists, move on

        FT_UInt glyphIndex = FT_Get_Char_Index(font->face, charcode);
        if (!glyphIndex) continue;

        FT_Error error = FT_Load_Glyph(font->face, glyphIndex, font->loadFlags);
        if (error) {
            log_error("error while loading glyph for character %d: %d", charcode,
                      error);
            continue;
        }

        R_Font_buildGlyph(font, charcode, glyphIndex);
        changed = true;
    }

    if (changed) {
        // Reupload the full buffer contents. To make this even more
        // dynamic, the buffers could be overallocated and only the added
        // data could be uploaded.
        // not necessary, glyph+curve buffers are only ~200kb for ASCII chars
        // 3080 PCI has 32GB/s bandwidth, these buffers are nothing
        R_Font_uploadBuffers(gctx, font);
    }
}

// =============================================================================
// R_Pass
// =============================================================================

// static array of ScreenPass render pipelines
// all supported texture formats created on app start
static int r_screen_pass_pipeline_count                 = 0;
static R_ScreenPassPipeline r_screen_pass_pipelines[32] = {};
WGPUShaderModule screen_pass_default_passthrough_vs     = NULL;
WGPUShaderModule screen_pass_default_passthrough_fs     = NULL;

R_ScreenPassPipeline R_GetScreenPassPipeline(GraphicsContext* gctx,
                                             WGPUTextureFormat format, SG_ID shader_id)
{

    if (!screen_pass_default_passthrough_vs || !screen_pass_default_passthrough_fs) {
        screen_pass_default_passthrough_vs = G_createShaderModule(
          gctx, default_postprocess_shader_string, "ScreenPass Default VS");
        screen_pass_default_passthrough_fs = G_createShaderModule(
          gctx, default_postprocess_shader_string, "ScreenPass Default FS");
    }

    for (int i = 0; i < r_screen_pass_pipeline_count; i++) {
        if (r_screen_pass_pipelines[i].format == format
            && r_screen_pass_pipelines[i].shader_id == shader_id) {
            return r_screen_pass_pipelines[i];
        }
    }
    // else create
    ASSERT(r_screen_pass_pipeline_count < ARRAY_LENGTH(r_screen_pass_pipelines));

    WGPUPrimitiveState primitiveState = {};
    primitiveState.topology           = WGPUPrimitiveTopology_TriangleList;
    primitiveState.cullMode           = WGPUCullMode_Back;

    // TODO transparency (dissallow partial transparency, see if fragment
    // discard writes to the depth buffer)
    WGPUBlendState blendState = G_createBlendState(false);

    WGPUColorTargetState colorTargetState = {};
    colorTargetState.format               = format;
    colorTargetState.blend                = &blendState;
    colorTargetState.writeMask            = WGPUColorWriteMask_All;

    // Setup shader module (defaults to passthrough shader)
    WGPUShaderModule vs_module = screen_pass_default_passthrough_vs;
    WGPUShaderModule fs_module = screen_pass_default_passthrough_fs;
    R_Shader* shader           = Component_GetShader(shader_id);
    if (shader && shader->vertex_shader_module && shader->fragment_shader_module) {
        vs_module = shader->vertex_shader_module;
        fs_module = shader->fragment_shader_module;
    }

    // vertex state
    WGPUVertexState vertexState = {};
    vertexState.module          = vs_module;
    vertexState.entryPoint      = VS_ENTRY_POINT;

    // fragment state
    WGPUFragmentState fragmentState = {};
    fragmentState.module            = fs_module;
    fragmentState.entryPoint        = FS_ENTRY_POINT;
    fragmentState.targetCount       = 1;
    fragmentState.targets           = &colorTargetState;

    // multisample state
    WGPUMultisampleState multisampleState = G_createMultisampleState(1);

    char pipeline_label[64] = {};
    if (shader) {
        snprintf(pipeline_label, sizeof(pipeline_label),
                 "ScreenPass RenderPipeline %lld %s", (i64)shader->id,
                 shader->name.c_str());
    }
    WGPURenderPipelineDescriptor pipeline_desc = {};
    pipeline_desc.label                        = pipeline_label;
    pipeline_desc.layout                       = NULL; // Using layout: auto
    pipeline_desc.primitive                    = primitiveState;
    pipeline_desc.vertex                       = vertexState;
    pipeline_desc.fragment                     = &fragmentState;
    pipeline_desc.depthStencil                 = NULL;
    pipeline_desc.multisample                  = multisampleState;

    R_ScreenPassPipeline* pipeline
      = &r_screen_pass_pipelines[r_screen_pass_pipeline_count++];
    pipeline->format    = format;
    pipeline->shader_id = shader_id;
    pipeline->gpu_pipeline
      = wgpuDeviceCreateRenderPipeline(gctx->device, &pipeline_desc);
    pipeline->frame_group_layout = wgpuRenderPipelineGetBindGroupLayout(
      pipeline->gpu_pipeline, 0); // 0 is the frame bind group

    return *pipeline;
}

static int r_compute_pass_pipeline_count                  = 0;
static R_ComputePassPipeline r_compute_pass_pipelines[64] = {};

R_ComputePassPipeline R_GetComputePassPipeline(GraphicsContext* gctx, R_Shader* shader)
{
    // first linear search
    for (int i = 0; i < r_compute_pass_pipeline_count; i++) {
        if (r_compute_pass_pipelines[i].shader_id == shader->id) {
            return r_compute_pass_pipelines[i];
        }
    }

    WGPUComputePipelineDescriptor desc = {};
    desc.compute.module                = shader->compute_shader_module;
    desc.compute.entryPoint            = "main";

    char pipeline_label[64] = {};
    snprintf(pipeline_label, sizeof(pipeline_label), "ComputePass Pipeline %lld %s",
             (i64)shader->id, shader->name.c_str());
    desc.label = pipeline_label;

    R_ComputePassPipeline* pipeline
      = &r_compute_pass_pipelines[r_compute_pass_pipeline_count++];
    pipeline->shader_id         = shader->id;
    pipeline->gpu_pipeline      = wgpuDeviceCreateComputePipeline(gctx->device, &desc);
    pipeline->bind_group_layout = wgpuComputePipelineGetBindGroupLayout(
      pipeline->gpu_pipeline, 0); // caching because this wgpu call leaks memory

    return *pipeline;
}
