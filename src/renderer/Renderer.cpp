#include "Renderer.h"
#include "Util.h"
#include "Shader.h"
#include "VertexArray.h"
#include "scenegraph/Geometry.h"
#include "scenegraph/Light.h"
#include <glad/glad.h>
#include "ShaderCode.h"


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

	auto& attributes = GetAttributes();
	auto& indices = GetIndices();

	// disable unused vertex buffers 
	for (auto& it : m_VBs) {
		auto location = it.first;
		if (
			attributes.find(location) == attributes.end()
			||
			attributes[location].NumVertices() == 0
		) {
			// disable attribute
			va.RemoveAttribute(location);

		}
	}

	// quick hack to set default color
	GLCall(glVertexAttrib3f(Geometry::COLOR_ATTRIB_IDX, 1.0f, 1.0f, 1.0f));

	// set vbo size to # non-zero length attributes
	// int numNonZeroLengthAttribs = 0;
	// for (auto& it : attributes) {
	// 	auto& attrib = it.second;
	// 	if (attrib.NumVertices() > 0)
	// 		++numNonZeroLengthAttribs;
	// }
	// m_VBs.resize(numNonZeroLengthAttribs);

	// loop over attributes
	size_t lastNumVertices = 0;
	size_t i = 0;
	for (auto& it : attributes) {
		auto& attrib = it.second;
		if (attrib.NumVertices() == 0)  continue;  // skip over unused attributes

		// create new vbo if not already created
		if (m_VBs.find(attrib.location) == m_VBs.end()) {
			// using pointers otherwise destructor will be called when VertexBuffer goes out of scope,
			// freeing the GPU buffer and causing a crash
			// Maybe can do this with move semantics and move operators, but for now pointers are simpler and quicker
			m_VBs[attrib.location] = new VertexBuffer(
				(void*) attrib.data.data(),
				attrib.SizeInBytes(),
				attrib.NumVertices(),
				GL_STATIC_DRAW  // TODO test GL_DYNAMIC and GL_STREAM
			);
		} else {
			auto* vb = m_VBs[attrib.location];

			// check if num vertices has changed; if so, regenerate
			if (vb->GetCount() != attrib.NumVertices()) {
				vb->SetBuffer(
					(void*) attrib.data.data(),
					attrib.SizeInBytes(),
					attrib.NumVertices(),
					GL_STATIC_DRAW  // probably static? the actual buffer geometry shouldn't be modified too much
				);
			} else {  // else just update the buffer data
				vb->SubBuffer(
					(void*) attrib.data.data(),
					attrib.SizeInBytes(),
					0
				);
			}
		}

		// TODO: how do you "unbind" a buffer and attrib pointer from a VAO??

		// sanity check on num vertices
		if (i == 0) {
			lastNumVertices = attrib.NumVertices();
		}
		else if (i > 0 && attrib.NumVertices() != lastNumVertices) {
			fprintf(stderr, 
				"lastNumVertices: %zu\n attrib.NumVertices(): %zu\n for attribute: %s\n", 
				lastNumVertices, attrib.NumVertices(), attrib.name.c_str()
			);
			throw std::runtime_error(
				"RenderGeometry::BuildGeometry(): number of vertices in each attribute must match\n"
			);
		}

		++i;
		// add to VAO
		va.AddBufferAndLayout(m_VBs[attrib.location], attrib);  // add vertex attrib pointers to VAO state
	}

	// add indices
	if (indices.size() > 0)
	{
		IndexBuffer &ib = GetIndex();
		ib.SetBuffer(
			&indices[0],
			indices.size(),
			GL_STATIC_DRAW);
		va.AddIndexBuffer(ib); // add index buffer to VAO
	}
}

/* =============================================================================
								RenderMaterial
===============================================================================*/

// set static vars
RenderMaterial* RenderMaterial::defaultMat = nullptr;

RenderMaterial::RenderMaterial(Material *mat, Renderer *renderer) : m_Mat(mat), m_Shader(nullptr), m_Renderer(renderer)
{

	// testing shader codegen
	std::cerr << ShaderCode::GenShaderSource("BASIC_VERTEX_SHADER") << std::endl;



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
		case MaterialType::Points:
			vertPath = "./renderer/shaders/PointsVert.glsl";
			fragPath = "./renderer/shaders/PointsFrag.glsl";
			break;
		case MaterialType::Mango:
			vertPath = "./renderer/shaders/BasicLightingVert.glsl";
			fragPath = "./renderer/shaders/mangoFrag.glsl";
			break;
		case MaterialType::Line:  // TODO: implement
			vertPath = "./renderer/shaders/LineVert.glsl";
			fragPath = "./renderer/shaders/LineFrag.glsl";
			break;
		default:  // default material (normal mat for now)
			vertPath = "renderer/shaders/BasicLightingVert.glsl";
			fragPath = "renderer/shaders/NormalFrag.glsl";
	}

	m_Shader = renderer->GetOrCreateShader(vertPath, fragPath);
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
		m_Shader = m_Renderer->GetOrCreateShader(mat->m_VertShaderPath, mat->m_FragShaderPath);
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
	// glClearColor(.9f, 0.9f, 0.9f, 1.0f);
	glClearColor(.8f, 0.8f, 0.8f, 1.0f);
	unsigned int clearBitfield = 0;
	if (color)
		clearBitfield |= GL_COLOR_BUFFER_BIT;
	if (depth)
		clearBitfield |= GL_DEPTH_BUFFER_BIT;
	GLCall(glClear(clearBitfield));
}

void Renderer::Draw(RenderGeometry *renderGeo, RenderMaterial *renderMat)
{
	Shader* shader = renderMat->GetShader();
	shader->Bind();

	VertexArray& va = renderGeo->GetArray();
	va.Bind();

	Material* CGL_mat = renderMat->GetMat();

	// set polygon mode
	switch (CGL_mat->GetPolygonMode()) {
		case MaterialPolygonMode::Fill:
			GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
			break;
		case MaterialPolygonMode::Line:
			GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_LINE));
			// set line width
			GLCall(glLineWidth(CGL_mat->GetLineWidth()));
			break;
		case MaterialPolygonMode::Point:
			GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_POINT));
			// set point size
			GLCall(glPointSize(CGL_mat->GetPointSize()));
			break;
		default:
			throw std::runtime_error("Polygon mode not set");
	}


	GLenum primitive = GL_TRIANGLES;
	// set primitive mode
	switch (CGL_mat->GetPrimitiveMode()) {
		case MaterialPrimitiveMode::Triangles:
			primitive = GL_TRIANGLES;
			break;
		case MaterialPrimitiveMode::TriangleStrip:
			primitive = GL_TRIANGLE_STRIP;
			break;
		case MaterialPrimitiveMode::Points:
			primitive = GL_POINTS;
			break;
		case MaterialPrimitiveMode::Lines:
			primitive = GL_LINES;
			break;
		case MaterialPrimitiveMode::LineStrip:
			primitive = GL_LINE_STRIP;
			break;
		case MaterialPrimitiveMode::LineLoop:
			primitive = GL_LINE_LOOP;
			break;
		default:
			primitive = GL_TRIANGLES;
	}


	if (renderGeo->ShouldDrawIndexed()) {
		GLCall(glDrawElements(
			primitive,
			renderGeo->GetIndices().size(),  // length of index buffer
			GL_UNSIGNED_INT,   // type of index in EBO
			0  // offset
		));
	} else {
		GLCall(glDrawArrays(
			primitive,
			0,  // starting index
			renderGeo->GetNumVertices()  // number of VERTICES to render
		));
	}
}

Shader *Renderer::GetOrCreateShader(const std::string &vertPath, const std::string &fragPath)
{
	ShaderKey key = std::make_pair(vertPath, fragPath);
	if (m_Shaders.find(key) != m_Shaders.end()) {
		return m_Shaders[key];
	}

	// not found, create it
	Shader* shader = new Shader(vertPath, fragPath);
	// cache it
	m_Shaders[key] = shader;
	return shader;
}
