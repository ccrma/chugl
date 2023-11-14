#pragma once

#include "chugl_pch.h"

#include "RendererState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "VertexArray.h"
#include "Util.h"
#include "ShaderCode.h"  // TODO remove after refactoring into cpp

#include "scenegraph/SceneGraphObject.h"
#include "scenegraph/Scene.h"
#include "scenegraph/Mesh.h"
#include "scenegraph/Camera.h"
#include "scenegraph/Material.h"
#include "scenegraph/Command.h"

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
	~RenderGeometry() {
		// at this point m_Geo should already be deleted

		// clear the buffer cache
		for (auto it = m_VBs.begin(); it != m_VBs.end(); ++it) {
			delete it->second;
		}
	}

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

	~RenderMaterial() {
		// at this point m_Mat should already be deleted
		delete m_Shader;
	}

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

	void SetLightingUniforms(Scene* scene, const std::vector<size_t>& lights); 
	void SetFogUniforms(Scene* scene);


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
public:  // framebuffer setup
	unsigned int m_FrameBufferID;
	unsigned int m_TextureColorbuffer;
	unsigned int m_RenderBufferID;
	Shader *m_ScreenShader;
	VertexArray* m_ScreenVA;
	VertexBuffer* m_ScreenPositionsVB;
	VertexBuffer* m_ScreenTexCoordsVB;

	void BuildFramebuffer(unsigned int width, unsigned int height);

	void UpdateFramebufferSize(unsigned int width, unsigned int height) {
		// TODO: check Khronos docs if it's okay to rebuild the framebuffer like this
		// update texture dimensions 
		glBindTexture(GL_TEXTURE_2D, m_TextureColorbuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		// update renderbuffer dimensions
		glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferID); 
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);  
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		// check if framebuffer is complete
		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	void BindFramebuffer() {
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));
	}

	void UnbindFramebuffer() {
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
	}

	void RenderScreen() {
		// disable depth test
		GLCall(glDisable(GL_DEPTH_TEST));
		// bind screen shader
		m_ScreenShader->Bind();
		// bind screen quad 
		m_ScreenVA->Bind();
		// bind texture
		GLCall(glActiveTexture(GL_TEXTURE0));  // activate slot 0
		GLCall(glBindTexture(GL_TEXTURE_2D, m_TextureColorbuffer));
		// set polygon mode to fill
		GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
		
		// TODO add culling?

		// draw
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 6));
	}

public: // skybox envmap vars
	static const float SKYBOX_VERTICES[];
	// sadly these need to be pointers 
	// because OpenGL context is not initialized until window is created
	// TODO: free these in destructor
	// better fix: don't initialize buffers in default constructor
	VertexArray* m_SkyboxVA;
	VertexBuffer* m_SkyboxVB;
	Shader m_SkyboxShader;
	CubeMapTexture m_SkyboxTexture;

public:
	void LoadSkyboxTexture(const std::vector<std::string>& faces) {
		m_SkyboxTexture.Load(faces);
	}

	void BuildSkybox() {
		m_SkyboxVA = new VertexArray();
		m_SkyboxVB = new VertexBuffer();
		// setup skybox geometry
		m_SkyboxVB->SetBuffer(SKYBOX_VERTICES, sizeof(float) * 36 * 3, 36, GL_STATIC_DRAW);
		m_SkyboxVA->AddBufferAndLayout(m_SkyboxVB, {
			CGL_GeoAttribute("a_Position", 0, 3)
		});

		// setup skybox shader
		const std::string& skyboxVert = ShaderCode::SKYBOX_VERT_CODE;
		const std::string& skyboxFrag = ShaderCode::SKYBOX_FRAG_CODE;

		m_SkyboxShader.Compile(skyboxVert, skyboxFrag, false, false);
	}

	void Clear(glm::vec3 bgCol, bool color = true, bool depth = true);
	void Draw(RenderGeometry* renderGeo, RenderMaterial* renderMat);

	// Rendering =======================================================================
	// TODO add cacheing for world matrices
	void RenderScene(Scene* scene, Camera* camera = nullptr) {
		assert(scene->IsScene());

		// enable depth testing
		GLCall(glEnable(GL_DEPTH_TEST));	

		// optionally change the camera to render from
		if (camera) {
			m_MainCamera = camera;
		}

		// clear the render state
		m_RenderState.Reset();

		// cache camera values
		m_RenderState.ComputeCameraUniforms(m_MainCamera);

		// cache renderables from target scene
		m_RenderState.PrepareScene(scene);

		OpaquePass();
		TransparentPass();

		// draw skybox last to avoid overdraw
		SkyboxPass();

		// OLD: render in DFS order
		// now we process the scenegraph into a render queue
		// RenderNodeAndChildren(scene);
	}

	void SkyboxPass() {
		// early out if no skybox
		if (!m_SkyboxTexture.IsLoaded()) { return; }

		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content

		// Flatten depth range to far plane
		// ref: https://gamedev.stackexchange.com/questions/83739/how-do-i-ensure-my-skybox-is-always-in-the-background-with-opengl
		// GLCall(glDepthRange(0.99f, 1.0f));

		// bind
		m_SkyboxVA->Bind();
		m_SkyboxShader.Bind();
		m_SkyboxTexture.Bind(0);

		// set shader uniforms (TODO can refactor to better uniform management system)
		m_SkyboxShader.setMat4f("u_Projection", m_RenderState.GetProjMat());
		m_SkyboxShader.setMat4f("u_View", 
			// remove translation component from view matrix
			// so that skybox is always centered around camera
			glm::mat4(glm::mat3(
				m_RenderState.GetViewMat()
			))
		);

		// draw
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));

		// restore depth range
		// GLCall(glDepthRange(0.0f, 1.0f));
		glDepthFunc(GL_LESS);  // restore default depth func
	}

	// render opaque meshes
	void OpaquePass() {
		for (auto* mesh : m_RenderState.GetOpaqueMeshes()) {
			// TODO: access cached world matrix here
			RenderMesh(mesh, mesh->GetWorldMatrix());
		}
	}

	void TransparentPass() {
		// disable depth writes
		glDepthMask(GL_FALSE);

		// transparent meshes are already sorted during insertion
		// don't need to sort here

		// render transparent meshes
		for (auto* mesh : m_RenderState.GetTransparentMeshes()) {
			RenderMesh(mesh, mesh->GetWorldMatrix());
		}

		// reenable depth writes
		glDepthMask(GL_TRUE);
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
			// std::cout << "rendergeo dirty, rebuilding\n";
			renderGeo->BuildGeometry();
		}

		// lookup or create render material
		RenderMaterial* renderMat = GetOrCreateRenderMat(mat);
		// TODO add a dirty check?

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
		renderMat->SetFogUniforms(m_RenderState.GetScene());
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

	// deprecating for now
	// sharing shaders between materials requires Materials to know how to unset uniforms/attributes
	// TODO: come back to this when trying to implement batching, how to accomodate uniforms...
	// Shader* GetOrCreateShader(
	// 	const std::string& vertPath, const std::string& fragPath,
	// 	bool vertIsPath, bool fragIsPath
	// );

public:  // memory management
	bool DeleteRenderGeometry(size_t ID) {
		if (m_RenderGeometries.find(ID) != m_RenderGeometries.end()) {
			delete m_RenderGeometries[ID];
			m_RenderGeometries.erase(ID);
			return true;
		}
		return false;
	}

	bool DeleteRenderMaterial(size_t ID) {
		if (m_RenderMaterials.find(ID) != m_RenderMaterials.end()) {
			delete m_RenderMaterials[ID];
			m_RenderMaterials.erase(ID);
			return true;
		}
		return false;
	}

	bool DeleteTexture(size_t ID) {
		if (m_Textures.find(ID) != m_Textures.end()) {
			delete m_Textures[ID];
			m_Textures.erase(ID);
			return true;
		}
		return false;
	}

	void ProcessDeletionQueue(Scene* scene);


private:  // private member vars
	RendererState m_RenderState;
	Camera* m_MainCamera;

	// GPU resources, map from SceneGraphNode ID to Renderer resource
	std::unordered_map<size_t, RenderGeometry*> m_RenderGeometries;
	std::unordered_map<size_t, RenderMaterial*> m_RenderMaterials;
	std::unordered_map<size_t, Texture*> m_Textures;


	// shader cache
	// typedef std::pair<std::string, std::string> ShaderKey;
	// std::unordered_map<ShaderKey, Shader*, Util::hash_pair> m_Shaders;
};

