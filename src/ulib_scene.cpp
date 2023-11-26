#include "ulib_scene.h"
#include "ulib_cgl.h"
#include "scenegraph/Command.h"
#include "renderer/scenegraph/SceneGraphObject.h"

//-----------------------------------------------------------------------------
// Object -> Scene
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_scene_ctor);
CK_DLL_DTOR(cgl_scene_dtor);

CK_DLL_MFUN(cgl_scene_set_background_color);
CK_DLL_MFUN(cgl_scene_get_background_color);

// light fns
CK_DLL_MFUN(cgl_scene_get_num_lights);
CK_DLL_MFUN(cgl_scene_get_default_light);

// fog fns
CK_DLL_MFUN(cgl_scene_set_fog_color);
CK_DLL_MFUN(cgl_scene_set_fog_density);
CK_DLL_MFUN(cgl_scene_set_fog_type);

CK_DLL_MFUN(cgl_scene_set_fog_enabled);
CK_DLL_MFUN(cgl_scene_set_fog_disabled);

CK_DLL_MFUN(cgl_scene_get_fog_color);
CK_DLL_MFUN(cgl_scene_get_fog_density);
CK_DLL_MFUN(cgl_scene_get_fog_type);

// skybox
CK_DLL_MFUN(cgl_scene_set_skybox_cubemap);
CK_DLL_MFUN(cgl_scene_get_skybox_cubemap);
CK_DLL_MFUN(cgl_scene_set_skybox_enabled);
CK_DLL_MFUN(cgl_scene_get_skybox_enabled);


//-----------------------------------------------------------------------------
// init_chugl_scene()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_scene(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");
	// CGL scene
	QUERY->begin_class(QUERY, "GScene", "GGen");
	QUERY->doc_class(QUERY, "Scene class. Static--all instances point to the same underlying ChuGL main scene. GGens must be added to a scene to be rendered");
    QUERY->add_ex(QUERY, "basic/fog.ck");
	
	QUERY->add_ctor(QUERY, cgl_scene_ctor);

	// static constants
	// TODO: add linear fog? but doesn't even look as good
	QUERY->add_svar(QUERY, "int", "FOG_EXP", true, (void *)&Scene::FOG_EXP);
	QUERY->doc_var(QUERY, "Fog type: exponential");

	QUERY->add_svar(QUERY, "int", "FOG_EXP2", true, (void *)&Scene::FOG_EXP2);
	QUERY->doc_var(QUERY, "Fog type: exponential-squared. more aggressive");

	// background color
	QUERY->add_mfun(QUERY, cgl_scene_set_background_color, "vec3", "backgroundColor");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->doc_func(QUERY, "Set the background color of the scene");


	QUERY->add_mfun(QUERY, cgl_scene_get_background_color, "vec3", "backgroundColor");
	QUERY->doc_func(QUERY, "Get the background color of the scene");

	// light --------------------------------------------------------
	QUERY->add_mfun(QUERY, cgl_scene_get_default_light, Light::CKName(LightType::Base), "light");
	QUERY->doc_func(QUERY, "Get the default directional light of the scene");

	QUERY->add_mfun(QUERY, cgl_scene_get_num_lights, "int", "numLights");
	QUERY->doc_func(QUERY, "Get the number of instantiated lights");


	// fog member vars ----------------------------------------------
	QUERY->add_mfun(QUERY, cgl_scene_set_fog_color, "vec3", "fogColor");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->doc_func(QUERY, "Set the fog color of the scene");

	QUERY->add_mfun(QUERY, cgl_scene_get_fog_color, "vec3", "fogColor");
	QUERY->doc_func(QUERY, "Get the fog color of the scene");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_density, "float", "fogDensity");
	QUERY->add_arg(QUERY, "float", "density");
	QUERY->doc_func(QUERY, "Set fog density. typically between 0.0 and 0.1");

	QUERY->add_mfun(QUERY, cgl_scene_get_fog_density, "float", "fogDensity");
	QUERY->doc_func(QUERY, "Get fog density");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_type, "int", "fogType");
	QUERY->add_arg(QUERY, "int", "type");
	QUERY->doc_func(QUERY, "Set fog type. Use one of the static constants: FOG_EXP or FOG_EXP2");

	QUERY->add_mfun(QUERY, cgl_scene_get_fog_type, "int", "fogType");
	QUERY->doc_func(QUERY, "Get fog type. Can be FOG_EXP or FOG_EXP2");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_enabled, "void", "enableFog");
	QUERY->doc_func(QUERY, "enable fog for the scene");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_disabled, "void", "disableFog");
	QUERY->doc_func(QUERY, "disable fog for the scene");

	// skybox --------------------------------------------------------
	QUERY->add_mfun(
		QUERY, 
		cgl_scene_set_skybox_cubemap, 
		CGL_Texture::CKName(CGL_TextureType::CubeMap), 
		"skybox"
	);
	QUERY->add_arg(
		QUERY, 
		CGL_Texture::CKName(CGL_TextureType::CubeMap), 
		"cubemap"
	);
	QUERY->doc_func(
		QUERY, 
		"Set the skybox of the scene. Pass in a cubemap texture. Will enable the skybox."
	);

	QUERY->add_mfun(QUERY, cgl_scene_get_skybox_cubemap, CGL_Texture::CKName(CGL_TextureType::CubeMap), "skybox");
	QUERY->doc_func(QUERY, "Get the skybox of the scene");

	QUERY->add_mfun(QUERY, cgl_scene_set_skybox_enabled, "int", "skyboxEnabled");
	QUERY->add_arg(QUERY, "int", "enabled");
	QUERY->doc_func(QUERY, "Enable or disable the skybox");

	QUERY->add_mfun(QUERY, cgl_scene_get_skybox_enabled, "int", "skyboxEnabled");
	QUERY->doc_func(QUERY, "Get whether the skybox is enabled or disabled");

	// end class -----------------------------------------------------
	QUERY->end_class(QUERY);

	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_scene_ctor)
{
	// temporary fix until a default pre-constructor is provided for chugins
	//
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	OBJ_MEMBER_INT(SELF, CGL::GetGGenDataOffset()) = (t_CKINT)scene;
}

CK_DLL_DTOR(cgl_scene_dtor)
{
	// Scene* mainScene = (Scene*) CGL::GetSGO(SELF);
	// don't call delete! because this is a static var
}

CK_DLL_MFUN(cgl_scene_set_background_color)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	scene->SetBackgroundColor(color.x, color.y, color.z);
	RETURN->v_vec3 = color;
	CGL::PushCommand(new UpdateSceneBackgroundColorCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_background_color)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	const auto &color = scene->GetBackgroundColor();
	RETURN->v_vec3 = {color.x, color.y, color.z};
}

CK_DLL_MFUN(cgl_scene_get_default_light)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	Light* defaultLight = scene->GetDefaultLight();
	RETURN->v_object = defaultLight ? defaultLight->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(cgl_scene_get_num_lights)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	RETURN->v_int = scene->m_Lights.size();
}	

CK_DLL_MFUN(cgl_scene_set_fog_color)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	scene->SetFogColor(color.x, color.y, color.z);
	RETURN->v_vec3 = color;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_color)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	const auto &color = scene->GetFogColor();
	RETURN->v_vec3 = {color.x, color.y, color.z};
}

CK_DLL_MFUN(cgl_scene_set_fog_density)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	t_CKFLOAT density = GET_NEXT_FLOAT(ARGS);
	scene->SetFogDensity(density);
	RETURN->v_float = density;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_density)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	RETURN->v_float = scene->GetFogDensity();
}

CK_DLL_MFUN(cgl_scene_set_fog_type)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	t_CKINT type = GET_NEXT_INT(ARGS);
	scene->SetFogType((FogType)type);
	RETURN->v_int = type;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_type)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	RETURN->v_int = scene->GetFogType();
}

CK_DLL_MFUN(cgl_scene_set_fog_enabled)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	scene->SetFogEnabled(true);
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_set_fog_disabled)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	scene->SetFogEnabled(false);
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

// skybox ==============================================

CK_DLL_MFUN(cgl_scene_set_skybox_cubemap)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	Chuck_Object* cubemap_obj = GET_NEXT_OBJECT(ARGS);

	CGL_CubeMap* cubemap = (CGL_CubeMap*) CGL::GetTexture(cubemap_obj);

	CGL::PushCommand(
		new UpdateSceneSkyboxCommand(
			scene,
			cubemap
		)
	);

	RETURN->v_object = cubemap_obj;
}

CK_DLL_MFUN(cgl_scene_get_skybox_cubemap)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	CGL_CubeMap* cubemap = scene->GetSkybox();
	RETURN->v_object = cubemap ? cubemap->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(cgl_scene_set_skybox_enabled)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	t_CKINT enabled = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new UpdateSceneSkyboxEnabledCommand(scene, enabled));
	RETURN->v_int = enabled;
}

CK_DLL_MFUN(cgl_scene_get_skybox_enabled)
{
	Scene *scene = (Scene *) CGL::GetSGO(SELF);
	RETURN->v_int = scene->GetSkyboxEnabled();
}