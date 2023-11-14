#pragma once

#include "chugl_pch.h"

class Shader
{
public:
	unsigned int m_RendererID;  // program ID
	std::string m_VertexPath, m_FragmentPath;
	std::string m_VertexSource, m_FragmentSource;
	bool m_FragFromFile, m_VertFromFile;  // true if gen from file, false if gen from string

	Shader(
		const std::string& vertexPath, const std::string& fragmentPath,
		bool vertexFromFile, bool fragmentFromFile
	);

	Shader()
		: m_VertexPath(""), m_FragmentPath(""),
		  m_VertexSource(""), m_FragmentSource(""),
		  m_VertFromFile(false), m_FragFromFile(false),
		  m_RendererID(0) {}

	~Shader();

	// Compile and create
	void Compile(
		const std::string& vertex, const std::string& fragment,
		bool vertexFromFile, bool fragmentFromFile
	);

	// hot reloading!
	void Reload();
	
	// activate the shader program
	void Bind();
	void Unbind();
	
	// uniform setters
	void setBool(const std::string& name, bool value) const;
	void setInt(const std::string& name, int value) const;
	void setInt2(const std::string& name, int x, int y) const;
	void setInt3(const std::string& name, int x, int y, int z) const;
	void setInt4(const std::string& name, int x, int y, int z, int w) const;
	void setFloat(const std::string& name, float value) const;
	void setFloat2(const std::string& name, float x, float y) const;
	void setFloat3(const std::string& name, float x, float y, float z) const;
	void setFloat3(const std::string& name, const glm::vec3 pos) const;
	void setFloat4(const std::string& name, float x, float y, float z, float w) const;
	void setTextureUnits(unsigned int n = 8);
	void setMat4f(const std::string& name, const glm::mat4& mat);


	// getters/setters
	std::string GetVertPath() const { return m_VertexPath; }
	std::string GetFragPath() const { return m_FragmentPath; }
private:
	mutable std::unordered_map<std::string, int> m_UniformLocationCache;
	static unsigned int CompileShader(const std::string& source, unsigned int type);
	unsigned int CreateShaderProgram(const std::string& vertexPath, const std::string& fragPath);
	int GetUniformLocation(const std::string& name) const;

};
