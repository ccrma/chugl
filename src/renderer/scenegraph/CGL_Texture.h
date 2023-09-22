// Scenegraph texture class (separate from any GPU/renderer-specific impl)
#pragma once

/*
Goals

1. load texture from file
2. load texture from raw array data

Params:
- wrap mode in each dimension (s,t,r) <==> (x, y, z)
    - repeat, mirrored repeat, clamp to edge, clamp to border
        - clamp_to_border needs to specify border color
            - impl: 
            float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);  
    - impl: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
- filter mode in each dimension (s,t,r) <==> (x, y, z)
    - nearest and linear
    - min filter and mag filter
    - glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
- mipmaps
    - generate or not generate
        - imple: GLCall(glGenerateMipmap(GL_TEXTURE_2D));
    - mipmap filtering (only makes sense to set on GL_TEXTURE_MIN_FILTER, because mipmaps are only when surfac is far away, and texture needs to be minified)
        - nearest_mipmap_nearest, linear_mipmap_nearest, nearest_mipmap_linear, linear_mipmap_linear
            X_mipmap_Y uses X for filtering, and Y for MIPMAP selection
            e.g. NEAREST_MIPMAP_LINEAR will linear interpolate between mipmaps, and use pixelated nearest filtering on that result
        - impl: glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
*/

#include "SceneGraphNode.h"

#include "chuck_def.h"

#include <string>


// typedef t_CKUINT CGL_TextureWrapMode;
// typedef t_CKUINT CGL_TextureFilterMode;

// wrap mode enums
enum class CGL_TextureWrapMode : t_CKUINT {
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge
};

// filter mode
enum class CGL_TextureFilterMode : t_CKUINT {
    Nearest = 0,
    Linear,
    // Nearest_MipmapNearest,
    // Linear_MipmapNearest,
    // Nearest_MipmapLinear,
    // Linear_MipmapLinear
};

enum class CGL_TextureType {
    Base = 0,
    Texture2D,
    TextureRawData,
    TextureCubeMap
};

struct CGL_TextureSamplerParams {
    CGL_TextureWrapMode wrapS, wrapT;  // ST <==> UV
    CGL_TextureFilterMode filterMin, filterMag;
    bool genMipMaps;
};


class CGL_Texture : public SceneGraphNode
{
public:
    // default constructor
    CGL_Texture() : m_FilePath(""), m_ImgBuffer(nullptr),
        m_Width(0), m_Height(0), 
        m_SamplerParams({CGL_TextureWrapMode::ClampToEdge, CGL_TextureWrapMode::ClampToEdge,
            CGL_TextureFilterMode::Linear, CGL_TextureFilterMode::Linear,
            true})
        {}

    // create texture from file
    CGL_Texture(const std::string& path) : 
        m_FilePath(path), m_ImgBuffer(nullptr),
        m_Width(0), m_Height(0),
        m_SamplerParams({CGL_TextureWrapMode::ClampToEdge, CGL_TextureWrapMode::ClampToEdge,
            CGL_TextureFilterMode::Linear, CGL_TextureFilterMode::Linear,
            true})
        {}

    // build texture from raw data buffer
    // TODO: values need to be converted to [0, 255] range
    CGL_Texture(int texWidth, int texHeight, unsigned char* texBuffer) :
        m_FilePath(""), m_ImgBuffer(texBuffer),
        m_Width(texWidth), m_Height(texHeight),
        m_SamplerParams({CGL_TextureWrapMode::ClampToEdge, CGL_TextureWrapMode::ClampToEdge,
            CGL_TextureFilterMode::Linear, CGL_TextureFilterMode::Linear,
            true})
        {}

    virtual ~CGL_Texture() {
        std::cerr << "entring CGL_Texture destructor" << std::endl;
        if (m_ImgBuffer) {
            std::cerr << "deleting m_ImgBuffer" << std::endl;
            delete[] m_ImgBuffer;
        }
    };

    virtual CGL_TextureType GetTextureType() { return CGL_TextureType::Texture2D; }  // TODO: refactor to make this abstract base class

    CGL_Texture * Clone() {
        CGL_Texture * tex = new CGL_Texture(*this);
        tex->SetID(GetID());
        // TODO: handle m_ImgBuffer memory. 
        // for now not a problem bc chuck doesn't support constructors yet or passing array types through DLL interface
        return tex;
    }

    bool NeedsUpdate() { return m_NewSampler || m_NewData || m_NewFilePath; }
    void ResetUpdateFlags() { m_NewSampler = false; m_NewData = false; m_NewFilePath = false; }
    bool HasNewSampler() { return m_NewSampler; }
    bool HasNewData() { return m_NewData; }
    bool HasNewFilePath() { return m_NewFilePath; }


    // set texture params
    // performance note: it's faster to create 2 duplicate textures with different sampler params than to switch back and forth constantly
    void SetWrapMode(CGL_TextureWrapMode s, CGL_TextureWrapMode t) { 
        m_SamplerParams.wrapS = s; m_SamplerParams.wrapT = t; 
    }
    void SetGenMipMaps(bool gen) { m_SamplerParams.genMipMaps = gen; }
    void SetFilterMode(CGL_TextureFilterMode min, CGL_TextureFilterMode mag) { 
        m_SamplerParams.filterMin = min; m_SamplerParams.filterMag = mag; 
    }
    void SetFilePath(const std::string& path) { m_FilePath = path; }
    void SetRawData(int texWidth, int texHeight, unsigned char* texBuffer) { m_Width = texWidth; m_Height = texHeight; m_ImgBuffer = texBuffer; }




// member vars ==========================================================================================================
    // update flags. these are set in the UpdateTextureXXX commands, and reset by renderer after regenerating GPU data
    bool m_NewSampler; // whether to reset texture sampling params
    bool m_NewData;   // whether to raw texture data buffer has changed
    bool m_NewFilePath;   // whether filepath has changed
    
    std::string m_FilePath;
    unsigned char * m_ImgBuffer;
    int m_Width, m_Height;

    // sampler options
    // TODO: image mapping type (https://threejs.org/docs/index.html#api/en/constants/Textures)
    // TODO: select UV channel?
    CGL_TextureSamplerParams m_SamplerParams;

// static constants (because pass enums as svars through chuck dll query is undefined)
    // wrap modes
    const static CGL_TextureWrapMode Repeat; 
    const static CGL_TextureWrapMode MirroredRepeat;
    const static CGL_TextureWrapMode ClampToEdge;

    // filter modes
    // note: X_MipmapY uses X for filtering, and Y for MIPMAP selection
    const static CGL_TextureFilterMode Nearest;
    const static CGL_TextureFilterMode Linear;
    const static CGL_TextureFilterMode Nearest_MipmapNearest;
    const static CGL_TextureFilterMode Linear_MipmapNearest;
    const static CGL_TextureFilterMode Nearest_MipmapLinear;
    const static CGL_TextureFilterMode Linear_MipmapLinear;
};