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

// PP_Effect --> PP_Output
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_pp_output_ctor);

// TODO: auto-gen UI and CK DLL QUERY based on shadercode annotation (the shader is the source of truth anyways)

CK_DLL_MFUN(chugl_pp_output_set_gamma);
CK_DLL_MFUN(chugl_pp_output_get_gamma);

//-----------------------------------------------------------------------------
// PP class declarations
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_pp_effect(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_passthrough(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_output(Chuck_DL_Query *QUERY);

//-----------------------------------------------------------------------------
// init_chugl_postprocess() query
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_postprocess(Chuck_DL_Query *QUERY)
{
    
    if (!init_chugl_pp_effect(QUERY)) return FALSE;
    if (!init_chugl_pp_passthrough(QUERY)) return FALSE;
    if (!init_chugl_pp_output(QUERY)) return FALSE;

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

    // TODO add param get/set

    QUERY->add_mfun(QUERY, chugl_pp_output_set_gamma, "float", "gamma");
    QUERY->add_arg(QUERY, "float", "gamma");
    QUERY->doc_func(QUERY, 
        "Set value for gamma correction."
        "Before writing to screen, all pixel colors are raised to the power of 1/gamma."
    );

    QUERY->add_mfun(QUERY, chugl_pp_output_get_gamma, "float", "gamma");
    QUERY->doc_func(QUERY, "Get value for gamma correction.");

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

    auto uniform = MaterialUniform::CreateFloat(OutputEffect::U_GAMMA, gamma);


    CGL::PushCommand(
        new UpdateEffectUniformCommand(
            effect, uniform
        )
    );
}

CK_DLL_MFUN(chugl_pp_output_get_gamma)
{
    OutputEffect* effect = (OutputEffect*) OBJ_MEMBER_INT(SELF, CGL::GetPPEffectDataOffset());
    RETURN->v_float = effect->GetGamma();
}