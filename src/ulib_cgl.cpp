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

#include "renderer/scenegraph/Camera.h"
#include "renderer/scenegraph/Command.h"
#include "renderer/scenegraph/Scene.h"
#include "renderer/scenegraph/Light.h"

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



// exports =========================================

t_CKBOOL init_chugl_events(Chuck_DL_Query *QUERY);
t_CKBOOL init_chugl_static_fns(Chuck_DL_Query *QUERY);
t_CKBOOL create_chugl_default_objs(Chuck_DL_Query *QUERY);

// initialize data offsets
t_CKUINT CGL::geometry_data_offset = 0;
t_CKUINT CGL::material_data_offset = 0;
t_CKUINT CGL::texture_data_offset = 0;
t_CKUINT CGL::ggen_data_offset = 0;
static t_CKUINT cgl_next_frame_event_data_offset = 0;
static t_CKUINT window_resize_event_data_offset = 0;

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
	init_chugl_geometry(QUERY);
	init_chugl_texture(QUERY);
	init_chugl_material(QUERY);
	init_chugl_obj(QUERY);
	init_chugl_camera(QUERY);
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
        SceneGraphObject * cglObj = CGL::GetSGO(ggen);

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

CK_DLL_SFUN(cgl_mouse_hide) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_HIDDEN)); }
CK_DLL_SFUN(cgl_mouse_lock) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_LOCKED)); }
CK_DLL_SFUN(cgl_mouse_show) { CGL::PushCommand(new SetMouseModeCommand(CGL::MOUSE_NORMAL)); }

CK_DLL_SFUN(cgl_window_fullscreen) { CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_FULLSCREEN)); }

CK_DLL_SFUN(cgl_window_windowed)
{
	t_CKINT width = GET_NEXT_INT(ARGS);
	t_CKINT height = GET_NEXT_INT(ARGS);
	CGL::PushCommand(new SetWindowModeCommand(CGL::WINDOW_WINDOWED, width, height));
}

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
		CGL::PushCommand(new CreateSceneGraphNodeCommand(&mainCamera, &mainScene, camObj, CGL::GetGGenDataOffset()));
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
		CGL::PushCommand(new CreateSceneGraphNodeCommand(defaultLight, &CGL::mainScene, lightObj, CGL::GetGGenDataOffset()));
		// add to scene command
		CGL::PushCommand(new RelationshipCommand(&CGL::mainScene, defaultLight, RelationshipCommand::Relation::AddChild));
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
	CGL::PushCommand(new CreateSceneGraphNodeCommand(mat, &CGL::mainScene, ckobj, CGL::GetMaterialDataOffset()));

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

//-----------------------------------------------------------------------------
// ChuGL Window State Setters / Getters (Render Thread --> Audio Thread communication)
//-----------------------------------------------------------------------------
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
