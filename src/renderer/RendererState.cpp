#include "RendererState.h"
#include "scenegraph/Camera.h"

void RendererState::ComputeCameraUniforms(Camera* camera)
{
	m_CameraTransform = camera->GetWorldMatrix();
	m_ViewMat = camera->GetViewMatrix();
	m_ViewPos = m_CameraTransform * glm::vec4(camera->GetPosition(), 1.0f);
	m_ProjMat = camera->GetProjectionMatrix();
}
