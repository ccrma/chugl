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
#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "core/file.h"
#include "core/hashmap.h"
#include "core/log.h"

#include "geometry.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj/tinyobj_loader_c.h>

#include <rapidobj/rapidobj.hpp>

CK_DLL_SFUN(assloader_load_obj);
CK_DLL_SFUN(assloader_load_obj_flip_y);

#define RAPID_FLOAT3_TO_GLM_VEC3(f3) glm::vec3(f3[0], f3[1], f3[2])

static void logRapidobjError(const rapidobj::Error& error, const char* filepath)
{
    log_warn("Could not load OBJ model \"%s\": %s", filepath,
             error.code.message().c_str());
    if (!error.line.empty()) {
        log_warn("On line %d: \"%s\"", error.line_num, error.line.c_str());
    }
}

// used to track geometries per material id during OBJ loading
// currently unused
static hashmap* ulib_assloader_mat2geo_map = NULL;
struct AssloaderMat2GeoItem {
    SG_ID mat_id; // key
    SG_ID geo_id; // value

    static int compare(const void* a, const void* b, void* udata)
    {
        UNUSED_VAR(udata);
        AssloaderMat2GeoItem* item_a = (AssloaderMat2GeoItem*)a;
        AssloaderMat2GeoItem* item_b = (AssloaderMat2GeoItem*)b;
        return item_a->mat_id - item_b->mat_id;
    }

    static uint64_t hash(const void* item, uint64_t seed0, uint64_t seed1)
    {
        AssloaderMat2GeoItem* key = (AssloaderMat2GeoItem*)item;
        return hashmap_xxhash3(&key->mat_id, sizeof(key->mat_id), seed0, seed1);
    }

    static SG_Geometry* get(SG_ID material_id)
    {
        AssloaderMat2GeoItem* item = (AssloaderMat2GeoItem*)hashmap_get(
          ulib_assloader_mat2geo_map, &material_id);

        return item ? SG_GetGeometry(item->geo_id) : NULL;
    }

    // creates a new geometry and sets
    static SG_Geometry* set(SG_ID material_id, SG_Geometry* geo)
    {
        AssloaderMat2GeoItem item = {};
        item.mat_id               = material_id;
        item.geo_id               = geo->id;

        const void* exists = hashmap_set(ulib_assloader_mat2geo_map, &item);
        ASSERT(!exists);
        UNUSED_VAR(exists);

        return geo;
    }
};

struct SG_Model {
    /*
    extends GGen
    Geo[]
    Mat[]
    GMesh[]
    vec3 bmin, bmax
    */
};

// SG_Sampler offsets
// idea: store these as static ints on SG_Sampler struct?
static t_CKUINT gmodel_offset_geo_array  = 0;
static t_CKUINT gmodel_offset_mat_array  = 0;
static t_CKUINT gmodel_offset_mesh_array = 0;

CK_DLL_CTOR(gmodel_ctor);
CK_DLL_CTOR(gmodel_ctor_with_fp);

void ulib_assloader_query(Chuck_DL_Query* QUERY)
{
    { // AssLoader --------------------------------------------------------------
        BEGIN_CLASS("AssLoader", "Object");
        DOC_CLASS("Utility for asset loading; supports .obj files");
        ADD_EX("basic/asset_loading.ck");

        SFUN(assloader_load_obj, "GGen", "loadObj");
        ARG("string", "filepath");
        DOC_FUNC("Load an .obj file from the given filepath");

        SFUN(assloader_load_obj_flip_y, "GGen", "loadObj");
        ARG("string", "filepath");
        ARG("int", "flip_y");
        DOC_FUNC(
          "Load an .obj file from the given filepath. If flip_y is true, the y-axis is "
          "flipped (default is false)");

        END_CLASS();
    }

    { // GModel --------------------------------------------------------------
        BEGIN_CLASS(SG_CKNames[SG_COMPONENT_MODEL], SG_CKNames[SG_COMPONENT_TRANSFORM]);
        DOC_CLASS("Class for loading models. Currently supports: OBJ");
        ADD_EX("basic/asset_loading.ck");

        gmodel_offset_geo_array = MVAR("Geometry[]", "geometries", true);
        DOC_VAR("TODO");

        gmodel_offset_mat_array = MVAR("Material[]", "materials", true);
        DOC_VAR("TODO");

        gmodel_offset_mesh_array = MVAR("GMesh[]", "meshes", true);
        DOC_VAR("TODO");

        CTOR(gmodel_ctor);
        DOC_FUNC(
          "Default constructor. Initializes an empty model which is functionally "
          "equivalent to a GGen. You probably want to use GModel(string path) instead");

        CTOR(gmodel_ctor_with_fp);
        ARG("string", "filepath");
        DOC_FUNC(
          "Initializes an model from the given asset file. Currently supports .obj "
          "files");

        END_CLASS();
    }

    // init resources
    ulib_assloader_mat2geo_map
      = hashmap_new(sizeof(AssloaderMat2GeoItem), 0, 0, 0, AssloaderMat2GeoItem::hash,
                    AssloaderMat2GeoItem::compare, NULL, NULL);
}

// impl ============================================================================

struct SG_AssetLoadDesc {
    int textures_flip_y = 0;
    int combine_geos    = 1; // TODO implement option where we don't combine
};

static Arena
  ulib_assloader_filebuffer_arena; // TODO free the file memory from OBJ loader!!!

static void ulib_assloader_tinyobj_filereader(void* ctx, const char* filename,
                                              const int is_mtl,
                                              const char* obj_filename, char** data,
                                              size_t* len)
{
    // NOTE: If you allocate the buffer with malloc(),
    // You can define your own memory management struct and pass it through `ctx`
    // to store the pointer and free memories at clean up stage(when you quit an
    // app)
    // This example uses mmap(), so no free() required.
    UNUSED_VAR(ctx);

    (*data) = NULL;
    (*len)  = 0;

    if (!filename) {
        if (is_mtl) {
            log_warn("AssLoader loading an OBJ \"%s\" with an empty .mtl filepath",
                     obj_filename);
        } else {
            log_warn("AssLoader called with an empty filepath");
        }
        return;
    }

    FileReadResult result = File_read(filename, true);
    if (result.data_owned == NULL) {
        log_warn("Cannot open file '%s' while trying to load OBJ %s", filename,
                 obj_filename);
    }

    *data = result.data_owned;
    *len  = result.size;
}

static void ulib_assloader_tinyobj_load(const char* filepath, Chuck_VM_Shred* SHRED,
                                        SG_AssetLoadDesc desc, SG_Transform* gmodel)
{
    tinyobj_attrib_t attrib;
    tinyobj_shape_t* shapes = NULL;
    size_t num_shapes;
    tinyobj_material_t* materials = NULL;
    size_t num_materials;
    int ret = tinyobj_parse_obj(
      &attrib, &shapes, &num_shapes, &materials, &num_materials, filepath,
      ulib_assloader_tinyobj_filereader, NULL, TINYOBJ_FLAG_TRIANGULATE);

    if (ret != TINYOBJ_SUCCESS) {
        switch (ret) {
            case TINYOBJ_ERROR_EMPTY: {
                log_warn("AssLoader error loading OBJ file '%s': empty file", filepath);
            } break;
            case TINYOBJ_ERROR_INVALID_PARAMETER: {
                log_warn("AssLoader error loading OBJ file '%s': invalid file",
                         filepath);
            } break;
            case TINYOBJ_ERROR_FILE_OPERATION: {
                log_warn(
                  "AssLoader error loading OBJ file '%s': invalid file operation",
                  filepath);
            } break;
            default: UNREACHABLE;
        }
        return;
    }

    // triangulation warning
    bool triangulated = (attrib.num_faces == attrib.num_face_num_verts * 3);
    if (!triangulated) {
        log_warn(
          "Warning, OBJ '%s' is could not be triangulated. May render "
          "incorrectly.",
          filepath);
    }

    // calculate how many Geometry and Material we need to create
    SG_ID* sg_component_ids
      = ARENA_PUSH_ZERO_COUNT(&audio_frame_arena, SG_ID, (num_materials + 1) * 3);
    SG_ID* geometry_ids
      = sg_component_ids
        + 1; // geometry_ids[-1] is the geo that corresponds to the default material
    SG_ID* material_ids
      = geometry_ids + num_materials + 1; // material_ids[-1] is the default material

    SG_ID* mesh_ids
      = material_ids + num_materials + 1; // mesh_ids[-1] is mesh from default material

    { // create materials
        for (size_t i = 0; i < num_materials; i++) {
            tinyobj_material_t* obj_material = materials + i;

            // create geometry for this material
            SG_Geometry* geo = ulib_geometry_create(SG_GEOMETRY, SHRED);
            geometry_ids[i]  = geo->id;
            ulib_component_set_name(geo, obj_material->name);

            // assumes material is phong (currently NOT supporting pbr extension)
            SG_Material* phong_material
              = ulib_material_create(SG_MATERIAL_PHONG, SHRED);

            // add to material id array
            material_ids[i] = phong_material->id;

            // set name
            ulib_component_set_name(phong_material, obj_material->name);

            // set uniforms
            // adding ambient + diffuse color (PhongMaterial doesn't differentiate
            // between diffuse color and ambient color)
            // TODO handle envmapping and ior and illumination model
            PhongParams::diffuse(phong_material,
                                 RAPID_FLOAT3_TO_GLM_VEC3(obj_material->diffuse)
                                   + RAPID_FLOAT3_TO_GLM_VEC3(obj_material->ambient));
            PhongParams::specular(phong_material,
                                  RAPID_FLOAT3_TO_GLM_VEC3(obj_material->specular));
            PhongParams::shininess(phong_material, obj_material->shininess);
            PhongParams::emission(phong_material,
                                  RAPID_FLOAT3_TO_GLM_VEC3(obj_material->emission));

            SG_TextureLoadDesc load_desc = {};
            load_desc.flip_y             = desc.textures_flip_y;
            load_desc.gen_mips           = true;
            std::string directory        = File_dirname(filepath);

            if (obj_material->diffuse_texname) {
                std::string tex_path = directory + obj_material->diffuse_texname;
                SG_Texture* tex
                  = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
                PhongParams::albedoTex(phong_material, tex);
                ulib_component_set_name(tex, tex_path.c_str());
            }

            if (obj_material->specular_texname) {
                std::string tex_path = directory + obj_material->specular_texname;
                SG_Texture* tex
                  = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
                PhongParams::specularTex(phong_material, tex);
                ulib_component_set_name(tex, tex_path.c_str());
            }

            if (obj_material->bump_texname) {
                std::string tex_path = directory + obj_material->bump_texname;
                SG_Texture* tex
                  = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
                PhongParams::normalTex(phong_material, tex);
                ulib_component_set_name(tex, tex_path.c_str());
            }

            if (obj_material->ambient_texname) {
                std::string tex_path = directory + obj_material->ambient_texname;
                SG_Texture* tex
                  = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
                PhongParams::aoTex(phong_material, tex);
                ulib_component_set_name(tex, tex_path.c_str());
            }

            // emissive textures currently unspported by tinyobj
            // TODO add and test with a map_ke OBJ
            // if (obj_material->emissive_texname) {
            //     std::string tex_path = directory + obj_material->emissive_texname;
            //     SG_Texture* tex
            //       = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            //     PhongParams::emissiveTex(phong_material, tex);
            //     ulib_component_set_name(tex, tex_path.c_str());
            // }
        }
    }

    float bmin[3];
    float bmax[3];
    bmin[0] = bmin[1] = bmin[2] = FLT_MAX;
    bmax[0] = bmax[1] = bmax[2] = -FLT_MAX;
    // TODO set bounding box

    // clang-format off
    for (size_t shape_i = 0; shape_i < num_shapes; shape_i++) {
        tinyobj_shape_t* obj_shape = shapes + shape_i;
        SG_Geometry* geo      = NULL;
        int prev_material_idx = -num_materials;

        // reset warning flags
        bool missing_uvs           = false;
        bool missing_normals       = false;
        bool uses_default_material = false; // true if any attribute materal_id < 0

        // assume each face is 3 vertices
        for (size_t face_idx = obj_shape->face_offset;
             face_idx < obj_shape->face_offset + obj_shape->length; ++face_idx) {

            { // get the correct  material and geometry
                int material_idx = attrib.material_ids[face_idx];
                if (material_idx < 0) uses_default_material = true;
                if (material_idx != prev_material_idx) {
                    bool need_create_default_material
                      = (material_idx < 0 && material_ids[-1] == 0);
                    if (need_create_default_material) {
                        material_idx = -1;
                        ASSERT(geometry_ids[-1] == 0); // geo should not be made yet
                        material_ids[-1]
                          = ulib_material_create(SG_MATERIAL_PHONG, SHRED)->id;
                        geometry_ids[-1] = ulib_geometry_create(SG_GEOMETRY, SHRED)->id;
                    }
                    ASSERT(material_ids[material_idx] && geometry_ids[material_idx]);
                    prev_material_idx = material_idx;
                    geo               = SG_GetGeometry(geometry_ids[material_idx]);
                }
            }
            
            // TODO only set geo name if we are *not* combining geos across shapes
//            ulib_component_set_name(geo, obj_shape->name);

            tinyobj_vertex_index_t indices[3]
              = { attrib.faces[3 * face_idx + 0], attrib.faces[3 * face_idx + 1],
                  attrib.faces[3 * face_idx + 2] };

            // get geometry buffers and allocate memory
            glm::vec3* positions = ARENA_PUSH_ZERO_COUNT(
              &geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION],
              glm::vec3, 3);

            glm::vec3* normals = ARENA_PUSH_ZERO_COUNT(
              &geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION], glm::vec3,
              3);

            glm::vec2* texcoords = ARENA_PUSH_ZERO_COUNT(
              &geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION], glm::vec2,
              3);
            for (int face_vert_idx = 0; face_vert_idx < 3; face_vert_idx++) {
                positions[face_vert_idx].x = attrib.vertices[3 * indices[face_vert_idx].v_idx + 0];
                positions[face_vert_idx].y = attrib.vertices[3 * indices[face_vert_idx].v_idx + 1];
                positions[face_vert_idx].z = attrib.vertices[3 * indices[face_vert_idx].v_idx + 2];
            }

            bool has_normal = (attrib.num_normals > 0 && indices[0].vn_idx >= 0 && indices[1].vn_idx >= 0
                               && indices[2].vn_idx >= 0);
            if (has_normal) {
                for (int face_vert_idx = 0; face_vert_idx < 3; face_vert_idx++) {
                    normals[face_vert_idx].x = attrib.normals[3 * indices[face_vert_idx].vn_idx + 0];
                    normals[face_vert_idx].y = attrib.normals[3 * indices[face_vert_idx].vn_idx + 1];
                    normals[face_vert_idx].z = attrib.normals[3 * indices[face_vert_idx].vn_idx + 2];
                }
            } else {
                missing_normals = true;
                normals[0] = glm::normalize(glm::cross(positions[2] - positions[0], positions[1] - positions[0])); 
                normals[1] = normals[0];
                normals[2] = normals[0];
            }

            for (int face_vert_idx = 0; face_vert_idx < 3; face_vert_idx++) {
                if (indices[face_vert_idx].vt_idx >= 0) {
                    texcoords[face_vert_idx].x = attrib.texcoords[2 * indices[face_vert_idx].vt_idx + 0];
                    texcoords[face_vert_idx].y = attrib.texcoords[2 * indices[face_vert_idx].vt_idx + 1];
                } else {
                    missing_uvs = true;
                }
            }
        } // for face

        if (uses_default_material) {
            log_warn(
              "Warning, OBJ '%s' Mesh '%s' is missing one or more .mtl descriptions. Rendering with a default PhongMaterial",
              filepath, obj_shape->name);
        }
        if (missing_normals) {
            log_warn(
              "Warning, OBJ '%s' Mesh '%s' is missing vertex normal data. Calculating from vertex positions.",
              filepath, obj_shape->name);
        }
        if (missing_uvs) {
            log_warn(
              "Warning, OBJ '%s' Mesh '%s' is missing vertex UV data. Defaulting to (0,0)",
              filepath, obj_shape->name);
        }
    } // foreach shape

    // create meshes
    // start from -1 to include default material/geo
    CK_DL_API API = g_chuglAPI;
    Chuck_ArrayInt* ck_geo_array = OBJ_MEMBER_INT_ARRAY(gmodel->ckobj, gmodel_offset_geo_array);
    Chuck_ArrayInt* ck_mat_array = OBJ_MEMBER_INT_ARRAY(gmodel->ckobj, gmodel_offset_mat_array);
    Chuck_ArrayInt* ck_mesh_array = OBJ_MEMBER_INT_ARRAY(gmodel->ckobj, gmodel_offset_mesh_array);
    for (int i = -1; i < (i32)num_materials; i++) {
        // check for default mat/geo
        if (material_ids[i] == 0) {
            ASSERT(geometry_ids[i] == 0)
            continue;
        }

        SG_Geometry* geo = SG_GetGeometry(geometry_ids[i]);
        SG_Material* mat = SG_GetMaterial(material_ids[i]);

        // update
        CQ_UpdateAllVertexAttributes(geo);

        // create mesh
        SG_Mesh* mesh
          = ulib_mesh_create(NULL, geo, mat, SHRED);
        mesh_ids[i] = mesh->id;

        // assign to parent
        if (gmodel) {
            CQ_PushCommand_AddChild(gmodel, mesh);
        } 
        
        // add to GModel
        g_chuglAPI->object->array_int_push_back(ck_geo_array, (t_CKINT) geo->ckobj);
        g_chuglAPI->object->array_int_push_back(ck_mat_array, (t_CKINT) mat->ckobj);
        g_chuglAPI->object->array_int_push_back(ck_mesh_array, (t_CKINT) mesh->ckobj);
    }
    // set model name
    ulib_component_set_name(gmodel, filepath);
}
// clang-format on

static SG_Transform* ulib_assloader_load_obj(const char* filepath,
                                             Chuck_VM_Shred* SHRED,
                                             SG_AssetLoadDesc desc)
{
    // for simplicity, does not support lines or points
    // renders all models with Phong lighting (for pbr use gltf loader instead)
    rapidobj::Result result = rapidobj::ParseFile(filepath);

    if (result.error) {
        logRapidobjError(result.error, filepath);
        return ulib_ggen_create(NULL, SHRED); // on error return empty ggen
    }

    rapidobj::Triangulate(result);

    if (result.error) {
        logRapidobjError(result.error, filepath);
        return ulib_ggen_create(NULL, SHRED); // on error return empty ggen
    }

    // first create all unique materials
    size_t num_materials = result.materials.size();
    SG_ID* material_ids
      = ARENA_PUSH_ZERO_COUNT(&audio_frame_arena, SG_ID, (num_materials + 1) * 2);
    material_ids += 1; // so that material_ids[-1] is the default material

    // then make a geometry for each material (for optimization purposes, we group
    // all shapes vertices with the same material idx under the same material) this
    // reduces a model like backpack.obj from 79 geometries and 1 material --> 1 geo
    // and 1 material (1 draw call!)
    SG_ID* geo_ids = material_ids + num_materials + 1;

    // we need #meshes = #materials. if >1 mesh, create a parent ggen to contain all
    SG_Transform* obj_shape_root = NULL;
    // if multiple shapes, return under parent root
    if (num_materials > 1) {
        obj_shape_root = ulib_ggen_create(NULL, SHRED);
    }

    for (size_t i = 0; i < num_materials; i++) {
        // create geometry for this material
        SG_Geometry* geo = ulib_geometry_create(SG_GEOMETRY, SHRED);
        geo_ids[i]       = geo->id;

        const rapidobj::Material& obj_material = result.materials[i];

        // assumes material is phong (currently NOT supporting pbr extension)
        SG_Material* phong_material = ulib_material_create(SG_MATERIAL_PHONG, SHRED);

        // add to material id array
        material_ids[i] = phong_material->id;

        // set name
        ulib_component_set_name(phong_material, obj_material.name.c_str());

        // set uniforms
        // adding ambient + diffuse color (PhongMaterial doesn't differentiate
        // between diffuse color and ambient color)
        PhongParams::diffuse(phong_material,
                             RAPID_FLOAT3_TO_GLM_VEC3(obj_material.diffuse)
                               + RAPID_FLOAT3_TO_GLM_VEC3(obj_material.ambient));
        PhongParams::specular(phong_material,
                              RAPID_FLOAT3_TO_GLM_VEC3(obj_material.specular));
        PhongParams::shininess(phong_material, obj_material.shininess);
        PhongParams::emission(phong_material,
                              RAPID_FLOAT3_TO_GLM_VEC3(obj_material.emission));

        // TODO set textures
        SG_TextureLoadDesc load_desc = {};
        load_desc.flip_y             = desc.textures_flip_y;
        load_desc.gen_mips           = true;
        std::string directory        = File_dirname(filepath);

        if (obj_material.diffuse_texname.size()) {
            std::string tex_path = directory + obj_material.diffuse_texname;
            SG_Texture* tex = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            PhongParams::albedoTex(phong_material, tex);
        }

        if (obj_material.specular_texname.size()) {
            std::string tex_path = directory + obj_material.specular_texname;
            SG_Texture* tex = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            PhongParams::specularTex(phong_material, tex);
        }

        if (obj_material.bump_texname.size()) {
            std::string tex_path = directory + obj_material.bump_texname;
            SG_Texture* tex = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            PhongParams::normalTex(phong_material, tex);
        }

        if (obj_material.ambient_texname.size()) {
            std::string tex_path = directory + obj_material.ambient_texname;
            SG_Texture* tex = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            PhongParams::aoTex(phong_material, tex);
        }

        if (obj_material.emissive_texname.size()) {
            std::string tex_path = directory + obj_material.ambient_texname;
            SG_Texture* tex = ulib_texture_load(tex_path.c_str(), &load_desc, SHRED);
            PhongParams::emissiveTex(phong_material, tex);
        }
    }

    // TODO set names
    for (const rapidobj::Shape& shape : result.shapes) {
        bool missing_normals  = false;
        bool missing_uvs      = false;
        int prev_material_idx = -100;
        SG_Geometry* face_geo = NULL;
        size_t num_vertices   = shape.mesh.indices.size();

        // reset the mat --> geo map for each shape/mesh
        ASSERT(hashmap_count(ulib_assloader_mat2geo_map) == 0);
        defer(hashmap_clear(ulib_assloader_mat2geo_map, false));

        ASSERT(num_vertices % 3 == 0);
        ASSERT(shape.mesh.indices.size() / 3 == shape.mesh.material_ids.size());

        if (!shape.lines.indices.empty()) {
            log_warn("Obj Shape \"%s\" has polylines; unsupported; skipping",
                     shape.name.c_str());
        }
        if (!shape.points.indices.empty()) {
            log_warn("Obj Shape \"%s\" has points; unsupported; skipping",
                     shape.name.c_str());
        }

        for (size_t i = 0; i < num_vertices; i++) {

            // every face update material and geometry
            if (i % 3 == 0) {
                size_t face_idx  = i / 3; // 3 vertices per face
                i32 material_idx = shape.mesh.material_ids[face_idx];
                if (material_idx != prev_material_idx) {
                    prev_material_idx = material_idx;
                    if (material_ids[material_idx] == 0) {
                        ASSERT(material_idx == -1)
                        ASSERT(geo_ids[material_idx] == 0);
                        // create default material
                        material_ids[material_idx]
                          = ulib_material_create(SG_MATERIAL_PHONG, SHRED)->id;
                        geo_ids[material_idx]
                          = ulib_geometry_create(SG_GEOMETRY, SHRED)->id;
                    }

                    face_geo = SG_GetGeometry(geo_ids[material_idx]);
                }
            }

            rapidobj::Index index = shape.mesh.indices[i];

            // get geometry buffers and allocate memory
            glm::vec3* positions = ARENA_PUSH_ZERO_TYPE(
              &face_geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION],
              glm::vec3);

            glm::vec3* normals = ARENA_PUSH_ZERO_TYPE(
              &face_geo->vertex_attribute_data[SG_GEOMETRY_NORMAL_ATTRIBUTE_LOCATION],
              glm::vec3);

            glm::vec2* texcoords = ARENA_PUSH_ZERO_TYPE(
              &face_geo->vertex_attribute_data[SG_GEOMETRY_UV_ATTRIBUTE_LOCATION],
              glm::vec2);

            float* pos   = &result.attributes.positions[index.position_index * 3];
            positions->x = pos[0];
            positions->y = pos[1];
            positions->z = pos[2];

            // copy normals
            if (index.normal_index < 0) {
                missing_normals = true;
            } else {
                float* norm = &result.attributes.normals[index.normal_index * 3];
                normals->x  = norm[0];
                normals->y  = norm[1];
                normals->z  = norm[2];
            }

            // copy uvs
            if (index.texcoord_index < 0) {
                missing_uvs = true;
            } else {
                float* uvs   = &result.attributes.texcoords[index.texcoord_index * 2];
                texcoords->x = uvs[0];
                texcoords->y = uvs[1];
            }
        }

        if (missing_normals) {
            log_error(
              "Warning, OBJ mesh %s is missing normal data. Defaulting to (0,0,0)",
              shape.name.c_str());
        }
        if (missing_uvs) {
            log_error("Warning, OBJ mesh %s is missing uv data. Defaulting to (0,0)",
                      shape.name.c_str());
        }

        // TODO eventually conslidate with `ulib_geometry_build()`
    }

    // start from -1 to include default material/geo
    for (int i = -1; i < (i32)num_materials; i++) {
        // check for default mat/geo
        if (!material_ids[i]) {
            ASSERT(!geo_ids[i])
            continue;
        }

        SG_Geometry* geo = SG_GetGeometry(geo_ids[i]);

        // update
        CQ_UpdateAllVertexAttributes(geo);

        // create mesh
        SG_Mesh* mesh
          = ulib_mesh_create(NULL, geo, SG_GetMaterial(material_ids[i]), SHRED);

        // assign to parent
        if (obj_shape_root) {
            CQ_PushCommand_AddChild(obj_shape_root, mesh);
        } else {
            obj_shape_root = mesh;
        }
    }

    return obj_shape_root;
}

CK_DLL_SFUN(assloader_load_obj)
{
    RETURN->v_object = NULL;

    SG_AssetLoadDesc desc = {};
    SG_Transform* obj_root
      = ulib_assloader_load_obj(API->object->str(GET_NEXT_STRING(ARGS)), SHRED, desc);
    RETURN->v_object = obj_root ? obj_root->ckobj : NULL;
}

CK_DLL_SFUN(assloader_load_obj_flip_y)
{
    RETURN->v_object     = NULL;
    const char* filepath = API->object->str(GET_NEXT_STRING(ARGS));
    bool flip_y          = (bool)GET_NEXT_INT(ARGS);

    SG_AssetLoadDesc desc  = {};
    desc.textures_flip_y   = flip_y;
    SG_Transform* obj_root = ulib_assloader_load_obj(filepath, SHRED, desc);
    RETURN->v_object       = obj_root ? obj_root->ckobj : NULL;
}

// =============================
// GModel
// =============================

static SG_Transform* ulib_gmodel_create(Chuck_Object* ckobj, Chuck_VM_Shred* shred)
{
    CK_DL_API API = g_chuglAPI;

    SG_Transform* model = ulib_ggen_create(ckobj, shred);
    OBJ_MEMBER_OBJECT(ckobj, gmodel_offset_geo_array)
      = chugin_createCkObj("Geometry[]", true, shred);
    OBJ_MEMBER_OBJECT(ckobj, gmodel_offset_mat_array)
      = chugin_createCkObj("Material[]", true, shred);
    OBJ_MEMBER_OBJECT(ckobj, gmodel_offset_mesh_array)
      = chugin_createCkObj("GMesh[]", true, shred);

    return model;
}

CK_DLL_CTOR(gmodel_ctor)
{
    ulib_gmodel_create(SELF, SHRED);
}

CK_DLL_CTOR(gmodel_ctor_with_fp)
{
    SG_Transform* model = ulib_gmodel_create(SELF, SHRED);

    Chuck_String* ck_str = GET_NEXT_STRING(ARGS);
    if (ck_str == NULL) return;
    const char* filepath = API->object->str(ck_str);
    if (filepath == NULL) return;

    SG_AssetLoadDesc desc = {};
    ulib_assloader_tinyobj_load(filepath, SHRED, desc, model);
}
