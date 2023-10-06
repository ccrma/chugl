#include "chuck_dl.h"

#include "Camera.h"
#include "Command.h"



// Camera --------------------------------------------------

CreateCameraCommand::CreateCameraCommand(
    Camera *camera, Scene *audioThreadScene, Chuck_Object *ckobj,
    t_CKUINT data_offset
) : m_Camera(nullptr) {
    // add to scene
    audioThreadScene->RegisterCamera(camera);
    // add pointer to chuck object
    camera->m_ChuckObject = ckobj;
    // add pointer from chuck obj to camera
	OBJ_MEMBER_INT(ckobj, data_offset) = (t_CKINT)(camera);
    // not cloning until multiple cameras are supported
}