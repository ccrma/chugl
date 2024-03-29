#include "window.h"

#include "ulib_cgl.h"
#include "ulib_colors.h"
#include "ulib_gui.h"
#include "ulib_geometry.h"
#include "ulib_texture.h"
#include "ulib_material.h"
#include "ulib_ggen.h"
#include "ulib_camera.h"
#include "ulib_mesh.h"
#include "ulib_light.h"
#include "ulib_scene.h"
#include "ulib_assimp.h"
#include "ulib_postprocess.h"
#include "ulib_text.h"

#include "renderer/scenegraph/Camera.h"
#include "renderer/scenegraph/Command.h"
#include "renderer/scenegraph/Scene.h"
#include "renderer/scenegraph/Light.h"
#include "renderer/scenegraph/Locator.h"
#include "renderer/scenegraph/chugl_postprocess.h"

#include <queue>

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

// glfw window params
CK_DLL_SFUN(cgl_window_get_width);
CK_DLL_SFUN(cgl_window_get_height);
CK_DLL_SFUN(cgl_window_get_aspect_ratio);
CK_DLL_SFUN(cgl_window_get_time);
CK_DLL_SFUN(cgl_window_get_dt);

// glfw mouse params
CK_DLL_SFUN(cgl_mouse_get_pos_x);
CK_DLL_SFUN(cgl_mouse_get_pos_y);
CK_DLL_SFUN(cgl_mouse_get_dx);
CK_DLL_SFUN(cgl_mouse_get_dy);
CK_DLL_SFUN(cgl_mouse_set_mode);
CK_DLL_SFUN(cgl_mouse_hide);
CK_DLL_SFUN(cgl_mouse_lock);
CK_DLL_SFUN(cgl_mouse_show);
CK_DLL_SFUN(chugl_imgui_want_capture_mouse);

// glfw framebuffer params
CK_DLL_SFUN(cgl_framebuffer_get_width);
CK_DLL_SFUN(cgl_framebuffer_get_height);

// glfw windowing fns
CK_DLL_SFUN(cgl_window_fullscreen);
CK_DLL_SFUN(cgl_window_windowed);
// CK_DLL_SFUN(cgl_window_maximize);
CK_DLL_SFUN(cgl_window_set_size);

CK_DLL_SFUN(cgl_window_get_title);
CK_DLL_SFUN(cgl_window_set_title);

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

// Get root of post processing chain
CK_DLL_SFUN(cgl_get_pp_root);

// set default font
CK_DLL_SFUN(cgl_set_default_font);
CK_DLL_SFUN(cgl_get_default_font);



// exports =========================================

t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query *QUERY);
t_CKBOOL create_chugl_default_objs(Chuck_DL_Query *QUERY);

// initialize data offsets
t_CKUINT CGL::geometry_data_offset = 0;
t_CKUINT CGL::material_data_offset = 0;
t_CKUINT CGL::texture_data_offset = 0;
t_CKUINT CGL::ggen_data_offset = 0;
t_CKUINT CGL::pp_effect_offset_data = 0;
t_CKUINT CGL::gui_data_offset = 0;
static t_CKUINT cgl_next_frame_event_data_offset = 0;
static t_CKUINT window_resize_event_data_offset = 0;

t_CKBOOL init_chugl(Chuck_DL_Query *QUERY)
{
    // get the VM and API
    Chuck_VM * vm = QUERY->ck_vm(QUERY);
    CK_DL_API api = QUERY->ck_api(QUERY);

    // set VM and API refs
    CGL::SetCKVM( vm );
    CGL::SetCKAPI( api );
    // set API in the scene graph node
    SceneGraphNode::SetCKAPI( api );
	// set API in the locator service
	Locator::SetCKAPI( api );


    // initialize ChuGL API
	init_chugl_colors(QUERY);
    init_chugl_events(QUERY);
	init_chugl_geometry(QUERY);
	init_chugl_texture(QUERY);
	init_chugl_material(QUERY);
	init_chugl_obj(QUERY);
	init_chugl_camera(QUERY);
	init_chugl_mesh(QUERY);
	init_chugl_text(QUERY);
	init_chugl_light(QUERY);
	init_chugl_scene(QUERY);
    init_chugl_assimp(QUERY);
	init_chugl_postprocess(QUERY);
    create_chugl_default_objs(QUERY);
	init_chugl_static_fns(QUERY);
	
	// init GUI
	if (!init_chugl_gui(QUERY)) return FALSE;

	return true;
}

//-----------------------------------------------------------------------------
// create_chugl_default_objs()
//-----------------------------------------------------------------------------
t_CKBOOL create_chugl_default_objs(Chuck_DL_Query *QUERY)
{
    // get the VM and API
    Chuck_VM * vm = QUERY->ck_vm(QUERY);
    CK_DL_API api = QUERY->ck_api(QUERY);
	// threadsafe event queue
    CglEvent::s_SharedEventQueue = api->vm->create_event_buffer(vm);
	assert(CglEvent::s_SharedEventQueue);

	// shred destroy listener
	QUERY->register_shreds_watcher(QUERY, cgl_shred_on_destroy_listener, ckvm_shreds_watch_REMOVE, NULL);

	// update() vt offset
    // get the GGen type
    Chuck_Type * t_ggen = api->type->lookup(vm, "GGen");
    // find the offset for update
    CGL::our_update_vt_offset = api->type->get_vtable_offset(vm, t_ggen, "update");

    // GGen instantiation listener
    api->type->callback_on_instantiate( cgl_ggen_on_instantiate_listener, t_ggen, vm, FALSE );

	return true;
}

// called when shred is taken out of circulation for any reason
// e.g. reached end, removed, VM cleared, etc.
CK_DLL_SHREDS_WATCHER(cgl_shred_on_destroy_listener)
{
    // double check
	assert(CODE == ckvm_shreds_watch_REMOVE);

	// remove from registered list
	CGL::UnregisterShred(SHRED);

	// remove from waiting list
	CGL::UnregisterShredWaiting(SHRED);

    // detach all GGens on shred
    CGL::DetachGGensFromShred( SHRED );
    
	// we tell the window to close when the last running
	// graphics shred is removed. 
	// this avoids the bug where window is closed 
	// when a non-graphics shred exits before 
	// any  graphics shreds have even been created
	bool isGraphicsShred = CGL::EraseFromShred2GGenMap(SHRED);
	
	// behavior: after window opens (ie GG.nextFrame() has been called once)
	// if the #shreds calling GG.nextFrame() drops to zero, window is 
	// immediately closed and chuck VM is shut down.
	if ( 
		isGraphicsShred && CGL::NoActiveGraphicsShreds()
	) {
		Locator::GC();  // gc to force any disconnected GGens to be cleaned up
		CGL::PushCommand(new CloseWindowCommand(&CGL::mainScene));
		CGL::Render();  // wake up render thread one last time to process the close window command
	}
}

CK_DLL_TYPE_ON_INSTANTIATE(cgl_ggen_on_instantiate_listener)
{
    // this should never be called by VM with a NULL object
    assert( OBJECT != NULL );
    // add mapping from SHRED->GGen
	CGL::RegisterGGenToShred(SHRED, OBJECT);
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
		// Garbage collect (TODO add API function to control this)
		Locator::GC();  
	}
}

// window resize event
CK_DLL_CTOR(cgl_window_resize_ctor)
{
	// store reference to our new class
	OBJ_MEMBER_INT(SELF, window_resize_event_data_offset) = (t_CKINT) new CglEvent(
		(Chuck_Event *)SELF, VM, API, CglEventType::CGL_WINDOW_RESIZE);
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

	// window functions
	QUERY->add_sfun(QUERY, cgl_window_get_width, "int", "windowWidth");
    QUERY->doc_func(QUERY, "Returns screen-space width of the window");
	QUERY->add_sfun(QUERY, cgl_window_get_height, "int", "windowHeight");
    QUERY->doc_func(QUERY, "Returns screen-space height of the window");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_width, "int", "frameWidth");
    QUERY->doc_func(QUERY, "Returns width of the framebuffer in pixels. Used for settings the viewport dimensions and camera aspect ratio");
	QUERY->add_sfun(QUERY, cgl_framebuffer_get_height, "int", "frameHeight");
    QUERY->doc_func(QUERY, "Returns height of the framebuffer in pixels. Used for settings the viewport dimensions and camera aspect ratio");
	QUERY->add_sfun(QUERY, cgl_window_get_aspect_ratio, "float", "aspect");
	QUERY->doc_func(QUERY, "Returns aspect ratio of the window. Equal to windowWidth / windowHeight");
	QUERY->add_sfun(QUERY, cgl_window_get_time, "dur", "windowUptime");
    QUERY->doc_func(QUERY, "Time in seconds since the grapics window was opened");
	QUERY->add_sfun(QUERY, cgl_window_get_dt, "float", "dt");
    QUERY->doc_func(QUERY, "Time in seconds since the last render frame"); 

	// mouse functions
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_x, "float", "mouseX");
    QUERY->doc_func(QUERY, "Mouse horizontal position in window screen-space");
	QUERY->add_sfun(QUERY, cgl_mouse_get_pos_y, "float", "mouseY");
    QUERY->doc_func(QUERY, "Mouse vertical position in window screen-space");

	QUERY->add_sfun(QUERY, cgl_mouse_get_dx, "float", "mouseDX");
	QUERY->doc_func(QUERY, "Mouse horizontal position change since last frame");
	QUERY->add_sfun(QUERY, cgl_mouse_get_dy, "float", "mouseDY");
	QUERY->doc_func(QUERY, "Mouse vertical position change since last frame");

	QUERY->add_sfun(QUERY, cgl_mouse_set_mode, "void", "mouseMode");
    QUERY->doc_func(QUERY, "Set mouse mode. Options are GG.MOUSE_LOCKED, GG.MOUSE_HIDDEN, GG.MOUSE_NORMAL.");
	QUERY->add_arg(QUERY, "int", "mode");

	QUERY->add_sfun(QUERY, cgl_mouse_hide, "void", "hideCursor");
    QUERY->doc_func(QUERY, "Hides mouse cursor when focused on window");
	QUERY->add_sfun(QUERY, cgl_mouse_lock, "void", "lockCursor");
    QUERY->doc_func(QUERY, "Hides and locks cursor to the window");
	QUERY->add_sfun(QUERY, cgl_mouse_show, "void", "showCursor");
    QUERY->doc_func(QUERY, "Default mouse behavior. Not hidden or locked");

	QUERY->add_sfun(QUERY, chugl_imgui_want_capture_mouse, "int", "mouseCapturedByUI");
	QUERY->doc_func(QUERY, 
		"Returns true if the ImGui library is currently capturing the mouse."
		"I.e. if the mouse is currently over an ImGui window"
		"Useful for disabling mouse controls when the mouse is over an ImGui window"
	);

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

	QUERY->add_sfun(QUERY, cgl_window_get_title, "string", "windowTitle");
	QUERY->doc_func(QUERY, "Returns the title of the window");
	QUERY->add_sfun(QUERY, cgl_window_set_title, "string", "windowTitle");
	QUERY->add_arg(QUERY, "string", "title");
	QUERY->doc_func(QUERY, "Sets the title of the window");

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

	// Post Processing
	QUERY->add_sfun(QUERY, cgl_get_pp_root, PP::Effect::CKName(PP::Type::Base), "fx");
	QUERY->doc_func(QUERY, "Returns the root of the post processing chain. See the ChuGL post processing tutorial for more information.");

	// Default font
	QUERY->add_sfun(QUERY, cgl_set_default_font, "string", "font");
	QUERY->add_arg(QUERY, "string", "fontPath");
	QUERY->doc_func(QUERY, "Sets the default font for all text rendering.");

	QUERY->add_sfun(QUERY, cgl_get_default_font, "string", "font");
	QUERY->doc_func(QUERY, "Gets the path to the default font file for all text rendering.");

	QUERY->end_class(QUERY);

	return true;
}

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
// get window aspect
CK_DLL_SFUN(cgl_window_get_aspect_ratio) { RETURN->v_float = CGL::GetAspectRatio(); }
// get glfw time
CK_DLL_SFUN(cgl_window_get_time) { RETURN->v_dur = API->vm->srate(VM) * CGL::GetTimeInfo().first; }
// get glfw dt
CK_DLL_SFUN(cgl_window_get_dt) { RETURN->v_float = CGL::GetTimeInfo().second; }
// get mouse x
CK_DLL_SFUN(cgl_mouse_get_pos_x) { RETURN->v_float = CGL::GetMousePos().first; }
// get mouse y
CK_DLL_SFUN(cgl_mouse_get_pos_y) { RETURN->v_float = CGL::GetMousePos().second; }
// get mouse dx
CK_DLL_SFUN(cgl_mouse_get_dx) { RETURN->v_float = CGL::GetMouseDelta().first; }
// get mouse dy
CK_DLL_SFUN(cgl_mouse_get_dy) { RETURN->v_float = CGL::GetMouseDelta().second; }

// set mouse mode
CK_DLL_SFUN(cgl_mouse_set_mode)
{
	t_CKINT mode = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetMouseModeCommand(&CGL::mainScene, mode));
}

CK_DLL_SFUN(cgl_mouse_hide) { CGL::PushCommand(new SetMouseModeCommand(&CGL::mainScene, CGL::MOUSE_HIDDEN)); }
CK_DLL_SFUN(cgl_mouse_lock) { CGL::PushCommand(new SetMouseModeCommand(&CGL::mainScene, CGL::MOUSE_LOCKED)); }
CK_DLL_SFUN(cgl_mouse_show) { CGL::PushCommand(new SetMouseModeCommand(&CGL::mainScene, CGL::MOUSE_NORMAL)); }
CK_DLL_SFUN(chugl_imgui_want_capture_mouse) { RETURN->v_int = CGL::GetMouseCapturedByImGUI() ? 1 : 0; }

CK_DLL_SFUN(cgl_window_fullscreen) { CGL::PushCommand(new SetWindowModeCommand(&CGL::mainScene, CGL::WINDOW_FULLSCREEN)); }

CK_DLL_SFUN(cgl_window_windowed)
{
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetWindowModeCommand(&CGL::mainScene, CGL::WINDOW_WINDOWED, width, height));
}

CK_DLL_SFUN(cgl_window_set_size)
{
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetWindowModeCommand(&CGL::mainScene, CGL::WINDOW_SET_SIZE, width, height));
}

// get glfw window title
CK_DLL_SFUN(cgl_window_get_title) { 
	RETURN->v_string = (Chuck_String *)API->object->create_string(
		VM, CGL::mainScene.m_WindowTitle.c_str(), false
	);
}

// set glfw window title
CK_DLL_SFUN(cgl_window_set_title) { 
	Chuck_String *title= GET_NEXT_STRING(ARGS);
	CGL::PushCommand(new SetWindowTitleCommand(&CGL::mainScene, API->object->str(title)));
	RETURN->v_string = title;
}

CK_DLL_SFUN(cgl_get_main_camera)
{
	// RETURN->v_object = (Chuck_Object *)CGL::GetMainCamera(
	// 	SHRED, API, VM);
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
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

CK_DLL_SFUN(cgl_get_pp_root)
{
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	RETURN->v_object = scene->GetRootEffect()->m_ChuckObject;
}

CK_DLL_SFUN(cgl_set_default_font) 
{ 
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	Chuck_String *fontPath = GET_NEXT_STRING(ARGS);

	CGL::PushCommand(new UpdateDefaultFontCommand(scene, API->object->str(fontPath)));

	RETURN->v_string = fontPath;
}

CK_DLL_SFUN(cgl_get_default_font) 
{ 
	Scene *scene = (Scene *)CGL::GetSGO(CGL::GetMainScene(SHRED, API, VM));
	RETURN->v_string = (Chuck_String *)API->object->create_string(
		VM, scene->GetDefaultFontPath().c_str(), false
	);
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
const t_CKUINT CGL::MOUSE_NORMAL = 0;
const t_CKUINT CGL::MOUSE_HIDDEN = 1;
const t_CKUINT CGL::MOUSE_LOCKED = 2;

// window modes
const t_CKUINT CGL::WINDOW_WINDOWED = 0;
const t_CKUINT CGL::WINDOW_FULLSCREEN = 1;
const t_CKUINT CGL::WINDOW_MAXIMIZED = 2;
const t_CKUINT CGL::WINDOW_RESTORE = 3;
const t_CKUINT CGL::WINDOW_SET_SIZE = 4;

// chugl start time
double CGL::chuglChuckStartTime = 0.0; // value of chuck `now` when chugl is first initialized
double CGL::chuglLastUpdateTime = 0.0;
bool CGL::useChuckTime = true;

// main loop hook
Chuck_DL_MainThreadHook *CGL::hook = nullptr;
bool CGL::hookActivated = false;

// Shred to GGen bookkeeping
std::unordered_map<Chuck_VM_Shred *, std::unordered_set<Chuck_Object *> > CGL::s_Shred2GGen;
std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> CGL::s_GGen2Shred;
std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> CGL::s_GGen2OriginShred;

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
CK_DL_API CGL::API; // specifically named API for decoupled OBJ_MEMBER_* access

void CGL::SetCKVM( Chuck_VM * theVM )
{
    s_vm = theVM;
}

void CGL::SetCKAPI( CK_DL_API theAPI )
{
    s_api = API = theAPI;
}

void CGL::DetachGGensFromShred(Chuck_VM_Shred *shred)
{
    if( s_Shred2GGen.find( shred ) == s_Shred2GGen.end() )
        return;

    // create a copy of the set of GGen IDs created on this shred that have NOT been garbage collected
	// we need a copy here because GGens which are disconnected may be garbage collected, which will
	// invalidate the iterator, i.e. they remove themselves from the Shred2GGen map
	// we store IDs and not pointers because pointers may be invalidated by garbage collection during 
	// the loop below
	std::vector<size_t> ggensCopy; 
	ggensCopy.reserve(s_Shred2GGen[shred].size());

	// populate copy
	for (auto *ggen : s_Shred2GGen[shred])
	{
		SceneGraphObject* sgo = CGL::GetSGO(ggen);
		assert(sgo);
		ggensCopy.push_back(sgo->GetID());
	}

    for( auto ggen_id : ggensCopy )
    {
        // verify
        assert( ggen_id > 0 );

		// get then GGen
		SceneGraphObject* sgo = (SceneGraphObject*) Locator::GetNode(ggen_id, true);
		if (!sgo) continue;  // already deleted

		Chuck_Object* ckobj = sgo->m_ChuckObject;
		if (!ckobj) continue;  // already deleted

		// edge case: if it's the main scene or camera, don't disconnect, because these static instances are shared across all shreds!
		// also don't remove if its the default dir light
		// TODO: why do we still need this check even though I create_without_shred ???
		if (sgo == &CGL::mainScene || sgo == &CGL::mainCamera || sgo == CGL::mainScene.GetDefaultLight())
			continue;

		// disconnect
		CGL::PushCommand(new DisconnectCommand(sgo));
    }
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
void CGL::FlushCommandQueue(Scene &scene)
{ // TODO: shouldn't command be associated with scenes?
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

//-----------------------------------------------------------------------------
// ChuGL Object Creation Helpers
//-----------------------------------------------------------------------------

CglEvent* CGL::GetShredUpdateEvent(Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM)
{
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
		OBJ_MEMBER_INT(sceneObj, CGL::GetGGenDataOffset()) = (t_CKINT)&CGL::mainScene;
		CGL::mainScene.m_ChuckObject = sceneObj;

		// create default camera
		Chuck_DL_Api::Type camCKType = API->type->lookup(VM, "GCamera");
		Chuck_DL_Api::Object camObj = API->object->create(shred, camCKType, true);
        // initial main scene camera position
        mainCamera.SetPosition( glm::vec3(0,0,5) );

        // no creation command b/c window already has static copy
		CGL::PushCommand(new CreateSceneGraphNodeCommand(&mainCamera, &mainScene, camObj, CGL::GetGGenDataOffset(), API));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, &mainCamera, RelationshipCommand::Relation::AddChild));
        // update camera position command
        CGL::PushCommand(new UpdatePositionCommand(&mainCamera));

		// create default light
		// TODO create generic create-chuck-obj method
		Light* defaultLight = new DirLight;
		Chuck_DL_Api::Type lightType = API->type->lookup(VM, defaultLight->myCkName());
		Chuck_Object* lightObj = API->object->create(shred, lightType, true);  // refcount for scene
		// creation command
		CGL::PushCommand(new CreateSceneGraphNodeCommand(defaultLight, &CGL::mainScene, lightObj, CGL::GetGGenDataOffset(), API));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, defaultLight, RelationshipCommand::Relation::AddChild));

		// create root post process effect
		PP::PassThroughEffect* rootEffect = new PP::PassThroughEffect;
		Chuck_DL_Api::Type effectType = API->type->lookup(VM, rootEffect->myCkName());
		// TODO: refcounting should jsut happen in the scene assignment
		Chuck_Object* effectObj = API->object->create(shred, effectType, true);  // refcount for scene
		// creation command
		CGL::PushCommand(new CreateSceneGraphNodeCommand(rootEffect, &CGL::mainScene, effectObj, CGL::GetPPEffectDataOffset(), API));
		// add to scene command
		CGL::PushCommand(new SetSceneRootEffectCommand(&CGL::mainScene, rootEffect));
	}

	return CGL::mainScene.m_ChuckObject;
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(mat, &CGL::mainScene, ckobj, CGL::GetMaterialDataOffset(), API));

	return mat;
}

// creates a chuck object for the passed-in go. DOES NOT CLONE THE GEO
Geometry* CGL::CreateChuckObjFromGeo(CK_DL_API API, Chuck_VM *VM, Geometry *geo, Chuck_VM_Shred *SHRED, bool refcount)
{
	// create chuck obj
	Chuck_DL_Api::Type type = API->type->lookup(VM, geo->myCkName());
	Chuck_DL_Api::Object ckobj = API->object->create(SHRED, type, refcount);

	// tell renderer to create a copy
	CGL::PushCommand(new CreateSceneGraphNodeCommand(geo, &CGL::mainScene, ckobj, geometry_data_offset, API));

	return geo;
}

CGL_Texture* CGL::CreateChuckObjFromTex(CK_DL_API API, Chuck_VM* VM, CGL_Texture* tex, Chuck_VM_Shred* SHRED, bool refcount)
{
	// create chuck obj
	Chuck_DL_Api::Type type = API->type->lookup(VM, tex->myCkName());
	Chuck_DL_Api::Object ckobj = API->object->create(SHRED, type, refcount);

	// tell renderer to create a copy
	CGL::PushCommand(new CreateSceneGraphNodeCommand(tex, &CGL::mainScene, ckobj, texture_data_offset, API));

	return tex;

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

void CGL::MeshSet( Mesh * mesh, Geometry * geo, Material * mat )
{
	// set on CGL side
	mesh->SetGeometry( geo );
	mesh->SetMaterial( mat );
	// command queue to update renderer side
	CGL::PushCommand( new SetMeshCommand( mesh ) );
}

//-----------------------------------------------------------------------------
// ChuGL Graphics Shred Bookkeeping
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
// ChuGL Auto Update (calls overriden update() on GGens)
//-----------------------------------------------------------------------------

// traverses chuck-side (audio thread) scenegraph and calls user-defined update() on GGens
void CGL::UpdateSceneGraph(Scene &scene, CK_DL_API API, Chuck_VM *VM, Chuck_VM_Shred * calling_shred)
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

        // must use shred associated with GGen
        auto it = s_GGen2OriginShred.find( ggen );
        // make sure ggen is in map
        assert( it != s_GGen2OriginShred.end() );
        // the shred
        Chuck_VM_Shred * origin_shred = (it->second);
        // make sure it's not null
        assert( origin_shred != NULL );

        // don't invoke update() for scene and camera
		if (ggen != nullptr && !obj->IsScene() && !obj->IsCamera())
		{
            // invoke the update function in immediate mode
			API->vm->invoke_mfun_immediate_mode(ggen, CGL::our_update_vt_offset, VM, origin_shred, &theArg, 1);
		}

		// add children to stack
		for (SceneGraphObject *child : obj->GetChildren())
		{
			queue.push(child);
		}
	}
}

//-----------------------------------------------------------------------------
// ChuGL Window State Setters / Getters (Render Thread --> Audio Thread communication)
//-----------------------------------------------------------------------------
CGL::WindowState::WindowState() :
// keep these dimensions in sync with default window size 
	windowWidth(Window::s_DefaultWindowWidth),
	windowHeight(Window::s_DefaultWindowHeight),  
// TODO: framebuffer dims not accurate on mac retina display, off by factor of 2 
	framebufferWidth(Window::s_DefaultWindowWidth),  
	framebufferHeight(Window::s_DefaultWindowHeight),  
	aspect(1.0f),
	mouseX(0), mouseY(0),
	mouseDX(0), mouseDY(0),
	mouseCapturedByImGUI(false),
	glfwTime(0), deltaTime(0), 
	fps(0) 
{}

std::pair<double, double> CGL::GetMousePos()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<double, double>(s_WindowState.mouseX, s_WindowState.mouseY);
}

std::pair<double, double> CGL::GetMouseDelta()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<double, double>(s_WindowState.mouseDX, s_WindowState.mouseDY);
}

bool CGL::GetMouseCapturedByImGUI()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return s_WindowState.mouseCapturedByImGUI;
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

t_CKFLOAT CGL::GetAspectRatio()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return (t_CKFLOAT)s_WindowState.windowWidth / (t_CKFLOAT)s_WindowState.windowHeight;
}

std::pair<double, double> CGL::GetTimeInfo()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	return std::pair<double, double>(s_WindowState.glfwTime, s_WindowState.deltaTime);
}

void CGL::SetMousePos(double x, double y)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);

	// mouse delta
	s_WindowState.mouseDX = x - s_WindowState.mouseX;
	s_WindowState.mouseDY = y - s_WindowState.mouseY;

	// mouse absolute
	s_WindowState.mouseX = x;
	s_WindowState.mouseY = y;
}

void CGL::ZeroMouseDeltas()
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.mouseDX = 0;
	s_WindowState.mouseDY = 0;
}

void CGL::SetMouseCapturedByImGUI(bool captured)
{
	std::unique_lock<std::mutex> lock(s_WindowStateLock);
	s_WindowState.mouseCapturedByImGUI = captured;
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
