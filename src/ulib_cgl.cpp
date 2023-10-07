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
// ChuGL Event Listeners
//-----------------------------------------------------------------------------
CK_DLL_SHREDS_WATCHER(cgl_shred_on_destroy_listener);

//-----------------------------------------------------------------------------
// ChuGL Events
//-----------------------------------------------------------------------------
// update event
CK_DLL_CTOR(cgl_update_ctor);
CK_DLL_DTOR(cgl_update_dtor);
// window resize
CK_DLL_CTOR(cgl_window_resize_ctor);
CK_DLL_DTOR(cgl_window_resize_dtor);

CK_DLL_MFUN(cgl_update_event_waiting_on);

//-----------------------------------------------------------------------------
// Static Fns
//-----------------------------------------------------------------------------
CK_DLL_SFUN(cgl_next_frame);
CK_DLL_SFUN(cgl_register);
CK_DLL_SFUN(cgl_unregister);

// glfw-state
CK_DLL_SFUN(cgl_window_get_width);
CK_DLL_SFUN(cgl_window_get_height);
CK_DLL_SFUN(cgl_window_get_time);
CK_DLL_SFUN(cgl_window_get_dt);
CK_DLL_SFUN(cgl_mouse_get_pos_x);
CK_DLL_SFUN(cgl_mouse_get_pos_y);
CK_DLL_SFUN(cgl_mouse_set_mode);
CK_DLL_SFUN(cgl_mouse_hide);
CK_DLL_SFUN(cgl_mouse_lock);
CK_DLL_SFUN(cgl_mouse_show);
CK_DLL_SFUN(cgl_framebuffer_get_width);
CK_DLL_SFUN(cgl_framebuffer_get_height);

// glfw windowing fns
CK_DLL_SFUN(cgl_window_fullscreen);
CK_DLL_SFUN(cgl_window_windowed);
// CK_DLL_SFUN(cgl_window_maximize);
CK_DLL_SFUN(cgl_window_set_size);

// accessing shared default GGens
CK_DLL_SFUN(cgl_get_main_camera);
CK_DLL_SFUN(cgl_get_main_scene);

// chugl shred debug
CK_DLL_SFUN(cgl_get_num_registered_shreds);
CK_DLL_SFUN(cgl_get_num_registered_waiting_shreds);

// chugl toggle whether to use system time or chuck time
CK_DLL_SFUN(cgl_use_chuck_time);

//-----------------------------------------------------------------------------
// ChuGL Object
//-----------------------------------------------------------------------------
// *structors
CK_DLL_CTOR(cgl_obj_ctor);
CK_DLL_DTOR(cgl_obj_dtor);

CK_DLL_MFUN(cgl_obj_get_id);
CK_DLL_MFUN(cgl_obj_update);

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
// CK_DLL_MFUN(cgl_obj_disconnect);
// CK_DLL_MFUN(cgl_obj_get_parent);
// CK_DLL_MFUN(cgl_obj_get_children);

// TODO: these need reference counting
CK_DLL_GFUN(ggen_op_gruck);	  // add child
CK_DLL_GFUN(ggen_op_ungruck); // remove child

//-----------------------------------------------------------------------------
// Object -> BaseCamera
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_cam_ctor);
CK_DLL_DTOR(cgl_cam_dtor);

CK_DLL_MFUN(cgl_cam_set_mode_persp); // switch to perspective mode
CK_DLL_MFUN(cgl_cam_set_mode_ortho); // switch to perspective mode
CK_DLL_MFUN(cgl_cam_get_mode);		 // switch to perspective mode

CK_DLL_MFUN(cgl_cam_set_clip);
CK_DLL_MFUN(cgl_cam_get_clip_near);
CK_DLL_MFUN(cgl_cam_get_clip_far);

// perspective camera params
// (no aspect, that's set automatically by renderer window resize callback)
CK_DLL_MFUN(cgl_cam_set_pers_fov); // set in degrees
CK_DLL_MFUN(cgl_cam_get_pers_fov); //

// ortho camera params
CK_DLL_MFUN(cgl_cam_set_ortho_size); // size of view volume (preserves screen aspect ratio)
CK_DLL_MFUN(cgl_cam_get_ortho_size); //

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

//-----------------------------------------------------------------------------
// Object -> Light
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_light_ctor); // abstract base class, no constructor
CK_DLL_DTOR(cgl_light_dtor);

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
// Geometry
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_geo_ctor);
CK_DLL_DTOR(cgl_geo_dtor);
CK_DLL_MFUN(cgl_geo_clone);

// box
CK_DLL_CTOR(cgl_geo_box_ctor);
CK_DLL_MFUN(cgl_geo_box_set);

// sphere
CK_DLL_CTOR(cgl_geo_sphere_ctor);
CK_DLL_MFUN(cgl_geo_sphere_set);
// TODO: sphere parameter setter

// circle
CK_DLL_CTOR(cgl_geo_circle_ctor);
CK_DLL_MFUN(cgl_geo_circle_set);

// plane
CK_DLL_CTOR(cgl_geo_plane_ctor);
CK_DLL_MFUN(cgl_geo_plane_set);

// torus
CK_DLL_CTOR(cgl_geo_torus_ctor);
CK_DLL_MFUN(cgl_geo_torus_set);

// lathe
CK_DLL_CTOR(cgl_geo_lathe_ctor);
CK_DLL_MFUN(cgl_geo_lathe_set);
CK_DLL_MFUN(cgl_geo_lathe_set_no_points);

// custom
CK_DLL_CTOR(cgl_geo_custom_ctor);
CK_DLL_MFUN(cgl_geo_set_attribute); // general case for any kind of vertex data
CK_DLL_MFUN(cgl_geo_set_positions);
CK_DLL_MFUN(cgl_geo_set_positions_vec3);
CK_DLL_MFUN(cgl_geo_set_colors);
CK_DLL_MFUN(cgl_geo_set_normals);
CK_DLL_MFUN(cgl_geo_set_uvs);
CK_DLL_MFUN(cgl_geo_set_indices);

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
CK_DLL_MFUN(cgl_mat_clone);

// base material options
// TODO: move all material options down into base class?
CK_DLL_MFUN(cgl_mat_set_polygon_mode);
CK_DLL_MFUN(cgl_mat_get_polygon_mode);
CK_DLL_MFUN(cgl_mat_set_color);
CK_DLL_MFUN(cgl_mat_get_color);
CK_DLL_MFUN(cgl_mat_set_point_size);
CK_DLL_MFUN(cgl_mat_set_line_width);
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

// flat shade mat
// CK_DLL_CTOR(cgl_mat_flat_ctor);

// phong specular mat
CK_DLL_CTOR(cgl_mat_phong_ctor);
// uniform setters
CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map);
CK_DLL_MFUN(cgl_mat_phong_set_specular_map);
CK_DLL_MFUN(cgl_mat_phong_set_specular_color);
CK_DLL_MFUN(cgl_mat_phong_set_log_shininess);
// uniform getters TODO

// custom shader mat
CK_DLL_CTOR(cgl_mat_custom_shader_ctor);
CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders);

// points mat
CK_DLL_CTOR(cgl_mat_points_ctor);

CK_DLL_MFUN(cgl_mat_points_set_size_attenuation);
CK_DLL_MFUN(cgl_mat_points_get_size_attenuation);

CK_DLL_MFUN(cgl_mat_points_set_sprite);

// mango mat (for debugging UVs)
CK_DLL_CTOR(cgl_mat_mango_ctor);

// basic line mat (note: line rendering is not well supported on modern OpenGL)
// most hardware doesn't support variable line width
// "using the build-in OpenGL functionality for this task is very limited, if working at all."
// for a better soln using texture-buffer line meshes, see: https://github.com/mhalber/Lines#texture-buffer-lines
CK_DLL_CTOR(cgl_mat_line_ctor);
CK_DLL_MFUN(cgl_mat_line_set_mode);	 // many platforms only support fixed width 1.0

//-----------------------------------------------------------------------------
// Object -> Mesh
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_mesh_ctor);
CK_DLL_DTOR(cgl_mesh_dtor);
CK_DLL_MFUN(cgl_mesh_set);
CK_DLL_MFUN(cgl_mesh_get_mat);
CK_DLL_MFUN(cgl_mesh_get_geo);
CK_DLL_MFUN(cgl_mesh_set_mat);
CK_DLL_MFUN(cgl_mesh_set_geo);

// duplicators
CK_DLL_MFUN(cgl_mesh_dup_mat);
CK_DLL_MFUN(cgl_mesh_dup_geo);
CK_DLL_MFUN(cgl_mesh_dup_all);

//-----------------------------------------------------------------------------
// Object -> Mesh -> Gxxxxx
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_gcube_ctor);
// CK_DLL_DTOR(cgl_gcube_dtor);

CK_DLL_CTOR(cgl_gsphere_ctor);
CK_DLL_CTOR(cgl_gcircle_ctor);
CK_DLL_CTOR(cgl_gplane_ctor);
CK_DLL_CTOR(cgl_gtorus_ctor);

CK_DLL_CTOR(cgl_glines_ctor);

CK_DLL_CTOR(cgl_gpoints_ctor);



//-----------------------------------------------------------------------------
// Object -> Group
//-----------------------------------------------------------------------------
CK_DLL_CTOR(cgl_group_ctor);
CK_DLL_DTOR(cgl_group_dtor);

// exports =========================================

t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_geo(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_texture(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_mat(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_obj(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_cam(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_scene(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_group(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_mesh(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_light(Chuck_DL_Query *QUERY);
t_CKBOOL create_chugl_default_objs(Chuck_DL_Query *QUERY);

static t_CKUINT cglframe_data_offset = 0;
static t_CKUINT cgl_next_frame_event_data_offset = 0;
static t_CKUINT window_resize_event_data_offset = 0;
static t_CKUINT ggen_data_offset = 0;
static t_CKUINT gcamera_data_offset = 0;
static t_CKUINT geometry_data_offset = 0;
static t_CKUINT texture_data_offset = 0;
static t_CKUINT cglmat_data_offset = 0;

t_CKBOOL init_chugl(Chuck_DL_Query *QUERY)
{
	init_chugl_events(QUERY);
	init_chugl_geo(QUERY);
	init_chugl_texture(QUERY);
	init_chugl_mat(QUERY);
	init_chugl_obj(QUERY);
	init_chugl_cam(QUERY);
	init_chugl_group(QUERY);
	init_chugl_mesh(QUERY);
	init_chugl_light(QUERY);
	init_chugl_scene(QUERY);
	create_chugl_default_objs(QUERY);
	init_chugl_static_fns(QUERY);

	return true;
}

//-----------------------------------------------------------------------------
// create_chugl_default_objs()
//-----------------------------------------------------------------------------
t_CKBOOL create_chugl_default_objs(Chuck_DL_Query *QUERY)
{
	// threadsafe event queue
	CglEvent::s_SharedEventQueue = QUERY->api()->vm->create_event_buffer(
		QUERY->vm());
	assert(CglEvent::s_SharedEventQueue);

	// main camera
	// Chuck_DL_Api::Type type = QUERY->api()->type->lookup(QUERY->api(), NULL, "GCamera");
	// Chuck_DL_Api::Object obj = QUERY->api()->object->create(QUERY->api(), NULL, type);

	// shred destroy listener
	QUERY->register_shreds_watcher(QUERY, cgl_shred_on_destroy_listener, CKVM_SHREDS_WATCH_REMOVE, NULL);

	// update() vt offset
    // get the GGen type
    Chuck_Type * t_ggen = QUERY->api()->type->lookup(QUERY->vm(), "GGen");
    // find the offset for update
    CGL::our_update_vt_offset = QUERY->api()->type->get_vtable_offset(QUERY->vm(), t_ggen, "update");
	
	return true;
}

CK_DLL_SHREDS_WATCHER(cgl_shred_on_destroy_listener)
{
	// called when shred is taken out of circulation for any reason
	// e.g. reached end, removed, VM cleared, etc.
	assert(CODE == CKVM_SHREDS_WATCH_REMOVE);

	// remove from registered list
	CGL::UnregisterShred(SHRED);

	// remove from waiting list
	CGL::UnregisterShredWaiting(SHRED);
}

//-----------------------------------------------------------------------------
// init_chugl_events()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY)
{
	// Update event ================================
	// triggered by main render thread after deepcopy is complete, and safe for chuck to begin updating the scene graph
	QUERY->begin_class(QUERY, "NextFrameEvent", "Event");
	QUERY->add_ctor(QUERY, cgl_update_ctor);
	QUERY->add_dtor(QUERY, cgl_update_dtor);
	cgl_next_frame_event_data_offset = QUERY->add_mvar(QUERY, "int", "@cgl_next_frame_event_data", false);

	QUERY->add_mfun(QUERY, cgl_update_event_waiting_on, "void", "waiting_on");
	QUERY->end_class(QUERY);

	// Window resize event ================================
	QUERY->begin_class(QUERY, "WindowResizeEvent", "Event");
	QUERY->add_ctor(QUERY, cgl_window_resize_ctor);
	QUERY->add_dtor(QUERY, cgl_window_resize_dtor);

	window_resize_event_data_offset = QUERY->add_mvar(QUERY, "int", "@cgl_window_resize_event_data", false);
	QUERY->end_class(QUERY);

	return true;
}

CK_DLL_CTOR(cgl_update_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset) = (t_CKINT) new CglEvent(
		(Chuck_Event *)SELF, SHRED->vm_ref, API, CglEventType::CGL_UPDATE);
}
CK_DLL_DTOR(cgl_update_dtor)
{
	CglEvent *cglEvent = (CglEvent *)OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset);
	CK_SAFE_DELETE(cglEvent);
	OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset) = 0;
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
	CglEvent *cglEvent = (CglEvent *)OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset);

	// activate chugl main thread hook (no-op if already activated)
	CGL::ActivateHook();

	// Add shred (no-op if already added)
	// CGL::RegisterShred(SHRED);

	// flips the boolean flag for this shred to true, meaning it has successfully
	CGL::MarkShredWaited(SHRED);

	// Add shred to waiting list
	CGL::RegisterShredWaiting(SHRED);
	// std::cerr << "REGISTERED SHRED WAITING" << SHRED << std::endl;

	// if #waiting == #registered, all CGL shreds have finished work, and we are safe to wakeup the renderer
	if (CGL::GetNumShredsWaiting() >= CGL::GetNumRegisteredShreds())
	{

		// if this is very first time calling, set initial dt
		// this allows chuck time and window dt to be totally synced!!
		// TODO: ask ge how to we get now and dt through dl API?

		// traverse scenegraph and call chuck-defined update() on all GGens
		CGL::UpdateSceneGraph(CGL::mainScene, API, VM, SHRED);

		CGL::ClearShredWaiting(); // clear thread waitlist
		CGL::Render();
	}
}

// window resize event
CK_DLL_CTOR(cgl_window_resize_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, window_resize_event_data_offset) = (t_CKINT) new CglEvent(
		(Chuck_Event *)SELF, SHRED->vm_ref, API, CglEventType::CGL_WINDOW_RESIZE);
}
CK_DLL_DTOR(cgl_window_resize_dtor)
{
	CglEvent *cglEvent = (CglEvent *)OBJ_MEMBER_INT(SELF, window_resize_event_data_offset);
	CK_SAFE_DELETE(cglEvent);
	OBJ_MEMBER_INT(SELF, window_resize_event_data_offset) = 0;
}

//-----------------------------------------------------------------------------
// init_chugl_static_fns()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, "GG", "Object"); // for global stuff
											   // static vars
											   // This will hide the cursor and lock it to the specified window.
	QUERY->add_svar(QUERY, "int", "MOUSE_LOCKED", TRUE, (void *)&CGL::MOUSE_LOCKED);
	// the cursor to become hidden when it is over a window but still want it to behave normally
	QUERY->add_svar(QUERY, "int", "MOUSE_HIDDEN", TRUE, (void *)&CGL::MOUSE_HIDDEN);
	// This mode puts no limit on the motion of the cursor.
	QUERY->add_svar(QUERY, "int", "MOUSE_NORMAL", TRUE, (void *)&CGL::MOUSE_NORMAL);

	QUERY->add_sfun(QUERY, cgl_next_frame, "NextFrameEvent", "nextFrame");

	QUERY->add_sfun(QUERY, cgl_unregister, "void", "unregister");
	QUERY->add_sfun(QUERY, cgl_register, "void", "register");

	// window state getters
	QUERY->add_sfun(QUERY, cgl_window_get_width, "int", "windowWidth");
	QUERY->add_sfun(QUERY, cgl_window_get_height, "int", "windowHeight");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_width, "int", "frameWidth");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_height, "int", "frameHeight");
	QUERY->add_sfun(QUERY, cgl_window_get_time, "dur", "windowUptime");
	QUERY->add_sfun(QUERY, cgl_window_get_dt, "float", "dt");
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_x, "float", "mouseX");
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_y, "float", "mouseY");
	QUERY->add_sfun(QUERY, cgl_mouse_set_mode, "void", "mouseMode");
	QUERY->add_arg(QUERY, "int", "mode");

	QUERY->add_sfun(QUERY, cgl_mouse_hide, "void", "hideCursor");
	QUERY->add_sfun(QUERY, cgl_mouse_lock, "void", "lockCursor");
	QUERY->add_sfun(QUERY, cgl_mouse_show, "void", "showCursor");

	QUERY->add_sfun(QUERY, cgl_window_fullscreen, "void", "fullscreen");
	QUERY->add_sfun(QUERY, cgl_window_windowed, "void", "windowed");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");

	// QUERY->add_sfun(QUERY, cgl_window_maximize, "void", "maximize");  // kind of bugged, not sure how this is different from fullscreen
	QUERY->add_sfun(QUERY, cgl_window_set_size, "void", "resolution");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");

	// Main Camera
	// TODO: is it possible to add an svar of type GCamera?
	QUERY->add_sfun(QUERY, cgl_get_main_camera, "GCamera", "camera");

	// Main scene
	QUERY->add_sfun(QUERY, cgl_get_main_scene, "GScene", "scene");

	// chugl shred debug
	QUERY->add_sfun(QUERY, cgl_get_num_registered_shreds, "int", "numRegisteredShreds");
	QUERY->add_sfun(QUERY, cgl_get_num_registered_waiting_shreds, "int", "numRegisteredWaitingShreds");

	// chugl chuck time for auto update(dt)
	QUERY->add_sfun(QUERY, cgl_use_chuck_time, "void", "useChuckTime");
	QUERY->add_arg(QUERY, "int", "use");

	QUERY->end_class(QUERY);

	return true;
}
/*============CGL static fns============*/
CK_DLL_SFUN(cgl_next_frame)
{

	// extract CglEvent from obj
	// TODO: workaround bug where create() object API is not calling preconstructors
	// https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate

	if (!CGL::HasShredWaited(SHRED))
	{
		API->vm->throw_exception(
			"NextFrameNotWaitedOnViolation",
			"You are calling .nextFrame() without chucking to now!\n"
			"Please replace this line with .nextFrame() => now;",
			SHRED);
	}

	RETURN->v_object = (Chuck_Object *)CGL::GetShredUpdateEvent(
		SHRED, API, VM);

	// register shred and set has-waited flag to false
	CGL::RegisterShred(SHRED);
}

CK_DLL_SFUN(cgl_register) { CGL::RegisterShred(SHRED); }
CK_DLL_SFUN(cgl_unregister) { CGL::UnregisterShred(SHRED); }

// get window width
CK_DLL_SFUN(cgl_window_get_width) { RETURN->v_int = CGL::GetWindowSize().first; }
// get window height
CK_DLL_SFUN(cgl_window_get_height) { RETURN->v_int = CGL::GetWindowSize().second; }
// get framebuffer width
CK_DLL_SFUN(cgl_framebuffer_get_width) { RETURN->v_int = CGL::GetFramebufferSize().first; }
// get framebuffer height
CK_DLL_SFUN(cgl_framebuffer_get_height) { RETURN->v_int = CGL::GetFramebufferSize().second; }
// get glfw time
CK_DLL_SFUN(cgl_window_get_time) { RETURN->v_dur = API->vm->srate(VM) * CGL::GetTimeInfo().first; }
// get glfw dt
CK_DLL_SFUN(cgl_window_get_dt) { RETURN->v_float = CGL::GetTimeInfo().second; }
// get mouse x
CK_DLL_SFUN(cgl_mouse_get_pos_x) { RETURN->v_float = CGL::GetMousePos().first; }
// get mouse y
CK_DLL_SFUN(cgl_mouse_get_pos_y) { RETURN->v_float = CGL::GetMousePos().second; }

// set mouse mode
CK_DLL_SFUN(cgl_mouse_set_mode)
{
	t_CKINT mode = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetMouseModeCommand(mode));
}

// hide mouse
CK_DLL_SFUN(cgl_mouse_hide) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_HIDDEN)); }
// lock mouse
CK_DLL_SFUN(cgl_mouse_lock) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_LOCKED)); }
// show mouse
CK_DLL_SFUN(cgl_mouse_show) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_NORMAL)); }

// set fullscreen
CK_DLL_SFUN(cgl_window_fullscreen) { CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_FULLSCREEN)); }
// set windowed
CK_DLL_SFUN(cgl_window_windowed)
{
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_WINDOWED, width, height));
}
// set maximize
// CK_DLL_SFUN(cgl_window_maximize) { CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_MAXIMIZED)); }

// QUERY->add_sfun(QUERY, cgl_window_windowed, "void", "windowed");
// set windowsize
CK_DLL_SFUN(cgl_window_set_size)
{
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_SET_SIZE, width, height));
}

CK_DLL_SFUN(cgl_get_main_camera)
{
	// RETURN->v_object = (Chuck_Object *)CGL::GetMainCamera(
	// 	SHRED, API, VM);
	Scene *scene = (Scene *)OBJ_MEMBER_INT(CGL::GetMainScene(SHRED, API, VM), ggen_data_offset);
	RETURN->v_object = scene->GetMainCamera()->m_ChuckObject;
}

CK_DLL_SFUN(cgl_get_main_scene)
{
	RETURN->v_object = (Chuck_Object *)CGL::GetMainScene(
		SHRED, API, VM);
}

CK_DLL_SFUN(cgl_get_num_registered_shreds)
{
	RETURN->v_int = CGL::GetNumRegisteredShreds();
}
CK_DLL_SFUN(cgl_get_num_registered_waiting_shreds)
{
	RETURN->v_int = CGL::GetNumShredsWaiting();
}

CK_DLL_SFUN(cgl_use_chuck_time)
{
	CGL::useChuckTime = GET_NEXT_INT(ARGS) != 0;
}

//-----------------------------------------------------------------------------
// init_chugl_geo()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_geo(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Base) , "Object");
	QUERY->add_ctor(QUERY, cgl_geo_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);
	geometry_data_offset = QUERY->add_mvar(QUERY, "int", "@geometry_data", false); // TODO: still bugged?

	// clone
	QUERY->add_mfun(QUERY, cgl_geo_clone, Geometry::CKName(GeometryType::Base), "clone");

	// TODO: add svars for attribute locations
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Box), Geometry::CKName(GeometryType::Base) );
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

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Sphere), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_sphere_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_sphere_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");

	QUERY->end_class(QUERY);

	// circle geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Circle), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_circle_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_circle_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
	QUERY->end_class(QUERY);

	// plane geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Plane), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_plane_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_plane_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "int", "widthSegments");
	QUERY->add_arg(QUERY, "int", "heightSegments");
	QUERY->end_class(QUERY);

	// Torus geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Torus), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_torus_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_torus_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "float", "tubeRadius");
	QUERY->add_arg(QUERY, "int", "radialSegments");
	QUERY->add_arg(QUERY, "int", "tubularSegments");
	QUERY->add_arg(QUERY, "float", "arcLength");
	QUERY->end_class(QUERY);

	// lathe geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Lathe), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_lathe_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set, "void", "set");
	QUERY->add_arg(QUERY, "float[]", "points"); // these are converted to vec2s
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set_no_points, "void", "set");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");

	QUERY->end_class(QUERY);

	// custom geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Custom), Geometry::CKName(GeometryType::Base) );
	QUERY->add_ctor(QUERY, cgl_geo_custom_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);

	QUERY->add_mfun(QUERY, cgl_geo_set_attribute, "void", "setAttribute");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "location");
	QUERY->add_arg(QUERY, "int", "numComponents");
	// QUERY->add_arg(QUERY, "int", "normalize");
	QUERY->add_arg(QUERY, "float[]", "data");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions, "void", "positions");
	QUERY->add_arg(QUERY, "float[]", "positions");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions_vec3, "void", "positions");
	QUERY->add_arg(QUERY, "vec3[]", "positions");

	QUERY->add_mfun(QUERY, cgl_geo_set_colors, "void", "colors");
	QUERY->add_arg(QUERY, "float[]", "colors");

	QUERY->add_mfun(QUERY, cgl_geo_set_normals, "void", "normals");
	QUERY->add_arg(QUERY, "float[]", "normals");

	QUERY->add_mfun(QUERY, cgl_geo_set_uvs, "void", "uvs");
	QUERY->add_arg(QUERY, "float[]", "uvs");

	QUERY->add_mfun(QUERY, cgl_geo_set_indices, "void", "indices");
	QUERY->add_arg(QUERY, "int[]", "uvs");

	QUERY->end_class(QUERY);

	return true;
}

// CGL Geometry =======================
CK_DLL_CTOR(cgl_geo_ctor) {}
CK_DLL_DTOR(cgl_geo_dtor) // all geos can share this base destructor
{
	Geometry *geo = (Geometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	CK_SAFE_DELETE(geo);
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = 0; // zero out the memory

	// TODO: trigger destruction callback and scenegraph removal command
}

CK_DLL_MFUN(cgl_geo_clone)
{
	Geometry *geo = (Geometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	RETURN->v_object = CGL::CreateChuckObjFromGeo(API, VM, geo->Clone(), SHRED, false)->m_ChuckObject;
}


// box geo
CK_DLL_CTOR(cgl_geo_box_ctor)
{
	std::cerr << "cgl_box_ctor\n";
	BoxGeometry *boxGeometry = new BoxGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)boxGeometry;
	std::cerr << "finished initializing boxgeometry\n";

	// set chuck object pointer
	boxGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(boxGeometry));
}

CK_DLL_MFUN(cgl_geo_box_set)
{
	BoxGeometry *geo = (BoxGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT depth = GET_NEXT_FLOAT(ARGS);
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
	SphereGeometry *sphereGeometry = new SphereGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)sphereGeometry;
	std::cerr << "finished initializing spheregeometry\n";

	sphereGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(sphereGeometry));
}

CK_DLL_MFUN(cgl_geo_sphere_set)
{
	SphereGeometry *geo = (SphereGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSeg = GET_NEXT_INT(ARGS);
	t_CKINT heightSeg = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, widthSeg, heightSeg, phiStart, phiLength, thetaStart, thetaLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Circle geo ---------
CK_DLL_CTOR(cgl_geo_circle_ctor)
{
	CircleGeometry *circleGeometry = new CircleGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)circleGeometry;
	std::cerr << "finished initializing circlegeometry\n";

	circleGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(circleGeometry));
}

CK_DLL_MFUN(cgl_geo_circle_set)
{
	CircleGeometry *geo = (CircleGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT thetaStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT thetaLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, segments, thetaStart, thetaLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// plane geo ----------
CK_DLL_CTOR(cgl_geo_plane_ctor)
{
	PlaneGeometry *planeGeometry = new PlaneGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)planeGeometry;
	std::cerr << "finished initializing planegeometry\n";

	planeGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(planeGeometry));
}

CK_DLL_MFUN(cgl_geo_plane_set)
{
	PlaneGeometry *geo = (PlaneGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT height = GET_NEXT_FLOAT(ARGS);
	t_CKINT widthSegments = GET_NEXT_INT(ARGS);
	t_CKINT heightSegments = GET_NEXT_INT(ARGS);
	geo->UpdateParams(width, height, widthSegments, heightSegments);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// torus geo  ----------
CK_DLL_CTOR(cgl_geo_torus_ctor)
{
	TorusGeometry *torusGeometry = new TorusGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)torusGeometry;
	std::cerr << "finished initializing torusgeometry\n";

	torusGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(torusGeometry));
}

CK_DLL_MFUN(cgl_geo_torus_set)
{
	TorusGeometry *geo = (TorusGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKFLOAT radius = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT tubeRadius = GET_NEXT_FLOAT(ARGS);
	t_CKINT radialSegments = GET_NEXT_INT(ARGS);
	t_CKINT tubularSegments = GET_NEXT_INT(ARGS);
	t_CKFLOAT arcLength = GET_NEXT_FLOAT(ARGS);
	geo->UpdateParams(radius, tubeRadius, radialSegments, tubularSegments, arcLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Lathe geo ----------
CK_DLL_CTOR(cgl_geo_lathe_ctor)
{
	LatheGeometry *lathe = new LatheGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)lathe;
	std::cerr << "finished initializing lathegeometry\n";

	lathe->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(lathe));
}

CK_DLL_MFUN(cgl_geo_lathe_set)
{
	LatheGeometry *geo = (LatheGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *points = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);

	geo->UpdateParams(points->m_vector, segments, phiStart, phiLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}
CK_DLL_MFUN(cgl_geo_lathe_set_no_points)
{
	LatheGeometry *geo = (LatheGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	t_CKINT segments = GET_NEXT_INT(ARGS);
	t_CKFLOAT phiStart = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT phiLength = GET_NEXT_FLOAT(ARGS);

	geo->UpdateParams(segments, phiStart, phiLength);

	CGL::PushCommand(new UpdateGeometryCommand(geo));
}

// Custom geo ---------
CK_DLL_CTOR(cgl_geo_custom_ctor)
{
	std::cerr << "cgl_custom_ctor\n";
	CustomGeometry *customGeometry = new CustomGeometry;
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = (t_CKINT)customGeometry;
	std::cerr << "finished initializing customgeometry\n";

	customGeometry->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateGeometryCommand(customGeometry));
}

CK_DLL_MFUN(cgl_geo_set_attribute)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	t_CKINT location = GET_NEXT_INT(ARGS);
	t_CKINT numComponents = GET_NEXT_INT(ARGS);
	bool normalize = GET_NEXT_INT(ARGS);
	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	// not stored in chuck-side copy to save time
	// geo->SetAttribute(name, location, numComponents, normalize, data);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, name->str(), location, numComponents, data->m_vector, normalize));
}

CK_DLL_MFUN(cgl_geo_set_positions)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, data->m_vector, false));
}

CK_DLL_MFUN(cgl_geo_set_positions_vec3)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	auto* data = (Chuck_Array24*)GET_NEXT_OBJECT(ARGS);

	// TODO extra round of copying here, can avoid if it matters
	std::vector<t_CKFLOAT> vec3s;
	vec3s.reserve(3 * data->m_vector.size());
	for (auto& val : data->m_vector) {
		vec3s.emplace_back(val.x);
		vec3s.emplace_back(val.y);
		vec3s.emplace_back(val.z);
	}

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, vec3s, false
		)
	);
}

// set colors
CK_DLL_MFUN(cgl_geo_set_colors)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, data->m_vector, false));
}

// set normals
CK_DLL_MFUN(cgl_geo_set_normals)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "normal", Geometry::NORMAL_ATTRIB_IDX, 3, data->m_vector, false));
}

// set uvs
CK_DLL_MFUN(cgl_geo_set_uvs)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "uv", Geometry::UV0_ATTRIB_IDX, 2, data->m_vector, false));
}

// set indices
CK_DLL_MFUN(cgl_geo_set_indices)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayInt *data = (Chuck_ArrayInt *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryIndicesCommand(geo, data->m_vector));
}

//-----------------------------------------------------------------------------
// init_chugl_texture()
//-----------------------------------------------------------------------------

t_CKBOOL init_chugl_texture(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, "Texture", "Object");
	QUERY->add_ctor(QUERY, cgl_texture_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);
	texture_data_offset = QUERY->add_mvar(QUERY, "int", "@texture_dat", false);

	// texture options (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", TRUE, (void *)&CGL_Texture::Repeat);
	QUERY->add_svar(QUERY, "int", "WRAP_MIRRORED", TRUE, (void *)&CGL_Texture::MirroredRepeat);
	QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", TRUE, (void *)&CGL_Texture::ClampToEdge);

	// not exposing mipmap filter options for simplicity
	QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", TRUE, (void *)&CGL_Texture::Nearest);
	QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", TRUE, (void *)&CGL_Texture::Linear);

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
	QUERY->begin_class(QUERY, "FileTexture", "Texture");
	QUERY->add_ctor(QUERY, cgl_texture_file_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);

	QUERY->add_mfun(QUERY, cgl_texture_file_set_filepath, "string", "path");
	QUERY->add_arg(QUERY, "string", "path");
	QUERY->add_mfun(QUERY, cgl_texture_file_get_filepath, "string", "path");
	QUERY->end_class(QUERY);

	// DataTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, "DataTexture", "Texture");
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
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	CK_SAFE_DELETE(texture);
	OBJ_MEMBER_INT(SELF, texture_data_offset) = 0;

	// TODO: send destroy command to CGL command queue
	//       - remove texture from scenegraph
	// 	     - callback hook for renderer to remove TextureMat from cache
	// idea: .dispose() of THREEjs. need to deliberately invoke freeing GPU resources from chuck side,
	// chuck destructor does NOT implicitly free GPU resources (probably safer this way)
}

CK_DLL_MFUN(cgl_texture_set_wrap)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	auto s = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS));
	auto t = static_cast<CGL_TextureWrapMode>(GET_NEXT_INT(ARGS));
	texture->SetWrapMode(s, t);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_wrap_s)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.wrapS);
}

CK_DLL_MFUN(cgl_texture_get_wrap_t)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.wrapS);
}

CK_DLL_MFUN(cgl_texture_set_filter)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	auto min = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS));
	auto mag = static_cast<CGL_TextureFilterMode>(GET_NEXT_INT(ARGS));
	texture->SetFilterMode(min, mag);

	CGL::PushCommand(new UpdateTextureSamplerCommand(texture));
}

CK_DLL_MFUN(cgl_texture_get_filter_min)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.filterMin);
}

CK_DLL_MFUN(cgl_texture_get_filter_mag)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	RETURN->v_int = static_cast<t_CKINT>(texture->m_SamplerParams.filterMag);
}

// FileTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_file_ctor)
{
	std::cerr << "cgl_texture_file_ctor" << std::endl;

	CGL_Texture *texture = new CGL_Texture(CGL_TextureType::File2D);
	OBJ_MEMBER_INT(SELF, texture_data_offset) = (t_CKINT)texture;

	texture->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateTextureCommand(texture));
}

CK_DLL_MFUN(cgl_texture_file_set_filepath)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	Chuck_String *path = GET_NEXT_STRING(ARGS);
	texture->m_FilePath = path->str(); // note: doesn't make sense to update flags on chuck-side copy because renderer doesn't have access

	CGL::PushCommand(new UpdateTexturePathCommand(texture));

	RETURN->v_string = path;
}

CK_DLL_MFUN(cgl_texture_file_get_filepath)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	RETURN->v_string = (Chuck_String *)API->object->create_string(VM, texture->m_FilePath.c_str(), false);
}

// DataTexture API impl =====================================================
CK_DLL_CTOR(cgl_texture_rawdata_ctor)
{
	std::cerr << "cgl_texture_rawdata_ctor" << std::endl;

	CGL_Texture *texture = new CGL_Texture(CGL_TextureType::RawData);
	OBJ_MEMBER_INT(SELF, texture_data_offset) = (t_CKINT)texture;

	texture->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateTextureCommand(texture));
}

CK_DLL_MFUN(cgl_texture_rawdata_set_data)
{
	CGL_Texture *texture = (CGL_Texture *)OBJ_MEMBER_INT(SELF, texture_data_offset);
	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);

	// update chuck-side texture ( no copy to avoid blocking audio thread )
	texture->SetRawData(data->m_vector, width, height, false);

	CGL::PushCommand(new UpdateTextureDataCommand(texture->GetID(), data->m_vector, width, height));
}

//-----------------------------------------------------------------------------
// init_chugl_mat()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mat(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL materials");

	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Base), "Object");
	QUERY->add_ctor(QUERY, cgl_mat_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);
	cglmat_data_offset = QUERY->add_mvar(QUERY, "int", "@cglmat_data", false);

	// clone
	QUERY->add_mfun(QUERY, cgl_mat_clone, Material::CKName(MaterialType::Base), "clone");

	// Material params (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "POLYGON_FILL", TRUE, (void *)&Material::POLYGON_FILL);
	QUERY->add_svar(QUERY, "int", "POLYGON_LINE", TRUE, (void *)&Material::POLYGON_LINE);
	QUERY->add_svar(QUERY, "int", "POLYGON_POINT", TRUE, (void *)&Material::POLYGON_POINT);

	QUERY->add_mfun(QUERY, cgl_mat_set_polygon_mode, "int", "polygonMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->add_mfun(QUERY, cgl_mat_get_polygon_mode, "int", "polygonMode");

	QUERY->add_mfun(QUERY, cgl_mat_set_point_size, "void", "pointSize");
	QUERY->add_arg(QUERY, "float", "size");

	QUERY->add_mfun(QUERY, cgl_mat_set_color, "vec3", "color");
	QUERY->add_arg(QUERY, "vec3", "col");

	QUERY->add_mfun(QUERY, cgl_mat_set_line_width, "void", "lineWidth");
	QUERY->add_arg(QUERY, "float", "width");

	// phong mat
	QUERY->add_mfun(QUERY, cgl_mat_phong_set_log_shininess, "float", "shine");
	QUERY->add_arg(QUERY, "float", "shininess");

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
	QUERY->add_arg(QUERY, "Texture", "texture");

	QUERY->end_class(QUERY);

	// normal material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Normal), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_norm_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_set_use_local_normals, "void", "useLocal");
	QUERY->add_arg(QUERY, "int", "useLocal");

	QUERY->end_class(QUERY);

	// flat material
	// QUERY->begin_class(QUERY, Material::CKName(MaterialType::Normal), Material::CKName(MaterialType::Base));
	// QUERY->add_ctor(QUERY, cgl_mat_flat_ctor);
	// QUERY->add_dtor(QUERY, cgl_mat_dtor);
	// QUERY->end_class(QUERY);



	// phong specular material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Phong), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_phong_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_diffuse_map, "void", "diffuseMap");
	QUERY->add_arg(QUERY, "Texture", "tex");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_map, "void", "specularMap");
	QUERY->add_arg(QUERY, "Texture", "tex");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_color, "vec3", "specularColor");
	QUERY->add_arg(QUERY, "vec3", "color");

	// TODO: add getters

	QUERY->end_class(QUERY);

	// custom shader material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::CustomShader), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_custom_shader_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_shaders, "void", "shaderPaths");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->add_arg(QUERY, "string", "frag");

	QUERY->end_class(QUERY);

	// points material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Points), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_points_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	QUERY->add_mfun(QUERY, cgl_mat_points_set_size_attenuation, "int", "attenuate");
	QUERY->add_arg(QUERY, "int", "attenuation");
	QUERY->add_mfun(QUERY, cgl_mat_points_get_size_attenuation, "int", "attenuate");

	QUERY->add_mfun(QUERY, cgl_mat_points_set_sprite, "Texture", "sprite");
	QUERY->add_arg(QUERY, "Texture", "sprite");

	QUERY->end_class(QUERY);

	// mango material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Mango), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_mango_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);
	QUERY->end_class(QUERY);

	// line material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Line), Material::CKName(MaterialType::Base));
	QUERY->add_ctor(QUERY, cgl_mat_line_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);

	// static vars
	QUERY->add_svar(QUERY, "int", "LINE_SEGMENTS", TRUE, (void *)&LineMaterial::LINE_SEGMENTS_MODE);
	QUERY->add_svar(QUERY, "int", "LINE_STRIP", TRUE, (void *)&LineMaterial::LINE_STRIP_MODE);
	QUERY->add_svar(QUERY, "int", "LINE_LOOP", TRUE, (void *)&LineMaterial::LINE_LOOP_MODE);

	QUERY->add_mfun(QUERY, cgl_mat_line_set_mode, "int", "mode");
	QUERY->add_arg(QUERY, "int", "mode");

	QUERY->end_class(QUERY);

	return true;
}
// CGL Materials ===================================================
CK_DLL_CTOR(cgl_mat_ctor)
{
	std::cerr << "cgl_mat_ctor\n";
	// dud, do nothing for now
}

CK_DLL_DTOR(cgl_mat_dtor) // all geos can share this base destructor
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CK_SAFE_DELETE(mat);
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = 0; // zero out the memory

	// TODO: send destroy command to CGL command queue
	//       - remove material from scenegraph
	// 	     - callback hook for renderer to remove RenderMat from cache
}


CK_DLL_MFUN(cgl_mat_clone)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	RETURN->v_object = CGL::CreateChuckObjFromMat(API, VM, mat->Clone(), SHRED, false)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mat_set_polygon_mode)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	auto mode = GET_NEXT_INT(ARGS);

	mat->SetPolygonMode((MaterialPolygonMode)mode);

	RETURN->v_int = mode;

	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::PolygonMode)));
}

CK_DLL_MFUN(cgl_mat_get_polygon_mode)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	RETURN->v_int = mat->GetPolygonMode();
}

// point size setter
CK_DLL_MFUN(cgl_mat_set_point_size)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	auto size = GET_NEXT_FLOAT(ARGS);
	mat->SetPointSize(size);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::POINT_SIZE_UNAME)));
}

CK_DLL_MFUN(cgl_mat_set_line_width)
{
	Material* mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKFLOAT width = GET_NEXT_FLOAT(ARGS);
	mat->SetLineWidth(width);

	RETURN->v_float = width;
	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::LINE_WIDTH_UNAME)));
}

CK_DLL_MFUN(cgl_mat_set_color)
{
	Material *mat = (Material*)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetColor(color.x, color.y, color.z, 1.0f);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::COLOR_UNAME)));
}


CK_DLL_MFUN(cgl_mat_set_uniform_float)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float2)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float3)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_float4)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	float value0 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value1 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value2 = static_cast<float>(GET_NEXT_FLOAT(ARGS));
	float value3 = static_cast<float>(GET_NEXT_FLOAT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int2)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int3)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_int4)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	int value0 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value1 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value2 = static_cast<int>(GET_NEXT_INT(ARGS));
	int value3 = static_cast<int>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value0, value1, value2, value3);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_bool)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	bool value = static_cast<bool>(GET_NEXT_INT(ARGS));

	MaterialUniform uniform = MaterialUniform::Create(name->str(), value);

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_MFUN(cgl_mat_set_uniform_texID)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *name = GET_NEXT_STRING(ARGS);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);

	MaterialUniform uniform = MaterialUniform::Create(name->str(), tex->GetID());

	mat->SetUniform(uniform);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, uniform));
}

CK_DLL_CTOR(cgl_mat_norm_ctor)
{
	std::cerr << "cgl_mat_norm_ctor";
	NormalMaterial *normMat = new NormalMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)normMat;
	std::cerr << "finished initializing norm material\n";

	normMat->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(normMat));
}

CK_DLL_MFUN(cgl_set_use_local_normals)
{
	NormalMaterial *mat = (NormalMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKINT use_local = GET_NEXT_INT(ARGS);
	if (use_local)
		mat->UseLocalNormals();
	else
		mat->UseWorldNormals();

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(NormalMaterial::USE_LOCAL_NORMALS_UNAME))));
}

// phong mat fns
CK_DLL_CTOR(cgl_mat_phong_ctor)
{
	PhongMaterial *phongMat = new PhongMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)phongMat;

	phongMat->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(phongMat));
}

CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map)
{
	PhongMaterial *mat = (PhongMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetDiffuseMap(tex);

	// TODO: how do I return the chuck texture object?
	// RETURN->v_object = SELF;

	// a lot of redundant work (entire uniform vector is copied). can optimize later
	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::DIFFUSE_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_map)
{
	PhongMaterial *mat = (PhongMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetSpecularMap(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_color)
{
	PhongMaterial *mat = (PhongMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetSpecularColor(color.x, color.y, color.z);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_log_shininess)
{
	PhongMaterial *mat = (PhongMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKFLOAT shininess = GET_NEXT_FLOAT(ARGS);
	mat->SetLogShininess(shininess);

	RETURN->v_float = shininess;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SHININESS_UNAME)));
}

// custom shader mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_custom_shader_ctor)
{
	ShaderMaterial *shaderMat = new ShaderMaterial("", "");
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)shaderMat;

	shaderMat->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(shaderMat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders)
{
	ShaderMaterial *mat = (ShaderMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	Chuck_String *vertPath = GET_NEXT_STRING(ARGS);
	Chuck_String *fragPath = GET_NEXT_STRING(ARGS);

	mat->SetShaderPaths(vertPath->str(), fragPath->str());

	CGL::PushCommand(new UpdateMaterialShadersCommand(mat));
}

// points mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_points_ctor)
{
	PointsMaterial *pointsMat = new PointsMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)pointsMat;

	pointsMat->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(pointsMat));
}

CK_DLL_MFUN(cgl_mat_points_set_size_attenuation)
{
	PointsMaterial *mat = (PointsMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKINT attenuation = GET_NEXT_INT(ARGS);
	mat->SetSizeAttenuation(attenuation);

	RETURN->v_int = attenuation;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SIZE_ATTENUATION_UNAME)));
}

CK_DLL_MFUN(cgl_mat_points_get_size_attenuation)
{
	PointsMaterial *mat = (PointsMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);

	RETURN->v_int = mat->GetSizeAttenuation() ? 1 : 0;
}

CK_DLL_MFUN(cgl_mat_points_set_sprite)
{
	PointsMaterial *mat = (PointsMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetSprite(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SPRITE_TEXTURE_UNAME)));
}

// set point color

// mango mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_mango_ctor)
{
	MangoMaterial *mangoMat = new MangoMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)mangoMat;

	mangoMat->m_ChuckObject = SELF;

	// Creation command
	CGL::PushCommand(new CreateMaterialCommand(mangoMat));
}

// line mat fns ---------------------------------

CK_DLL_CTOR(cgl_mat_line_ctor)
{
	LineMaterial *lineMat = new LineMaterial;
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = (t_CKINT)lineMat;
	
	lineMat->m_ChuckObject = SELF;

	CGL::PushCommand(new CreateMaterialCommand(lineMat));
}


CK_DLL_MFUN(cgl_mat_line_set_mode)
{
	LineMaterial *mat = (LineMaterial *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKINT mode = GET_NEXT_INT(ARGS);
	mat->SetLineMode((MaterialPrimitiveMode)mode);

	RETURN->v_int = mode;
	CGL::PushCommand(new UpdateMaterialOptionCommand(mat, *mat->GetOption(MaterialOptionParam::PrimitiveMode)));
}

//-----------------------------------------------------------------------------
// init_chugl_obj()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_obj(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL Object");

	// GGen =========================================
	QUERY->begin_class(QUERY, "GGen", "Object");
	QUERY->add_ctor(QUERY, cgl_obj_ctor);
	QUERY->add_dtor(QUERY, cgl_obj_dtor);
	ggen_data_offset = QUERY->add_mvar(QUERY, "int", "@ggen_data", false);

	QUERY->add_mfun(QUERY, cgl_obj_get_id, "int", "id");

	QUERY->add_mfun(QUERY, cgl_obj_update, "void", "update");
	QUERY->add_arg(QUERY, "float", "dt");

	// transform getters ===========
	// get obj direction vectors in world space
	QUERY->add_mfun(QUERY, cgl_obj_get_right, "vec3", "right");
	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "forward");
	QUERY->add_mfun(QUERY, cgl_obj_get_up, "vec3", "up");

	QUERY->add_mfun(QUERY, cgl_obj_get_pos, "vec3", "pos");
	QUERY->add_mfun(QUERY, cgl_obj_get_rot, "vec3", "rot");
	QUERY->add_mfun(QUERY, cgl_obj_get_scale, "vec3", "sca");

	QUERY->add_mfun(QUERY, cgl_obj_get_world_pos, "vec3", "worldPos");

	// transform setters ===========
	QUERY->add_mfun(QUERY, cgl_obj_translate_by, "GGen", "translate");
	QUERY->add_arg(QUERY, "vec3", "trans_vec");

	QUERY->add_mfun(QUERY, cgl_obj_rot_on_local_axis, "GGen", "rotateOnLocalAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_on_world_axis, "GGen", "rotateOnWorldAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_x, "GGen", "rotX");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_y, "GGen", "rotY");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_rot_z, "GGen", "rotZ");
	QUERY->add_arg(QUERY, "float", "deg");

	QUERY->add_mfun(QUERY, cgl_obj_pos_x, "GGen", "posX");
	QUERY->add_arg(QUERY, "float", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_pos_y, "GGen", "posY");
	QUERY->add_arg(QUERY, "float", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_pos_z, "GGen", "posZ");
	QUERY->add_arg(QUERY, "float", "pos");

	// TODO: add scale setters in each dimension

	// TODO: add option to pass UP vector to lookat
	QUERY->add_mfun(QUERY, cgl_obj_lookat_vec3, "GGen", "lookAt");
	QUERY->add_arg(QUERY, "vec3", "pos");

	QUERY->add_mfun(QUERY, cgl_obj_lookat_float, "GGen", "lookAt");
	QUERY->add_arg(QUERY, "float", "x");
	QUERY->add_arg(QUERY, "float", "y");
	QUERY->add_arg(QUERY, "float", "z");

	QUERY->add_mfun(QUERY, cgl_obj_set_pos, "GGen", "position");
	QUERY->add_arg(QUERY, "vec3", "pos_vec");

	QUERY->add_mfun(QUERY, cgl_obj_set_rot, "GGen", "rotation"); // sets from eulers
	QUERY->add_arg(QUERY, "vec3", "eulers");

	QUERY->add_mfun(QUERY, cgl_obj_set_scale, "GGen", "scale");
	QUERY->add_arg(QUERY, "vec3", "scale");

	// scenegraph relationship methods ===========
	// overload GGen --> GGen
	QUERY->add_op_overload_binary(QUERY, ggen_op_gruck, "GGen", "-->",
								  "GGen", "lhs", "GGen", "rhs");

	QUERY->add_op_overload_binary(QUERY, ggen_op_ungruck, "GGen", "--<",
								  "GGen", "lhs", "GGen", "rhs");

	QUERY->end_class(QUERY);

	return true;
}

// CGLObject DLL ==============================================
CK_DLL_CTOR(cgl_obj_ctor)
{
	// std::cerr << "cgl_obj_ctor" << std::endl;
	Chuck_DL_Api::Type type = API->type->lookup(VM, "GGen");
	auto thisType = API->object->get_type(SELF);
	if (
		API->type->is_equal(type, thisType)  // this type is a GGen
		||
		thisType->originHint == te_originUserDefined  // this type is defined .ck file
		|| 
		thisType->originHint == te_originImport       // .ck file included in search path
	)
	{
		SceneGraphObject *cglObj = new SceneGraphObject;
		OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)cglObj;

		cglObj->m_ChuckObject = SELF;

		CGL::PushCommand(new CreateSceneGraphObjectCommand(cglObj));
	}
}

CK_DLL_DTOR(cgl_obj_dtor)
{
	std::cerr << "cgl_obj_dtor" << std::endl;
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// CK_SAFE_DELETE(cglObj);  // TODO: the bandit ship of ref count memory managemnet
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;
}

CK_DLL_MFUN(cgl_obj_get_id)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_int = cglObj->GetID();
}

CK_DLL_MFUN(cgl_obj_update) {}

CK_DLL_MFUN(cgl_obj_get_right)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &right = cglObj->GetRight();
	RETURN->v_vec3 = {right.x, right.y, right.z};
}
CK_DLL_MFUN(cgl_obj_get_forward)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetForward();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_get_up)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetUp();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_translate_by)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 trans = GET_NEXT_VEC3(ARGS);
	cglObj->Translate(glm::vec3(trans.x, trans.y, trans.z));

	// add to command queue
	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_scale_by)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->Scale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_on_local_axis)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnLocalAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_on_world_axis)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnWorldAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new TransformCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateX(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateY(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateZ(deg);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.x = posX;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posY = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.y = posY;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_pos_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.z = posZ;
	cglObj->SetPosition(pos);
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_lookat_vec3)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->LookAt(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_lookat_float)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT x = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT y = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT z = GET_NEXT_FLOAT(ARGS);
	cglObj->LookAt(glm::vec3(x, y, z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_pos)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_rot)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetRotation(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_scale)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new TransformCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_get_world_pos)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetWorldPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_get_rot)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = glm::degrees(glm::eulerAngles(cglObj->GetRotation()));
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_get_scale)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetScale();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_GFUN(ggen_op_gruck)
{
	// get the arguments
	Chuck_Object *lhs = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *rhs = GET_NEXT_OBJECT(ARGS);

	// get internal representation
	SceneGraphObject *LHS = (SceneGraphObject *)OBJ_MEMBER_INT(lhs, ggen_data_offset);
	SceneGraphObject *RHS = (SceneGraphObject *)OBJ_MEMBER_INT(rhs, ggen_data_offset);

	// command
	CGL::PushCommand(new RelationshipCommand(RHS, LHS, RelationshipCommand::Relation::AddChild));

	// return RHS
	RETURN->v_object = rhs;
}

CK_DLL_GFUN(ggen_op_ungruck)
{
	// get the arguments
	Chuck_Object *lhs = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *rhs = GET_NEXT_OBJECT(ARGS);
	// get internal representation
	SceneGraphObject *LHS = (SceneGraphObject *)OBJ_MEMBER_INT(lhs, ggen_data_offset);
	SceneGraphObject *RHS = (SceneGraphObject *)OBJ_MEMBER_INT(rhs, ggen_data_offset);

	// command
	CGL::PushCommand(new RelationshipCommand(RHS, LHS, RelationshipCommand::Relation::RemoveChild));

	// return RHS
	RETURN->v_object = rhs;
}

//-----------------------------------------------------------------------------
// init_chugl_scene()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_scene(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");
	// CGL scene
	QUERY->begin_class(QUERY, "GScene", "GGen");
	QUERY->add_ctor(QUERY, cgl_scene_ctor);
	QUERY->add_dtor(QUERY, cgl_scene_dtor);

	// static constants
	// TODO: add linear fog? but doesn't even look as good
	QUERY->add_svar(QUERY, "int", "FOG_EXP", true, (void *)&Scene::FOG_EXP);
	QUERY->add_svar(QUERY, "int", "FOG_EXP2", true, (void *)&Scene::FOG_EXP2);

	// background color
	QUERY->add_mfun(QUERY, cgl_scene_set_background_color, "vec3", "backgroundColor");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->add_mfun(QUERY, cgl_scene_get_background_color, "vec3", "backgroundColor");

	// light
	QUERY->add_mfun(QUERY, cgl_scene_get_default_light, Light::CKName(LightType::Base), "light");
	QUERY->add_mfun(QUERY, cgl_scene_get_num_lights, "int", "numLights");


	// fog member vars
	QUERY->add_mfun(QUERY, cgl_scene_set_fog_color, "vec3", "fogColor");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->add_mfun(QUERY, cgl_scene_get_fog_color, "vec3", "fogColor");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_density, "float", "fogDensity");
	QUERY->add_arg(QUERY, "float", "density");
	QUERY->add_mfun(QUERY, cgl_scene_get_fog_density, "float", "fogDensity");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_type, "int", "fogType");
	QUERY->add_arg(QUERY, "int", "type");
	QUERY->add_mfun(QUERY, cgl_scene_get_fog_type, "int", "fogType");

	QUERY->add_mfun(QUERY, cgl_scene_set_fog_enabled, "void", "enableFog");
	QUERY->add_mfun(QUERY, cgl_scene_set_fog_disabled, "void", "disableFog");

	QUERY->end_class(QUERY);

	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_scene_ctor)
{
	// temporary fix until a default pre-constructor is provided for chugins
	//
	Scene *scene = (Scene *)OBJ_MEMBER_INT(CGL::GetMainScene(SHRED, API, VM), ggen_data_offset);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)scene;
	// OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)&CGL::mainScene;
}
CK_DLL_DTOR(cgl_scene_dtor)
{
	// Scene* mainScene = (Scene*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// don't call delete! because this is a static var

	// TODO is this necessary?
	// OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;  // zero out the memory
}

CK_DLL_MFUN(cgl_scene_set_background_color)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	scene->SetBackgroundColor(color.x, color.y, color.z);
	RETURN->v_vec3 = color;
	CGL::PushCommand(new UpdateSceneBackgroundColorCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_background_color)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &color = scene->GetBackgroundColor();
	RETURN->v_vec3 = {color.x, color.y, color.z};
}

CK_DLL_MFUN(cgl_scene_get_default_light)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_object = scene->m_Lights.size() > 0 ? scene->m_Lights[0]->m_ChuckObject : nullptr;
}

CK_DLL_MFUN(cgl_scene_get_num_lights)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_int = scene->m_Lights.size();
}	

CK_DLL_MFUN(cgl_scene_set_fog_color)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	scene->SetFogColor(color.x, color.y, color.z);
	RETURN->v_vec3 = color;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_color)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &color = scene->GetFogColor();
	RETURN->v_vec3 = {color.x, color.y, color.z};
}

CK_DLL_MFUN(cgl_scene_set_fog_density)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT density = GET_NEXT_FLOAT(ARGS);
	scene->SetFogDensity(density);
	RETURN->v_float = density;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_density)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = scene->GetFogDensity();
}

CK_DLL_MFUN(cgl_scene_set_fog_type)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKINT type = GET_NEXT_INT(ARGS);
	scene->SetFogType((FogType)type);
	RETURN->v_int = type;
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_get_fog_type)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_int = scene->GetFogType();
}

CK_DLL_MFUN(cgl_scene_set_fog_enabled)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	scene->SetFogEnabled(true);
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

CK_DLL_MFUN(cgl_scene_set_fog_disabled)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	scene->SetFogEnabled(false);
	CGL::PushCommand(new UpdateSceneFogCommand(scene));
}

//-----------------------------------------------------------------------------
// init_chugl_cam()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_cam(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL Camera");
	// CGL camera
	QUERY->begin_class(QUERY, "GCamera", "GGen");
	QUERY->add_ctor(QUERY, cgl_cam_ctor);
	QUERY->add_dtor(QUERY, cgl_cam_dtor);

	// static vars
	// perspective mode
	QUERY->add_svar(QUERY, "int", "MODE_PERSP", true, (void *)&Camera::MODE_PERSPECTIVE);
	QUERY->add_svar(QUERY, "int", "MODE_ORTHO", true, (void *)&Camera::MODE_ORTHO);

	QUERY->add_mfun(QUERY, cgl_cam_set_mode_persp, "void", "perspective");
	QUERY->add_mfun(QUERY, cgl_cam_set_mode_ortho, "void", "orthographic");
	QUERY->add_mfun(QUERY, cgl_cam_get_mode, "int", "mode");

	// clipping planes
	QUERY->add_mfun(QUERY, cgl_cam_set_clip, "void", "clip");
	QUERY->add_arg(QUERY, "float", "near");
	QUERY->add_arg(QUERY, "float", "far");
	QUERY->add_mfun(QUERY, cgl_cam_get_clip_near, "float", "clipNear");
	QUERY->add_mfun(QUERY, cgl_cam_get_clip_far, "float", "clipFar");

	// fov (in degrees)
	QUERY->add_mfun(QUERY, cgl_cam_set_pers_fov, "float", "fov");
	QUERY->add_arg(QUERY, "float", "f");
	QUERY->add_mfun(QUERY, cgl_cam_get_pers_fov, "float", "fov");

	// ortho view size
	QUERY->add_mfun(QUERY, cgl_cam_set_ortho_size, "float", "viewSize");
	QUERY->add_arg(QUERY, "float", "s");
	QUERY->add_mfun(QUERY, cgl_cam_get_ortho_size, "float", "viewSize");

	QUERY->end_class(QUERY);

	return true;
}

// CGL Camera =======================
CK_DLL_CTOR(cgl_cam_ctor)
{
	// for now just return the main camera
	// very important: have to access main scene through CGL::GetMainScene() because
	// that's where the default camera construction happens
	Scene *scene = (Scene *)OBJ_MEMBER_INT(CGL::GetMainScene(SHRED, API, VM), ggen_data_offset);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)(scene->GetMainCamera());
}

CK_DLL_DTOR(cgl_cam_dtor)
{
	// TODO: no destructors for static vars (we don't want one camera handle falling out of scope to delete the only camera)

	// Camera* mainCam = (Camera*)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// don't call delete! because this is a static var
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0; // zero out the memory
}

CK_DLL_MFUN(cgl_cam_set_mode_persp)
{
	Scene *scene = (Scene *)OBJ_MEMBER_INT(CGL::GetMainScene(SHRED, API, VM), ggen_data_offset);
	// Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	Camera * cam = scene->GetMainCamera();
	cam->SetPerspective();

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_set_mode_ortho)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	cam->SetOrtho();

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_mode)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_int = cam->GetMode();
}

CK_DLL_MFUN(cgl_cam_set_clip)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT n = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT f = GET_NEXT_FLOAT(ARGS);
	cam->SetClipPlanes(n, f);

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_clip_near)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cam->GetClipNear();
}

CK_DLL_MFUN(cgl_cam_get_clip_far)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cam->GetClipFar();
}

CK_DLL_MFUN(cgl_cam_set_pers_fov)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT f = GET_NEXT_FLOAT(ARGS);
	cam->SetFOV(f);

	RETURN->v_float = f;

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_pers_fov)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cam->GetFOV();
}

CK_DLL_MFUN(cgl_cam_set_ortho_size)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT s = GET_NEXT_FLOAT(ARGS);
	cam->SetSize(s);

	RETURN->v_float = s;

	CGL::PushCommand(new UpdateCameraCommand(cam));
}

CK_DLL_MFUN(cgl_cam_get_ortho_size)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cam->GetSize();
}

//-----------------------------------------------------------------------------
// init_chugl_mesh()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mesh(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");

	QUERY->begin_class(QUERY, "GMesh", "GGen");
	QUERY->add_ctor(QUERY, cgl_mesh_ctor);
	QUERY->add_dtor(QUERY, cgl_mesh_dtor);

	QUERY->add_mfun(QUERY, cgl_mesh_set, "void", "set");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");

	QUERY->add_mfun(QUERY, cgl_mesh_set_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");

	QUERY->add_mfun(QUERY, cgl_mesh_set_mat, Material::CKName(MaterialType::Base), "mat");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");

	QUERY->add_mfun(QUERY, cgl_mesh_get_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_mfun(QUERY, cgl_mesh_get_mat, Material::CKName(MaterialType::Base), "mat");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_mat, Material::CKName(MaterialType::Base), "dupMat");
	QUERY->add_mfun(QUERY, cgl_mesh_dup_geo, Geometry::CKName(GeometryType::Base) , "dupGeo");
	QUERY->add_mfun(QUERY, cgl_mesh_dup_all, "GMesh", "dup");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCube", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gcube_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GSphere", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gsphere_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCircle", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gcircle_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPlane", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gplane_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GTorus", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gtorus_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GLines", "GMesh");
	QUERY->add_ctor(QUERY, cgl_glines_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPoints", "GMesh");
	QUERY->add_ctor(QUERY, cgl_gpoints_ctor);
	// QUERY->add_dtor(QUERY, cgl_gcube_dtor);
	QUERY->end_class(QUERY);



	return true;
}
// CGL Scene ==============================================
CK_DLL_CTOR(cgl_mesh_ctor)
{
	// Chuck_DL_Api::Type type = API->type->lookup(VM, "GMesh");
	// if (API->type->is_equal(type, API->object->get_type(SELF))) {
	// 	std::cerr << "GMesh instantiated" << std::endl;
	// }

	Mesh *mesh = new Mesh();
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)mesh;

	mesh->m_ChuckObject = SELF;
	CGL::PushCommand(new CreateMeshCommand(mesh));
}

CK_DLL_DTOR(cgl_mesh_dtor)
{
	std::cerr << "cgl_mesh_dtor" << std::endl;
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(mesh);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0; // zero out the memory

	// TODO: need to remove from scenegraph
}

static void cglMeshSet(Mesh *mesh, Geometry *geo, Material *mat)
{
	// set on CGL side
	mesh->SetGeometry(geo);
	mesh->SetMaterial(mat);
	// command queue to update renderer side
	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_set)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	// Geometry * geo = (Geometry *)GET_NEXT_OBJECT(ARGS);
	// Material * mat = (Material *)GET_NEXT_OBJECT(ARGS);
	Chuck_Object *geo_obj = GET_NEXT_OBJECT(ARGS);
	Chuck_Object *mat_obj = GET_NEXT_OBJECT(ARGS);

	Geometry *geo = geo_obj == nullptr ? nullptr : (Geometry *)OBJ_MEMBER_INT(geo_obj, geometry_data_offset);
	Material *mat = mat_obj == nullptr ? nullptr : (Material *)OBJ_MEMBER_INT(mat_obj, cglmat_data_offset);

	cglMeshSet(mesh, geo, mat);
}

CK_DLL_MFUN(cgl_mesh_set_geo)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	Chuck_Object *geo_obj = GET_NEXT_OBJECT(ARGS);
	Geometry *geo = geo_obj == nullptr ? nullptr : (Geometry *)OBJ_MEMBER_INT(geo_obj, geometry_data_offset);

	mesh->SetGeometry(geo);

	RETURN->v_object = geo_obj;

	std::cerr << "cgl_mesh_set_geo" << std::endl;

	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_set_mat)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	Chuck_Object *mat_obj = GET_NEXT_OBJECT(ARGS);
	Material *mat = mat_obj == nullptr ? nullptr : (Material *)OBJ_MEMBER_INT(mat_obj, cglmat_data_offset);

	mesh->SetMaterial(mat);

	RETURN->v_object = mat_obj;

	std::cerr << "cgl_mesh_set_mat" << std::endl;

	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_get_mat)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_object = mesh->GetMaterial()->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_get_geo)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_object = mesh->GetGeometry()->m_ChuckObject;
}



// duplicators
CK_DLL_MFUN(cgl_mesh_dup_mat)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_object = CGL::DupMeshMat(API, VM, mesh, SHRED)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_dup_geo)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_object = CGL::DupMeshGeo(API, VM, mesh, SHRED)->m_ChuckObject;
}

CK_DLL_MFUN(cgl_mesh_dup_all)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CGL::DupMeshGeo(API, VM, mesh, SHRED);
	CGL::DupMeshMat(API, VM, mesh, SHRED);
	RETURN->v_object = SELF;
}

CK_DLL_CTOR(cgl_gcube_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new BoxGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gsphere_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new SphereGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gcircle_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CircleGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gplane_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new PlaneGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gtorus_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PhongMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new TorusGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_glines_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new LineMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CustomGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

	std::vector<double> firstLine = {0, 0, 0, 1, 1, 1};

	// initialize with single line
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, firstLine, false
		)
	);

    cglMeshSet(mesh, geo, mat);
}

CK_DLL_CTOR(cgl_gpoints_ctor)
{
    Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);

	Material* mat = new PointsMaterial;
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, true);
	Geometry* geo = new CustomGeometry;
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, true);

	std::vector<double> firstPoint = {0, 0, 0};

	// initialize with single line
	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "position", Geometry::POSITION_ATTRIB_IDX, 3, firstPoint, false
		)
	);

    cglMeshSet(mesh, geo, mat);
}

//-----------------------------------------------------------------------------
// init_chugl_group()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_group(Chuck_DL_Query *QUERY)
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
	Group *group = new Group;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)group;
	group->m_ChuckObject = SELF;
	CGL::PushCommand(new CreateGroupCommand(group));
}

CK_DLL_DTOR(cgl_group_dtor)
{
	Group *group = (Group *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(group);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0; // zero out the memory

	// TODO: need to remove from scenegraph
}

//-----------------------------------------------------------------------------
// init_chugl_light()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_light(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, Light::CKName(LightType::Base), "GGen");
	QUERY->add_ctor(QUERY, cgl_light_ctor);
	QUERY->add_dtor(QUERY, cgl_light_dtor);

	QUERY->add_mfun(QUERY, cgl_light_set_intensity, "float", "intensity");  // 0 -- 1
	QUERY->add_arg(QUERY, "float", "i");
	QUERY->add_mfun(QUERY, cgl_light_set_ambient, "vec3", "ambient"); 
	QUERY->add_arg(QUERY, "vec3", "a");
	QUERY->add_mfun(QUERY, cgl_light_set_diffuse, "vec3", "diffuse");
	QUERY->add_arg(QUERY, "vec3", "d");
	QUERY->add_mfun(QUERY, cgl_light_set_specular, "vec3", "specular");
	QUERY->add_arg(QUERY, "vec3", "s");

	QUERY->add_mfun(QUERY, cgl_light_get_intensity, "float", "intensity");
	QUERY->add_mfun(QUERY, cgl_light_get_ambient, "vec3", "ambient");
	QUERY->add_mfun(QUERY, cgl_light_get_diffuse, "vec3", "diffuse");
	QUERY->add_mfun(QUERY, cgl_light_get_specular, "vec3", "specular");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Light::CKName(LightType::Point), Light::CKName(LightType::Base));
	QUERY->add_ctor(QUERY, cgl_point_light_ctor);
	QUERY->add_dtor(QUERY, cgl_light_dtor);

	QUERY->add_mfun(QUERY, cgl_points_light_set_falloff, "void", "falloff");
	QUERY->add_arg(QUERY, "float", "linear");
	QUERY->add_arg(QUERY, "float", "quadratic");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Light::CKName(LightType::Directional), Light::CKName(LightType::Base));
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
	Light *group = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	CK_SAFE_DELETE(group);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0; // zero out the memory

	// TODO: need to remove from scenegraph with a destroy command
}

CK_DLL_MFUN(cgl_light_set_intensity)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT i = GET_NEXT_FLOAT(ARGS);
	light->m_Params.intensity = i;
	RETURN->v_float = i;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_ambient)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	auto amb = GET_NEXT_VEC3(ARGS);
	light->m_Params.ambient = {amb.x, amb.y, amb.z};
	RETURN->v_vec3 = amb;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_diffuse)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	auto diff = GET_NEXT_VEC3(ARGS);
	light->m_Params.diffuse = {diff.x, diff.y, diff.z};
	RETURN->v_vec3 = diff;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_set_specular)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	auto spec = GET_NEXT_VEC3(ARGS);
	light->m_Params.specular = {spec.x, spec.y, spec.z};
	RETURN->v_vec3 = spec;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_MFUN(cgl_light_get_intensity)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = light->m_Params.intensity;
}

CK_DLL_MFUN(cgl_light_get_ambient)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_vec3 = {light->m_Params.ambient.x, light->m_Params.ambient.y, light->m_Params.ambient.z};
}

CK_DLL_MFUN(cgl_light_get_diffuse)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_vec3 = {light->m_Params.diffuse.x, light->m_Params.diffuse.y, light->m_Params.diffuse.z};
}

CK_DLL_MFUN(cgl_light_get_specular)
{
	Light *light = (Light *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_vec3 = {light->m_Params.specular.x, light->m_Params.specular.y, light->m_Params.specular.z};
}

CK_DLL_CTOR(cgl_point_light_ctor)
{
	PointLight *light = new PointLight;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)light;

	CGL::PushCommand(new CreateLightCommand(light, &CGL::mainScene, SELF));
}

CK_DLL_MFUN(cgl_points_light_set_falloff)
{
	PointLight *light = (PointLight *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT linear = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT quadratic = GET_NEXT_FLOAT(ARGS);
	light->m_Params.linear = linear;
	light->m_Params.quadratic = quadratic;

	CGL::PushCommand(new UpdateLightCommand(light));
}

CK_DLL_CTOR(cgl_dir_light_ctor)
{
	DirLight *light = new DirLight;
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = (t_CKINT)light;

	CGL::PushCommand(new CreateLightCommand(light, &CGL::mainScene, SELF));
}

//-----------------------------------------------------------------------------
// ChuGL Event impl (TODO refactor into separate file)
//-----------------------------------------------------------------------------
// CglEventstatic initialization (again, should be refactored to be accessible through chuck.h)
std::vector<CglEvent *> CglEvent::m_FrameEvents;
std::vector<CglEvent *> CglEvent::m_UpdateEvents;
std::vector<CglEvent *> CglEvent::m_WindowResizeEvents;
CBufferSimple *CglEvent::s_SharedEventQueue;

std::vector<CglEvent *> &CglEvent::GetEventQueue(CglEventType type)
{
	switch (type)
	{
	case CglEventType::CGL_UPDATE:
		return m_UpdateEvents;
	case CglEventType::CGL_FRAME:
		return m_FrameEvents;
	case CglEventType::CGL_WINDOW_RESIZE:
		return m_WindowResizeEvents;
	default:
		throw std::runtime_error("invalid CGL event type");
	}
}

/*
void CglEvent::wait(Chuck_VM_Shred* shred)
{
	m_Event->wait(shred, m_VM);
}
*/

CglEvent::CglEvent(
	Chuck_Event *event, Chuck_VM *vm, CK_DL_API api, CglEventType event_type)
	: m_Event(event), m_VM(vm), m_API(api), m_EventType(event_type)
{
	GetEventQueue(event_type).push_back(this);
}

CglEvent::~CglEvent()
{
	auto &eventQueue = GetEventQueue(m_EventType);

	// TODO: this is not locksafe...can get removed while renderer is reading

	// remove from listeners list
	auto it = std::find(eventQueue.begin(), eventQueue.end(), this);
	assert(it != eventQueue.end()); // sanity check
	if (it != eventQueue.end())
		eventQueue.erase(it);
}

void CglEvent::Broadcast()
{
	// (should be) threadsafe
	m_API->vm->queue_event(m_VM, m_Event, 1, s_SharedEventQueue);
}

// broadcasts all events of type event_type
void CglEvent::Broadcast(CglEventType event_type)
{
	auto &eventQueue = GetEventQueue(event_type);
	for (auto &event : eventQueue)
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
Camera CGL::mainCamera;

// initialize offset into vtable for update() function on GGens. if < 0, not a valid offset.
t_CKINT CGL::our_update_vt_offset = -1;

// Initialization for Shred Registration structures
std::unordered_map<Chuck_VM_Shred *, bool> CGL::m_RegisteredShreds;
std::unordered_set<Chuck_VM_Shred *> CGL::m_WaitingShreds;

Chuck_DL_Api::Object CGL::s_UpdateEvent = nullptr;

// CGL static command queue initialization
std::vector<SceneGraphCommand *> CGL::m_ThisCommandQueue;
std::vector<SceneGraphCommand *> CGL::m_ThatCommandQueue;
bool CGL::m_CQReadTarget = false; // false = this, true = that
std::mutex CGL::m_CQLock;		  // only held when 1: adding new command and 2: swapping the read/write queues

// CGL window state initialization

std::mutex CGL::s_WindowStateLock;
CGL::WindowState CGL::s_WindowState = {
	1, 1,	  // window width and height (in screen coordinates)
	1, 1,	  // frame buffer width and height (in pixels)
	0.0, 0.0, // mouse X, mouse Y
	0.0, 0.0  // glfw time, delta time
};

// mouse modes
const unsigned int CGL::MOUSE_NORMAL = 0;
const unsigned int CGL::MOUSE_HIDDEN = 1;
const unsigned int CGL::MOUSE_LOCKED = 2;

// window modes

const unsigned int CGL::WINDOW_WINDOWED = 0;
const unsigned int CGL::WINDOW_FULLSCREEN = 1;
const unsigned int CGL::WINDOW_MAXIMIZED = 2;
const unsigned int CGL::WINDOW_RESTORE = 3;
const unsigned int CGL::WINDOW_SET_SIZE = 4;

// chugl start time
double CGL::chuglChuckStartTime = 0.0; // value of chuck `now` when chugl is first initialized
double CGL::chuglLastUpdateTime = 0.0;
bool CGL::useChuckTime = true;

// main loop hook
Chuck_DL_MainThreadHook *CGL::hook = nullptr;
bool CGL::hookActivated = false;

void CGL::ActivateHook()
{
	if (hookActivated || !hook)
		return;
	hook->activate(hook);
	hookActivated = true;
}

void CGL::DeactivateHook()
{
	if (!hookActivated || !hook)
		return;
	hook->deactivate(hook);
	// hookActivated = false;  // don't set to false to prevent window from reactivating and reopening after escape
}

// can pick a better name maybe...calling this wakes up renderer thread
void CGL::Render()
{
	std::unique_lock<std::mutex> lock(GameLoopLock);
	shouldRender = true;
	lock.unlock();

	renderCondition.notify_one(); // wakeup the blocking render thread
}

// sleep render thread until notified by chuck
void CGL::WaitOnUpdateDone()
{
	std::unique_lock<std::mutex> lock(GameLoopLock);
	renderCondition.wait(lock, []()
						 { return shouldRender; });
	shouldRender = false;
	// lock auto releases in destructor
}

// swap the command queue double buffer
void CGL::SwapCommandQueues()
{
	// grab lock
	std::lock_guard<std::mutex> lock(m_CQLock);
	// swap
	m_CQReadTarget = !m_CQReadTarget;
	// lock released out of scope
}

// perform all queued commands to sync the renderer scenegraph with the CGL scenegraph
void CGL::FlushCommandQueue(Scene &scene, bool swap)
{ // TODO: shouldn't command be associated with scenes?
	// swap the command queues (so we can read from what was just being written to)
	if (swap)
		SwapCommandQueues(); // Note: this already locks the command queue

	// we no longer need to hold a lock here because all writes are done to the other queue

	// get the new read queue
	std::vector<SceneGraphCommand *> &readQueue = GetReadCommandQueue(); // std::cout << "flushing " + std::to_string(readQueue.size()) + " commands\n";

	// execute all commands in the read queue
	for (SceneGraphCommand *cmd : readQueue)
	{
		cmd->execute(&scene);
		delete cmd; // release memory TODO make this a unique_ptr or something instead
	}

	readQueue.clear();
}

// adds command to the read queue
void CGL::PushCommand(SceneGraphCommand *cmd)
{
	// lock the command queue
	std::lock_guard<std::mutex> lock(m_CQLock);

	// get the write queue
	std::vector<SceneGraphCommand *> &writeQueue = GetWriteCommandQueue();

	// add the command to the write queue
	writeQueue.push_back(cmd);
}

// creates a chuck object for the passed-in mat. DOES NOT CLONE THE MATERIAL
// DOES pass a creation command to create the material on render thread
Material* CGL::CreateChuckObjFromMat(
	CK_DL_API API, Chuck_VM *VM, Material *mat, Chuck_VM_Shred *SHRED, bool refcount
)
{
	// create chuck obj
	Chuck_DL_Api::Type type = API->type->lookup(VM, mat->myCkName());
	Chuck_DL_Api::Object obj = API->object->create(SHRED, type, refcount);
	mat->m_ChuckObject = obj;

	// set address in chuck obj
	OBJ_MEMBER_INT(obj, cglmat_data_offset) = (t_CKINT)mat;

	// tell renderer to create a copy
	CGL::PushCommand(new CreateMaterialCommand(mat));
	return mat;
}

// creates a chuck object for the passed-in go. DOES NOT CLONE THE GEO
Geometry* CGL::CreateChuckObjFromGeo(CK_DL_API API, Chuck_VM *VM, Geometry *geo, Chuck_VM_Shred *SHRED, bool refcount)
{
	// create chuck obj
	Chuck_DL_Api::Type type = API->type->lookup(VM, geo->myCkName());
	Chuck_DL_Api::Object obj = API->object->create(SHRED, type, refcount);
	geo->m_ChuckObject = obj;

	// set address in chuck obj
	OBJ_MEMBER_INT(obj, geometry_data_offset) = (t_CKINT)geo;

	// tell renderer to create a copy
	CGL::PushCommand(new CreateGeometryCommand(geo));
	return geo;
}

Material* CGL::DupMeshMat(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED)
{
	if (!mesh->GetMaterial()) {
		std::cerr << "cannot duplicate a null material" << std::endl;
		return nullptr;
	}
	Material* newMat = CGL::CreateChuckObjFromMat(API, VM, mesh->GetMaterial()->Dup(), SHRED, true);
	// set new mat on this mesh!
	mesh->SetMaterial(newMat);
	// tell renderer to set new mat on this mesh
	CGL::PushCommand(new SetMeshCommand(mesh));
	return newMat;
}

Geometry* CGL::DupMeshGeo(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED)
{
	if (!mesh->GetGeometry()) {
		std::cerr << "cannot duplicate a null geometry" << std::endl;
		return nullptr;
	}

	Geometry* newGeo = CGL::CreateChuckObjFromGeo(API, VM, mesh->GetGeometry()->Dup(), SHRED, true);
	// set new geo on this mesh!
	mesh->SetGeometry(newGeo);
	// tell renderer to set new geo on this mesh
	CGL::PushCommand(new SetMeshCommand(mesh));
	return newGeo;
}


void CGL::RegisterShred(Chuck_VM_Shred *shred)
{
	m_RegisteredShreds[shred] = false;
}

void CGL::UnregisterShred(Chuck_VM_Shred *shred)
{
	m_RegisteredShreds.erase(shred);
}

bool CGL::IsShredRegistered(Chuck_VM_Shred *shred)
{
	return m_RegisteredShreds.find(shred) != m_RegisteredShreds.end();
}

void CGL::MarkShredWaited(Chuck_VM_Shred *shred)
{
	m_RegisteredShreds[shred] = true;
}

// returns false if shred has called .nextFrame() in previous frame
// without chucking to now e.g. .nextFrame() => now;
bool CGL::HasShredWaited(Chuck_VM_Shred *shred)
{
	if (!IsShredRegistered(shred))
		return true; // never called before
	return m_RegisteredShreds[shred];
}

size_t CGL::GetNumRegisteredShreds()
{
	return m_RegisteredShreds.size();
}

size_t CGL::GetNumShredsWaiting()
{
	return m_WaitingShreds.size();
}

void CGL::ClearShredWaiting()
{
	m_WaitingShreds.clear();
}

void CGL::RegisterShredWaiting(Chuck_VM_Shred *shred)
{
	// m_WaitingShreds.push_back(shred);
	m_WaitingShreds.insert(shred);
}
void CGL::UnregisterShredWaiting(Chuck_VM_Shred *shred)
{
	m_WaitingShreds.erase(shred);
}

Chuck_DL_Api::Object CGL::GetShredUpdateEvent(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM)
{
	// log size
	// std::cerr << "shred event map size: " + std::to_string(m_ShredEventMap.size()) << std::endl;
	// lookup
	if (s_UpdateEvent == nullptr)
	{
		Chuck_DL_Api::Type type = API->type->lookup(VM, "NextFrameEvent");
		Chuck_DL_Api::Object obj = API->object->create_without_shred(VM, type, true);

		// for now constructor will add chuck event to the eventQueue
		// as long as there is only one, and it's created in first call to nextFrame BEFORE renderer wakes up then this is threadsafe
		// TODO to support multiple windows, chugl event queue read/write will need to be lock protected
		cgl_update_ctor((Chuck_Object *)obj, NULL, VM, shred, API);

		s_UpdateEvent = obj;
	}
	return s_UpdateEvent;
}

Chuck_DL_Api::Object CGL::GetMainScene(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM)
{
	if (CGL::mainScene.m_ChuckObject == nullptr) {
		// TODO implement CreateSceneCommand
		Chuck_DL_Api::Type sceneCKType = API->type->lookup(VM, "GScene");
		Chuck_DL_Api::Object sceneObj = API->object->create(shred, sceneCKType, true);
		OBJ_MEMBER_INT(sceneObj, ggen_data_offset) = (t_CKINT)&CGL::mainScene;
		CGL::mainScene.m_ChuckObject = sceneObj;

		// create default camera
		Chuck_DL_Api::Type camCKType = API->type->lookup(VM, "GCamera");
		Chuck_DL_Api::Object camObj = API->object->create(shred, camCKType, true);
		// no creation command b/c window already has static copy
		CGL::PushCommand(new CreateCameraCommand(&mainCamera, &mainScene, camObj, ggen_data_offset));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, &mainCamera, RelationshipCommand::Relation::AddChild));

		// create default light
		// TODO create generic create-chuck-obj method
		Light* defaultLight = new DirLight;
		Chuck_DL_Api::Type lightType = API->type->lookup(VM, defaultLight->myCkName());
		Chuck_Object* lightObj = API->object->create(shred, lightType, true);  // refcount for scene
		OBJ_MEMBER_INT(lightObj, ggen_data_offset) = (t_CKINT)defaultLight;  // chuck obj points to sgo
		// creation command
		CGL::PushCommand(new CreateLightCommand(defaultLight, &CGL::mainScene, lightObj));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, defaultLight, RelationshipCommand::Relation::AddChild));
	}
	return CGL::mainScene.m_ChuckObject;
}

// traverses chuck-side (audio thread) scenegraph and calls user-defined update() on GGens
void CGL::UpdateSceneGraph(Scene &scene, CK_DL_API API, Chuck_VM *VM, Chuck_VM_Shred *shred)
{
	assert(CGL::our_update_vt_offset >= 0);

	t_CKTIME chuckTimeNow = API->vm->now(VM);
	t_CKTIME chuckTimeDiff = chuckTimeNow - CGL::chuglLastUpdateTime;
	t_CKFLOAT ckdt = chuckTimeDiff / API->vm->srate(VM);
	CGL::chuglLastUpdateTime = chuckTimeNow;

	Chuck_DL_Arg theArg;
	theArg.kind = kindof_FLOAT;
	if (CGL::useChuckTime)
	{
		theArg.value.v_float = ckdt;
	}
	else
	{
		theArg.value.v_float = std::min(1.0, CGL::GetTimeInfo().second); // this dt should be the same the one gotten by chuck CGL.dt()
	}

	// theArg.value.v_float = ckdt;

	std::queue<SceneGraphObject *> queue;
	queue.push(&scene);

	// BFS through graph
	// TODO: scene and
	while (!queue.empty())
	{
		SceneGraphObject *obj = queue.front();
		queue.pop();

		// call update
		Chuck_Object *ggen = obj->m_ChuckObject;
		if (ggen != nullptr && !obj->IsScene() && !obj->IsCamera())
		{
			API->vm->invoke_mfun_immediate_mode(ggen, CGL::our_update_vt_offset, VM, shred, &theArg, 1);
		}

		// add children to stack
		for (SceneGraphObject *child : obj->GetChildren())
		{
			queue.push(child);
		}
	}
}

std::pair<double, double> CGL::GetMousePos()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<double, double>(s_WindowState.mouseX, s_WindowState.mouseY);
}

std::pair<int, int> CGL::GetWindowSize()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<int, int>(s_WindowState.windowWidth, s_WindowState.windowHeight);
}

std::pair<int, int> CGL::GetFramebufferSize()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<int, int>(s_WindowState.framebufferWidth, s_WindowState.framebufferHeight);
}

std::pair<double, double> CGL::GetTimeInfo()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<double, double>(s_WindowState.glfwTime, s_WindowState.deltaTime);
}

void CGL::SetMousePos(double x, double y)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.mouseX = x;
	s_WindowState.mouseY = y;
}

void CGL::SetWindowSize(int width, int height)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.windowWidth = width;
	s_WindowState.windowHeight = height;
}

void CGL::SetFramebufferSize(int width, int height)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.framebufferWidth = width;
	s_WindowState.framebufferHeight = height;
}

void CGL::SetTimeInfo(double glfwTime, double deltaTime)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.glfwTime = glfwTime;
	s_WindowState.deltaTime = deltaTime;
}
