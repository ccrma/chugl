#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(
	unsigned int width, unsigned int height, bool isMultisampled
) : m_FrameBufferID(0), m_ColorBufferID(0), m_DepthBufferID(0),
    m_Width(0), 
    m_Height(0), 
    m_ColorAttachment(0), 
    m_DepthAttachment(0),
    m_IsMultisampled(false)
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

    UpdateSize(width, height);

    // attach it to currently bound framebuffer object
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorBufferID, 0)); 
    GLCall(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthBufferID));

    // check if framebuffer is complete
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

    // unbind framebuffer
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));  
}

void FrameBuffer::UpdateSize(unsigned int width, unsigned int height)
{
    // make sure this is already initialized
    assert(m_FrameBufferID != 0 && m_ColorBufferID != 0 && m_DepthBufferID != 0); 
    
    // early out
    if (width == m_Width && height == m_Height) return;

    // update member vars
    m_Width = width;
    m_Height = height;

    // update texture dimensions 
    GLCall(glBindTexture(GL_TEXTURE_2D, m_ColorBufferID));
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));

    // update renderbuffer dimensions
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, m_DepthBufferID)); 
    GLCall(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height));
    GLCall(glBindRenderbuffer(GL_RENDERBUFFER, 0));
}