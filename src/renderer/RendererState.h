#pragma once

#include "chugl_pch.h"

class Camera;
class Light;
class Scene;
class Mesh;

// used to track state while rendering the scenegraph
class RendererState  
{
public:
	RendererState() : m_ViewMat(1.0f), m_CameraTransform(1.0f), m_ViewPos(0.0f) { 
		m_ModelStack.push_back(glm::mat4(1.0f)); 
	};

	// Tracking transform stack ============================================
	inline void PushTransform(const glm::mat4& transform) { m_ModelStack.push_back(transform); };
	inline void PopTransform() { m_ModelStack.pop_back(); };
	inline const glm::mat4& GetTopTransform() const { return m_ModelStack.back(); }

	// Camera stuff ========================================================
	void ComputeCameraUniforms(Camera* camera);
	inline glm::mat4 GetViewMat() { return m_ViewMat;  }
	inline glm::mat4 GetProjMat() { return m_ProjMat;  }
	inline glm::mat4 GetCameraTransform() { return m_CameraTransform;  }
	inline glm::vec3 GetViewPos() { return m_ViewPos;  }

	// Scene stuff =========================================================
	void PrepareScene(Scene* scene);
	inline Scene* GetScene() { return m_Scene; }

	// Reset state =========================================================
	void Reset();

private:
	std::vector<glm::mat4> m_ModelStack;  // used to track model transform hierarchy

	// opaque and transparent meshes
	std::vector<Mesh*> m_OpaqueMeshes;
	std::vector<Mesh*> m_TransparentMeshes;

	// cache camera data
	glm::mat4 m_ProjMat, m_ViewMat, m_CameraTransform;
	glm::vec3 m_ViewPos;

	// scene reference
	Scene* m_Scene;
	Camera* m_Camera;

public:
	std::vector<Mesh*>& GetOpaqueMeshes() { return m_OpaqueMeshes; }
	std::vector<Mesh*>& GetTransparentMeshes() { return m_TransparentMeshes; }
};
