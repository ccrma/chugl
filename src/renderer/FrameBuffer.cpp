#include "FrameBuffer.h"
#include "Graphics.h"

FrameBuffer::FrameBuffer(
	unsigned int width, unsigned int height, bool hdr, bool isMultisampled
) : m_FrameBufferID(0), m_ColorBufferID(0), m_DepthBufferID(0),
    m_Width(width), 
    m_Height(height), 
    m_ColorAttachment(0), 
    m_DepthAttachment(0),
    m_IsMultisampled(isMultisampled),
    m_HDR(hdr)
{
    // gen and bind
    GLCall(glGenFramebuffers(1, &m_FrameBufferID));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID));    

    // create attachments
    // create and attach depth buffer
    // TODO add texture or renderbuffer option
    // TODO: HDR support
    GLCall(glGenTextures(1, &m_ColorBufferID));
    GLCall(glGenRenderbuffers(1, &m_DepthBufferID));

    UpdateSize(width, height, true);

    // attach it to currently bound framebuffer object
    if (m_IsMultisampled) {
        GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorBufferID, 0)); 
    } else {
        GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBufferID, 0)); 
    }   
    // attach depth buffer
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));

    // check if framebuffer is complete
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    // unbind framebuffer
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));  
}

FrameBuffer::~FrameBuffer()
{
    if (m_FrameBufferID != 0) GLCall(glDeleteFramebuffers(1, &m_FrameBufferID));
    if (m_ColorBufferID != 0) GLCall(glDeleteTextures(1, &m_ColorBufferID));
    if (m_DepthBufferID != 0) GLCall(glDeleteRenderbuffers(1, &m_DepthBufferID));
}

void FrameBuffer::UpdateSize(unsigned int width, unsigned int height, bool force /*= false*/)
{
    // make sure this is already initialized
    assert(m_FrameBufferID != 0 && m_ColorBufferID != 0 && m_DepthBufferID != 0); 
    
    // early out
    if (!force && width == m_Width && height == m_Height) return;

    // update member vars
    m_Width = width;
    m_Height = height;

    // update texture dimensions 
    auto textureTarget = m_IsMultisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
    auto internalFormat = m_HDR ? GL_RGBA16F : GL_RGBA8;

    GLCall(glBindTexture(textureTarget, m_ColorBufferID));
    if (m_IsMultisampled) {
        GLCall(glTexImage2DMultisample(textureTarget, 4, internalFormat, width, height, GL_TRUE));
    } else {
        GLCall(glTexImage2D(textureTarget, 0, internalFormat, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		GLCall(glTexParameteri(textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
		GLCall(glTexParameteri(textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    }
    GLCall(glBindTexture(textureTarget, 0));

    // update renderbuffer dimensions
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID)); 
    if (m_IsMultisampled) {
        GLCall(glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height));
    } else {
        GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
    }
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}

void FrameBuffer::Bind() const
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID)); 
}

void FrameBuffer::BindColorAttachment() const
{
    GLCall(glActiveTexture(GL_TEXTURE0));  // activate slot 0
    if (m_IsMultisampled) {
        GLCall(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorBufferID));
    } else {
        GLCall(glBindTexture(GL_TEXTURE_2D, m_ColorBufferID));
    }
}

void FrameBuffer::Unbind() const
{
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0)); 
}
