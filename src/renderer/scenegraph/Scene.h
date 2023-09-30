#pragma once
#include "SceneGraphObject.h"
#include <unordered_map>

class Light;


class Scene : public SceneGraphObject
{
public:
	Scene() {
		fprintf(stderr, "Scene constructor (%zu)\n", m_ID);
	}
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

	inline void RegisterLight(Light* light) {
		m_Lights.push_back(light);
	}

	bool CheckNode(size_t id) {
		return m_SceneGraphMap.find(id) != m_SceneGraphMap.end();
	}

	SceneGraphNode * GetNode(size_t id) { 
		if (CheckNode(id))
			return m_SceneGraphMap[id]; 
		return nullptr;
	}


	// problem: don't render lights if they are deparented from scene
	// temporarily solved by checking if light is child of current scene by walking up scenegraph
	// big soln: get rid of scenegraphs altogether. add matrix math class to chuck, so users can implement scenegraph there if they really want
		// all scenegraphobjects are instead stored as a flightlist, each with a pointer to 1 parent scene
		// no parent/child relationships, except for 1 scene and all its children 
	
	// lights and nodes should be stored separate from scene, store inside render state
	std::vector<Light*> m_Lights;  // list of all lights in scene
private:  // attributes
	// this lives in scene obj for now because we want to decouple from the renderer
	// to support multiple scenes in the future, can make this map static
	std::unordered_map<size_t, SceneGraphNode*> m_SceneGraphMap;  // map of all scene graph objects


public: // major hack, for now because there's only 1 scene, storing render state options here
	// THESE ARE NOT THREADSAFE, ONLY WRITE/READ FROM RENDER THREAD
	static unsigned int mouseMode;
	static bool updateMouseMode; 

	static unsigned int windowMode;  // which mode, fullscreen or windowed
	static bool updateWindowMode;  // whether to update window mode
	static int windowedWidth, windowedHeight;  // last user-set window size


};
