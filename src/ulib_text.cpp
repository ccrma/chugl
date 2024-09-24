#include "ulib_helper.h"

#include "sg_command.h"
#include "sg_component.h"

#include "shaders.h"

#define GET_TEXT(ckobj) SG_GetText(OBJ_MEMBER_UINT(ckobj, component_offset_id));

CK_DLL_SFUN(gtext_set_default_font);

CK_DLL_CTOR(gtext_ctor);

CK_DLL_MFUN(gtext_set_color);
CK_DLL_MFUN(gtext_get_color);

CK_DLL_MFUN(gtext_set_text);
CK_DLL_MFUN(gtext_get_text);

CK_DLL_MFUN(gtext_set_font);
CK_DLL_MFUN(gtext_get_font);

CK_DLL_MFUN(gtext_set_vertical_spacing);
CK_DLL_MFUN(gtext_get_vertical_spacing);

CK_DLL_MFUN(gtext_set_control_points);
CK_DLL_MFUN(gtext_get_control_points);

CK_DLL_MFUN(gtext_set_texture);
CK_DLL_MFUN(gtext_get_texture);

void ulib_text_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_TEXT], SG_CKNames[SG_COMPONENT_TRANSFORM]);
    DOC_CLASS(
      "Builtin fonts are \"chugl:cousine-regular\", \"chugl:karla-regular\", "
      "\"chugl:proggy-tiny\", \"chugl:proggy-clean\" which can be assigned to "
      "GText.font(string)");

    SFUN(gtext_set_default_font, "void", "defaultFont");
    ARG("string", "default_font");
    DOC_FUNC("Set default font file to be used by all GText not given a font path");

    CTOR(gtext_ctor);

    MFUN(gtext_set_color, "void", "color");
    ARG("vec4", "color");

    MFUN(gtext_get_color, "vec4", "color");

    MFUN(gtext_set_text, "void", "text");
    ARG("string", "text");
    DOC_FUNC("Set text");

    MFUN(gtext_get_text, "string", "text");
    DOC_FUNC("Get text");

    MFUN(gtext_set_font, "void", "font");
    ARG("string", "font");
    DOC_FUNC(
      "Set path to a font file (supported types: .otf and .ttf). If not provided, will "
      "default to the font set via GText.defaultFont(). See top of class doc for list "
      "of builtin font names");

    MFUN(gtext_get_font, "string", "font");
    DOC_FUNC("Get path to font file");

    MFUN(gtext_set_vertical_spacing, "void", "spacing");
    ARG("float", "spacing");
    DOC_FUNC("Set vertical line spacing. Default is 1.0.");

    MFUN(gtext_get_vertical_spacing, "float", "spacing");

    MFUN(gtext_set_control_points, "void", "controlPoints");
    ARG("vec2", "control_points");
    DOC_FUNC(
      "Set control points for text. x = horizontal, y = verticalThe control point is a "
      "ratio within the text's bounding box that determines where its origin isFor "
      "example, (0.5, 0.5) will place the origin at the center the text. (0.0, 0.0) "
      "will "
      "place the origin at the bottom-left of the text. (1.0, 1.0) will place the "
      "origin at "
      "the top-right of the text.");

    MFUN(gtext_get_control_points, "vec2", "controlPoints");

    MFUN(gtext_set_texture, "void", "texture");
    ARG(SG_CKNames[SG_COMPONENT_TEXTURE], "texture");
    DOC_FUNC("Set a texture to be applied to the text block. Default 1 white pixel");

    MFUN(gtext_get_texture, SG_CKNames[SG_COMPONENT_TEXTURE], "texture");
    DOC_FUNC("Get the current texture applied to the text block");

    END_CLASS();
}

CK_DLL_SFUN(gtext_set_default_font)
{
    CQ_PushCommand_TextDefaultFont(API->object->str(GET_NEXT_STRING(ARGS)));
}

CK_DLL_CTOR(gtext_ctor)
{
    // IDEA: extend GMesh?

    SG_Text* text = SG_CreateText(SELF);
    ASSERT(text->type == SG_COMPONENT_TEXT);

    OBJ_MEMBER_UINT(SELF, component_offset_id) = text->id;

    text->text = "hello ChuGL";

    // create gtext material
    Chuck_Object* material_ckobj
      = chugin_createCkObj(SG_CKNames[SG_COMPONENT_MATERIAL], true);
    SG_Material* material = SG_CreateMaterial(material_ckobj, SG_MATERIAL_TEXT3D);
    OBJ_MEMBER_UINT(material_ckobj, component_offset_id) = material->id;
    CQ_PushCommand_MaterialCreate(material);

    // assign material to text
    SG_Mesh::setMaterial(text, material);

    // get shader
    SG_Shader* gtext_shader = SG_GetShader(g_material_builtin_shaders.gtext_shader_id);

    // set shader
    chugl_materialSetShader(material, gtext_shader);

    // initialize default uniforms
    SG_Material::uniformVec4f(material, 2, glm::vec4(1.0f)); // color
    SG_Material::uniformFloat(material, 3, 1.0);             // antialiasing window
    SG_Material::uniformInt(material, 4, 1);                 // enable ssaa
    CQ_PushCommand_MaterialSetUniform(material, 2);
    CQ_PushCommand_MaterialSetUniform(material, 3);
    CQ_PushCommand_MaterialSetUniform(material, 4);

    SG_Texture* tex = SG_GetTexture(g_builtin_textures.white_pixel_id);
    SG_Material::setTexture(material, 6, tex);
    CQ_PushCommand_MaterialSetUniform(material, 6);

    SG_Material::setSampler(material, 7, SG_SAMPLER_DEFAULT);
    CQ_PushCommand_MaterialSetUniform(material, 7);

    CQ_PushCommand_TextRebuild(text);
}

CK_DLL_MFUN(gtext_set_color)
{
    SG_Text* text         = GET_TEXT(SELF);
    SG_Material* material = SG_GetMaterial(text->_mat_id);
    t_CKVEC4 color        = GET_NEXT_VEC4(ARGS);

    SG_Material::uniformVec4f(material, 2, { color.x, color.y, color.z, color.w });
    CQ_PushCommand_MaterialSetUniform(material, 2);
}

CK_DLL_MFUN(gtext_get_color)
{
    SG_Text* text         = GET_TEXT(SELF);
    SG_Material* material = SG_GetMaterial(text->_mat_id);
    glm::vec4 color       = material->uniforms[2].as.vec4f;
    RETURN->v_vec4        = { color.r, color.g, color.b, color.a };
}

CK_DLL_MFUN(gtext_set_text)
{
    SG_Text* text = GET_TEXT(SELF);
    text->text    = API->object->str(GET_NEXT_STRING(ARGS));

    CQ_PushCommand_TextRebuild(text);
}

CK_DLL_MFUN(gtext_get_text)
{
    SG_Text* text    = GET_TEXT(SELF);
    RETURN->v_string = chugin_createCkString(text->text.c_str());
}

CK_DLL_MFUN(gtext_set_font)
{
    SG_Text* text   = GET_TEXT(SELF);
    text->font_path = API->object->str(GET_NEXT_STRING(ARGS));

    CQ_PushCommand_TextRebuild(text);
}

CK_DLL_MFUN(gtext_get_font)
{
    SG_Text* text    = GET_TEXT(SELF);
    RETURN->v_string = chugin_createCkString(text->font_path.c_str());
}

CK_DLL_MFUN(gtext_set_vertical_spacing)
{
    SG_Text* text          = GET_TEXT(SELF);
    text->vertical_spacing = GET_NEXT_FLOAT(ARGS);

    CQ_PushCommand_TextRebuild(text);
}

CK_DLL_MFUN(gtext_get_vertical_spacing)
{
    SG_Text* text   = GET_TEXT(SELF);
    RETURN->v_float = text->vertical_spacing;
}

CK_DLL_MFUN(gtext_set_control_points)
{
    SG_Text* text        = GET_TEXT(SELF);
    text->control_points = GET_NEXT_VEC2(ARGS);

    CQ_PushCommand_TextRebuild(text);
}

CK_DLL_MFUN(gtext_get_control_points)
{
    SG_Text* text  = GET_TEXT(SELF);
    RETURN->v_vec2 = text->control_points;
}

CK_DLL_MFUN(gtext_set_texture)
{
    SG_Text* text         = GET_TEXT(SELF);
    SG_Material* material = SG_GetMaterial(text->_mat_id);
    Chuck_Object* ckobj   = GET_NEXT_OBJECT(ARGS);
    SG_Texture* tex       = SG_GetTexture(OBJ_MEMBER_UINT(ckobj, component_offset_id));

    SG_Material::setTexture(material, 6, tex);

    CQ_PushCommand_MaterialSetUniform(material, 6);
}

CK_DLL_MFUN(gtext_get_texture)
{
    SG_Text* text         = GET_TEXT(SELF);
    SG_Material* material = SG_GetMaterial(text->_mat_id);
    SG_Texture* texture   = SG_GetTexture(material->uniforms[6].as.texture_id);
    RETURN->v_object      = texture->ckobj;
}
