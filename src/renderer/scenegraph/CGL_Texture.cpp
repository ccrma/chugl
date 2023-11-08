// Scenegraph texture class

#include "CGL_Texture.h"

// Chuck Naming
CGL_Texture::CkTypeMap CGL_Texture::s_CkTypeMap = {
    {CGL_TextureType::File2D, "FileTexture"},
    {CGL_TextureType::RawData, "DataTexture"},
};

const char* CGL_Texture::CKName(CGL_TextureType type) 
{
    return s_CkTypeMap[type].c_str();
}

// static initializers
const t_CKUINT CGL_Texture::Repeat = CGL_TextureWrapMode::Repeat;
const t_CKUINT CGL_Texture::MirroredRepeat = CGL_TextureWrapMode::MirroredRepeat;
const t_CKUINT CGL_Texture::ClampToEdge = CGL_TextureWrapMode::ClampToEdge;

const t_CKUINT CGL_Texture::Nearest = CGL_TextureFilterMode::Nearest;
const t_CKUINT CGL_Texture::Linear = CGL_TextureFilterMode::Linear;
// const CGL_TextureFilterMode CGL_Texture::Nearest_MipmapNearest = CGL_TextureFilterMode::Nearest_MipmapNearest;
// const CGL_TextureFilterMode CGL_Texture::Linear_MipmapNearest = CGL_TextureFilterMode::Linear_MipmapNearest;
// const CGL_TextureFilterMode CGL_Texture::Nearest_MipmapLinear = CGL_TextureFilterMode::Nearest_MipmapLinear;
// const CGL_TextureFilterMode CGL_Texture::Linear_MipmapLinear = CGL_TextureFilterMode::Linear_MipmapLinear;

// Update flags

const unsigned int CGL_Texture::NEW_SAMPLER      =     1 << 0; 
const unsigned int CGL_Texture::NEW_RAWDATA      =     1 << 1;
const unsigned int CGL_Texture::NEW_FILEPATH     =     1 << 2;
const unsigned int CGL_Texture::NEW_DIMENSIONS   =     1 << 3;     

// create or recreate texture from rawdata. currently only supports 4channel RGBA and unsigned byte types
void CGL_Texture::SetRawData(
    std::vector<double> &ck_array, int texWidth, int texHeight,
    bool doCopy
)
{
    // copy tex params
    m_Width = texWidth; m_Height = texHeight;

    // vectorized transform from double to unsigned char
    if (doCopy) {  // don't copy on chuck side to avoid blocking audio thread
        m_DataBuffer.resize(ck_array.size());
        std::transform(
            ck_array.begin(), ck_array.end(), m_DataBuffer.begin(), 
            [](double val) {
                return static_cast<unsigned char>(val);
            }
        );
    }
}
