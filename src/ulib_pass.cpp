#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_PASS(ckobj) SG_GetPass(OBJ_MEMBER_UINT(ckobj, component_offset_id))

// external API
SG_ID ulib_pass_createPass(SG_PassType pass_type);
SG_ID ulib_pass_createOutputPass();

SG_Material* chugl_createInternalMaterial(SG_MaterialType material_type,
                                          SG_Shader* shader)
{
    // internal materials are not connected to chuck objects
    SG_Material* material                  = SG_CreateMaterial(NULL, material_type);
    material->pso.exclude_from_render_pass = true;
    SG_Material::shader(material, shader);

    CQ_PushCommand_MaterialCreate(material);

    return material;
}

// Pass
CK_DLL_CTOR(pass_ctor);
CK_DLL_MFUN(pass_get_next);
CK_DLL_GFUN(pass_op_gruck);   // add pass
CK_DLL_GFUN(pass_op_ungruck); // remove pass

// RenderPass
CK_DLL_CTOR(renderpass_ctor);
CK_DLL_MFUN(renderpass_set_resolve_target);
CK_DLL_MFUN(renderpass_get_resolve_target);

CK_DLL_MFUN(renderpass_set_color_target_clear_on_load);
CK_DLL_MFUN(renderpass_get_color_target_clear_on_load);

// TODO get_resolve_target
// TODO set/get camera and scene
// - enforce camera is part of scene
// TODO add RenderPassColorAttachment.loadOp / storeOp / clearValue (clearValue in
// GScene)
// TODO add set/get HDR?
// TODO add set/get MSAA sample count?
// TODO scissor + viewport (after ScreenPass, not sure if we apply scissor/viewport to
// RenderPass or screenpass...)

// ScenePass
CK_DLL_CTOR(screenpass_ctor);
CK_DLL_CTOR(screenpass_ctor_with_params);
CK_DLL_MFUN(screenpass_set_target);
CK_DLL_MFUN(screenpass_set_shader);
// TODO get_target

// OutputPass : ScreenPass
CK_DLL_CTOR(outputpass_ctor);
CK_DLL_MFUN(outputpass_set_input_texture);
CK_DLL_MFUN(outputpass_set_tonemap);
CK_DLL_MFUN(outputpass_get_tonemap);
CK_DLL_MFUN(outputpass_set_gamma);
CK_DLL_MFUN(outputpass_get_gamma);
CK_DLL_MFUN(outputpass_set_exposure);
CK_DLL_MFUN(outputpass_get_exposure);

// ComputePass
CK_DLL_CTOR(computepass_ctor); // don't send creation CQ Command until shader is set
CK_DLL_MFUN(computepass_set_shader);
CK_DLL_MFUN(computepass_set_uniform_float);
CK_DLL_MFUN(computepass_set_storage_buffer);
CK_DLL_MFUN(computepass_set_uniform_int);
CK_DLL_MFUN(computepass_set_uniform_float2);
CK_DLL_MFUN(computepass_set_workgroup);

// BloomPass
CK_DLL_CTOR(bloompass_ctor); // don't send creation CQ Command until shader is set
CK_DLL_MFUN(bloompass_set_input_render_texture);
CK_DLL_MFUN(bloompass_get_output_render_texture);
CK_DLL_MFUN(bloompass_set_internal_blend);
CK_DLL_MFUN(bloompass_set_final_blend);
CK_DLL_MFUN(bloompass_get_internal_blend);
CK_DLL_MFUN(bloompass_get_final_blend);
CK_DLL_MFUN(bloompass_set_num_levels);
CK_DLL_MFUN(bloompass_get_num_levels);
CK_DLL_MFUN(bloompass_set_threshold);
CK_DLL_MFUN(bloompass_get_threshold);

// bloom pass params
// SG_ID bloom_downscale_material_id;
// SG_ID bloom_upscale_material_id;
// SG_ID bloom_input_render_texture_id;

/*
==optimize==
currently hijacking the ChuGL material system to update screenpass shader params
- we implicitly create a material every time a shader is set on the ScreenPass
- this material generates an R_Pipeline which is NOT used for rendering,
and will be a dud in R_RenderScene when iterating over pipelines
- but you shouldn't have many ScreenPasses anyways, accepting the cost for now
*/

// Other
// WindowTexture

const char* ulib_pass_classname(SG_PassType pass_type)
{
    switch (pass_type) {
        case SG_PassType_Root: return SG_CKNames[SG_COMPONENT_PASS];
        case SG_PassType_Render: return "RenderPass";
        case SG_PassType_Compute: return "ComputePass";
        case SG_PassType_Screen: return "ScreenPass";
        default: ASSERT(false);
    }
    return NULL;
}

void ulib_pass_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_PASS], SG_CKNames[SG_COMPONENT_BASE]);

    CTOR(pass_ctor);

    MFUN(pass_get_next, SG_CKNames[SG_COMPONENT_PASS], "next");
    DOC_FUNC("Get the GPass this is connected to");

    QUERY->add_op_overload_binary(QUERY, pass_op_gruck, SG_CKNames[SG_COMPONENT_PASS],
                                  "-->", SG_CKNames[SG_COMPONENT_PASS], "lhs",
                                  SG_CKNames[SG_COMPONENT_PASS], "rhs");

    // overload GGen --< GGen
    QUERY->add_op_overload_binary(QUERY, pass_op_ungruck, SG_CKNames[SG_COMPONENT_PASS],
                                  "--<", SG_CKNames[SG_COMPONENT_PASS], "lhs",
                                  SG_CKNames[SG_COMPONENT_PASS], "rhs");
    END_CLASS();

    // RenderPass --------------------------------------------------------------
    {

        BEGIN_CLASS(ulib_pass_classname(SG_PassType_Render),
                    SG_CKNames[SG_COMPONENT_PASS]);
        DOC_CLASS(
          " Render pass for drawing a GScene. If RenderPass.scene() is not set, will "
          "default to the main scene, GG.scene()"
          " If RenderPass.target() is not set, will default to the screen. "
          " If RenderPass.camera() is not set, will default to the scene's main "
          "camera: "
          "GG.scene().camera()");

        CTOR(renderpass_ctor);

        MFUN(renderpass_set_resolve_target, "void", "target");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "target");
        DOC_FUNC("Set the target texture to draw the scene to.");

        MFUN(renderpass_get_resolve_target, SG_CKNames[SG_COMPONENT_TEXTURE], "target");
        DOC_FUNC("Get the target texture to draw the scene to.");

        MFUN(renderpass_set_color_target_clear_on_load, "void", "autoClearColor");
        ARG("int", "clear");
        DOC_FUNC(
          "Set whether the framebuffer's color target should be cleared each "
          "frame. Default true.");

        MFUN(renderpass_get_color_target_clear_on_load, "int", "autoClearColor");
        DOC_FUNC("Get whether the framebuffer's color target is cleared each frame");

        END_CLASS();
    }

    // ScreenPass ----------------------------------------------------------------
    {
        BEGIN_CLASS(ulib_pass_classname(SG_PassType_Screen),
                    SG_CKNames[SG_COMPONENT_PASS]);

        CTOR(screenpass_ctor);

        CTOR(screenpass_ctor_with_params);
        ARG(SG_CKNames[SG_COMPONENT_SHADER], "screen_shader");

        MFUN(screenpass_set_target, "void", "target");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "target");

        MFUN(screenpass_set_shader, "void", "shader");
        ARG(SG_CKNames[SG_COMPONENT_SHADER], "shader");

        END_CLASS();
    }

    // OutputPass ----------------------------------------------------------------
    {
        BEGIN_CLASS("OutputPass", SG_CKNames[SG_COMPONENT_PASS]);

        CTOR(outputpass_ctor);

        MFUN(outputpass_set_input_texture, "void", "input");
        ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "input_texture");

        MFUN(outputpass_set_tonemap, "void", "tonemap");
        ARG("int", "tonemap_type");

        MFUN(outputpass_get_tonemap, "int", "tonemap");

        // Note: removing gamma correction, swapchain output view is already srgb
        // MFUN(outputpass_set_gamma, "void", "gamma");
        // ARG("float", "gamma");

        // MFUN(outputpass_get_gamma, "float", "gamma");

        MFUN(outputpass_set_exposure, "void", "exposure");
        ARG("float", "exposure");

        MFUN(outputpass_get_exposure, "float", "exposure");

        END_CLASS();
    }

    // ComputePass --------------------------------------------------------------
    {
        BEGIN_CLASS("ComputePass", SG_CKNames[SG_COMPONENT_PASS]);

        CTOR(computepass_ctor);

        MFUN(computepass_set_shader, "void", "shader");
        ARG(SG_CKNames[SG_COMPONENT_SHADER], "shader");

        MFUN(computepass_set_uniform_float, "void", "uniformFloat");
        ARG("int", "location");
        ARG("float", "uniform_value");

        MFUN(computepass_set_uniform_float2, "void", "uniformFloat2");
        ARG("int", "location");
        ARG("vec2", "uniform_value");

        MFUN(computepass_set_storage_buffer, "void", "storageBuffer");
        ARG("int", "location");
        ARG("StorageBuffer", "buffer");

        MFUN(computepass_set_uniform_int, "void", "uniformInt");
        ARG("int", "location");
        ARG("int", "uniform_value");

        MFUN(computepass_set_workgroup, "void", "workgroup");
        ARG("int", "x");
        ARG("int", "y");
        ARG("int", "z");

        END_CLASS();

        { // BloomPass
            BEGIN_CLASS("BloomPass", SG_CKNames[SG_COMPONENT_PASS]);

            CTOR(bloompass_ctor);

            MFUN(bloompass_set_input_render_texture, "void", "input");
            ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "bloom_texture");
            DOC_FUNC("Set the render texture to apply bloom to");

            MFUN(bloompass_get_output_render_texture, SG_CKNames[SG_COMPONENT_TEXTURE],
                 "output");
            DOC_FUNC("Get the render texture that the bloom pass writes to");

            MFUN(bloompass_set_internal_blend, "void", "radius");
            ARG("float", "blend_factor");
            DOC_FUNC(
              "Set the blend factor between mip levels of the bloom texture during "
              "upsample");

            MFUN(bloompass_set_final_blend, "void", "intensity");
            ARG("float", "blend_factor");
            DOC_FUNC(
              "Set the blend factor between the bloom texture and the original image");

            MFUN(bloompass_get_internal_blend, "float", "radius");
            DOC_FUNC("Get the blend factor between mip levels of the bloom texture");

            MFUN(bloompass_get_final_blend, "float", "intensity");
            DOC_FUNC(
              "Get the blend factor between the bloom texture and the original image");

            MFUN(bloompass_set_num_levels, "void", "levels");
            ARG("int", "num_levels");
            DOC_FUNC(
              "Number of blur passes to apply to the bloom texture. "
              "Clamped between 0 and 16.");

            MFUN(bloompass_get_num_levels, "int", "levels");
            DOC_FUNC("Get the number of blur passes applied to the bloom texture.");

            MFUN(bloompass_set_threshold, "void", "threshold");
            ARG("float", "threshold");
            DOC_FUNC(
              "Set the threshold for the bloom pass (colors with all rgb values below "
              "threshold are not bloomed)");

            MFUN(bloompass_get_threshold, "float", "threshold");
            DOC_FUNC("Get the threshold for the bloom pass");

            END_CLASS();
        }
    }
}

SG_ID ulib_pass_createPass(SG_PassType pass_type)
{
    CK_DL_API API            = g_chuglAPI;
    Chuck_Object* pass_ckobj = chugin_createCkObj(ulib_pass_classname(pass_type), true);

    SG_Pass* pass = SG_CreatePass(pass_ckobj, pass_type);
    ASSERT(pass->pass_type == pass_type);

    OBJ_MEMBER_UINT(pass_ckobj, component_offset_id) = pass->id;

    CQ_PushCommand_PassUpdate(pass);

    return pass->id;
}

CK_DLL_CTOR(pass_ctor)
{
    // don't allow instantiation of abstract base class
    if (chugin_typeEquals(SELF, SG_CKNames[SG_COMPONENT_PASS])) {
        CK_THROW(
          "InvalidClassInstantiation",
          "GPass is an abstract base class, do not instantiate directly. Use one of "
          "the children classes e.g. RenderPass / ComputePass / ScreenPass instead",
          SHRED);
    }
}

CK_DLL_MFUN(pass_get_next)
{
    SG_Pass* pass      = GET_PASS(SELF);
    SG_Pass* next_pass = SG_GetPass(pass->next_pass_id);

    RETURN->v_object = next_pass ? next_pass->ckobj : NULL;
}

CK_DLL_GFUN(pass_op_gruck)
{
    // get the arguments
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

    if (!lhs || !rhs) {
        std::string errMsg = std::string("in gruck operator: ")
                             + (lhs ? "LHS" : "[null]") + " --> "
                             + (rhs ? "RHS" : "[null]");
        // nullptr exception
        API->vm->throw_exception("NullPointerException", errMsg.c_str(), SHRED);
        return;
    }

    // get internal representation
    SG_Pass* lhs_pass = SG_GetPass(OBJ_MEMBER_UINT(lhs, component_offset_id));
    SG_Pass* rhs_pass = SG_GetPass(OBJ_MEMBER_UINT(rhs, component_offset_id));

    if (!SG_Pass::connect(lhs_pass, rhs_pass)) {
        CK_LOG(CK_LOG_WARNING,
               "warning GPass --> GPass failed! Cannot connect NULL passes, cannot "
               "form cycles in the GPass chain");
    }

    // command
    CQ_PushCommand_PassUpdate(lhs_pass);

    // return RHS
    RETURN->v_object = rhs;
}

CK_DLL_GFUN(pass_op_ungruck)
{
    // get the arguments
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

    // get internal representation
    SG_Pass* lhs_pass = SG_GetPass(OBJ_MEMBER_UINT(lhs, component_offset_id));
    SG_Pass* rhs_pass = SG_GetPass(OBJ_MEMBER_UINT(rhs, component_offset_id));

    SG_Pass::disconnect(lhs_pass, rhs_pass);

    // command
    CQ_PushCommand_PassUpdate(lhs_pass);

    // return RHS
    RETURN->v_object = rhs;
}

// ============================================================================
// RenderPass
// ============================================================================

CK_DLL_CTOR(renderpass_ctor)
{
    SG_Pass* pass = SG_CreatePass(SELF, SG_PassType_Render);
    ASSERT(pass->type == SG_COMPONENT_PASS);
    ASSERT(pass->pass_type == SG_PassType_Render);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(renderpass_set_resolve_target)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Render);

    Chuck_Object* target = GET_NEXT_OBJECT(ARGS);
    SG_Texture* texture
      = target ? SG_GetTexture(OBJ_MEMBER_UINT(target, component_offset_id)) : NULL;
    SG_Pass::resolveTarget(pass, texture);

    // command TODO
    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(renderpass_get_resolve_target)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Render);

    SG_Texture* texture = SG_GetTexture(pass->resolve_target_id);
    RETURN->v_object    = texture ? texture->ckobj : NULL;
}

CK_DLL_MFUN(renderpass_set_color_target_clear_on_load)
{
    GET_PASS(SELF)->color_target_clear_on_load = GET_NEXT_INT(ARGS);
}

CK_DLL_MFUN(renderpass_get_color_target_clear_on_load)
{
    RETURN->v_int = GET_PASS(SELF)->color_target_clear_on_load;
}

// ============================================================================
// ScreenPass
// ============================================================================

CK_DLL_CTOR(screenpass_ctor)
{
    SG_Pass* pass = SG_CreatePass(SELF, SG_PassType_Screen);
    ASSERT(pass->type == SG_COMPONENT_PASS);
    ASSERT(pass->pass_type == SG_PassType_Screen);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_CTOR(screenpass_ctor_with_params)
{
    // get the arguments
    Chuck_Object* screen_shader = GET_NEXT_OBJECT(ARGS);
    SG_Shader* shader
      = screen_shader ?
          SG_GetShader(OBJ_MEMBER_UINT(screen_shader, component_offset_id)) :
          NULL;

    SG_Pass* pass = SG_CreatePass(SELF, SG_PassType_Screen);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    SG_Material* mat = chugl_createInternalMaterial(SG_MATERIAL_CUSTOM, shader);

    SG_Pass::screenShader(pass, mat, shader);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(screenpass_set_target)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Screen);
    SG_Texture* target
      = SG_GetTexture(OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), component_offset_id));

    SG_Pass::screenTexture(pass, target);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(screenpass_set_shader)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Screen);
    SG_Shader* shader
      = SG_GetShader(OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), component_offset_id));

    // for now we don't really care about destroying the previous mat...
    // ==optimize== free the previous material
    SG_Material* mat = chugl_createInternalMaterial(SG_MATERIAL_CUSTOM, shader);

    SG_Pass::screenShader(pass, mat, shader);

    CQ_PushCommand_PassUpdate(pass);
}

// ============================================================================
// OutputPass
// ============================================================================

SG_Pass* ulib_pass_createOutputPass(Chuck_Object* ckobj)
{
    CK_DL_API API = g_chuglAPI;

    SG_Pass* pass = SG_CreatePass(ckobj, SG_PassType_Screen);
    ASSERT(pass->type == SG_COMPONENT_PASS);
    ASSERT(pass->pass_type == SG_PassType_Screen);
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = pass->id;

    SG_Shader* output_pass_shader
      = SG_GetShader(g_material_builtin_shaders.output_pass_shader_id);

    SG_Material* mat
      = chugl_createInternalMaterial(SG_MATERIAL_CUSTOM, output_pass_shader);

    // set output_pass uniforms
    SG_Material::setTexture(mat, 0, SG_GetTexture(g_builtin_textures.white_pixel_id));
    CQ_PushCommand_MaterialSetUniform(mat, 0);

    SG_Material::setSampler(mat, 1, SG_SAMPLER_DEFAULT); // sampler
    CQ_PushCommand_MaterialSetUniform(mat, 1);

    // locking gamma to 1.0 (no gamma correction)
    // because we enforce swapchain output view to be srgb, which applies gamma for us.
    SG_Material::uniformFloat(mat, 2, 1.0); // gamma
    CQ_PushCommand_MaterialSetUniform(mat, 2);

    SG_Material::uniformFloat(mat, 3, 1.0); // exposure
    CQ_PushCommand_MaterialSetUniform(mat, 3);

    // SG_Material::uniformInt(mat, 4, 4); // TONEMAP_ACES
    SG_Material::uniformInt(mat, 4, 5); // TONEMAP_UNCHARTED
    CQ_PushCommand_MaterialSetUniform(mat, 4);

    // push pass through CQ
    SG_Pass::screenShader(pass, mat, output_pass_shader);
    CQ_PushCommand_PassUpdate(pass);

    return pass;
}

CK_DLL_CTOR(outputpass_ctor)
{
    ulib_pass_createOutputPass(SELF);
}

CK_DLL_MFUN(outputpass_set_input_texture)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Screen);

    SG_Texture* input_texture
      = SG_GetTexture(OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), component_offset_id));
    if (!input_texture) return;

    SG_Material* material = SG_GetMaterial(pass->screen_material_id);
    ASSERT(material);

    // set uniform
    SG_Material::setTexture(material, 0, input_texture);
    CQ_PushCommand_MaterialSetUniform(material, 0);
}

CK_DLL_MFUN(outputpass_set_tonemap)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Screen);

    t_CKINT tonemap_type = GET_NEXT_INT(ARGS);

    SG_Material* material = SG_GetMaterial(pass->screen_material_id);
    ASSERT(material);

    // set uniform
    SG_Material::uniformInt(material, 4, tonemap_type);
    CQ_PushCommand_MaterialSetUniform(material, 4);
}

CK_DLL_MFUN(outputpass_get_tonemap)
{
    SG_Pass* pass         = GET_PASS(SELF);
    SG_Material* material = SG_GetMaterial(pass->screen_material_id);
    RETURN->v_int         = material->uniforms[4].as.i;
}

CK_DLL_MFUN(outputpass_set_gamma)
{
    SG_Pass* pass         = GET_PASS(SELF);
    t_CKFLOAT gamma       = GET_NEXT_FLOAT(ARGS);
    SG_Material* material = SG_GetMaterial(pass->screen_material_id);

    // set uniform
    // TODO can use CODE(...) macro to replace 2 with OUTPUT_PASS_GAMMA_BINDING
    SG_Material::uniformFloat(material, 2, gamma);
    CQ_PushCommand_MaterialSetUniform(material, 2);
}

CK_DLL_MFUN(outputpass_get_gamma)
{
    SG_Pass* pass         = GET_PASS(SELF);
    SG_Material* material = SG_GetMaterial(pass->screen_material_id);
    RETURN->v_float       = material->uniforms[2].as.f;
}

CK_DLL_MFUN(outputpass_set_exposure)
{
    SG_Pass* pass         = GET_PASS(SELF);
    t_CKFLOAT exposure    = GET_NEXT_FLOAT(ARGS);
    SG_Material* material = SG_GetMaterial(pass->screen_material_id);

    // set uniform
    SG_Material::uniformFloat(material, 3, exposure);
    CQ_PushCommand_MaterialSetUniform(material, 3);
}

CK_DLL_MFUN(outputpass_get_exposure)
{
    SG_Pass* pass         = GET_PASS(SELF);
    SG_Material* material = SG_GetMaterial(pass->screen_material_id);
    RETURN->v_float       = material->uniforms[3].as.f;
}

// ============================================================================
// ComputePass
// ============================================================================

CK_DLL_CTOR(computepass_ctor)
{
    SG_Pass* pass = SG_CreatePass(SELF, SG_PassType_Compute);
    ASSERT(pass->type == SG_COMPONENT_PASS);
    ASSERT(pass->pass_type == SG_PassType_Compute);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, NULL);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(computepass_set_shader)
{
    SG_Pass* pass = GET_PASS(SELF);
    SG_Shader* shader
      = SG_GetShader(OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), component_offset_id));

    SG_Material* mat = (pass->compute_material_id == 0) ?
                         chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, shader) :
                         SG_GetMaterial(pass->compute_material_id);

    SG_Pass::computeShader(pass, mat, shader);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(computepass_set_uniform_float)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Compute);

    t_CKINT location        = GET_NEXT_INT(ARGS);
    t_CKFLOAT uniform_value = GET_NEXT_FLOAT(ARGS);

    SG_Material* material = (pass->compute_material_id == 0) ?
                              chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, NULL) :
                              SG_GetMaterial(pass->compute_material_id);

    // set uniform
    SG_Material::uniformFloat(material, location, uniform_value);
    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(computepass_set_uniform_float2)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Compute);

    t_CKINT location       = GET_NEXT_INT(ARGS);
    t_CKVEC2 uniform_value = GET_NEXT_VEC2(ARGS);

    SG_Material* material = (pass->compute_material_id == 0) ?
                              chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, NULL) :
                              SG_GetMaterial(pass->compute_material_id);

    // set uniform
    SG_Material::uniformVec2f(material, location, { uniform_value.x, uniform_value.y });
    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(computepass_set_storage_buffer)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Compute);

    t_CKINT location     = GET_NEXT_INT(ARGS);
    Chuck_Object* buffer = GET_NEXT_OBJECT(ARGS);

    SG_Material* material = (pass->compute_material_id == 0) ?
                              chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, NULL) :
                              SG_GetMaterial(pass->compute_material_id);

    SG_Buffer* sg_buffer = SG_GetBuffer(OBJ_MEMBER_UINT(buffer, component_offset_id));

    // set storage buffer
    SG_Material::storageBuffer(material, location, sg_buffer);
    CQ_PushCommand_MaterialSetUniform(material, location);
}

CK_DLL_MFUN(computepass_set_workgroup)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Compute);

    t_CKINT x = GET_NEXT_INT(ARGS);
    t_CKINT y = GET_NEXT_INT(ARGS);
    t_CKINT z = GET_NEXT_INT(ARGS);

    SG_Pass::workgroupSize(pass, x, y, z);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(computepass_set_uniform_int)
{
    SG_Pass* pass = GET_PASS(SELF);
    ASSERT(pass->pass_type == SG_PassType_Compute);

    t_CKINT location      = GET_NEXT_INT(ARGS);
    t_CKINT uniform_value = GET_NEXT_INT(ARGS);

    SG_Material* material = (pass->compute_material_id == 0) ?
                              chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, NULL) :
                              SG_GetMaterial(pass->compute_material_id);

    // set uniform
    SG_Material::uniformInt(material, location, uniform_value);
    CQ_PushCommand_MaterialSetUniform(material, location);
}

// ============================================================================
// BloomPass
// ============================================================================

CK_DLL_CTOR(bloompass_ctor)
{
    SG_Pass* pass                              = SG_CreatePass(SELF, SG_PassType_Bloom);
    OBJ_MEMBER_UINT(SELF, component_offset_id) = pass->id;

    SG_Shader* bloom_downsample_shader
      = SG_GetShader(g_material_builtin_shaders.bloom_downsample_screen_shader_id);
    SG_Shader* bloom_upsample_shader
      = SG_GetShader(g_material_builtin_shaders.bloom_upsample_screen_shader_id);

    // create default output render texture
    SG_TextureDesc output_render_texture_desc = {};
    output_render_texture_desc.usage = WGPUTextureUsage_RenderAttachment
                                             | WGPUTextureUsage_TextureBinding
                                             | WGPUTextureUsage_StorageBinding;
    output_render_texture_desc.format = WGPUTextureFormat_RGBA16Float;
    SG_Texture* output_render_texture
      = SG_CreateTexture(&output_render_texture_desc, NULL, SHRED, false);

    SG_Material* bloom_downsample_mat
      = chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, bloom_downsample_shader);
    SG_Material* bloom_upsample_mat
      = chugl_createInternalMaterial(SG_MATERIAL_COMPUTE, bloom_upsample_shader);

    // initialize uniforms for downsample mat
    SG_Material::uniformFloat(bloom_downsample_mat, 2, 0.0); // threshold
    CQ_PushCommand_MaterialSetUniform(bloom_downsample_mat, 2);

    // initialize uniforms for upsample mat
    SG_Material::uniformFloat(bloom_upsample_mat, 4, 0.85); // internal blend
    CQ_PushCommand_MaterialSetUniform(bloom_upsample_mat, 4);
    SG_Material::uniformFloat(bloom_upsample_mat, 5, 0.2); // final blend
    CQ_PushCommand_MaterialSetUniform(bloom_upsample_mat, 5);

    // update pass
    pass->bloom_downsample_material_id = bloom_downsample_mat->id;
    pass->bloom_upsample_material_id   = bloom_upsample_mat->id;
    SG_Pass::bloomOutputRenderTexture(pass, output_render_texture);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(bloompass_set_input_render_texture)
{
    SG_Pass* pass = GET_PASS(SELF);

    SG_Texture* bloom_texture
      = SG_GetTexture(OBJ_MEMBER_UINT(GET_NEXT_OBJECT(ARGS), component_offset_id));

    SG_Pass::bloomInputRenderTexture(pass, bloom_texture);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(bloompass_get_output_render_texture)
{
    SG_Pass* pass = GET_PASS(SELF);

    SG_Texture* bloom_texture = SG_GetTexture(pass->bloom_output_render_texture_id);

    RETURN->v_object = bloom_texture ? bloom_texture->ckobj : NULL;
}

CK_DLL_MFUN(bloompass_set_internal_blend)
{
    SG_Pass* pass   = GET_PASS(SELF);
    t_CKFLOAT blend = GET_NEXT_FLOAT(ARGS);

    SG_Material* bloom_upsample_material
      = SG_GetMaterial(pass->bloom_upsample_material_id);

    SG_Material::uniformFloat(bloom_upsample_material, 4, blend);
    CQ_PushCommand_MaterialSetUniform(bloom_upsample_material, 4);
}

CK_DLL_MFUN(bloompass_set_final_blend)
{
    SG_Pass* pass   = GET_PASS(SELF);
    t_CKFLOAT blend = GET_NEXT_FLOAT(ARGS);

    SG_Material* bloom_upsample_material
      = SG_GetMaterial(pass->bloom_upsample_material_id);

    SG_Material::uniformFloat(bloom_upsample_material, 5, blend);
    CQ_PushCommand_MaterialSetUniform(bloom_upsample_material, 5);
}

CK_DLL_MFUN(bloompass_get_internal_blend)
{
    SG_Pass* pass = GET_PASS(SELF);

    SG_Material* bloom_upsample_material
      = SG_GetMaterial(pass->bloom_upsample_material_id);

    RETURN->v_float = bloom_upsample_material->uniforms[4].as.f;
}

CK_DLL_MFUN(bloompass_get_final_blend)
{
    SG_Pass* pass = GET_PASS(SELF);

    SG_Material* bloom_upsample_material
      = SG_GetMaterial(pass->bloom_upsample_material_id);

    RETURN->v_float = bloom_upsample_material->uniforms[5].as.f;
}

CK_DLL_MFUN(bloompass_set_num_levels)
{
    SG_Pass* pass               = GET_PASS(SELF);
    pass->bloom_num_blur_levels = GET_NEXT_INT(ARGS);
    CLAMP(pass->bloom_num_blur_levels, 1, 16);

    CQ_PushCommand_PassUpdate(pass);
}

CK_DLL_MFUN(bloompass_get_num_levels)
{
    SG_Pass* pass = GET_PASS(SELF);
    RETURN->v_int = pass->bloom_num_blur_levels;
}

CK_DLL_MFUN(bloompass_set_threshold)
{
    SG_Pass* pass       = GET_PASS(SELF);
    t_CKFLOAT threshold = GET_NEXT_FLOAT(ARGS);

    SG_Material* bloom_downsample_material
      = SG_GetMaterial(pass->bloom_downsample_material_id);

    SG_Material::uniformFloat(bloom_downsample_material, 2, threshold);
    CQ_PushCommand_MaterialSetUniform(bloom_downsample_material, 2);
}

CK_DLL_MFUN(bloompass_get_threshold)
{
    SG_Pass* pass = GET_PASS(SELF);
    SG_Material* bloom_downsample_material
      = SG_GetMaterial(pass->bloom_downsample_material_id);

    RETURN->v_float = bloom_downsample_material->uniforms[2].as.f;
}
