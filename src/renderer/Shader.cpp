#include "Shader.h"
#include "Util.h"
#include <shadinclude/Shadinclude.hpp>

// TODO: break this into read file, compile, link&validate separate functions
Shader::Shader(
    const std::string& vertex, const std::string& fragment,
    bool vertexFromFile, bool fragmentFromFile
)
    : m_VertexPath(""), m_FragmentPath(""), 
    m_VertexSource(""), m_FragmentSource(""),
    m_VertFromFile(vertexFromFile), m_FragFromFile(fragmentFromFile)
{
    if (vertexFromFile) {
        m_VertexSource = Shadinclude::load(vertex);
        m_VertexPath = vertex;
    }
    else {
        m_VertexSource = vertex;
    }

    if (fragmentFromFile) {
        m_FragmentSource = Shadinclude::load(fragment);
        m_FragmentPath = fragment;
    }
    else {
        m_FragmentSource = fragment;
    }

    m_RendererID = CreateShaderProgram(m_VertexSource, m_FragmentSource);
    ASSERT(m_RendererID);

    Bind();
}

Shader::~Shader()
{
    GLCall(glDeleteProgram(m_RendererID));
}

void Shader::Reload()
{
    assert(false); // not implemented yet
    unsigned int reloadedProgram = CreateShaderProgram(m_VertexPath, m_FragmentPath);
    if (!reloadedProgram) { Util::println("failed to reload shader program"); }

	Util::println("reloading shader new ID: " + std::to_string(reloadedProgram));

    // clear cache
    m_UniformLocationCache.clear();

    // rebind
	m_RendererID = reloadedProgram;
	Bind();
}

void Shader::Bind()
{
    GLCall(glUseProgram(m_RendererID));
}

void Shader::Unbind()
{
    GLCall(glUseProgram(0));
}

void Shader::setBool(const std::string& name, bool value) const
{
    GLCall(glUniform1i(GetUniformLocation(name), (int)value));
}

void Shader::setInt(const std::string& name, int value) const
{
    GLCall(glUniform1i(GetUniformLocation(name), value));
}

void Shader::setInt2(const std::string &name, int x, int y) const
{
    GLCall(glUniform2i(GetUniformLocation(name), x, y));
}

void Shader::setInt3(const std::string &name, int x, int y, int z) const
{
    GLCall(glUniform3i(GetUniformLocation(name), x, y, z));
}

void Shader::setInt4(const std::string &name, int x, int y, int z, int w) const
{
    GLCall(glUniform4i(GetUniformLocation(name), x, y, z, w));
}

void Shader::setFloat(const std::string& name, float value) const
{
    GLCall(glUniform1f(GetUniformLocation(name), value));
}

void Shader::setFloat2(const std::string &name, float x, float y) const
{
    GLCall(glUniform2f(GetUniformLocation(name), x, y));
}

void Shader::setFloat3(const std::string& name, float x, float y, float z) const
{
    GLCall(glUniform3f(GetUniformLocation(name), x, y, z));
}

void Shader::setFloat3(const std::string& name, const glm::vec3 pos) const
{
    GLCall(glUniform3f(GetUniformLocation(name), pos.x, pos.y, pos.z));
}

void Shader::setFloat4(const std::string& name, float x, float y, float z, float w) const
{
    GLCall(glUniform4f(GetUniformLocation(name), x, y, z, w));
}

// maps texture unit slots to texture uniforms
void Shader::setTextureUnits(unsigned int n)
{
    for (unsigned int i = 0; i < n; i++) {
        // Util::println("setting uniform texture unit u_Texture" + std::to_string(i));
        setInt("u_Texture" + std::to_string(i), i);
    }
}

void Shader::setMat4f(const std::string& name, const glm::mat4& mat)
{
    GLCall(glUniformMatrix4fv(
        GetUniformLocation(name),
        1, // num matrices
        GL_FALSE, // whether to tranpose
        glm::value_ptr(mat)
    ));
}

unsigned int Shader::CompileShader(const std::string& source, unsigned int type)
{
    unsigned int id = glCreateShader(type);
	const char* src = source.c_str();  // doesn't create new string, returns pointer to existing string

	GLCall(glShaderSource(id, 1, &src, NULL));
	GLCall(glCompileShader(id));

	int status;
	GLCall(glGetShaderiv(id, GL_COMPILE_STATUS, &status));
	if (status != GL_TRUE)
	{
		char message[1024];
		GLCall(glGetShaderInfoLog(id, 1024, NULL, message));
		std::cout 
			<< "ERROR::SHADER::COMPILATION::" 
			<< ((type == GL_VERTEX_SHADER) ? "vertex shader" : "fragment shader")
			<< " FAILED\n" << message << std::endl;
	}

	return id;
}

// compiles shader from source strings (NOT file paths)
unsigned int Shader::CreateShaderProgram(const std::string& vertexCode, const std::string& fragmentCode)
{

    // Shader Compilation ================================
    unsigned int vertexShader, fragmentShader;
    vertexShader = Shader::CompileShader(vertexCode, GL_VERTEX_SHADER);
    fragmentShader = Shader::CompileShader(fragmentCode, GL_FRAGMENT_SHADER);

    // Program Linking =======================================
    unsigned int programID = glCreateProgram();
    
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    int status;
    (glGetProgramiv(programID, GL_LINK_STATUS, &status));
    if (!status) {
        char message[1024];
        (glGetProgramInfoLog(programID, 1024, NULL, message));
        std::cout << "ERROR::SHADER::LINKING FAILED\n" << message << std::endl;
        return 0;
    }

    (glValidateProgram(programID));
    (glGetProgramiv(programID, GL_VALIDATE_STATUS, &status));
    if (!status) {
        char message[1024];
        (glGetProgramInfoLog(programID, 1024, NULL, message));
        std::cout << "ERROR::SHADER::VALIDATING FAILED\n" << message << std::endl;
        return 0;
    }

    (glDeleteShader(vertexShader));
    (glDeleteShader(fragmentShader));

    return programID;
}

int Shader::GetUniformLocation(const std::string& name) const
{
    if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end()) {
        return m_UniformLocationCache[name];
    }

    // If location is equal to -1, the data passed in to uniform setters will be silently ignored and the specified uniform variable will not be changed.
    GLCall(unsigned int location = glGetUniformLocation(m_RendererID, name.c_str()));

    // if (location == -1)
    //     std::cout 
    //         << "Warning: uniform " 
    //         << name 
    //         << " does not exist for shader: " 
    //         << GetFragPath()
    //         << "\n"
    //         << GetVertPath()
    //         << std::endl;

	m_UniformLocationCache[name] = location;
    return location;
}
