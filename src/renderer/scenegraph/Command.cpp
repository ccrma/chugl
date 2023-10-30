#include "Command.h"
#include "chugl_pch.h"

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

// MeshCommands (will refactor once we add an ID locator service)

CreateMeshCommand::CreateMeshCommand(Mesh *mesh, Scene *audioThreadScene, Chuck_Object *ckobj, t_CKUINT data_offset)
:  mesh_ID(mesh->GetID()), mat_ID(mesh->GetMaterialID()), geo_ID(mesh->GetGeometryID())
{
    // set audio thread ownership
    mesh->SetIsAudioThreadObject(true);

    // add to scene
    audioThreadScene->RegisterNode(mesh);
    // add pointer to chuck object
    mesh->m_ChuckObject = ckobj;
    // add pointer from chuck obj to node 
	OBJ_MEMBER_INT(ckobj, data_offset) = (t_CKINT) mesh;

}

void CreateMeshCommand::execute(Scene *scene)
{
    // Get the cloned material and geometry
    Material* clonedMat = dynamic_cast<Material*>(scene->GetNode(mat_ID));
    Geometry* clonedGeo = dynamic_cast<Geometry*>(scene->GetNode(geo_ID));
    Mesh* newMesh = new Mesh(clonedGeo, clonedMat);
    newMesh->SetID(mesh_ID);

    newMesh->SetIsAudioThreadObject(false);  // false, the clone is owned by render thread
    newMesh->m_ChuckObject = nullptr;  // we DON'T want render thread to every touch ckobj

    scene->RegisterNode(newMesh);
}

//-----------------------------------------------------------------------------
// DestroySceneGraphNodeCommand 
//-----------------------------------------------------------------------------

DestroySceneGraphNodeCommand::DestroySceneGraphNodeCommand(
        Chuck_Object* ckobj,
        t_CKUINT data_offset,
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
