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
#include <vector>


enum CGL_TextureType : t_CKUINT {
    Base = 0,
    File2D,
    RawData,
    Count
};

enum class CGL_TextureWrapMode : t_CKUINT {
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge
};

enum class CGL_TextureFilterMode : t_CKUINT {
    Nearest = 0,
    Linear,
    // Nearest_MipmapNearest,
    // Linear_MipmapNearest,
    // Nearest_MipmapLinear,
    // Linear_MipmapLinear
};


struct CGL_TextureSamplerParams {
    CGL_TextureWrapMode wrapS, wrapT;  // ST <==> UV
    CGL_TextureFilterMode filterMin, filterMag;
    bool genMipMaps;

    // default constructor
    CGL_TextureSamplerParams() : wrapS(CGL_TextureWrapMode::ClampToEdge), wrapT(CGL_TextureWrapMode::ClampToEdge),
        filterMin(CGL_TextureFilterMode::Linear), filterMag(CGL_TextureFilterMode::Linear),
        genMipMaps(true)
        {}
};


class CGL_Texture : public SceneGraphNode
{
public:
    // default constructor
    CGL_Texture(CGL_TextureType t = CGL_TextureType::Base) : 
        type(t), m_UpdateFlags(0),
        m_FilePath(""),
        m_Width(0), m_Height(0)
        {}
    
    // destructor
    virtual ~CGL_Texture() {
        // all state stored in structs and vectors, no need to do anything
    }

	virtual bool IsTexture() { return true; }

    CGL_Texture * Clone() {
        CGL_Texture * tex = new CGL_Texture(*this);
        tex->SetID(GetID());
        return tex;
    }

    bool NeedsUpdate() { return m_UpdateFlags != 0; }
    void ResetUpdateFlags() { m_UpdateFlags = 0; }

    bool HasNewSampler() { return m_UpdateFlags & CGL_Texture::NEW_SAMPLER; }
    bool HasNewRawData() { return m_UpdateFlags & CGL_Texture::NEW_RAWDATA; }
    bool HasNewFilePath() { return m_UpdateFlags & CGL_Texture::NEW_FILEPATH; }
    bool HasNewDimensions() { return m_UpdateFlags & CGL_Texture::NEW_DIMENSIONS; }

    bool SetNewSampler() { return m_UpdateFlags |= CGL_Texture::NEW_SAMPLER; }
    bool SetNewRawData() { return m_UpdateFlags |= CGL_Texture::NEW_RAWDATA; }
    bool SetNewFilePath() { return m_UpdateFlags |= CGL_Texture::NEW_FILEPATH; }
    bool SetNewDimensions() { return m_UpdateFlags |= CGL_Texture::NEW_DIMENSIONS; }


    // set texture params
    // performance note: it's faster to create 2 duplicate textures with different sampler params than to switch back and forth constantly
    void SetWrapMode(CGL_TextureWrapMode s, CGL_TextureWrapMode t) { 
        m_SamplerParams.wrapS = s; m_SamplerParams.wrapT = t; 
    }
    void SetGenMipMaps(bool gen) { m_SamplerParams.genMipMaps = gen; }
    void SetFilterMode(CGL_TextureFilterMode min, CGL_TextureFilterMode mag) { 
        m_SamplerParams.filterMin = min; m_SamplerParams.filterMag = mag; 
    }

    // File methods
    void SetFilePath(const std::string& path) { m_FilePath = path; }

    // RawData methods
    void SetRawData(
        std::vector<double>& ck_array, 
        int texWidth, 
        int texHeight,
        bool doCopy
    );

// member vars ==========================================================================================================
    CGL_TextureType type;

    // update flags. these are set in the UpdateTextureXXX commands, and reset by renderer after regenerating GPU data
    unsigned int m_UpdateFlags;
    const static unsigned int NEW_SAMPLER; // whether to reset texture sampling params
    const static unsigned int NEW_RAWDATA;   // whether to raw texture data buffer has changed
    const static unsigned int NEW_FILEPATH;   // whether filepath has changed
    const static unsigned int NEW_DIMENSIONS;     // whether texture dimensions have changed
    
    // DATA (TODO put in union or refactor this into multiple classes w/ polymorphism)
    std::string m_FilePath;                                        // for FileTexture
    std::vector<unsigned char> m_DataBuffer; int m_Width, m_Height; // for DataTexture
    

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