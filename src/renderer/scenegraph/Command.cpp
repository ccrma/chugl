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
