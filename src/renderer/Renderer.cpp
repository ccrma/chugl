#include "Renderer.h"
#include "Util.h"
#include "Shader.h"
#include "VertexArray.h"
#include "scenegraph/Geometry.h"
#include "scenegraph/Light.h"
#include <glad/glad.h>


/* =============================================================================
								RenderGeometry	
===============================================================================*/

// setup VAO given populated vbo, ebo etc
void RenderGeometry::BuildGeometry() {
										 // set vao
	if (m_Geo->IsDirty()) {
		m_Geo->BuildGeometry();
	}

	VertexArray& va = GetArray();
	va.Bind();

	auto& vertices = GetVertices();
	auto& indices = GetIndices();

	// set vbo
	VertexBuffer& vb = GetBuffer();
	vb.SetBuffer(
		(void*)&vertices[0],
		vertices.size() * sizeof(Vertex),  // size in bytes
		vertices.size(),  // num elements
		GL_STATIC_DRAW  // probably static? the actual buffer geometry shouldn't be modified too much
	);

	// set attributes
	auto& layout = GetLayout();
	layout.Push("position", GL_FLOAT, 3, false);  //
	layout.Push("normal", GL_FLOAT, 3, false);  // 
	layout.Push("uv", GL_FLOAT, 2, false);  //

											// set indices
	IndexBuffer& ib = GetIndex();
	ib.SetBuffer(
		&indices[0],
		(unsigned int) (3 * indices.size()),   // x3 because each index has 3 ints
		GL_STATIC_DRAW
	);

	// add to VAO
	va.AddBufferAndLayout(vb, layout);  // add vertex attrib pointers to VAO state
	va.AddIndexBuffer(ib);  // add index buffer to VAO 
}

/* =============================================================================
								RenderMaterial
===============================================================================*/

// set static vars
RenderMaterial* RenderMaterial::defaultMat = nullptr;

// statics

RenderMaterial* RenderMaterial::GetDefaultMaterial() {
	if (defaultMat == nullptr)
		defaultMat = new RenderMaterial(new NormalMaterial);

	return defaultMat;
}

void RenderMaterial::SetLightingUniforms(Scene* scene, const std::vector<Light*>& lights) {
		// accumulators
		int numPointLights = 0; 
		int numDirLights = 0;

		const std::string pointLightName = "u_PointLights";
		const std::string dirLightName = "u_DirLights";

		// loop over lights
		for (int i = 0; i < lights.size(); i++) {
			Light* light = lights[i];

			// skip if not a child of current scene
			if (!light || !light->BelongsToSceneObject(scene)) continue;

			switch (light->GetLightType()) {
				case LightType::None:
					throw std::runtime_error("Light type not set");
				case LightType::Point:
					light->SetShaderUniforms(m_Shader, numPointLights);
					++numPointLights;
					break;
				case LightType::Directional:
					light->SetShaderUniforms(m_Shader, numDirLights);
					++numDirLights;
					break;
				case LightType::Spot:
					throw std::runtime_error("Spot lights not implemented");
					break;
				default:
					throw std::runtime_error("Light type not set");
			}
		}

		// set count uniforms
		m_Shader->setInt("u_NumPointLights", numPointLights);
		m_Shader->setInt("u_NumDirLights", numDirLights);
	}




/* =============================================================================
								Renderer	
===============================================================================*/

void Renderer::Clear(bool color, bool depth)
{	
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	unsigned int clearBitfield = 0;
	if (color)
		clearBitfield |= GL_COLOR_BUFFER_BIT;
	if (depth)
		clearBitfield |= GL_DEPTH_BUFFER_BIT;
	GLCall(glClear(clearBitfield));
}

