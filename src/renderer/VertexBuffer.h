#pragma once

class VertexBuffer
{
private:
	unsigned int m_RendererID;
	unsigned int m_Size, m_Count;
public:
	VertexBuffer();
	VertexBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage);
	~VertexBuffer();

	void SetBuffer(const void* data, unsigned int size, unsigned int count, unsigned int usage);
	void SubBuffer(const void* data, unsigned int size, unsigned int offset);
	void Bind() const;
	void Unbind() const;

	inline unsigned int GetSize() const { return m_Size; }  // size in bytes
	inline unsigned int GetCount() const { return m_Count; }  // number of vertices
};
