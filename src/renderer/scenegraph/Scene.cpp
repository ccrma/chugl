#include "Scene.h"
#include "Light.h"
#include "Camera.h"
// static render state (options passed from chuck to configure glfw)

// mouse modes
unsigned int Scene::mouseMode = 0;
bool Scene::updateMouseMode = false; 

// window modes
unsigned int Scene::windowMode = 0;  // which mode, fullscreen or windowed
bool Scene::updateWindowMode = false;  // whether to update window mode
int Scene::windowedWidth = 0;
int Scene::windowedHeight = 0;  // last user-set window size
bool Scene::windowShouldClose = false;

// fog modes
const unsigned int Scene::FOG_EXP = FogType::Exponential;
const unsigned int Scene::FOG_EXP2 = FogType::ExponentialSquared;


void Scene::RegisterNode(SceneGraphNode *node)
{
    m_SceneGraphMap[node->GetID()] = node;

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

    SceneGraphNode* node = GetNode(id);

    // remove from map
    m_SceneGraphMap.erase(id);

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
