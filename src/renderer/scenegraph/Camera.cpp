#include "Camera.h"

const unsigned int Camera::MODE_PERSPECTIVE = CameraType::PERSPECTIVE;
const unsigned int Camera::MODE_ORTHO = CameraType::ORTHO;

glm::mat4 Camera::GetProjectionMatrix()
{
    if (GetMode() == CameraType::PERSPECTIVE)
		return glm::perspective(
            glm::radians(params.fov), params.aspect, params.nearPlane, params.farPlane 
        );
    else {
        float width = params.size * params.aspect;
        float height = params.size;
        return glm::ortho<float>( // extents in WORLD SPACE units
            -width/ 2.0f, width / 2.0f,
            -height / 2.0f, height / 2.0f,
            params.nearPlane, params.farPlane
        );

    }
    
}
