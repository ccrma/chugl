#pragma once

class IndexBuffer
{
private:
	unsigned int m_RendererID;
	unsigned int m_Count;
public:
	IndexBuffer();
	IndexBuffer(const void* data, unsigned int count, unsigned int usage);
	~IndexBuffer();

	void SetBuffer(const void* data, unsigned int count, unsigned int usage);
	void Bind() const;
	void Unbind() const;

	inline unsigned int GetCount() const { return m_Count;  }
};

