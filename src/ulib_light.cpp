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

// TODO add ambient to scene

CK_DLL_CTOR(ulib_light_ctor);
CK_DLL_CTOR(ulib_light_ctor_with_type);

CK_DLL_MFUN(ulib_light_set_type);
CK_DLL_MFUN(ulib_light_get_type);

CK_DLL_MFUN(ulib_light_set_color);
CK_DLL_MFUN(ulib_light_get_color);

CK_DLL_MFUN(ulib_light_set_intensity);
CK_DLL_MFUN(ulib_light_get_intensity);

CK_DLL_CTOR(ulib_dir_light_ctor);

CK_DLL_CTOR(ulib_point_light_ctor);
CK_DLL_MFUN(ulib_point_light_get_radius);
CK_DLL_MFUN(ulib_point_light_set_radius);
CK_DLL_MFUN(ulib_point_light_get_falloff_exponent);
CK_DLL_MFUN(ulib_point_light_set_falloff_exponent);

#define GET_LIGHT(ckobj) SG_GetLight(OBJ_MEMBER_UINT(ckobj, component_offset_id))

SG_Light* ulib_light_create(Chuck_Object* ckobj, SG_LightType type)
{
    CK_DL_API API = g_chuglAPI;

    // execute change on audio thread side
    SG_Light* light  = SG_CreateLight(ckobj);
    light->desc.type = type;
    // save SG_ID
    OBJ_MEMBER_UINT(ckobj, component_offset_id) = light->id;

    CQ_PushCommand_LightUpdate(light);
    return light;
}

static void ulib_light_query(Chuck_DL_Query* QUERY)
{
    BEGIN_CLASS(SG_CKNames[SG_COMPONENT_LIGHT], SG_CKNames[SG_COMPONENT_TRANSFORM]);
    DOC_CLASS("Base class for all light components.");
    ADD_EX("basic/light.ck");
    ADD_EX("basic/pbr.ck");

    static t_CKINT light_type_directional = SG_LightType_Directional;
    static t_CKINT light_type_point       = SG_LightType_Point;
    SVAR("int", "Directional", &light_type_directional);
    SVAR("int", "Point", &light_type_point);

    // -------------------------

    CTOR(ulib_light_ctor);

    CTOR(ulib_light_ctor_with_type);
    ARG("int", "type");

    MFUN(ulib_light_set_type, "void", "mode");
    ARG("int", "mode");
    DOC_FUNC("Set the light type. Use GLight.Directional or GLight.Point");

    MFUN(ulib_light_get_type, "int", "mode");
    DOC_FUNC("Get the light type. Returns either GLight.Directional or GLight.Point.");

    MFUN(ulib_light_set_color, "void", "color");
    ARG("vec3", "color");
    DOC_FUNC("Set the light color.");

    MFUN(ulib_light_get_color, "vec3", "color");
    DOC_FUNC("Get the light color.");

    MFUN(ulib_light_set_intensity, "void", "intensity");
    ARG("float", "intensity");
    DOC_FUNC(
      "Set the light intensity. Default 1.0. Use 0 to turn off the light. The "
      "intensity is multiplied by the color in final lighting calculations.");

    MFUN(ulib_light_get_intensity, "float", "intensity");
    DOC_FUNC("Get the light intensity.");

    END_CLASS();

    // TODO document

    // point light ------------------------------------------------------------

    BEGIN_CLASS("GPointLight", SG_CKNames[SG_COMPONENT_LIGHT]);
    DOC_CLASS("Point light component.");

    CTOR(ulib_point_light_ctor);

    MFUN(ulib_point_light_get_radius, "float", "radius");
    DOC_FUNC("Get the point light radius.");

    MFUN(ulib_point_light_set_radius, "void", "radius");
    ARG("float", "radius");
    DOC_FUNC("Set the point light radius.");

    MFUN(ulib_point_light_get_falloff_exponent, "float", "falloff");
    DOC_FUNC(
      "Get the point light falloff exponent, i.e. how quickly the light intensity "
      "ramps down to 0. A value of 1.0 means linear, 2.0 means quadratic. Default "
      "is 2.0");

    MFUN(ulib_point_light_set_falloff_exponent, "void", "falloff");
    ARG("float", "falloff_exponent");
    DOC_FUNC(
      "Set the point light falloff exponent, i.e. how quickly the light intensity "
      "ramps down to 0. A value of 1.0 means linear, 2.0 means quadratic. Default "
      "is 2.0");

    END_CLASS();

    // directional light ------------------------------------------------------

    BEGIN_CLASS("GDirLight", SG_CKNames[SG_COMPONENT_LIGHT]);
    DOC_CLASS("Directional light component.");

    CTOR(ulib_dir_light_ctor);

    END_CLASS();
}

CK_DLL_CTOR(ulib_light_ctor)
{
    ulib_light_create(SELF, SG_LightType_Directional);
}

CK_DLL_CTOR(ulib_light_ctor_with_type)
{
    t_CKINT type = GET_NEXT_INT(ARGS);
    ulib_light_create(SELF, (SG_LightType)type);
}

CK_DLL_MFUN(ulib_light_set_type)
{
    SG_Light* light  = GET_LIGHT(SELF);
    t_CKINT type     = GET_NEXT_INT(ARGS);
    light->desc.type = (SG_LightType)type;

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_MFUN(ulib_light_get_type)
{
    SG_Light* light = GET_LIGHT(SELF);
    RETURN->v_int   = (t_CKINT)light->desc.type;
}

CK_DLL_MFUN(ulib_light_set_color)
{
    SG_Light* light = GET_LIGHT(SELF);
    t_CKVEC3 color  = GET_NEXT_VEC3(ARGS);

    light->desc.color = { color.x, color.y, color.z };

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_MFUN(ulib_light_get_color)
{
    SG_Light* light = GET_LIGHT(SELF);
    RETURN->v_vec3  = { light->desc.color.r, light->desc.color.g, light->desc.color.b };
}

CK_DLL_MFUN(ulib_light_set_intensity)
{
    SG_Light* light     = GET_LIGHT(SELF);
    t_CKFLOAT intensity = GET_NEXT_FLOAT(ARGS);

    light->desc.intensity = intensity;

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_MFUN(ulib_light_get_intensity)
{
    SG_Light* light = GET_LIGHT(SELF);
    RETURN->v_float = light->desc.intensity;
}

CK_DLL_CTOR(ulib_point_light_ctor)
{
    ulib_light_create(SELF, SG_LightType_Point);
}

CK_DLL_MFUN(ulib_point_light_get_radius)
{
    RETURN->v_float = GET_LIGHT(SELF)->desc.point_radius;
}

CK_DLL_MFUN(ulib_point_light_set_radius)
{
    SG_Light* light          = GET_LIGHT(SELF);
    t_CKFLOAT radius         = GET_NEXT_FLOAT(ARGS);
    light->desc.point_radius = radius;

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_MFUN(ulib_point_light_get_falloff_exponent)
{
    RETURN->v_float = GET_LIGHT(SELF)->desc.point_falloff;
}

CK_DLL_MFUN(ulib_point_light_set_falloff_exponent)
{
    SG_Light* light           = GET_LIGHT(SELF);
    t_CKFLOAT falloff         = GET_NEXT_FLOAT(ARGS);
    light->desc.point_falloff = falloff;

    CQ_PushCommand_LightUpdate(light);
}

CK_DLL_CTOR(ulib_dir_light_ctor)
{
    ulib_light_create(SELF, SG_LightType_Directional);
}
