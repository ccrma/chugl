#include "chugl_postprocess.h"
#include "Locator.h"

using namespace PP;

// =================================================================================================
// PP::Effect
// =================================================================================================

Effect::CkTypeMap Effect::s_CkTypeMap = {
    { Type::Base, "FX" },
    { Type::PassThrough, "PassThroughFX" },
    { Type::Output, "OutputFX" },
    { Type::Invert, "InvertFX"},
    { Type::Bloom, "BloomFX" }
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
// Color Invert Effect 
// =================================================================================================
const std::string InverseEffect::U_MIX = "u_Mix";

// =================================================================================================
// Output Effect 
// =================================================================================================

const std::string OutputEffect::U_GAMMA = "u_Gamma";
const std::string OutputEffect::U_TONEMAP = "u_Tonemap";
const std::string OutputEffect::U_EXPOSURE = "u_Exposure";

const int OutputEffect::TONEMAP_NONE = 0;
const int OutputEffect::TONEMAP_LINEAR = 1;
const int OutputEffect::TONEMAP_REINHARD = 2;
const int OutputEffect::TONEMAP_CINEON = 3;
const int OutputEffect::TONEMAP_ACES = 4;
const int OutputEffect::TONEMAP_UNCHARTED = 5;


// =================================================================================================
// Bloom Effect 
// =================================================================================================
const std::string BloomEffect::U_STRENGTH = "u_BloomStrength";
const std::string BloomEffect::U_RADIUS = "u_FilterRadius";
const std::string BloomEffect::U_THRESHOLD = "u_Threshold";
// const std::string BloomEffect::U_THRESHOLD_KNEE = "u_ThresholdKnee";
const std::string BloomEffect::U_LEVELS = "u_Levels";
const std::string BloomEffect::U_BLEND_MODE = "u_BlendMode";

const std::string BloomEffect::U_KARIS_ENABLED = "u_KarisEnabled";
const std::string BloomEffect::U_KARIS_MODE = "u_KarisMode";

const t_CKUINT BloomEffect::BLEND_MIX = 0;
const t_CKUINT BloomEffect::BLEND_ADD = 1;