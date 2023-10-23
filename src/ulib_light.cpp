#include "ulib_light.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"

//-----------------------------------------------------------------------------
// Object -> Light
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_light_ctor); // abstract base class, no constructor

CK_DLL_MFUN(cgl_light_set_intensity);
CK_DLL_MFUN(cgl_light_set_ambient);
CK_DLL_MFUN(cgl_light_set_diffuse);
CK_DLL_MFUN(cgl_light_set_specular);

CK_DLL_MFUN(cgl_light_get_intensity);
CK_DLL_MFUN(cgl_light_get_ambient);
CK_DLL_MFUN(cgl_light_get_diffuse);
CK_DLL_MFUN(cgl_light_get_specular);

// point light
CK_DLL_CTOR(cgl_point_light_ctor);
CK_DLL_MFUN(cgl_points_light_set_falloff);

// directional light
CK_DLL_CTOR(cgl_dir_light_ctor);

//-----------------------------------------------------------------------------
// init_chugl_light()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_light(Chuck_DL_Query *QUERY)
{
	// GLight =================================
	QUERY->begin_class(QUERY, Light::CKName(LightType::Base), "GGen");
	QUERY->doc_class(QUERY, "Light class. Parent class of GPointLight and GDirectionalLight. Don't instantiate this class directly.");

	QUERY->add_ctor(QUERY, cgl_light_ctor);

	QUERY->add_mfun(QUERY, cgl_light_set_intensity, "float", "intensity");  // 0 -- 1
	QUERY->add_arg(QUERY, "float", "i");
	QUERY->doc_func(QUERY, "Set intensity from 0-1. 0 is off, 1 is full intensity");

	QUERY->add_mfun(QUERY, cgl_light_set_ambient, "vec3", "ambient"); 
	QUERY->add_arg(QUERY, "vec3", "a");
	QUERY->doc_func(QUERY, "Set ambient color. Ambient color is multiplied by the material ambient color, and will be visible even when no light is directly shining on the object");

	QUERY->add_mfun(QUERY, cgl_light_set_diffuse, "vec3", "diffuse");
	QUERY->add_arg(QUERY, "vec3", "d");
	QUERY->doc_func(QUERY, "Set diffuse color. Diffuse color is multiplied by the material diffuse color");

	QUERY->add_mfun(QUERY, cgl_light_set_specular, "vec3", "specular");
	QUERY->add_arg(QUERY, "vec3", "s");
	QUERY->doc_func(QUERY, "Set specular color. Specular color is multiplied by the material specular color");

	QUERY->add_mfun(QUERY, cgl_light_get_intensity, "float", "intensity");
	QUERY->doc_func(QUERY, "Get light intensity");

	QUERY->add_mfun(QUERY, cgl_light_get_ambient, "vec3", "ambient");
	QUERY->doc_func(QUERY, "Get the light ambient color");

	QUERY->add_mfun(QUERY, cgl_light_get_diffuse, "vec3", "diffuse");
	QUERY->doc_func(QUERY, "Get the light diffuse color");

	QUERY->add_mfun(QUERY, cgl_light_get_specular, "vec3", "specular");
	QUERY->doc_func(QUERY, "Get the light specular color");

	QUERY->end_class(QUERY);

	// GPointLight =================================
	QUERY->begin_class(QUERY, Light::CKName(LightType::Point), Light::CKName(LightType::Base));
	QUERY->doc_class(QUERY, "Point light class");
    QUERY->add_ex(QUERY, "basic/light.ck");

	QUERY->add_ctor(QUERY, cgl_point_light_ctor);

	QUERY->add_mfun(QUERY, cgl_points_light_set_falloff, "void", "falloff");
	QUERY->add_arg(QUERY, "float", "linear");
	QUERY->add_arg(QUERY, "float", "quadratic");
	QUERY->doc_func(QUERY, "Set point light falloff. See https://learnopengl.com/Lighting/Light-casters for a falloff chart");

	QUERY->end_class(QUERY);

	// GDirLight =================================
	QUERY->begin_class(QUERY, Light::CKName(LightType::Directional), Light::CKName(LightType::Base));
	QUERY->doc_class(QUERY, "Directional class. Position of this light has no affect, only rotation");
	QUERY->add_ctor(QUERY, cgl_dir_light_ctor);
	QUERY->end_class(QUERY);

	return true;
}

// CGL Lights ===================================================

CK_DLL_CTOR(cgl_light_ctor)
{
	// abstract class. nothing to do
}

CK_DLL_MFUN(cgl_light_set_intensity)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	t_CKFLOAT i = GET_NEXT_FLOAT(ARGS);
	light->m_Params.intensity = i;
	RETURN->v_float = i;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_ambient)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	auto amb = GET_NEXT_VEC3(ARGS);
	light->m_Params.ambient = {amb.x, amb.y, amb.z};
	RETURN->v_vec3 = amb;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_diffuse)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	auto diff = GET_NEXT_VEC3(ARGS);
	light->m_Params.diffuse = {diff.x, diff.y, diff.z};
	RETURN->v_vec3 = diff;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_specular)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	auto spec = GET_NEXT_VEC3(ARGS);
	light->m_Params.specular = {spec.x, spec.y, spec.z};
	RETURN->v_vec3 = spec;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_get_intensity)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	RETURN->v_float = light->m_Params.intensity;
}

CK_DLL_MFUN(cgl_light_get_ambient)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	RETURN->v_vec3 = {light->m_Params.ambient.x, light->m_Params.ambient.y, light->m_Params.ambient.z};
}

CK_DLL_MFUN(cgl_light_get_diffuse)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	RETURN->v_vec3 = {light->m_Params.diffuse.x, light->m_Params.diffuse.y, light->m_Params.diffuse.z};
}

CK_DLL_MFUN(cgl_light_get_specular)
{
	Light *light = (Light *) CGL::GetSGO(SELF);
	RETURN->v_vec3 = {light->m_Params.specular.x, light->m_Params.specular.y, light->m_Params.specular.z};
}

CK_DLL_CTOR(cgl_point_light_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new PointLight, &CGL::mainScene, SELF, CGL::GetGGenDataOffset()));
}

CK_DLL_MFUN(cgl_points_light_set_falloff)
{
	PointLight *light = (PointLight *) CGL::GetSGO(SELF);
	t_CKFLOAT linear = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT quadratic = GET_NEXT_FLOAT(ARGS);
	light->m_Params.linear = linear;
	light->m_Params.quadratic = quadratic;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_CTOR(cgl_dir_light_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new DirLight, &CGL::mainScene, SELF, CGL::GetGGenDataOffset()));
}

