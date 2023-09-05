#pragma once
#include <vector>
#include "glm/glm.hpp"

class Camera;

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

	void Reset() {
		m_ModelStack.clear();
		m_ModelStack.push_back(glm::mat4(1.0f));
	}
private:
	std::vector<glm::mat4> m_ModelStack;  // used to track model transform hierarchy

	// cache camera data
	glm::mat4 m_ProjMat, m_ViewMat, m_CameraTransform;
	glm::vec3 m_ViewPos;


};
