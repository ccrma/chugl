#pragma once

#include "chugl_pch.h"

// Garbage Collection Macros ============================
// x is a pointer to a SceneGraphNode
#define CHUGL_NODE_QUEUE_RELEASE(x)     do { Locator::QueueCKRelease(x); } while(0)
#define CHUGL_NODE_ADD_REF(x)           do { Locator::CKAddRef(x); } while(0)

class SceneGraphNode;  // avoid circular dependency

typedef std::unordered_map<size_t, SceneGraphNode*> SceneGraphMap;  // map of all scene graph objects

// locator service
class Locator
{
public:
    static void RegisterNode(SceneGraphNode* node);
    static SceneGraphNode* GetNode(size_t id, bool isAudioThread);
	static bool CheckNode(size_t id, bool isAudioThread);
    static void UnregisterNode(size_t id, bool isAudioThread);
    static void PrintContents(bool isAudioThread);

private:
    static SceneGraphMap& GetSceneGraphMap(bool isAudioThread) {
        return isAudioThread ? m_AudioSceneGraphMap : m_RendererSceneGraphMap;
    }

	static SceneGraphMap m_AudioSceneGraphMap;  // scenegraph map for chuck audio thread
	static SceneGraphMap m_RendererSceneGraphMap;  // scenegraph map for renderer thread's copy

private:
    static const Chuck_DL_Api* s_CKAPI;
public:
    // access the chugin runtime API
    static void SetCKAPI( const Chuck_DL_Api* api ) { s_CKAPI = api; }
    // access the chugin runtime API
    static const Chuck_DL_Api* CKAPI() { return s_CKAPI; }

// garbage collection queue
// allows us to defer calling ck_release on objects until we flush the queue
// preventing free/delete from being called on the audio audio thread until
// *after* we have finished disconnecting / cleanup / etc
private:
	// NOT threadsafe, only call from audio thread
	static std::vector<size_t> s_AudioThreadGCQueueRead;
	static std::vector<size_t> s_AudioThreadGCQueueWrite;
    static bool s_WhichGCQueue;
    static void SwapGCQueues() { s_WhichGCQueue = !s_WhichGCQueue; }
    static std::vector<size_t>& GetGCQueueRead() { return s_WhichGCQueue ? s_AudioThreadGCQueueRead : s_AudioThreadGCQueueWrite; }
    static std::vector<size_t>& GetGCQueueWrite() { return s_WhichGCQueue ? s_AudioThreadGCQueueWrite : s_AudioThreadGCQueueRead; }

public:
	static void QueueCKRelease(SceneGraphNode* node);
	static void CKAddRef(SceneGraphNode* node);
	static void GC();
};