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

// fog modes
const unsigned int Scene::FOG_EXP = FogType::Exponential;
const unsigned int Scene::FOG_EXP2 = FogType::ExponentialSquared;

Scene::Scene() : m_BackgroundColor(glm::vec3(0.9f))
{
    // actually, going to create default light in ulib chugl for now
    // so changes can be propagated to renderer thread
    // Give every scene default dirlight
    // m_DefaultLight = new DirLight;
    // m_DefaultLight->SetRotation(glm::vec3(-45.0f, 0.0f, 0.0f));
    // AddChild(m_DefaultLight);
    // RegisterLight(m_DefaultLight);
    // RegisterNode(m_DefaultLight);

    // remember to create chuck object for this light in ulib_cgl
}

Scene::~Scene()
{
    // not deleting m_DefaultLight here in case there are other chuck-side references
}

void Scene::RegisterLight(Light* light)
{
    RegisterNode(light);
    m_Lights.push_back(light);
}

void Scene::RegisterCamera(Camera *camera)
{
    RegisterNode(camera);
    m_Cameras.push_back(camera);
}
