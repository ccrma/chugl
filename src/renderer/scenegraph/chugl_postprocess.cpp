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
