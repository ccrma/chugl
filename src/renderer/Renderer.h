#pragma once

#include "RendererState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Util.h"

#include "scenegraph/SceneGraphObject.h"
#include "scenegraph/Scene.h"
#include "scenegraph/Mesh.h"
#include "scenegraph/Camera.h"
#include "scenegraph/Material.h"
#include "scenegraph/Command.h"

#include <mutex>
#include <vector>
#include <unordered_map>

class Shader;
class Geometry;
class Renderer;


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
	bool ShouldDrawIndexed() { return m_Geo->m_Indices.size() > 0;}
	void BuildGeometry();
	unsigned int GetNumVertices() { return m_Geo->m_Attributes[Geometry::POSITION_ATTRIB_IDX].NumVertices(); }


	// CPU side data
	std::vector<unsigned int>& GetIndices() { return m_Geo->m_Indices; }
	Geometry::AttributeMap& GetAttributes() { return m_Geo->m_Attributes; }

	// GPU buffers
	typedef std::unordered_map<unsigned int, VertexBuffer*> VertexBufferCache;
	VertexBufferCache& GetBuffers() { return m_VBs; }
	VertexArray& GetArray() { return m_VA; }
	IndexBuffer& GetIndex() { return m_IB; }
private:
	Geometry* m_Geo;
    
	// GPU buffer data
    VertexArray m_VA;
	VertexBufferCache m_VBs;
	IndexBuffer m_IB;
};

struct GlobalUniforms {
	// MVP matrices
	glm::mat4 u_Model, u_View, u_Projection;

	// normals
	glm::mat4 u_Normal;

	// camera
	glm::vec3 u_ViewPos;
};

// Manages GPU side data for a material
class RenderMaterial {
public:
	RenderMaterial(Material* mat, Renderer* renderer);

	~RenderMaterial() {}

	// CPU side data
	std::string GetVertPath() { return m_Shader->GetVertPath(); }
	std::string GetFragPath() { return m_Shader->GetFragPath(); }
	Material* GetMat() { return m_Mat; }

	// GPU side data
	Shader* GetShader() { return m_Shader; }
	void UpdateShader();
	void BindShader() { m_Shader->Bind(); }
	void SetLocalUniforms();
	void SetGlobalUniforms(const GlobalUniforms& globals) {

		m_Shader->setMat4f("u_Model", globals.u_Model);
		m_Shader->setMat4f("u_View", globals.u_View);
		m_Shader->setMat4f("u_Projection", globals.u_Projection);
		m_Shader->setMat4f("u_Normal", globals.u_Normal);
		m_Shader->setFloat3("u_ViewPos", globals.u_ViewPos);
	}

	void SetLightingUniforms(Scene* scene, const std::vector<Light*>& lights); 


public:  // statics
	static RenderMaterial* GetDefaultMaterial(Renderer* renderer);
	static RenderMaterial* defaultMat;

private:
	Material* m_Mat;
	Shader* m_Shader;
	Renderer* m_Renderer;

};




class Renderer
{
public:
	void Clear(bool color = true, bool depth = true);
	void Draw(RenderGeometry* renderGeo, RenderMaterial* renderMat);




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

		RenderNodeAndChildren(scene);
	}

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

		// update shader program if changed
		renderMat->UpdateShader();

		// set uniforms
		renderMat->BindShader();
		renderMat->SetLocalUniforms();
		renderMat->SetGlobalUniforms({
			worldTransform,
			m_RenderState.GetViewMat(),
			m_RenderState.GetProjMat(),
			glm::transpose(glm::inverse(worldTransform)),  // TODO cache this normal matrix or move to math util library
			m_RenderState.GetViewPos()
		});
		renderMat->SetLightingUniforms(m_RenderState.GetScene(), m_RenderState.GetScene()->m_Lights);

		// draw
		Draw(renderGeo, renderMat);

		// std::cerr << " num textures: " << m_Textures.size() << std::endl;
		// std::cerr << " num geometries: " << m_RenderGeometries.size() << std::endl;
		// std::cerr << " num rendermaterials: " << m_RenderMaterials.size() << std::endl;
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
			return RenderMaterial::GetDefaultMaterial(this);
		}

		size_t ID = mat->GetID();
		if (m_RenderMaterials.find(ID) != m_RenderMaterials.end()) {
			return m_RenderMaterials[ID];
		}

		// not found, create it
		RenderMaterial* renderMat = new RenderMaterial(mat, this);
		// cache it
		m_RenderMaterials[ID] = renderMat;
		return renderMat;
	}

	Texture* GetOrCreateTexture(size_t ID) {
		CGL_Texture* tex = dynamic_cast<CGL_Texture*>(m_RenderState.GetScene()->GetNode(ID));
		if (!tex)  // no scenegraph texture, use default
			return Texture::GetDefaultWhiteTexture();
		
		if (m_Textures.find(ID) != m_Textures.end()) {
			return m_Textures[ID];
		}

		// not found, create it
		Texture* texture = new Texture(tex);
		// cache it
		m_Textures[ID] = texture;
		return texture;
	}

	Shader* GetOrCreateShader(const std::string& vertPath, const std::string& fragPath);

private:  // private member vars
	RendererState m_RenderState;
	Camera* m_MainCamera;

	// GPU resources, map from SceneGraphNode ID to Renderer resource
	std::unordered_map<size_t, RenderGeometry*> m_RenderGeometries;
	std::unordered_map<size_t, RenderMaterial*> m_RenderMaterials;
	std::unordered_map<size_t, Texture*> m_Textures;
	// shader cache
	typedef std::pair<std::string, std::string> ShaderKey;
	std::unordered_map<ShaderKey, Shader*, Util::hash_pair> m_Shaders;


};

