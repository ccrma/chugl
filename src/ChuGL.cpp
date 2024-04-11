//-----------------------------------------------------------------------------
// Entaro ChucK Developer!
// This is a Chugin boilerplate, generated by chuginate!
//-----------------------------------------------------------------------------

// include pch
// #include "chugl_pch.h"

// vendor includes
#include <GLFW/glfw3.h>
#include <glfw3webgpu/glfw3webgpu.h>

#include "core/log.h"
#include "ulib_cgl.h"

// #include "window.h"

// ============================================================================
// Window
// ============================================================================

#define CHUGL_WINDOW_DEFAULT_WIDTH 640
#define CHUGL_WINDOW_DEFAULT_HEIGHT 480

struct CHUGL_Window {
};

// ============================================================================
// Chugin Callbacks
// ============================================================================

// references to VM and API
static Chuck_VM* g_chuglVM  = NULL;
static CK_DL_API g_chuglAPI = NULL;

t_CKBOOL chugl_main_loop_hook(void* bindle)
{
    // std::cerr << "INSIDE chugl main loop hook!" << std::endl;
    // Window window;
    // window.DisplayLoop();

    // main loop
    GLFWWindow* window = NULL;
    { // Initialize GLFW and window
        glfwInit();
        if (!glfwInit()) {
            log_fatal("Failed to initialize GLFW\n");
            return false;
        }

        // Create the window without an OpenGL context
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(CHUGL_WINDOW_DEFAULT_WIDTH,
                                  CHUGL_WINDOW_DEFAULT_HEIGHT,
                                  "WebGPU-Renderer", NULL, NULL);

        if (!window) {
            log_fatal("Failed to create GLFW window\n");
            glfwTerminate();
            return false;
        }
    }

    // { // init graphics context
    //     if (!GraphicsContext::init(&runner->gctx, runner->window)) {
    //         log_fatal("Failed to initialize graphics context\n");
    //         return false;
    //     }
    // }

    // { // set window callbacks
    //     // Set the user pointer to be "this"
    //     glfwSetWindowUserPointer(runner->window, runner);
    //     glfwSetMouseButtonCallback(runner->window, mouseButtonCallback);
    //     glfwSetScrollCallback(runner->window, scrollCallback);
    //     glfwSetCursorPosCallback(runner->window, cursorPositionCallback);
    //     glfwSetKeyCallback(runner->window, keyCallback);
    //     glfwSetFramebufferSizeCallback(runner->window, onWindowResize);
    // }

    return true;

    // std::cerr << "==exiting chugl window==" << std::endl;
    CGL::DeactivateHook();

    // remove all shreds (should trigger shutdown, unless running in --loop
    // mode)
    if (g_chuglVM && g_chuglAPI) g_chuglAPI->vm->remove_all_shreds(g_chuglVM);

    return TRUE;
}

t_CKBOOL chugl_main_loop_quit(void* bindle)
{
    // std::cerr << "LEAVING chugl main loop hook" << std::endl;
    // window.Terminate();
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
}

// ChuGL chugin entry point
CK_DLL_QUERY(ChuGL)
{
    // set up for main thread hook, for running ChuGL on the main thread
    CGL::hook = QUERY->create_main_thread_hook(QUERY, chugl_main_loop_hook,
                                               chugl_main_loop_quit, NULL);

    // initialize ChuGL API
    init_chugl(QUERY);

    // remember
    g_chuglVM  = QUERY->ck_vm(QUERY);
    g_chuglAPI = QUERY->ck_api(QUERY);

    // wasn't that a breeze?
    return TRUE;
}
