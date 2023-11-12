#include "RendererState.h"
#include "scenegraph/Camera.h"
#include "scenegraph/Mesh.h"
#include "scenegraph/Scene.h"
#include "scenegraph/Material.h"

void RendererState::ComputeCameraUniforms(Camera* camera)
{
	m_Camera = camera;
	m_CameraTransform = camera->GetWorldMatrix();
	m_ViewMat = camera->GetViewMatrix();
	m_ViewPos = camera->GetWorldPosition();
	m_ProjMat = camera->GetProjectionMatrix();
}

void RendererState::PrepareScene(Scene *scene)
{
	m_Scene = scene; 

	// Update transparency comparator
	// UpdateTransparencyComparator(m_ViewPos);

	// walk scenegraph and collect opaque and transparent meshes
	static std::vector<SceneGraphObject *> queue;
	queue.push_back(scene);

	// DFS through graph
	while (!queue.empty())
	{
		SceneGraphObject *obj = queue.back();
		queue.pop_back();

		// add to mesh list if it's a mesh
		if (obj->IsMesh())
		{
			Mesh *mesh = dynamic_cast<Mesh*>(obj);
			Material* mat = mesh->GetMaterial();
			if (mat && mat->IsTransparent())
			{
				m_TransparentMeshes.push_back(mesh);
			}
			else
			{
				m_OpaqueMeshes.push_back(mesh);
			}
		}
		
		// TODO: can precompute world matrices here

		// add children to stack
		for (auto child : obj->GetChildren())
		{
			queue.push_back(child);
		}
	}

	// sort transparent meshes by distance from camera
	std::sort(
		m_TransparentMeshes.begin(), m_TransparentMeshes.end(), 
		[&](const Mesh* lhs, const Mesh* rhs) {
			// sort by distance from camera
			auto l_dist = glm::distance2(lhs->GetWorldPosition(), m_ViewPos);
			auto r_dist = glm::distance2(rhs->GetWorldPosition(), m_ViewPos);
			// farther away comes first, so that closer objects are rendered on top
			return l_dist > r_dist;
		}
	);

	// TODO sort opaque meshes front to back
}

void RendererState::Reset()
{
	m_ModelStack.clear();
	m_ModelStack.push_back(glm::mat4(1.0f));

	// clear mesh lists, but retain capacity
	// avoids reallocation cost but may cause memory problems
	int opaqueCapacity = m_OpaqueMeshes.capacity();
	m_OpaqueMeshes.clear();
	m_OpaqueMeshes.reserve(opaqueCapacity);

	int transparentCapacity = m_TransparentMeshes.capacity();
	m_TransparentMeshes.clear();
	m_TransparentMeshes.reserve(transparentCapacity);
}