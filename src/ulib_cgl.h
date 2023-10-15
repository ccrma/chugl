#pragma once

#include "chuck_def.h"
#include "chuck_dl.h"


#include <condition_variable>
#include <unordered_map>
#include <unordered_set>
#include <vector>

t_CKBOOL init_chugl(Chuck_DL_Query * QUERY);

// foward decls =========================================
class Camera;
class Geometry;
class Material;
class Mesh;
class Scene;
class SceneGraphObject;
class SceneGraphCommand;



// exports =========================================
// DLL_QUERY cgl_query(Chuck_DL_Query* QUERY);

// class definitions ===============================

enum class CglEventType {
	CGL_FRAME,			// triggered at start of each window loop (assumes only 1 window)
	CGL_UPDATE,			// triggered after renderer swaps command queue double buffer (assumes only 1 renderer)
	CGL_WINDOW_RESIZE	// TODO: trigger on window resize
};

// storage class for thread-safe events
// all events are broadcast on the shared m_event_queue
// (as opposed to creating a separate circular buff for each event like how Osc does it)
class CglEvent  
{
public:
	CglEvent(Chuck_Event* event, Chuck_VM* vm, CK_DL_API api, CglEventType event_type);
	~CglEvent();
	// void wait(Chuck_VM_Shred* shred);
	void Broadcast();
	static void Broadcast(CglEventType event_type);
	static std::vector<CglEvent*>& GetEventQueue(CglEventType event_type);
	Chuck_Event* GetEvent() { return m_Event; }

	static CBufferSimple* s_SharedEventQueue;

private:
	Chuck_VM* m_VM;
	Chuck_Event* m_Event;
	CK_DL_API m_API;
	CglEventType m_EventType;

	// event queues, shared by all events
	static std::vector<CglEvent*> m_FrameEvents;
	static std::vector<CglEvent*> m_UpdateEvents;  // not used for now, will be used to support multiple windows
	static std::vector<CglEvent*> m_WindowResizeEvents;
};

// catch all class for exposing rendering API, will refactor later
class CGL
{
public:
	static void Render();  // called from chuck side to say update is done! begin deepcopy
	static void WaitOnUpdateDone();
	static bool shouldRender;
	static std::mutex GameLoopLock;
	static std::condition_variable renderCondition;

	static Scene mainScene;
	static Camera mainCamera;

public: // command queue methods
	static void SwapCommandQueues();
	static void FlushCommandQueue(Scene& scene, bool swap);
	static void PushCommand(SceneGraphCommand * cmd);

private:
	// hashset to track registered shreds
	// the key bool is set to false when a shred calls nextFrame, and true in the waiting on callback
	static std::unordered_map<Chuck_VM_Shred*, bool> m_RegisteredShreds;
	static std::unordered_set<Chuck_VM_Shred*> m_WaitingShreds;  // shreds for other others to finish their update logic

	static CglEvent* s_UpdateEvent;

	// static std::unordered_map<Chuck_VM_Shred*, Chuck_DL_Api::Object> m_ShredEventMap;  // map of shreds to CglUpdate Event 
public:  // creating chuck object helpers
	static Material* CreateChuckObjFromMat(CK_DL_API API, Chuck_VM *VM, Material *mat, Chuck_VM_Shred *SHRED, bool refcount);
	static Geometry* CreateChuckObjFromGeo(CK_DL_API API, Chuck_VM *VM, Geometry *geo, Chuck_VM_Shred *SHRED, bool refcount);
	static Material* DupMeshMat(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED);
	static Geometry* DupMeshGeo(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED);

public:
	static void RegisterShred(Chuck_VM_Shred* shred);
	static void UnregisterShred(Chuck_VM_Shred* shred);
	static bool IsShredRegistered(Chuck_VM_Shred* shred);
	static void MarkShredWaited(Chuck_VM_Shred* shred);
	static bool HasShredWaited(Chuck_VM_Shred *shred);
	static size_t GetNumRegisteredShreds();
	static void RegisterShredWaiting(Chuck_VM_Shred* shred);
	static void UnregisterShredWaiting(Chuck_VM_Shred* shred);
	static void ClearShredWaiting();
	static size_t GetNumShredsWaiting();

	// shred event map helper fns
	static CglEvent* GetShredUpdateEvent(
		Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM
	);

public:  // default GGens
	// static Chuck_DL_Api::Object GetMainCamera(
	// 	Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM
	// );
	static Chuck_DL_Api::Object GetMainScene(
		Chuck_VM_Shred *shred, CK_DL_API API, Chuck_VM *VM
	);

public: // scenegraph update traversal
 
	// TODO: what happens when graphics window loses focus for a long time and then returns?
	// does it suddenly pass a large dt? if so we may need to throttle
	static void UpdateSceneGraph(
		Scene& scene, CK_DL_API API, Chuck_VM* VM, Chuck_VM_Shred* shred
	);
	static t_CKINT our_update_vt_offset;

public: // global, lock-protected state for sending info from GLFW --> Chuck
    
    // struct to hold a state for one window
	struct WindowState {
        // window width and height (in screen coordinates)
        t_CKINT windowWidth, windowHeight;
        // frame buffer width and height (in pixels)
        t_CKINT framebufferWidth, framebufferHeight;
        // mouse X, mouse Y
        double mouseX, mouseY;
        // glfw time, delta time
		double glfwTime, deltaTime;
        // frames per second
        t_CKINT fps;

        // constructor
        WindowState() : windowWidth(1), windowHeight(1),
            framebufferWidth(1), framebufferHeight(1), mouseX(0), mouseY(0),
            glfwTime(0), deltaTime(0), fps(0) { }
	};

    // TODO: will need to be non-static if we support multiple windows
    static WindowState s_WindowState;
	static std::mutex s_WindowStateLock;
	// making these doubles to reduce lock frequency
	static std::pair<double, double> GetMousePos();
	static std::pair<int, int> GetWindowSize();
	static std::pair<int, int> GetFramebufferSize();
	static std::pair<double, double> GetTimeInfo();
	static void SetMousePos(double x, double y);
	static void SetWindowSize(int width, int height);
	static void SetFramebufferSize(int width, int height);
	static void SetTimeInfo(double glfwTime, double deltaTime);
    static void SetFPS( int fps );
    static int  GetFPS();

public:
	static double chuglChuckStartTime;  // value of chuck `now` when chugl is first initialized
	static double chuglLastUpdateTime;  // value of chuck `now` when chugl is first initialized
	static bool useChuckTime;

public:  // mouse modes
	static const unsigned int MOUSE_NORMAL;
	static const unsigned int MOUSE_HIDDEN;
	static const unsigned int MOUSE_LOCKED;

public: // window modes
	static const unsigned int WINDOW_WINDOWED;
	static const unsigned int WINDOW_FULLSCREEN;
	static const unsigned int WINDOW_MAXIMIZED;
	static const unsigned int WINDOW_RESTORE;
	static const unsigned int WINDOW_SET_SIZE;

public: // global main thread hook
	static Chuck_DL_MainThreadHook* hook;
	static bool hookActivated;
	static void ActivateHook();
	static void DeactivateHook();

public: // VM and API references
    static void SetCKVM( Chuck_VM * theVM );
    static void SetCKAPI( CK_DL_API theAPI );
    static Chuck_VM * CKVM() { return s_vm; }
    static CK_DL_API CKAPI() { return s_api; }

protected:
    static Chuck_VM * s_vm;
    static CK_DL_API s_api;

private: // attributes
	// command queues 
	// the commands need to be executed before renderering...putting here for now
	static std::vector<SceneGraphCommand*> m_ThisCommandQueue;
	static std::vector<SceneGraphCommand*> m_ThatCommandQueue;
	static bool m_CQReadTarget;  // false = this, true = that
	// command queue lock
	static std::mutex m_CQLock; // only held when 1: adding new command and 2: swapping the read/write queues
private:
	static inline std::vector<SceneGraphCommand*>& GetReadCommandQueue() { 
		return m_CQReadTarget ? m_ThatCommandQueue : m_ThisCommandQueue; 
	}
	// get the write target command queue
	static inline std::vector<SceneGraphCommand*>& GetWriteCommandQueue() {
		return m_CQReadTarget ? m_ThisCommandQueue : m_ThatCommandQueue;
	}
};
