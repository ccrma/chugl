#pragma once

// TODO move into .cpp
#include "Shader.h"
#include "scenegraph/chugl_postprocess.h"
#include "ShaderCode.h"

// Abstract base class for post-processing effects
// Renderer impl of chugl_postprocess API

#include "Graphics.h"

class Renderer;
class BaseEffect;
class BloomEffect;

class PostProcessEffect
{
protected:
    PP::Effect* m_Effect;
    PostProcessEffect(PP::Effect* effect) : m_Effect(effect) {
        // assert all not nulll
        assert(effect);
    }
public:  // factory
    static PostProcessEffect* Create(PP::Effect* chugl_effect, unsigned int viewportWidth, unsigned int viewportHeight);
public:
    virtual ~PostProcessEffect() {}

    virtual void Apply(
        Renderer& renderer, unsigned int srcTexture, unsigned int writeFrameBufferID
    ) = 0;

    // handle window resize
    virtual void Resize(unsigned int viewportWidth, unsigned int viewportHeight) = 0;
    
    // handle any changes to underlying chugl effect no capture via uniforms
    virtual void Update() {}

    PP::Effect* GetChuglEffect() { return m_Effect; }

    // TODO: abstract fn to generate a GUI for this effect
    // TODO: timer
    // TODO: bypass
};

// Most basic post process effect. Single shader, no secondary FBOs, single pass
class BasicEffect : public PostProcessEffect
{
private:
    Shader* m_Shader;
public:
    BasicEffect(
        PP::Effect* effect, Shader* shader
    ) : PostProcessEffect(effect), m_Shader(shader) {
        // assert all not nulll
        assert(effect);
        assert(shader);
    }
    virtual ~BasicEffect() { if (m_Shader) delete m_Shader; }
    virtual void Apply(Renderer& renderer, unsigned int srcTexture, unsigned int writeFrameBufferID) override;
    virtual void Resize(unsigned int viewportWidth, unsigned int viewportHeight) override { /* nothing */ }
    virtual void Update() override { 
        if (m_Effect->GetType() == PP::Type::Custom) {
            PP::CustomEffect* customEffect = dynamic_cast<PP::CustomEffect*>(m_Effect);
            if (customEffect->GetRebuildShader()) {
                // TODO: refactor this once we have an actual shader management system
                delete m_Shader;
                m_Shader = new Shader(
                    ShaderCode::PP_VERT, customEffect->GetScreenShader(), 
                    false, customEffect->IsPath()
                );

                // new shader, clear old uniforms
                // customEffect->ClearUniforms();
                // actually don't clear, because this invalidates uniforms that were set at the same frame as shader switch
                

                // reset flag
                customEffect->SetRebuildShader(false);
            }
        }
    }
};


class BloomEffect : public PostProcessEffect
{
public:
    BloomEffect(PP::Effect* effect, unsigned int viewportWidth, unsigned int viewportHeight) 
    :
    PostProcessEffect(effect),
    // shaders
    m_DownSampleShader(nullptr), 
    m_UpSampleShader(nullptr),
    m_BlendShader(nullptr),
    // viewport
    m_SrcViewportWidth(viewportWidth), m_SrcViewportHeight(viewportHeight)
    {
        GLCall(glGenFramebuffers(1, &m_FrameBufferID));
        GenerateMipChain(viewportWidth, viewportHeight);
        
        // glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // assign shaders (TODO: improved shader cache system)
        m_DownSampleShader = new Shader(ShaderCode::PP_VERT, ShaderCode::PP_BLOOM_DOWNSAMPLE, false, false);
        m_UpSampleShader = new Shader(ShaderCode::PP_VERT, ShaderCode::PP_BLOOM_UPSAMPLE, false, false);
        m_BlendShader = new Shader(ShaderCode::PP_VERT, ShaderCode::PP_BLOOM_BLEND, false, false);

        m_DownSampleShader->Bind();
        m_DownSampleShader->setInt("srcTexture", 0);

        m_UpSampleShader->Bind();
        m_UpSampleShader->setInt("srcTexture", 0);

        m_BlendShader->Bind();
        m_BlendShader->setInt("srcTexture", 0);
        m_BlendShader->setInt("bloomTexture", 1);
    }

    virtual ~BloomEffect()
    {
        for (int i = 0; i < m_MipChain.size(); i++) {
            glDeleteTextures(1, &m_MipChain[i].texID);
            // m_MipChain[i].texID = 0;
        }
        glDeleteFramebuffers(1, &m_FrameBufferID);
        // mFBO = 0;
        // mInit = false;

        // clear shaders
        if (m_DownSampleShader) delete m_DownSampleShader;
        if (m_UpSampleShader) delete m_UpSampleShader;
        if (m_BlendShader) delete m_BlendShader;
    }


    virtual void Apply(Renderer& renderer, unsigned int srcTexture, unsigned int writeFrameBufferID) override
    {
        Bind();

        RenderDownsamples( /* srcTexture */ );
        RenderUpsamples();

        // Restore viewport
        glViewport(0, 0, m_SrcViewportWidth, m_SrcViewportHeight);

        // blend and write to writeFrameBuffer
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, writeFrameBufferID)); 

        // Use shader
        m_BlendShader->Bind();

        // bind src texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcTexture);
        // glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, srcTexture);
        // m_BlendShader->setInt("srcTexture", 0);  // must match uniform name in PP_BLOOM_BLEND
        // bind bloom texture
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, GetBloomTexture());
        // glBindTexture(GL_TEXTURE_2D, GetBloomTexture());
        // m_BlendShader->setInt("bloomTexture", 1); // must match uniform name in PP_BLOOM_BLEND 

        // apply blending uniforms
        auto* chugl_bloom = GetChuglBloom();
        m_BlendShader->setFloat(PP::BloomEffect::U_STRENGTH, chugl_bloom->GetStrength());
        m_BlendShader->setInt(PP::BloomEffect::U_BLEND_MODE, chugl_bloom->GetBlendMode());
        m_BlendShader->setInt(PP::BloomEffect::U_LEVELS, GetChainLength());

        // Render screen-filled quad
        // (current don't need to because already done in postproecsspass)
    }

    virtual void Resize(unsigned int viewportWidth, unsigned int viewportHeight) override
    {
        if (m_SrcViewportWidth == viewportWidth && m_SrcViewportHeight == viewportHeight) return;

        m_SrcViewportWidth = viewportWidth;
        m_SrcViewportHeight = viewportHeight;
        GenerateMipChain(viewportWidth, viewportHeight);
    }


private:  // internal methods
    PP::BloomEffect* GetChuglBloom() const {
        PP::BloomEffect* chugl_bloom = dynamic_cast<PP::BloomEffect*>(m_Effect);
        assert(chugl_bloom);
        return chugl_bloom;
    }

    void GenerateMipChain(unsigned int viewportWidth, unsigned int viewportHeight) {
        // Clear existing mip chain
        for (int i = 0; i < m_MipChain.size(); i++) {
            glDeleteTextures(1, &m_MipChain[i].texID);
            // m_MipChain[i].texID = 0;
        }
        m_MipChain.clear();

        glm::vec2 mipSize((float)viewportWidth, (float)viewportHeight);
        glm::ivec2 mipIntSize((int)viewportWidth, (int)viewportHeight);

        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));

        // pre-generate max length mip chain
        for (unsigned int i = 0; i < MAX_CHAIN_LENGTH; i++)
        {
            Mip mip;

            mipSize *= 0.5f;
            mipIntSize /= 2;
            mip.size = mipSize;
            mip.iSize = mipIntSize;

            glGenTextures(1, &mip.texID);
            glBindTexture(GL_TEXTURE_2D, mip.texID);
            // we are downscaling an HDR color buffer, so we need a float texture format
            // TODO: can we ignore the alpha channel here?
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
                        mipIntSize.x, mipIntSize.y,
                        0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            m_MipChain.emplace_back(mip);

            // out if we've reached the smallest size
            if (mipIntSize.x <= 1 && mipIntSize.y <= 1) break;
        }

        // TODO: can we reuse ping/pong frame buffers?

        GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_MipChain[0].texID, 0));

        // setup attachments
        unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
        GLCall(glDrawBuffers(1, attachments));

        // check completion status
        int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            printf("gbuffer FBO error, status: 0x\%x\n", status);
            // glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void Bind() const {
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));
    }

    unsigned int GetBloomTexture() const {
        return m_MipChain[0].texID;
    }

    int GetChainLength() {
        return glm::clamp<int>(GetChuglBloom()->GetLevels(), 0, m_MipChain.size() - 1);
    }

    void RenderDownsamples() {
        m_DownSampleShader->Bind();

        // set uniforms
        m_DownSampleShader->setFloat2("u_SrcResolution", m_SrcViewportWidth, m_SrcViewportHeight);
        auto* chugl_bloom = GetChuglBloom();
        m_DownSampleShader->setFloat(PP::BloomEffect::U_THRESHOLD, chugl_bloom->GetThreshold());

        m_DownSampleShader->setBool(PP::BloomEffect::U_KARIS_ENABLED, chugl_bloom->GetKarisEnabled());

        // m_DownSampleShader->setFloat(PP::BloomEffect::U_THRESHOLD_KNEE, chugl_bloom->GetThresholdKnee());

        // should already be bound
        // Bind srcTexture (HDR color buffer) as initial texture input
        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, srcTexture);

        // Progressively downsample through the mip chain
        for (int i = 0; i < GetChainLength(); i++)
        {
            const Mip& mip = m_MipChain[i];
            // set mip level
            m_DownSampleShader->setInt("u_MipLevel", i);
            // rescale viewport
            glViewport(0, 0, mip.size.x, mip.size.y);
            // bind destination
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, mip.texID, 0);

            // Render screen-filled quad of resolution of current mip
            // glBindVertexArray(quadVAO);
            // Screen quad VAO shoulds already be bound
            glDrawArrays(GL_TRIANGLES, 0, 3);
            // glBindVertexArray(0);

            // Set current mip resolution as srcResolution for next iteration
            m_DownSampleShader->setFloat2("u_SrcResolution", mip.size.x, mip.size.y);
            // Set current mip as texture input for next iteration
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mip.texID);
            // m_DownSampleShader->setInt("srcTexture", 0);
        }
    }

    void RenderUpsamples() {
        m_UpSampleShader->Bind();
        m_UpSampleShader->setFloat(PP::BloomEffect::U_RADIUS, GetChuglBloom()->GetRadius());

        // Enable additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);

        for (int i = GetChainLength() - 1; i > 0; i--)
        {
            const Mip& mip = m_MipChain[i];
            const Mip& nextMip = m_MipChain[i-1];

            // Bind viewport and texture from where to read
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mip.texID);

            // Set framebuffer render target (we write to this texture)
            glViewport(0, 0, nextMip.size.x, nextMip.size.y);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                GL_TEXTURE_2D, nextMip.texID, 0);

            // Render screen-filled quad of resolution of current mip
            // glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            // glBindVertexArray(0);
        }

        // Disable additive blending
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Restore if this was default
        // glDisable(GL_BLEND);

        // mUpsampleShader->Deactivate();
    }

private:  // shader setup
    Shader* m_DownSampleShader;
    Shader* m_UpSampleShader;
    Shader* m_BlendShader;
    unsigned int m_SrcViewportWidth, m_SrcViewportHeight;

private:  // framebuffer setup
    unsigned int m_FrameBufferID;
    static const int MAX_CHAIN_LENGTH;


    struct Mip {
        glm::vec2 size;
        glm::ivec2 iSize;
        unsigned int texID;
    };
    std::vector<Mip> m_MipChain;
};