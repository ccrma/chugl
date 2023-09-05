#include "Util.h"
#include "VertexBuffer.h"
#include <iostream>

VertexBuffer::VertexBuffer()
	: m_Count(0), m_Size(0)
{
	GLCall(glGenBuffers(1, &m_RendererID));
}

VertexBuffer::VertexBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage = GL_STATIC_DRAW)
	: m_Count(count), m_Size(size)
{
	GLCall(glGenBuffers(1, &m_RendererID));
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

VertexBuffer::~VertexBuffer()
{
	// https://docs.gl/gl3/glDeleteBuffers
	GLCall(glDeleteBuffers(1, &m_RendererID));
	std::cout << "destroying vertex buffer" << std::endl;
}

void VertexBuffer::SetBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage)
{
	m_Count = count;
	m_Size = size;
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
	GLCall(glBufferData(GL_ARRAY_BUFFER, size, data, usage));
}

void VertexBuffer::Bind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, m_RendererID));
}

void VertexBuffer::Unbind() const
{
	GLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}
