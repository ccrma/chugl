#pragma once
#include <string>
#include <unordered_map>

// const std::string SHADER_VERSION;

// const std::string TRANSFORM_UNIFORMS;
// const std::string LIGHTING_UNIFORMS;

// const std::string BASIC_VERTEX_SHADER;

class ShaderCode  // singleton
{
public:
// static ShaderCode* instance() {
//     if (!i) i = new ShaderCode();
//     return i;
// }

// constructs shader source code, following includes, no quotes "" needed
static std::string GenShaderSource(const std::string& name);

// map of names to shader code components
typedef std::unordered_map<std::string, const std::string> ShaderMap;
static ShaderMap s_CodeMap;

private:
    ShaderCode();
    ~ShaderCode() {}
    static ShaderCode* i;

};




