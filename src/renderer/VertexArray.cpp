#include <glad/glad.h>
#include "Util.h"
#include "VertexArray.h"

VertexArray::VertexArray() : m_IndexBuffer(nullptr), m_VertexBuffer(nullptr)
{
	GLCall(glGenVertexArrays(1, &m_RendererID));
	GLCall(glBindVertexArray(m_RendererID));
}

VertexArray::~VertexArray()
{
	GLCall(glDeleteVertexArrays(1, &m_RendererID));
}

void VertexArray::Bind() const
{
	GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::Unbind() const
{
	GLCall(glBindVertexArray(0));
}

void VertexArray::AddBufferAndLayout(const VertexBuffer& vb, const VertexBufferLayout& layout)
{
	m_VertexBuffer = &vb;

	this->Bind();
	vb.Bind();
	const auto& elements = layout.GetElements();
	unsigned int offset = 0;
	for (unsigned int i = 0; i < elements.size(); i++)
	{
		const VertexBufferElement& element = elements[i];
		GLCall(glEnableVertexAttribArray(i));
		GLCall(glVertexAttribPointer(
			i, element.count, element.type, element.normalize, layout.GetStride(), (void *) (size_t) offset
		));
		offset += (element.count * VertexBufferElement::GetSizeOfType(element.type));
	}
	// keeping for reference
    // how to interpret vertex data (of the currently bound VBO aka GL_ARRAY_BUFFER) 
	/*
    glVertexAttribPointer(  // for vertex position
        0,  // which vertex attribute we want to configure, e.g. layout (location = 0) in vertex shader sets location of vertex attribute to 0
        3,  // count of vertex attribute (e.g. a UV has 2 floats, so 2)
        GL_FLOAT,  // type of data
        GL_FALSE,  // whether or not to normalize
        6 * sizeof(float),  // also can pass 0 for tightly packed attributes
        (void*)0  // offset
    );
	*/

}

void VertexArray::AddIndexBuffer(const IndexBuffer& ib)
{
	Bind();
	ib.Bind();
	m_IndexBuffer = &ib;
}
