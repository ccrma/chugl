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
#include "ulib_gui.h"

// system includes
#include <iostream>
#include <stdexcept>
#include <condition_variable>

// glm includes
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// imgui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


/* =============================================================================
						    * Static init *	
===============================================================================*/
Renderer Window::renderer;
Scene Window::scene;


/* =============================================================================
						    * GLFW callbacks *	
===============================================================================*/

// window size in pixels
static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(
        0, 0,  // index of lower left corner of viewport, in pixels
        width, height
    );

    // update framebuffer texture size
    Window::renderer.UpdateFramebufferSize(width, height);

    // update window state
    Window* w = Window::GetWindow(window);
    w->SetViewSize(width, height);
    CGL::SetFramebufferSize(width, height);

    // auto-update camera aspect
    if (height > 0) {  // don't change aspect if screen minimizes, otherwises causes div by 0 crash
        if (Window::scene.GetMainCamera()) {
            Window::scene.GetMainCamera()->params.aspect = (float)width / (float)height;
        }
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

    // window close command
    if (Scene::windowShouldClose) {
        glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
        Scene::windowShouldClose = false;  // reset flag
    }

}

Window::Window(int viewWidth, int viewHeight) : m_ViewWidth(viewWidth), m_ViewHeight(viewHeight), m_DeltaTime(0.0f)
{
    // Set error callback ======================================
    glfwSetErrorCallback(glfwErrorCallback);

    // init and select openGL version ==========================
    const char * glsl_version = "#version 330";
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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
    glfwPollEvents();  // call poll events first to get correct framebuffer size (glfw bug: https://github.com/glfw/glfw/issues/1968)
    int frameBufferWidth, frameBufferHeight;
    glfwGetFramebufferSize(m_Window, &frameBufferWidth, &frameBufferHeight);
    glViewport(0, 0, frameBufferWidth, frameBufferHeight);

    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    // call the callback to first initialize 
    framebuffer_size_callback(m_Window, frameBufferWidth, frameBufferHeight);

    glfwSetKeyCallback(m_Window, keyCallback);

    // mouse settings =====================================
    glfwSetCursorPosCallback(m_Window, cursor_position_callback);


    // OpenGL Metadata =======================================
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    // std::cout << "Maximum number of 4-component vertex attributes: " << nrAttributes << std::endl;
    
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
    
    // face culling
    // GLCall(glEnable(GL_CULL_FACE));
    // GLCall(glCullFace(GL_BACK));

    // imgui setup =======================================
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; 
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // - Our Emscripten build process allows embedding fonts to be accessible at runtime from the "fonts/" folder. See Makefile.emscripten for details.
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);


}

Window::~Window()
{   
    // Cleanup imgui
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // Cleanup glfw
    glfwDestroyWindow(m_Window);
	glfwTerminate();
}


static bool show_another_window = true;
static void draw_imgui() {
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    // ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    // ImGui::Text("Hello from another window!");
    // if (ImGui::Button("Close Me"))
    //     show_another_window = false;
    // ImGui::End();
    
    // ImGui::ShowDemoWindow();


    // Draw the GUI
    GUI::Manager::DrawGUI();



    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows
    // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
    //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}


void Window::DisplayLoop()
{
    // seed random number generator ===========================
    srand((unsigned int)time(0));

    // Render initialization ==================================
    renderer.BuildFramebuffer(m_ViewWidth, m_ViewHeight);
    std::cout << "framebuffer width " << m_ViewWidth << std::endl;
    std::cout << "framebuffer height " << m_ViewHeight << std::endl; 

    // Copy from CGL scenegraph ====================================    
    // TODO should just clone these
    scene.SetID(CGL::mainScene.GetID());  // copy scene ID
    scene.RegisterNode(&scene);  // register itself

    // size of moving average circular buffer
    const t_CKUINT N = 30;
    double deltaTimes[N];
    double deltaSum = 0;
    memset( deltaTimes, 0, sizeof(deltaTimes) );
    t_CKUINT dtWrite = 0;
    t_CKUINT dtInitFrames = 0;

    // Render Loop ===========================================
    double prevTickTime = glfwGetTime();
    while (!glfwWindowShouldClose(m_Window))
    {
        // ======================
        // enter critical section
        // ======================
        // waiting for audio synchronization (see cgl_update_event_waiting_on)
        // (i.e., when all registered GG.nextFrame() are called on their respective shreds)
        CGL::WaitOnUpdateDone();

        // question: why does putting this AFTER time calculation cause everything to be so choppy at high FPS?
        // hypothesis: puts time calculation into the critical region
        // time is updated only when all chuck shreds are on wait queue
        // guaranteeing that when they awake, they'll be using fresh dt data

        // get current window uptime
        double currentTime = glfwGetTime();
        // time since last frame
        m_DeltaTime = currentTime - prevTickTime;
        // update for next frame
        prevTickTime = currentTime;

        // increment frame counter
        if( dtInitFrames < N ) dtInitFrames++;
        // ensure minimum delta time to avoid potential divide by zero
        double deltaTime = m_DeltaTime >= .000001 ? m_DeltaTime : .000001;
        // subtract from accumulator
        deltaSum -= deltaTimes[dtWrite];
        // add the new value
        deltaSum += deltaTime;
        // remember
        deltaTimes[dtWrite] = deltaTime;
        // increment and modulo
        dtWrite = (dtWrite+1)%N;
        // update FPS
        if( deltaSum > 0.000001 ) CGL::SetFPS( ck_min(dtInitFrames,N) / deltaSum );
        // set time time
        CGL::SetTimeInfo( currentTime, m_DeltaTime );

        /* Note: this sync mechanism also gets rid of the problem where chuck runs away
        e.g. if the time it takes the renderer flush the queue is greater than
        the time it takes chuck to write, ie write rate > flush rate,
        each command queue will get longer and longer, continually worsening performance
        shouldn't happen because chuck runs in vm and the flushing happens natively but 
        you never know */

        /* two locks here:
        1 for writing/swapping the command queues
            - this lock is grabbed by chuck every time we do a CGL call
            - supports writing CGL commands whenever, even outside game loop
        1 for the condition_var used to synchronize audio and graphics each frame
            - combined with the chuck-side update_event, allows for writing frame-accurate cgl commands
            - exposes a gameloop to chuck, gauranteed to be executed once per frame
        deadlock shouldn't happen because both locks are never held at the same time */

        // swap command queues
        CGL::SwapCommandQueues();

        // done swapping the double buffer, let chuck know it's good to continue pushing commands
        // this wakes all shreds currently waiting on GG.nextFrame()
        CglEvent::Broadcast(CglEventType::CGL_UPDATE);
        // ====================
        // end critical section
        // ====================

        // now apply changes from the command queue chuck is NO Longer writing to
        // this executes all commands on the command queue, performs actions from CK code
        // essentially applying a diff to bring graphics state up to date with what is done in CK code
        CGL::FlushCommandQueue(scene, false);
        
        // process any glfw options passed from chuck
        UpdateState();

        // garbage collection! delete GPU-side data for any scenegraph objects that were deleted in chuck
        renderer.ProcessDeletionQueue(&scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        renderer.BindFramebuffer();
        renderer.Clear(
            scene.GetBackgroundColor(),
            true,   // clear color buffer
            true    // clear depth buffer
        );
        renderer.RenderScene(&scene, scene.GetMainCamera());

        // screen pass: copy contents from framebuffer to screen
        renderer.UnbindFramebuffer();  // unbind framebuffer, bind default framebuffer
        renderer.Clear(
            scene.GetBackgroundColor(),
            true,   // clear color buffer
            false   // don't clear depth buffer
        );
        renderer.RenderScreen();

        // Handle Events, Draw framebuffer
        glfwPollEvents();  // TODO: maybe put event handling in a separate thread?

        // imgui rendering
        draw_imgui();

        // swap double buffer
        // blocks until glfwSwapInterval screen updates have occured, accounting for vsync
        glfwSwapBuffers(m_Window);
    }
}
