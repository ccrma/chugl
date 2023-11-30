#include "ulib_text.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "scenegraph/chugl_text.h"


//-----------------------------------------------------------------------------
// Object --> GText 
//-----------------------------------------------------------------------------
CK_DLL_CTOR(chugl_text_ctor);

CK_DLL_MFUN(chugl_text_set_text);
CK_DLL_MFUN(chugl_text_get_text);


t_CKBOOL init_chugl_text(Chuck_DL_Query *QUERY)
{
    QUERY->begin_class(QUERY, CHGL_Text::CKName(), "GGen");

    QUERY->add_ctor(QUERY, chugl_text_ctor);
    // no destructor needed, can piggy-back off of current GGen garbage collection

    QUERY->add_mfun(QUERY, chugl_text_set_text, "string", "text");
    QUERY->add_arg(QUERY, "string", "text");
    QUERY->doc_func(QUERY, "set text");

    QUERY->add_mfun(QUERY, chugl_text_get_text, "string", "text");
    QUERY->doc_func(QUERY, "get text");

    QUERY->end_class(QUERY);

    return TRUE;
}

CK_DLL_CTOR(chugl_text_ctor)
{
	CGL::PushCommand(
        new CreateSceneGraphNodeCommand(new CHGL_Text, &CGL::mainScene, SELF, CGL::GetGGenDataOffset(), API)
    );
}

CK_DLL_MFUN(chugl_text_set_text)
{
	auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);

	Chuck_String* ck_text = GET_NEXT_STRING(ARGS);
    std::string text = API->object->str(ck_text);

    if (text != gtext->GetText()) {
        gtext->SetText(text);
        CGL::PushCommand(new UpdateGTextCommand(gtext));
    }

	RETURN->v_string = ck_text;
}

CK_DLL_MFUN(chugl_text_get_text)
{
	auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);
	RETURN->v_string = (Chuck_String *)API->object->create_string(
		VM, gtext->GetText().c_str(), false
	);
}