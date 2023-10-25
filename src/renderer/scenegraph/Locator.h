#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"

typedef std::unordered_map<size_t, SceneGraphNode*> SceneGraphMap;  // map of all scene graph objects

// locator service
class Locator
{
public:
    static void RegisterNode(SceneGraphNode* node) {
        GetSceneGraphMap(node->IsAudioThreadObject())[node->GetID()] = node;
    }

    static SceneGraphNode* GetNode(size_t id, bool isAudioThread) {
        SceneGraphNode* node = CheckNode(id, isAudioThread) ? GetSceneGraphMap(isAudioThread)[id] : nullptr;
        if (node) {
            assert(node->IsAudioThreadObject() == isAudioThread);
        }
        return node;
    }

	static bool CheckNode(size_t id, bool isAudioThread) { 
        auto& map = GetSceneGraphMap(isAudioThread);
        return map.find(id) != map.end();
    }

    static void UnregisterNode(size_t id, bool isAudioThread)
    {
        GetSceneGraphMap(isAudioThread).erase(id);
    }


private:
    static SceneGraphMap& GetSceneGraphMap(bool isAudioThread) {
        return isAudioThread ? m_AudioSceneGraphMap : m_RendererSceneGraphMap;
    }

	static SceneGraphMap m_AudioSceneGraphMap;  // scenegraph map for chuck audio thread
	static SceneGraphMap m_RendererSceneGraphMap;  // scenegraph map for renderer thread's copy
};