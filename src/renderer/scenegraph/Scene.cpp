#include "Scene.h"
#include "Light.h"
#include "Camera.h"
#include "CGL_Texture.h"

// fog modes
const t_CKUINT Scene::FOG_EXP = FogType::Exponential;
const t_CKUINT Scene::FOG_EXP2 = FogType::ExponentialSquared;


void Scene::RegisterNode(SceneGraphNode *node)
{
    assert(node);
    assert(node->IsAudioThreadObject() == IsAudioThreadObject());

    Locator::RegisterNode(node);

    // register light
    if (node->IsLight()) {
        RegisterLight((Light*)node);
        return;
    }

    // register camera
    if (node->IsCamera()) {
        RegisterCamera((Camera*)node);
        return;
    }
}

// remove node from all resource maps
void Scene::UnregisterNode(size_t id)
{
    if (!CheckNode(id)) return;

    SceneGraphNode* node = Locator::GetNode(id, IsAudioThreadObject());

    // make sure they belong to the same thread as the scene
    assert(node);
    assert(node->IsAudioThreadObject() == IsAudioThreadObject());

    // remove from locator 
    Locator::UnregisterNode(id, IsAudioThreadObject());

    if (node->IsLight()) {
        for (auto it = m_Lights.begin(); it != m_Lights.end(); ++it) {
            if (*it == id) {
                m_Lights.erase(it);
                return;
            }
        }
    }

    // remove from cameras list
    if (node->IsCamera()) {
        for (auto it = m_Cameras.begin(); it != m_Cameras.end(); ++it) {
            if (*it == id) {
                m_Cameras.erase(it);
                return;
            }
        }
    }
}

void Scene::RegisterLight(Light* light)
{
    assert(light);
    m_Lights.push_back(light->GetID());
}

void Scene::RegisterCamera(Camera *camera)
{
    assert(camera);
    m_Cameras.push_back(camera->GetID());
}

void Scene::SetSkybox(CGL_CubeMap *skybox)
{
    // TODO: refcount this
    skyboxID = skybox->GetID(); 
    // updateSkybox = true;
}
