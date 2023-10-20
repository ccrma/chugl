#include <algorithm>
#include <iostream>
#include <stdexcept>

#include "ulib_cgl.h"
#include "ulib_colors.h"
#include "ulib_gui.h"

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
CK_DLL_TYPE_ON_INSTANTIATE(cgl_ggen_on_instantiate_listener);

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

// get FPS
CK_DLL_SFUN(cgl_get_fps);

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

	// Position 
	CK_DLL_MFUN(cgl_obj_get_pos_x);
	CK_DLL_MFUN(cgl_obj_set_pos_x);

	CK_DLL_MFUN(cgl_obj_get_pos_y);
	CK_DLL_MFUN(cgl_obj_set_pos_y);

	CK_DLL_MFUN(cgl_obj_get_pos_z);
	CK_DLL_MFUN(cgl_obj_set_pos_z);

	CK_DLL_MFUN(cgl_obj_set_pos);
	CK_DLL_MFUN(cgl_obj_get_pos);

	CK_DLL_MFUN(cgl_obj_get_pos_world);
	CK_DLL_MFUN(cgl_obj_set_pos_world);

	CK_DLL_MFUN(cgl_obj_translate);
	CK_DLL_MFUN(cgl_obj_translate_x);
	CK_DLL_MFUN(cgl_obj_translate_y);
	CK_DLL_MFUN(cgl_obj_translate_z);

	// Rotation
	CK_DLL_MFUN(cgl_obj_get_rot_x);
	CK_DLL_MFUN(cgl_obj_set_rot_x);

	CK_DLL_MFUN(cgl_obj_get_rot_y);
	CK_DLL_MFUN(cgl_obj_set_rot_y);

	CK_DLL_MFUN(cgl_obj_get_rot_z);
	CK_DLL_MFUN(cgl_obj_set_rot_z);

	CK_DLL_MFUN(cgl_obj_set_rot);
	CK_DLL_MFUN(cgl_obj_get_rot);

	CK_DLL_MFUN(cgl_obj_rotate);
	CK_DLL_MFUN(cgl_obj_rotate_x);
	CK_DLL_MFUN(cgl_obj_rotate_y);
	CK_DLL_MFUN(cgl_obj_rotate_z);

	CK_DLL_MFUN(cgl_obj_rot_on_local_axis);
	CK_DLL_MFUN(cgl_obj_rot_on_world_axis);

	CK_DLL_MFUN(cgl_obj_lookat_vec3);
	CK_DLL_MFUN(cgl_obj_lookat_dir);
	// CK_DLL_MFUN(cgl_obj_rotate_by);  // no rotate by design because converting from euler angles to quat is ambiguous

	// Scale
	CK_DLL_MFUN(cgl_obj_get_scale_x);
	CK_DLL_MFUN(cgl_obj_set_scale_x);

	CK_DLL_MFUN(cgl_obj_get_scale_y);
	CK_DLL_MFUN(cgl_obj_set_scale_y);

	CK_DLL_MFUN(cgl_obj_get_scale_z);
	CK_DLL_MFUN(cgl_obj_set_scale_z);

	CK_DLL_MFUN(cgl_obj_set_scale);
	CK_DLL_MFUN(cgl_obj_set_scale_uniform);
	CK_DLL_MFUN(cgl_obj_get_scale);
	
	CK_DLL_MFUN(cgl_obj_get_scale_world);
	CK_DLL_MFUN(cgl_obj_set_scale_world);

// parent-child scenegraph API
// CK_DLL_MFUN(cgl_obj_disconnect);
// CK_DLL_MFUN(cgl_obj_get_parent);
// CK_DLL_MFUN(cgl_obj_get_children);

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

// mouse cast from camera
CK_DLL_MFUN(chugl_cam_screen_coord_to_world_ray);
CK_DLL_MFUN(chugl_cam_world_pos_to_screen_coord);


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
CK_DLL_MFUN(cgl_geo_set_colors_vec3);
CK_DLL_MFUN(cgl_geo_set_colors_vec4);
CK_DLL_MFUN(cgl_geo_set_normals);
CK_DLL_MFUN(cgl_geo_set_normals_vec3);
CK_DLL_MFUN(cgl_geo_set_uvs);
CK_DLL_MFUN(cgl_geo_set_uvs_vec2);
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
CK_DLL_MFUN(cgl_mat_set_alpha);
CK_DLL_MFUN(cgl_mat_get_alpha);
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
CK_DLL_MFUN(cgl_set_use_world_normals);


// flat shade mat
CK_DLL_CTOR(cgl_mat_flat_ctor);

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
CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_shader);
CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_shader);
CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_string);
CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_string);

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

CK_DLL_CTOR(cgl_gsphere_ctor);
CK_DLL_CTOR(cgl_gcircle_ctor);
CK_DLL_CTOR(cgl_gplane_ctor);
CK_DLL_CTOR(cgl_gtorus_ctor);

CK_DLL_CTOR(cgl_glines_ctor);

CK_DLL_CTOR(cgl_gpoints_ctor);


// exports =========================================

t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_geo(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_texture(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_mat(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_obj(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_cam(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_scene(Chuck_DL_Query *QUERY);
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
    // set VM and API refs
    CGL::SetCKVM( QUERY->vm() );
    CGL::SetCKAPI( QUERY->api() );
    // set API in the scene graph node
    SceneGraphNode::SetCKAPI( QUERY->api() );

	// init GUI
	if (!init_chugl_gui(QUERY)) return FALSE;

    // initialize ChuGL API
	init_chugl_colors(QUERY);
    init_chugl_events(QUERY);
	init_chugl_geo(QUERY);
	init_chugl_texture(QUERY);
	init_chugl_mat(QUERY);
	init_chugl_obj(QUERY);
	init_chugl_cam(QUERY);
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
    
    // GGen instantiation listener
    QUERY->api()->type->callback_on_instantiate( cgl_ggen_on_instantiate_listener, t_ggen, QUERY->vm(), TRUE );
	
	return true;
}

// mapping shred to GGens created on that shred
static std::map<Chuck_VM_Shred *, std::list<Chuck_Object *> > g_shred2ggen;

static void detach_ggens_from_shred( Chuck_VM_Shred * shred )
{
    if( g_shred2ggen.find( shred ) == g_shred2ggen.end() )
        return;

    // get list of GGens
    std::list<Chuck_Object *> & ggens = g_shred2ggen[shred];
    
    for( auto * ggen : ggens )
    {
        // verify
        assert( ggen != NULL );
        
        // get scenegraph within the GGen
        SceneGraphObject * cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(ggen, ggen_data_offset);

		// edge case: if it's the main scene or camera, don't disconnect, because these static instances are shared across all shreds!
		// also don't remove if its the default dir light
		// TODO: why do we still need this check even though I create_without_shred ???
		if (cglObj == &CGL::mainScene || cglObj == &CGL::mainCamera || cglObj == CGL::mainScene.GetDefaultLight())
			goto reset_origin_shred;

        // if null, it is possible shred was removed between GGen instantiate and its pre-constructor
        if( cglObj )
        {
            // disconnect
			CGL::PushCommand(new DisconnectCommand(cglObj));
        }

reset_origin_shred:
        // get origin shred
        Chuck_VM_Shred * originShred = CGL::CKAPI()->object->get_origin_shred( ggen );
        // make sure if ggen has an origin shred, it is this one
        assert( !originShred || originShred == shred );
        // also clear reference to this shred
        CGL::CKAPI()->object->set_origin_shred( ggen, NULL );
    }

    // release ref count on all GGen; in theory this could be done in the loop above,
    // assuming all refcounts properly handled; in practice this is a bit "safer" in case
    // of inconstencies / bugs elsewhere
    for( auto * ggen : ggens )
    {
		// TODO: need to come up with a new ggen refcount logic that makes the following code not leak:
		/*
		while (true) {
			GCube c;
		}
		*/
        // release it
		CGL::CKAPI()->object->release(ggen);
    }
}

// called when shred is taken out of circulation for any reason
// e.g. reached end, removed, VM cleared, etc.
CK_DLL_SHREDS_WATCHER(cgl_shred_on_destroy_listener)
{
    // double check
	assert(CODE == CKVM_SHREDS_WATCH_REMOVE);

	// remove from registered list
	CGL::UnregisterShred(SHRED);

	// remove from waiting list
	CGL::UnregisterShredWaiting(SHRED);

    // detach all GGens on shred
    detach_ggens_from_shred( SHRED );
    
	// we tell the window to close when the last running
	// graphics shred is removed. 
	// this avoids the bug where window is closed 
	// when a non-graphics shred exits before 
	// any  graphics shreds have even been created
    if( g_shred2ggen.find( SHRED ) != g_shred2ggen.end() ) {
		g_shred2ggen.erase( SHRED );
		if (g_shred2ggen.empty()) {
			CGL::PushCommand(new CloseWindowCommand());
			CGL::Render();  // wake up render thread one last time to process the close window command
		}
	}

}

CK_DLL_TYPE_ON_INSTANTIATE(cgl_ggen_on_instantiate_listener)
{
    // this should never be called by VM with a NULL object
    assert( OBJECT != NULL );
    // add mapping from SHRED->GGen
    g_shred2ggen[SHRED].push_back( OBJECT );
    // add ref
	CGL::CKAPI()->object->add_ref(OBJECT);
}


//-----------------------------------------------------------------------------
// init_chugl_events()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY)
{
	// Update event ================================
	// triggered by main render thread after deepcopy is complete, and safe for chuck to begin updating the scene graph
	// (intentionally left out of CKDocs)
	QUERY->begin_class(QUERY, "NextFrameEvent", "Event");
	QUERY->add_ctor(QUERY, cgl_update_ctor);
	// QUERY->add_dtor(QUERY, cgl_update_dtor);
	cgl_next_frame_event_data_offset = QUERY->add_mvar(QUERY, "int", "@cgl_next_frame_event_data", false);

	QUERY->add_mfun(QUERY, cgl_update_event_waiting_on, "void", "waiting_on");
	QUERY->end_class(QUERY);

	// Window resize event ================================
	QUERY->begin_class(QUERY, "WindowResizeEvent", "Event");
	QUERY->doc_class(QUERY, "Event triggered whenever the ChuGL window is resized, either by the user or programmatically.");
	
	QUERY->add_ctor(QUERY, cgl_window_resize_ctor);
	// QUERY->add_dtor(QUERY, cgl_window_resize_dtor);

	window_resize_event_data_offset = QUERY->add_mvar(QUERY, "int", "@cgl_window_resize_event_data", false);
	QUERY->end_class(QUERY);

	return true;
}

CK_DLL_CTOR(cgl_update_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset) = (t_CKINT) CGL::GetShredUpdateEvent(SHRED, API, VM);
}

CK_DLL_DTOR(cgl_update_dtor)
{
	OBJ_MEMBER_INT(SELF, cgl_next_frame_event_data_offset) = 0;
}

//-----------------------------------------------------------------------------
// this is called by chuck VM at the earliest point when a shred begins to wait on an Event
// used to catch GG.nextFrame() => now; on one or more shreds
// once all expected shreds are waiting on GG.nextFrame(), this function signals the graphics-side
//-----------------------------------------------------------------------------
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

	// flips the boolean flag for this shred to true, meaning it has successfully
	CGL::MarkShredWaited(SHRED);

	// Add shred to waiting list
	CGL::RegisterShredWaiting(SHRED);
	// std::cerr << "REGISTERED SHRED WAITING" << SHRED << std::endl;

	// if #waiting == #registered, all CGL shreds have finished work, and we are safe to wakeup the renderer
	if (CGL::GetNumShredsWaiting() >= CGL::GetNumRegisteredShreds())
	{
		// traverse scenegraph and call chuck-defined update() on all GGens
		CGL::UpdateSceneGraph(CGL::mainScene, API, VM, SHRED);
        // clear thread waitlist; all expected shreds are now waiting on GG.nextFrame()
        CGL::ClearShredWaiting();
        // signal the graphics-side that audio-side is done processing for this frame
        // see CGL::WaitOnUpdateDone()
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
	// TODO need to make this threadsafe (either make static or add a lock)
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
    QUERY->doc_class(QUERY, "Base ChuGL utility class");
    QUERY->add_ex(QUERY, "basic/gameloop.ck");

	QUERY->add_svar(QUERY, "int", "MOUSE_LOCKED", TRUE, (void *)&CGL::MOUSE_LOCKED);
    QUERY->doc_var(QUERY, "When passed to GG.mouseMode(mode), hides and locks the cursor to the ChuGL window. Good for FPS cameras");
	// the cursor to become hidden when it is over a window but still want it to behave normally
	QUERY->add_svar(QUERY, "int", "MOUSE_HIDDEN", TRUE, (void *)&CGL::MOUSE_HIDDEN);
    QUERY->doc_var(QUERY, "When passed to GG.mouseMode(mode), hides the cursor when focused on the ChuGL window");
	// This mode puts no limit on the motion of the cursor.
	QUERY->add_svar(QUERY, "int", "MOUSE_NORMAL", TRUE, (void *)&CGL::MOUSE_NORMAL);
    QUERY->doc_var(QUERY, "When passed to GG.mouseMode(mode), neither hides the cursor nor locks it to the ChuGL window (this is the default behavior)");

	QUERY->add_sfun(QUERY, cgl_next_frame, "NextFrameEvent", "nextFrame");
    QUERY->doc_func(QUERY, 
		"Registers the calling shred to be notified when the next frame is finished rendering. When all graphics shreds are finished calling."
		"Note: this function returns an event that MUST be waited on for correct behavior, i.e. GG.nextFrame() => now;"
		"See the ChuGL tutorial and examples for more information."
	);

	QUERY->add_sfun(QUERY, cgl_unregister, "void", "unregister");
    QUERY->doc_func(QUERY, "For internal debug purposes, unregisters the calling shred from the ChuGL's list of graphics-related shreds.");
	QUERY->add_sfun(QUERY, cgl_register, "void", "register");
    QUERY->doc_func(QUERY, "For interal debug purposes, registers the calling shred to ChuGL's list of graphics-related shreds.");

	// window state getters
	QUERY->add_sfun(QUERY, cgl_window_get_width, "int", "windowWidth");
    QUERY->doc_func(QUERY, "Returns screen-space width of the window");
	QUERY->add_sfun(QUERY, cgl_window_get_height, "int", "windowHeight");
    QUERY->doc_func(QUERY, "Returns screen-space height of the window");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_width, "int", "frameWidth");
    QUERY->doc_func(QUERY, "Returns width of the framebuffer in pixels. Used for settings the viewport dimensions and camera aspect ratio");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_height, "int", "frameHeight");
    QUERY->doc_func(QUERY, "Returns height of the framebuffer in pixels. Used for settings the viewport dimensions and camera aspect ratio");
	QUERY->add_sfun(QUERY, cgl_window_get_time, "dur", "windowUptime");
    QUERY->doc_func(QUERY, "Time in seconds since the grapics window was opened");
	QUERY->add_sfun(QUERY, cgl_window_get_dt, "float", "dt");
    QUERY->doc_func(QUERY, "Time in seconds since the last render frame"); 
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_x, "float", "mouseX");
    QUERY->doc_func(QUERY, "Mouse horizontal position in window screen-space");
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_y, "float", "mouseY");
    QUERY->doc_func(QUERY, "Mouse vertical position in window screen-space");
	QUERY->add_sfun(QUERY, cgl_mouse_set_mode, "void", "mouseMode");
    QUERY->doc_func(QUERY, "Set mouse mode. Options are GG.MOUSE_LOCKED, GG.MOUSE_HIDDEN, GG.MOUSE_NORMAL.");
	QUERY->add_arg(QUERY, "int", "mode");

	QUERY->add_sfun(QUERY, cgl_mouse_hide, "void", "hideCursor");
    QUERY->doc_func(QUERY, "Hides mouse cursor when focused on window");
	QUERY->add_sfun(QUERY, cgl_mouse_lock, "void", "lockCursor");
    QUERY->doc_func(QUERY, "Hides and locks cursor to the window");
	QUERY->add_sfun(QUERY, cgl_mouse_show, "void", "showCursor");
    QUERY->doc_func(QUERY, "Default mouse behavior. Not hidden or locked");

	QUERY->add_sfun(QUERY, cgl_window_fullscreen, "void", "fullscreen");
    QUERY->doc_func(QUERY, "Fullscreen the window. This will significantly improve performance");
	QUERY->add_sfun(QUERY, cgl_window_windowed, "void", "windowed");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");
    QUERY->doc_func(QUERY, "Enter windowed mode and set the window size to the specified width and height");

	// QUERY->add_sfun(QUERY, cgl_window_maximize, "void", "maximize");  // kind of bugged, not sure how this is different from fullscreen
	QUERY->add_sfun(QUERY, cgl_window_set_size, "void", "resolution");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");
    QUERY->doc_func(QUERY, "Change resolution of current window. Will NOT exit fullscreen mode");

	// Main Camera
	// TODO: is it possible to add an svar of type GCamera?
	QUERY->add_sfun(QUERY, cgl_get_main_camera, "GCamera", "camera");
    QUERY->doc_func(QUERY, "Gets the GCamera used for rendering the main scene");

	// Main scene
	QUERY->add_sfun(QUERY, cgl_get_main_scene, "GScene", "scene");
    QUERY->doc_func(QUERY, "Gets the main scene, which is the root / parent of all GGens. Only the main scene and its connected GGens are rendered.");

	// chugl shred debug
	QUERY->add_sfun(QUERY, cgl_get_num_registered_shreds, "int", "numRegisteredShreds");
    QUERY->doc_func(QUERY, "Internal debug. Get number of registered graphics shreds");
	QUERY->add_sfun(QUERY, cgl_get_num_registered_waiting_shreds, "int", "numRegisteredWaitingShreds");
    QUERY->doc_func(QUERY, "Internal debug. Get number of registered graphics shreds currently waiting on GG.nextFrame()");

	// chugl chuck time for auto update(dt)
	QUERY->add_sfun(QUERY, cgl_use_chuck_time, "void", "useChuckTime");
	QUERY->add_arg(QUERY, "int", "use");
    QUERY->doc_func(QUERY, "Internal debug. Switches between using Chuck VM time or GLFW window time for auto-updates");

    // fps()
    QUERY->add_sfun(QUERY, cgl_get_fps, "int", "fps");
    QUERY->doc_func(QUERY, "FPS of current window, averaged over sliding window of 30 frames");

	QUERY->end_class(QUERY);

	return true;
}
/*============CGL static fns============*/
CK_DLL_SFUN(cgl_next_frame)
{

	// extract CglEvent from obj
	// TODO: workaround bug where create() object API is not calling preconstructors
	// https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate

	// initialize ckobj for mainScene
	CGL::GetMainScene(SHRED, API, VM);

	if (!CGL::HasShredWaited(SHRED))
	{
		API->vm->throw_exception(
			"NextFrameNotWaitedOnViolation",
			"You are calling .nextFrame() without chucking to now!\n"
			"Please replace this line with .nextFrame() => now;",
			SHRED);
	}

	RETURN->v_object = (Chuck_Object *)CGL::GetShredUpdateEvent(SHRED, API, VM)->GetEvent();

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

CK_DLL_SFUN(cgl_get_fps)
{
    RETURN->v_int = CGL::GetFPS();
}

//-----------------------------------------------------------------------------
// init_chugl_geo()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_geo(Chuck_DL_Query *QUERY)
{
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Base) , "Object");
    QUERY->doc_class(QUERY, "Base geometry class, do not instantiate directly");
    QUERY->add_ex(QUERY, "basic/polygon-modes.ck");

	QUERY->add_ctor(QUERY, cgl_geo_ctor);
	QUERY->add_dtor(QUERY, cgl_geo_dtor);
	geometry_data_offset = QUERY->add_mvar(QUERY, "int", "@geometry_data", false); // TODO: still bugged?

	// attribute locations
	QUERY->add_svar(QUERY, "int", "POS_ATTRIB_LOC", TRUE, (void *)&Geometry::POSITION_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "NORM_ATTRIB_LOC", TRUE, (void *)&Geometry::NORMAL_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "COL_ATTRIB_LOC", TRUE, (void *)&Geometry::COLOR_ATTRIB_IDX);
	QUERY->add_svar(QUERY, "int", "UV0_ATTRIB_LOC", TRUE, (void *)&Geometry::UV0_ATTRIB_IDX);

	// clone
	QUERY->add_mfun(QUERY, cgl_geo_clone, Geometry::CKName(GeometryType::Base), "clone");
    QUERY->doc_func(QUERY, "clone the geometry, including all attributes");

	// attribute setters
	QUERY->add_mfun(QUERY, cgl_geo_set_attribute, "void", "setAttribute");
	QUERY->add_arg(QUERY, "string", "name");
	QUERY->add_arg(QUERY, "int", "location");
	QUERY->add_arg(QUERY, "int", "numComponents");
	// QUERY->add_arg(QUERY, "int", "normalize");
	QUERY->add_arg(QUERY, "float[]", "data");
    QUERY->doc_func(QUERY, "Set the attribute data for the given attribute location, to be passed into the vertex shader. Builtin attribute locations are POS_ATTRIB_LOC, NORM_ATTRIB_LOC, COL_ATTRIB_LOC, UV0_ATTRIB_LOC");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions, "void", "positions");
	QUERY->add_arg(QUERY, "float[]", "positions");
    QUERY->doc_func(QUERY, "Set position attribute data from an array of floats; every 3 floats correspond to (x, y, z) values of a vertex position");

	QUERY->add_mfun(QUERY, cgl_geo_set_positions_vec3, "void", "positions");
	QUERY->add_arg(QUERY, "vec3[]", "positions");
    QUERY->doc_func(QUERY, "Set position attribute data from an array of vec3 (x, y, z)");

	QUERY->add_mfun(QUERY, cgl_geo_set_colors, "void", "colors");
	QUERY->add_arg(QUERY, "float[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of floats; every 4 floats corresdpond to (r, g, b, a) values of a vertex color);" );

    QUERY->add_mfun(QUERY, cgl_geo_set_colors_vec3, "void", "colors");
    QUERY->add_arg(QUERY, "vec3[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of vec3 (r, g, b); alpha is assumed to be 1.0" );

    QUERY->add_mfun(QUERY, cgl_geo_set_colors_vec4, "void", "colors");
    QUERY->add_arg(QUERY, "vec4[]", "colors");
    QUERY->doc_func(QUERY, "Set color attribute data from an array of vec4 (r, g, b, a)" );

	QUERY->add_mfun(QUERY, cgl_geo_set_normals, "void", "normals");
	QUERY->add_arg(QUERY, "float[]", "normals");
    QUERY->doc_func(QUERY, "Set normal attribute data from an array of floats; every 3 floats corresdpond to (x, y, z) values of a vertex normal");

    QUERY->add_mfun(QUERY, cgl_geo_set_normals_vec3, "void", "normals");
    QUERY->add_arg(QUERY, "vec3[]", "normals");
    QUERY->doc_func(QUERY, "Set normal attribute data from an array of vec3 (x, y, z)");

	QUERY->add_mfun(QUERY, cgl_geo_set_uvs, "void", "uvs");
	QUERY->add_arg(QUERY, "float[]", "uvs");
    QUERY->doc_func(QUERY, "Set UV attribute data from an array of floats; every pair of floats corresponds to (u, v) values (used for texture mapping)");

    QUERY->add_mfun(QUERY, cgl_geo_set_uvs_vec2, "void", "uvs");
    QUERY->add_arg(QUERY, "vec2[]", "uvs");
    QUERY->doc_func(QUERY, "Set UV attribute data from an array of vec2 (u,v) or (s,t)");

	QUERY->add_mfun(QUERY, cgl_geo_set_indices, "void", "indices");
	QUERY->add_arg(QUERY, "int[]", "indices");
    QUERY->doc_func(QUERY, "sets vertex indices for indexed drawing. If not set, renderer will default to non-indexed drawing");

	// TODO: add svars for attribute locations
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Box), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for boxes aka cubes");
	QUERY->add_ctor(QUERY, cgl_geo_box_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_box_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "float", "depth");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "int", "depthSeg");
    QUERY->doc_func(QUERY, "Set box dimensions and subdivisions");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Sphere), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for spheres");
	QUERY->add_ctor(QUERY, cgl_geo_sphere_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_sphere_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "widthSeg");
	QUERY->add_arg(QUERY, "int", "heightSeg");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set sphere dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// circle geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Circle), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for circles");
	QUERY->add_ctor(QUERY, cgl_geo_circle_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_circle_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "thetaStart");
	QUERY->add_arg(QUERY, "float", "thetaLength");
    QUERY->doc_func(QUERY, "Set cirle dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// plane geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Plane), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for planes");
	QUERY->add_ctor(QUERY, cgl_geo_plane_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_plane_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->add_arg(QUERY, "float", "height");
	QUERY->add_arg(QUERY, "int", "widthSegments");
	QUERY->add_arg(QUERY, "int", "heightSegments");
    QUERY->doc_func(QUERY, "Set plane dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// Torus geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Torus), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for toruses");
	QUERY->add_ctor(QUERY, cgl_geo_torus_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_torus_set, "void", "set");
	QUERY->add_arg(QUERY, "float", "radius");
	QUERY->add_arg(QUERY, "float", "tubeRadius");
	QUERY->add_arg(QUERY, "int", "radialSegments");
	QUERY->add_arg(QUERY, "int", "tubularSegments");
	QUERY->add_arg(QUERY, "float", "arcLength");
    QUERY->doc_func(QUERY, "Set torus dimensions and subdivisions");

	QUERY->end_class(QUERY);

	// lathe geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Lathe), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for constructing vertex data for lathes (i.e. rotated curves)");
	QUERY->add_ex(QUERY, "basic/polygon-modes.ck");

	QUERY->add_ctor(QUERY, cgl_geo_lathe_ctor);

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set, "void", "set");
	QUERY->add_arg(QUERY, "float[]", "path"); // these are converted to vec2s
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->doc_func(QUERY, 
		"Set lathe curve, dimensions and subdivisions. Path is rotated phiLength to form a curved surface"
		"NOTE: path takes a float[] of alternating x,y values, describing a 2D curve in the x,y plane"
		"These values are rotated around the y-axis to form a 3D surface"
	);

	QUERY->add_mfun(QUERY, cgl_geo_lathe_set_no_points, "void", "set");
	QUERY->add_arg(QUERY, "int", "segments");
	QUERY->add_arg(QUERY, "float", "phiStart");
	QUERY->add_arg(QUERY, "float", "phiLength");
    QUERY->doc_func(QUERY, "Set lathe dimensions and subdivisions while maintaining the previously set curve");

	QUERY->end_class(QUERY);

	// custom geo
	QUERY->begin_class(QUERY, Geometry::CKName(GeometryType::Custom), Geometry::CKName(GeometryType::Base) );
    QUERY->doc_class(QUERY, "Geometry class for providing your own vertex data. Used implicitly by GLines and GPoints");
    QUERY->add_ex(QUERY, "basic/custom-geo.ck");
    QUERY->add_ex(QUERY, "basic/obj-loader.ck");

	QUERY->add_ctor(QUERY, cgl_geo_custom_ctor);


	QUERY->end_class(QUERY);

	return true;
}

// CGL Geometry =======================
CK_DLL_CTOR(cgl_geo_ctor) {}
CK_DLL_DTOR(cgl_geo_dtor) // all geos can share this base destructor
{
	Geometry *geo = (Geometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	OBJ_MEMBER_INT(SELF, geometry_data_offset) = 0; // zero out the memory

	CGL::PushCommand(new DestroySceneGraphNodeCommand(geo, &CGL::mainScene));
}

CK_DLL_MFUN(cgl_geo_clone)
{
	Geometry *geo = (Geometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
	// Note: we are NOT refcounting here because we're returning a reference to the new cloned object
	// If this returned reference is assigned to a chuck variable, chuck should handle the refcounting
	// bumping the refcount here would cause a memory leak, as the refcount would never be decremented
	RETURN->v_object = CGL::CreateChuckObjFromGeo(API, VM, (Geometry *)geo->Clone(), SHRED, false)->m_ChuckObject;
}


// box geo
CK_DLL_CTOR(cgl_geo_box_ctor)
{
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new BoxGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new SphereGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new CircleGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new PlaneGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new TorusGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new LatheGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new CustomGeometry, &CGL::mainScene, SELF, geometry_data_offset));
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

CK_DLL_MFUN(cgl_geo_set_colors_vec3)
{
    CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
    auto* data = (Chuck_Array24*)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    std::vector<t_CKFLOAT> vec4s;
    vec4s.reserve(4 * data->m_vector.size());
    for (auto& val : data->m_vector) {
        vec4s.emplace_back(val.x);
        vec4s.emplace_back(val.y);
        vec4s.emplace_back(val.z);
        vec4s.emplace_back(1.0);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, vec4s, false
        )
    );
}

CK_DLL_MFUN(cgl_geo_set_colors_vec4)
{
    CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);
    auto* data = (Chuck_Array32*)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    std::vector<t_CKFLOAT> vec4s;
    vec4s.reserve(4 * data->m_vector.size());
    for (auto& val : data->m_vector) {
        vec4s.emplace_back(val.x);
        vec4s.emplace_back(val.y);
        vec4s.emplace_back(val.z);
        vec4s.emplace_back(val.w);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "color", Geometry::COLOR_ATTRIB_IDX, 4, vec4s, false
        )
    );
}

// set normals
CK_DLL_MFUN(cgl_geo_set_normals)
{
	CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

	Chuck_ArrayFloat *data = (Chuck_ArrayFloat *)GET_NEXT_OBJECT(ARGS);

	CGL::PushCommand(
		new UpdateGeometryAttributeCommand(
			geo, "normal", Geometry::NORMAL_ATTRIB_IDX, 3, data->m_vector, false)
    );
}

// set normals
CK_DLL_MFUN(cgl_geo_set_normals_vec3)
{
    CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

    auto * data = (Chuck_Array24*)GET_NEXT_OBJECT(ARGS);

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
            geo, "normal", Geometry::NORMAL_ATTRIB_IDX, 3, vec3s, false)
    );
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

// set uvs
CK_DLL_MFUN(cgl_geo_set_uvs_vec2)
{
    CustomGeometry *geo = (CustomGeometry *)OBJ_MEMBER_INT(SELF, geometry_data_offset);

    auto* data = (Chuck_Array16*)GET_NEXT_OBJECT(ARGS);

    // TODO extra round of copying here, can avoid if it matters
    std::vector<t_CKFLOAT> vec2s;
    vec2s.reserve(2 * data->m_vector.size());
    for( auto & val : data->m_vector) {
        vec2s.emplace_back(val.x);
        vec2s.emplace_back(val.y);
    }

    CGL::PushCommand(
        new UpdateGeometryAttributeCommand(
            geo, "uv", Geometry::UV0_ATTRIB_IDX, 2, vec2s, false));
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
    QUERY->doc_class(QUERY, "Base texture class, do not instantiate directly");

	QUERY->add_ctor(QUERY, cgl_texture_ctor);
	QUERY->add_dtor(QUERY, cgl_texture_dtor);
	texture_data_offset = QUERY->add_mvar(QUERY, "int", "@texture_data", false);

	// texture options (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "WRAP_REPEAT", TRUE, (void *)&CGL_Texture::Repeat);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to repeat for UVs outside of [0,1]");
	QUERY->add_svar(QUERY, "int", "WRAP_MIRRORED", TRUE, (void *)&CGL_Texture::MirroredRepeat);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to repeat and mirror for UVs outside of [0,1]");
	QUERY->add_svar(QUERY, "int", "WRAP_CLAMP", TRUE, (void *)&CGL_Texture::ClampToEdge);
    QUERY->doc_var(QUERY, "When passed into Texture.wrap(), sets the texture to clamp to the border pixel color for UVs outside of [0,1]");

	// not exposing mipmap filter options for simplicity
	QUERY->add_svar(QUERY, "int", "FILTER_NEAREST", TRUE, (void *)&CGL_Texture::Nearest);
    QUERY->doc_var(QUERY, "When passed into Texture.filter(), sets texture sampler to use nearest-neighbor filtering");
	QUERY->add_svar(QUERY, "int", "FILTER_LINEAR", TRUE, (void *)&CGL_Texture::Linear);
    QUERY->doc_var(QUERY, "When passed into Texture.filter(), sets texture sampler to use bilinear filtering");

	// member fns -----------------------------------------------------------
	QUERY->add_mfun(QUERY, cgl_texture_set_wrap, "void", "wrap");
	QUERY->add_arg(QUERY, "int", "s");
	QUERY->add_arg(QUERY, "int", "t");
    QUERY->doc_func(QUERY, "Set texture wrap modes along s and t dimensions");

	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_s, "int", "wrapS");
    QUERY->doc_func(QUERY, "Set texture wrap modes along s dimensions");
	QUERY->add_mfun(QUERY, cgl_texture_get_wrap_t, "int", "wrapT");
    QUERY->doc_func(QUERY, "Set texture wrap modes along t dimensions");

	QUERY->add_mfun(QUERY, cgl_texture_set_filter, "void", "filter");
	QUERY->add_arg(QUERY, "int", "min");
	QUERY->add_arg(QUERY, "int", "mag");
    QUERY->doc_func(QUERY, "Set texture sampler min and mag filter modes Texture.FILTER_NEAREST or Texture.FILTER_LINEAR");

	QUERY->add_mfun(QUERY, cgl_texture_get_filter_min, "int", "filterMin");
    QUERY->doc_func(QUERY, "Set texture sampler minification filter. Default FILTER_LINEAR");
	QUERY->add_mfun(QUERY, cgl_texture_get_filter_mag, "int", "filterMag");
    QUERY->doc_func(QUERY, "Set texture sampler magnification filter. Default FILTER_LINEAR");

	QUERY->end_class(QUERY);

	// FileTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, "FileTexture", "Texture");
    QUERY->doc_class(QUERY, "Class for loading textures from external files");
    QUERY->add_ex(QUERY, "textures/textures-1.ck");

	QUERY->add_ctor(QUERY, cgl_texture_file_ctor);

	QUERY->add_mfun(QUERY, cgl_texture_file_set_filepath, "string", "path");
	QUERY->add_arg(QUERY, "string", "path");
    QUERY->doc_func(QUERY, "loads texture data from path");

	QUERY->add_mfun(QUERY, cgl_texture_file_get_filepath, "string", "path");
    QUERY->doc_func(QUERY, "Get the filepath for the currently-loaded texture");

	QUERY->end_class(QUERY);

	// DataTexture -----------------------------------------------------------
	QUERY->begin_class(QUERY, "DataTexture", "Texture");
    QUERY->doc_class(QUERY, "Class for dynamically creating textures from chuck arrays");
    QUERY->add_ex(QUERY, "audioshader/audio-texture.ck");

	QUERY->add_ctor(QUERY, cgl_texture_rawdata_ctor);

	QUERY->add_mfun(QUERY, cgl_texture_rawdata_set_data, "void", "data");
	QUERY->add_arg(QUERY, "float[]", "data");
	QUERY->add_arg(QUERY, "int", "width");
	QUERY->add_arg(QUERY, "int", "height");
    QUERY->doc_func(QUERY, 
		"Set the data for this texture. Data is expected to be a float array of length width*height*4, "
		"where each pixel is represented by 4 floats for r,g,b,a."
		"Currently only supports unsigned bytes, so each float must be in range [0,255]"
	);
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
	OBJ_MEMBER_INT(SELF, texture_data_offset) = 0;

	CGL::PushCommand(new DestroySceneGraphNodeCommand(texture, &CGL::mainScene));
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
	CGL_Texture *texture = new CGL_Texture(CGL_TextureType::File2D);
	CGL::PushCommand(new CreateSceneGraphNodeCommand(texture, &CGL::mainScene, SELF, texture_data_offset));
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
	CGL_Texture *texture = new CGL_Texture(CGL_TextureType::RawData);
	CGL::PushCommand(new CreateSceneGraphNodeCommand(texture, &CGL::mainScene, SELF, texture_data_offset));
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
	QUERY->doc_class(QUERY, "Base material class, do not instantiate directly");
    QUERY->add_ex(QUERY, "basic/polygon-modes.ck");

	QUERY->add_ctor(QUERY, cgl_mat_ctor);
	QUERY->add_dtor(QUERY, cgl_mat_dtor);
	cglmat_data_offset = QUERY->add_mvar(QUERY, "int", "@cglmat_data", false);

	// clone
	QUERY->add_mfun(QUERY, cgl_mat_clone, Material::CKName(MaterialType::Base), "clone");
	QUERY->doc_func(QUERY, "Clones this material");

	// Material params (static constants) ---------------------------------
	QUERY->add_svar(QUERY, "int", "POLYGON_FILL", TRUE, (void *)&Material::POLYGON_FILL);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to set polygon rendering to filled triangles, default");
	QUERY->add_svar(QUERY, "int", "POLYGON_LINE", TRUE, (void *)&Material::POLYGON_LINE);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to render geometry as a line mesh");
	QUERY->add_svar(QUERY, "int", "POLYGON_POINT", TRUE, (void *)&Material::POLYGON_POINT);
	QUERY->doc_var(QUERY, "pass into Material.polygonMode() to render geometry as a point mesh (points drawn at each vertex position)");

	// line rendering static vars
	QUERY->add_svar(QUERY, "int", "LINE_SEGMENTS", TRUE, (void *)&Material::LINE_SEGMENTS_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a series of segments, where every 2 vertices is a line");
	QUERY->add_svar(QUERY, "int", "LINE_STRIP", TRUE, (void *)&Material::LINE_STRIP_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a continuous strip, connecting each vertex to the next");
	QUERY->add_svar(QUERY, "int", "LINE_LOOP", TRUE, (void *)&Material::LINE_LOOP_MODE);
	QUERY->doc_var(QUERY, "used by LineMaterial to render lines as a loop, connecting each vertex to the next, and also the last to the first");

	QUERY->add_mfun(QUERY, cgl_mat_set_polygon_mode, "int", "polygonMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->doc_func(QUERY, "set the rendering mode for this material, can be Material.POLYGON_FILL, Material.POLYGON_LINE, or Material.POLYGON_POINT");

	QUERY->add_mfun(QUERY, cgl_mat_get_polygon_mode, "int", "polygonMode");
	QUERY->doc_func(QUERY, "get the rendering mode for this material, can be Material.POLYGON_FILL, Material.POLYGON_LINE, or Material.POLYGON_POINT");

	QUERY->add_mfun(QUERY, cgl_mat_set_point_size, "void", "pointSize");
	QUERY->add_arg(QUERY, "float", "size");
	QUERY->doc_func(QUERY, "set point size if rendering with Material.POLYGON_POINT. NOTE: unsupported on macOS");

	QUERY->add_mfun(QUERY, cgl_mat_set_color, "vec3", "color");
	QUERY->add_arg(QUERY, "vec3", "rgb");
	QUERY->doc_func(QUERY, "set material color uniform as an rgb. Alpha set to 1.0");

	QUERY->add_mfun(QUERY, cgl_mat_set_alpha, "float", "alpha");
	QUERY->add_arg(QUERY, "float", "alpha");
	QUERY->doc_func(QUERY, "set the alpha of the material color");

	QUERY->add_mfun(QUERY, cgl_mat_get_alpha, "float", "alpha");
	QUERY->doc_func(QUERY, "get the alpha of the material color");

	QUERY->add_mfun(QUERY, cgl_mat_set_line_width, "void", "lineWidth");
	QUERY->add_arg(QUERY, "float", "width");
	QUERY->doc_func(QUERY, "set line width if rendering with Material.POLYGON_LINE. NOTE: unsupported on macOS");

	// norm mat fns
	QUERY->add_mfun(QUERY, cgl_set_use_local_normals, "void", "localNormals");
	QUERY->doc_func(QUERY, "For NormalsMaterial: color surface using local-space normals");
	
	QUERY->add_mfun(QUERY, cgl_set_use_world_normals, "void", "worldNormals");
	QUERY->doc_func(QUERY, "For NormalsMaterial: color surface using world-space normals");

	// phong mat fns (TODO add getters, need to fix texture creation)
	QUERY->add_mfun(QUERY, cgl_mat_phong_set_log_shininess, "float", "shine");
	QUERY->add_arg(QUERY, "float", "shininess");
	QUERY->doc_func(QUERY, "For PhongMaterial: set shininess exponent, default 5");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_diffuse_map, "void", "diffuseMap");
	QUERY->add_arg(QUERY, "Texture", "tex");
	QUERY->doc_func(QUERY, "Set diffuse map texture");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_map, "void", "specularMap");
	QUERY->add_arg(QUERY, "Texture", "tex");
	QUERY->doc_func(QUERY, "Set specular map texture");

	QUERY->add_mfun(QUERY, cgl_mat_phong_set_specular_color, "vec3", "specular");
	QUERY->add_arg(QUERY, "vec3", "color");
	QUERY->doc_func(QUERY, "For PhongMat: set specular color");

	// shader mat fns  (TODO allow setting vert and frag separately)
	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_shaders, "void", "shaders");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex and fragment shaders");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_vert_shader, "void", "vertShader");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex shader path");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_frag_shader, "void", "fragShader");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set fragment shader path");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_vert_string, "void", "vertString");
	QUERY->add_arg(QUERY, "string", "vert");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set vertex shader string");

	QUERY->add_mfun(QUERY, cgl_mat_custom_shader_set_frag_string, "void", "fragString");
	QUERY->add_arg(QUERY, "string", "frag");
	QUERY->doc_func(QUERY, "For ShaderMaterial: set fragment shader string");


	// points mat fns
	QUERY->add_mfun(QUERY, cgl_mat_points_set_size_attenuation, "int", "attenuatePoints");
	QUERY->add_arg(QUERY, "int", "attenuation");
	QUERY->doc_func(QUERY, "For PointMaterial: set if point size should be scaled by distance from camera");

	QUERY->add_mfun(QUERY, cgl_mat_points_get_size_attenuation, "int", "attenuatePoints");
	QUERY->doc_func(QUERY, "For PointMaterial: returns 1 if point size is being scaled by distance from camera, else 0");

	QUERY->add_mfun(QUERY, cgl_mat_points_set_sprite, "Texture", "pointSprite");
	QUERY->add_arg(QUERY, "Texture", "sprite");
	QUERY->doc_func(QUERY, "For PointMaterial: set sprite texture for point sprite rendering");

	// line mat fns
	QUERY->add_mfun(QUERY, cgl_mat_line_set_mode, "int", "lineMode");
	QUERY->add_arg(QUERY, "int", "mode");
	QUERY->doc_func(QUERY, "For LineMaterial: set line mode. Can be Material.LINE_SEGMENTS, Material.LINE_STRIP, or Material.LINE_LOOP");


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
	QUERY->doc_class(QUERY, "Color each pixel using the surface normal at that point");
	QUERY->add_ctor(QUERY, cgl_mat_norm_ctor);
	QUERY->end_class(QUERY);

	// flat material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Flat), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using a flat color");
	QUERY->add_ctor(QUERY, cgl_mat_flat_ctor);
	QUERY->end_class(QUERY);

	// phong specular material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Phong), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using a phong specular shading");
	QUERY->add_ctor(QUERY, cgl_mat_phong_ctor);
	QUERY->end_class(QUERY);

	// custom shader material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::CustomShader), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using the custom glsl shaders you provide via Material.shaders()");
    QUERY->add_ex(QUERY, "audioshader/audio-texture.ck");


	QUERY->add_ctor(QUERY, cgl_mat_custom_shader_ctor);
	QUERY->end_class(QUERY);

	// points material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Points), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Used by GPoints");
    QUERY->add_ex(QUERY, "basic/points.ck");
	QUERY->add_ctor(QUERY, cgl_mat_points_ctor);
	QUERY->end_class(QUERY);

	// mango material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Mango), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Color each pixel using its UV coordinates. Looks like a mango, yum.");
	QUERY->add_ctor(QUERY, cgl_mat_mango_ctor);
	QUERY->end_class(QUERY);

	// line material
	QUERY->begin_class(QUERY, Material::CKName(MaterialType::Line), Material::CKName(MaterialType::Base));
	QUERY->doc_class(QUERY, "Used by GLines");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek-minimal.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek.ck");

	QUERY->add_ctor(QUERY, cgl_mat_line_ctor);
	QUERY->end_class(QUERY);

	return true;
}
// CGL Materials ===================================================
CK_DLL_CTOR(cgl_mat_ctor)
{
	// dud, do nothing for now
}

CK_DLL_DTOR(cgl_mat_dtor) // all geos can share this base destructor
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	OBJ_MEMBER_INT(SELF, cglmat_data_offset) = 0; // zero out the memory

	CGL::PushCommand(new DestroySceneGraphNodeCommand(mat, &CGL::mainScene));
}


CK_DLL_MFUN(cgl_mat_clone)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	RETURN->v_object = CGL::CreateChuckObjFromMat(API, VM, (Material*) mat->Clone(), SHRED, false)->m_ChuckObject;
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

CK_DLL_MFUN(cgl_mat_set_alpha)
{
	Material *mat = (Material*)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKFLOAT alpha = GET_NEXT_FLOAT(ARGS);
	mat->SetAlpha(alpha);

	RETURN->v_float = alpha;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(Material::COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_get_alpha)
{
	Material *mat = (Material*)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	RETURN->v_float = mat->GetAlpha();
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
	NormalMaterial *normMat = new NormalMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(normMat, &CGL::mainScene, SELF, cglmat_data_offset));
}

CK_DLL_MFUN(cgl_set_use_local_normals)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	mat->UseLocalNormals();

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(Material::USE_LOCAL_NORMALS_UNAME))));
}

CK_DLL_MFUN(cgl_set_use_world_normals)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	mat->UseWorldNormals();

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *(mat->GetUniform(Material::USE_LOCAL_NORMALS_UNAME))));
}


// flat mat fns
CK_DLL_CTOR(cgl_mat_flat_ctor)
{
	FlatMaterial *flatMat = new FlatMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(flatMat, &CGL::mainScene, SELF, cglmat_data_offset));
}


// phong mat fns
CK_DLL_CTOR(cgl_mat_phong_ctor)
{
	PhongMaterial *phongMat = new PhongMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(phongMat, &CGL::mainScene, SELF, cglmat_data_offset));
}

CK_DLL_MFUN(cgl_mat_phong_set_diffuse_map)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetDiffuseMap(tex);

	// TODO: how do I return the chuck texture object?
	// RETURN->v_object = SELF;

	// a lot of redundant work (entire uniform vector is copied). can optimize later
	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(Material::DIFFUSE_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_map)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetSpecularMap(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_MAP_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_specular_color)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKVEC3 color = GET_NEXT_VEC3(ARGS);
	mat->SetSpecularColor(color.x, color.y, color.z);

	RETURN->v_vec3 = color;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(PhongMaterial::SPECULAR_COLOR_UNAME)));
}

CK_DLL_MFUN(cgl_mat_phong_set_log_shininess)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKFLOAT shininess = GET_NEXT_FLOAT(ARGS);
	mat->SetLogShininess(shininess);

	RETURN->v_float = shininess;

	CGL::PushCommand(new UpdateMaterialUniformCommand(
		mat, *mat->GetUniform(Material::SHININESS_UNAME)));
}

// custom shader mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_custom_shader_ctor)
{
	ShaderMaterial *shaderMat = new ShaderMaterial("", "");
	CGL::PushCommand(new CreateSceneGraphNodeCommand(shaderMat, &CGL::mainScene, SELF, cglmat_data_offset));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_shaders)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertPath = GET_NEXT_STRING(ARGS);
	Chuck_String *fragPath = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragPath->str(), true);
	mat->SetVertShader(vertPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_shader)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertPath = GET_NEXT_STRING(ARGS);

	mat->SetVertShader(vertPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_shader)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *fragPath = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragPath->str(), true);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_vert_string)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *vertString = GET_NEXT_STRING(ARGS);

	mat->SetVertShader(vertString->str(), false);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}

CK_DLL_MFUN(cgl_mat_custom_shader_set_frag_string)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	if (mat->GetMaterialType() != MaterialType::CustomShader)
	{
		std::cerr << "ERROR: material is not a custom shader material, cannot set custom shaders" << std::endl;
		return;
	}

	Chuck_String *fragString = GET_NEXT_STRING(ARGS);

	mat->SetFragShader(fragString->str(), false);

	CGL::PushCommand(new UpdateMaterialShadersCommand((ShaderMaterial *)mat));
}


// points mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_points_ctor)
{
	PointsMaterial *pointsMat = new PointsMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(pointsMat, &CGL::mainScene, SELF, cglmat_data_offset));
}

CK_DLL_MFUN(cgl_mat_points_set_size_attenuation)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	t_CKINT attenuation = GET_NEXT_INT(ARGS);
	mat->SetSizeAttenuation(attenuation);

	RETURN->v_int = attenuation;

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SIZE_ATTENUATION_UNAME)));
}

CK_DLL_MFUN(cgl_mat_points_get_size_attenuation)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);

	RETURN->v_int = mat->GetSizeAttenuation() ? 1 : 0;
}

CK_DLL_MFUN(cgl_mat_points_set_sprite)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
	CGL_Texture *tex = (CGL_Texture *)OBJ_MEMBER_INT(GET_NEXT_OBJECT(ARGS), texture_data_offset);
	mat->SetSprite(tex);

	CGL::PushCommand(new UpdateMaterialUniformCommand(mat, *mat->GetUniform(PointsMaterial::POINT_SPRITE_TEXTURE_UNAME)));
}

// set point color

// mango mat fns ---------------------------------
CK_DLL_CTOR(cgl_mat_mango_ctor)
{
	MangoMaterial *mangoMat = new MangoMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(mangoMat, &CGL::mainScene, SELF, cglmat_data_offset));
}

// line mat fns ---------------------------------

CK_DLL_CTOR(cgl_mat_line_ctor)
{
	LineMaterial *lineMat = new LineMaterial;
	CGL::PushCommand(new CreateSceneGraphNodeCommand(lineMat, &CGL::mainScene, SELF, cglmat_data_offset));
}


CK_DLL_MFUN(cgl_mat_line_set_mode)
{
	Material *mat = (Material *)OBJ_MEMBER_INT(SELF, cglmat_data_offset);
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
	QUERY->doc_class(QUERY, "Base class for all Gens. Can be extended to create your own, or initialized as an empty group container");
	QUERY->add_ex(QUERY, "basic/orbits.ck");
	QUERY->add_ex(QUERY, "basic/circles.ck");

	QUERY->add_ctor(QUERY, cgl_obj_ctor);
	QUERY->add_dtor(QUERY, cgl_obj_dtor);
	ggen_data_offset = QUERY->add_mvar(QUERY, "int", "@ggen_data", false);

	QUERY->add_mfun(QUERY, cgl_obj_get_id, "int", "id");
	QUERY->doc_func(QUERY, "Internal debug. Get the unique ChuGL ID of this GGen");

	QUERY->add_mfun(QUERY, cgl_obj_update, "void", "update");
	QUERY->add_arg(QUERY, "float", "dt");
	QUERY->doc_func(
		QUERY, 
		"This method is automatically invoked once per frame for all GGens connected to the scene graph."
		"Override this method in custom GGen classes to implement your own update logic."
	);

	QUERY->add_mfun(QUERY, cgl_obj_get_right, "vec3", "right");
	QUERY->doc_func(QUERY, "Get the right vector of this GGen in world space");

	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "forward");
	QUERY->doc_func(QUERY, "Get the forward vector of this GGen in world space");

	QUERY->add_mfun(QUERY, cgl_obj_get_up, "vec3", "up");
	QUERY->doc_func(QUERY, "Get the up vector of this GGen in world space");

	// Position ===============================================================

	// float posX()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_x, "float", "posX");
	QUERY->doc_func(QUERY, "Get X position of this GGen in local space");

	// float posX(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_x, "float", "posX");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set X position of this GGen in local space");

	// float posY()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_y, "float", "posY");
	QUERY->doc_func(QUERY, "Get Y position of this GGen in local space");

	// float posY(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_y, "float", "posY");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set Y position of this GGen in local space");

	// float posZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_z, "float", "posZ");
	QUERY->doc_func(QUERY, "Get Z position of this GGen in local space");

	// float posZ(float)
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_z, "float", "posZ");
	QUERY->add_arg(QUERY, "float", "pos");
	QUERY->doc_func(QUERY, "Set Z position of this GGen in local space");

	// vec3 pos()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos, "vec3", "pos");
	QUERY->doc_func(QUERY, "Get object position in local space");

	// vec3 pos( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_pos, "vec3", "pos");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Set object position in local space");

	// vec3 posWorld()
	QUERY->add_mfun(QUERY, cgl_obj_get_pos_world, "vec3", "posWorld");
	QUERY->doc_func(QUERY, "Get object position in world space");

	// vec3 posWorld( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_pos_world, "vec3", "posWorld");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Set object position in world space");

	// GGen translate( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_translate, "GGen", "translate");
	QUERY->add_arg(QUERY, "vec3", "translation");
	QUERY->doc_func(QUERY, "Translate this GGen by the given vector");

	// GGen translateX( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_x, "GGen", "translateX");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the X axis in local space");

	// GGen translateY( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_y, "GGen", "translateY");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the Y axis in local space");

	// GGen translateZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_translate_z, "GGen", "translateZ");
	QUERY->add_arg(QUERY, "float", "amt");
	QUERY->doc_func(QUERY, "Translate this GGen by given amount on the Z axis in local space");

	// Rotation ===============================================================

	// float rotX()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_x, "vec3", "rotX");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the X axis in local space");

	// float rotX( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_x, "vec3", "rotX");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the X axis in local space to the given radians");

	// float rotY()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_y, "vec3", "rotY");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the Y axis in local space");

	// float rotY( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_y, "vec3", "rotY");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the Y axis in local space to the given radians");

	// float rotZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot_z, "vec3", "rotZ");
	QUERY->doc_func(QUERY, "Get the rotation of this GGen on the Z axis in local space");

	// float rotZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot_z, "vec3", "rotZ");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Set the rotation of this GGen on the Z axis in local space to the given radians");

	// vec3 rot()
	QUERY->add_mfun(QUERY, cgl_obj_get_rot, "vec3", "rot");
	QUERY->doc_func(QUERY, "Get object rotation in local space as euler angles in radians");

	// vec3 rot( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_rot, "vec3", "rot");
	QUERY->add_arg(QUERY, "vec3", "eulers");
	QUERY->doc_func(QUERY, "Set rotation of this GGen in local space as euler angles");

	// GGen rotate( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_rotate, "GGen", "rotate");
	QUERY->add_arg(QUERY, "vec3", "eulers");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given euler angles in local space");

	// GGen rotateX( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_x, "GGen", "rotateX");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the X axis in local space");

	// GGen rotateY( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_y, "GGen", "rotateY");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the Y axis in local space");

	// GGen rotateZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_rotate_z, "GGen", "rotateZ");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the Z axis in local space");

	// GGen rotateOnLocalAxis( vec3, float )
	QUERY->add_mfun(QUERY, cgl_obj_rot_on_local_axis, "GGen", "rotateOnLocalAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the given axis in local space");

	// GGen rotateOnWorldAxis( vec3, float )
	QUERY->add_mfun(QUERY, cgl_obj_rot_on_world_axis, "GGen", "rotateOnWorldAxis");
	QUERY->add_arg(QUERY, "vec3", "axis");
	QUERY->add_arg(QUERY, "float", "radians");
	QUERY->doc_func(QUERY, "Rotate this GGen by the given radians on the given axis in world space");

	// GGen lookAt( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_lookat_vec3, "GGen", "lookAt");
	QUERY->add_arg(QUERY, "vec3", "pos");
	QUERY->doc_func(QUERY, "Look at the given position in world space");

	// vec3 lookAtDir()
	QUERY->add_mfun(QUERY, cgl_obj_get_forward, "vec3", "lookAtDir");
	QUERY->doc_func(QUERY, "Get the direction this GGen is looking, i.e. the forward vector of this GGen in world space");

	// Scale ===============================================================

	// float scaX()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_x, "float", "scaX");
	QUERY->doc_func(QUERY, "Get X scale of this GGen in local space");

	// float scaX( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_x, "float", "scaX");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set X scale of this GGen in local space");

	// float scaY()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_y, "float", "scaY");
	QUERY->doc_func(QUERY, "Get Y scale of this GGen in local space");

	// float scaY( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_y, "float", "scaY");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set Y scale of this GGen in local space");

	// float scaZ()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_z, "float", "scaZ");
	QUERY->doc_func(QUERY, "Get Z scale of this GGen in local space");

	// float scaZ( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_z, "float", "scaZ");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set Z scale of this GGen in local space");

	// vec3 sca()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale, "vec3", "sca");
	QUERY->doc_func(QUERY, "Get object scale in local space");

	// vec3 sca( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale, "vec3", "sca");
	QUERY->add_arg(QUERY, "vec3", "scale");
	QUERY->doc_func(QUERY, "Set object scale in local space");

	// vec3 sca( float )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_uniform, "vec3", "sca");
	QUERY->add_arg(QUERY, "float", "scale");
	QUERY->doc_func(QUERY, "Set object scale in local space uniformly across all axes");

	// vec3 scaWorld()
	QUERY->add_mfun(QUERY, cgl_obj_get_scale_world, "vec3", "scaWorld");
	QUERY->doc_func(QUERY, "Get object scale in world space");

	// vec3 scaWorld( vec3 )
	QUERY->add_mfun(QUERY, cgl_obj_set_scale_world, "vec3", "scaWorld");
	QUERY->add_arg(QUERY, "vec3", "scale");
	QUERY->doc_func(QUERY, "Set object scale in world space");

	// scenegraph relationship methods =======================================

	// overload GGen --> GGen
	QUERY->add_op_overload_binary(QUERY, ggen_op_gruck, "GGen", "-->",
								  "GGen", "lhs", "GGen", "rhs");

	// overload GGen --< GGen
	QUERY->add_op_overload_binary(QUERY, ggen_op_ungruck, "GGen", "--<",
								  "GGen", "lhs", "GGen", "rhs");

	QUERY->end_class(QUERY);

	return true;
}

// CGLObject DLL ==============================================
CK_DLL_CTOR(cgl_obj_ctor)
{
	Chuck_DL_Api::Type type = API->type->lookup(VM, "GGen");  // TODO cache this
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
		CGL::PushCommand(new CreateSceneGraphNodeCommand(cglObj, &CGL::mainScene, SELF, ggen_data_offset));
	}
}

CK_DLL_DTOR(cgl_obj_dtor)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	OBJ_MEMBER_INT(SELF, ggen_data_offset) = 0;

	CGL::PushCommand(new DestroySceneGraphNodeCommand(cglObj, &CGL::mainScene));
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

// Position Impl ===============================================================

CK_DLL_MFUN(cgl_obj_get_pos_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetPosition().x;
}

CK_DLL_MFUN(cgl_obj_set_pos_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.x = posX;
	cglObj->SetPosition(pos);
	RETURN->v_float = posX;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetPosition().y;
}

CK_DLL_MFUN(cgl_obj_set_pos_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posY = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.y = posY;
	cglObj->SetPosition(pos);
	RETURN->v_float = posY;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetPosition().z;
}

CK_DLL_MFUN(cgl_obj_set_pos_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT posZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 pos = cglObj->GetPosition();
	pos.z = posZ;
	cglObj->SetPosition(pos);
	RETURN->v_float = posZ;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_pos)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_pos_world)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetWorldPosition();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_pos_world)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetWorldPosition(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdatePositionCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_translate)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 trans = GET_NEXT_VEC3(ARGS);
	cglObj->Translate(glm::vec3(trans.x, trans.y, trans.z));

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({amt, 0, 0});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({0, amt, 0});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_translate_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT amt = GET_NEXT_FLOAT(ARGS);
	cglObj->Translate({0, 0, amt});

	// add to command queue
	CGL::PushCommand(new UpdatePositionCommand(cglObj));

	RETURN->v_object = SELF;
}

// Rotation Impl ===============================================================


CK_DLL_MFUN(cgl_obj_get_rot_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetRotation().x;
}

CK_DLL_MFUN(cgl_obj_set_rot_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateX(rad);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetRotation().y;
}

CK_DLL_MFUN(cgl_obj_set_rot_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateY(rad);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetRotation().z;
}

CK_DLL_MFUN(cgl_obj_set_rot_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateZ(rad);
	RETURN->v_float = rad;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_rot)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetEulerRotationRadians();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_rot)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetRotation(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->Rotate(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateX(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateY(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rotate_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT rad = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateZ(rad);
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_rot_on_local_axis)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnLocalAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new UpdateRotationCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_rot_on_world_axis)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	t_CKFLOAT deg = GET_NEXT_FLOAT(ARGS);
	cglObj->RotateOnWorldAxis(glm::vec3(vec.x, vec.y, vec.z), deg);

	CGL::PushCommand(new UpdateRotationCommand(cglObj));

	RETURN->v_object = SELF;
}

CK_DLL_MFUN(cgl_obj_lookat_vec3)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->LookAt(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_object = SELF;
	CGL::PushCommand(new UpdateRotationCommand(cglObj));
}

// Scale impl ===============================================================

CK_DLL_MFUN(cgl_obj_get_scale_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetScale().x;
}

CK_DLL_MFUN(cgl_obj_set_scale_x)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT scaleX = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.x = scaleX;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleX;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetScale().y;
}

CK_DLL_MFUN(cgl_obj_set_scale_y)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT scaleY = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.y = scaleY;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleY;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	RETURN->v_float = cglObj->GetScale().z;
}

CK_DLL_MFUN(cgl_obj_set_scale_z)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT scaleZ = GET_NEXT_FLOAT(ARGS);
	glm::vec3 scale = cglObj->GetScale();
	scale.z = scaleZ;
	cglObj->SetScale(scale);
	RETURN->v_float = scaleZ;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}


CK_DLL_MFUN(cgl_obj_get_scale)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetScale();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_scale)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_set_scale_uniform)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT s = GET_NEXT_FLOAT(ARGS);
	cglObj->SetScale({s, s, s});
	RETURN->v_vec3 = {s, s, s};
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

CK_DLL_MFUN(cgl_obj_get_scale_world)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	const auto &vec = cglObj->GetWorldScale();
	RETURN->v_vec3 = {vec.x, vec.y, vec.z};
}

CK_DLL_MFUN(cgl_obj_set_scale_world)
{
	SceneGraphObject *cglObj = (SceneGraphObject *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 vec = GET_NEXT_VEC3(ARGS);
	cglObj->SetWorldScale(glm::vec3(vec.x, vec.y, vec.z));
	RETURN->v_vec3 = vec;
	CGL::PushCommand(new UpdateScaleCommand(cglObj));
}

// Scenegraph Relationship Impl ===============================================================

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

	// light
	QUERY->add_mfun(QUERY, cgl_scene_get_default_light, Light::CKName(LightType::Base), "light");
	QUERY->doc_func(QUERY, "Get the default directional light of the scene");

	QUERY->add_mfun(QUERY, cgl_scene_get_num_lights, "int", "numLights");
	QUERY->doc_func(QUERY, "Get the number of instantiated lights");


	// fog member vars
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
	Light* defaultLight = scene->GetDefaultLight();
	RETURN->v_object = defaultLight ? defaultLight->m_ChuckObject : nullptr;
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
	QUERY->doc_class(QUERY, "Camera class. Static--all instances point to the same underlying ChuGL main camera");
    QUERY->add_ex(QUERY, "basic/mousecast.ck");

	QUERY->add_ctor(QUERY, cgl_cam_ctor);

	// static vars
	// perspective mode
	QUERY->add_svar(QUERY, "int", "PERSPECTIVE", true, (void *)&Camera::MODE_PERSPECTIVE);
	QUERY->add_svar(QUERY, "int", "ORTHO", true, (void *)&Camera::MODE_ORTHO);

	QUERY->add_mfun(QUERY, cgl_cam_set_mode_persp, "void", "perspective");
	QUERY->doc_func(QUERY, "Set camera to perspective mode");

	QUERY->add_mfun(QUERY, cgl_cam_set_mode_ortho, "void", "orthographic");
	QUERY->doc_func(QUERY, "Set camera to orthographic mode");

	QUERY->add_mfun(QUERY, cgl_cam_get_mode, "int", "mode");
	QUERY->doc_func(QUERY, "Get camera mode. Can be GCamera.MODE_PERSP or GCamera.MODE_ORTHO");


	// clipping planes
	QUERY->add_mfun(QUERY, cgl_cam_set_clip, "void", "clip");
	QUERY->add_arg(QUERY, "float", "near");
	QUERY->add_arg(QUERY, "float", "far");
	QUERY->doc_func(QUERY, "Set camera clipping planes");

	QUERY->add_mfun(QUERY, cgl_cam_get_clip_near, "float", "clipNear");
	QUERY->doc_func(QUERY, "get near clipping plane");


	QUERY->add_mfun(QUERY, cgl_cam_get_clip_far, "float", "clipFar");
	QUERY->doc_func(QUERY, "get far clipping plane");

	// fov (in degrees)
	QUERY->add_mfun(QUERY, cgl_cam_set_pers_fov, "float", "fov");
	QUERY->add_arg(QUERY, "float", "f");
	QUERY->doc_func(QUERY, "(perspective mode) set the field of view in degrees");

	QUERY->add_mfun(QUERY, cgl_cam_get_pers_fov, "float", "fov");
	QUERY->doc_func(QUERY, "(perspective mode) get the field of view in degrees");


	// ortho view size
	QUERY->add_mfun(QUERY, cgl_cam_set_ortho_size, "float", "viewSize");
	QUERY->add_arg(QUERY, "float", "s");
	QUERY->doc_func(QUERY, "(orthographic mode) set the height of the view in pixels. Width is automatically calculated based on aspect ratio");

	QUERY->add_mfun(QUERY, cgl_cam_get_ortho_size, "float", "viewSize");
	QUERY->doc_func(QUERY, "(orthographic mode) get the height of the view in pixels");

	// raycast from mousepos
	QUERY->add_mfun(QUERY, chugl_cam_screen_coord_to_world_ray, "vec3", "screenCoordToWorldRay");
	QUERY->add_arg(QUERY, "float", "screenX");
	QUERY->add_arg(QUERY, "float", "screenY");
	QUERY->doc_func(QUERY, 
		"Get a ray in world space representing the normalized directional vector from camera world position to screen coordinate"
		"screenX and screenY are screen coordinates, which you can get from GG.mouseX() and GG.mouseY() or you can pass coordinates directly"
		"useful if you want to do mouse picking or raycasting"
	);

	QUERY->add_mfun(QUERY, chugl_cam_world_pos_to_screen_coord, "vec3", "worldPosToScreenCoord");
	QUERY->add_arg(QUERY, "vec3", "worldPos");
	QUERY->doc_func(QUERY, 
		"Get a screen coordinate from a world position by casting a ray from worldPos back to the camera and finding the intersection with the near clipping plane"
		"worldPos is a vec3 representing a world position"
		"returns a vec3. X and Y are screen coordinates, Z is the depth-value of the worldPos"
		"Remember, screen coordinates have origin at the top-left corner of the window"
	);

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

CK_DLL_MFUN(chugl_cam_screen_coord_to_world_ray)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKFLOAT screenX = GET_NEXT_FLOAT(ARGS);
	t_CKFLOAT screenY = GET_NEXT_FLOAT(ARGS);

	// first convert to normalized device coordinates in range [-1, 1]
	auto windowSize = CGL::GetWindowSize();
	float x = ( (2.0f * screenX) / windowSize.first ) - 1.0f;
	float y = 1.0f - ( (2.0f * screenY) / windowSize.second );
	float z = 1.0f;
	glm::vec4 ray_clip = glm::vec4(x, y, -1.0, 1.0);
	// convert to eye space
	glm::mat4 proj = cam->GetProjectionMatrix();
	glm::vec4 ray_eye = glm::inverse(proj) * ray_clip;
	// convert to world space
	ray_eye = glm::vec4(ray_eye.x, ray_eye.y, -1.0, 0.0);
	glm::mat4 view = cam->GetViewMatrix();
	glm::vec3 ray_wor = glm::inverse(view) * ray_eye;
	// normalize
	ray_wor = glm::normalize(ray_wor);
	RETURN->v_vec3 = {ray_wor.x, ray_wor.y, ray_wor.z};
}

CK_DLL_MFUN(chugl_cam_world_pos_to_screen_coord)
{
	Camera *cam = (Camera *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	t_CKVEC3 worldPos = GET_NEXT_VEC3(ARGS);

	// first convert to clip space
	glm::mat4 view = cam->GetViewMatrix();
	glm::mat4 proj = cam->GetProjectionMatrix();
	glm::vec4 clipPos = proj * view * glm::vec4(worldPos.x, worldPos.y, worldPos.z, 1.0f);

	// convert to normalized device coordinates in range [-1, 1]
	auto windowSize = CGL::GetWindowSize();
	float x = (clipPos.x / clipPos.w + 1.0f) / 2.0f * windowSize.first;
	// need to invert y because screen coordinates are top-left origin
	float y = (1.0f - clipPos.y / clipPos.w) / 2.0f * windowSize.second;
	// z is depth value
	float z = clipPos.z / clipPos.w;
	RETURN->v_vec3 = {x, y, z};
}

//-----------------------------------------------------------------------------
// init_chugl_mesh()
//-----------------------------------------------------------------------------
t_CKBOOL init_chugl_mesh(Chuck_DL_Query *QUERY)
{
	// EM_log(CK_LOG_INFO, "ChuGL scene");

	QUERY->begin_class(QUERY, "GMesh", "GGen");
	QUERY->doc_class(QUERY, "Mesh class. A mesh is a geometry and a material. It can be added to a scene to be rendered. Parent class of GCube, GSphere, GCircle, etc");

	QUERY->add_ctor(QUERY, cgl_mesh_ctor);

	QUERY->add_mfun(QUERY, cgl_mesh_set, "void", "set");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Set the geometry and material of the mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_set_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->add_arg(QUERY, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->doc_func(QUERY, "Set the mesh geometry");


	QUERY->add_mfun(QUERY, cgl_mesh_set_mat, Material::CKName(MaterialType::Base), "mat");
	QUERY->add_arg(QUERY, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Set the mesh material");

	QUERY->add_mfun(QUERY, cgl_mesh_get_geo, Geometry::CKName(GeometryType::Base) , "geo");
	QUERY->doc_func(QUERY, "Get the mesh geometry");

	QUERY->add_mfun(QUERY, cgl_mesh_get_mat, Material::CKName(MaterialType::Base), "mat");
	QUERY->doc_func(QUERY, "Get the mesh material");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_mat, Material::CKName(MaterialType::Base), "dupMat");
	QUERY->doc_func(QUERY, "Clone the mesh material and set it to this mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_geo, Geometry::CKName(GeometryType::Base) , "dupGeo");
	QUERY->doc_func(QUERY, "Clone the mesh geometry and set it to this mesh");

	QUERY->add_mfun(QUERY, cgl_mesh_dup_all, "GMesh", "dup");
	QUERY->doc_func(QUERY, "Clone both the mesh geometry and material and set it to this mesh");

	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCube", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses BoxGeometry and PhongMaterial");

	QUERY->add_ctor(QUERY, cgl_gcube_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GSphere", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses SphereGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gsphere_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GCircle", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CircleGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gcircle_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPlane", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses PlaneGeometry and PhongMaterial");
	QUERY->add_ctor(QUERY, cgl_gplane_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GTorus", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses TorusGeometry and PhongMaterial");

	QUERY->add_ctor(QUERY, cgl_gtorus_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GLines", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CustomGeometry and LineMaterial");
	QUERY->add_ex(QUERY, "basic/circles.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek-minimal.ck");
    QUERY->add_ex(QUERY, "sndpeek/sndpeek.ck");

	QUERY->add_ctor(QUERY, cgl_glines_ctor);
	QUERY->end_class(QUERY);

	QUERY->begin_class(QUERY, "GPoints", "GMesh");
	QUERY->doc_class(QUERY, "Creates a Mesh that uses CustomGeometry and PointMaterial");
	QUERY->add_ctor(QUERY, cgl_gpoints_ctor);
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

	Mesh *mesh = new Mesh;
	CGL::PushCommand(new CreateMeshCommand(mesh, &CGL::mainScene, SELF, ggen_data_offset));
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


	CGL::PushCommand(new SetMeshCommand(mesh));
}

CK_DLL_MFUN(cgl_mesh_set_mat)
{
	Mesh *mesh = (Mesh *)OBJ_MEMBER_INT(SELF, ggen_data_offset);
	Chuck_Object *mat_obj = GET_NEXT_OBJECT(ARGS);
	Material *mat = mat_obj == nullptr ? nullptr : (Material *)OBJ_MEMBER_INT(mat_obj, cglmat_data_offset);

	mesh->SetMaterial(mat);

	RETURN->v_object = mat_obj;


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
	// don't refcount here because will be refcounted when we assign to mat
	CGL::CreateChuckObjFromMat(API, VM, mat, SHRED, false);  
	Geometry* geo = new BoxGeometry;
	// don't refcount here because will be refcounted when we assign to mat
	CGL::CreateChuckObjFromGeo(API, VM, geo, SHRED, false);

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

	std::vector<double> firstLine = {0, 0, 0, 0, 0, 0};

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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new PointLight, &CGL::mainScene, SELF, ggen_data_offset));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(new DirLight, &CGL::mainScene, SELF, ggen_data_offset));
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

CglEvent* CGL::s_UpdateEvent = nullptr;

// CGL static command queue initialization
std::vector<SceneGraphCommand *> CGL::m_ThisCommandQueue;
std::vector<SceneGraphCommand *> CGL::m_ThatCommandQueue;
bool CGL::m_CQReadTarget = false; // false = this, true = that
std::mutex CGL::m_CQLock;		  // only held when 1: adding new command and 2: swapping the read/write queues

// CGL window state initialization
std::mutex CGL::s_WindowStateLock;
CGL::WindowState CGL::s_WindowState;

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
	hookActivated = false;  // don't set to false to prevent window from reactivating and reopening after escape
}

// VM and API references
Chuck_VM * CGL::s_vm = NULL;
CK_DL_API CGL::s_api = NULL;

void CGL::SetCKVM( Chuck_VM * theVM )
{
    s_vm = theVM;
}

void CGL::SetCKAPI( CK_DL_API theAPI )
{
    s_api = theAPI;
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
	Chuck_DL_Api::Object ckobj = API->object->create(SHRED, type, refcount);

	// tell renderer to create a copy
	CGL::PushCommand(new CreateSceneGraphNodeCommand(mat, &CGL::mainScene, ckobj, cglmat_data_offset));

	return mat;
}

// creates a chuck object for the passed-in go. DOES NOT CLONE THE GEO
Geometry* CGL::CreateChuckObjFromGeo(CK_DL_API API, Chuck_VM *VM, Geometry *geo, Chuck_VM_Shred *SHRED, bool refcount)
{
	// create chuck obj
	Chuck_DL_Api::Type type = API->type->lookup(VM, geo->myCkName());
	Chuck_DL_Api::Object ckobj = API->object->create(SHRED, type, refcount);

	// tell renderer to create a copy
	CGL::PushCommand(new CreateSceneGraphNodeCommand(geo, &CGL::mainScene, ckobj, geometry_data_offset));

	return geo;
}

Material* CGL::DupMeshMat(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED)
{
	if (!mesh->GetMaterial()) {
		std::cerr << "cannot duplicate a null material" << std::endl;
		return nullptr;
	}

	// again, we don't bump the refcount here because it's already bumped in SetMaterial()
	Material* newMat = CGL::CreateChuckObjFromMat(API, VM, mesh->GetMaterial()->Dup(), SHRED, false);
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

	// we don't bump the refcount here because it's already bumped in SetGeometry()
	Geometry* newGeo = CGL::CreateChuckObjFromGeo(API, VM, mesh->GetGeometry()->Dup(), SHRED, false);
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

CglEvent* CGL::GetShredUpdateEvent(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM)
{
	// log size
	// std::cerr << "shred event map size: " + std::to_string(m_ShredEventMap.size()) << std::endl;
	// lookup
	if (CGL::s_UpdateEvent == nullptr)
	{
		Chuck_DL_Api::Type type = API->type->lookup(VM, "NextFrameEvent");
		Chuck_DL_Api::Object obj = API->object->create_without_shred(VM, type, true);

		// for now constructor will add chuck event to the eventQueue
		// as long as there is only one, and it's created in first call to nextFrame BEFORE renderer wakes up then this is threadsafe
		// TODO to support multiple windows, chugl event queue read/write will need to be lock protected
		// cgl_update_ctor((Chuck_Object *)obj, NULL, VM, shred, API);
		CGL::s_UpdateEvent = new CglEvent((Chuck_Event *)obj, VM, API, CglEventType::CGL_UPDATE);

		OBJ_MEMBER_INT(obj, cgl_next_frame_event_data_offset) = (t_CKINT)CGL::s_UpdateEvent;
	}
	return CGL::s_UpdateEvent;
}

Chuck_DL_Api::Object CGL::GetMainScene(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM)
{
	// instantiate scene + default camera and light if not already
	if (CGL::mainScene.m_ChuckObject == nullptr) {
		// TODO implement CreateSceneCommand
		Chuck_DL_Api::Type sceneCKType = API->type->lookup(VM, "GScene");
		Chuck_DL_Api::Object sceneObj = API->object->create(shred, sceneCKType, true);
		// Chuck_DL_Api::Object sceneObj = API->object->create_without_shred(VM, sceneCKType, true);
		OBJ_MEMBER_INT(sceneObj, ggen_data_offset) = (t_CKINT)&CGL::mainScene;
		CGL::mainScene.m_ChuckObject = sceneObj;

		// create default camera
		Chuck_DL_Api::Type camCKType = API->type->lookup(VM, "GCamera");
		Chuck_DL_Api::Object camObj = API->object->create(shred, camCKType, true);
		// Chuck_DL_Api::Object camObj = API->object->create_without_shred(VM, camCKType, true);
		// no creation command b/c window already has static copy
		CGL::PushCommand(new CreateSceneGraphNodeCommand(&mainCamera, &mainScene, camObj, ggen_data_offset));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, &mainCamera, RelationshipCommand::Relation::AddChild));

		// create default light
		// TODO create generic create-chuck-obj method
		Light* defaultLight = new DirLight;
		Chuck_DL_Api::Type lightType = API->type->lookup(VM, defaultLight->myCkName());
		Chuck_Object* lightObj = API->object->create(shred, lightType, true);  // refcount for scene
		// Chuck_Object* lightObj = API->object->create_without_shred(VM, lightType, true);  // refcount for scene
		// OBJ_MEMBER_INT(lightObj, ggen_data_offset) = (t_CKINT)defaultLight;  // chuck obj points to sgo
		// creation command
		CGL::PushCommand(new CreateSceneGraphNodeCommand(defaultLight, &CGL::mainScene, lightObj, ggen_data_offset));
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

void CGL::SetFPS( int fps )
{
    std::unique_lock<std::mutex> lock(s_WindowStateLock);
    s_WindowState.fps = fps;
}

int CGL::GetFPS()
{
    std::unique_lock<std::mutex> lock(s_WindowStateLock);
    return s_WindowState.fps;
}
