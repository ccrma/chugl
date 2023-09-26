#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"
// #include "VertexBufferLayout.h"

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

	void AddBufferAndLayout(const VertexBuffer& vb, const CGL_GeoAttribute& attribute);
	void AddIndexBuffer(const IndexBuffer& ib);
};
