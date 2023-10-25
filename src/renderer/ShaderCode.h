#pragma once
#include <string>
#include <unordered_map>

enum ShaderType : unsigned int {
    Vertex = 0,
    Fragment,
    Geometry,
    Compute
};

class ShaderCode  // singleton
{
public:
// static ShaderCode* instance() {
//     if (!i) i = new ShaderCode();
//     return i;
// }

// constructs shader source code, following includes, no quotes "" needed
static std::string GenShaderSource(
    const std::string& name,
    ShaderType type,
    bool fog = true
);

// code component names
static const std::string BASIC_VERT;
static const std::string NORMAL_FRAG;
static const std::string FLAT_FRAG;

// screen shader components
static const std::string SCREEN_VERT;
static const std::string SCREEN_FRAG;

// map of names to shader code components
typedef std::unordered_map<std::string, const std::string> ShaderMap;
static ShaderMap s_CodeMap;


private:
    ShaderCode();
    ~ShaderCode() {}
    static ShaderCode* i;

};




