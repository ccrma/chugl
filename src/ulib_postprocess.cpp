#include "ulib_postprocess.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "scenegraph/chugl_postprocess.h"

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

//-----------------------------------------------------------------------------
// PP_Effect --> PP_Root
//-----------------------------------------------------------------------------
// CK_DLL_CTOR(cgl_pp_root_ctor);

//-----------------------------------------------------------------------------
// PP_Effect --> PP_PassThrough
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_pp_passthrough_ctor);

//-----------------------------------------------------------------------------
// PP class declarations
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_pp_effect(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_pp_passthrough(Chuck_DL_Query *QUERY);

//-----------------------------------------------------------------------------
// init_chugl_postprocess() query
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_postprocess(Chuck_DL_Query *QUERY)
{
    
    if (!init_chugl_pp_effect(QUERY)) return FALSE;
    if (!init_chugl_pp_passthrough(QUERY)) return FALSE;

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