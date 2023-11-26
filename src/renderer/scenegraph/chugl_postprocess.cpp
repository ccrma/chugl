#include "chugl_postprocess.h"
#include "Locator.h"

using namespace PP;

// =================================================================================================
// PP::Effect
// =================================================================================================

Effect::CkTypeMap Effect::s_CkTypeMap = {
    { Type::Base, "PP_Effect" },
    { Type::PassThrough, "PP_PassThrough" },
    { Type::Output, "PP_Output" }
};

void PP::Effect::SetUniform(const MaterialUniform &uniform)
{
    // copied directly from Material::SetUniform

    // texture refcounting
    if (uniform.type == UniformType::Texture) {
        // get old uniform, if it exists
        auto it = m_Uniforms.find(uniform.name);

        // if it's a texture, refcount it
        if (uniform.type == UniformType::Texture) {
            CGL_Texture* texture = (CGL_Texture* )Locator::GetNode(uniform.texID, IsAudioThreadObject());
            CHUGL_NODE_ADD_REF(texture);
        }

        // if old uniform was a texture, unrefcount it
        if (it != m_Uniforms.end() && it->second.type == UniformType::Texture) {
            CGL_Texture* oldTexture = (CGL_Texture* )Locator::GetNode(it->second.texID, IsAudioThreadObject());
            CHUGL_NODE_QUEUE_RELEASE(oldTexture);
        }
    }

    // set uniform
    m_Uniforms[uniform.name] = uniform;
}

Effect *PP::Effect::Next()
{
    return Locator::GetNode<Effect>(m_NextID, IsAudioThreadObject());
}

Effect *PP::Effect::NextEnabled()
{
    if (!m_NextID) return nullptr;

    Effect* next = Locator::GetNode<Effect>(m_NextID, IsAudioThreadObject());
    if (!next) return nullptr;

    if (next->m_Bypass) return next->NextEnabled();

    return next;
}

// =================================================================================================
// Output Effect 
// =================================================================================================

const std::string OutputEffect::U_GAMMA = "u_Gamma";
