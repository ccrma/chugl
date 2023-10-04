#include "window.h"

// CGL includes
#include "renderer/Includes.h"
#include "renderer/Renderer.h"
#include "renderer/Util.h"

#include "renderer/scenegraph/SceneGraphObject.h"
#include "renderer/scenegraph/Camera.h"
#include "renderer/scenegraph/Scene.h"

// chuck CORE includes
#include "ulib_cgl.h" // TODO: need to expose graphics entry point in chuck.h

// system includes
#include <iostream>
#include <stdexcept>
#include <condition_variable>

// glm includes
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


/* =============================================================================
						    * Static init *	
===============================================================================*/
Renderer Window::renderer;
Camera Window::camera;
Scene Window::scene;


/* =============================================================================
						    * GLFW callbacks *	
===============================================================================*/

// window size in pixels
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // TODO: figure out how to get access to active window object
    // Window::GetInstance().SetViewSize(width, height);
    glViewport(
        0, 0,  // index of lower left corner of viewport, in pixels
        width, height
    );
    // update window state
    Window* w = Window::GetWindow(window);
    w->SetViewSize(width, height);
    CGL::SetFramebufferSize(width, height);

    // auto-update camera aspect
    if (height > 0) {  // don't change aspect if screen minimizes, otherwises causes div by 0 crash
        Window::camera.params.aspect = (float)width / (float)height;
    }

    CglEvent::Broadcast(CglEventType::CGL_WINDOW_RESIZE);  // doesn't matter if we brodcast this in frambuffer callback or window size callback
}

// window sizs in screen-space coordinates
static void window_size_callback(GLFWwindow* window, int width, int height)
{
    CGL::SetWindowSize(width, height);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    CGL::SetMousePos(xpos, ypos);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}


static void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error[%i]: %s\n", error, description);
}




/* =============================================================================
						    * Window *	
===============================================================================*/

// static initializers
std::unordered_map<GLFWwindow*, Window*> Window::s_WindowMap;


void Window::SetViewSize(int width, int height)
{
    m_ViewWidth = width;
    m_ViewHeight = height;
}

void Window::UpdateState()
{
    // mouse mode
    if (Scene::updateMouseMode) {
        if (Scene::mouseMode == CGL::MOUSE_NORMAL) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (Scene::mouseMode == CGL::MOUSE_HIDDEN) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else if (Scene::mouseMode == CGL::MOUSE_LOCKED) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (Scene::updateWindowMode) {
        if (Scene::windowMode == CGL::WINDOW_WINDOWED) {
            // set window (pos at 100,100 so title bar appears....silly)
            glfwSetWindowMonitor(m_Window, NULL, 100, 100, Scene::windowedWidth, Scene::windowedHeight, GLFW_DONT_CARE);
            //re-enable decorations
            glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        }
        else if (Scene::windowMode == CGL::WINDOW_FULLSCREEN) {
            // set to primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else if (Scene::windowMode == CGL::WINDOW_MAXIMIZED) {
            // maximize
            glfwMaximizeWindow(m_Window);
        } else if (Scene::windowMode == CGL::WINDOW_RESTORE) {
            // restore
            glfwRestoreWindow(m_Window);
        } else if (Scene::windowMode == CGL::WINDOW_SET_SIZE) {
            // if in windowed mode, sets size in screen coordinates
            // if in full screen, changes resolution 
            glfwSetWindowSize(m_Window, Scene::windowedWidth, Scene::windowedHeight);
        }
    }
}

Window::Window(int viewWidth, int viewHeight) : m_ViewWidth(viewWidth), m_ViewHeight(viewHeight), m_DeltaTime(0.0f)
{
    // Set error callback ======================================
    glfwSetErrorCallback(glfwErrorCallback);

    // init and select openGL version ==========================
    glfwInit();
    #ifdef __APPLE__
    /* We need to explicitly ask for a 3.2 context on OS X */
    glfwWindowHint (GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint (GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint (GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint (GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #endif

    // window hints =============================================
    glfwWindowHint(GLFW_SAMPLES, 4);  // MSAA

    
    // create window ============================================
    m_Window = glfwCreateWindow(m_ViewWidth, m_ViewHeight, "ChuGL", NULL, NULL);
    if (m_Window == NULL)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(m_Window);
    SetWindow(m_Window, this);  // add to static map
    CGL::SetWindowSize(m_ViewWidth, m_ViewHeight);



    // VSYNC =================================================
    // glfwSwapInterval(0);  // TODO: for now disabling vsync introduces jitter
    glfwSwapInterval(1);  

    // Initialize GLAD =======================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Print Context Info =================================================
    std::cerr << "====================GLFW Info========================" << std::endl;
    std::cerr << "GLFW Version: " << glfwGetVersionString() << std::endl;
    std::cerr << "====================OpenGL Context Info========================" << std::endl;
    std::cerr << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cerr << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cerr << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cerr << "OpenGL Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cerr << "===============================================================" << std::endl;

    // OpenGL Viewport and Callbacks =========================
    // for high-DPI displays, framebuffer size is actually a multiple of window size
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(m_Window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);

    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glfwSetKeyCallback(m_Window, keyCallback);

    // mouse settings =====================================
    glfwSetCursorPosCallback(m_Window, cursor_position_callback);


    // OpenGL Metadata =======================================
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum number of 4-component vertex attributes: " << nrAttributes << std::endl;
    
    // GLEnables ===============================================
    // Blending (TODO) should this be part of renderer? 
    GLCall(glEnable(GL_BLEND));
    GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    // depth testing
    GLCall(glEnable(GL_DEPTH_TEST));

    // MSAA
    GLCall(glEnable(GL_MULTISAMPLE));

    // Line and point smoothing
    GLCall(glEnable(GL_LINE_SMOOTH));
    // GLCall(glEnable(GL_POINT_SMOOTH));  // not defined by glad

    // point size
    GLCall(glEnable(GL_PROGRAM_POINT_SIZE));


}

Window::~Window()
{   
    std::cout << "calling window destructor, terminating glfw window" << std::endl;
    glfwDestroyWindow(m_Window);
	glfwTerminate();
}


void Window::DisplayLoop()
{
    // Scene setup ==========================================
    camera.params.aspect = (float(m_ViewWidth) / (float)m_ViewHeight);
    camera.SetPosition(0.0f, 0.0f, 3.0f);


    // Copy from CGL scenegraph ====================================    
    scene.SetID(CGL::mainScene.GetID());  // copy scene ID
    scene.RegisterNode(&scene);  // register itself
    camera.SetID(CGL::mainCamera.GetID());  // copy maincam ID
    scene.RegisterNode(&camera);  // register camera

    // Render Loop ===========================================
    float previousFPSTime = (float)glfwGetTime();
    float prevTickTime = previousFPSTime;
    int frameCount = 0;
    while (!glfwWindowShouldClose(m_Window))
    {
        CGL::WaitOnUpdateDone();  
        // TODO: why does putting this AFTER time calculation cause everything to be so choppy at high FPS?
        // hypothesis: puts time calculation into the critical region
        // time is updated only when all chuck shreds are on wait queue
        // guaranteeing that when they awake, they'll be using fresh dt data

        // FPS counter
		++frameCount;
        float currentTime = (float)glfwGetTime();
        // deltaTime
        m_DeltaTime = currentTime - prevTickTime;
        prevTickTime = currentTime;
        if (currentTime - previousFPSTime >= 1.0f) {
            std::cerr << "FPS: " << frameCount << std::endl;
            // Util::println(std::to_string(frameCount));
            frameCount = 0;
            previousFPSTime = currentTime;
        }
        CGL::SetTimeInfo(currentTime, m_DeltaTime);

        /*
        Note: this sync mechanism also gets rid of the problem where chuck runs away
        e.g. if the time it takes the renderer flush the queue is greater than
        the time it takes chuck to write, ie write rate > flush rate,
        each command queue will get longer and longer, continually worsening performance
        shouldn't happen because chuck runs in vm and the flushing happens natively but 
        you never know
        */
        /*
        two locks here:
        1 for writing/swapping the command queues
            - this lock is grabbed by chuck every time we do a CGL call
            - supports writing CGL commands whenever, even outside game loop
        1 for the condition_var used to synchronize audio and graphics each frame
            - combined with the chuck-side update_event, allows for writing frame-accurate cgl commands
            - exposes a gameloop to chuck, gauranteed to be executed once per frame

        deadlock shouldn't happen because both locks are never held at the same time
        */
        { // critical section: swap command queus
            CGL::SwapCommandQueues();
        }

        // done swapping the double buffer, let chuck know it's good to continue pushing commands
        // TODO: does this need to be lock protected?
        // scenario: another shred is spawned which is NOT blocked and initializes a new shred, writing to event queue  
        // at the same time render thread is reading from event queue and broadcasting
        // temp workaround: create all your shreds and Chugl events BEFORE calling first .Render()
        CglEvent::Broadcast(CglEventType::CGL_UPDATE);

        // now apply changes from the command queue chuck is NO Longer writing to 
        CGL::FlushCommandQueue(scene, false);
        
        // process any glfw options passed from chuck
        UpdateState();

        // now renderer can work on drawing the copied scenegraph 
        renderer.Clear(scene.GetBackgroundColor());
        renderer.RenderScene(&scene, &camera);

        // Handle Events, Draw framebuffer
        glfwPollEvents();
        //glfwWaitEvents();  // blocking version of PollEvents. How does blocking work? "puts the calling thread to sleep until at least one event is available in the event queue" https://www.glfw.org/docs/latest/group__window.html#ga15a5a1ee5b3c2ca6b15ca209a12efd14
        glfwSwapBuffers(m_Window);  // blocks until glfwSwapInterval screen updates have occured
    }
}

