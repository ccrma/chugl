#pragma once

#include "chugl_pch.h"

enum class ShaderType : t_CKUINT {
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

// post process shader components
static const std::string PP_VERT;               // vertex shared by all post process shaders
static const std::string PP_PASS_THROUGH;       // passthrough effect
static const std::string PP_OUTPUT;             // output effect
static const std::string PP_BLOOM_DOWNSAMPLE;              // bloom effect downsampling
static const std::string PP_BLOOM_UPSAMPLE;                // bloom effect upsampling
static const std::string PP_BLOOM_BLEND;                // bloom effect upsampling

// skybox shader impl
static const std::string SKYBOX_VERT_CODE;
static const std::string SKYBOX_FRAG_CODE;

// map of names to shader code components
typedef std::unordered_map<std::string, const std::string> ShaderMap;
static ShaderMap s_CodeMap;


private:
    ShaderCode();
    ~ShaderCode() {}
    static ShaderCode* i;

};




