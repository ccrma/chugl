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

CK_DLL_SFUN(assloader_load_obj);
CK_DLL_SFUN(assloader_load_obj_flip_y);

#define FLOAT3_TO_GLM_VEC3(f3) glm::vec3(f3[0], f3[1], f3[2])

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

// GModel offsets
// idea: store these as static ints on SG_Sampler struct?
static t_CKUINT gmodel_offset_geo_array    = 0;
static t_CKUINT gmodel_offset_mat_array    = 0;
static t_CKUINT gmodel_offset_mesh_array   = 0;
static t_CKUINT gmodel_offset_bbox_min     = 0;
static t_CKUINT gmodel_offset_bbox_max     = 0;
static t_CKUINT gmodel_offset_vertex_count = 0;

CK_DLL_CTOR(gmodel_ctor);
CK_DLL_CTOR(gmodel_ctor_with_fp);
CK_DLL_CTOR(gmodel_ctor_with_fp_and_desc);

// ModelLoadDesc offsets
static t_CKUINT model_load_desc_offset_flip_texture_y = 0;
static t_CKUINT model_load_desc_offset_combine_geos   = 0;

CK_DLL_CTOR(model_load_desc_ctor);

void ulib_assloader_query(Chuck_DL_Query* QUERY)
{
    { // AssLoader --------------------------------------------------------------
        BEGIN_CLASS("AssLoader", "Object");
        DOC_CLASS("Utility for asset loading; supports .obj files");
        ADD_EX("basic/asset-loading.ck");

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

    { // ModelDesc --------------------------------------------------------------
        BEGIN_CLASS("ModelLoadDesc", "Object");
        DOC_CLASS("Options for model loading. Use in conjuntion with GModel");

        model_load_desc_offset_combine_geos = MVAR("int", "combine", false);
        DOC_VAR(
          "Default false. If true, will combine model geometries where possible under "
          "a single Mesh. This improves rendering efficiency at the cost of freedom to "
          "move the individual parts of a model independently");

        model_load_desc_offset_flip_texture_y = MVAR("int", "flipTextures", false);
        DOC_VAR(
          "Default false. If true, will flip all model textures along the y-axis. This "
          "is a quick fix to account for differences in Y-axis convention across "
          "different asset creation tools");

        CTOR(model_load_desc_ctor);

        END_CLASS();
    }

    { // GModel --------------------------------------------------------------
        BEGIN_CLASS(SG_CKNames[SG_COMPONENT_MODEL], SG_CKNames[SG_COMPONENT_TRANSFORM]);
        DOC_CLASS("Class for loading models. Currently supports: OBJ");
        ADD_EX("basic/gmodel.ck");

        gmodel_offset_geo_array = MVAR("Geometry[]", "geometries", true);
        DOC_VAR("List of all geometries in this model.");

        gmodel_offset_mat_array = MVAR("Material[]", "materials", true);
        DOC_VAR("List of all materials in this model.");

        gmodel_offset_mesh_array = MVAR("GMesh[]", "meshes", true);
        DOC_VAR("List of all meshes in this model.");

        gmodel_offset_bbox_max = MVAR("vec3", "max", true);
        DOC_VAR(
          "Get the max position of this model's bounding box. NOT scale-adjusted, "
          "multiply by GModel.scaWorld() to get scaled bounding box.");

        gmodel_offset_bbox_min = MVAR("vec3", "min", true);
        DOC_VAR(
          "Get the min position of this model's bounding box. NOT scale-adjusted, "
          "multiply by GModel.scaWorld() to get scaled bounding box.");

        gmodel_offset_vertex_count = MVAR("int", "vertexCount", true);
        DOC_VAR("The number of vertices in this model");

        CTOR(gmodel_ctor);
        DOC_FUNC(
          "Default constructor. Initializes an empty model which is functionally "
          "equivalent to a GGen. You probably want to use GModel(string path) instead");

        CTOR(gmodel_ctor_with_fp);
        ARG("string", "filepath");
        DOC_FUNC(
          "Initializes an model from the given asset file. Currently supports .obj "
          "files");

        CTOR(gmodel_ctor_with_fp_and_desc);
        ARG("string", "filepath");
        ARG("ModelLoadDesc", "options");
        DOC_FUNC(
          "Initializes an model from an asset file with the given model loading "
          "options. Currently supports .obj files");

        END_CLASS();
    }

    // init resources
    ulib_assloader_mat2geo_map
      = hashmap_new(sizeof(AssloaderMat2GeoItem), 0, 0, 0, AssloaderMat2GeoItem::hash,
                    AssloaderMat2GeoItem::compare, NULL, NULL);
}

// impl ============================================================================

struct SG_AssetLoadDesc {
    int combine_geos    = 0;
    int textures_flip_y = 0;

    static SG_AssetLoadDesc from(Chuck_Object* ckobj)
    {
        CK_DL_API API = g_chuglAPI;
        return {
            (int)OBJ_MEMBER_INT(ckobj, model_load_desc_offset_combine_geos),
            (int)OBJ_MEMBER_INT(ckobj, model_load_desc_offset_flip_texture_y),
        };
    }
};

static Arena ulib_assloader_tinyobj_filedata_pointers; // track allocations from openned
                                                       // files to free later
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
        log_warn("cannot open file '%s' while trying to load OBJ %s", filename,
                 obj_filename);
    }

    *data = result.data_owned;
    *len  = result.size;
    *ARENA_PUSH_TYPE(&ulib_assloader_tinyobj_filedata_pointers, char*)
      = result.data_owned;
}

struct ModelLoadObjResult {
    Arena geo_ckobj_list; // list of Chuck_Object*
    Arena mat_ckobj_list; // list of Chuck_Object*
    Arena gmesh_id_list;  // list of SG_ID
    glm::vec3 bmin;
    glm::vec3 bmax;
    size_t vertex_count;

    static void free(ModelLoadObjResult* result)
    {
        Arena::free(&result->geo_ckobj_list);
        Arena::free(&result->mat_ckobj_list);
        Arena::free(&result->gmesh_id_list);
    }
};

// ==optimize== if has_normals, do indexed draw
static ModelLoadObjResult ulib_assloader_tinyobj_load(const char* filepath,
                                                      Chuck_VM_Shred* SHRED,
                                                      SG_AssetLoadDesc desc)
{
    ModelLoadObjResult result = {};

    const char* extension = (File_getExtension(filepath));
    bool is_obj = (strcmp(extension, "obj") == 0 || strcmp(extension, "Obj") == 0
                   || strcmp(extension, "OBJ") == 0);
    if (!is_obj) {
        log_warn(
          "cannot load model from file '%s', only OBJ files with extension .obj are "
          "supported",
          filepath);
        return result;
    }

    tinyobj_attrib_t attrib       = {};
    tinyobj_shape_t* shapes       = NULL;
    size_t num_shapes             = 0;
    tinyobj_material_t* materials = NULL;
    size_t num_materials          = 0;
    defer({ // cleanup
        tinyobj_attrib_free(&attrib);
        tinyobj_shapes_free(shapes, num_shapes);
        tinyobj_materials_free(materials, num_materials);
    });

    int ret = tinyobj_parse_obj(
      &attrib, &shapes, &num_shapes, &materials, &num_materials, filepath,
      ulib_assloader_tinyobj_filereader, NULL, TINYOBJ_FLAG_TRIANGULATE);

    // free memory allocated by file reader
    for (int file_malloc_idx = 0; file_malloc_idx < ARENA_LENGTH(
                                    &ulib_assloader_tinyobj_filedata_pointers, void*);
         ++file_malloc_idx) {
        void* data_MALLOC = *ARENA_GET_TYPE(&ulib_assloader_tinyobj_filedata_pointers,
                                            void*, file_malloc_idx);
        free(data_MALLOC);
    }
    Arena::clear(&ulib_assloader_tinyobj_filedata_pointers);

    if (ret != TINYOBJ_SUCCESS) {
        switch (ret) {
            case TINYOBJ_ERROR_EMPTY: {
                log_warn("error loading OBJ file '%s': empty file", filepath);
            } break;
            case TINYOBJ_ERROR_INVALID_PARAMETER: {
                log_warn("error loading OBJ file '%s': invalid file", filepath);
            } break;
            case TINYOBJ_ERROR_FILE_OPERATION: {
                log_warn("error loading OBJ file '%s': invalid file operation",
                         filepath);
            } break;
            default: UNREACHABLE;
        }
        return result;
    }

    // triangulation warning
    bool triangulated = (attrib.num_faces == attrib.num_face_num_verts * 3);
    if (!triangulated) {
        log_warn(
          "OBJ '%s' is could not be triangulated. may render "
          "incorrectly.",
          filepath);
    }

    // calculate how many Geometry and Material we need to create
    SG_ID* sg_component_ids = NULL;
    SG_ID* material_ids     = NULL;
    SG_ID* geometry_ids     = NULL;
    SG_ID* mesh_ids         = NULL;
    sg_component_ids
      = ARENA_PUSH_ZERO_COUNT(&audio_frame_arena, SG_ID, (num_materials + 1) * 3);
    material_ids
      = sg_component_ids; // material_ids[num_materials] is the default material
    geometry_ids
      = material_ids + num_materials + 1; // geometry_ids[num_materials] is the geo that
                                          // corresponds to the default material
    mesh_ids = geometry_ids + num_materials
               + 1; // mesh_ids[num_materials] is mesh from default material

    { // create materials
        for (size_t i = 0; i < num_materials; i++) {
            tinyobj_material_t* obj_material = materials + i;

            // create geometry for this material
            if (desc.combine_geos) {
                SG_Geometry* geo = ulib_geometry_create(SG_GEOMETRY, SHRED);
                geometry_ids[i]  = geo->id;
                ulib_component_set_name(geo, obj_material->name);
            }

            // assumes material is phong (currently NOT supporting pbr extension)
            SG_Material* phong_material
              = ulib_material_create(SG_MATERIAL_PHONG, SHRED);

            *ARENA_PUSH_TYPE(&result.mat_ckobj_list, Chuck_Object*)
              = phong_material->ckobj;

            // add to material id array
            material_ids[i] = phong_material->id;

            // set name
            ulib_component_set_name(phong_material, obj_material->name);

            // set uniforms
            // adding ambient + diffuse color (PhongMaterial doesn't differentiate
            // between diffuse color and ambient color)
            // TODO handle envmapping and ior and illumination model
            PhongParams::diffuse(phong_material,
                                 FLOAT3_TO_GLM_VEC3(obj_material->diffuse)
                                   + FLOAT3_TO_GLM_VEC3(obj_material->ambient));
            PhongParams::specular(phong_material,
                                  FLOAT3_TO_GLM_VEC3(obj_material->specular));
            PhongParams::shininess(phong_material, obj_material->shininess);
            PhongParams::emission(phong_material,
                                  FLOAT3_TO_GLM_VEC3(obj_material->emission));

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

    glm::vec3 bmin(FLT_MAX);
    glm::vec3 bmax(-FLT_MAX);

    // clang-format off
    for (size_t shape_i = 0; shape_i < num_shapes; shape_i++) {
        if (!desc.combine_geos) { // zero out so we can build new meshes/geos per mesh
			ZERO_ARRAY_PTR(geometry_ids, num_materials + 1);
			ZERO_ARRAY_PTR(mesh_ids, num_materials + 1);
        }

        tinyobj_shape_t* obj_shape = shapes + shape_i;
        SG_Geometry* geo      = NULL;
        int prev_material_idx = num_materials + 1;

        // reset warning flags
        bool missing_uvs           = false;
        bool missing_normals       = false;
        bool uses_default_material = false; // true if any attribute materal_id < 0

        // assume each face is 3 vertices
        for (size_t face_idx = obj_shape->face_offset;
             face_idx < obj_shape->face_offset + obj_shape->length; ++face_idx) {

            { // get the correct material and geometry
                int material_idx = attrib.material_ids[face_idx];
                if (material_idx < 0) {
                  uses_default_material = true;
                  material_idx = num_materials;
                }
                if (material_idx != prev_material_idx) {
                    if (material_ids[material_idx] == 0) {
                        ASSERT(geometry_ids[material_idx] == 0); // geo should not be made yet
                        SG_Material* default_material = ulib_material_create(SG_MATERIAL_PHONG, SHRED);
                        material_ids[material_idx] = default_material->id;
                        *ARENA_PUSH_TYPE(&result.mat_ckobj_list, Chuck_Object*) = default_material->ckobj;
                        ulib_component_set_name(default_material, "OBJ Default Material");
                    }

                    if (geometry_ids[material_idx] == 0) {
                        ASSERT(!desc.combine_geos || material_idx == num_materials); // this should only happen when we are creating geos per shape
                        geo = ulib_geometry_create(SG_GEOMETRY, SHRED);
                        geometry_ids[material_idx] = geo->id;
                        ulib_component_set_name(geo, obj_shape->name);
                        *ARENA_PUSH_TYPE(&result.geo_ckobj_list, Chuck_Object*) = geo->ckobj;
                    }

                    ASSERT(material_ids[material_idx] && geometry_ids[material_idx]);
                    prev_material_idx = material_idx;
                    geo               = SG_GetGeometry(geometry_ids[material_idx]);
                }
                ASSERT(geo);
            }

            tinyobj_vertex_index_t indices[3]
              = { attrib.faces[3 * face_idx + 0], attrib.faces[3 * face_idx + 1],
                  attrib.faces[3 * face_idx + 2] };

            // get geometry buffers and allocate memory
            glm::vec3* positions = ARENA_PUSH_ZERO_COUNT(
              &geo->vertex_attribute_data[SG_GEOMETRY_POSITION_ATTRIBUTE_LOCATION],
              glm::vec3, 3);
            result.vertex_count += 3;

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

                bmin = glm::min(bmin, positions[face_vert_idx]);
                bmax = glm::max(bmax, positions[face_vert_idx]);
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
              "OBJ '%s' Mesh '%s' is missing one or more .mtl descriptions. rendering with a default PhongMaterial",
              filepath, obj_shape->name);
        }
        if (missing_normals) {
            log_warn(
              "OBJ '%s' Mesh '%s' is missing vertex normal data. calculating from vertex positions.",
              filepath, obj_shape->name);
        }
        if (missing_uvs) {
            log_warn(
              "OBJ '%s' Mesh '%s' is missing vertex UV data. defaulting to (0,0)",
              filepath, obj_shape->name);
        }

        bool last_shape = (shape_i == num_shapes - 1);
        if (!desc.combine_geos || last_shape) {
            // create meshes and add to gmodel
            for (int i = 0; i <= num_materials; i++) {
                // check for default mat/geo
                if (material_ids[i] == 0 || geometry_ids[i] == 0) continue;

                SG_Geometry* geo = SG_GetGeometry(geometry_ids[i]);
                SG_Material* mat = SG_GetMaterial(material_ids[i]);

                // update
                CQ_UpdateAllVertexAttributes(geo);

                // create mesh
                SG_Mesh* mesh
                = ulib_mesh_create(NULL, geo, mat, SHRED);
                ulib_component_set_name(mesh, geo->name);
                
                // add to GModel
                *ARENA_PUSH_TYPE(&result.gmesh_id_list, SG_ID) = mesh->id;
            }
        }
    } // foreach shape

    if (num_shapes > 0) {
        result.bmin = bmin;
        result.bmax = bmax;
    }

    return result;
}
// clang-format on

static Chuck_Object* ulib_assloader_load_obj(bool flip_y, const char* filepath,
                                             Chuck_VM_Shred* SHRED)
{
    SG_Transform* obj_root    = NULL;
    SG_AssetLoadDesc desc     = {};
    desc.textures_flip_y      = flip_y;
    ModelLoadObjResult result = ulib_assloader_tinyobj_load(filepath, SHRED, desc);
    defer(ModelLoadObjResult::free(
      &result)); // TODO don't need to alloc, just keep return pointers to static arrays

    int num_meshes = ARENA_LENGTH(&result.gmesh_id_list, SG_ID);

    if (num_meshes == 0) {
        obj_root = ulib_ggen_create(NULL, SHRED);
    } else if (num_meshes == 1) {
        obj_root = SG_GetMesh(*ARENA_GET_TYPE(&result.gmesh_id_list, SG_ID, 0));
    } else {
        obj_root = ulib_ggen_create(NULL, SHRED);

        for (int i = 0; i < num_meshes; ++i) {
            SG_Mesh* mesh
              = SG_GetMesh(*ARENA_GET_TYPE(&result.gmesh_id_list, SG_ID, i));
            // child meshes
            CQ_PushCommand_AddChild(obj_root, mesh);
        }
    }

    // name
    ASSERT(obj_root);
    ulib_component_set_name(obj_root, File_basename(filepath));

    return obj_root->ckobj;
}

CK_DLL_SFUN(assloader_load_obj)
{
    RETURN->v_object
      = ulib_assloader_load_obj(false, API->object->str(GET_NEXT_STRING(ARGS)), SHRED);
}

CK_DLL_SFUN(assloader_load_obj_flip_y)
{
    const char* filepath = API->object->str(GET_NEXT_STRING(ARGS));
    bool flip_y          = (bool)GET_NEXT_INT(ARGS);
    RETURN->v_object     = ulib_assloader_load_obj(flip_y, filepath, SHRED);
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
    OBJ_MEMBER_VEC3(ckobj, gmodel_offset_bbox_max)    = { 0, 0, 0 };
    OBJ_MEMBER_VEC3(ckobj, gmodel_offset_bbox_min)    = { 0, 0, 0 };
    OBJ_MEMBER_INT(ckobj, gmodel_offset_vertex_count) = 0;

    return model;
}

static void ulib_gmodel_load(SG_Transform* model, const char* filepath,
                             ModelLoadObjResult result)
{
    CK_DL_API API = g_chuglAPI;

    int mesh_count = ARENA_LENGTH(&result.gmesh_id_list, SG_ID);
    int mat_count  = ARENA_LENGTH(&result.mat_ckobj_list, Chuck_Object*);
    int geo_count  = ARENA_LENGTH(&result.geo_ckobj_list, Chuck_Object*);

    Chuck_ArrayInt* ck_geo_array
      = OBJ_MEMBER_INT_ARRAY(model->ckobj, gmodel_offset_geo_array);
    Chuck_ArrayInt* ck_mat_array
      = OBJ_MEMBER_INT_ARRAY(model->ckobj, gmodel_offset_mat_array);
    Chuck_ArrayInt* ck_mesh_array
      = OBJ_MEMBER_INT_ARRAY(model->ckobj, gmodel_offset_mesh_array);

    // append components
    for (int i = 0; i < mesh_count; i++) {
        SG_Mesh* mesh = SG_GetMesh(*ARENA_GET_TYPE(&result.gmesh_id_list, SG_ID, i));
        // child meshes
        CQ_PushCommand_AddChild(model, mesh);
        API->object->array_int_push_back(ck_mesh_array, (t_CKINT)mesh->ckobj);
    }

    for (int i = 0; i < mat_count; i++)
        API->object->array_int_push_back(
          ck_mat_array,
          (t_CKINT)*ARENA_GET_TYPE(&result.mat_ckobj_list, Chuck_Object*, i));

    for (int i = 0; i < geo_count; i++)
        API->object->array_int_push_back(
          ck_geo_array,
          (t_CKINT)*ARENA_GET_TYPE(&result.geo_ckobj_list, Chuck_Object*, i));

    // name
    ulib_component_set_name(model, File_basename(filepath));

    // set bounding box
    OBJ_MEMBER_VEC3(model->ckobj, gmodel_offset_bbox_min)
      = { result.bmin.x, result.bmin.y, result.bmin.z };
    OBJ_MEMBER_VEC3(model->ckobj, gmodel_offset_bbox_max)
      = { result.bmax.x, result.bmax.y, result.bmax.z };

    // set vertex count
    OBJ_MEMBER_INT(model->ckobj, gmodel_offset_vertex_count) = result.vertex_count;

    // cleanup
    ModelLoadObjResult::free(&result);
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
    ulib_gmodel_load(model, filepath,
                     ulib_assloader_tinyobj_load(filepath, SHRED, desc));
}

CK_DLL_CTOR(gmodel_ctor_with_fp_and_desc)
{
    SG_Transform* model = ulib_gmodel_create(SELF, SHRED);

    Chuck_String* ck_str = GET_NEXT_STRING(ARGS);
    if (ck_str == NULL) return;
    const char* filepath = API->object->str(ck_str);
    if (filepath == NULL) return;

    SG_AssetLoadDesc desc = SG_AssetLoadDesc::from(GET_NEXT_OBJECT(ARGS));

    ulib_gmodel_load(model, filepath,
                     ulib_assloader_tinyobj_load(filepath, SHRED, desc));
}

// =============================
// ModelLoadDesc
// =============================

CK_DLL_CTOR(model_load_desc_ctor)
{
    OBJ_MEMBER_INT(SELF, model_load_desc_offset_combine_geos)   = 0;
    OBJ_MEMBER_INT(SELF, model_load_desc_offset_flip_texture_y) = 0;
}
