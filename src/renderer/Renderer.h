#pragma once

#include "RendererState.h"
#include "scenegraph/SceneGraphObject.h"
#include "scenegraph/Scene.h"
#include "scenegraph/Mesh.h"
#include "scenegraph/Camera.h"
#include "scenegraph/Material.h"
#include "scenegraph/Command.h"

#include <mutex>
#include <vector>
#include <unordered_map>

class VertexArray;
class Shader;
class Geometry;


// Renderer helper classes (encapsulate SceneGraph classes)
/*
This encapsulation is to decouple the scenegraph objects from any
graphics-specific implementation code. 
E.g. the Geometry scenegraph node only contains CPU-side vertex data,
and does NOT know about any GPU-side vertex buffers, etc.

The renderer is responsible for creating and managing the GPU-side

This way the same CGL SceneGraph can be implemented by variety of rendering backends,
including OpenGL, WebGPU, Vulkan, etc.
*/

// Given a Geometry object, this class manages the GPU-side vertex buffers
class RenderGeometry {
public:
	RenderGeometry(Geometry* geo) : m_Geo(geo) {
		BuildGeometry();
	}
	~RenderGeometry() {}

    inline void Bind() { m_VA.Bind(); }  // bind the underlying geometry
	inline bool IsDirty() { return m_Geo->IsDirty(); }
	void BuildGeometry();


	// CPU side data
	std::vector<Index>& GetIndices() { return m_Geo->m_Indices; }
	std::vector<Vertex>& GetVertices() { return m_Geo->m_Vertices; }

	// GPU buffers
	VertexBufferLayout& GetLayout() { return m_Layout; }
	VertexBuffer& GetBuffer() { return m_VB; }
	VertexArray& GetArray() { return m_VA; }
	IndexBuffer& GetIndex() { return m_IB; }
private:
	Geometry* m_Geo;
    
	// GPU buffer data
    VertexArray m_VA;
    VertexBuffer m_VB;
	IndexBuffer m_IB;
    VertexBufferLayout m_Layout;
};

struct GlobalUniforms {
	glm::mat4 u_Model, u_View, u_Projection;

	// normals
	glm::mat4 u_Normal;

	// camera
	glm::vec3 u_ViewPos;

	// time
	float u_Time;
};

// Manages GPU side data for a material
class RenderMaterial {
public:
	RenderMaterial(Material* mat) : m_Mat(mat), m_Shader(nullptr) 
	{	
		std::string vertPath, fragPath;
		// factory method to create the correct shader based on the material type
		// TODO just hardcode strings in cpp for builtin shaders
		switch(mat->GetMaterialType()) {
			case MaterialType::Normal:
				// TODO: really should abstract this to a shader resource locator class
				vertPath = "renderer/shaders/BasicLightingVert.glsl";
				fragPath = "renderer/shaders/NormalFrag.glsl";
				break;
			case MaterialType::Phong:
 				vertPath = "./renderer/shaders/BasicLightingVert.glsl";
				fragPath = "./renderer/shaders/BasicLightingFrag.glsl";
				break;
			default:  // default material (normal mat for now)
				vertPath = "renderer/shaders/BasicLightingVert.glsl";
				fragPath = "renderer/shaders/NormalFrag.glsl";
		}

		m_Shader = new Shader(vertPath, fragPath);

	}
	~RenderMaterial() {}

	// CPU side data
	std::string GetVertPath() { return m_Shader->GetVertPath(); }
	std::string GetFragPath() { return m_Shader->GetFragPath(); }
	Material* GetMat() { return m_Mat; }

	// GPU side data
	Shader* GetShader() { return m_Shader; }
	virtual void BindShader() { m_Shader->Bind(); }
	void SetLocalUniforms() {

		m_Mat->SetLocalUniforms(m_Shader);  // TODO eventually move this logic out of Material and into RenderMaterial
	}
	void SetGlobalUniforms(const GlobalUniforms& globals) {

		m_Shader->setMat4f("u_Model", globals.u_Model);
		m_Shader->setMat4f("u_View", globals.u_View);
		m_Shader->setMat4f("u_Projection", globals.u_Projection);
		m_Shader->setMat4f("u_Normal", globals.u_Normal);
		m_Shader->setFloat3("u_ViewPos", globals.u_ViewPos);
		m_Shader->setFloat("u_Time", globals.u_Time);
	}

	void SetLightingUniforms(Scene* scene, const std::vector<Light*>& lights); 


public:  // statics
	static RenderMaterial* GetDefaultMaterial();
	static RenderMaterial* defaultMat;

private:
	Material* m_Mat;
	Shader* m_Shader;

	// texture list
	std::list<Texture*> m_Textures;


};




class Renderer
{
public:
	void Clear(bool color = true, bool depth = true);
	void Draw(RenderGeometry* renderGeo, RenderMaterial* renderMat) {
		Shader* shader = renderMat->GetShader();
		shader->Bind();

		VertexArray& va = renderGeo->GetArray();
		va.Bind();

		// wireframe
		if (renderMat->GetMat()->GetWireFrame()) {
			GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
		} else {
			GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		}

		if (va.GetIndexBuffer() == nullptr) {
			GLCall(glDrawArrays(
				GL_TRIANGLES,
				0,  // starting index
				va.GetVertexBuffer()->GetCount()
			));
		}
		else
		{
			GLCall(glDrawElements(
				GL_TRIANGLES,
				va.GetIndexBufferCount(),
				GL_UNSIGNED_INT,   // type of index in EBO
				0  // offset
			));
		}
	}




	// Rendering =======================================================================
	// TODO add cacheing for world matrices
	void RenderScene(Scene* scene, Camera* camera = nullptr) {
		assert(scene->IsScene());

		// optionally change the camera to render from
		if (camera) {
			m_MainCamera = camera;
		}

		// clear the render state
		m_RenderState.Reset();

		// cache camera values
		m_RenderState.ComputeCameraUniforms(m_MainCamera);

		// cache current scene being rendered
		m_RenderState.SetScene(scene);
		
		// TODO set globals (lighting)
		RenderNodeAndChildren(scene);
	}

private:  // private methods
	void RenderNodeAndChildren(SceneGraphObject* sgo) {
		// TODO add matrix caching
		glm::mat4 worldTransform = m_RenderState.GetTopTransform() * sgo->GetModelMatrix();
		// glm::mat4 worldTransform = sgo->GetModelMatrix();

		// if its drawable, draw it
		// Note: this allows us to keep the actual render logic and graphics API calls OUT of the SceneGraphObjects
		// the scenegraph structure and data is decoupled from the rendering.
		// scenegraph just provides data; renderer parses and figures out what to draw
		// this decoupling allows supporting multiple renderers in the future
		if (sgo->IsMesh()) {
			RenderMesh(dynamic_cast<Mesh*>(sgo), worldTransform);
		}
		
		// add transform state
		m_RenderState.PushTransform(worldTransform);

		// recursively render children (TODO: should do this inplace in case of deeeeep scenegraphs blowing the stack)
		for (const auto& child : sgo->GetChildren()) {
			RenderNodeAndChildren(child);
		}

		// done rendering children, pop last transform from stack
		m_RenderState.PopTransform();
	}
	
	// TODO change to render command for batch rendering
	void RenderMesh(Mesh* mesh, glm::mat4 worldTransform) {
		Geometry* geo = mesh->GetGeometry();
		Material* mat = mesh->GetMaterial();

		// if no geometry nothing to draw
		if (!geo) { return; }


		// lookup or create render geometry
		RenderGeometry* renderGeo = GetOrCreateRenderGeo(geo);
		// check dirty TODO should prob move this elsewhere
		if (renderGeo->IsDirty()) {
			std::cout << "rendergeo dirty, rebuilding\n";
			renderGeo->BuildGeometry();
		}

		// lookup or create render material
		RenderMaterial* renderMat = GetOrCreateRenderMat(mat);

		// set uniforms
		renderMat->BindShader();
		renderMat->SetLocalUniforms();
		renderMat->SetGlobalUniforms({
			worldTransform,
			m_RenderState.GetViewMat(),
			m_RenderState.GetProjMat(),
			glm::transpose(glm::inverse(worldTransform)),  // TODO cache this normal matrix or move to math util library
			m_RenderState.GetViewPos(),
			0.0f // time
		});
		renderMat->SetLightingUniforms(m_RenderState.GetScene(), m_RenderState.GetScene()->m_Lights);

		// draw
		Draw(renderGeo, renderMat);
	}

	RenderGeometry* GetOrCreateRenderGeo(Geometry* geo) {
		size_t ID = geo->GetID();
		if (m_RenderGeometries.find(ID) != m_RenderGeometries.end()) {
			return m_RenderGeometries[ID];
		}

		// not found, create it
		RenderGeometry* renderGeo = new RenderGeometry(geo);
		// cache it
		m_RenderGeometries[ID] = renderGeo;
		return renderGeo;
	}

	RenderMaterial* GetOrCreateRenderMat(Material* mat) {
		if (mat == nullptr) { // return default material
			return RenderMaterial::GetDefaultMaterial();
		}

		size_t ID = mat->GetID();
		if (m_RenderMaterials.find(ID) != m_RenderMaterials.end()) {
			return m_RenderMaterials[ID];
		}

		// not found, create it
		RenderMaterial* renderMat = new RenderMaterial(mat);
		// cache it
		m_RenderMaterials[ID] = renderMat;
		return renderMat;
	}



private:  // private member vars
	RendererState m_RenderState;
	Camera* m_MainCamera;

	// GPU resources
	// std::unordered_map<Geometry*, RenderGeometry*> m_RenderGeometries;
	std::unordered_map<size_t, RenderGeometry*> m_RenderGeometries;
	// std::unordered_map<Material*, RenderMaterial*> m_RenderMaterials;
	std::unordered_map<size_t, RenderMaterial*> m_RenderMaterials;

};

