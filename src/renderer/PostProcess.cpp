#include "PostProcess.h"
#include "Renderer.h"
#include "scenegraph/chugl_postprocess.h"
#include "scenegraph/Locator.h"



PostProcessEffect* PostProcessEffect::Create(PP::Effect* chugl_effect, unsigned int viewportWidth, unsigned int viewportHeight)
{

	const std::string& screenShaderVert = ShaderCode::PP_VERT;
	std::string screenShaderFrag;

	// get fragment shader code based on pp type
	switch (chugl_effect->GetType()) {
	case PP::Type::Base:
		throw std::runtime_error("Cannot create PostProcessEffect from base class");
	case PP::Type::PassThrough:
		screenShaderFrag = ShaderCode::PP_PASS_THROUGH;
		// return effect
		return new BasicEffect(
			chugl_effect,
			new Shader(screenShaderVert, screenShaderFrag, false, false)
		);
	case PP::Type::Output:
		screenShaderFrag = ShaderCode::PP_OUTPUT;
		// return effect
		return new BasicEffect(
			chugl_effect,
			new Shader(screenShaderVert, screenShaderFrag, false, false)
		);
    case PP::Type::Invert:
		return new BasicEffect(
			chugl_effect,
			new Shader(screenShaderVert, ShaderCode::PP_INVERT, false, false)
		);
    case PP::Type::Monochrome:
        return new BasicEffect(
            chugl_effect,
            new Shader(screenShaderVert, ShaderCode::PP_MONOCHROME, false, false)
        );
	case PP::Type::Bloom:
		return new BloomEffect(chugl_effect, viewportWidth, viewportHeight);
		break;
	default:
		throw std::runtime_error("Unknown PP::Type");
	}
}

// Most basic post process effect. Single shader, no secondary FBOs, single pass
void BasicEffect::Apply(Renderer& renderer, unsigned int srcTexture, unsigned int writeFrameBufferID)
{
    // bind shader
    m_Shader->Bind();
    
    // texture unit counter
    unsigned int textureUnit = 1;  // start at 1 because 0 is reserved for the scene texture

    // set uniforms
    // TODO: consolidate with RenderMaterial::SetLocalUniforms()
    for (auto& it : m_Effect->GetUniforms()) {

        auto& name = it.first;
        auto& uniform = it.second;

        Texture* rendererTexture = nullptr;

        switch (uniform.type) {
        case UniformType::Bool:
            m_Shader->setBool(name, uniform.b);
            break;
        case UniformType::Int:
            m_Shader->setInt(name, uniform.i);
            break;
        case UniformType::Int2:
            m_Shader->setInt2(name, uniform.i2[0], uniform.i2[1]);
            break;
        case UniformType::Int3:
            m_Shader->setInt3(name, uniform.i3[0], uniform.i3[1], uniform.i3[2]);
            break;
        case UniformType::Int4:
            m_Shader->setInt4(name, uniform.i4[0], uniform.i4[1], uniform.i4[2], uniform.i4[3]);
            break;
        case UniformType::Float:
            m_Shader->setFloat(name, uniform.f);
            break;
        case UniformType::Float2:
            m_Shader->setFloat2(name, uniform.f2[0], uniform.f2[1]);
            break;
        case UniformType::Float3:
            m_Shader->setFloat3(name, uniform.f3[0], uniform.f3[1], uniform.f3[2]);
            break;
        case UniformType::Float4:
            m_Shader->setFloat4(name, uniform.f4[0], uniform.f4[1], uniform.f4[2], uniform.f4[3]);
            break;
        case UniformType::Texture:
            rendererTexture = renderer.GetOrCreateTexture(uniform.texID);
            rendererTexture->Bind(textureUnit);

            // update GPU params if CGL_shader has been modified
            rendererTexture->Update();

            m_Shader->setInt(name, textureUnit++);
            break;
        default:
            throw std::runtime_error("Unknown UniformType");
        }
    }
}

// ====================
// Bloom
// ====================

// enough downsample steps to go from 65536x65536 to 1x1
const int BloomEffect::MAX_CHAIN_LENGTH = 16;