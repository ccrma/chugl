#pragma once

#include "chugl_pch.h"

// ChuGL version string
#define CHUGL_VERSION_STRING "0.1.5-dev (alpha)"

// ChuGL query interface
t_CKBOOL init_chugl( Chuck_DL_Query * QUERY );

// foward decls =========================================
class Camera;
class Geometry;
class CGL_Texture;
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
	static void FlushCommandQueue(Scene& scene);
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
	static CGL_Texture* CreateChuckObjFromTex(CK_DL_API API, Chuck_VM* VM, CGL_Texture *tex, Chuck_VM_Shred* SHRED, bool refcount);
	static Material* DupMeshMat(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED);
	static Geometry* DupMeshGeo(CK_DL_API API, Chuck_VM *VM, Mesh *mesh, Chuck_VM_Shred *SHRED);
	static void MeshSet( Mesh * mesh, Geometry * geo, Material * mat );

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

private: // chuck object offsets
	static t_CKUINT geometry_data_offset;
	static t_CKUINT material_data_offset;
	static t_CKUINT texture_data_offset;
	static t_CKUINT ggen_data_offset;
	static t_CKUINT pp_effect_offset_data;
	static t_CKUINT gui_data_offset;

public:  // chuck object offset setters and getters
	static void SetGeometryDataOffset(t_CKUINT offset) { geometry_data_offset = offset; }
	static t_CKUINT GetGeometryDataOffset() { return geometry_data_offset; }
	static void SetMaterialDataOffset(t_CKUINT offset) { material_data_offset = offset; }
	static t_CKUINT GetMaterialDataOffset() { return material_data_offset; }
	static void SetTextureDataOffset(t_CKUINT offset) { texture_data_offset = offset; }
	static t_CKUINT GetTextureDataOffset() { return texture_data_offset; }
	static void SetGGenDataOffset(t_CKUINT offset) { ggen_data_offset = offset; }
	static t_CKUINT GetGGenDataOffset() { return ggen_data_offset; }
	static void SetPPEffectDataOffset(t_CKUINT offset) { pp_effect_offset_data = offset; }
	static t_CKUINT GetPPEffectDataOffset() { return pp_effect_offset_data; }
	static void SetGUIDataOffset(t_CKUINT offset) { gui_data_offset = offset; }
	static t_CKUINT GetGUIDataOffset() { return gui_data_offset; }

public: // chuck obj getters
	static Geometry* GetGeometry(Chuck_Object* ckobj) { return (Geometry *)OBJ_MEMBER_INT(ckobj, CGL::geometry_data_offset); }
	static Material* GetMaterial(Chuck_Object* ckobj) { return (Material *)OBJ_MEMBER_INT(ckobj, CGL::material_data_offset); }
	static CGL_Texture* GetTexture(Chuck_Object* ckobj) { return (CGL_Texture *)OBJ_MEMBER_INT(ckobj, CGL::texture_data_offset); }
	static SceneGraphObject* GetSGO(Chuck_Object* ckobj) { return (SceneGraphObject *)OBJ_MEMBER_INT(ckobj, CGL::ggen_data_offset); }

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
		Scene& scene, CK_DL_API API, Chuck_VM* VM, Chuck_VM_Shred* calling_shred
	);
	static t_CKINT our_update_vt_offset;

public: // global, lock-protected state for sending info from GLFW --> Chuck
    
    // struct to hold a state for one window
	struct WindowState {
        // window width and height (in screen coordinates)
        t_CKINT windowWidth, windowHeight;
        // frame buffer width and height (in pixels)
        t_CKINT framebufferWidth, framebufferHeight;
		// window aspect
		t_CKFLOAT aspect;
        // mouse X, mouse Y
        double mouseX, mouseY;
		// mouse delta X, mouse delta Y
		double mouseDX, mouseDY;
		// imgui capture mouse
		bool mouseCapturedByImGUI;
        // glfw time, delta time
		double glfwTime, deltaTime;
        // frames per second
        t_CKINT fps;

        // constructor
        WindowState();
	};

    // TODO: will need to be non-static if we support multiple windows
    static WindowState s_WindowState;
	static std::mutex s_WindowStateLock;
	// making these doubles to reduce lock frequency
	static std::pair<double, double> GetMousePos();
	static std::pair<double, double> GetMouseDelta();
	static bool GetMouseCapturedByImGUI();
	static std::pair<int, int> GetWindowSize();
	static std::pair<int, int> GetFramebufferSize();
	static t_CKFLOAT GetAspectRatio();
	static std::pair<double, double> GetTimeInfo();
	static void SetMousePos(double x, double y);
	static void ZeroMouseDeltas();
	static void SetMouseCapturedByImGUI(bool captured);
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
	static const t_CKUINT MOUSE_NORMAL;
	static const t_CKUINT MOUSE_HIDDEN;
	static const t_CKUINT MOUSE_LOCKED;

public: // window modes
	static const t_CKUINT WINDOW_WINDOWED;
	static const t_CKUINT WINDOW_FULLSCREEN;
	static const t_CKUINT WINDOW_MAXIMIZED;
	static const t_CKUINT WINDOW_RESTORE;
	static const t_CKUINT WINDOW_SET_SIZE;

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
    static CK_DL_API API; // specifically named API for decoupled OBJ_MEMBER_* access

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

private:  // SHRED --> GGen bookkeeping 
	// mapping shred to GGens created on that shred
	// Chuck_Objects are added on GGen construction, and removed on GGen destruction
	// only touched by audio thread, does not need to be lock protected
	static std::unordered_map<Chuck_VM_Shred*, std::unordered_set<Chuck_Object*>> s_Shred2GGen;
	// A separate map to track GGen --> Shred
	// This allows us to, in O(1) time, clear a GGen from a shred's GGen list inside
	// the GGen's CK_DLL_DTOR.
	// This is necessary because there are other instances, depending where/when an object is released,
	// where the CK_DLL_DTOR `SHRED` param is NULL
	static std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> s_GGen2Shred;

	// Run it back
	// This map is used by the auto-update invoker, to track the top-level ancestor
	// shred of the shred this GGen was created on.
	// This origin shred contains file-level variables that may be accessed in
	// this GGen's update(float dt) function
	// GGens register to this map on instantiation
	// GGens remove themselves from this map on destruction
	static std::unordered_map<Chuck_Object*, Chuck_VM_Shred*> s_GGen2OriginShred;
public: 

	static void RegisterGGenToShred(Chuck_VM_Shred *shred, Chuck_Object *ggen) {
		// register shred --> GGen
		s_Shred2GGen[shred].insert(ggen);

		// a GGen should only ever be associated with 1 shred
		assert(s_GGen2Shred.find(ggen) == s_GGen2Shred.end());

		// register GGen --> shred
		s_GGen2Shred[ggen] = shred;

		// register GGen --> origin shred
		auto* parent_shred = shred;
		// walk up parent chain until top-level
		while (API->shred->parent(parent_shred)) {
			parent_shred = API->shred->parent(parent_shred);
		}
		s_GGen2OriginShred[ggen] = parent_shred;
	}

	static void UnregisterGGenFromShred(Chuck_VM_Shred *shred, Chuck_Object *ggen) {
		// unregister GGen --> ancestor/origin shred
		// call this first in case the original shred has already exited and cleaned up
		s_GGen2OriginShred.erase(ggen);

		// if ggen not found in ggen2shred map, then it's already been unregistered
		// probably from it's parent shred already being destroyed
		if (s_GGen2Shred.find(ggen) == s_GGen2Shred.end()) return;

		// handle case where shred is NULL, try to find origin shred via lookup
		if (shred == NULL) {
			shred = s_GGen2Shred[ggen];
		} 

		// shred should match the shred in the GGen2Shred map
		assert(s_GGen2Shred[ggen] == shred);

		// unregister shred --> GGen
		if (s_Shred2GGen.find(shred) != s_Shred2GGen.end()) {
			s_Shred2GGen[shred].erase(ggen);
		}

		// unregister GGen --> shred
		s_GGen2Shred.erase(ggen);
	}

	static void DetachGGensFromShred(Chuck_VM_Shred *shred);
	
	// erases shred from map (call when shred is destroyed)
	static bool EraseFromShred2GGenMap(Chuck_VM_Shred *shred) {
		// flag for whether this is a graphics shred
		bool shredInMap = false;
		// first remove from GGen2Shred map
		if (s_Shred2GGen.find(shred) != s_Shred2GGen.end()) {
			shredInMap = true;
			for (auto* ggen : s_Shred2GGen[shred]) {
				s_GGen2Shred.erase(ggen);
				// DON'T REMOVE FROM ORIGIN SHRED MAP
				// in case origin shred is still alive, and has a reference to this GGen
			}
		}
		// then remove from Shred2GGen map
		s_Shred2GGen.erase(shred);
		return shredInMap;
	}

	static bool NoActiveGraphicsShreds() {
		return s_Shred2GGen.empty();
	}

	static bool NoActiveGGens() {
		if (s_Shred2GGen.empty()) assert(s_GGen2Shred.empty());
		if (s_GGen2Shred.empty()) assert(s_Shred2GGen.empty());

		return s_GGen2OriginShred.empty() && s_GGen2Shred.empty(); 
	}
};
