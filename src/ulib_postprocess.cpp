#include "ulib_postprocess.h"
#include "ulib_cgl.h"
#include "ulib_gui.h"
#include "scenegraph/Command.h"
#include "scenegraph/chugl_postprocess.h"
#include "scenegraph/Locator.h"

using namespace PP;

//-----------------------------------------------------------------------------
// PP Effect (base class for all post process effects)
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_pp_effect_ctor);
CK_DLL_DTOR(cgl_pp_effect_dtor);

CK_DLL_MFUN(cgl_pp_effect_set_next);
CK_DLL_MFUN(cgl_pp_effect_get_next);
CK_DLL_MFUN(cgl_pp_effect_remove_next);

CK_DLL_MFUN(cgl_pp_effect_set_bypass);
CK_DLL_MFUN(cgl_pp_effect_get_bypass);

// CK_DLL_MFUN(cgl_pp_effect_create_gui);

//-----------------------------------------------------------------------------
// PP_Effect --> PP_Root
//-----------------------------------------------------------------------------
// CK_DLL_CTOR(cgl_pp_root_ctor);

//-----------------------------------------------------------------------------
// PP_Effect --> PP_PassThrough
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_pp_passthrough_ctor);

//-----------------------------------------------------------------------------
// PP_Effect --> PP_Output
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_pp_output_ctor);

// TODO: auto-gen UI and CK DLL QUERY based on shadercode annotation (the shader is the source of truth anyways)

CK_DLL_MFUN(chugl_pp_output_set_gamma);
CK_DLL_MFUN(chugl_pp_output_get_gamma);

CK_DLL_MFUN(chugl_pp_output_set_tonemap_method);
CK_DLL_MFUN(chugl_pp_output_get_tonemap_method);

CK_DLL_MFUN(chugl_pp_output_set_exposure);
CK_DLL_MFUN(chugl_pp_output_get_exposure);

//-----------------------------------------------------------------------------
// PP_Effect --> Bloom 
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_pp_bloom_ctor);

CK_DLL_MFUN(chugl_pp_bloom_get_strength);
CK_DLL_MFUN(chugl_pp_bloom_set_strength);
CK_DLL_MFUN(chugl_pp_bloom_get_radius);
CK_DLL_MFUN(chugl_pp_bloom_set_radius);


//-----------------------------------------------------------------------------
// PP class declarations
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_pp_effect(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_passthrough(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_output(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_bloom(Chuck_DL_Query *QUERY);

//-----------------------------------------------------------------------------
// init_chugl_postprocess() query
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_postprocess(Chuck_DL_Query *QUERY)
{
    
    if (!init_chugl_pp_effect(QUERY)) return FALSE;
    if (!init_chugl_pp_passthrough(QUERY)) return FALSE;
    if (!init_chugl_pp_output(QUERY)) return FALSE;
    if (!init_chugl_pp_bloom(QUERY)) return FALSE;

    return TRUE;
}

//-----------------------------------------------------------------------------
// PP_Effect
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_effect(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::Base), "Object");

    QUERY->add_ctor(QUERY, cgl_pp_effect_ctor);
    QUERY->add_dtor(QUERY, cgl_pp_effect_dtor);

    QUERY->add_mfun(QUERY, cgl_pp_effect_set_next, Effect::CKName(Type::Base), "next");
    QUERY->add_arg(QUERY, Effect::CKName(Type::Base), "next");
    QUERY->doc_func(QUERY, "Set the next effect in the chain");

    QUERY->add_mfun(QUERY, cgl_pp_effect_get_next, Effect::CKName(Type::Base), "next");
    QUERY->doc_func(QUERY, "Get the next effect in the chain");

    QUERY->add_mfun(QUERY, cgl_pp_effect_remove_next, Effect::CKName(Type::Base), "removeNext");
    QUERY->doc_func(QUERY, "Remove the next effect in the chain. Returns the removed effect.");

    QUERY->add_mfun(QUERY, cgl_pp_effect_set_bypass, "int", "bypass");
    QUERY->add_arg(QUERY, "int", "flag");
    QUERY->doc_func(QUERY, 
        "Set the bypass flag. If true then this effect will be skipped in the effects chain"
        "and the next effect will be used instead."
    );

    QUERY->add_mfun(QUERY, cgl_pp_effect_get_bypass, "int", "bypass");
    QUERY->doc_func(QUERY, "Get the bypass flag");

    // Custom GUI Element
    // QUERY->add_mfun(QUERY, cgl_pp_effect_create_gui, GUI::Manager::GetCkName(GUI::Type::Element), "UI");
    // QUERY->doc_func(QUERY, "Creates a custom GUI element to control this effect's parameters.");


    // data offset
    CGL::SetPPEffectDataOffset(QUERY->add_mvar(QUERY, "int", "@pp_effect_data", false));

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(cgl_pp_effect_ctor)
{
    // no constructor for abstract base class
}

CK_DLL_DTOR(cgl_pp_effect_dtor)
{
    // TODO: implement refcounting
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset()) = 0;
}

CK_DLL_MFUN(cgl_pp_effect_set_next)
{
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    Effect* next = (Effect*) OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), CGL::GetPPEffectDataOffset());

    RETURN->v_object = next ? next->m_ChuckObject : nullptr;

    CGL::PushCommand(new AddEffectCommand(effect, next));
}

CK_DLL_MFUN(cgl_pp_effect_get_next)
{
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    Effect* next = effect->Next();
    RETURN->v_object = next ? next->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(cgl_pp_effect_remove_next)
{
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    Effect* next = effect->Next();

    RETURN->v_object = next ? next->m_ChuckObject : nullptr;

    CGL::PushCommand(new RemoveEffectCommand(effect));
}

CK_DLL_MFUN(cgl_pp_effect_set_bypass)
{
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    t_CKINT bypass = GET_NEXT_INT(ARGS);

    RETURN->v_int = bypass ? 1 : 0;

    CGL::PushCommand(new BypassEffectCommand(effect, bypass));
}

CK_DLL_MFUN(cgl_pp_effect_get_bypass)
{
    Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_int = effect->GetBypass() ? 1 : 0;
}

// Bug: doesn't work for multiple effects past the 1st in chain for some reason
// CK_DLL_MFUN(cgl_pp_effect_create_gui)
// {
//     Effect* effect = (Effect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
//     size_t effectID = effect->GetID();

//     std::function<void(GUI::Element*, bool&)> lambda;  

//     switch (effect->GetType()) {
//     case Type::Base:
//     case Type::PassThrough:
//     case Type::Output:
//         lambda = [effectID](GUI::Element* gui, bool& bypass) {
//             // early out if effect no longer exists
//             // (currently the GUI Draw() is only ever called by the render thread)
//             // And the render thread is solely responsible for garbage collecting its own data,
//             // including PP effects
//             // Therefore checking that the effect exists at this point in time is sufficient
//             // (the effect will not be freed while the GUI is being drawn)
//             auto* effect = Locator::GetNode<PP::Effect>(effectID, false); // NOT the audio thread
//             if (!effect) return;
//             // draw label
//             ImGui::SeparatorText(gui->GetLabel().c_str());
//             // draw bypass checkbox
//             // if (ImGui::Checkbox("Bypass", &bypass)) {
//             //     effect->SetBypass(bypass);
//             // }
//             ImGui::Checkbox("Bypass", effect->GetBypassPtr());
//         };
//         break;
//     default:
//         throw std::runtime_error("Unknown PP::Type");
//     }

//     // create chuck-side GUI element
//     Chuck_DL_Api::Type gui_type = API->type->lookup(VM, GUI::Manager::GetCkName(GUI::Type::Element));
//     Chuck_Object* ck_obj = API->object->create(SHRED, gui_type, false);

//     // create GUI element impl
//     auto* gui = new GUI::Custom<bool>(ck_obj, lambda, false);

//     // set chuck-side pointer to GUI element impl
// 	OBJ_MEMBER_INT(ck_obj, CGL::GetGUIDataOffset()) = (t_CKINT) gui;

//     RETURN->v_object = ck_obj;
// }

//-----------------------------------------------------------------------------
// PP_Effect
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_passthrough(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::PassThrough), Effect::CKName(Type::Base));
    QUERY->doc_class(QUERY, 
        "A pass-through effect. This effect does nothing and simply passes the input texture to the output framebuffer."
        "Intended for internal use and testing"
    );


    QUERY->add_ctor(QUERY, cgl_pp_passthrough_ctor);

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(cgl_pp_passthrough_ctor)
{
	CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new PassThroughEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset()
        )
    );
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_output(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::Output), Effect::CKName(Type::Base));
    QUERY->doc_class(QUERY, 
        "The output effect, which applies gamma correction and tonemapping."
        "If used, this effect should always be the last effect in the chain."
    );

    QUERY->add_ctor(QUERY, chugl_pp_output_ctor);

    QUERY->add_svar(QUERY, "int", "TONEMAP_NONE", TRUE, (void*) &OutputEffect::TONEMAP_NONE);
    QUERY->doc_var(QUERY, "No tonemapping. HDR color values will be clamped to [0, 1].");

    QUERY->add_svar(QUERY, "int", "TONEMAP_LINEAR", TRUE, (void*) &OutputEffect::TONEMAP_LINEAR);
    QUERY->doc_var(QUERY,
        "Linear tonemapping. HDR color values will be clamped to [0, 1] after being scaled by exposure"
    );

    QUERY->add_svar(QUERY, "int", "TONEMAP_REINHARD", TRUE, (void*) &OutputEffect::TONEMAP_REINHARD);
    QUERY->doc_var(QUERY,
        "Reinhard tonemapping. HDR color values will be mapped with the formula (HDR / (HDR + 1)) * exposure" 
    );

    QUERY->add_svar(QUERY, "int", "TONEMAP_CINEON", TRUE, (void*) &OutputEffect::TONEMAP_CINEON);
    QUERY->doc_var(QUERY,
        "Cineon tonemapping. Source: http://filmicworlds.com/blog/filmic-tonemapping-operators/"
    );

    QUERY->add_svar(QUERY, "int", "TONEMAP_ACES", TRUE, (void*) &OutputEffect::TONEMAP_ACES);
    QUERY->doc_var(QUERY,
        "Tone mapping approach developed by the Academy Color Encoding System (ACES)."
        "Gilves filmic quality"
    );

    QUERY->add_svar(QUERY, "int", "TONEMAP_UNCHARTED", TRUE, (void*) &OutputEffect::TONEMAP_UNCHARTED);
    QUERY->doc_var(QUERY,
        "Tone mapping used by Uncharted 2."
        "Source: http://filmicworlds.com/blog/filmic-tonemapping-operators/"
    );


    // TODO add param get/set

    QUERY->add_mfun(QUERY, chugl_pp_output_set_gamma, "float", "gamma");
    QUERY->add_arg(QUERY, "float", "gamma");
    QUERY->doc_func(QUERY, 
        "Set value for gamma correction."
        "Before writing to screen, all pixel colors are raised to the power of 1/gamma."
    );

    QUERY->add_mfun(QUERY, chugl_pp_output_get_gamma, "float", "gamma");
    QUERY->doc_func(QUERY, "Get value for gamma correction.");

    QUERY->add_mfun(QUERY, chugl_pp_output_set_tonemap_method, "int", "toneMap");
    QUERY->add_arg(QUERY, "int", "method");
    QUERY->doc_func(QUERY, 
        "Set the tonemapping method."
        "0 = none, 1 = Reinhard"
    );

    QUERY->add_mfun(QUERY, chugl_pp_output_get_tonemap_method, "int", "toneMap");
    QUERY->doc_func(QUERY, "Get the tonemapping method.");

    QUERY->add_mfun(QUERY, chugl_pp_output_set_exposure, "float", "exposure");
    QUERY->add_arg(QUERY, "float", "exposure");
    QUERY->doc_func(QUERY, 
        "Set the exposure value. Multiplies the HDR color values before tonemapping."
    );

    QUERY->add_mfun(QUERY, chugl_pp_output_get_exposure, "float", "exposure");
    QUERY->doc_func(QUERY, "Get the exposure value.");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_pp_output_ctor)
{
	CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new OutputEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset()
        )
    );
}

CK_DLL_MFUN(chugl_pp_output_set_gamma)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float gamma = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = gamma;



    CGL::PushCommand(
        new UpdateEffectUniformCommand(
            effect, MaterialUniform::CreateFloat(OutputEffect::U_GAMMA, gamma)
        )
    );
}

CK_DLL_MFUN(chugl_pp_output_get_gamma)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetGamma();
}

CK_DLL_MFUN(chugl_pp_output_set_tonemap_method)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    int method = GET_NEXT_INT(ARGS);

    RETURN->v_int = method;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateInt(OutputEffect::U_TONEMAP, method) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_output_get_tonemap_method)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_int = effect->GetTonemap();
}

CK_DLL_MFUN(chugl_pp_output_set_exposure)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float exposure = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = exposure;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateFloat(OutputEffect::U_EXPOSURE, exposure) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_output_get_exposure)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetExposure();
}

//-----------------------------------------------------------------------------
// Bloom 
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_bloom(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::Bloom), Effect::CKName(Type::Base));
    QUERY->doc_class(QUERY, 
        "Physically-based Bloom effect. Combines input texture with a blurred version of itself to create a glow effect."
        "Recommended to use with Output effect, for gamma correction and tone mapping."
    );

    QUERY->add_ctor(QUERY, chugl_pp_bloom_ctor);

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_strength, "float", "strength");
    QUERY->doc_func(QUERY, "Get the bloom strength.");

    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_strength, "float", "strength");
    QUERY->add_arg(QUERY, "float", "strength");
    QUERY->doc_func(
        QUERY, 
        "Set the bloom strength."
        "If the bloom blending mode is additive, this is the multiplier for the bloom texture."
        "If the bloom blending mode is mix, this is the interpolation factor between the input texture and the bloom texture."
    );

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_radius, "float", "radius");
    QUERY->doc_func(QUERY, "Get the bloom filter radius.");

    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_radius, "float", "radius");
    QUERY->add_arg(QUERY, "float", "radius");
    QUERY->doc_func(QUERY, "Radius of filter kernel during bloom blur pass.");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_pp_bloom_ctor)
{
    CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new BloomEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset()
        )
    );
}

CK_DLL_MFUN(chugl_pp_bloom_get_strength)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetStrength();
}

CK_DLL_MFUN(chugl_pp_bloom_set_strength)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float strength = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = strength ;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateFloat(BloomEffect::U_STRENGTH, strength) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_bloom_get_radius)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetRadius();
}

CK_DLL_MFUN(chugl_pp_bloom_set_radius)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float radius = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = radius;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateFloat(BloomEffect::U_RADIUS, radius) 
        ) 
    );
}