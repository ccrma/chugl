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

CK_DLL_GFUN(chugl_pp_op_gruck);	  // add next effect to chain


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
// PP_Effect --> PP_Invert 
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_pp_invert_ctor);

CK_DLL_MFUN(chugl_pp_invert_get_mix);
CK_DLL_MFUN(chugl_pp_invert_set_mix);

//-----------------------------------------------------------------------------
// PP_Effect --> MonochromeFX 
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_pp_monochrome_ctor);

CK_DLL_MFUN(chugl_pp_monochrome_get_mix);
CK_DLL_MFUN(chugl_pp_monochrome_set_mix);
CK_DLL_MFUN(chugl_pp_monochrome_get_color);
CK_DLL_MFUN(chugl_pp_monochrome_set_color);


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
CK_DLL_MFUN(chugl_pp_bloom_set_threshold);
CK_DLL_MFUN(chugl_pp_bloom_get_threshold);

// hard threshold looks good enough
// CK_DLL_MFUN(chugl_pp_bloom_set_threshold_knee);
// CK_DLL_MFUN(chugl_pp_bloom_get_threshold_knee);

CK_DLL_MFUN(chugl_pp_bloom_set_levels);
CK_DLL_MFUN(chugl_pp_bloom_get_levels);

CK_DLL_MFUN(chugl_pp_bloom_set_blend_mode);
CK_DLL_MFUN(chugl_pp_bloom_get_blend_mode);

CK_DLL_MFUN(chugl_pp_bloom_set_karis_enabled);
CK_DLL_MFUN(chugl_pp_bloom_get_karis_enabled);

//-----------------------------------------------------------------------------
// PP class declarations
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_pp_effect(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_passthrough(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_invert(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_monochrome(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_output(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_bloom(Chuck_DL_Query *QUERY);

//-----------------------------------------------------------------------------
// init_chugl_postprocess() query
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_postprocess(Chuck_DL_Query *QUERY)
{
    
    if (!init_chugl_pp_effect(QUERY)) return FALSE;
    if (!init_chugl_pp_passthrough(QUERY)) return FALSE;
    if (!init_chugl_pp_invert(QUERY)) return FALSE;
    if (!init_chugl_pp_monochrome(QUERY)) return FALSE;
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


	QUERY->add_op_overload_binary(QUERY, chugl_pp_op_gruck, Effect::CKName(Type::Base), "-->",
								  Effect::CKName(Type::Base), "lhs", Effect::CKName(Type::Base), "rhs");

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

CK_DLL_GFUN(chugl_pp_op_gruck)
{
    // get args
    Chuck_Object* lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* rhs = GET_NEXT_OBJECT(ARGS);

	if (!lhs || !rhs) {
		std::string errMsg = std::string("in gruck operator: ") + (lhs?"LHS":"[null]") + " --> " + (rhs?"RHS":"[null]");
		// nullptr exception
		API->vm->throw_exception(
			"NullPointerException",
			errMsg.c_str(),
			SHRED
		);
		return;
	}

    // get effect objects
    Effect* lhsEffect = (Effect*) OBJ_MEMBER_INT(lhs, CGL::GetPPEffectDataOffset());
    Effect* rhsEffect = (Effect*) OBJ_MEMBER_INT(rhs, CGL::GetPPEffectDataOffset());

    // add rhs to lhs
    CGL::PushCommand(new AddEffectCommand(lhsEffect, rhsEffect));

    RETURN->v_object = rhs;
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
// Passthrough
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
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset(), API
        )
    );
}

//-----------------------------------------------------------------------------
// Invert 
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_invert(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::Invert), Effect::CKName(Type::Base));
    QUERY->doc_class(QUERY, 
        "An effect that inverts the colors of the input texture. inverseColor = 1.0 - inputColor"
        "Place this AFTER the Output pass, when Tonemapping has already been applied and colors have been mapped into the range [0, 1]."
    );

    QUERY->add_ctor(QUERY, chugl_pp_invert_ctor);

    QUERY->add_mfun(QUERY, chugl_pp_invert_get_mix, "float", "mix");
    QUERY->doc_func(QUERY, "Get the mix factor for the invert effect.");

    QUERY->add_mfun(QUERY, chugl_pp_invert_set_mix, "float", "mix");
    QUERY->add_arg(QUERY, "float", "mix");
    QUERY->doc_func(QUERY, 
        "Set the mix factor for the invert effect."
        "0 = no invert, 1 = full invert. Interpolates between input texture and inverted texture."
    );

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_pp_invert_ctor)
{
    CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new InverseEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset(), API
        )
    );
}

CK_DLL_MFUN(chugl_pp_invert_get_mix)
{
    InverseEffect* effect = (InverseEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetMix();
}

CK_DLL_MFUN(chugl_pp_invert_set_mix)
{
    InverseEffect* effect = (InverseEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float mix = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = mix;

    CGL::PushCommand(
        new UpdateEffectUniformCommand(
            effect, MaterialUniform::CreateFloat(InverseEffect::U_MIX, mix)
        )
    );
}

//-----------------------------------------------------------------------------
// Monochrome 
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_pp_monochrome(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, Effect::CKName(Type::Monochrome), Effect::CKName(Type::Base));
    QUERY->doc_class(QUERY, 
        "An effect that converts colors of the input texture to grayscale, and then multiplies by a color."
        "Can be placed before or after the Output pass."
    );

    QUERY->add_ctor(QUERY, chugl_pp_monochrome_ctor);

    QUERY->add_mfun(QUERY, chugl_pp_monochrome_get_mix, "float", "mix");
    QUERY->doc_func(QUERY, "Get the mix factor for the monochrome effect.");

    QUERY->add_mfun(QUERY, chugl_pp_monochrome_set_mix, "float", "mix");
    QUERY->add_arg(QUERY, "float", "mix");
    QUERY->doc_func(QUERY, 
        "Set the mix factor for the effect."
        "0 = no monochrome, 1 = full monochrome. Interpolates between input texture and monochrome texture."
    );

    QUERY->add_mfun(QUERY, chugl_pp_monochrome_get_color, "vec3", "color");
    QUERY->doc_func(QUERY, "Get the color for the monochrome effect.");

    QUERY->add_mfun(QUERY, chugl_pp_monochrome_set_color, "vec3", "color");
    QUERY->add_arg(QUERY, "vec3", "color");
    QUERY->doc_func(QUERY, 
        "Set the color for the effect."
        "The color will be multiplied by the grayscale value of the input texture."
    );

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_pp_monochrome_ctor)
{
    CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new MonoChromeEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset(), API
        )
    );
}

CK_DLL_MFUN(chugl_pp_monochrome_get_mix)
{
    MonoChromeEffect* effect = (MonoChromeEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetMix();
}

CK_DLL_MFUN(chugl_pp_monochrome_set_mix)
{
    MonoChromeEffect* effect = (MonoChromeEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float mix = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = mix;

    CGL::PushCommand(
        new UpdateEffectUniformCommand(
            effect, MaterialUniform::CreateFloat(MonoChromeEffect::U_MIX, mix)
        )
    );
}

CK_DLL_MFUN(chugl_pp_monochrome_get_color)
{
    MonoChromeEffect* effect = (MonoChromeEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    auto& color = effect->GetColor();
    RETURN->v_vec3 = {color.r, color.g, color.b};
}

CK_DLL_MFUN(chugl_pp_monochrome_set_color)
{
    MonoChromeEffect* effect = (MonoChromeEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    t_CKVEC3 color = GET_NEXT_VEC3(ARGS);

    RETURN->v_vec3 = color;

    CGL::PushCommand(
        new UpdateEffectUniformCommand(
            effect, MaterialUniform::CreateFloat3(MonoChromeEffect::U_COLOR, color.x, color.y, color.z)
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
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset(), API
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

    // svars ===================================================================
    QUERY->add_svar(QUERY, "int", "BLEND_ADD", TRUE, (void*) &BloomEffect::BLEND_ADD);
    QUERY->doc_var(QUERY, "Additive blending mode. Bloom texture is multiplied by strength and added to input texture.");

    QUERY->add_svar(QUERY, "int", "BLEND_MIX", TRUE, (void*) &BloomEffect::BLEND_MIX);
    QUERY->doc_var(QUERY, "Mix blending mode. Bloom texture is interpolated with input texture by strength.");

    // mfuns ===================================================================
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

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_threshold, "float", "threshold");
    QUERY->doc_func(QUERY, "Get the bloom threshold.");

    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_threshold, "float", "threshold");
    QUERY->add_arg(QUERY, "float", "threshold");
    QUERY->doc_func(QUERY, "Threshold for bloom effect. Pixels with brightness below this value will not be bloomed.");

    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_levels, "int", "levels");
    QUERY->add_arg(QUERY, "int", "numLevels");
    QUERY->doc_func(QUERY, 
        "Number of blur passes to apply to the bloom texture."
        "Clamped between 1 and 16."
    );

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_levels, "int", "levels");
    QUERY->doc_func(QUERY, "Get the number of blur passes applied to the bloom texture.");

    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_blend_mode, "int", "blend");
    QUERY->add_arg(QUERY, "int", "blendMode");
    QUERY->doc_func(QUERY, 
        "Set the blend mode for the bloom effect."
        "Use one of the static constants: ADD or MIX"
    );

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_blend_mode, "int", "blend");
    QUERY->doc_func(QUERY, "Get the blend mode for the bloom effect.");


    QUERY->add_mfun(QUERY, chugl_pp_bloom_set_karis_enabled, "int", "karisAverage");
    QUERY->add_arg(QUERY, "int", "karisEnabled");
    QUERY->doc_func(QUERY, 
        "Enable or disable Karis averaging"
        "Reduces flickering artifacts (aka fireflies) caused by overly bright subpixels"
    );

    QUERY->add_mfun(QUERY, chugl_pp_bloom_get_karis_enabled, "int", "karisAverage");
    QUERY->doc_func(QUERY, "Get the Karis averaging flag.");

    // QUERY->add_mfun(QUERY, chugl_pp_bloom_get_threshold_knee, "float", "thresholdKnee");
    // QUERY->doc_func(QUERY, "Get the bloom threshold knee.");

    // QUERY->add_mfun(QUERY, chugl_pp_bloom_set_threshold_knee, "float", "thresholdKnee");
    // QUERY->add_arg(QUERY, "float", "thresholdKnee");
    // QUERY->doc_func(QUERY, 
    //     "Threshold knee for bloom effect. Controls how suddenly pixels near the threshold are bloomed."
    //     "See: https://catlikecoding.com/unity/tutorials/custom-srp/post-processing/"
    //     "This value should be between 0 and 1."
    // );

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_pp_bloom_ctor)
{
    CGL::PushCommand(
        new CreateSceneGraphNodeCommand(
            new BloomEffect,
            &CGL::mainScene, SELF, CGL::GetPPEffectDataOffset(), API
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

CK_DLL_MFUN(chugl_pp_bloom_set_threshold)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    float threshold = GET_NEXT_FLOAT(ARGS);

    RETURN->v_float = threshold;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateFloat(BloomEffect::U_THRESHOLD, threshold) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_bloom_get_threshold)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetThreshold();
}

CK_DLL_MFUN(chugl_pp_bloom_set_levels)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    int levels = GET_NEXT_INT(ARGS);

    RETURN->v_int = levels;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateInt(BloomEffect::U_LEVELS, levels) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_bloom_get_levels)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_int = effect->GetLevels();
}

CK_DLL_MFUN(chugl_pp_bloom_set_blend_mode)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    int blendMode = GET_NEXT_INT(ARGS);

    RETURN->v_int = blendMode;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand( 
            effect, MaterialUniform::CreateInt(BloomEffect::U_BLEND_MODE, blendMode) 
        ) 
    );
}

CK_DLL_MFUN(chugl_pp_bloom_get_blend_mode)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_int = effect->GetBlendMode();
}

CK_DLL_MFUN(chugl_pp_bloom_set_karis_enabled)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    int karis= GET_NEXT_INT(ARGS);

    RETURN->v_int = karis;

    CGL::PushCommand( 
        new UpdateEffectUniformCommand(
            effect, MaterialUniform::CreateBool(BloomEffect::U_KARIS_ENABLED, karis ? true : false) 
        ) 
    );

}

CK_DLL_MFUN(chugl_pp_bloom_get_karis_enabled)
{
    BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_int = effect->GetKarisEnabled() ? 1 : 0;
}

// CK_DLL_MFUN(chugl_pp_bloom_set_threshold_knee)
// {
//     BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
//     float thresholdKnee = GET_NEXT_FLOAT(ARGS);

//     RETURN->v_float = thresholdKnee;

//     CGL::PushCommand( 
//         new UpdateEffectUniformCommand( 
//             effect, MaterialUniform::CreateFloat(BloomEffect::U_THRESHOLD_KNEE, thresholdKnee) 
//         ) 
//     );
// }

// CK_DLL_MFUN(chugl_pp_bloom_get_threshold_knee)
// {
//     BloomEffect* effect = (BloomEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
//     RETURN->v_float = effect->GetThresholdKnee();
// }