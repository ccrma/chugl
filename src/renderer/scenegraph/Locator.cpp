#include "Locator.h"
#include "SceneGraphNode.h"

SceneGraphMap Locator::m_AudioSceneGraphMap;  // scenegraph map for chuck audio thread
SceneGraphMap Locator::m_RendererSceneGraphMap;  // scenegraph map for renderer thread's copy

const Chuck_DL_Api* Locator::s_CKAPI = nullptr;

std::vector<size_t> Locator::s_AudioThreadGCQueueRead;
std::vector<size_t> Locator::s_AudioThreadGCQueueWrite;
bool Locator::s_WhichGCQueue = false;

void Locator::RegisterNode(SceneGraphNode *node)
{
    if (!node) return;
    GetSceneGraphMap(node->IsAudioThreadObject())[node->GetID()] = node;
}

SceneGraphNode *Locator::GetNode(size_t id, bool isAudioThread)
{
    if (id == 0) return nullptr;
    SceneGraphNode* node = CheckNode(id, isAudioThread) ? GetSceneGraphMap(isAudioThread)[id] : nullptr;
    if (node) {
        assert(node->IsAudioThreadObject() == isAudioThread);
    }
    return node;
}

bool Locator::CheckNode(size_t id, bool isAudioThread)
{
    if (id == 0) return false;
    auto& map = GetSceneGraphMap(isAudioThread);
    return map.find(id) != map.end();
}

void Locator::UnregisterNode(size_t id, bool isAudioThread)
{
    GetSceneGraphMap(isAudioThread).erase(id);
}

void Locator::PrintContents(bool isAudioThread)
{
    auto& map = GetSceneGraphMap(isAudioThread);
    std::cout << "============Map contents============" << std::endl;
    for (auto it : map) {
        std::cout << it.first << " | " << it.second << std::endl;
    }
    std::cout << "====================================" << std::endl;
}

// Garbage Collection ====================================================================

void Locator::QueueCKRelease(SceneGraphNode *node)
{
    if (!node) return;
    if (!node->IsAudioThreadObject()) return;
    GetGCQueueWrite().push_back(node->GetID());
}

void Locator::CKAddRef(SceneGraphNode *node)
{
    if (!node) return;
    if (!node->IsAudioThreadObject()) return;
    CKAPI()->object->add_ref(node->m_ChuckObject);
}

void Locator::GC()
{
    // swap queues
    SwapGCQueues();

    // get read queue (now the old write queue)
    auto& readQueue = GetGCQueueRead();

    // release all objects in read queue
    for (auto id : readQueue) {
        SceneGraphNode * node = Locator::GetNode(id, true);
        if (!node) continue;  // node already deleted
        CKAPI()->object->release(node->m_ChuckObject);
    }

    // clear read queue
    readQueue.clear();
}
