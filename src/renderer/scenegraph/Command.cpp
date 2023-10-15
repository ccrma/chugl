#include "chuck_dl.h"

#include "Camera.h"
#include "Command.h"

CreateSceneGraphNodeCommand::CreateSceneGraphNodeCommand(
    SceneGraphNode* node,
    Scene* audioThreadScene,
    Chuck_Object* ckobj,
    t_CKUINT data_offset
) : m_Clone(node->Clone())
{
    // set audio thread ownership
    node->SetIsAudioThreadObject(true);
    m_Clone->SetIsAudioThreadObject(false);  // false, the clone is owned by render thread

    // add to scene
    audioThreadScene->RegisterNode(node);
    // add pointer to chuck object
    node->m_ChuckObject = ckobj;
    m_Clone->m_ChuckObject = nullptr;  // we DON'T want render thread to every touch ckobj
    // add pointer from chuck obj to node 
	OBJ_MEMBER_INT(ckobj, data_offset) = (t_CKINT) node;
}

void CreateSceneGraphNodeCommand::execute(Scene *scene)
{
    scene->RegisterNode(m_Clone);
}
