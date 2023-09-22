// Scenegraph texture class

#include "CGL_Texture.h"

// static initializers
const CGL_TextureWrapMode CGL_Texture::Repeat = CGL_TextureWrapMode::Repeat;
const CGL_TextureWrapMode CGL_Texture::MirroredRepeat = CGL_TextureWrapMode::MirroredRepeat;
const CGL_TextureWrapMode CGL_Texture::ClampToEdge = CGL_TextureWrapMode::ClampToEdge;

const CGL_TextureFilterMode CGL_Texture::Nearest = CGL_TextureFilterMode::Nearest;
const CGL_TextureFilterMode CGL_Texture::Linear = CGL_TextureFilterMode::Linear;
// const CGL_TextureFilterMode CGL_Texture::Nearest_MipmapNearest = CGL_TextureFilterMode::Nearest_MipmapNearest;
// const CGL_TextureFilterMode CGL_Texture::Linear_MipmapNearest = CGL_TextureFilterMode::Linear_MipmapNearest;
// const CGL_TextureFilterMode CGL_Texture::Nearest_MipmapLinear = CGL_TextureFilterMode::Nearest_MipmapLinear;
// const CGL_TextureFilterMode CGL_Texture::Linear_MipmapLinear = CGL_TextureFilterMode::Linear_MipmapLinear;