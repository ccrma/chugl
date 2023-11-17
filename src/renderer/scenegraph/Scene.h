#pragma once

#include "chugl_pch.h"

#include "SceneGraphObject.h"
#include "Locator.h"

class Light;
class DirLight;
class Camera;
class CGL_CubeMap;


enum FogType : t_CKUINT {
	Exponential = 0,
	ExponentialSquared
};

struct FogUniforms {
	glm::vec3 color;
	float density;
	FogType type;
	bool enabled;

	FogUniforms() 
	: color(glm::vec3(0.0f)), density(0.0f), type(FogType::Exponential), enabled(true) {}
};

class Scene : public SceneGraphObject
{
public:
	Scene() 
		: m_BackgroundColor(glm::vec3(0.0f)),
		m_FogUniforms(FogUniforms()),
		// mouse modes
		m_MouseMode(0),
		m_UpdateMouseMode(false),
		// window modes
		m_WindowMode(0),
		m_UpdateWindowMode(false),
		m_WindowedWidth(0),
		m_WindowedHeight(0),
		m_WindowShouldClose(false),
		// window title
		m_UpdateWindowTitle(false),
		m_WindowTitle("ChuGL"),
		// skybox
		skyboxEnabled(false)
		// updateSkybox(false)
	{}
	virtual ~Scene() {}
	virtual bool IsScene() override { return true; }

	virtual SceneGraphNode* Clone() override {
		assert(false);
		return nullptr;

		// Scene * scene = new Scene();

		// // copy ID
		// scene->SetID(this->GetID());
		
		// // TODO: clone all nodes
		// // for (auto it = m_SceneGraphMap.begin(); it != m_SceneGraphMap.end(); ++it) {
		// // 	scene->RegisterNode(it->second->Clone());
		// // }

		// return scene;
	}

	// register new SceneGraphNode
	void RegisterNode(SceneGraphNode* node);
	// remove any scene pointers to this node
	void UnregisterNode(size_t id);
	bool CheckNode(size_t id) { return Locator::CheckNode(id, IsAudioThreadObject()); }
	SceneGraphNode * GetNode(size_t id) { return Locator::GetNode(id, IsAudioThreadObject()); }


	
private:  // attributes
	// this lives in scene obj for now because we want to decouple from the renderer
	// to support multiple scenes in the future, can make this map static
	void RegisterLight(Light* light);
	void RegisterCamera(Camera* camera);

public: // lightin
	// problem: don't render lights if they are deparented from scene
	// temporarily solved by checking if light is child of current scene by walking up scenegraph
	// big soln: get rid of scenegraphs altogether. add matrix math class to chuck, so users can implement scenegraph there if they really want
		// all scenegraphobjects are instead stored as a flightlist, each with a pointer to 1 parent scene
		// no parent/child relationships, except for 1 scene and all its children 
	// lights and nodes should be stored separate from scene, store inside render state
	// std::vector<Light*> m_Lights;  // list of all lights in scene
	std::vector<size_t> m_Lights;  // list of all lights in scene
	Light* GetDefaultLight() { 
		if (m_Lights.size() == 0) return nullptr;

		// delete light if its not registered
		while (!CheckNode(m_Lights[0])) {
			m_Lights.erase(m_Lights.begin());
			if (m_Lights.size() == 0) return nullptr;
		}

		return (Light*)GetNode(m_Lights[0]);
	}


public: // camera
	// std::vector<Camera*> m_Cameras;  // list of all cameras in scene
	std::vector<size_t> m_Cameras;  // list of all cameras in scene
	Camera* GetMainCamera() { 
		if (m_Cameras.size() == 0) return nullptr;

		// delete camera if its not registered
		while (!CheckNode(m_Cameras[0])) {
			m_Cameras.erase(m_Cameras.begin());
			if (m_Cameras.size() == 0) return nullptr;
		}

		return (Camera*)GetNode(m_Cameras[0]);
	}

public: // fog
	FogUniforms m_FogUniforms;
	static const t_CKUINT FOG_EXP;
	static const t_CKUINT FOG_EXP2;
	void SetFogColor(float r, float g, float b) {
		m_FogUniforms.color = glm::vec3(r, g, b);
	}
	glm::vec3 GetFogColor() { return m_FogUniforms.color; }
	void SetFogDensity(float d) { m_FogUniforms.density = d; }
	float GetFogDensity() { return m_FogUniforms.density; }
	void SetFogType(FogType type) { m_FogUniforms.type = type; }
	FogType GetFogType() { return m_FogUniforms.type; }
	void SetFogEnabled(bool enabled) { m_FogUniforms.enabled = enabled; }
	bool GetFogEnabled() { return m_FogUniforms.enabled; }


private:  // skybox data
	bool skyboxEnabled;
	// bool updateSkybox;
	size_t skyboxID;

public: // skybox methods
	void SetSkyboxEnabled(bool enabled) { skyboxEnabled = enabled; }
	bool GetSkyboxEnabled() { return skyboxEnabled; }
	// void SetUpdateSkybox(bool enabled) { updateSkybox = enabled; }
	// bool GetUpdateSkybox() { return updateSkybox; }
	void SetSkybox(CGL_CubeMap* skybox);
	size_t GetSkyboxID() { return skyboxID; }

public: // background color ie clear color
	glm::vec3 m_BackgroundColor;
	void SetBackgroundColor(float r, float g, float b) {
		m_BackgroundColor = glm::vec3(r, g, b);
	}
	glm::vec3 GetBackgroundColor() { return m_BackgroundColor; }

private:  // for propogating deletion to the renderer
	std::vector<size_t> m_DeletionQueue;
public:
	void AddToDeletionQueue(size_t id) { m_DeletionQueue.push_back(id); }
	void ClearDeletionQueue() { m_DeletionQueue.clear(); }
	std::vector<size_t>& GetDeletionQueue() { return m_DeletionQueue; }

public: // major hack, for now because there's only 1 scene, storing render state options here
	// but maybe these modes can be stored in CGL class? or create a "window" scenegraph type that stores per-window metadata and settings

	// mouse modes
	unsigned int m_MouseMode;
	bool m_UpdateMouseMode; 

	// windowing modes
	unsigned int m_WindowMode;  // which mode, fullscreen or windowed
	bool m_UpdateWindowMode;  // whether to update window mode
	int m_WindowedWidth, m_WindowedHeight;  // last user-set window size
	bool m_WindowShouldClose;

	// window title
	bool m_UpdateWindowTitle;
	std::string m_WindowTitle;
};
