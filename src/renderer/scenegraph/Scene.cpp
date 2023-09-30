#include "Scene.h"
// static render state (options passed from chuck to configure glfw)

// mouse modes
unsigned int Scene::mouseMode = 0;
bool Scene::updateMouseMode = false; 

// window modes
unsigned int Scene::windowMode = 0;  // which mode, fullscreen or windowed
bool Scene::updateWindowMode = false;  // whether to update window mode
int Scene::windowedWidth = 0;
int Scene::windowedHeight = 0;  // last user-set window size