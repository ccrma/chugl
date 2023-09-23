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

RenderMaterial::RenderMaterial(Material *mat, Renderer *renderer) : m_Mat(mat), m_Shader(nullptr), m_Renderer(renderer)
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
		case MaterialType::CustomShader:
			// until ChucK gets destructors, we default to default shader
			vertPath = ((ShaderMaterial*) mat)->m_VertShaderPath == "" ? "renderer/shaders/BasicLightingVert.glsl" : ((ShaderMaterial*) mat)->m_VertShaderPath;
			fragPath = ((ShaderMaterial*) mat)->m_FragShaderPath == "" ? "renderer/shaders/NormalFrag.glsl" : ((ShaderMaterial*) mat)->m_FragShaderPath;
			break;
		default:  // default material (normal mat for now)
			vertPath = "renderer/shaders/BasicLightingVert.glsl";
			fragPath = "renderer/shaders/NormalFrag.glsl";
	}

	// TODO: cache and share shader programs across render materials
	// TODO: add default shader (unity hot pink? or mango UV is better)
	m_Shader = new Shader(vertPath, fragPath);
}

void RenderMaterial::UpdateShader()
{
	if (!m_Mat) return;
	if (m_Mat->GetMaterialType() != MaterialType::CustomShader) return;

	// TODO: add hot reloading
	ShaderMaterial* mat = dynamic_cast<ShaderMaterial*>(m_Mat);
	if ( mat->m_VertShaderPath != m_Shader->GetVertPath() ||
		mat->m_FragShaderPath != m_Shader->GetFragPath()) {
		// Note: we DON'T delete the previous shader program because it may be in use by other render materials
		// Yes might leak, but you shouldn't be creating that many shaders anyways
		// long term fix: add ref counting, delete the shader if its linked to 0 render materials
		m_Shader = new Shader(mat->m_VertShaderPath, mat->m_FragShaderPath);
	}
}

RenderMaterial* RenderMaterial::GetDefaultMaterial(Renderer* renderer) {
	if (defaultMat == nullptr)
		defaultMat = new RenderMaterial(new NormalMaterial, renderer);

	return defaultMat;
}

void RenderMaterial::SetLocalUniforms()
{
	size_t textureCounter = 0;
	for (auto& it: m_Mat->m_Uniforms)
	{
		auto& uniform = it.second;
		Texture* rendererTexture;
		switch (uniform.type)
		{
			case UniformType::Float:
				m_Shader->setFloat(uniform.name, uniform.f);
				break;
			case UniformType::Float2:
				m_Shader->setFloat2(uniform.name, uniform.f2[0], uniform.f2[1]);
				break;
			case UniformType::Float3:
				m_Shader->setFloat3(uniform.name, uniform.f3[0], uniform.f3[1], uniform.f3[2]);
				break;
			case UniformType::Float4:
				m_Shader->setFloat4(uniform.name, uniform.f4[0], uniform.f4[1], uniform.f4[2], uniform.f4[3]);
				break;
			case UniformType::Int:
				m_Shader->setInt(uniform.name, uniform.i);
				break;
			case UniformType::Int2:
				m_Shader->setInt2(uniform.name, uniform.i2[0], uniform.i2[1]);
				break;
			case UniformType::Int3:
				m_Shader->setInt3(uniform.name, uniform.i3[0], uniform.i3[1], uniform.i3[2]);
				break;
			case UniformType::Int4:
				m_Shader->setInt4(uniform.name, uniform.i4[0], uniform.i4[1], uniform.i4[2], uniform.i4[3]);
				break;
			case UniformType::Bool:
				m_Shader->setBool(uniform.name, uniform.b);
				break;
			case UniformType::Texture:
				// bind texture first
				rendererTexture = m_Renderer->GetOrCreateTexture(uniform.texID);
				rendererTexture->Bind(textureCounter);
				// update GPU params if CGL_shader has been modified
				rendererTexture->Update();
				m_Shader->setInt(uniform.name, textureCounter);
				textureCounter++;
				break;
			default:
				throw std::runtime_error("Uniform type not supported");
		}
	}
}

void RenderMaterial::SetLightingUniforms(Scene *scene, const std::vector<Light *> &lights)
{
    // accumulators
    int numPointLights = 0;
    int numDirLights = 0;

    const std::string pointLightName = "u_PointLights";
    const std::string dirLightName = "u_DirLights";

    // loop over lights
    for (int i = 0; i < lights.size(); i++)
    {
        Light *light = lights[i];

        // skip if not a child of current scene
        if (!light || !light->BelongsToSceneObject(scene))
            continue;

        switch (light->GetLightType())
        {
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
	// glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClearColor(.9f, 0.9f, 0.9f, 1.0f);
	unsigned int clearBitfield = 0;
	if (color)
		clearBitfield |= GL_COLOR_BUFFER_BIT;
	if (depth)
		clearBitfield |= GL_DEPTH_BUFFER_BIT;
	GLCall(glClear(clearBitfield));
}

