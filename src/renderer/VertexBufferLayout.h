#pragma once

#include "chugl_pch.h"
#include "Util.h"

/*
* Helper class for VertexAttribPointer assignment on VertexArrayObjects
*/

struct VertexBufferElement
{
	std::string name;
	unsigned int type;
	unsigned int count;
	unsigned char normalize;

	// ref: https://gist.github.com/davawen/af1490ffb3bbcf9ddc0cbab82e9f27aa
	static unsigned int GetSizeOfType(unsigned int type)
	{
		switch (type)
		{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			return sizeof(GLbyte);
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			return sizeof(GLshort);
		case GL_INT_2_10_10_10_REV:
		case GL_INT:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
		case GL_UNSIGNED_INT:
			return sizeof(GLint);
		case GL_FLOAT:
			return sizeof(GLfloat);
		case GL_DOUBLE:
			return sizeof(GLdouble);
		case GL_FIXED:
			return sizeof(GLfixed);
		case GL_HALF_FLOAT:
			return sizeof(GLhalf);
		}
		assert(false);
		return 0;
	}
};

class VertexBufferLayout
{
private:
	std::vector<VertexBufferElement> m_Elements;
	unsigned int m_Stride;
public:
	VertexBufferLayout() : m_Stride(0) {};

	void Push(const std::string& name, unsigned int type, unsigned int count, unsigned char normalize)
	{
		m_Elements.push_back({ name, type, count, normalize });
		m_Stride += count * VertexBufferElement::GetSizeOfType(type);
	}

	inline const std::vector<VertexBufferElement>& GetElements() const { return m_Elements; }
	inline unsigned int GetStride() const { return m_Stride; }
};
