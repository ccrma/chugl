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

CK_DLL_MFUN(chugl_text_set_font);
CK_DLL_MFUN(chugl_text_get_font);

CK_DLL_MFUN(chugl_text_set_color);
CK_DLL_MFUN(chugl_text_get_color);

CK_DLL_MFUN(chugl_text_set_control_points);
CK_DLL_MFUN(chugl_text_get_control_points);

CK_DLL_MFUN(chugl_text_set_line_spacing);
CK_DLL_MFUN(chugl_text_get_line_spacing);


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

    QUERY->add_mfun(QUERY, chugl_text_set_font, "string", "font");
    QUERY->add_arg(QUERY, "string", "font");
    QUERY->doc_func(QUERY, 
        "set path to a font file (supported types: .otf and .ttf)"
        "If not provided, will default to the font set via GG.font()"
    );

    QUERY->add_mfun(QUERY, chugl_text_get_font, "string", "font");
    QUERY->doc_func(QUERY, "get font path");

    QUERY->add_mfun(QUERY, chugl_text_set_color, "vec3", "color");
    QUERY->add_arg(QUERY, "vec3", "color");
    QUERY->doc_func(QUERY, "set text color. supports HDR values");

    QUERY->add_mfun(QUERY, chugl_text_get_color, "vec3", "color");
    QUERY->doc_func(QUERY, "get text color");

    QUERY->add_mfun(QUERY, chugl_text_set_control_points, "vec2", "controlPoints");
    QUERY->add_arg(QUERY, "vec2", "controlPoints");
    QUERY->doc_func(QUERY, 
        "set control points for text. x = horizontal, y = vertical"
        "The control point is a ratio within the text's bounding box that determines where its origin is"
        "For example, (0.5, 0.5) will place the origin at the center the text"
        "(0.0, 0.0) will place the origin at the top-left of the text"
        "(1.0, 1.0) will place the origin at the bottom-right of the text"
    );

    QUERY->add_mfun(QUERY, chugl_text_get_control_points, "vec2", "controlPoints");
    QUERY->doc_func(QUERY, "get control points");

    QUERY->add_mfun(QUERY, chugl_text_set_line_spacing, "float", "lineSpacing");
    QUERY->add_arg(QUERY, "float", "lineSpacing");
    QUERY->doc_func(QUERY, "set vertical line spacing. Default is 1.0");

    QUERY->add_mfun(QUERY, chugl_text_get_line_spacing, "float", "lineSpacing");
    QUERY->doc_func(QUERY, "get vertical line spacing");

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

CK_DLL_MFUN(chugl_text_set_font)
{
    auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);

    Chuck_String* ck_font = GET_NEXT_STRING(ARGS);
    std::string font = API->object->str(ck_font);

    if (font != gtext->GetFontPath()) {
        gtext->SetFontPath(font);
        CGL::PushCommand(new UpdateGTextCommand(gtext));
    }

    RETURN->v_string = ck_font;
}

CK_DLL_MFUN(chugl_text_get_font)
{
    auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);
    RETURN->v_string = (Chuck_String *)API->object->create_string(
        VM, gtext->GetFontPath().c_str(), false
    );
}

CK_DLL_MFUN(chugl_text_set_color)
{
    auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);

    t_CKVEC3 ck_color = GET_NEXT_VEC3(ARGS);

    gtext->SetColor(glm::vec4(ck_color.x, ck_color.y, ck_color.z, 1.0f));
    CGL::PushCommand(new UpdateGTextCommand(gtext));

    RETURN->v_vec3 = ck_color;
}

CK_DLL_MFUN(chugl_text_get_color)
{
    auto* gtext = dynamic_cast<CHGL_Text*>(CGL::GetSGO(SELF));
    assert(gtext);

    glm::vec4 color = gtext->GetColor();
    RETURN->v_vec3 = {color.x, color.y, color.z};
}

CK_DLL_MFUN(chugl_text_set_control_points)
{
    auto *gtext = dynamic_cast<CHGL_Text *>(CGL::GetSGO(SELF));
    assert(gtext);

    t_CKVEC2 ck_controlPoints = GET_NEXT_VEC2(ARGS);

    gtext->SetControlPoints(glm::vec2(ck_controlPoints.x, ck_controlPoints.y));
    CGL::PushCommand(new UpdateGTextCommand(gtext));

    RETURN->v_vec2 = ck_controlPoints;
}

CK_DLL_MFUN(chugl_text_get_control_points)
{
    auto *gtext = dynamic_cast<CHGL_Text *>(CGL::GetSGO(SELF));
    assert(gtext);

    glm::vec2 controlPoints = gtext->GetControlPoints();
    RETURN->v_vec2 = {controlPoints.x, controlPoints.y};
}

CK_DLL_MFUN(chugl_text_set_line_spacing)
{
    auto *gtext = dynamic_cast<CHGL_Text *>(CGL::GetSGO(SELF));
    assert(gtext);

    t_CKFLOAT ck_lineSpacing = GET_NEXT_FLOAT(ARGS);

    gtext->SetLineSpacing(ck_lineSpacing);
    CGL::PushCommand(new UpdateGTextCommand(gtext));

    RETURN->v_float = ck_lineSpacing;
}

CK_DLL_MFUN(chugl_text_get_line_spacing)
{
    auto *gtext = dynamic_cast<CHGL_Text *>(CGL::GetSGO(SELF));
    assert(gtext);

    RETURN->v_float = gtext->GetLineSpacing();
}