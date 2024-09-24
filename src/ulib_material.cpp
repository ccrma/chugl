#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_SHADER(ckobj) SG_GetShader(OBJ_MEMBER_UINT(ckobj, component_offset_id))
#define GET_MATERIAL(ckobj) SG_GetMaterial(OBJ_MEMBER_UINT(ckobj, component_offset_id))

void chugl_initDefaultMaterials();

CK_DLL_CTOR(shader_desc_ctor);
static t_CKUINT shader_desc_vertex_string_offset     = 0;
static t_CKUINT shader_desc_fragment_string_offset   = 0;
static t_CKUINT shader_desc_vertex_filepath_offset   = 0;
static t_CKUINT shader_desc_fragment_filepath_offset = 0;
static t_CKUINT shader_desc_vertex_layout_offset     = 0;
static t_CKUINT shader_desc_compute_string_offset    = 0;
static t_CKUINT shader_desc_compute_filepath_offset  = 0;
static t_CKUINT shader_desc_is_lit                   = 0;

CK_DLL_CTOR(shader_ctor_default);
CK_DLL_CTOR(shader_ctor);
CK_DLL_MFUN(shader_get_vertex_string);
CK_DLL_MFUN(shader_get_fragment_string);
CK_DLL_MFUN(shader_get_vertex_filepath);
CK_DLL_MFUN(shader_get_fragment_filepath);
CK_DLL_MFUN(shader_get_vertex_layout);
CK_DLL_MFUN(shader_get_lit);

CK_DLL_CTOR(material_ctor);

// material pso
CK_DLL_MFUN(material_get_shader);
CK_DLL_MFUN(material_set_shader);
CK_DLL_MFUN(material_get_cullmode);
CK_DLL_MFUN(material_set_cullmode);
CK_DLL_MFUN(material_set_topology);
CK_DLL_MFUN(material_get_topology);

// material uniforms
CK_DLL_MFUN(material_uniform_remove);
CK_DLL_MFUN(material_uniform_active_locations);

CK_DLL_MFUN(material_set_uniform_float);
CK_DLL_MFUN(material_get_uniform_float);
CK_DLL_MFUN(material_set_uniform_float2);
CK_DLL_MFUN(material_get_uniform_float2);
CK_DLL_MFUN(material_set_uniform_float3);
CK_DLL_MFUN(material_get_uniform_float3);
CK_DLL_MFUN(material_set_uniform_float4);
CK_DLL_MFUN(material_get_uniform_float4);

CK_DLL_MFUN(material_set_uniform_int);
CK_DLL_MFUN(material_get_uniform_int);
CK_DLL_MFUN(material_set_uniform_int2);
CK_DLL_MFUN(material_get_uniform_int2);
CK_DLL_MFUN(material_set_uniform_int3);
CK_DLL_MFUN(material_get_uniform_int3);
CK_DLL_MFUN(material_set_uniform_int4);
CK_DLL_MFUN(material_get_uniform_int4);

// getting back storage buffers is tricky because it may have been modified by shader
// so no getter for now (until we figure out compute shaders)
/*
possible impl for storage buffer getter:
- StorageBufferEvent to handle async buffer map from gpu --> render thread --> audio
thread
- GPUBuffer component exposed through chugl API that can be queried for buffer data
    - setStorageBuffer() instead of taking a ck_FloatArray, takes a GPUBuffer component
*/
CK_DLL_MFUN(material_set_storage_buffer);
CK_DLL_MFUN(material_set_storage_buffer_external);
CK_DLL_MFUN(material_set_sampler);
CK_DLL_MFUN(material_set_texture);
CK_DLL_MFUN(material_set_storage_texture);

CK_DLL_MFUN(material_get_sampler);
CK_DLL_MFUN(material_get_texture);

CK_DLL_CTOR(lines2d_material_ctor);
CK_DLL_MFUN(lines2d_material_get_thickness);
CK_DLL_MFUN(lines2d_material_set_thickness);
CK_DLL_MFUN(lines2d_material_get_color);
CK_DLL_MFUN(lines2d_material_set_color);
CK_DLL_MFUN(lines2d_material_get_extrusion);
CK_DLL_MFUN(lines2d_material_set_extrusion);
CK_DLL_MFUN(lines2d_material_get_loop);
CK_DLL_MFUN(lines2d_material_set_loop);

// flat material
CK_DLL_CTOR(flat_material_ctor);
CK_DLL_MFUN(flat_material_get_color);
CK_DLL_MFUN(flat_material_set_color);
CK_DLL_MFUN(flat_material_get_sampler);
CK_DLL_MFUN(flat_material_set_sampler);
CK_DLL_MFUN(flat_material_get_color_map);
CK_DLL_MFUN(flat_material_set_color_map);

CK_DLL_CTOR(uv_material_ctor);

CK_DLL_CTOR(normal_material_ctor);
CK_DLL_MFUN(normal_material_set_worldspace_normals);
CK_DLL_MFUN(normal_material_get_worldspace_normals);

CK_DLL_CTOR(tangent_material_ctor);
CK_DLL_MFUN(tangent_material_set_worldspace_tangents);
CK_DLL_MFUN(tangent_material_get_worldspace_tangents);

// phong ---------------------------------------------------------------------
CK_DLL_CTOR(phong_material_ctor);

CK_DLL_MFUN(phong_material_get_specular_color);
CK_DLL_MFUN(phong_material_set_specular_color);

CK_DLL_MFUN(phong_material_get_diffuse_color);
CK_DLL_MFUN(phong_material_set_diffuse_color);

CK_DLL_MFUN(phong_material_get_log_shininess);
CK_DLL_MFUN(phong_material_set_log_shininess);

CK_DLL_MFUN(phong_material_get_emission_color);
CK_DLL_MFUN(phong_material_set_emission_color);

CK_DLL_MFUN(phong_material_get_normal_factor);
CK_DLL_MFUN(phong_material_set_normal_factor);

CK_DLL_MFUN(phong_material_get_ao_factor);
CK_DLL_MFUN(phong_material_set_ao_factor);

CK_DLL_MFUN(phong_material_get_albedo_tex);
CK_DLL_MFUN(phong_material_set_albedo_tex);

CK_DLL_MFUN(phong_material_get_specular_tex);
CK_DLL_MFUN(phong_material_set_specular_tex);

CK_DLL_MFUN(phong_material_get_ao_tex);
CK_DLL_MFUN(phong_material_set_ao_tex);

CK_DLL_MFUN(phong_material_get_emissive_tex);
CK_DLL_MFUN(phong_material_set_emissive_tex);

CK_DLL_MFUN(phong_material_get_normal_tex);
CK_DLL_MFUN(phong_material_set_normal_tex);

// pbr ---------------------------------------------------------------------
CK_DLL_CTOR(pbr_material_ctor);

CK_DLL_MFUN(pbr_material_get_albedo);
CK_DLL_MFUN(pbr_material_set_albedo);

CK_DLL_MFUN(pbr_material_get_emissive);
CK_DLL_MFUN(pbr_material_set_emissive);

CK_DLL_MFUN(pbr_material_get_metallic);
CK_DLL_MFUN(pbr_material_set_metallic);

CK_DLL_MFUN(pbr_material_get_roughness);
CK_DLL_MFUN(pbr_material_set_roughness);

CK_DLL_MFUN(pbr_material_get_normal_factor);
CK_DLL_MFUN(pbr_material_set_normal_factor);

CK_DLL_MFUN(pbr_material_get_ao_factor);
CK_DLL_MFUN(pbr_material_set_ao_factor);

CK_DLL_MFUN(pbr_material_get_albedo_tex);
CK_DLL_MFUN(pbr_material_set_albedo_tex);

CK_DLL_MFUN(pbr_material_get_normal_tex);
CK_DLL_MFUN(pbr_material_set_normal_tex);

CK_DLL_MFUN(pbr_material_get_ao_tex);
CK_DLL_MFUN(pbr_material_set_ao_tex);

CK_DLL_MFUN(pbr_material_get_mr_tex);
CK_DLL_MFUN(pbr_material_set_mr_tex);

CK_DLL_MFUN(pbr_material_get_emissive_tex);
CK_DLL_MFUN(pbr_material_set_emissive_tex);

static_assert(sizeof(WGPUVertexFormat) == sizeof(int),
              "WGPUVertexFormat size mismatch");

void ulib_material_cq_update_all_uniforms(SG_Material* material)
{
    int num_uniforms = ARRAY_LENGTH(material->uniforms);
    for (int i = 0; i < num_uniforms; i++) {
        SG_MaterialUniform* uniform = &material->uniforms[i];
        switch (uniform->type) {
            case SG_MATERIAL_UNIFORM_NONE: break;
            case SG_MATERIAL_UNIFORM_STORAGE_BUFFER: {
                ASSERT(false); // not implemented
            } break;
            default: {
                CQ_PushCommand_MaterialSetUniform(material, i);
            }
        }
    }
}

void ulib_material_query(Chuck_DL_Query* QUERY)
{
    // TODO today: documentation

    BEGIN_CLASS("VertexFormat", "Object");
    DOC_CLASS("Vertex format enum. Used to describe vertex data layout in ShaderDesc.");

    static t_CKINT format_float  = WGPUVertexFormat_Float32;
    static t_CKINT format_float2 = WGPUVertexFormat_Float32x2;
    static t_CKINT format_float3 = WGPUVertexFormat_Float32x3;
    static t_CKINT format_float4 = WGPUVertexFormat_Float32x4;
    static t_CKINT format_int    = WGPUVertexFormat_Sint32;
    static t_CKINT format_int2   = WGPUVertexFormat_Sint32x2;
    static t_CKINT format_int3   = WGPUVertexFormat_Sint32x3;
    static t_CKINT format_int4   = WGPUVertexFormat_Sint32x4;

    SVAR("int", "FLOAT", &format_float);
    SVAR("int", "FLOAT2", &format_float2);
    SVAR("int", "FLOAT3", &format_float3);
    SVAR("int", "FLOAT4", &format_float4);
    SVAR("int", "INT", &format_int);
    SVAR("int", "INT2", &format_int2);
    SVAR("int", "INT3", &format_int3);
    SVAR("int", "INT4", &format_int4);

    END_CLASS();

    // ShaderDesc -----------------------------------------------------
    BEGIN_CLASS("ShaderDesc", "Object");
    DOC_CLASS(
      "Shader description object. Used to create a Shader component."
      "Set either vertexString or vertexFilepath, and either fragmentString or "
      "fragmentFilepath. `vertexLayout` field describes the vertex data layout "
      "of buffers going into the vertex shader--use the VertexFormat enum.");

    CTOR(shader_desc_ctor);

    shader_desc_vertex_string_offset     = MVAR("string", "vertexString", false);
    shader_desc_fragment_string_offset   = MVAR("string", "fragmentString", false);
    shader_desc_vertex_filepath_offset   = MVAR("string", "vertexFilepath", false);
    shader_desc_fragment_filepath_offset = MVAR("string", "fragmentFilepath", false);
    shader_desc_vertex_layout_offset     = MVAR("int[]", "vertexLayout", false);
    DOC_VAR(
      "Array of VertexFormat enums describing the vertex data layout."
      "E.g. if your vertex shader takes a vec3 position and a vec2 uv, set "
      "`vertexLayout` to [VertexFormat.FLOAT3, VertexFormat.FLOAT2].");
    shader_desc_compute_string_offset   = MVAR("string", "computeString", false);
    shader_desc_compute_filepath_offset = MVAR("string", "computeFilepath", false);
    shader_desc_is_lit                  = MVAR("int", "lit", false);
    DOC_VAR(
      "set to true if the shader is lit (uses lighting calculations). If set, the "
      "renderer will pass in lighting information as part of the per-frame uniforms");

    END_CLASS();

    // Shader -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_SHADER], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(shader_ctor_default);

    CTOR(shader_ctor);
    ARG("ShaderDesc", "shader_desc");
    DOC_FUNC("Create a Shader component. Immutable.");

    MFUN(shader_get_vertex_string, "string", "vertexString");
    DOC_FUNC("Get the vertex shader string passed in the ShaderDesc at creation.");

    MFUN(shader_get_fragment_string, "string", "fragmentString");
    DOC_FUNC("Get the fragment shader string passed in the ShaderDesc at creation.");

    MFUN(shader_get_vertex_filepath, "string", "vertexFilepath");
    DOC_FUNC("Get the vertex shader filepath passed in the ShaderDesc at creation.");

    MFUN(shader_get_fragment_filepath, "string", "fragmentFilepath");
    DOC_FUNC("Get the fragment shader filepath passed in the ShaderDesc at creation.");

    MFUN(shader_get_vertex_layout, "int[]", "vertexLayout");
    DOC_FUNC("Get the vertex layout passed in the ShaderDesc at creation.");

    MFUN(shader_get_lit, "int", "lit");
    DOC_FUNC("Get whether the shader is lit (uses lighting calculations).");

    END_CLASS();

    // Material -----------------------------------------------------
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_MATERIAL], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(material_ctor);

    // svars
    static t_CKINT cullmode_none  = WGPUCullMode_None;
    static t_CKINT cullmode_front = WGPUCullMode_Front;
    static t_CKINT cullmode_back  = WGPUCullMode_Back;
    SVAR("int", "CULL_NONE", &cullmode_none);
    DOC_VAR("No culling.");
    SVAR("int", "CULL_FRONT", &cullmode_front);
    DOC_VAR("Cull front faces.");
    SVAR("int", "CULL_BACK", &cullmode_back);
    DOC_VAR("Cull back faces.");

    static t_CKINT topology_pointlist     = WGPUPrimitiveTopology_PointList;
    static t_CKINT topology_linelist      = WGPUPrimitiveTopology_LineList;
    static t_CKINT topology_linestrip     = WGPUPrimitiveTopology_LineStrip;
    static t_CKINT topology_trianglelist  = WGPUPrimitiveTopology_TriangleList;
    static t_CKINT topology_trianglestrip = WGPUPrimitiveTopology_TriangleStrip;
    SVAR("int", "Topology_PointList", &topology_pointlist);
    DOC_VAR("Interpret each vertex as a point.");
    SVAR("int", "Topology_LineList", &topology_linelist);
    DOC_VAR("Interpret each pair of vertices as a line.");
    SVAR("int", "Topology_LineStrip", &topology_linestrip);
    DOC_VAR(
      "Each vertex after the first defines a line primitive between it and the "
      "previous vertex.");
    SVAR("int", "Topology_TriangleList", &topology_trianglelist);
    DOC_VAR("Interpret each triplet of vertices as a triangle.");
    SVAR("int", "Topology_TriangleStrip", &topology_trianglestrip);
    DOC_VAR(
      "Each vertex after the first two defines a triangle primitive between it and the "
      "previous two vertices.");

    // pso modifiers (shouldn't be set often, so we lump all together in a single
    // command that copies the entire PSO struct)
    MFUN(material_get_shader, SG_CKNames[SG_COMPONENT_SHADER], "shader");

    MFUN(material_set_shader, "void", "shader");
    ARG(SG_CKNames[SG_COMPONENT_SHADER], "shader");

    MFUN(material_get_cullmode, "int", "cullMode");
    DOC_FUNC(
      "Get the cull mode of the material. Material.CULL_NONE, Material.CULL_FRONT, or "
      "Material.CULL_BACK.");

    MFUN(material_set_cullmode, "void", "cullMode");
    ARG("int", "cullMode");
    DOC_FUNC(
      "Set the cull mode of the material. valid options: Material.CULL_NONE, "
      "Material.CULL_FRONT, or Material.CULL_BACK.");

    MFUN(material_set_topology, "void", "topology");
    ARG("int", "topology");
    DOC_FUNC(
      "Set the primitive topology of the material. valid options: "
      "Material.Topology_PointList, Material.Topology_LineList, "
      "Material.Topology_LineStrip, Material.Topology_TriangleList, or "
      "Material.Topology_TriangleStrip.");

    MFUN(material_get_topology, "int", "topology");
    DOC_FUNC(
      "Get the primitive topology of the material. Material.Topology_PointList, "
      "Material.Topology_LineList, Material.Topology_LineStrip, "
      "Material.Topology_TriangleList, or Material.Topology_TriangleStrip.");

    // uniforms
    MFUN(material_uniform_remove, "void", "removeUniform");
    ARG("int", "location");

    MFUN(material_uniform_active_locations, "int[]", "activeUniformLocations");

    MFUN(material_set_uniform_float, "void", "uniformFloat");
    ARG("int", "location");
    ARG("float", "uniform_value");

    MFUN(material_get_uniform_float, "float", "uniformFloat");
    ARG("int", "location");

    MFUN(material_set_uniform_int, "void", "uniformInt");
    ARG("int", "location");
    ARG("int", "uniform_value");

    MFUN(material_get_uniform_int, "int", "uniformInt");
    ARG("int", "location");

    MFUN(material_set_uniform_float2, "void", "uniformVec2");
    ARG("int", "location");
    ARG("vec2", "uniform_value");

    MFUN(material_get_uniform_float2, "vec2", "uniformVec2");
    ARG("int", "location");

    MFUN(material_set_uniform_float3, "void", "uniformVec3");
    ARG("int", "location");
    ARG("vec3", "uniform_value");

    MFUN(material_get_uniform_float3, "vec3", "uniformVec3");
    ARG("int", "location");

    MFUN(material_set_uniform_float4, "void", "uniformVec4");
    ARG("int", "location");
    ARG("vec4", "uniform_value");

    MFUN(material_get_uniform_float4, "vec4", "uniformVec4");
    ARG("int", "location");

    MFUN(material_set_uniform_int2, "void", "uniformInt2");
    ARG("int", "location");
    ARG("int", "x");
    ARG("int", "y");

    MFUN(material_get_uniform_int2, "int[]", "uniformInt2");
    ARG("int", "location");

    MFUN(material_set_uniform_int3, "void", "uniformInt3");
    ARG("int", "location");
    ARG("int", "x");
    ARG("int", "y");
    ARG("int", "z");

    MFUN(material_get_uniform_int3, "int[]", "uniformInt3");
    ARG("int", "location");

    MFUN(material_set_uniform_int4, "void", "uniformInt4");
    ARG("int", "location");
    ARG("int", "x");
    ARG("int", "y");
    ARG("int", "z");
    ARG("int", "w");

    MFUN(material_get_uniform_int4, "int[]", "uniformInt4");
    ARG("int", "location");

    // storage buffers
    MFUN(material_set_storage_buffer, "void", "storageBuffer");
    ARG("int", "location");
    ARG("float[]", "storageBuffer");

    // external storage buffer
    MFUN(material_set_storage_buffer_external, "void", "storageBuffer");
    ARG("int", "location");
    ARG("StorageBuffer", "storageBuffer");

    MFUN(material_set_sampler, "void", "sampler");
    ARG("int", "location");
    ARG("TextureSampler", "sampler");

    MFUN(material_get_sampler, "TextureSampler", "sampler");
    ARG("int", "location");

    MFUN(material_set_texture, "void", "texture");
    ARG("int", "location");
    ARG("Texture", "texture");

    MFUN(material_get_texture, "Texture", "texture");
    ARG("int", "location");

    MFUN(material_set_storage_texture, "void", "storageTexture");
    ARG("int", "location");
    ARG("Texture", "texture");
    DOC_FUNC(
      "Binds a storage texture at the given location. Defaults to the textures base "
      "mip level 0.");

    // abstract class, no destructor or constructor
    END_CLASS();

    // Lines2DMaterial -----------------------------------------------------
    BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_LINES2D],
                SG_CKNames[SG_COMPONENT_MATERIAL]);

    CTOR(lines2d_material_ctor);

    MFUN(lines2d_material_get_thickness, "float", "width");
    DOC_FUNC("Get the thickness of the lines in the material.");

    MFUN(lines2d_material_set_thickness, "void", "width");
    ARG("float", "thickness");
    DOC_FUNC("Set the thickness of the lines in the material.");

    MFUN(lines2d_material_get_color, "vec3", "color");
    DOC_FUNC("Get the line color");

    MFUN(lines2d_material_set_color, "void", "color");
    ARG("vec3", "color");
    DOC_FUNC("Set the line color");

    // MFUN(lines2d_material_get_extrusion, "float", "extrusion");
    // DOC_FUNC(
    //   "Get the miter extrusion ratio of the line. Varies from 0.0 to 1.0. A value of
    //   " "0.5 means the line width is split evenly on each side of each line segment "
    //   "position.");

    // MFUN(lines2d_material_set_extrusion, "void", "extrusion");
    // ARG("float", "extrusion");
    // DOC_FUNC(
    //   "Set the miter extrusion ratio of the line. Varies from 0.0 to 1.0. A value of
    //   " "0.5 means the line width is split evenly on each side of each line segment "
    //   "position.");

    // MFUN(lines2d_material_get_loop, "int", "loop");
    // DOC_FUNC("Get whether the line segments form a closed loop");

    // MFUN(lines2d_material_set_loop, "void", "loop");
    // ARG("int", "loop");
    // DOC_FUNC(
    //   "Set whether the line segments form a closed loop. Set via material.loop(true)
    //   " "or material.loop(false)");

    END_CLASS();

    // FlatMaterial -----------------------------------------------------

    BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_FLAT],
                SG_CKNames[SG_COMPONENT_MATERIAL]);
    DOC_CLASS("Simple flat-shaded material (not affected by lighting).");

    CTOR(flat_material_ctor);

    // color uniform
    MFUN(flat_material_get_color, "vec3", "color");
    DOC_FUNC("Get the color of the material.");

    MFUN(flat_material_set_color, "void", "color");
    ARG("vec3", "color");
    DOC_FUNC("Set material color uniform as an rgb. Alpha set to 1.0.");

    MFUN(flat_material_get_sampler, "TextureSampler", "sampler");
    DOC_FUNC("Get the sampler of the material.");

    MFUN(flat_material_set_sampler, "void", "sampler");
    ARG("TextureSampler", "sampler");
    DOC_FUNC("Set the sampler of the material.");

    MFUN(flat_material_get_color_map, "Texture", "colorMap");
    DOC_FUNC("Get the color map of the material.");

    MFUN(flat_material_set_color_map, "void", "colorMap");
    ARG("Texture", "colorMap");
    DOC_FUNC("Set the color map of the material.");

    END_CLASS();

    // UV Material -----------------------------------------------------

    BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_UV],
                SG_CKNames[SG_COMPONENT_MATERIAL]);
    DOC_CLASS("Visualize UV coordinates of a mesh.");

    CTOR(uv_material_ctor);

    END_CLASS();

    // Normal Material -----------------------------------------------------

    BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_NORMAL],
                SG_CKNames[SG_COMPONENT_MATERIAL]);
    DOC_CLASS("Visualize normals of a mesh.");

    CTOR(normal_material_ctor);

    MFUN(normal_material_set_worldspace_normals, "void", "worldspaceNormals");
    ARG("int", "use_worldspace_normals");
    DOC_FUNC(
      "Set whether to use worldspace normals. If false, visualizes normals in local "
      "object space.");

    MFUN(normal_material_get_worldspace_normals, "int", "worldspaceNormals");
    DOC_FUNC(
      "Get whether to use worldspace normals. If false, visualizes normals in local "
      "object space.");

    END_CLASS();

    // Tangent Material -----------------------------------------------------

    BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_TANGENT],
                SG_CKNames[SG_COMPONENT_MATERIAL]);
    DOC_CLASS("Visualize tangents of a mesh.");

    CTOR(tangent_material_ctor);

    MFUN(tangent_material_set_worldspace_tangents, "void", "worldspaceTangents");
    ARG("int", "use_worldspace_tangents");
    DOC_FUNC(
      "Set whether to use worldspace tangents. If false, visualizes tangents in local "
      "object space.");

    MFUN(tangent_material_get_worldspace_tangents, "int", "worldspaceTangents");
    DOC_FUNC(
      "Get whether to use worldspace tangents. If false, visualizes tangents in local "
      "object space.");

    END_CLASS();

    // Phong Material -----------------------------------------------------
    {
        BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_PHONG],
                    SG_CKNames[SG_COMPONENT_MATERIAL]);
        DOC_CLASS("Phong specular shading model");

        CTOR(phong_material_ctor);

        PHONG_MATERIAL_METHODS(phong);

        END_CLASS();
    }

    // PBR Material -----------------------------------------------------
    {
        BEGIN_CLASS(SG_MaterialTypeNames[SG_MATERIAL_PBR],
                    SG_CKNames[SG_COMPONENT_MATERIAL]);

        CTOR(pbr_material_ctor);

        MFUN(pbr_material_get_albedo, "vec3", "color");
        DOC_FUNC("Get the albedo color of the material.");

        MFUN(pbr_material_set_albedo, "void", "color");
        ARG("vec3", "albedo");
        DOC_FUNC("Set the albedo color of the material.");

        MFUN(pbr_material_get_emissive, "vec3", "emissive");
        DOC_FUNC("Get the emissive color of the material.");

        MFUN(pbr_material_set_emissive, "void", "emissive");
        ARG("vec3", "emissive");
        DOC_FUNC("Set the emissive color of the material.");

        MFUN(pbr_material_get_metallic, "float", "metallic");
        DOC_FUNC("Get the metallic factor of the material.");

        MFUN(pbr_material_set_metallic, "void", "metallic");
        ARG("float", "metallic");

        MFUN(pbr_material_get_roughness, "float", "roughness");
        DOC_FUNC("Get the roughness factor of the material.");

        MFUN(pbr_material_set_roughness, "void", "roughness");
        ARG("float", "roughness");
        DOC_FUNC("Set the roughness factor of the material.");

        MFUN(pbr_material_get_normal_factor, "float", "normalFactor");
        DOC_FUNC(
          "Get the normal factor of the material. Scales effect of normal map. Default "
          "1.0");

        MFUN(pbr_material_set_normal_factor, "void", "normalFactor");
        ARG("float", "normalFactor");
        DOC_FUNC(
          "Set the normal factor of the material. Scales effect of normal map. Default "
          "1.0");

        MFUN(pbr_material_get_ao_factor, "float", "aoFactor");
        DOC_FUNC("Get the ambient occlusion factor of the material. Default 1.0");

        MFUN(pbr_material_set_ao_factor, "void", "aoFactor");
        ARG("float", "aoFactor");
        DOC_FUNC("Set the ambient occlusion factor of the material. Default 1.0");

        MFUN(pbr_material_get_albedo_tex, SG_CKNames[SG_COMPONENT_TEXTURE], "colorMap");
        DOC_FUNC("Get the albedo texture of the material.");

        MFUN(pbr_material_set_albedo_tex, "void", "colorMap");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "albedoTexture");
        DOC_FUNC("Set the albedo texture of the material.");

        MFUN(pbr_material_get_normal_tex, SG_CKNames[SG_COMPONENT_TEXTURE],
             "normalMap");
        DOC_FUNC("Get the normal texture of the material.");

        MFUN(pbr_material_set_normal_tex, "void", "normalMap");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "normalTexture");
        DOC_FUNC("Set the normal texture of the material.");

        MFUN(pbr_material_get_ao_tex, SG_CKNames[SG_COMPONENT_TEXTURE], "aoMap");
        DOC_FUNC("Get the ambient occlusion texture of the material.");

        MFUN(pbr_material_set_ao_tex, "void", "aoMap");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "aoTexture");
        DOC_FUNC("Set the ambient occlusion texture of the material.");

        MFUN(pbr_material_get_mr_tex, SG_CKNames[SG_COMPONENT_TEXTURE], "mrMap");
        DOC_FUNC("Get the metallic-roughness texture of the material.");

        MFUN(pbr_material_set_mr_tex, "void", "mrMap");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "mrTexture");
        DOC_FUNC("Set the metallic-roughness texture of the material.");

        MFUN(pbr_material_get_emissive_tex, SG_CKNames[SG_COMPONENT_TEXTURE],
             "emissiveMap");
        DOC_FUNC("Get the emissive texture of the material.");

        MFUN(pbr_material_set_emissive_tex, "void", "emissiveMap");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "emissiveTexture");
        DOC_FUNC("Set the emissive texture of the material.");

        // abstract class, no destructor or constructor
        END_CLASS();
    }

    // initialize default components
    chugl_initDefaultMaterials();
}

// Shader ===================================================================

CK_DLL_CTOR(shader_desc_ctor)
{
    // chuck doesn't initialize class member vars in constructor. create manually
    // create string members (empty string)
    OBJ_MEMBER_STRING(SELF, shader_desc_vertex_string_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_fragment_string_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_vertex_filepath_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_fragment_filepath_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_compute_string_offset)
      = chugin_createCkString("");
    OBJ_MEMBER_STRING(SELF, shader_desc_compute_filepath_offset)
      = chugin_createCkString("");
}

CK_DLL_CTOR(shader_ctor_default)
{
    CK_THROW("ShaderConstructor",
             "Shader default constructor not allowed. use Shader(ShaderDesc) instead",
             SHRED);
}

CK_DLL_CTOR(shader_ctor)
{
    Chuck_Object* shader_desc = GET_NEXT_OBJECT(ARGS);

    WGPUVertexFormat vertex_layout[SG_GEOMETRY_MAX_VERTEX_ATTRIBUTES] = {};
    int vertex_layout_len = chugin_copyCkIntArray(
      OBJ_MEMBER_INT_ARRAY(shader_desc, shader_desc_vertex_layout_offset),
      (int*)vertex_layout, ARRAY_LENGTH(vertex_layout));

    // create shader on audio side
    SG_Shader* shader = SG_CreateShader(
      SELF,
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_vertex_string_offset)),
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_fragment_string_offset)),
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_vertex_filepath_offset)),
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_fragment_filepath_offset)),
      vertex_layout, vertex_layout_len,
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_compute_string_offset)),
      API->object->str(
        OBJ_MEMBER_STRING(shader_desc, shader_desc_compute_filepath_offset)),
      (bool)OBJ_MEMBER_INT(shader_desc, shader_desc_is_lit));

    // save component id
    OBJ_MEMBER_UINT(SELF, component_offset_id) = shader->id;

    // push to command queue
    CQ_PushCommand_ShaderCreate(shader);
}

CK_DLL_MFUN(shader_get_vertex_string)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->vertex_string_owned);
}

CK_DLL_MFUN(shader_get_fragment_string)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->fragment_string_owned);
}

CK_DLL_MFUN(shader_get_vertex_filepath)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->vertex_filepath_owned);
}

CK_DLL_MFUN(shader_get_fragment_filepath)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_string  = chugin_createCkString(shader->fragment_filepath_owned);
}

CK_DLL_MFUN(shader_get_vertex_layout)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_object  = (Chuck_Object*)chugin_createCkIntArray(
      (int*)shader->vertex_layout, ARRAY_LENGTH(shader->vertex_layout));
}

CK_DLL_MFUN(shader_get_lit)
{
    SG_Shader* shader = GET_SHADER(SELF);
    RETURN->v_int     = (t_CKINT)shader->lit;
}

// Material ===================================================================

CK_DLL_CTOR(material_ctor)
{
    SG_Material* material = SG_CreateMaterial(SELF, SG_MATERIAL_CUSTOM);
    ASSERT(material->type == SG_COMPONENT_MATERIAL);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = material->id;

    CQ_PushCommand_MaterialCreate(material);
}

CK_DLL_MFUN(material_get_shader)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Shader* shader     = SG_GetShader(material->pso.sg_shader_id);
    RETURN->v_object      = shader ? shader->ckobj : NULL;
}

void chugl_materialSetShader(SG_Material* material, SG_Shader* shader)
{
    SG_Material::shader(material, shader);
    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_set_shader)
{
    Chuck_Object* shader  = GET_NEXT_OBJECT(ARGS);
    SG_Material* material = GET_MATERIAL(SELF);

    chugl_materialSetShader(material, GET_SHADER(shader));
}

CK_DLL_MFUN(material_get_cullmode)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_int         = (t_CKINT)material->pso.cull_mode;
}

CK_DLL_MFUN(material_set_cullmode)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    t_CKINT cull_mode       = GET_NEXT_INT(ARGS);
    material->pso.cull_mode = (WGPUCullMode)cull_mode;

    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_set_topology)
{
    SG_Material* material            = GET_MATERIAL(SELF);
    t_CKINT primitive_topology       = GET_NEXT_INT(ARGS);
    material->pso.primitive_topology = (WGPUPrimitiveTopology)primitive_topology;

    CQ_PushCommand_MaterialUpdatePSO(material);
}

CK_DLL_MFUN(material_get_topology)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_int         = (t_CKINT)material->pso.primitive_topology;
}

CK_DLL_MFUN(material_uniform_active_locations)
{
    SG_Material* material = GET_MATERIAL(SELF);

    int active_locations[SG_MATERIAL_MAX_UNIFORMS];
    int active_locations_count = 0;

    for (int i = 0; i < SG_MATERIAL_MAX_UNIFORMS; i++) {
        if (material->uniforms[i].type != SG_MATERIAL_UNIFORM_NONE) {
            active_locations[active_locations_count++] = i;
        }
    }

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(active_locations,
                                                              active_locations_count);
}

CK_DLL_MFUN(material_uniform_remove)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    SG_Material::removeUniform(material, location);

    // TODO push to command queue
}

CK_DLL_MFUN(material_set_uniform_float)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    t_CKINT location        = GET_NEXT_INT(ARGS);
    t_CKFLOAT uniform_value = GET_NEXT_FLOAT(ARGS);
    float uniform_value_f32 = (float)uniform_value;

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_FLOAT);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_FLOAT) {
        CK_THROW("MaterialGetUniformFloat", "Uniform location is not a float", SHRED);
    }

    RETURN->v_float = material->uniforms[location].as.f;
}

CK_DLL_MFUN(material_set_uniform_float2)
{
    SG_Material* material       = GET_MATERIAL(SELF);
    t_CKINT location            = GET_NEXT_INT(ARGS);
    t_CKVEC2 uniform_value      = GET_NEXT_VEC2(ARGS);
    glm::vec2 uniform_value_f32 = { uniform_value.x, uniform_value.y };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC2F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float2)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC2F) {
        CK_THROW("MaterialGetUniformFloat2", "Uniform location is not a vec2", SHRED);
    }

    glm::vec2 uniform_value = material->uniforms[location].as.vec2f;
    RETURN->v_vec2          = { uniform_value.x, uniform_value.y };
}

CK_DLL_MFUN(material_set_uniform_float3)
{
    SG_Material* material       = GET_MATERIAL(SELF);
    t_CKINT location            = GET_NEXT_INT(ARGS);
    t_CKVEC3 uniform_value      = GET_NEXT_VEC3(ARGS);
    glm::vec3 uniform_value_f32 = { uniform_value.x, uniform_value.y, uniform_value.z };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC3F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float3)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC3F) {
        CK_THROW("MaterialGetUniformFloat3", "Uniform location is not a vec3", SHRED);
    }

    glm::vec3 uniform_value = material->uniforms[location].as.vec3f;
    RETURN->v_vec3          = { uniform_value.x, uniform_value.y, uniform_value.z };
}

CK_DLL_MFUN(material_set_uniform_float4)
{
    SG_Material* material  = GET_MATERIAL(SELF);
    t_CKINT location       = GET_NEXT_INT(ARGS);
    t_CKVEC4 uniform_value = GET_NEXT_VEC4(ARGS);
    glm::vec4 uniform_value_f32
      = { uniform_value.x, uniform_value.y, uniform_value.z, uniform_value.w };

    SG_Material::setUniform(material, location, &uniform_value_f32,
                            SG_MATERIAL_UNIFORM_VEC4F);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_float4)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_VEC4F) {
        CK_THROW("MaterialGetUniformFloat4", "Uniform location is not a vec4", SHRED);
    }

    glm::vec4 uniform_value = material->uniforms[location].as.vec4f;
    RETURN->v_vec4
      = { uniform_value.x, uniform_value.y, uniform_value.z, uniform_value.w };
}

CK_DLL_MFUN(material_set_uniform_int)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    t_CKINT uniform_value = GET_NEXT_INT(ARGS);
    i32 uniform_value_i32 = (i32)uniform_value;

    SG_Material::setUniform(material, location, &uniform_value_i32,
                            SG_MATERIAL_UNIFORM_INT);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_int)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_INT) {
        CK_THROW("MaterialGetUniformInt", "Uniform location is not an int", SHRED);
    }

    RETURN->v_int = material->uniforms[location].as.i;
}

CK_DLL_MFUN(material_set_uniform_int2)
{
    SG_Material* material    = GET_MATERIAL(SELF);
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT x                = GET_NEXT_INT(ARGS);
    t_CKINT y                = GET_NEXT_INT(ARGS);
    glm::ivec2 uniform_value = { x, y };

    SG_Material::setUniform(material, location, &uniform_value,
                            SG_MATERIAL_UNIFORM_IVEC2);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_int2)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_IVEC2) {
        CK_THROW("MaterialGetUniformInt2", "Uniform location is not an int2", SHRED);
    }

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(
      (int*)&material->uniforms[location].as.ivec2, 2);
}

CK_DLL_MFUN(material_set_uniform_int3)
{
    SG_Material* material    = GET_MATERIAL(SELF);
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT x                = GET_NEXT_INT(ARGS);
    t_CKINT y                = GET_NEXT_INT(ARGS);
    t_CKINT z                = GET_NEXT_INT(ARGS);
    glm::ivec3 uniform_value = { x, y, z };

    SG_Material::setUniform(material, location, &uniform_value,
                            SG_MATERIAL_UNIFORM_IVEC3);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_int3)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_IVEC3) {
        CK_THROW("MaterialGetUniformInt3", "Uniform location is not an int3", SHRED);
    }

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(
      (int*)&material->uniforms[location].as.ivec3, 3);
}

CK_DLL_MFUN(material_set_uniform_int4)
{
    SG_Material* material    = GET_MATERIAL(SELF);
    t_CKINT location         = GET_NEXT_INT(ARGS);
    t_CKINT x                = GET_NEXT_INT(ARGS);
    t_CKINT y                = GET_NEXT_INT(ARGS);
    t_CKINT z                = GET_NEXT_INT(ARGS);
    t_CKINT w                = GET_NEXT_INT(ARGS);
    glm::ivec4 uniform_value = { x, y, z, w };

    SG_Material::setUniform(material, location, &uniform_value,
                            SG_MATERIAL_UNIFORM_IVEC4);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_uniform_int4)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_IVEC4) {
        CK_THROW("MaterialGetUniformInt4", "Uniform location is not an int4", SHRED);
    }

    RETURN->v_object = (Chuck_Object*)chugin_createCkIntArray(
      (int*)&material->uniforms[location].as.ivec4, 4);
}

CK_DLL_MFUN(material_set_storage_buffer)
{
    SG_Material* material    = GET_MATERIAL(SELF);
    t_CKINT location         = GET_NEXT_INT(ARGS);
    Chuck_ArrayFloat* ck_arr = GET_NEXT_FLOAT_ARRAY(ARGS);

    SG_Material::setStorageBuffer(material, location);

    CQ_PushCommand_MaterialSetStorageBuffer(material, location, ck_arr);
}

CK_DLL_MFUN(material_set_storage_buffer_external)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Buffer* buffer     = SG_GetBuffer(OBJ_MEMBER_UINT(ckobj, component_offset_id));

    SG_Material::storageBuffer(material, location, buffer);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_set_sampler)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    SG_Sampler sampler    = SG_Sampler::fromCkObj(GET_NEXT_OBJECT(ARGS));

    SG_Material::setSampler(material, location, sampler);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_sampler)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_SAMPLER) {
        CK_THROW("MaterialGetSampler", "Material bind location is not a sampler",
                 SHRED);
    }

    RETURN->v_object = ulib_texture_ckobj_from_sampler(
      material->uniforms[location].as.sampler, false, SHRED);
}

CK_DLL_MFUN(material_set_texture)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id));

    SG_Material::setTexture(material, location, tex);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(material_get_texture)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);

    if (material->uniforms[location].type != SG_MATERIAL_UNIFORM_TEXTURE
        && material->uniforms[location].type != SG_MATERIAL_STORAGE_TEXTURE) {
        CK_THROW("MaterialGetTexture", "Material bind location is not a texture",
                 SHRED);
    }

    SG_Texture* tex  = SG_GetTexture(material->uniforms[location].as.texture_id);
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(material_set_storage_texture)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT location      = GET_NEXT_INT(ARGS);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id));

    SG_Material::setStorageTexture(material, location, tex);

    CQ_PushCommand_MaterialSetUniform(material, location);
}

// Lines2DMaterial ===================================================================

static void ulib_material_init_uniforms_and_pso(SG_Material* material)
{
    switch (material->material_type) {
        case SG_MATERIAL_CUSTOM: {
            // do nothing
        } break;
        case SG_MATERIAL_LINES2D: {
            SG_Material::uniformFloat(material, 0, 0.1f); // thickness
            CQ_PushCommand_MaterialSetUniform(material, 0);

            SG_Material::uniformVec3f(material, 1, glm::vec3(1.0f)); // color
            CQ_PushCommand_MaterialSetUniform(material, 1);

            // SG_Material::uniformInt(material, 2, 0); // loop
            // CQ_PushCommand_MaterialSetUniform(material, 2);

            // SG_Material::uniformFloat(material, 3, 0.5f); // extrusion
            // CQ_PushCommand_MaterialSetUniform(material, 3);

            // shader
            SG_Shader* lines2d_shader
              = SG_GetShader(g_material_builtin_shaders.lines2d_shader_id);
            ASSERT(lines2d_shader);

            chugl_materialSetShader(material, lines2d_shader);

            // set pso
            material->pso.primitive_topology = WGPUPrimitiveTopology_TriangleStrip;
            CQ_PushCommand_MaterialUpdatePSO(material);
        } break;
        case SG_MATERIAL_FLAT: {
            SG_Shader* flat_shader
              = SG_GetShader(g_material_builtin_shaders.flat_shader_id);
            chugl_materialSetShader(material, flat_shader);

            // set uniform
            SG_Material::uniformVec4f(material, 0, glm::vec4(1.0f));
            SG_Material::setSampler(material, 1, SG_SAMPLER_DEFAULT);
            SG_Material::setTexture(
              material, 2,
              SG_GetTexture(g_builtin_textures.white_pixel_id)); // color map

            ulib_material_cq_update_all_uniforms(material);
        } break;
        case SG_MATERIAL_UV: {
            // init shader
            SG_Shader* shader = SG_GetShader(g_material_builtin_shaders.uv_shader_id);
            ASSERT(shader);

            chugl_materialSetShader(material, shader);
        } break;
        case SG_MATERIAL_NORMAL: {
            // init shader
            SG_Shader* shader
              = SG_GetShader(g_material_builtin_shaders.normal_shader_id);
            ASSERT(shader);

            chugl_materialSetShader(material, shader);

            SG_Material::uniformInt(material, 0, 1); // use_worldspace_normals
            CQ_PushCommand_MaterialSetUniform(material, 0);
        } break;
        case SG_MATERIAL_TANGENT: {
            // init shader
            SG_Shader* shader
              = SG_GetShader(g_material_builtin_shaders.tangent_shader_id);
            ASSERT(shader);

            chugl_materialSetShader(material, shader);

            SG_Material::uniformInt(material, 0, 1); // use_worldspace_tangents
            CQ_PushCommand_MaterialSetUniform(material, 0);
        } break;
        case SG_MATERIAL_PHONG: {
            SG_Shader* shader
              = SG_GetShader(g_material_builtin_shaders.phong_shader_id);
            chugl_materialSetShader(material, shader);

            // init uniforms
            {
                PhongParams::specular(material, glm::vec3(.2f));
                PhongParams::diffuse(material, glm::vec3(1.0f));
                PhongParams::shininess(material, 64.0f);
                PhongParams::emission(material, glm::vec3(0.0f));
                PhongParams::normalFactor(material, 1.0f);
                PhongParams::aoFactor(material, 1.0f);

                // textures
                PhongParams::sampler(material, SG_SAMPLER_DEFAULT);
                PhongParams::albedoTex(
                  material, SG_GetTexture(g_builtin_textures.white_pixel_id));
                PhongParams::specularTex(
                  material, SG_GetTexture(g_builtin_textures.white_pixel_id));
                PhongParams::aoTex(material,
                                   SG_GetTexture(g_builtin_textures.white_pixel_id));
                PhongParams::emissiveTex(
                  material, SG_GetTexture(g_builtin_textures.black_pixel_id));
                PhongParams::normalTex(
                  material, SG_GetTexture(g_builtin_textures.normal_pixel_id));
            }
        } break;
        case SG_MATERIAL_PBR: {
            // init shader
            SG_Shader* shader = SG_GetShader(g_material_builtin_shaders.pbr_shader_id);
            ASSERT(shader);

            chugl_materialSetShader(material, shader);

            // init uniforms
            {
                SG_Material::setSampler(material, 0,
                                        SG_SAMPLER_DEFAULT); // texture sampler
                SG_Material::setTexture(
                  material, 1,
                  SG_GetTexture(g_builtin_textures.white_pixel_id)); // albedo
                SG_Material::setTexture(
                  material, 2,
                  SG_GetTexture(g_builtin_textures.normal_pixel_id)); // normal
                SG_Material::setTexture(
                  material, 3,
                  SG_GetTexture(g_builtin_textures.white_pixel_id)); // ao
                SG_Material::setTexture(
                  material, 4,
                  SG_GetTexture(g_builtin_textures.white_pixel_id)); // mr
                SG_Material::setTexture(
                  material, 5,
                  SG_GetTexture(g_builtin_textures.black_pixel_id));     // emissive
                SG_Material::uniformVec4f(material, 6, glm::vec4(1.0f)); // albedo
                SG_Material::uniformVec3f(material, 7, glm::vec3(0.0f)); // emissive
                SG_Material::uniformFloat(material, 8, 0.0f);            // metallic
                SG_Material::uniformFloat(material, 9, 0.5f);            // roughness
                SG_Material::uniformFloat(material, 10, 1.0f); // normal factor
                SG_Material::uniformFloat(material, 11, 1.0f); // ao factor

                ulib_material_cq_update_all_uniforms(material);
            }
        } break;
        default: ASSERT(false);
    }
}

SG_Material* ulib_material_create(SG_MaterialType type, Chuck_VM_Shred* shred)
{
    CK_DL_API API = g_chuglAPI;

    Chuck_Object* ckobj = NULL;
    if (shred)
        ckobj = chugin_createCkObj(SG_MaterialTypeNames[type], false, shred);
    else
        ckobj = chugin_createCkObj(SG_MaterialTypeNames[type], false);

    SG_Material* material = SG_CreateMaterial(ckobj, type);
    ASSERT(material->type == SG_COMPONENT_MATERIAL);
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = material->id;

    CQ_PushCommand_MaterialCreate(material);

    ulib_material_init_uniforms_and_pso(material);

    return material;
}

CK_DLL_CTOR(lines2d_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_LINES2D;

    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(lines2d_material_get_thickness)
{
    RETURN->v_float = GET_MATERIAL(SELF)->uniforms[0].as.f;
}

CK_DLL_MFUN(lines2d_material_set_thickness)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT thickness   = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 0, (f32)thickness);
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(lines2d_material_get_color)
{
    glm::vec3 color = GET_MATERIAL(SELF)->uniforms[1].as.vec3f;
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(lines2d_material_set_color)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKVEC3 color        = GET_NEXT_VEC3(ARGS);

    SG_Material::uniformVec3f(material, 1, glm::vec3(color.x, color.y, color.z));
    CQ_PushCommand_MaterialSetUniform(material, 1);
}

CK_DLL_MFUN(lines2d_material_get_extrusion)
{
    RETURN->v_float = GET_MATERIAL(SELF)->uniforms[3].as.f;
}

CK_DLL_MFUN(lines2d_material_set_extrusion)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT extrusion   = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 3, (f32)extrusion);
    CQ_PushCommand_MaterialSetUniform(material, 3);
}

CK_DLL_MFUN(lines2d_material_get_loop)
{
    RETURN->v_int = GET_MATERIAL(SELF)->uniforms[2].as.i;
}

CK_DLL_MFUN(lines2d_material_set_loop)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKINT loop          = GET_NEXT_INT(ARGS);

    SG_Material::uniformInt(material, 2, loop ? 1 : 0);
    CQ_PushCommand_MaterialSetUniform(material, 2);
}

// FlatMaterial ===================================================================

CK_DLL_CTOR(flat_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_FLAT;
    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(flat_material_get_color)
{
    glm::vec4 color = GET_MATERIAL(SELF)->uniforms[0].as.vec4f;
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(flat_material_set_color)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKVEC3 color        = GET_NEXT_VEC3(ARGS);

    SG_Material::uniformVec4f(material, 0, glm::vec4(color.x, color.y, color.z, 1));
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(flat_material_get_sampler)
{
    RETURN->v_object = ulib_texture_ckobj_from_sampler(
      GET_MATERIAL(SELF)->uniforms[1].as.sampler, false, SHRED);
}

CK_DLL_MFUN(flat_material_set_sampler)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);

    SG_Material::setSampler(material, 1, SG_Sampler::fromCkObj(ckobj));
    CQ_PushCommand_MaterialSetUniform(material, 1);
}

CK_DLL_MFUN(flat_material_get_color_map)
{
    RETURN->v_object
      = SG_GetTexture(GET_MATERIAL(SELF)->uniforms[2].as.texture_id)->ckobj;
}

CK_DLL_MFUN(flat_material_set_color_map)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.white_pixel_id);

    SG_Material::setTexture(material, 2, tex);
    CQ_PushCommand_MaterialSetUniform(material, 2);
}

// UVMaterial ===================================================================

CK_DLL_CTOR(uv_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_UV;

    ulib_material_init_uniforms_and_pso(material);
}

// NormalMaterial ===================================================================

CK_DLL_CTOR(normal_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_NORMAL;

    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(normal_material_set_worldspace_normals)
{
    SG_Material* material          = GET_MATERIAL(SELF);
    t_CKINT use_worldspace_normals = GET_NEXT_INT(ARGS);

    SG_Material::uniformInt(material, 0, use_worldspace_normals ? 1 : 0);
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(normal_material_get_worldspace_normals)
{
    RETURN->v_int = GET_MATERIAL(SELF)->uniforms[0].as.i;
}

// TangentMaterial ===================================================================

CK_DLL_CTOR(tangent_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_TANGENT;

    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(tangent_material_set_worldspace_tangents)
{
    SG_Material* material           = GET_MATERIAL(SELF);
    t_CKINT use_worldspace_tangents = GET_NEXT_INT(ARGS);

    SG_Material::uniformInt(material, 0, use_worldspace_tangents ? 1 : 0);
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(tangent_material_get_worldspace_tangents)
{
    RETURN->v_int = GET_MATERIAL(SELF)->uniforms[0].as.i;
}

// PhongMaterial ===================================================================

CK_DLL_CTOR(phong_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_PHONG;

    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(phong_material_get_specular_color)
{
    glm::vec3 color = *PhongParams::specular(GET_MATERIAL(SELF));
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(phong_material_set_specular_color)
{
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    PhongParams::specular(GET_MATERIAL(SELF), glm::vec3(color.x, color.y, color.z));
}

CK_DLL_MFUN(phong_material_get_diffuse_color)
{
    glm::vec4 color = *PhongParams::diffuse(GET_MATERIAL(SELF));
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(phong_material_set_diffuse_color)
{
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    PhongParams::diffuse(GET_MATERIAL(SELF), glm::vec3(color.x, color.y, color.z));
}

CK_DLL_MFUN(phong_material_get_log_shininess)
{
    RETURN->v_float = glm::log2(*PhongParams::shininess(GET_MATERIAL(SELF)));
}

CK_DLL_MFUN(phong_material_set_log_shininess)
{
    t_CKFLOAT shininess = GET_NEXT_FLOAT(ARGS);
    PhongParams::shininess(GET_MATERIAL(SELF), glm::pow(2.0f, (f32)shininess));
}

CK_DLL_MFUN(phong_material_get_emission_color)
{
    glm::vec3 color = *PhongParams::emission(GET_MATERIAL(SELF));
    RETURN->v_vec3  = { color.r, color.g, color.b };
}

CK_DLL_MFUN(phong_material_set_emission_color)
{
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
    PhongParams::emission(GET_MATERIAL(SELF), glm::vec3(color.x, color.y, color.z));
}

CK_DLL_MFUN(phong_material_get_normal_factor)
{
    RETURN->v_float = *PhongParams::normalFactor(GET_MATERIAL(SELF));
}

CK_DLL_MFUN(phong_material_set_normal_factor)
{
    PhongParams::normalFactor(GET_MATERIAL(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(phong_material_get_ao_factor)
{
    RETURN->v_float = *PhongParams::aoFactor(GET_MATERIAL(SELF));
}

CK_DLL_MFUN(phong_material_set_ao_factor)
{
    PhongParams::aoFactor(GET_MATERIAL(SELF), GET_NEXT_FLOAT(ARGS));
}

CK_DLL_MFUN(phong_material_get_albedo_tex)
{
    SG_Texture* tex  = PhongParams::albedoTex(GET_MATERIAL(SELF));
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(phong_material_set_albedo_tex)
{
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex
      = ckobj ? SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) : NULL;
    PhongParams::albedoTex(GET_MATERIAL(SELF), tex);
}

CK_DLL_MFUN(phong_material_get_specular_tex)
{
    SG_Texture* tex  = PhongParams::specularTex(GET_MATERIAL(SELF));
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(phong_material_set_specular_tex)
{
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex
      = ckobj ? SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) : NULL;
    PhongParams::specularTex(GET_MATERIAL(SELF), tex);
}

CK_DLL_MFUN(phong_material_get_ao_tex)
{
    SG_Texture* tex  = PhongParams::aoTex(GET_MATERIAL(SELF));
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(phong_material_set_ao_tex)
{
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex
      = ckobj ? SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) : NULL;
    PhongParams::aoTex(GET_MATERIAL(SELF), tex);
}

CK_DLL_MFUN(phong_material_get_emissive_tex)
{
    SG_Texture* tex  = PhongParams::emissiveTex(GET_MATERIAL(SELF));
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(phong_material_set_emissive_tex)
{
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex
      = ckobj ? SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) : NULL;
    PhongParams::emissiveTex(GET_MATERIAL(SELF), tex);
}

CK_DLL_MFUN(phong_material_get_normal_tex)
{
    SG_Texture* tex  = PhongParams::normalTex(GET_MATERIAL(SELF));
    RETURN->v_object = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(phong_material_set_normal_tex)
{
    Chuck_Object* ckobj = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex
      = ckobj ? SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id)) : NULL;
    PhongParams::normalTex(GET_MATERIAL(SELF), tex);
}

// PBRMaterial ===================================================================

CK_DLL_CTOR(pbr_material_ctor)
{
    SG_Material* material   = GET_MATERIAL(SELF);
    material->material_type = SG_MATERIAL_PBR;

    ulib_material_init_uniforms_and_pso(material);
}

CK_DLL_MFUN(pbr_material_get_albedo)
{
    SG_Material* material = GET_MATERIAL(SELF);
    glm::vec4 color       = material->uniforms[6].as.vec4f;

    RETURN->v_vec3 = { color.r, color.g, color.b };
}

CK_DLL_MFUN(pbr_material_set_albedo)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKVEC3 color        = GET_NEXT_VEC3(ARGS);

    SG_Material::uniformVec4f(material, 6, glm::vec4(color.x, color.y, color.z, 1));
    CQ_PushCommand_MaterialSetUniform(material, 6);
}

CK_DLL_MFUN(pbr_material_get_emissive)
{
    SG_Material* material = GET_MATERIAL(SELF);
    glm::vec3 color       = material->uniforms[7].as.vec3f;

    RETURN->v_vec3 = { color.r, color.g, color.b };
}

CK_DLL_MFUN(pbr_material_set_emissive)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKVEC3 color        = GET_NEXT_VEC3(ARGS);

    SG_Material::uniformVec3f(material, 7, glm::vec3(color.x, color.y, color.z));
    CQ_PushCommand_MaterialSetUniform(material, 7);
}

CK_DLL_MFUN(pbr_material_get_metallic)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_float       = material->uniforms[8].as.f;
}

CK_DLL_MFUN(pbr_material_set_metallic)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT metallic    = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 8, (f32)metallic);
    CQ_PushCommand_MaterialSetUniform(material, 8);
}

CK_DLL_MFUN(pbr_material_get_roughness)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_float       = material->uniforms[9].as.f;
}

CK_DLL_MFUN(pbr_material_set_roughness)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT roughness   = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 9, (f32)roughness);
    CQ_PushCommand_MaterialSetUniform(material, 9);
}

CK_DLL_MFUN(pbr_material_get_normal_factor)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_float       = material->uniforms[10].as.f;
}

CK_DLL_MFUN(pbr_material_set_normal_factor)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT normal      = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 10, (f32)normal);
    CQ_PushCommand_MaterialSetUniform(material, 10);
}

CK_DLL_MFUN(pbr_material_get_ao_factor)
{
    SG_Material* material = GET_MATERIAL(SELF);
    RETURN->v_float       = material->uniforms[11].as.f;
}

CK_DLL_MFUN(pbr_material_set_ao_factor)
{
    SG_Material* material = GET_MATERIAL(SELF);
    t_CKFLOAT ao          = GET_NEXT_FLOAT(ARGS);

    SG_Material::uniformFloat(material, 11, (f32)ao);
    CQ_PushCommand_MaterialSetUniform(material, 11);
}

CK_DLL_MFUN(pbr_material_get_albedo_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Texture* tex       = SG_GetTexture(material->uniforms[1].as.texture_id);
    RETURN->v_object      = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(pbr_material_set_albedo_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_INT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.white_pixel_id);

    SG_Material::setTexture(material, 1, tex);
    CQ_PushCommand_MaterialSetUniform(material, 1);
}

CK_DLL_MFUN(pbr_material_get_normal_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Texture* tex       = SG_GetTexture(material->uniforms[2].as.texture_id);
    RETURN->v_object      = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(pbr_material_set_normal_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_INT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.normal_pixel_id);

    SG_Material::setTexture(material, 2, tex);
    CQ_PushCommand_MaterialSetUniform(material, 2);
}

CK_DLL_MFUN(pbr_material_get_ao_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Texture* tex       = SG_GetTexture(material->uniforms[3].as.texture_id);
    RETURN->v_object      = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(pbr_material_set_ao_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_INT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.white_pixel_id);

    SG_Material::setTexture(material, 3, tex);
    CQ_PushCommand_MaterialSetUniform(material, 3);
}

CK_DLL_MFUN(pbr_material_get_mr_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Texture* tex       = SG_GetTexture(material->uniforms[4].as.texture_id);
    RETURN->v_object      = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(pbr_material_set_mr_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_INT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.white_pixel_id);

    SG_Material::setTexture(material, 4, tex);
    CQ_PushCommand_MaterialSetUniform(material, 4);
}

CK_DLL_MFUN(pbr_material_get_emissive_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    SG_Texture* tex       = SG_GetTexture(material->uniforms[5].as.texture_id);
    RETURN->v_object      = tex ? tex->ckobj : NULL;
}

CK_DLL_MFUN(pbr_material_set_emissive_tex)
{
    SG_Material* material = GET_MATERIAL(SELF);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = ckobj ?
                              SG_GetTexture(OBJ_MEMBER_INT(ckobj, component_offset_id)) :
                              SG_GetTexture(g_builtin_textures.black_pixel_id);

    SG_Material::setTexture(material, 5, tex);
    CQ_PushCommand_MaterialSetUniform(material, 5);
}

// init default materials ========================================================

static SG_ID
chugl_createShader(CK_DL_API API, const char* vertex_string,
                   const char* fragment_string, const char* vertex_filepath,
                   const char* fragment_filepath, WGPUVertexFormat* vertex_layout,
                   int vertex_layout_count, const char* compute_string = "",
                   const char* compute_filepath = "", bool lit = false)
{
    Chuck_Object* shader_ckobj
      = chugin_createCkObj(SG_CKNames[SG_COMPONENT_SHADER], true);

    // create shader on audio side
    SG_Shader* shader = SG_CreateShader(
      shader_ckobj, vertex_string, fragment_string, vertex_filepath, fragment_filepath,
      vertex_layout, vertex_layout_count, compute_string, compute_filepath, lit);

    // save component id
    OBJ_MEMBER_UINT(shader_ckobj, component_offset_id) = shader->id;

    // push to command queue
    CQ_PushCommand_ShaderCreate(shader);

    return shader->id;
}

static SG_ID chugl_createComputeShader(const char* compute_string,
                                       const char* compute_filepath)
{
    return chugl_createShader(g_chuglAPI, NULL, NULL, NULL, NULL, NULL, 0,
                              compute_string, compute_filepath);
}

void chugl_initDefaultMaterials()
{
    static WGPUVertexFormat standard_vertex_layout[] = {
        WGPUVertexFormat_Float32x3, // position
        WGPUVertexFormat_Float32x3, // normal
        WGPUVertexFormat_Float32x2, // uv
        WGPUVertexFormat_Float32x4, // tangent
    };

    static WGPUVertexFormat gtext_vertex_layout[] = {
        WGPUVertexFormat_Float32x2, // position
        WGPUVertexFormat_Float32x2, // uv
        WGPUVertexFormat_Sint32,    // glyph_index
    };

    g_material_builtin_shaders.lines2d_shader_id = chugl_createShader(
      g_chuglAPI, lines2d_shader_string, lines2d_shader_string, NULL, NULL, NULL, 0);
    g_material_builtin_shaders.flat_shader_id = chugl_createShader(
      g_chuglAPI, flat_shader_string, flat_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout));
    g_material_builtin_shaders.gtext_shader_id = chugl_createShader(
      g_chuglAPI, gtext_shader_string, gtext_shader_string, NULL, NULL,
      gtext_vertex_layout, ARRAY_LENGTH(gtext_vertex_layout));

    g_material_builtin_shaders.output_pass_shader_id
      = chugl_createShader(g_chuglAPI, output_pass_shader_string,
                           output_pass_shader_string, NULL, NULL, NULL, 0);

    g_material_builtin_shaders.bloom_downsample_shader_id
      = chugl_createComputeShader(bloom_downsample_shader_string, NULL);

    g_material_builtin_shaders.bloom_upsample_shader_id
      = chugl_createComputeShader(bloom_upsample_shader_string, NULL);

    g_material_builtin_shaders.bloom_downsample_screen_shader_id
      = chugl_createShader(g_chuglAPI, bloom_downsample_screen_shader,
                           bloom_downsample_screen_shader, NULL, NULL, NULL, 0);

    g_material_builtin_shaders.bloom_upsample_screen_shader_id
      = chugl_createShader(g_chuglAPI, bloom_upsample_screen_shader,
                           bloom_upsample_screen_shader, NULL, NULL, NULL, 0);

    // pbr material
    g_material_builtin_shaders.pbr_shader_id = chugl_createShader(
      g_chuglAPI, pbr_shader_string, pbr_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout), NULL, NULL, true);

    // uv material
    g_material_builtin_shaders.uv_shader_id = chugl_createShader(
      g_chuglAPI, uv_shader_string, uv_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout), NULL, NULL, false);

    // normal material
    g_material_builtin_shaders.normal_shader_id = chugl_createShader(
      g_chuglAPI, normal_shader_string, normal_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout), NULL, NULL, false);

    // tangent material
    g_material_builtin_shaders.tangent_shader_id = chugl_createShader(
      g_chuglAPI, tangent_shader_string, tangent_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout), NULL, NULL, false);

    // phong
    g_material_builtin_shaders.phong_shader_id = chugl_createShader(
      g_chuglAPI, phong_shader_string, phong_shader_string, NULL, NULL,
      standard_vertex_layout, ARRAY_LENGTH(standard_vertex_layout), NULL, NULL, true);

    // points
    g_material_builtin_shaders.points_shader_id = chugl_createShader(
      g_chuglAPI, points_shader_string, points_shader_string, NULL, NULL, NULL, 0);
}
