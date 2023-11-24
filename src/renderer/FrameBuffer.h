#pragma once

// TODO: move all into .cpp
#include "Graphics.h"

class FrameBuffer
{
private:  // member vars
    unsigned int m_FrameBufferID, m_ColorBufferID, m_DepthBufferID;
    unsigned int m_Width, m_Height;
    unsigned int m_ColorAttachment, m_DepthAttachment;
    bool m_IsMultisampled;
public:
    FrameBuffer(
        unsigned int width, unsigned int height, bool isMultisampled = false
    );

    ~FrameBuffer() 
    {
        if (m_FrameBufferID != 0) GLCall(glDeleteFramebuffers(1, &m_FrameBufferID));
        if (m_ColorBufferID != 0) GLCall(glDeleteTextures(1, &m_ColorBufferID));
        if (m_DepthBufferID != 0) GLCall(glDeleteRenderbuffers(1, &m_DepthBufferID));
    }

    void UpdateSize(unsigned int width, unsigned int height);

    void Bind() const { 
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID)); 
    }

    void BindColorAttachment() const {
        GLCall(glActiveTexture(GL_TEXTURE0));  // activate slot 0
        GLCall(glBindTexture(GL_TEXTURE_2D, m_ColorBufferID));
    }

    void Unbind() const { 
        GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); 
    }
};