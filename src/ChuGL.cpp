// clang-format off
#include "all.cpp"
#include "ulib_helper.h"

// ulibs
#include "ulib_color.cpp"
#include "ulib_box2d.cpp"
#include "ulib_component.cpp"
#include "ulib_camera.cpp"
#include "ulib_scene.cpp"
#include "ulib_geometry.cpp"
#include "ulib_imgui.cpp"
#include "ulib_material.cpp"
#include "ulib_texture.cpp"
#include "ulib_text.cpp"
#include "ulib_window.cpp"
#include "ulib_pass.cpp"
#include "ulib_buffer.cpp"
#include "ulib_light.cpp"
#include "ulib_assloader.cpp"

// vendor
#include <sokol/sokol_time.h>
// clang-format on

// ChuGL version string
#define CHUGL_VERSION_STRING "0.1.5 (alpha)"

static Chuck_DL_MainThreadHook* hook = NULL;
static bool hookActivated            = false;

// metadata required for scene rendering
struct GG_Config {
    SG_ID mainScene;
    SG_ID mainCamera;
    SG_ID root_pass_id;
    SG_ID default_render_pass_id;
    SG_ID default_output_pass_id;

    // options
    bool auto_update_scenegraph = true;
};

GG_Config gg_config = {};

static f64 ckdt_sec      = 0;
static f64 system_dt_sec = 0;

t_CKBOOL chugl_main_loop_hook(void* bindle)
{
    UNUSED_VAR(bindle);

    ASSERT(g_chuglAPI && g_chuglVM);

    App app = {};

    App::init(&app, g_chuglVM, g_chuglAPI);
    App::start(&app); // blocking

    { // cleanup (after exiting main loop)
        // remove all shreds (should trigger shutdown, unless running in --loop
        // mode)
        if (g_chuglVM && g_chuglAPI) g_chuglAPI->vm->remove_all_shreds(g_chuglVM);

        App::end(&app);
    }

    return true;
}

t_CKBOOL chugl_main_loop_quit(void* bindle)
{
    UNUSED_VAR(bindle);
    SG_Free();
    CQ_Free();
    return true;
}

// ChuGL chugin info func
CK_DLL_INFO(ChuGL)
{
    // set module name
    QUERY->setname(QUERY, "ChuGL");

    // set info
    QUERY->setinfo(QUERY, CHUGIN_INFO_AUTHORS, "Andrew Zhu Aday & Ge Wang");
    QUERY->setinfo(QUERY, CHUGIN_INFO_CHUGIN_VERSION, CHUGL_VERSION_STRING);
    QUERY->setinfo(
      QUERY, CHUGIN_INFO_DESCRIPTION,
      "ChuGL (ChucK Graphics Library) is a unified audiovisual programming "
      "framework built into the ChucK programming language.");
    QUERY->setinfo(QUERY, CHUGIN_INFO_URL, "https://chuck.stanford.edu/chugl/");
    QUERY->setinfo(QUERY, CHUGIN_INFO_EMAIL, "azaday@ccrma.stanford.edu");

    { // setup
        // initialize performance counters
        stm_setup();
    }
}

// ============================================================================
// Query
// ============================================================================

static t_CKUINT chugl_next_frame_event_data_offset = 0;

static void autoUpdateScenegraph(Arena* arena, SG_Scene* scene, Chuck_VM* VM,
                                 CK_DL_API API, t_CKINT _ggen_update_vt_offset)
{
    // only update once per frame
    if (scene->last_auto_update_frame == g_frame_count) return;

    // mark updated this frame
    scene->last_auto_update_frame = g_frame_count;

    Chuck_DL_Arg theArg;
    theArg.kind          = kindof_FLOAT;
    theArg.value.v_float = g_last_dt;

    void* arena_orig_top             = Arena::top(arena);
    *(ARENA_PUSH_TYPE(arena, SG_ID)) = scene->id;

    // BFS through graph
    // TODO: can just walk linearly through entity arenas instead?
    // but then how do we know if something is part of the active scene graph?
    while (Arena::top(arena) != arena_orig_top) {
        ARENA_POP_TYPE(arena, SG_ID);
        SG_ID sg_id = *(SG_ID*)Arena::top(arena);

        SG_Transform* xform = SG_GetTransform(sg_id);
        ASSERT(xform != NULL);

        Chuck_Object* ggen = xform->ckobj;
        ASSERT(ggen != NULL);

        Chuck_VM_Shred* origin_shred = chugin_getOriginShred(ggen);
        API->vm->invoke_mfun_immediate_mode(ggen, _ggen_update_vt_offset, VM,
                                            origin_shred, &theArg, 1);

        // add children to stack
        size_t numChildren  = SG_Transform::numChildren(xform);
        SG_ID* children_ptr = ARENA_PUSH_COUNT(arena, SG_ID, numChildren);
        memcpy(children_ptr, xform->childrenIDs.base, sizeof(SG_ID) * numChildren);
    }
}

CK_DLL_SFUN(chugl_next_frame)
{
    // extract CglEvent from obj
    // TODO: workaround bug where create() object API is not calling
    // preconstructors
    // https://trello.com/c/JwhVQEpv/48-cglnextframe-now-not-calling-preconstructor-of-cglupdate

    if (!Sync_HasShredWaited(SHRED))
        API->vm->throw_exception(
          "NextFrameNotWaitedOnViolation",
          "You are calling .nextFrame() without chucking to now!\n"
          "Please replace this line with .nextFrame() => now;",
          SHRED);

    // RETURN->v_object = (Chuck_Object *)CGL::GetShredUpdateEvent(SHRED, API,
    // VM)->GetEvent();
    RETURN->v_object = (Chuck_Object*)Event_Get(CHUGL_EventType::NEXT_FRAME, API, VM);

    // register shred and set has-waited flag to false
    Sync_RegisterShred(SHRED);

    // bugfix: grabbing this lock prevents race with render thread
    // broadcasting nextFrameEvent and setting waitingShreds to 0.
    // Unlocked in event_next_frame_waiting_on after shred has
    // been added to the nextFrameEvent waiting queue.
    // Render thread holds this lock when broadcasting + setting waitingShreds
    // to 0
    spinlock::lock(&waitingShredsLock);
}

//-----------------------------------------------------------------------------
// this is called by chuck VM at the earliest point when a shred begins to wait
// on an Event used to catch GG.nextFrame() => now; on one or more shreds once
// all expected shreds are waiting on GG.nextFrame(), this function signals the
// graphics-side
//-----------------------------------------------------------------------------
CK_DLL_MFUN(event_next_frame_waiting_on)
{
    // see comment in chugl_next_frame
    ++waitingShreds;
    bool allShredsWaiting = waitingShreds == registeredShreds.size();

    // when new shreds are sporked, they will have `allShredsWaiting = true`
    // so to prevent multiple updates, we also check against the last update frame
    // and only update if this is the first time this frame
    static i64 last_update_frame_count{ -1 };
    bool first_of_last_shreds_waited
      = last_update_frame_count != waiting_shreds_frame_count;
    if (allShredsWaiting) {
        last_update_frame_count = waiting_shreds_frame_count;
    }

    spinlock::unlock(&waitingShredsLock);

    ASSERT(registeredShreds.find(SHRED) != registeredShreds.end());
    registeredShreds[SHRED] = true; // mark shred waited

    // THIS IS A VERY IMPORTANT FUNCTION. See
    // https://trello.com/c/Gddnu21j/6-chuglrender-refactor-possible-deadlock-between-cglupdate-and-render
    // and
    // https://github.com/ccrma/chugl/blob/2023-chugl-int/design/multishred-render-1.ck
    // for further context

    // activate hook only on GG.nextFrame();
    if (!hookActivated) {
        hookActivated = true;
        hook->activate(hook);
    }

    if (allShredsWaiting && first_of_last_shreds_waited) {
        // if #waiting == #registered, all chugl shreds have finished work, and
        // we are safe to wakeup the renderer
        // TODO: bug. If a shred does NOT call GG.nextFrame in an infinite loop,
        // i.e. does nextFrame() once and then goes on to say process audio,
        // this code will stay be expecting that shred to call nextFrame() again
        // and waitingShreds will never == registeredShreds.size() thus hanging
        // the renderer

        { // process dt
            // process dt in audio samples
            static t_CKTIME chuglLastUpdateTime{ API->vm->now(VM) };

            t_CKTIME chuckTimeNow  = API->vm->now(VM);
            t_CKTIME chuckTimeDiff = chuckTimeNow - chuglLastUpdateTime;
            ckdt_sec               = chuckTimeDiff / API->vm->srate(VM);
            chuglLastUpdateTime    = chuckTimeNow;

            // process dt with OS-provided timer
            static u64 system_last_time{ stm_now() };
            u64 system_dt_ticks = stm_laptime(&system_last_time);
            system_dt_sec       = stm_sec(system_dt_ticks);

            // update render thread dt
            g_last_dt = CHUGL_Window_dt();
            g_frame_count++;

#ifdef CHUGL_DEBUG
            spinlock::lock(&waitingShredsLock);
            ASSERT(g_frame_count - 1 == waiting_shreds_frame_count);
            spinlock::unlock(&waitingShredsLock);
#endif
        }

        // traverse rendegraph chuck-defined update() on all render passes
        if (gg_config.auto_update_scenegraph) {
            SG_Pass* pass = SG_GetPass(gg_config.root_pass_id);
            while (pass) {
                if (pass->pass_type == SG_PassType_Render) {
                    SG_Scene* scene = SG_GetScene(pass->scene_id);
                    ASSERT(scene != NULL);
                    autoUpdateScenegraph(&audio_frame_arena, scene, g_chuglVM,
                                         g_chuglAPI, ggen_update_vt_offset);
                }
                pass = SG_GetPass(pass->next_pass_id);
            }
        }

        // Garbage collect (TODO add API function to control this via GG
        // config) SG_GC();

        // signal the graphics-side that audio-side is done processing for
        // this frame
        Sync_SignalUpdateDone();

        // clear audio frame arena
        Arena::clear(&audio_frame_arena);
    }
}

CK_DLL_SFUN(chugl_gc)
{
    SG_GC();
}

CK_DLL_SFUN(chugl_get_scene)
{
    RETURN->v_object = SG_GetScene(gg_config.mainScene)->ckobj;
}

CK_DLL_SFUN(chugl_set_scene)
{
    SG_ID prev_scene_id = gg_config.mainScene;

    // get new scene
    Chuck_Object* newScene = GET_NEXT_OBJECT(ARGS);
    SG_Scene* sg_scene = SG_GetScene(OBJ_MEMBER_UINT(newScene, component_offset_id));

    // bump refcount on new scene
    SG_AddRef(sg_scene);

    // assign new scene
    gg_config.mainScene = sg_scene ? sg_scene->id : 0;
    CQ_PushCommand_GG_Scene(sg_scene);

    // decrement refcount on old scene
    SG_DecrementRef(prev_scene_id);
}

CK_DLL_SFUN(chugl_get_fps)
{
    RETURN->v_float = CHUGL_Window_fps();
    return;
}

CK_DLL_SFUN(chugl_get_dt)
{
    RETURN->v_float = g_last_dt;
}

CK_DLL_SFUN(chugl_get_frame_count)
{
    RETURN->v_uint = g_frame_count;
}

CK_DLL_SFUN(chugl_get_root_pass)
{
    RETURN->v_object = SG_GetPass(gg_config.root_pass_id)->ckobj;
}

CK_DLL_SFUN(chugl_get_default_render_pass)
{
    RETURN->v_object = SG_GetPass(gg_config.default_render_pass_id)->ckobj;
}

CK_DLL_SFUN(chugl_get_default_output_pass)
{
    RETURN->v_object = SG_GetPass(gg_config.default_output_pass_id)->ckobj;
}

CK_DLL_SFUN(chugl_get_auto_update_scenegraph)
{
    RETURN->v_int = gg_config.auto_update_scenegraph ? 1 : 0;
}

CK_DLL_SFUN(chugl_set_auto_update_scenegraph)
{
    gg_config.auto_update_scenegraph = (GET_NEXT_INT(ARGS) != 0);
}

CK_DLL_SFUN(chugl_get_default_camera)
{
    RETURN->v_object = SG_GetCamera(gg_config.mainCamera)->ckobj;
}

CK_DLL_SFUN(chugl_set_log_level)
{
    log_set_level(GET_NEXT_INT(ARGS));
}

// ============================================================================
// Chugin entry point
// ============================================================================
CK_DLL_QUERY(ChuGL)
{
    // set log level
#ifdef CHUGL_RELEASE
    log_set_level(LOG_ERROR); // only log errors and fatal in release mode
#endif

    // remember
    g_chuglVM  = QUERY->ck_vm(QUERY);
    g_chuglAPI = QUERY->ck_api(QUERY);

    // audio frame arena
    Arena::init(&audio_frame_arena, 64 * KILOBYTE);

    // initialize component pool
    // TODO: have a single ChuGL_Context that manages this all
    SG_Init(g_chuglAPI);
    CQ_Init();

    // set up for main thread hook, for running ChuGL on the main thread
    hook = QUERY->create_main_thread_hook(QUERY, chugl_main_loop_hook,
                                          chugl_main_loop_quit, NULL);

    // TODO: add shred_on_destroy listener

    // initialize ChuGL API ========================================
    { // chugl events
        // triggered by main render thread after deepcopy is complete, and safe
        // for chuck to begin updating the scene graph (intentionally left out
        // of CKDocs)
        QUERY->begin_class(QUERY, "NextFrameEvent", "Event");
        DOC_CLASS(
          "Don't instantiate this class directly. Use GG.nextFrame() => now; "
          "instead.");
        // no destructor for singleton
        chugl_next_frame_event_data_offset
          = QUERY->add_mvar(QUERY, "int", "@next_frame_event_data", false);

        QUERY->add_mfun(QUERY, event_next_frame_waiting_on, "void", "waiting_on");
        QUERY->end_class(QUERY);
    }

    { // create default ckobjs
        g_builtin_ckobjs.empty_float_array = chugin_createCkFloatArray(NULL, 0, true);

        float init_2d_pos[2] = { 0.0f, 0.0f };
        g_builtin_ckobjs.init_2d_pos
          = chugin_createCkFloatArray(init_2d_pos, ARRAY_LENGTH(init_2d_pos), true);

        glm::vec3 white_color = glm::vec3(1.0f);
        g_builtin_ckobjs.init_white_color
          = chugin_createCkFloat3Array(&white_color, 1, true);

        float white_pixel_data[4]         = { 1.0f, 1.0f, 1.0f, 1.0f };
        g_builtin_ckobjs.white_pixel_data = chugin_createCkFloatArray(
          white_pixel_data, ARRAY_LENGTH(white_pixel_data), true);

        float black_pixel_data[4]         = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_builtin_ckobjs.black_pixel_data = chugin_createCkFloatArray(
          black_pixel_data, ARRAY_LENGTH(black_pixel_data), true);

        float normal_pixel_data[4]         = { 0.5f, 0.5f, 1.0f, 1.0f };
        g_builtin_ckobjs.normal_pixel_data = chugin_createCkFloatArray(
          normal_pixel_data, ARRAY_LENGTH(normal_pixel_data), true);
    }

    ulib_color_query(QUERY);
    ulib_box2d_query(QUERY);

    ulib_window_query(QUERY);
    ulib_component_query(QUERY);
    ulib_texture_query(QUERY);
    ulib_ggen_query(QUERY);
    ulib_light_query(QUERY);
    ulib_imgui_query(QUERY);
    ulib_camera_query(QUERY);
    ulib_gscene_query(QUERY);
    ulib_buffer_query(QUERY);
    ulib_geometry_query(QUERY);
    ulib_material_query(QUERY);
    ulib_mesh_query(QUERY);
    ulib_pass_query(QUERY);
    ulib_text_query(QUERY);
    ulib_assloader_query(QUERY);

    { // GG static functions
        QUERY->begin_class(QUERY, "GG", "Object");

        // svars
        static t_CKUINT gg_log_level_trace = LOG_TRACE;
        static t_CKUINT gg_log_level_debug = LOG_DEBUG;
        static t_CKUINT gg_log_level_info  = LOG_INFO;
        static t_CKUINT gg_log_level_warn  = LOG_WARN;
        static t_CKUINT gg_log_level_error = LOG_ERROR;
        static t_CKUINT gg_log_level_fatal = LOG_FATAL;
        SVAR("int", "LogLevel_Trace", &gg_log_level_trace);
        SVAR("int", "LogLevel_Debug", &gg_log_level_debug);
        SVAR("int", "LogLevel_Info", &gg_log_level_info);
        SVAR("int", "LogLevel_Warn", &gg_log_level_warn);
        SVAR("int", "LogLevel_Error", &gg_log_level_error);
        SVAR("int", "LogLevel_Fatal", &gg_log_level_fatal);

        SFUN(chugl_set_log_level, "void", "logLevel");
        ARG("int", "level");
        DOC_FUNC(
          "Set the log level for ChuGL's renderer and graphics thread. "
          "Levels are: GG.LogLevel_Trace, GG.LogLevel_Debug, GG.LogLevel_Info, "
          "GG.LogLevel_Warn, GG.LogLevel_Error, GG.LogLevel_Fatal. "
          "Setting a log level will allow all messages of that level and higher "
          "to be printed to the console. Default is GG.LogLevel_Error.");

        QUERY->add_sfun(QUERY, chugl_next_frame, "NextFrameEvent", "nextFrame");
        QUERY->doc_func(
          QUERY,
          "Registers the calling shred to be notified when the next frame is "
          "finished rendering. When all graphics shreds are finished calling."
          "Note: this function returns an event that MUST be waited on for "
          "correct behavior, i.e. GG.nextFrame() => now;"
          "See the ChuGL tutorial and examples for more information.");

        QUERY->add_sfun(QUERY, chugl_get_scene, SG_CKNames[SG_COMPONENT_SCENE],
                        "scene");
        QUERY->add_sfun(QUERY, chugl_set_scene, SG_CKNames[SG_COMPONENT_SCENE],
                        "scene");
        QUERY->add_arg(QUERY, SG_CKNames[SG_COMPONENT_SCENE], "scene");

        // QUERY->add_sfun(QUERY, chugl_gc, "void", "gc");
        // QUERY->doc_func(QUERY, "Trigger garbage collection");

        SFUN(chugl_get_fps, "float", "fps");
        DOC_FUNC("FPS of current window, updated every second");

        // note: we use window time here instead of ckdt_sec or system_dt_sec
        // (which are laptimes calculated on the audio thread every cycle of
        // nextFrameWaitingOn ) because the latter values are highly unstable, and do
        // not correspond to the actual average FPS of the graphics window. dt is most
        // likely used for graphical animations, and so therefore should be set by the
        // actual render thread, not the audio thread.
        SFUN(chugl_get_dt, "float", "dt");
        DOC_FUNC("return the laptime of the graphics thread's last frame in seconds");

        SFUN(chugl_get_frame_count, "int", "fc");
        DOC_FUNC("return the number of frames rendered since the start of the program");

        SFUN(chugl_get_root_pass, SG_CKNames[SG_COMPONENT_PASS], "rootPass");
        DOC_FUNC("Get the root pass of the current scene");

        SFUN(chugl_get_default_render_pass, "RenderPass", "renderPass");
        DOC_FUNC("Get the default render pass (renders the main scene");

        SFUN(chugl_get_default_output_pass, "OutputPass", "outputPass");
        DOC_FUNC(
          "Get the default output pass (renders the main scene to the screen, "
          "with default tonemapping and exposure settings");

        SFUN(chugl_get_auto_update_scenegraph, "int", "autoUpdate");
        DOC_FUNC(
          "Returns true if GGen update() functions are automatically called "
          "on all GGens in each active scene graph every frame. Default is true.");

        SFUN(chugl_set_auto_update_scenegraph, "void", "autoUpdate");
        ARG("int", "autoUpdate");
        DOC_FUNC(
          "Set whether GGen update() functions are automatically called "
          "on all GGens in active scene graphs every frame. Default is true.");

        SFUN(gwindow_fullscreen, "void", "fullscreen");
        DOC_FUNC(
          "Shorthand for GWindow.fullscreen(). Added for backwards compatibility");

        SFUN(chugl_get_default_camera, "GOrbitCamera", "camera");
        DOC_FUNC(
          "Shorthand for getting the default GOrbitCamera that is created upon "
          "startup");

        END_CLASS();
    } // GG

    { // Default components
        // scene
        Chuck_Object* scene_ckobj
          = chugin_createCkObj(SG_CKNames[SG_COMPONENT_SCENE], true);
        SG_Scene* scene     = ulib_scene_create(scene_ckobj);
        gg_config.mainScene = scene->id;
        CQ_PushCommand_GG_Scene(scene);

        // default directional light
        Chuck_Object* dir_light_ckobj
          = chugin_createCkObj(SG_CKNames[SG_COMPONENT_LIGHT], true);
        SG_Light* dir_light
          = ulib_light_create(dir_light_ckobj, SG_LightType_Directional);
        CQ_PushCommand_AddChild(scene, dir_light);
        // angle light down slightly
        SG_Transform::lookAt(dir_light, glm::vec3(0.0f, -1.0f, -1.0f));
        CQ_PushCommand_SetRotation(dir_light);

        // default orbit camera
        SG_Camera* default_camera
          = ulib_camera_create(chugin_createCkObj("GOrbitCamera", false));
        CQ_PushCommand_AddChild(scene, default_camera);
        SG_Scene::setMainCamera(scene, default_camera);
        CQ_PushCommand_SceneUpdate(scene);
        gg_config.mainCamera = default_camera->id;

        // passRoot()
        gg_config.root_pass_id = ulib_pass_createPass(SG_PassType_Root);
        SG_Pass* root_pass     = SG_GetPass(gg_config.root_pass_id);

        // renderPass for main scene
        gg_config.default_render_pass_id = ulib_pass_createPass(SG_PassType_Render);
        SG_Pass* render_pass             = SG_GetPass(gg_config.default_render_pass_id);
        render_pass->scene_id            = gg_config.mainScene;
        CQ_PushCommand_PassUpdate(render_pass);

        // connect root to renderPass
        SG_Pass::connect(root_pass, render_pass);

        // set default render texture as output of render pass
        SG_Texture* render_texture
          = SG_GetTexture(g_builtin_textures.default_render_texture_id);
        SG_Pass::resolveTarget(render_pass, render_texture);

        // output pass
        Chuck_Object* output_pass_ckobj = chugin_createCkObj("OutputPass", true);
        SG_Pass* output_pass            = ulib_pass_createOutputPass(output_pass_ckobj);
        gg_config.default_output_pass_id = output_pass->id;

        // connect renderPass to outputPass
        SG_Pass::connect(render_pass, output_pass);

        // set render texture as input to output pass
        SG_Material* material = SG_GetMaterial(output_pass->screen_material_id);
        SG_Material::setTexture(material, 0, render_texture);
        CQ_PushCommand_MaterialSetUniform(material, 0);

        // update all passes over cq
        CQ_PushCommand_PassUpdate(root_pass);
        CQ_PushCommand_PassUpdate(render_pass);
        CQ_PushCommand_PassUpdate(output_pass);
    }

    // wasn't that a breeze?
    return true;
}
