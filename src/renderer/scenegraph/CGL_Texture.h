// Scenegraph texture class (separate from any GPU/renderer-specific impl)
#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"

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


enum class CGL_TextureType : t_CKUINT {
    Base = 0,
    File2D,
    RawData2D,
    CubeMap,
    Count
};

enum CGL_TextureWrapMode : t_CKUINT {
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge
};

enum CGL_TextureFilterMode : t_CKUINT {
    Nearest = 0,
    Linear,
    // Nearest_MipmapNearest,
    // Linear_MipmapNearest,
    // Nearest_MipmapLinear,
    // Linear_MipmapLinear
};

enum class CGL_TextureColorSpace : t_CKUINT {
    Linear = 0,
    sRGB = 1
};


// TODO: can this be extended to other texture types
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
protected:
    // update flags. these are set in the UpdateTextureXXX commands,
    // and reset by renderer after regenerating GPU data
    unsigned int m_UpdateFlags; 
    CGL_TextureSamplerParams m_SamplerParams;
    CGL_TextureColorSpace m_ColorSpace;
public:
    CGL_Texture() : m_UpdateFlags(0), m_ColorSpace(CGL_TextureColorSpace::Linear)  {}
    
    virtual ~CGL_Texture() {
        // all state stored in structs and vectors, no need to do anything
    }

	bool IsTexture() const { return true; }
    virtual CGL_TextureType GetTextureType() const { return CGL_TextureType::Base; }

    bool NeedsUpdate() { return m_UpdateFlags != 0; }
    void ResetUpdateFlags() { m_UpdateFlags = 0; }

public:  // update sampler params 
    void SetFilterMode(CGL_TextureFilterMode min, CGL_TextureFilterMode mag) { 
        m_SamplerParams.filterMin = min; m_SamplerParams.filterMag = mag; 
    }
    void SetWrapMode(CGL_TextureWrapMode s, CGL_TextureWrapMode t) { 
        m_SamplerParams.wrapS = s; m_SamplerParams.wrapT = t; 
    }
    void SetGenMipMaps(bool gen) { m_SamplerParams.genMipMaps = gen; }
    void SetSamplerParams(const CGL_TextureSamplerParams& params) { m_SamplerParams = params; }
    const CGL_TextureSamplerParams& GetSamplerParams() { return m_SamplerParams; }

public:  // color space
    void SetColorSpace(CGL_TextureColorSpace colorSpace) { m_ColorSpace = colorSpace; }
    CGL_TextureColorSpace GetColorSpace() { return m_ColorSpace; }

public: // update flags
    const static unsigned int NEW_SAMPLER; // whether to reset texture sampling params
    const static unsigned int NEW_RAWDATA;   // whether to raw texture data buffer has changed
    const static unsigned int NEW_FILEPATH;   // whether filepath has changed
    const static unsigned int NEW_DIMENSIONS;     // whether texture dimensions have changed
    const static unsigned int NEW_COLORSPACE;     // whether texture colorspace has changed

    bool HasNewSampler() { return m_UpdateFlags & CGL_Texture::NEW_SAMPLER; }
    bool HasNewRawData() { return m_UpdateFlags & CGL_Texture::NEW_RAWDATA; }
    bool HasNewFilePath() { return m_UpdateFlags & CGL_Texture::NEW_FILEPATH; }
    bool HasNewDimensions() { return m_UpdateFlags & CGL_Texture::NEW_DIMENSIONS; }
    bool HasNewColorSpace() { return m_UpdateFlags & CGL_Texture::NEW_COLORSPACE; }

    bool SetNewSampler() { return m_UpdateFlags |= CGL_Texture::NEW_SAMPLER; }
    bool SetNewRawData() { return m_UpdateFlags |= CGL_Texture::NEW_RAWDATA; }
    bool SetNewFilePath() { return m_UpdateFlags |= CGL_Texture::NEW_FILEPATH; }
    bool SetNewDimensions() { return m_UpdateFlags |= CGL_Texture::NEW_DIMENSIONS; }
    bool SetNewColorSpace() { return m_UpdateFlags |= CGL_Texture::NEW_COLORSPACE; }
    
// static constants (because pass enums as svars through chuck dll query is undefined)
    // wrap modes
    const static t_CKUINT Repeat; 
    const static t_CKUINT MirroredRepeat;
    const static t_CKUINT ClampToEdge;

    // filter modes
    // note: X_MipmapY uses X for filtering, and Y for MIPMAP selection
    // for now, mipmaps are always linearly interpolated
    const static t_CKUINT Nearest;
    const static t_CKUINT Linear;

    const static t_CKUINT ColorSpace_Linear;
    const static t_CKUINT ColorSpace_sRGB;

public: // chuck type names
    // TODO can probably template this and genarlize across all scenegraph classes?
    typedef std::unordered_map<CGL_TextureType, const std::string, EnumClassHash> CkTypeMap;
    static CkTypeMap s_CkTypeMap;
    static const char* CKName(CGL_TextureType type);
    virtual const char* myCkName() { return CKName(GetTextureType()); }
// member vars ==========================================================================================================
    
    // sampler options
    // TODO: image mapping type (https://threejs.org/docs/index.html#api/en/constants/Textures)
    // TODO: select UV channel?
};

class FileTexture2D : public CGL_Texture 
{
private:
    std::string m_FilePath;
    unsigned int m_Width, m_Height;

public:
    FileTexture2D() : CGL_Texture(), m_FilePath("") {}

    virtual FileTexture2D* Clone() override {
        FileTexture2D * tex = new FileTexture2D(*this);
        tex->SetID(GetID());
        return tex;
    }

    virtual CGL_TextureType GetTextureType() const override {
         return CGL_TextureType::File2D; 
    }

    void SetFilePath(const std::string& path) { m_FilePath = path; }
    const std::string& GetFilePath() { return m_FilePath; }
    
    void SetDimensions(int width, int height) { m_Width = width; m_Height = height; }
};

class DataTexture2D : public CGL_Texture 
{
private:
    std::vector<unsigned char> m_DataBuffer; 
    unsigned int m_Width, m_Height; // for DataTexture
public:
    DataTexture2D() : 
        CGL_Texture(),
        m_Width(0), 
        m_Height(0)
    {}

    virtual DataTexture2D* Clone() override {
        DataTexture2D* tex = new DataTexture2D(*this);
        tex->SetID(GetID());
        return tex;
    }

    virtual CGL_TextureType GetTextureType() const override {
         return CGL_TextureType::RawData2D; 
    }

    // RawData methods
    bool SetRawData(
        std::vector<unsigned char>& data, 
        int texWidth, 
        int texHeight,
        bool doCopy = false,
        bool doMove = false
    );

    void SetDimensions(int width, int height) { m_Width = width; m_Height = height; }

public: // member var getters
    const std::vector<unsigned char>& GetDataBuffer() { return m_DataBuffer; }
    unsigned int GetWidth() { return m_Width; }
    unsigned int GetHeight() { return m_Height; }
};

class CGL_CubeMap : public CGL_Texture
{
private:
    std::vector<std::string> m_FilePaths;
    
public:
    CGL_CubeMap() : CGL_Texture() {
        // initialize all faces to empty string
        m_FilePaths.resize(6, "");

        // custom sampler params currently not supported for cubemaps
        // otherwise would initialize here
    }

    void SetFilePaths(const std::vector<std::string>& faces) {
        assert(faces.size() == 6);
        m_FilePaths = faces;
    }
    std::vector<std::string>& GetFilePaths() { return m_FilePaths; }


    virtual CGL_TextureType GetTextureType() const override {
         return CGL_TextureType::CubeMap; 
    }

    virtual CGL_CubeMap * Clone() override {
        CGL_CubeMap * tex = new CGL_CubeMap(*this);
        tex->SetID(GetID());
        return tex;
    }
};
