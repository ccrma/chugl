#pragma once
#include "SceneGraphObject.h"
#include <unordered_map>

class Scene : public SceneGraphObject
{
public:
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

};
