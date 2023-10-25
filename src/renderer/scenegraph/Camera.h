#pragma once

#include "chugl_pch.h"
#include "SceneGraphObject.h"

enum CameraType : unsigned int {
	NONE = 0,
	PERSPECTIVE,
	ORTHO,
};

struct CameraParams {
	float aspect;
	float fov;
	float size;  // orthographic size (scales view volume while preserving ratio of widht to height)
	float nearPlane;
	float farPlane;
	CameraType type;
};

// for now compressing both types into a single object (bc can only have 1 camera anyways)
class Camera : public SceneGraphObject
{
public: // member vars
	CameraParams params;
public:
	Camera(
		// default perspective params
		float aspect = 4.0/3.0,  // viewWidth / viewHeight
		float fov = 45.0f, // fov in radians
		// default ortho params
		float size = 6.6f,
		// shared params
		float nearPlane = 0.1f,  
		float farPlane = 100.0f
	) : params({aspect, fov, size, nearPlane, farPlane, CameraType::PERSPECTIVE})
	{}
	bool IsCamera() override { return true;  }
	void SetPerspective() { params.type = CameraType::PERSPECTIVE; }
	void SetOrtho() { params.type = CameraType::ORTHO; }
	void SetClipPlanes(float n, float f) { params.nearPlane = n; params.farPlane = f; }
	void SetFOV(float fov) { params.fov = fov; }
	void SetSize(float size) { params.size = size; }
	glm::mat4 GetViewMatrix() { 
		return glm::inverse(GetWorldMatrix());
		// return GetInvModelMatrix(); 
	}
	CameraType GetMode() { return params.type;  }
	glm::mat4 GetProjectionMatrix();  // mat to transform from view space --> clip space 
	float GetClipNear() { return params.nearPlane; }
	float GetClipFar() { return params.farPlane; }
	float GetFOV() { return params.fov; }
	float GetSize() { return params.size; }

	Camera* Clone() override {
		Camera* c = new Camera();
		c->SetID(this->GetID());
		c->params = this->params;
		return c;
	}


	static const unsigned int MODE_PERSPECTIVE;
	static const unsigned int MODE_ORTHO;
};

/*
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
*/