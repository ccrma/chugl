#include "PostProcess.h"
#include "Renderer.h"
#include "scenegraph/chugl_postprocess.h"
#include "scenegraph/Locator.h"

void PostProcessEffect::Apply(Renderer& renderer)
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
