#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "ulib_cgl.h"
#include "renderer/scenegraph/Camera.h"
#include "renderer/scenegraph/Command.h"
#include "renderer/scenegraph/Scene.h"
#include "renderer/scenegraph/Light.h"
#include "renderer/scenegraph/CGL_Texture.h"

#include "chuck_vm.h"
#include "chuck_dl.h"


//-----------------------------------------------------------------------------
// ChuGL Events
//-----------------------------------------------------------------------------
// frame event
CK_DLL_CTOR(cgl_frame_ctor);
CK_DLL_DTOR(cgl_frame_dtor);
// update event
CK_DLL_CTOR(cgl_update_ctor);
CK_DLL_DTOR(cgl_update_dtor);

CK_DLL_MFUN(cgl_update_event_waiting_on);


//-----------------------------------------------------------------------------
// Static Fns
//-----------------------------------------------------------------------------
CK_DLL_SFUN(cgl_next_frame);
CK_DLL_SFUN(cgl_register);
CK_DLL_SFUN(cgl_unregister);


//-----------------------------------------------------------------------------
// ChuGL Object
//-----------------------------------------------------------------------------
// *structors
CK_DLL_CTOR(cgl_obj_ctor);
CK_DLL_DTOR(cgl_obj_dtor);

CK_DLL_MFUN(cgl_obj_get_id);

// transform API
CK_DLL_MFUN(cgl_obj_get_right);
CK_DLL_MFUN(cgl_obj_get_forward);
CK_DLL_MFUN(cgl_obj_get_up);
CK_DLL_MFUN(cgl_obj_translate_by);
CK_DLL_MFUN(cgl_obj_scale_by);
CK_DLL_MFUN(cgl_obj_rot_on_local_axis);
CK_DLL_MFUN(cgl_obj_rot_on_world_axis);
CK_DLL_MFUN(cgl_obj_rot_x);
CK_DLL_MFUN(cgl_obj_rot_y);
CK_DLL_MFUN(cgl_obj_rot_z);
CK_DLL_MFUN(cgl_obj_pos_x);
CK_DLL_MFUN(cgl_obj_pos_y);
CK_DLL_MFUN(cgl_obj_pos_z);
CK_DLL_MFUN(cgl_obj_lookat_vec3);
CK_DLL_MFUN(cgl_obj_lookat_float);
CK_DLL_MFUN(cgl_obj_set_pos);
CK_DLL_MFUN(cgl_obj_set_rot);
CK_DLL_MFUN(cgl_obj_set_scale);
CK_DLL_MFUN(cgl_obj_get_pos);
CK_DLL_MFUN(cgl_obj_get_rot);
CK_DLL_MFUN(cgl_obj_get_scale);
CK_DLL_MFUN(cgl_obj_get_world_pos);

// parent-child scenegraph API
CK_DLL_MFUN(cgl_obj_add_child);
CK_DLL_GFUN(ggen_op_gruck);

//-----------------------------------------------------------------------------
// Object -> BaseCamera
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_cam_ctor);
CK_DLL_DTOR(cgl_cam_dtor);

//-----------------------------------------------------------------------------
// Object -> Scene
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_scene_ctor);
CK_DLL_DTOR(cgl_scene_dtor);

//-----------------------------------------------------------------------------
// Object -> Light
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_light_ctor);  // abstract base class, no constructor
CK_DLL_DTOR(cgl_light_dtor);

// point light
CK_DLL_CTOR(cgl_point_light_ctor);
// CK_DLL_MFUN(cgl_light_point_set); // TODO: allow setting params

// directional light
CK_DLL_CTOR(cgl_dir_light_ctor);


//-----------------------------------------------------------------------------
// Geometry
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_geo_ctor);
CK_DLL_DTOR(cgl_geo_dtor);

// box
CK_DLL_CTOR(cgl_geo_box_ctor);
CK_DLL_MFUN(cgl_geo_box_set);

// sphere
CK_DLL_CTOR(cgl_geo_sphere_ctor);
// TODO: sphere parameter setter

// custom
CK_DLL_CTOR(cgl_geo_custom_ctor);
CK_DLL_MFUN(cgl_geo_set_attribute);  // general case for any kind of vertex data
CK_DLL_MFUN(cgl_geo_set_positions);


//-----------------------------------------------------------------------------
// Texture 
//-----------------------------------------------------------------------------

CK_DLL_CTOR(cgl_texture_ctor);
CK_DLL_DTOR(cgl_texture_dtor);

// sampler wrap mode
CK_DLL_MFUN(cgl_texture_set_wrap);
CK_DLL_MFUN(cgl_texture_get_wrap_s);
CK_DLL_MFUN(cgl_texture_get_wrap_t);

// sampler filter mode
CK_DLL_MFUN(cgl_texture_set_filter);
CK_DLL_MFUN(cgl_texture_get_filter_min);
CK_DLL_MFUN(cgl_texture_get_filter_mag);

// Texture --> FileTexture (texture from filepath .png .jpg etc)
CK_DLL_CTOR(cgl_texture_file_ctor);
CK_DLL_MFUN(cgl_texture_file_set_filepath);
CK_DLL_MFUN(cgl_texture_file_get_filepath);

// Texture --> DataTexture (texture from chuck array)
CK_DLL_CTOR(cgl_texture_rawdata_ctor);
CK_DLL_MFUN(cgl_texture_rawdata_set_data);
// CK_DLL_MFUN(cgl_texture_rawdata_get_data);


// TODO: getters? 

//-----------------------------------------------------------------------------
// Materials
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_mat_ctor);
CK_DLL_DTOR(cgl_mat_dtor);

// base material options
CK_DLL_MFUN(cgl_mat_set_polygon_mode);
CK_DLL_MFUN(cgl_mat_get_polygon_mode);
CK_DLL_MFUN(cgl_mat_set_point_size);
// CK_DLL_MFUN(cgl_mat_set_line_width);
// CK_DLL_MFUN(cgl_mat_set_cull_mode);  // TODO

	// uniform setters
CK_DLL_MFUN(cgl_mat_set_uniform_float);
CK_DLL_MFUN(cgl_mat_set_uniform_float2);
CK_DLL_MFUN(cgl_mat_set_uniform_float3);
CK_DLL_MFUN(cgl_mat_set_uniform_float4);
CK_DLL_MFUN(cgl_mat_set_uniform_int);
CK_DLL_MFUN(cgl_mat_set_uniform_int2);
CK_DLL_MFUN(cgl_mat_set_uniform_int3);
CK_DLL_MFUN(cgl_mat_set_uniform_int4);
CK_DLL_MFUN(cgl_mat_set_uniform_bool);
CK_DLL_MFUN(cgl_mat_set_uniform_texID);

// normal mat
CK_DLL_CTOR(cgl_mat_norm_ctor);
CK_DLL_MFUN(cgl_set_use_local_normals);

// phong specular mat
CK_DLL_CTOR(cgl_mat_phong_ctor);
	// uniform setters
CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map);
CK_DLL_MFUN(cgl_mat_phong_set_specular_map);
CK_DLL_MFUN(cgl_mat_phong_set_diffuse_color);
CK_DLL_MFUN(cgl_mat_phong_set_specular_color);
CK_DLL_MFUN(cgl_mat_phong_set_log_shininess);
	// uniform getters TODO

// custom shader mat
CK_DLL_CTOR(cgl_mat_custom_shader_ctor);
CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders);

// points mat
CK_DLL_CTOR(cgl_mat_points_ctor);




//-----------------------------------------------------------------------------
// Object -> Mesh
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_mesh_ctor);
CK_DLL_DTOR(cgl_mesh_dtor);
CK_DLL_MFUN(cgl_mesh_set);

//-----------------------------------------------------------------------------
// Object -> Group
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_group_ctor);
CK_DLL_DTOR(cgl_group_dtor);



// exports =========================================

t_CKBOOL init_chugl_events(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_geo(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_texture(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_mat(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_obj(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_cam(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_scene(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_group(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_mesh(Chuck_DL_Query* QUERY);
t_CKBOOL init_chugl_light(Chuck_DL_Query* QUERY);


static t_CKUINT cglframe_data_offset = 0;
static t_CKUINT cglupdate_data_offset = 0;
static t_CKUINT ggen_data_offset = 0;
static t_CKUINT cglcamera_data_offset = 0;
static t_CKUINT cglgeo_data_offset = 0;
static t_CKUINT cgltexture_data_offset = 0;
static t_CKUINT cglmat_data_offset = 0;

t_CKBOOL init_chugl(Chuck_DL_Query * QUERY)
{
	init_chugl_events(QUERY);
	init_chugl_static_fns(QUERY);
	init_chugl_geo(QUERY);
	init_chugl_texture(QUERY);
	init_chugl_mat(QUERY);
	init_chugl_obj(QUERY);
	init_chugl_cam(QUERY);
	init_chugl_scene(QUERY);
	init_chugl_group(QUERY);
	init_chugl_mesh(QUERY);
	init_chugl_light(QUERY);

	return true;
}

//-----------------------------------------------------------------------------
// init_chugl_events()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_events(Chuck_DL_Query* QUERY)
{
	CglEvent::s_SharedEventQueue = QUERY->api()->vm->create_event_buffer(
		QUERY->vm()
	);

	// Frame event =================================
	QUERY->begin_class(QUERY, "CglFrame", "Event");
	QUERY->add_ctor(QUERY, cgl_frame_ctor);
	QUERY->add_dtor(QUERY, cgl_frame_dtor);
	// TODO: maybe add a frame count member var
	// TODO: add a glfw time variable (will be different from chuck time)
	// reserve varaible in chuck internal class to store reference
	cglframe_data_offset = QUERY->add_mvar(QUERY, "int", "@cglframe_data", false);
	QUERY->end_class(QUERY);

	// Update event ================================
	// triggered by main render thread after deepcopy is complete, and safe for chuck to begin updating the scene graph
	QUERY->begin_class(QUERY, "CglUpdate", "Event");
	QUERY->add_ctor(QUERY, cgl_update_ctor);
	QUERY->add_dtor(QUERY, cgl_update_dtor);
	cglupdate_data_offset = QUERY->add_mvar(QUERY, "int", "@cglupdate_data", false);

	QUERY->add_mfun(QUERY, cgl_update_event_waiting_on, "void", "waiting_on");


	QUERY->end_class(QUERY);

	return true;
}

CK_DLL_CTOR(cgl_frame_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, cglframe_data_offset) = (t_CKINT) new CglEvent(
		(Chuck_Event*)SELF, SHRED->vm_ref, API, CglEventType::CGL_FRAME
	);
}
CK_DLL_DTOR(cgl_frame_dtor)
{
	CglEvent* cglEvent = (CglEvent*)OBJ_MEMBER_INT(SELF, cglframe_data_offset);
	CK_SAFE_DELETE(cglEvent);
	OBJ_MEMBER_INT(SELF, cglframe_data_offset) = 0;
}
CK_DLL_CTOR(cgl_update_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, cglupdate_data_offset) = (t_CKINT) new CglEvent(
		(Chuck_Event*)SELF, SHRED->vm_ref, API, CglEventType::CGL_UPDATE
	);
	std::cerr << "!!!!cgl_update_ctor" << std::endl;
}
CK_DLL_DTOR(cgl_update_dtor)
{
	CglEvent* cglEvent = (CglEvent*)OBJ_MEMBER_INT(SELF, cglupdate_data_offset);
	CK_SAFE_DELETE(cglEvent);
	OBJ_MEMBER_INT(SELF, cglupdate_data_offset) = 0;

	std::cerr << "~~~~cgl_update_dtor" << std::endl;
}
CK_DLL_MFUN(cgl_update_event_waiting_on)
{
	// THIS IS A VERY IMPORTANT FUNCTION. See 
	// https://trello.com/c/Gddnu21j/6-chuglrender-refactor-possible-deadlock-between-cglupdate-and-render
	// and 
	// https://github.com/ccrma/chugl/blob/2023-chugl-int/design/multishred-render-1.ck
	// for further context

	// not used for now, will become relevant if we ever want to support multiple 
	// windows and/or renderers
	// std::cerr << "entered waiting_on" << std::endl;
	CglEvent* cglEvent = (CglEvent*)OBJ_MEMBER_INT(SELF, cglupdate_data_offset);

	// Add shred (no-op if already added)
	CGL::RegisterShred(SHRED);
	// std::cerr << "REGISTERED SHRED " << SHRED << std::endl;

	// Add shred to waiting list
	CGL::RegisterShredWaiting(SHRED);
	// std::cerr << "REGISTERED SHRED WAITING" << SHRED << std::endl;

	// if #waiting == #registered, all CGL shreds have finished work, and we are safe to wakeup the renderer
	if (CGL::GetNumShredsWaiting() >= CGL::GetNumRegisteredShreds()) {
		// std::cerr << "#waiting == #registered == " << CGL::GetNumShredsWaiting() << " waking up renderer" << std::endl;
		CGL::ClearShredWaiting();  // clear thread waitlist 
		CGL::Render();
	}
}
//-----------------------------------------------------------------------------
// init_chugl_static_fns()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL static fns");

	QUERY->begin_class(QUERY, "CGL", "Object");  // for global stuff
	QUERY->add_sfun(QUERY, cgl_next_frame, "CglUpdate", "nextFrame");

	QUERY->add_sfun(QUERY, cgl_unregister, "void", "unregister");
	QUERY->add_sfun(QUERY, cgl_register, "void", "register");


	QUERY->end_class(QUERY);

	return true;
}
/*============CGL static fns============*/
CK_DLL_SFUN(cgl_next_frame)
{

	// extract CglEvent from obj
	// TODO: workaround bug where create() object API is not calling preconstructors
	// https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate
	RETURN->v_object = (Chuck_Object *)CGL::GetCachedShredUpdateEvent(
		SHRED, API, VM
	);

}
CK_DLL_SFUN(cgl_register)
{
	CGL::RegisterShred(SHRED);
}
CK_DLL_SFUN(cgl_unregister)
{
	CGL::UnregisterShred(SHRED);
}


//-----------------------------------------------------------------------------
// init_chugl_geo()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_geo(Chuck_DL_Query* QUERY)
{
	QUERY->begin_class(QUERY, "CglGeo", "Object");
	QUERY->add_ctor(QUERY, cgl_geo_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);
	cglgeo_data_offset = QUERY->add_mvar(QUERY, "int", "@cglgeo_data", false);  // TODO: still bugged?

	// TODO: add svars for attribute locations
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "BoxGeo", "CglGeo");
	QUERY->add_ctor(QUERY, cgl_geo_box_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_box_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "float", "depth");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "int", "depthSeg");
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "SphereGeo", "CglGeo");
	QUERY->add_ctor(QUERY, cgl_geo_sphere_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);
	QUERY->end_class(QUERY);

	// custom geo
	QUERY->begin_class(QUERY, "CustomGeo", "CglGeo");
	QUERY->add_ctor(QUERY, cgl_geo_custom_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_set_attribute, "void", "setAttribute");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "location");
	QUERY->add_arg(QUERY, "int", "numComponents");
	// QUERY->add_arg(QUERY, "int", "normalize");
	QUERY->add_arg(QUERY, "float[]", "data");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions, "void", "setPositions");
	QUERY->add_arg(QUERY, "float[]", "positions");
	
	QUERY->end_class(QUERY);

	return true;
}

// CGL Geometry =======================
CK_DLL_CTOR(cgl_geo_ctor) {}
CK_DLL_DTOR(cgl_geo_dtor)  // all geos can share this base destructor
{
	Geometry* geo = (Geometry*)OBJ_MEMBER_INT(SELF, cglgeo_data_offset);
	CK_SAFE_DELETE(geo);
	OBJ_MEMBER_INT(SELF, cglgeo_data_offset) = 0;  // zero out the memory

	// TODO: trigger destruction callback and scenegraph removal command
}

// box geo
CK_DLL_CTOR(cgl_geo_box_ctor)
{
	std::cerr << "cgl_box_ctor\n";
	BoxGeometry* boxGeo = new BoxGeometry;
	OBJ_MEMBER_INT(SELF, cglgeo_data_offset) = (t_CKINT) boxGeo;
	std::cerr << "finished initializing boxgeo\n";

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(boxGeo));
}

CK_DLL_MFUN(cgl_geo_box_set)
{ BoxGeometry* geo = (BoxGeometry*)OBJ_MEMBER_INT(SELF, cglgeo_data_offset); t_CKFLOAT width = GET_NEXT_FLOAT(ARGS); t_CKFLOAT height = GET_NEXT_FLOAT(ARGS); t_CKFLOAT depth = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSeg = GET_NEXT_INT(ARGS);
	t_CKINT heightSeg = GET_NEXT_INT(ARGS);
	t_CKINT depthSeg = GET_NEXT_INT(ARGS);
	geo->UpdateParams(width, height, depth, widthSeg, heightSeg, depthSeg);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// sphere geo 
CK_DLL_CTOR(cgl_geo_sphere_ctor)
{
	std::cerr << "cgl_sphere_ctor\n";
	SphereGeometry* sphereGeo = new SphereGeometry;
	OBJ_MEMBER_INT(SELF, cglgeo_data_offset) = (t_CKINT) sphereGeo;
	std::cerr << "finished initializing spheregeo\n";

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(sphereGeo));
}

// Custom geo ---------
CK_DLL_CTOR(cgl_geo_custom_ctor)
{
	std::cerr << "cgl_custom_ctor\n";
	CustomGeometry* customGeo = new CustomGeometry;
	OBJ_MEMBER_INT(SELF, cglgeo_data_offset) = (t_CKINT) customGeo;
	std::cerr << "finished initializing customgeo\n";

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(customGeo));
}

CK_DLL_MFUN(cgl_geo_set_attribute)
{
	CustomGeometry* geo = (CustomGeometry*)OBJ_MEMBER_INT(SELF, cglgeo_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	t_CKINT location = GET_NEXT_INT(ARGS);
	t_CKINT numComponents = GET_NEXT_INT(ARGS);
	bool normalize = GET_NEXT_INT(ARGS);
	Chuck_Array8* data = (Chuck_Array8*) GET_NEXT_OBJECT(ARGS);

	// not stored in chuck-side copy to save time
	// geo->SetAttribute(name, location, numComponents, normalize, data);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, name->str(), location, numComponents, data->m_vector, normalize
		)
	);
}

CK_DLL_MFUN(cgl_geo_set_positions)
{
	CustomGeometry* geo = (CustomGeometry*)OBJ_MEMBER_INT(SELF, cglgeo_data_offset);

	Chuck_Array8* data = (Chuck_Array8*) GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, data->m_vector, false
		)
	);
}


//-----------------------------------------------------------------------------
// init_chugl_texture()
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_texture(Chuck_DL_Query* QUERY)
{
	QUERY->begin_class(QUERY, "CglTexture", "Object");
	QUERY->add_ctor(QUERY, cgl_texture_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);
	cgltexture_data_offset = QUERY->add_mvar(QUERY, "int", "@cgltexture_dat", false);

	// texture options (static constants) ---------------------------------
    QUERY->add_svar( QUERY, "int", "WRAP_REPEAT", TRUE, (void *)&CGL_Texture::Repeat);
    QUERY->add_svar( QUERY, "int", "WRAP_MIRRORED", TRUE, (void *)&CGL_Texture::MirroredRepeat);
    QUERY->add_svar( QUERY, "int", "WRAP_CLAMP", TRUE, (void *)&CGL_Texture::ClampToEdge);

	// not exposing mipmap filter options for simplicity
    QUERY->add_svar( QUERY, "int", "FILTER_NEAREST", TRUE, (void *)&CGL_Texture::Nearest);
    QUERY->add_svar( QUERY, "int", "FILTER_LINEAR", TRUE, (void *)&CGL_Texture::Linear);

	// member fns -----------------------------------------------------------
	QUERY->add_mfun(QUERY, cgl_texture_set_wrap, "void", "wrap");
	QUERY->add_arg(QUERY, "int", "s");
	QUERY->add_arg(QUERY, "int", "t");

	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_s, "int", "wrapS");
	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_t, "int", "wrapT");

	QUERY->add_mfun(QUERY, cgl_texture_set_filter, "void", "filter");
	QUERY->add_arg(QUERY, "int", "min");
	QUERY->add_arg(QUERY, "int", "mag");
	QUERY->add_mfun(QUERY, cgl_texture_get_filter_min, "int", "filterMin");
	QUERY->add_mfun(QUERY, cgl_texture_get_filter_mag, "int", "filterMag");
	QUERY->end_class(QUERY);


	// FileTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, "FileTexture", "CglTexture");
	QUERY->add_ctor(QUERY, cgl_texture_file_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);

	QUERY->add_mfun(QUERY, cgl_texture_file_set_filepath, "string", "path");
	QUERY->add_arg(QUERY, "string", "path");
	QUERY->add_mfun(QUERY, cgl_texture_file_get_filepath, "string", "path");
	QUERY->end_class(QUERY);


	// DataTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, "DataTexture", "CglTexture");
	QUERY->add_ctor(QUERY, cgl_texture_rawdata_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);

	QUERY->add_mfun(QUERY, cgl_texture_rawdata_set_data, "void", "data");
	QUERY->add_arg(QUERY, "float[]", "data");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");
	QUERY->end_class(QUERY);



	return true;
}

// CGL_Texture API impl =====================================================
CK_DLL_CTOR(cgl_texture_ctor)
{
	// abstract base texture class, do nothing
	// chuck DLL will call all constructors in QUERY inheritance chain
}

CK_DLL_DTOR(cgl_texture_dtor)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	CK_SAFE_DELETE(texture);
	OBJ_MEMBER_INT(SELF, cgltexture_data_offset) = 0;

	// TODO: send destroy command to CGL command queue
	//       - remove texture from scenegraph
	// 	     - callback hook for renderer to remove TextureMat from cache
	// idea: .dispose() of THREEjs. need to deliberately invoke freeing GPU resources from chuck side,
	// chuck destructor does NOT implicitly free GPU resources (probably safer this way)
}

CK_DLL_MFUN(cgl_texture_set_wrap)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	auto s = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS)); 
	auto t = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS));
	texture->SetWrapMode(s, t);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_wrap_s)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.wrapS);
}

CK_DLL_MFUN(cgl_texture_get_wrap_t)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.wrapS);
}

CK_DLL_MFUN(cgl_texture_set_filter)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	auto min = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS)); 
	auto mag = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS));
	texture->SetFilterMode(min, mag);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_filter_min)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.filterMin);
}

CK_DLL_MFUN(cgl_texture_get_filter_mag)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.filterMag);
}

// FileTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_file_ctor)
{
	std::cerr << "cgl_texture_file_ctor" << std::endl;

	CGL_Texture* texture = new CGL_Texture(CGL_TextureType::File2D);
	OBJ_MEMBER_INT(SELF, cgltexture_data_offset) = (t_CKINT) texture;

	// Creation command
	CGL::PushCommand(new CreateTextureCommand(texture));
}

CK_DLL_MFUN(cgl_texture_file_set_filepath)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	Chuck_String * path = GET_NEXT_STRING(ARGS);
	texture->m_FilePath = path->str();  // note: doesn't make sense to update flags on chuck-side copy because renderer doesn't have access

	CGL::PushCommand(new UpdateTexturePathCommand(texture));

	RETURN->v_string = path;
}

CK_DLL_MFUN(cgl_texture_file_get_filepath)
{
	CGL_Texture* texture = (CGL_Texture*)OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
    RETURN->v_string = (Chuck_String *) API->object->create_string(API, SHRED, texture->m_FilePath.c_str());
}

// DataTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_rawdata_ctor)
{
	std::cerr << "cgl_texture_rawdata_ctor" << std::endl;

	CGL_Texture* texture = new CGL_Texture(CGL_TextureType::RawData);
	OBJ_MEMBER_INT(SELF, cgltexture_data_offset) = (t_CKINT) texture;

	// Creation command
	CGL::PushCommand(new CreateTextureCommand(texture));
}

CK_DLL_MFUN(cgl_texture_rawdata_set_data)
{
	CGL_Texture* texture = (CGL_Texture *) OBJ_MEMBER_INT(SELF, cgltexture_data_offset);
	Chuck_Array8* data = (Chuck_Array8*) GET_NEXT_OBJECT(ARGS);
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);

	// update chuck-side texture ( no copy to avoid blocking audio thread )
	texture->SetRawData(data->m_vector, width, height, false);

	CGL::PushCommand(new UpdateTextureDataCommand(texture->GetID(), data->m_vector, width, height));
}


//-----------------------------------------------------------------------------
// init_chugl_mat()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mat(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL materials");

	QUERY->begin_class(QUERY, "CglMat", "Object");
	QUERY->add_ctor(QUERY, cgl_mat_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);
	cglmat_data_offset = QUERY->add_mvar(QUERY, "int", "@cglmat_data", false);

	// Material params (static constants) ---------------------------------
    QUERY->add_svar(QUERY, "int", "POLYGON_FILL", TRUE, (void *)&Material::POLYGON_FILL);
    QUERY->add_svar(QUERY, "int", "POLYGON_LINE", TRUE, (void *)&Material::POLYGON_LINE);
	QUERY->add_svar(QUERY, "int", "POLYGON_POINT", TRUE, (void*)&Material::POLYGON_POINT);

	QUERY->add_mfun(QUERY, cgl_mat_set_polygon_mode, "int", "polygonMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->add_mfun(QUERY, cgl_mat_get_polygon_mode, "int", "polygonMode");

	QUERY->add_mfun(QUERY, cgl_mat_set_point_size, "void", "pointSize");
	QUERY->add_arg(QUERY, "float", "size");
	// QUERY->add_mfun(QUERY, cgl_mat_set_line_width, "void", "lineWidth");
	// QUERY->add_arg(QUERY, "float", "width");

	// uniform setters
	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float, "void", "uniformFloat");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float2, "void", "uniformFloat2");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float3, "void", "uniformFloat3");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");
	QUERY->add_arg(QUERY, "float", "f2");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_float4, "void", "uniformFloat4");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "float", "f0");
	QUERY->add_arg(QUERY, "float", "f1");
	QUERY->add_arg(QUERY, "float", "f2");
	QUERY->add_arg(QUERY, "float", "f3");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int, "void", "uniformInt");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int2, "void", "uniformInt2");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int3, "void", "uniformInt3");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");
	QUERY->add_arg(QUERY, "int", "i2");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_int4, "void", "uniformInt4");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "i0");
	QUERY->add_arg(QUERY, "int", "i1");
	QUERY->add_arg(QUERY, "int", "i2");
	QUERY->add_arg(QUERY, "int", "i3");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_bool, "void", "uniformBool");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "b0");

	QUERY->add_mfun(QUERY, cgl_mat_set_uniform_texID, "void", "uniformTexture");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "CglTexture", "texture");

	QUERY->end_class(QUERY);

	// normal material
	QUERY->begin_class(QUERY, "NormMat", "CglMat");
	QUERY->add_ctor(QUERY, cgl_mat_norm_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_set_use_local_normals, "void", "useLocal");
	QUERY->add_arg(QUERY, "int", "useLocal");

	QUERY->end_class(QUERY);

	// phong specular material
	QUERY->begin_class(QUERY, "PhongMat", "CglMat");
	QUERY->add_ctor(QUERY, cgl_mat_phong_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_diffuse_map, "void", "diffuseMap");
	QUERY->add_arg(QUERY, "CglTexture", "tex");
	
	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_map, "void", "specularMap");
	QUERY->add_arg(QUERY, "CglTexture", "tex");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_diffuse_color, "vec3", "diffuseColor");
	QUERY->add_arg(QUERY, "vec3", "color");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_color, "vec3", "specularColor");
	QUERY->add_arg(QUERY, "vec3", "color");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_log_shininess, "float", "shine");
	QUERY->add_arg(QUERY, "float", "shininess");

	// TODO: add getters

	QUERY->end_class(QUERY);

	// custom shader material
	QUERY->begin_class(QUERY, "ShaderMat", "CglMat");
	QUERY->add_ctor(QUERY, cgl_mat_custom_shader_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_shaders, "void", "shaderPaths");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->add_arg(QUERY, "string", "frag");

	QUERY->end_class(QUERY);

	// points material
	QUERY->begin_class(QUERY, "PointsMat", "CglMat");
	QUERY->add_ctor(QUERY, cgl_mat_points_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->end_class(QUERY);


	return true;
}
// CGL Materials ===================================================
CK_DLL_CTOR(cgl_mat_ctor)
{
	std::cerr << "cgl_mat_ctor\n";
	// dud, do nothing for now
}

CK_DLL_DTOR(cgl_mat_dtor)  // all geos can share this base destructor
{
	Material* mat = (Material*)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CK_SAFE_DELETE(mat);
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = 0;  // zero out the memory

	// TODO: send destroy command to CGL command queue
	//       - remove material from scenegraph
	// 	     - callback hook for renderer to remove RenderMat from cache
}

CK_DLL_MFUN(cgl_mat_set_polygon_mode)
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	auto mode  = GET_NEXT_INT(ARGS);
	
	mat->SetPolygonMode((MaterialPolygonMode) mode);

	RETURN->v_int = mode;

	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::PolygonMode)));
}

CK_DLL_MFUN(cgl_mat_get_polygon_mode)
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	RETURN->v_int = mat->GetPolygonMode();
}

// point size setter
CK_DLL_MFUN(cgl_mat_set_point_size)
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	auto size = GET_NEXT_FLOAT(ARGS);
	mat->SetPointSize(size);

CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::POINT_SIZE_UNAME)));
}

// line width setter
// CK_DLL_MFUN(cgl_mat_set_line_width)
// {
// 	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
// 	auto width = GET_NEXT_FLOAT(ARGS);
// 	mat->SetLineWidth(width);

// 	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::LineWidth)));
// }

// TODO: can refactor these uniform setters to call a shared function
CK_DLL_MFUN( cgl_mat_set_uniform_float )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	float value = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_float2 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_float3 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_float4 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value3 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_int )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	int value = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_int2 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_int3 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN( cgl_mat_set_uniform_int4 )
{
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value3 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_bool) {
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	bool value = static_cast<bool>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_texID) {
	Material* mat = (Material*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* name = GET_NEXT_STRING(ARGS);
	CGL_Texture* tex = (CGL_Texture*) OBJ_MEMBER_INT (GET_NEXT_OBJECT(ARGS), cgltexture_data_offset);

	MaterialUniform uniform = MaterialUniform::Create(name->str(), tex->GetID());

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}


CK_DLL_CTOR(cgl_mat_norm_ctor)
{
	std::cerr << "cgl_mat_norm_ctor";
	NormalMaterial* normMat = new NormalMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT) normMat;
	std::cerr << "finished initializing norm material\n";

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(normMat));
}

CK_DLL_MFUN(cgl_set_use_local_normals)
{
	NormalMaterial* mat = (NormalMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	t_CKINT use_local = GET_NEXT_INT(ARGS);
	if (use_local)
		mat->UseLocalNormals();
	else
		mat->UseWorldNormals();
	// TODO: add command for this 

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(NormalMaterial::USE_LOCAL_NORMALS_UNAME)))
	);
}

// phong mat fns
CK_DLL_CTOR(cgl_mat_phong_ctor)
{
	PhongMaterial* phongMat = new PhongMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT) phongMat;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(phongMat));
}

CK_DLL_MFUN( cgl_mat_phong_set_diffuse_map )
{
	PhongMaterial* mat = (PhongMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	CGL_Texture* tex = (CGL_Texture*) OBJ_MEMBER_INT (GET_NEXT_OBJECT(ARGS), cgltexture_data_offset);
	mat->SetDiffuseMap(tex);

	// TODO: how do I return the chuck texture object? 
	// RETURN->v_object = SELF;

	// a lot of redundant work (entire uniform vector is copied). can optimize later
	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::DIFFUSE_MAP_UNAME)
	));
}

CK_DLL_MFUN( cgl_mat_phong_set_specular_map )
{
	PhongMaterial* mat = (PhongMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	CGL_Texture* tex = (CGL_Texture*) OBJ_MEMBER_INT (GET_NEXT_OBJECT(ARGS), cgltexture_data_offset);
	mat->SetSpecularMap(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_MAP_UNAME)
	));
}


CK_DLL_MFUN( cgl_mat_phong_set_diffuse_color )
{
	PhongMaterial* mat = (PhongMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetDiffuseColor(color.x, color.y, color.z);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::DIFFUSE_COLOR_UNAME)
	));
}

CK_DLL_MFUN( cgl_mat_phong_set_specular_color )
{
	PhongMaterial* mat = (PhongMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetSpecularColor(color.x, color.y, color.z);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_COLOR_UNAME)
	));
}

CK_DLL_MFUN( cgl_mat_phong_set_log_shininess )
{
	PhongMaterial* mat = (PhongMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	t_CKFLOAT shininess = GET_NEXT_FLOAT(ARGS);
	mat->SetLogShininess(shininess);

	RETURN->v_float = shininess;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SHININESS_UNAME)
	));
}

// custom shader mat fns ---------------------------------
CK_DLL_CTOR( cgl_mat_custom_shader_ctor )
{
	ShaderMaterial* shaderMat = new ShaderMaterial("", "");
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT) shaderMat;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(shaderMat));
}

CK_DLL_MFUN( cgl_mat_custom_shader_set_shaders )
{
	ShaderMaterial* mat = (ShaderMaterial*) OBJ_MEMBER_INT (SELF, cglmat_data_offset);
	Chuck_String* vertPath = GET_NEXT_STRING(ARGS);
	Chuck_String* fragPath = GET_NEXT_STRING(ARGS);

	mat->SetShaderPaths(vertPath->str(), fragPath->str());

	CGL::PushCommand(new UpdateMaterialShadersCommand(mat));
}

// points mat fns ---------------------------------
CK_DLL_CTOR( cgl_mat_points_ctor )
{
	PointsMaterial* pointsMat = new PointsMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT) pointsMat;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(pointsMat));
}




//-----------------------------------------------------------------------------
// init_chugl_obj()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_obj(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL Object");

	// GGen =========================================
	QUERY->begin_class(QUERY, "GGen", "Object");
	QUERY->add_ctor(QUERY, cgl_obj_ctor);
	QUERY->add_dtor(QUERY, cgl_obj_dtor);
	ggen_data_offset = QUERY->add_mvar(QUERY, "int", "@ggen_data", false);

	QUERY->add_mfun(QUERY, cgl_obj_get_id, "int", "GetID");


	// transform getters ===========
	// get obj direction vectors in world space
	QUERY->add_mfun(QUERY, cgl_obj_get_right, "vec3", "GetRight");
	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "GetForward");
	QUERY->add_mfun(QUERY, cgl_obj_get_up, "vec3", "GetUp");

	QUERY->add_mfun(QUERY, cgl_obj_get_pos, "vec3", "GetPosition");
	QUERY->add_mfun(QUERY, cgl_obj_get_rot, "vec3", "GetRotation");
	QUERY->add_mfun(QUERY, cgl_obj_get_scale, "vec3", "GetScale");

	QUERY->add_mfun(QUERY, cgl_obj_get_world_pos, "vec3", "GetWorldPosition");

	// transform setters ===========
	QUERY->add_mfun(QUERY, cgl_obj_translate_by, "GGen", "TranslateBy");
	QUERY->add_arg(QUERY, "vec3", "trans_vec");

	QUERY->add_mfun(QUERY, cgl_obj_scale_by, "GGen", "ScaleBy");
	QUERY->add_arg(QUERY, "vec3", "scale_vec");

	QUERY->add_mfun(QUERY, cgl_obj_rot_on_local_axis, "GGen", "RotateOnLocalAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_on_world_axis, "GGen", "RotateOnWorldAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_x, "GGen", "RotateX");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_y, "GGen", "RotateY");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_z, "GGen", "RotateZ");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_pos_x, "GGen", "PosX");
	QUERY->add_arg(QUERY, "float", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_pos_y, "GGen", "PosY");
	QUERY->add_arg(QUERY, "float", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_pos_z, "GGen", "PosZ");
	QUERY->add_arg(QUERY, "float", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_lookat_vec3, "GGen", "LookAt");
	QUERY->add_arg(QUERY, "vec3", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_lookat_float, "GGen", "LookAt");
	QUERY->add_arg(QUERY, "float", "x");
	QUERY->add_arg(QUERY, "float", "y");
	QUERY->add_arg(QUERY, "float", "z");

	QUERY->add_mfun(QUERY, cgl_obj_set_pos, "GGen", "SetPosition");
	QUERY->add_arg(QUERY, "vec3", "pos_vec");

	QUERY->add_mfun(QUERY, cgl_obj_set_rot, "GGen", "SetRotation");  // sets from eulers
	QUERY->add_arg(QUERY, "vec3", "eulers");

	QUERY->add_mfun(QUERY, cgl_obj_set_scale, "GGen", "SetScale");
	QUERY->add_arg(QUERY, "vec3", "scale");


	// scenegraph relationship methods ===========
	QUERY->add_mfun(QUERY, cgl_obj_add_child, "void", "AddChild");  
	QUERY->add_arg(QUERY, "GGen", "child");
    
    // overload GGen --> GGen
    QUERY->add_op_overload_binary( QUERY, ggen_op_gruck, "GGen", "-->",
                                   "GGen", "lhs", "GGen", "rhs" );

	QUERY->end_class(QUERY);

	return true;
}

// CGLObject DLL ==============================================
CK_DLL_CTOR(cgl_obj_ctor)
{
	// no ctor, meant to be abstract class
}
CK_DLL_DTOR(cgl_obj_dtor)
{
	SceneGraphObject* cglObj = (SceneGraphObject*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(cglObj);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;
}

CK_DLL_MFUN(cgl_obj_get_id)
{
	SceneGraphObject* cglObj = (SceneGraphObject*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_int = cglObj->GetID();
}

CK_DLL_MFUN(cgl_obj_get_right)
{
	SceneGraphObject* cglObj = (SceneGraphObject*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto& right = cglObj->GetRight();
	RETURN->v_vec3 = { right.x, right.y, right.z };
}
CK_DLL_MFUN(cgl_obj_get_forward)
{
	SceneGraphObject* cglObj = (SceneGraphObject*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto& vec = cglObj->GetForward();
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(cgl_obj_get_up)
{
	SceneGraphObject* cglObj = (SceneGraphObject*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto& vec = cglObj->GetUp();
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}


CK_DLL_MFUN(cgl_obj_translate_by)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 trans = GET_NEXT_VEC3(ARGS);
	cglObj->Translate(glm::vec3(trans.x, trans.y, trans.z));

	// add to command queue
	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_scale_by)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->Scale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_on_local_axis)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnLocalAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_on_world_axis)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnWorldAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_x)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateX(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_y)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateY(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_z)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateZ(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_x)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT posX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.x = posX;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_y)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT posY= GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.y = posY;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_z)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT posZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.z = posZ;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_lookat_vec3)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->LookAt(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_lookat_float)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKFLOAT x = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT y = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT z = GET_NEXT_FLOAT(ARGS);
	cglObj->LookAt(glm::vec3(x, y, z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_pos)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_rot)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetRotation(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_scale)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	const auto& vec = cglObj->GetPosition();
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(cgl_obj_get_world_pos)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	const auto& vec = cglObj->GetWorldPosition();
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(cgl_obj_get_rot)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	const auto& vec = glm::degrees(glm::eulerAngles(cglObj->GetRotation()));
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(cgl_obj_get_scale)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	const auto& vec = cglObj->GetScale();
	RETURN->v_vec3 = { vec.x, vec.y, vec.z };
}

CK_DLL_MFUN(cgl_obj_add_child)
{
	SceneGraphObject* cglObj = (SceneGraphObject*) OBJ_MEMBER_INT (SELF, ggen_data_offset);
	Chuck_Object* child_obj = GET_NEXT_OBJECT(ARGS);
	SceneGraphObject* child = (SceneGraphObject*) OBJ_MEMBER_INT (child_obj, ggen_data_offset);
	cglObj->AddChild(child);

	// command
	CGL::PushCommand(new AddChildCommand(cglObj, child));
}

CK_DLL_GFUN(ggen_op_gruck)
{
    // get the arguments
    Chuck_Object * lhs = GET_NEXT_OBJECT(ARGS);
    Chuck_Object * rhs = GET_NEXT_OBJECT(ARGS);
    // get internal representation
    SceneGraphObject * LHS = (SceneGraphObject*) OBJ_MEMBER_INT (lhs, ggen_data_offset);
    SceneGraphObject * RHS = (SceneGraphObject*) OBJ_MEMBER_INT (rhs, ggen_data_offset);
    // add child
    RHS->AddChild(LHS);

    // command
    CGL::PushCommand(new AddChildCommand(RHS, LHS));
    
    // return RHS
    RETURN->v_object = rhs;
}


//-----------------------------------------------------------------------------
// init_chugl_scene()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_scene(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");
	// CGL scene
	QUERY->begin_class(QUERY, "CglScene", "GGen");
	QUERY->add_ctor(QUERY, cgl_scene_ctor);
	QUERY->add_dtor(QUERY, cgl_scene_dtor);
	QUERY->end_class(QUERY);

	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_scene_ctor)
{
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) &CGL::mainScene;
}
CK_DLL_DTOR(cgl_scene_dtor)
{
	Scene* mainScene = (Scene*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// don't call delete! because this is a static var
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory
}

//-----------------------------------------------------------------------------
// init_chugl_cam()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_cam(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL Camera");
	// CGL camera
	QUERY->begin_class(QUERY, "CglCamera", "GGen");
	QUERY->add_ctor(QUERY, cgl_cam_ctor);
	QUERY->add_dtor(QUERY, cgl_cam_dtor);
	// cglcamera_data_offset = QUERY->add_mvar(QUERY, "int", "@cglcamera_data", false);
	QUERY->end_class(QUERY);

	return true;
}

// CGL Camera =======================
CK_DLL_CTOR(cgl_cam_ctor)
{
	// store reference to main camera 
	// NOT A BUG: camera inherits methods from cglobject, so it needs 
	// to use the same offset. wtf!!
	// TODO: ask Ge is this is the right way to do inheritence in this DLL interface
	//OBJ_MEMBER_INT(SELF, cglcamera_data_offset) = (t_CKINT) &CGL::mainCamera;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) &CGL::mainCamera;
}
CK_DLL_DTOR(cgl_cam_dtor)
{
	Camera* mainCam = (Camera*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// don't call delete! because this is a static var
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory
}

//-----------------------------------------------------------------------------
// init_chugl_mesh()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mesh(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");

	QUERY->begin_class(QUERY, "CglMesh", "GGen");
	QUERY->add_ctor(QUERY, cgl_mesh_ctor);
	QUERY->add_dtor(QUERY, cgl_mesh_dtor);
	
	QUERY->add_mfun(QUERY, cgl_mesh_set, "void", "set");
	QUERY->add_arg(QUERY, "CglGeo", "geo");
	QUERY->add_arg(QUERY, "CglMat", "mat");

	QUERY->end_class(QUERY);

	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_mesh_ctor)
{
	Mesh* mesh = new Mesh();
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) mesh;
	CGL::PushCommand(new CreateMeshCommand(mesh));
}

CK_DLL_DTOR(cgl_mesh_dtor)
{
	Mesh* mesh = (Mesh*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(mesh);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory

	// TODO: need to remove from scenegraph
}

CK_DLL_MFUN(cgl_mesh_set)
{
	Mesh* mesh = (Mesh*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
    //Geometry * geo = (Geometry *)GET_NEXT_OBJECT(ARGS);
    //Material * mat = (Material *)GET_NEXT_OBJECT(ARGS);
    Chuck_Object* geo_obj = GET_NEXT_OBJECT(ARGS);
    Chuck_Object* mat_obj = GET_NEXT_OBJECT(ARGS); 
    Geometry* geo = (Geometry *)OBJ_MEMBER_INT( geo_obj, cglgeo_data_offset );
    Material* mat = (Material *)OBJ_MEMBER_INT( mat_obj, cglmat_data_offset);
	
	// set on CGL side
	mesh->SetGeometry(geo);
	mesh->SetMaterial(mat);

	// command queue to update renderer side
	CGL::PushCommand(new SetMeshCommand(mesh));
}

//-----------------------------------------------------------------------------
// init_chugl_group()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_group(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL group");

	// CGL Group
	QUERY->begin_class(QUERY, "CglGroup", "GGen");
	QUERY->add_ctor(QUERY, cgl_group_ctor);
	QUERY->add_dtor(QUERY, cgl_group_dtor);
	QUERY->end_class(QUERY);

	return true;
}
// CGL Group 
CK_DLL_CTOR(cgl_group_ctor)
{
	Group* group = new Group;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) group;
	CGL::PushCommand(new CreateGroupCommand(group));
}

CK_DLL_DTOR(cgl_group_dtor)
{
	Group* group = (Group*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(group);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory

	// TODO: need to remove from scenegraph
}

//-----------------------------------------------------------------------------
// init_chugl_light()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_light(Chuck_DL_Query* QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL geometry");

	// geometry
	QUERY->begin_class(QUERY, "Light", "GGen");
	QUERY->add_ctor(QUERY, cgl_light_ctor);
	QUERY->add_dtor(QUERY, cgl_light_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "PointLight", "Light");
	QUERY->add_ctor(QUERY, cgl_point_light_ctor);
	QUERY->add_dtor(QUERY, cgl_light_dtor);
	// QUERY->add_mfun(QUERY, cgl_geo_box_set, "void", "set");
	// QUERY->add_arg(QUERY, "float", "width");
	// QUERY->add_arg(QUERY, "float", "height");
	// QUERY->add_arg(QUERY, "float", "depth");
	// QUERY->add_arg(QUERY, "int", "widthSeg");
	// QUERY->add_arg(QUERY, "int", "heightSeg");
	// QUERY->add_arg(QUERY, "int", "depthSeg");
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "DirLight", "Light");
	QUERY->add_ctor(QUERY, cgl_dir_light_ctor);
	QUERY->add_dtor(QUERY, cgl_light_dtor);
	QUERY->end_class(QUERY);

	return true;
}


// CGL Lights ===================================================

CK_DLL_CTOR(cgl_light_ctor)
{
	// abstract class. nothing to do
}

CK_DLL_DTOR(cgl_light_dtor)
{
	Group* group = (Group*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(group);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory

	// TODO: need to remove from scenegraph with a destroy command
}

CK_DLL_CTOR(cgl_point_light_ctor)
{
	PointLight* light = new PointLight;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) light;
	CGL::PushCommand(new CreateLightCommand(light));
}

CK_DLL_CTOR(cgl_dir_light_ctor)
{
	DirLight* light = new DirLight;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT) light;
	CGL::PushCommand(new CreateLightCommand(light));
}

//-----------------------------------------------------------------------------
// ChuGL Event impl (TODO refactor into separate file)
//-----------------------------------------------------------------------------
// CglEventstatic initialization (again, should be refactored to be accessible through chuck.h)
std::vector<CglEvent*> CglEvent::m_FrameEvents;
std::vector<CglEvent*> CglEvent::m_UpdateEvents;
std::vector<CglEvent*> CglEvent::m_WindowResizeEvents;
CBufferSimple* CglEvent::s_SharedEventQueue;

std::vector<CglEvent*>& CglEvent::GetEventQueue(CglEventType type)
{
	switch (type) {
	case CglEventType::CGL_UPDATE:			return m_UpdateEvents;
	case CglEventType::CGL_FRAME:			return m_FrameEvents;
	case CglEventType::CGL_WINDOW_RESIZE:	return m_WindowResizeEvents;
	default:								throw std::runtime_error("invalid CGL event type");
	}
	
}

/*
void CglEvent::wait(Chuck_VM_Shred* shred)
{
	m_Event->wait(shred, m_VM);
}
*/

CglEvent::CglEvent(
	Chuck_Event* event, Chuck_VM* vm, CK_DL_API api, CglEventType event_type
)
	: m_Event(event), m_VM(vm), m_API(api), m_EventType(event_type)
{
	GetEventQueue(event_type).push_back(this);
}

CglEvent::~CglEvent()
{
	auto& eventQueue = GetEventQueue(m_EventType);

	// remove from listeners list
	auto it = std::find(eventQueue.begin(), eventQueue.end(), this);
	assert(it != eventQueue.end());  // sanity check
	if (it != eventQueue.end())
		eventQueue.erase(it);
}

void CglEvent::Broadcast()
{	
	// (should be) threadsafe
	m_API->vm->queue_event(m_VM, m_Event, 1, s_SharedEventQueue);
	

	// using non-thread-safe event buffer for now
	// m_VM->queue_event(m_Event, 1, NULL);
	// m_Event->queue_broadcast();
}

// broadcasts all events of type event_type
void CglEvent::Broadcast(CglEventType event_type)
{
	auto& eventQueue = GetEventQueue(event_type);
	for (auto& event : eventQueue)
		event->Broadcast();
}
//-----------------------------------------------------------------------------
// ChuGL synchronization impl (TODO refactor into separate file and probability split this class up)
//-----------------------------------------------------------------------------

// CGL static initialization
bool CGL::shouldRender = false;
std::mutex CGL::GameLoopLock;
std::condition_variable CGL::renderCondition;
Scene CGL::mainScene;
SceneGraphObject CGL::mainCamera;
// Chuck_Event CGL::s_UpdateChuckEvent;

// Initialization for Shred Registration structures
std::unordered_set<Chuck_VM_Shred*> CGL::m_RegisteredShreds;
std::vector<Chuck_VM_Shred*> CGL::m_WaitingShreds;
std::unordered_map<Chuck_VM_Shred*, Chuck_DL_Api::Object> CGL::m_ShredEventMap;  // map of shreds to CglUpdate Event 

// CGL static command queue initialization
std::vector<SceneGraphCommand*> CGL::m_ThisCommandQueue;
std::vector<SceneGraphCommand*> CGL::m_ThatCommandQueue;
bool CGL::m_CQReadTarget = false;  // false = this, true = that
std::mutex CGL::m_CQLock; // only held when 1: adding new command and 2: swapping the read/write queues

// can pick a better name maybe...calling this wakes up renderer thread
void CGL::Render()
{
	std::unique_lock<std::mutex> lock(GameLoopLock);
	shouldRender = true;
	lock.unlock();

	renderCondition.notify_one();  // wakeup the blocking render thread
}

// sleep render thread until notified by chuck
void CGL::WaitOnUpdateDone() {
	std::unique_lock<std::mutex> lock(GameLoopLock);
	renderCondition.wait(lock, []() { return shouldRender; });
	shouldRender = false; 
	// lock auto releases in destructor
}

// swap the command queue double buffer
void CGL::SwapCommandQueues() { 
	// grab lock
	std::lock_guard<std::mutex> lock(m_CQLock);
	// swap
	m_CQReadTarget = !m_CQReadTarget;
	// lock released out of scope
}

// perform all queued commands to sync the renderer scenegraph with the CGL scenegraph
void CGL::FlushCommandQueue(Scene& scene, bool swap) {  // TODO: shouldn't command be associated with scenes?
	// swap the command queues (so we can read from what was just being written to)
	if (swap)
		SwapCommandQueues();  // Note: this already locks the command queue

	// we no longer need to hold a lock here because all writes are done to the other queue

	// get the new read queue
	std::vector<SceneGraphCommand*>& readQueue = GetReadCommandQueue();\
	// std::cout << "flushing " + std::to_string(readQueue.size()) + " commands\n";

	// execute all commands in the read queue
	for (auto& cmd : readQueue) {
		cmd->execute(&scene);
		delete cmd;  // release memory TODO make this a unique_ptr or something instead
	}

	readQueue.clear();
}

// adds command to the read queue
void CGL::PushCommand(SceneGraphCommand * cmd) {
	// lock the command queue
	std::lock_guard<std::mutex> lock(m_CQLock);

	// get the write queue
	std::vector<SceneGraphCommand*>& writeQueue = GetWriteCommandQueue();

	// add the command to the write queue
	writeQueue.push_back(cmd);
}

void CGL::RegisterShred(Chuck_VM_Shred *shred)
{
	m_RegisteredShreds.insert(shred);
}

void CGL::UnregisterShred(Chuck_VM_Shred *shred)
{
	m_RegisteredShreds.erase(shred);
}

bool CGL::IsShredRegistered(Chuck_VM_Shred *shred)
{
	return m_RegisteredShreds.find(shred) != m_RegisteredShreds.end();
}

size_t CGL::GetNumRegisteredShreds()
{
    return m_RegisteredShreds.size();
}

size_t CGL::GetNumShredsWaiting()
{
	return m_WaitingShreds.size();
}

void CGL::ClearShredWaiting() {
	m_WaitingShreds.clear();
}

void CGL::RegisterShredWaiting(Chuck_VM_Shred* shred)
{
	m_WaitingShreds.push_back(shred);
}


Chuck_DL_Api::Object CGL::GetCachedShredUpdateEvent(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM) { 
	// log size
	// std::cerr << "shred event map size: " + std::to_string(m_ShredEventMap.size()) << std::endl;
	// lookup
	if (m_ShredEventMap.find(shred) != m_ShredEventMap.end()) {
		return m_ShredEventMap[shred];
	} else {
		Chuck_DL_Api::Type type = API->object->get_type(API, shred, "CglUpdate");
		Chuck_DL_Api::Object obj = API->object->create(API, shred, type);
		cgl_update_ctor( (Chuck_Object*)obj, NULL, VM, shred, API );
		m_ShredEventMap[shred] = obj;
		return obj;
	}

}


