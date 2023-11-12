#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"

// forward decls
struct CGL_GeoAttribute;

class VertexArray
{
private:
	unsigned int m_RendererID;
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;

	void RemoveAttribute(const CGL_GeoAttribute& attribute);
	void RemoveAttribute(unsigned int location);
	void AddBufferAndLayout(const VertexBuffer* vb, const CGL_GeoAttribute& attribute);
	void AddIndexBuffer(const IndexBuffer& ib);
};
