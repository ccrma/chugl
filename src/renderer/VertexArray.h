#pragma once

#include "IndexBuffer.h"
#include "VertexBuffer.h"
#include "VertexBufferLayout.h"

class VertexArray
{
private:
	unsigned int m_RendererID;
	const IndexBuffer* m_IndexBuffer;
	const VertexBuffer* m_VertexBuffer;
public:
	VertexArray();
	~VertexArray();

	void Bind() const;
	void Unbind() const;

	void AddBufferAndLayout(const VertexBuffer& vb, const VertexBufferLayout& layout);
	void AddIndexBuffer(const IndexBuffer& ib);
	
	inline const VertexBuffer* GetVertexBuffer() const { return m_VertexBuffer; }

	inline unsigned int GetIndexBufferCount() { return (m_IndexBuffer == nullptr) ? 0 : m_IndexBuffer->GetCount(); }
	inline const IndexBuffer* GetIndexBuffer() const { return m_IndexBuffer;  }

};
