#include "Renderer.h"
#include "Shader.h"
#include "ShaderCode.h"
#include "VertexArray.h"

#include "scenegraph/Geometry.h"
#include "scenegraph/Light.h"


/* =============================================================================
								RenderGeometry	
===============================================================================*/

// setup VAO given populated vbo, ebo etc
void RenderGeometry::BuildGeometry() {
										 // set vao
	if (m_Geo->IsDirty()) {
		m_Geo->BuildGeometry();
		m_Geo->m_Dirty = false;
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
	GLCall(glVertexAttrib4f(Geometry::COLOR_ATTRIB_IDX, 1.0f, 1.0f, 1.0f, 1.0f));

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
				// GL_DYNAMIC_DRAW  // doesn't seem to make a difference
				// GL_STREAM_DRAW   // doesn't seem to make a difference
				// with all 3 modes, sndpeek with 1024 waterfall depth is 54 fps on andrew's macbook

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
	std::string vert, frag;
	bool vertIsPath = false, fragIsPath = false;
	// factory method to create the correct shader based on the material type
	// TODO just hardcode strings in cpp for builtin shaders
	switch(mat->GetMaterialType()) {
		case MaterialType::Normal:
			// TODO: really should abstract this to a shader resource locator class
			vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource(ShaderCode::NORMAL_FRAG, ShaderType::Fragment);
			break;
		case MaterialType::Flat:
			vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource(ShaderCode::FLAT_FRAG, ShaderType::Fragment);
			break;
		case MaterialType::Phong:
			vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource("PHONG_FRAG", ShaderType::Fragment);
			break;
		case MaterialType::CustomShader:
			// until ChucK gets destructors, we default to default shader
			vert = mat->GetVertShader();
			frag = mat->GetFragShader();
			vertIsPath = mat->GetVertIsPath();
			fragIsPath = mat->GetFragIsPath();
			if (vert == "") {
				vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
				vertIsPath = false;
			}
			if (frag == "") {
				frag = ShaderCode::GenShaderSource(ShaderCode::NORMAL_FRAG, ShaderType::Fragment);
				fragIsPath = false;
			}
			break;
		case MaterialType::Points:
			vert = ShaderCode::GenShaderSource("POINTS_VERT", ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource("POINTS_FRAG", ShaderType::Fragment);
			break;
		case MaterialType::Mango:
			vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource("MANGO_FRAG", ShaderType::Fragment);
			break;
		case MaterialType::Line:  // TODO: implement
			vert = ShaderCode::GenShaderSource("LINES_VERT", ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource("LINES_FRAG", ShaderType::Fragment);
			break;
		default:  // default material (normal mat for now)
			vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
			frag = ShaderCode::GenShaderSource(ShaderCode::NORMAL_FRAG, ShaderType::Fragment);
	}

	m_Shader = new Shader(vert, frag, vertIsPath, fragIsPath);
	mat->SetRecompileFlag(false);
	
}

void RenderMaterial::UpdateShader()
{
	if (!m_Mat) return;
	if (m_Mat->GetMaterialType() != MaterialType::CustomShader) return;

	// TODO: add hot reloading
	// TODO: fix this so that only changing either frag or vert will still work 
	// (right now) will try to load empty string for the other shader
	ShaderMaterial* mat = dynamic_cast<ShaderMaterial*>(m_Mat);
	if (!mat->RecompileShader()) return;

	// set up shader paths or codegen
	std::string vert = mat->GetVertShader();
	std::string frag = mat->GetFragShader();
	bool vertIsPath = mat->GetVertIsPath();
	bool fragIsPath = mat->GetFragIsPath();
	if (vert == "") {
		vert = ShaderCode::GenShaderSource(ShaderCode::BASIC_VERT, ShaderType::Vertex);
		vertIsPath = false;
	}
	if (frag == "") {
		frag = ShaderCode::GenShaderSource(ShaderCode::NORMAL_FRAG, ShaderType::Fragment);
		fragIsPath = false;
	}

	// Note: we DON'T delete the previous shader program because it may be in use by other render materials
	// Yes might leak, but you shouldn't be creating that many shaders anyways
	// long term fix: add ref counting, delete the shader if its linked to 0 render materials
	// actually deleting shouldn't be a problem now but still gotta figure 
	// out how to do it correctly
	// delete m_Shader;  // TODO
	m_Shader = new Shader(
		vert, frag, vertIsPath, fragIsPath
	);
	mat->SetRecompileFlag(false);
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
				// TODO: do we need to do any texture refcounting here?
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

void RenderMaterial::SetLightingUniforms(Scene *scene, const std::vector<size_t>& lights)
{
    // accumulators
    int numPointLights = 0;
    int numDirLights = 0;

    const std::string pointLightName = "u_PointLights";
    const std::string dirLightName = "u_DirLights";

    // loop over lights
    for (int i = 0; i < lights.size(); i++)
    {
        Light *light = (Light *)scene->GetNode(lights[i]);

        // skip if not a child of current scene
        if (!light || !light->BelongsToSceneObject(scene))
            continue;

        switch (light->GetLightType())
        {
        case LightType::Base:
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

// sets fog uniforms, assumes shader already bound
void RenderMaterial::SetFogUniforms(Scene *scene)
{
	// uniform names must match definitions in ShaderCode.cpp
	m_Shader->setFloat3("u_FogParams.color", scene->m_FogUniforms.color);
	m_Shader->setFloat("u_FogParams.density", scene->m_FogUniforms.density);
	m_Shader->setInt("u_FogParams.type", (int)scene->m_FogUniforms.type);
	m_Shader->setBool("u_FogParams.enabled", scene->m_FogUniforms.enabled);
}

/* =============================================================================
								Renderer	
===============================================================================*/

void Renderer::Clear(glm::vec3 bgCol, bool color, bool depth)
{	
	GLCall(glClearColor(bgCol.r, bgCol.g, bgCol.b, 1.0f));
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
			// if mac also set primitive, otherwise crashes (note point size is not supported, points are tiny!!)
			#ifdef __APPLE__
				primitive = GL_POINTS;
			#endif
			// set point size
			GLCall(glPointSize(CGL_mat->GetPointSize()));
			break;
		default:
			throw std::runtime_error("Polygon mode not set");
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

void Renderer::ProcessDeletionQueue(Scene *scene)
{
	auto& deletionQueue = scene->GetDeletionQueue();

	// loop over IDs
	for (auto& id : deletionQueue) {

		// delete render geometry
		if (DeleteRenderGeometry(id)) continue;

		// delete render material
		if (DeleteRenderMaterial(id)) continue;

		// delete texture
		if (DeleteTexture(id)) continue;
	}

	// clear queue
	scene->ClearDeletionQueue();
}

// deprecating this function for now, bc we want different materials of the 
// same type to actually use different shaders
// this has to do with different materials of the same type having different uniforms and attributes
// if they all use the same shader program, the shader will keep the same uniforms and attributes from past calls
// Shader *Renderer::GetOrCreateShader(
// 	const std::string &vert, const std::string &frag,
// 	bool vertIsPath, bool fragIsPath
// )
// {
// 	assert(false);
// 	ShaderKey key = std::make_pair(vert, frag);
// 	if (m_Shaders.find(key) != m_Shaders.end()) {
// 		return m_Shaders[key];
// 	}

// 	// not found, create it
// 	Shader* shader = new Shader(vert, frag, vertIsPath, fragIsPath);
// 	// cache it
// 	m_Shaders[key] = shader;
// 	return shader;
// }
