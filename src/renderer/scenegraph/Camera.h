#pragma once

#include "SceneGraphObject.h"
#include "glm/glm.hpp"

enum class CameraType {
	NONE = 0,
	PERSPECTIVE,
	ORTHO,
};

class Camera : public SceneGraphObject
{
public:
	virtual glm::mat4 GetProjectionMatrix() = 0;  // mat to transform from view space --> clip space 
	virtual inline glm::mat4 GetViewMatrix() { return GetInvModelMatrix(); }
	virtual inline CameraType GetCameraType() { return CameraType::NONE;  }

	virtual Camera* Clone() = 0;

	virtual bool IsCamera() override { return true;  }

};

class PerspectiveCamera : public Camera
{
public:
	PerspectiveCamera(
		float aspect,  // viewWidth / viewHeight
		float fov = glm::radians(45.0f), // fov in radians 
		float near = 0.1f,  
		float far = 100.0f
	) : m_Aspect(aspect), m_Fov(fov), 
		m_Near(near), m_Far(far) 
		{};
	virtual Camera* Clone() override {
		return new PerspectiveCamera(m_Aspect, m_Fov, m_Near, m_Far);
	}
	virtual glm::mat4 GetProjectionMatrix() override {
		return glm::perspective(m_Fov, m_Aspect, m_Near, m_Far);
	}
	virtual inline CameraType GetCameraType() override { return CameraType::PERSPECTIVE;  }
	
	
	// TODO create getters/setters for camera params
	float m_Aspect, m_Fov, m_Near, m_Far;

};

class OrthoCamera : public Camera
{
public:
	OrthoCamera(
		float left = 0.0f, float right = 800.0f,
		float bottom = 0.0f, float top = 600.0f,
		float near = 0.1f, float far = 100.0f
	) : m_Left(left), m_Right(right), 
		m_Bottom(bottom), m_Top(top),
		m_Near(near), m_Far(far) 
		{};

	virtual Camera* Clone() override {
		return new OrthoCamera(m_Left, m_Right, m_Bottom, m_Top, m_Near, m_Far);
	}

	virtual glm::mat4 GetProjectionMatrix() override {
		return glm::ortho(
			m_Left, m_Right,
			m_Bottom, m_Top,
			m_Near, m_Far
		);
	};
	virtual inline CameraType GetCameraType() override { return CameraType::ORTHO; }

	// TODO: add zoom https://threejs.org/docs/index.html?q=camera#api/en/cameras/OrthographicCamera.zoom

	// TODO create getters/setters for these
	float m_Left, m_Right, m_Bottom, m_Top, m_Near, m_Far;
private:
};
