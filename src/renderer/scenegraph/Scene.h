#pragma once
#include "SceneGraphObject.h"
#include <unordered_map>

class Light;
class DirLight;


enum FogType : unsigned int {
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
	Scene();
	virtual ~Scene();
	virtual bool IsScene() override { return true; }

	Scene * Clone() {

		Scene * scene = new Scene();

		// copy ID
		scene->SetID(this->GetID());
		
		// TODO: clone all nodes
		// for (auto it = m_SceneGraphMap.begin(); it != m_SceneGraphMap.end(); ++it) {
		// 	scene->RegisterNode(it->second->Clone());
		// }

		return scene;
	}

	// register new SceneGraphNode
	inline void RegisterNode(SceneGraphNode* node) {
		m_SceneGraphMap[node->GetID()] = node;
	}

	void RegisterLight(Light* light);

	bool CheckNode(size_t id) {
		return m_SceneGraphMap.find(id) != m_SceneGraphMap.end();
	}

	SceneGraphNode * GetNode(size_t id) { 
		if (CheckNode(id))
			return m_SceneGraphMap[id]; 
		return nullptr;
	}


	
private:  // attributes
	// this lives in scene obj for now because we want to decouple from the renderer
	// to support multiple scenes in the future, can make this map static
	std::unordered_map<size_t, SceneGraphNode*> m_SceneGraphMap;  // map of all scene graph objects

public: // lightin
	// problem: don't render lights if they are deparented from scene
	// temporarily solved by checking if light is child of current scene by walking up scenegraph
	// big soln: get rid of scenegraphs altogether. add matrix math class to chuck, so users can implement scenegraph there if they really want
		// all scenegraphobjects are instead stored as a flightlist, each with a pointer to 1 parent scene
		// no parent/child relationships, except for 1 scene and all its children 
	// lights and nodes should be stored separate from scene, store inside render state
	std::vector<Light*> m_Lights;  // list of all lights in scene
	// Light* m_DefaultLight;  // default directional light
	// Light* GetDefaultLight() { return m_DefaultLight; }

public: // fog
	FogUniforms m_FogUniforms;
	static const unsigned int FOG_EXP;
	static const unsigned int FOG_EXP2;
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

public: // background color ie clear color
	glm::vec3 m_BackgroundColor;
	void SetBackgroundColor(float r, float g, float b) {
		m_BackgroundColor = glm::vec3(r, g, b);
	}
	glm::vec3 GetBackgroundColor() { return m_BackgroundColor; }

public: // major hack, for now because there's only 1 scene, storing render state options here
	// THESE ARE NOT THREADSAFE, ONLY WRITE/READ FROM RENDER THREAD
	// set indirectly via scenegraph commands
	// all this in order to maintain strict decoupling between scenegraph and any specific renderer impl
	// but maybe these modes can be stored in CGL class? or create a "window" scenegraph type that stores per-window metadata and settings
	static unsigned int mouseMode;
	static bool updateMouseMode; 

	static unsigned int windowMode;  // which mode, fullscreen or windowed
	static bool updateWindowMode;  // whether to update window mode
	static int windowedWidth, windowedHeight;  // last user-set window size


};
