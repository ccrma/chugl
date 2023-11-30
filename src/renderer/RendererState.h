#pragma once

#include "chugl_pch.h"

class Camera;
class Light;
class Scene;
class Mesh;
class Renderer;
class Texture;
class CHGL_Text;

// used to track state while rendering the scenegraph
class RendererState  
{
public:
	RendererState(Renderer* renderer) 
	: m_Renderer(renderer), m_ViewMat(1.0f), m_CameraTransform(1.0f), m_ViewPos(0.0f),
	m_Scene(nullptr), m_Camera(nullptr), m_SkyboxTexture(nullptr), m_PostProcessingEnabled(false)
	{
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
	Scene* GetScene() { return m_Scene; }
	Texture* GetSkyboxTexture() { return m_SkyboxTexture; }

	// PP ==================================================================
	bool IsPostProcessingEnabled() { return m_PostProcessingEnabled; }

	// Reset state =========================================================
	void Reset();

private:
	Renderer *m_Renderer;  // the renderer that owns this state

	std::vector<glm::mat4> m_ModelStack;  // used to track model transform hierarchy

	// opaque and transparent meshes
	std::vector<Mesh*> m_OpaqueMeshes;
	std::vector<Mesh*> m_TransparentMeshes;
	std::vector<CHGL_Text*> m_Texts;

	// cache camera data
	glm::mat4 m_ProjMat, m_ViewMat, m_CameraTransform;
	glm::vec3 m_ViewPos;

	// scene reference
	Scene* m_Scene;
	Camera* m_Camera;
	
	// cache envmap
	Texture* m_SkyboxTexture;
	
	// post processing
	bool m_PostProcessingEnabled;

public:
	std::vector<Mesh*>& GetOpaqueMeshes() { return m_OpaqueMeshes; }
	std::vector<Mesh*>& GetTransparentMeshes() { return m_TransparentMeshes; }
	std::vector<CHGL_Text*>& GetTexts() { return m_Texts; }
};
