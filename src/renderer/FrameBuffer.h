#pragma once

class FrameBuffer
{
private:  // member vars
    unsigned int m_FrameBufferID, m_ColorBufferID, m_DepthBufferID;
    unsigned int m_Width, m_Height;
    unsigned int m_ColorAttachment, m_DepthAttachment;
    bool m_IsMultisampled;
    bool m_HDR;
public:
    FrameBuffer(
        unsigned int width, unsigned int height, bool hdr, bool isMultisampled = false
    );

    ~FrameBuffer();

    unsigned int GetID() const { return m_FrameBufferID; }
    unsigned int GetWidth() const { return m_Width; }
    unsigned int GetHeight() const { return m_Height; }

    void UpdateSize(unsigned int width, unsigned int height, bool force = false);

    void Bind() const;
    void BindColorAttachment() const;
    void Unbind() const;
};