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
				GL_STATIC_DRAW 
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

RenderMaterial::RenderMaterial(Material *mat, Renderer *renderer) 
	: m_Mat(mat), m_Shader(nullptr), m_Renderer(renderer), m_TextureCounter(0)
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
	for (auto& it: m_Mat->GetLocalUniforms())
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
				rendererTexture->Bind(m_TextureCounter);
				// update GPU params if CGL_shader has been modified
				rendererTexture->Update();
				// set uniform
				SetTextureUniform(uniform.name);
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
							Renderer Static 
===============================================================================*/

const float Renderer::SKYBOX_VERTICES[] = {
	// positions          
    -1.0f,  1.0f, -1.0f,
    -1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f, -1.0f,  1.0f,
    -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
     1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f,  1.0f,
    -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f, -1.0f,  1.0f,
     1.0f, -1.0f,  1.0f
};

/* =============================================================================
								Renderer	
===============================================================================*/

void Renderer::BuildFramebuffer(unsigned int width, unsigned int height) {
	m_ScreenVA = new VertexArray();
	m_ScreenPositionsVB = new VertexBuffer();
	m_ScreenTexCoordsVB = new VertexBuffer();

	m_FrameBufferPing = new FrameBuffer(width, height, true, false);
	m_FrameBufferPong = new FrameBuffer(width, height, true, false);
	m_SceneFrameBufferMS = new FrameBuffer(width, height, true, true);
	// m_SceneFrameBufferMS = new FrameBuffer(width, height, false);

	// setup screen triangle (in ndc)
	// We can cover the entire screen with a single triangle
	// requires only 3 vertices instead of 6, and prevents redundant fragment processing
	// along pixels that would bridge the edge between two triangles in a quad
	// See the link below for more info
	// https://catlikecoding.com/unity/tutorials/custom-srp/post-processing/

	float positions[] = {
		-1.0f, 3.0f, 0.0f,   // top left
		-1.0f, -1.0f, 0.0f,  // bottom left
		 3.0f, -1.0f, 0.0f,  // bottom right 
	};
	float texCoords[] = {
		0.0f, 2.0f,  // top left
		0.0f, 0.0f,  // bottom left
		2.0f, 0.0f,  // bottom right 
	};

	m_ScreenPositionsVB->SetBuffer(
		positions, 
		sizeof(positions), 
		3,  // num vertices
		GL_STATIC_DRAW
	);
	m_ScreenTexCoordsVB->SetBuffer(
		texCoords, 
		sizeof(texCoords), 
		3,  // num vertices
		GL_STATIC_DRAW
	);

	m_ScreenVA->AddBufferAndLayout(
		m_ScreenPositionsVB,
		CGL_GeoAttribute("a_Position", 0, 3)
	);

	m_ScreenVA->AddBufferAndLayout(
		m_ScreenTexCoordsVB,
		CGL_GeoAttribute("a_TexCoord", 1, 2)
	);

	// setup screen shader
	const std::string& screenShaderVert = ShaderCode::PP_VERT;
	const std::string& screenShaderFrag = ShaderCode::PP_PASS_THROUGH;

	auto* passThroughShader = new Shader(
		screenShaderVert,
		screenShaderFrag,
		false, false
	);
}

void Renderer::UpdateFramebufferSize(unsigned int width, unsigned int height)
{
	// Update member vars
	m_ViewportWidth = width;
	m_ViewportHeight = height;

	// TODO: propagate to post processors
	// loop over all effects in map
	for (auto& it : m_Effects) {
		auto* effect = it.second;
		effect->Resize(width, height);
	}

	// Update framebuffers
	if (m_FrameBufferPing) m_FrameBufferPing->UpdateSize(width, height);
	if (m_FrameBufferPong) m_FrameBufferPong->UpdateSize(width, height);
	if (m_SceneFrameBufferMS) m_SceneFrameBufferMS->UpdateSize(width, height);
}

void Renderer::PostProcessPass()
{
	Scene* scene = m_RenderState.GetScene();
	PP::Effect* chugl_root_effect = scene->GetRootEffect();

	// root should always exist
	assert(chugl_root_effect);

	// get root effect
	PostProcessEffect* rootEffect = GetOrCreateEffect(chugl_root_effect->GetID());

	// early out if no active post processing effects
	if (!rootEffect || !rootEffect->GetChuglEffect()->NextEnabled()) return;
	
	// disable depth test
	GLCall(glDisable(GL_DEPTH_TEST));
	// bind screen quad 
	m_ScreenVA->Bind();
	// set polygon mode to fill
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
	// face culling on the screen mesh so that only front faces are drawn
	GLCall(glEnable(GL_CULL_FACE));
	GLCall(glCullFace(GL_BACK));


	// step through post processing effects
	bool ping = true;  // start with reading from ping buffer
	int effectCount = 0;
	PP::Effect* chugl_effect = rootEffect->GetChuglEffect()->NextEnabled();

	// for first effect, blit from MSAA framebuffer to read frame buffer
	auto& writeFrameBuffer = GetReadFrameBuffer(ping);
	assert(writeFrameBuffer.GetWidth() == m_SceneFrameBufferMS->GetWidth());
	assert(writeFrameBuffer.GetHeight() == m_SceneFrameBufferMS->GetHeight());
	GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, m_SceneFrameBufferMS->GetID()));
	GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, writeFrameBuffer.GetID()));
	GLCall(
		glBlitFramebuffer(
			0, 0, 
			m_SceneFrameBufferMS->GetWidth(), m_SceneFrameBufferMS->GetHeight(), 
			0, 0, 
			writeFrameBuffer.GetWidth(), writeFrameBuffer.GetHeight(),
			GL_COLOR_BUFFER_BIT, 
			// GL_LINEAR
			GL_NEAREST
		)
	);

	while (chugl_effect) {
		PostProcessEffect* effect = GetOrCreateEffect(chugl_effect->GetID());
		// std::cout << "effect num: " << effectCount++ << std::endl;
		// if last effect, unbind framebuffer
		unsigned int writeFrameBufferID {0};
		if (!chugl_effect->NextEnabled()) {
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); 
		} else {
			// bind framebuffer
			GetWriteFrameBuffer(ping).Bind();
			writeFrameBufferID = GetWriteFrameBuffer(ping).GetID();
		}

		// bind color attachment from previous effect
		auto& readFramebuffer = GetReadFrameBuffer(ping);
		readFramebuffer.BindColorAttachment();

		// clear
		Clear(
			m_RenderState.GetScene()->GetBackgroundColor(),
			true,   // clear color buffer
			false   // don't clear depth buffer
		);

		// bind screen shader
		effect->Apply(*this, readFramebuffer.GetColorBufferID(), writeFrameBufferID);  
		// draw
		GLCall(glDrawArrays(GL_TRIANGLES, 0, 3));

		// move on to next effect
		chugl_effect = chugl_effect->NextEnabled();
		// flip ping pong buffer
		ping = !ping;
		// incr effect count
		++effectCount;
	}

	// disable face fulling
	GLCall(glDisable(GL_CULL_FACE));
}

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

PostProcessEffect *Renderer::GetOrCreateEffect(size_t ID)
{
	// return cached effect if it exists
	if (m_Effects.find(ID) != m_Effects.end()) {
		return m_Effects[ID];
	}

	// lookup effect in scenegraph
	PP::Effect* chugl_effect = dynamic_cast<PP::Effect*>(m_RenderState.GetScene()->GetNode(ID));
	if (!chugl_effect) return nullptr;

	// not found, create it
	PostProcessEffect* newEffect = PostProcessEffect::Create(chugl_effect, m_ViewportWidth, m_ViewportHeight);

	// cache it
	m_Effects[ID] = newEffect;
	return newEffect;
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

		// TODO: delete PP effect
	}

	// clear queue
	scene->ClearDeletionQueue();
}

void Renderer::RenderScene(Scene* scene, Camera* camera)
{
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

	// draw skybox last to avoid overdraw
	SkyboxPass();

	// must draw after skybox pass so that transparent objects are drawn on top of skybox
	TransparentPass();

	// post processing pass
	if (m_RenderState.IsPostProcessingEnabled()) {
		PostProcessPass();
	}
}

void Renderer::SkyboxPass(int textureUnit)
{
	auto* scene = m_RenderState.GetScene();

	// early out if skybox disabled
	if (!scene->GetSkyboxEnabled()) { return; }

	Texture* skyboxTexture = GetOrCreateTexture(scene->GetSkyboxID(), nullptr);

	// out if no skybox texture
	if (!skyboxTexture || !skyboxTexture->IsLoaded()) { return; }

	// update skybox texture if it's dirty
	skyboxTexture->Update();

	// change depth function so depth test passes when values are equal to depth buffer's content
	glDepthFunc(GL_LEQUAL);  

	// bind
	m_SkyboxVA->Bind();
	m_SkyboxShader->Bind();
	skyboxTexture->Bind(textureUnit);

	// set shader uniforms (TODO can refactor to better uniform management system)
	m_SkyboxShader->setMat4f("u_Projection", m_RenderState.GetProjMat());
	m_SkyboxShader->setMat4f("u_View", 
		// remove translation component from view matrix
		// so that skybox is always centered around camera
		glm::mat4(glm::mat3(
			m_RenderState.GetViewMat()
		))
	);

	// set polygon mode to fill
	GLCall(glPolygonMode(GL_FRONT_AND_BACK, GL_FILL));
	// draw
	GLCall(glDrawArrays(GL_TRIANGLES, 0, 36));

	// restore default depth func
	glDepthFunc(GL_LESS);  
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
