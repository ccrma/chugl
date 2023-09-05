#include "IndexBuffer.h"
#include "Util.h"

IndexBuffer::IndexBuffer() : m_Count(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

IndexBuffer::IndexBuffer(const void* data, unsigned int count, unsigned int usage = GL_STATIC_DRAW)
	: m_Count(count)
{
	GLCall(glGenBuffers(1, &m_RendererID));
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, usage));
}

IndexBuffer::~IndexBuffer()
{
	// https://docs.gl/gl3/glDeleteBuffers
	GLCall(glDeleteBuffers(1, &m_RendererID));
}

void IndexBuffer::SetBuffer(const void* data, unsigned int count, unsigned int usage)
{
	m_Count = count;
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
	GLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(unsigned int), data, usage));
}

void IndexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID));
}

void IndexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
}

