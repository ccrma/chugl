#include "window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

// CGL includes
#include "renderer/Renderer.h"
#include "renderer/Graphics.h"

#include "renderer/scenegraph/SceneGraphObject.h"
#include "renderer/scenegraph/Camera.h"
#include "renderer/scenegraph/Scene.h"
#include "renderer/scenegraph/Locator.h"

// chuck CORE includes
#include "ulib_cgl.h"
#include "ulib_gui.h"

// imgui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// pch
#include "chugl_pch.h"

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
    // TODO: figure out how to get access to active window object
    // Window::GetInstance().SetViewSize(width, height);

    glViewport(
        0, 0,  // index of lower left corner of viewport, in pixels
        width, height
    );

    // update framebuffer texture size
    Window::renderer.UpdateFramebufferSize(width, height);

    // update window state
    Window* w = Window::GetWindow(window);
    w->SetFrameSize(width, height);
    CGL::SetFramebufferSize(width, height);

    // auto-update camera aspect
    if (height > 0) {  // don't change aspect if screen minimizes, otherwises causes div by 0 crash
        if (Window::scene.GetMainCamera()) {
            // bug: this doesn't correctly set aspect on the audio-thread camera scenegraph node
            // for now its okay, because this field is not currently exposed to chuck in ulib_camera.cpp
            Window::scene.GetMainCamera()->params.aspect = (float)width / (float)height;
        }
    }

    CglEvent::Broadcast(CglEventType::CGL_WINDOW_RESIZE);  // doesn't matter if we broadcast this in framebuffer callback or window size callback
}

// window sizs in screen-space coordinates
static void window_size_callback(GLFWwindow* window, int width, int height)
{
    Window* w = Window::GetWindow(window);
    w->SetWindowSize(width, height);
    CGL::SetWindowSize(width, height);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    // uncomment to disable mouse input when imgui is active
    // ImGuiIO& io = ImGui::GetIO();
    // if (io.WantCaptureMouse) return;

    CGL::SetMousePos(xpos, ypos);
}

static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

// static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
// {
//     if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
//         double xpos, ypos;
//         glfwGetCursorPos(window, &xpos, &ypos);
//         CGL::SetMousePos(xpos, ypos);
//         CGL::SetMouseLeftDown(true);
//     }
//     else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
//         CGL::SetMouseLeftDown(false);
//     }
// }

static void glfwErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error[%i]: %s\n", error, description);
}

static void APIENTRY glDebugOutput(GLenum source, 
                            GLenum type, 
                            unsigned int id, 
                            GLenum severity, 
                            GLsizei length, 
                            const char *message, 
                            const void *userParam)
{
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return; // ignore these non-significant error codes

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;
    std::cout << "Source: " << source << std::endl;
    std::cout << "Type: " << type << std::endl;
    std::cout << "Severity: " << severity << std::endl;

    // TODO: current glad is opengl 4.1, so below constants are undefined
    // switch (source)
    // {
    //     case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
    //     case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
    //     case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
    //     case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
    //     case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
    //     case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    // } std::cout << std::endl;

    // switch (type)
    // {
    //     case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
    //     case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
    //     case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break; 
    //     case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
    //     case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
    //     case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
    //     case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
    //     case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
    //     case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    // } std::cout << std::endl;
    
    // switch (severity)
    // {
    //     case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
    //     case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
    //     case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
    //     case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    // } std::cout << std::endl;
    std::cout << std::endl;
}


/* =============================================================================
						    * Window *	
===============================================================================*/

// static initializers
std::unordered_map<GLFWwindow*, Window*> Window::s_WindowMap;
int Window::s_DefaultWindowWidth = 1244;
int Window::s_DefaultWindowHeight = 700;

void Window::SetWindowSize(int width, int height)
{
    m_WindowWidth = width;
    m_WindowHeight = height;
}

void Window::SetFrameSize(int width, int height)
{
    m_FrameWidth = width;
    m_FrameHeight = height;
}

// Updates window state based on render-thread scene params
void Window::UpdateState(Scene& scene)
{
    // mouse mode
    if (scene.m_UpdateMouseMode) {
        if (scene.m_MouseMode == CGL::MOUSE_NORMAL) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else if (scene.m_MouseMode == CGL::MOUSE_HIDDEN) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else if (scene.m_MouseMode == CGL::MOUSE_LOCKED) {
            glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }

    if (scene.m_UpdateWindowMode) {
        if (scene.m_WindowMode == CGL::WINDOW_WINDOWED) {
            // set window (pos at 100,100 so title bar appears....silly)
            glfwSetWindowMonitor(m_Window, NULL, 100, 100, scene.m_WindowedWidth, scene.m_WindowedHeight, GLFW_DONT_CARE);
            //re-enable decorations
            glfwSetWindowAttrib(m_Window, GLFW_DECORATED, GLFW_TRUE);
        }
        else if (scene.m_WindowMode == CGL::WINDOW_FULLSCREEN) {
            // set to primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            glfwSetWindowMonitor(m_Window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        } else if (scene.m_WindowMode == CGL::WINDOW_MAXIMIZED) {
            // maximize
            glfwMaximizeWindow(m_Window);
        } else if (scene.m_WindowMode == CGL::WINDOW_RESTORE) {
            // restore
            glfwRestoreWindow(m_Window);
        } else if (scene.m_WindowMode == CGL::WINDOW_SET_SIZE) {
            // if in windowed mode, sets size in screen coordinates
            // if in full screen, changes resolution 
            glfwSetWindowSize(m_Window, scene.m_WindowedWidth, scene.m_WindowedHeight);
        }

        // reset flag
        scene.m_UpdateWindowMode = false;
    }

    // window close command
    if (scene.m_WindowShouldClose) {
        glfwSetWindowShouldClose(m_Window, GLFW_TRUE);
        scene.m_WindowShouldClose = false;  // reset flag
    }

    // window title
    if (scene.m_UpdateWindowTitle) {
        glfwSetWindowTitle(m_Window, scene.m_WindowTitle.c_str());
        scene.m_UpdateWindowTitle = false;
    }

}

Window::Window(int windowWidth, int windowHeight) 
    : m_WindowWidth(windowWidth), m_WindowHeight(windowWidth), 
    m_FrameWidth(windowWidth), m_FrameHeight(windowWidth),
    m_DeltaTime(0.0f), m_Window(nullptr)
{
    // Set error callback ======================================
    glfwSetErrorCallback(glfwErrorCallback);

    // init and select openGL version ==========================
    const char * glsl_version = "#version 330";
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef CHUGL_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // window hints =============================================
    glfwWindowHint(GLFW_SAMPLES, 4);  // MSAA

    
    // create window ============================================
    m_Window = glfwCreateWindow(m_WindowWidth, m_WindowHeight, "ChuGL", NULL, NULL);
    if (m_Window == NULL)
    {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(m_Window);
    SetWindow(m_Window, this);  // add to static map

    // VSYNC =================================================
#ifdef NDEBUG
    glfwSwapInterval(1);  
#else
	// we disable vsync in debug mode to more consistently catch
    // race conditions, concurrency bugs
    glfwSwapInterval(0);  
#endif

    // Initialize GLAD =======================================
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    // Print Context Info =================================================
    std::cerr << "============================= ChuGL =================================" << std::endl;
    std::cerr << "ChuGL version: " << CHUGL_VERSION_STRING << std::endl;
    std::cerr << "ChucK compatibility: " << CHUCK_VERSION_STRING << std::endl;
    std::cerr << "===================== OpenGL context info ===========================" << std::endl;
    std::cerr << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cerr << "OpenGL vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cerr << "OpenGL renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cerr << "OpenGL shading language version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cerr << "=========================== GLFW info ===============================" << std::endl;
    std::cerr << "GLFW version: " << glfwGetVersionString() << std::endl;
    std::cerr << "=====================================================================" << std::endl;

    // OpenGL Debug callback ==================================

    // enable OpenGL debug context if context allows for debug context
#ifdef CHUGL_DEBUG
    // TODO: unsupported in current glad 4.1 version
    // int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    // if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    // {
    //     glEnable(GL_DEBUG_OUTPUT);
    //     glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
    //     glDebugMessageCallback(glDebugOutput, nullptr);
    //     glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    // }
#endif

    // OpenGL Viewport and Callbacks =========================
    glfwSetFramebufferSizeCallback(m_Window, framebuffer_size_callback);
    glfwSetWindowSizeCallback(m_Window, window_size_callback);
    glfwSetKeyCallback(m_Window, keyCallback);

    glfwPollEvents();  // call poll events first to get correct framebuffer size (glfw bug: https://github.com/glfw/glfw/issues/1968)

    // for high-DPI displays, framebuffer size is actually a multiple of window size
    int frameBufferWidth, frameBufferHeight;
    int actualWindowWidth, actualWindowHeight;
    glfwGetFramebufferSize(m_Window, &frameBufferWidth, &frameBufferHeight);
    glfwGetWindowSize(m_Window, &actualWindowWidth, &actualWindowHeight);

    glViewport(0, 0, frameBufferWidth, frameBufferHeight);

    // call the callback to first initialize 
    framebuffer_size_callback(m_Window, frameBufferWidth, frameBufferHeight);
    window_size_callback(m_Window, actualWindowWidth, actualWindowHeight);

    // mouse settings =====================================
    glfwSetCursorPosCallback(m_Window, cursor_position_callback);


    // OpenGL Metadata =======================================
    // only print in debug
#ifdef CHUGL_DEBUG
    int nrAttributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &nrAttributes);
    std::cout << "Maximum number of 4-component vertex attributes: " << nrAttributes << std::endl;

    int maxTextureUnits;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    std::cout << "Maximum number of texture units: " << maxTextureUnits << std::endl;

    int maxTextureSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    std::cout << "Maximum texture size: " << maxTextureSize << std::endl;
#endif // CHUGL_DEBUG
    
    // GLEnables ===============================================
    // Blending (TODO) should this be part of renderer? 
    GLCall(glEnable(GL_BLEND));
    // linear interpolation for alpha blending
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
    
    // update imgui mouse capture
    CGL::SetMouseCapturedByImGUI(io.WantCaptureMouse);
}


void Window::DisplayLoop()
{
    // Tracy startup ========================================
#ifdef TRACY_ENABLE
    tracy::StartupProfiler();
#endif

    // seed random number generator ===========================
    srand((unsigned int)time(0));

    // Render initialization ==================================
    // This setup must happen *after* OpenGL context is created
    renderer.BuildFramebuffer(m_FrameWidth, m_FrameHeight);
    std::cout << "framebuffer width " << m_FrameWidth << std::endl;
    std::cout << "framebuffer height " << m_FrameHeight << std::endl; 

    // builds skybox buffers and cubemap texture
    renderer.BuildSkybox();

    // Copy from CGL scenegraph ====================================    
    // TODO should just clone these
    scene.SetID(CGL::mainScene.GetID());  // copy scene ID
    scene.SetIsAudioThreadObject(false);  // mark as belonging to render thread
    Locator::RegisterNode(&scene);  // register scene in locator
    scene.m_ChuckObject = nullptr;  // we DON'T want render thread to every touch ckobj

    // size of moving average circular buffer
    const t_CKUINT N = 30;
    double deltaTimes[N];
    double deltaSum = 0;
    memset( deltaTimes, 0, sizeof(deltaTimes) );
    t_CKUINT dtWrite = 0;
    t_CKUINT dtInitFrames = 0;
    bool firstFrame = true;

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
        CGL::FlushCommandQueue(scene);
        
        // process any glfw options passed from chuck
        UpdateState(scene);

        // hack to update main camera aspect on first frame
        // reason: camera is created on audio-thread BEFORE activating main hook and starting this display loop
        // so we can't set the aspect on chuck audio-thread side b/c window has not yet been launched, and we don't
        // know the actual aspect ratio
        // we only know the aspect ratio at this point, after the first flush of the command queue, which 
        // creates the camera on the render thread
        // Other workarounds:
        // - create default camera on render thread, like default scene (not idea, ideally we want ALL objects
        // even the scene to be created on the audio thread)
        // - have an initial "load" phase like love.load() which is used for initialization of camera, scene,
        // materials, etc. 
        //   - better. can have load screen while this is happening. will be long-term solution
        if (firstFrame) {
            if (scene.GetMainCamera()) {
                scene.GetMainCamera()->params.aspect = (float) GetFrameWidth() / (float) GetFrameHeight();
            }
        }

        // garbage collection! delete GPU-side data for any scenegraph objects that were deleted in chuck
        renderer.ProcessDeletionQueue(&scene); // IMPORTANT: should happen after flushing command queue

        // now renderer can work on drawing the copied scenegraph
        renderer.RenderScene(&scene, scene.GetMainCamera());

        // output pass: apply gamma correction, tone mapping, anti-aliasing, etc.
        // Tonemapping:
        // - reinhard
        // - try clamp: color.rgb = min(color.rgb, 60.0);
            // avoids precision limitations
        // Gamma Correction
        // - add Texture.color_space constants to set internal texture formats

        // Handle Events, Draw framebuffer
        CGL::ZeroMouseDeltas();  // reset mouse deltas
        glfwPollEvents();  // TODO: maybe put event handling in a separate thread?

        // imgui rendering
        draw_imgui();

        // swap double buffer
        // blocks until glfwSwapInterval screen updates have occured, accounting for vsync
        glfwSwapBuffers(m_Window);

        // Tracy Frame
        FrameMark;

        firstFrame = false;
    }

    // Tracy shutdown (must happen before chuck VM calls dlclose and unloads this chugl library)
    // otherwise `FreeLibrary()` hangs in the chuck VM thread
    // Note: this depends on TRACY_DELAYED_INIT and TRACY_MANUAL_LIFETIME being defined in the cmake configuration
    // see: https://github.com/wolfpld/tracy/issues/245
    // By calling this here, exiting the chugl window allows the chuck VM to exit cleanly

#ifdef TRACY_ENABLE
    tracy::ShutdownProfiler();
#endif
}
