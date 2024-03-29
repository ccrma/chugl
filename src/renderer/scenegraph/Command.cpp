#include "Command.h"
#include "chugl_pch.h"

CreateSceneGraphNodeCommand::CreateSceneGraphNodeCommand(
    SceneGraphNode* node,
    Scene* audioThreadScene,
    Chuck_Object* ckobj,
    t_CKUINT data_offset,
    CK_DL_API API
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

//-----------------------------------------------------------------------------
// DestroySceneGraphNodeCommand 
//-----------------------------------------------------------------------------

DestroySceneGraphNodeCommand::DestroySceneGraphNodeCommand(
        Chuck_Object* ckobj,
        t_CKUINT data_offset,
        CK_DL_API API,
        Scene* audioThreadScene
) : m_ID(0)
{
	SceneGraphNode* node = (SceneGraphNode*) OBJ_MEMBER_INT(ckobj, data_offset);
    // check
    assert(node->IsAudioThreadObject());
    // set m_ID
    m_ID = node->GetID();
    // zero out the chuck object memory
    OBJ_MEMBER_INT(node->m_ChuckObject, data_offset) = 0; // zero out the memory
    // remove from audio thread scene
    audioThreadScene->UnregisterNode(m_ID);
    CK_SAFE_DELETE(node);
}

void DestroySceneGraphNodeCommand::execute(Scene *renderThreadScene)
{
    SceneGraphNode* node = renderThreadScene->GetNode(m_ID);
    // remove from scenegraph
    renderThreadScene->UnregisterNode(m_ID);
    // add ID to deletion queue, so that renderer may destroy its GPU-side data
    renderThreadScene->AddToDeletionQueue(m_ID);
    // call destructor
    CK_SAFE_DELETE(node);
}


//-----------------------------------------------------------------------------
// UpdateCameraCommand
//-----------------------------------------------------------------------------
UpdateCameraCommand::UpdateCameraCommand(Camera *cam)
    : m_CamID(cam->GetID()), params(cam->params)
{}

void UpdateCameraCommand::execute(Scene *scene)
{
    Camera* cam = dynamic_cast<Camera*>(scene->GetNode(m_CamID));
    assert(cam);
    cam->params = params;
}
