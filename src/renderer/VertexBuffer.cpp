#include "Graphics.h"
#include "VertexBuffer.h"

VertexBuffer::VertexBuffer()
	: m_Count(0), m_Size(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

VertexBuffer::VertexBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage = GL_STATIC_DRAW)
	: m_Count(count), m_Size(size)
{
	GLCall(glGenBuffers(1, &m_RendererID));
	SetBuffer(data, size, count, usage);
	// GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
	// GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

VertexBuffer::~VertexBuffer()
{
	// https://docs.gl/gl3/glDeleteBuffers
	GLCall(glDeleteBuffers(1, &m_RendererID));
	// std::cout << "destroying vertex buffer" << std::endl;
}

void VertexBuffer::SetBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage)
{
	m_Count = count;
	m_Size = size;
	Bind();
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

// substitutes buffer data in place, rather than recreating new one
void VertexBuffer::SubBuffer(const void *data, unsigned int size, unsigned int offset)
{
	Bind();
	GLCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
}


void VertexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
