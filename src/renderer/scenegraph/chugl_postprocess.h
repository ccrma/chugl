#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"
#include "Material.h"  // used for UniformType enum. TODO refactor into separate file

namespace PP
{

enum class Type : t_CKUINT
{
    Base = 0,
    PassThrough,
    Output,
    Invert,
    Monochrome,
    Custom,
    Bloom
};

class Effect : public SceneGraphNode
{
public:
    typedef std::unordered_map<std::string, MaterialUniform> UniformMap;
protected:
    size_t m_NextID;
    bool m_Bypass;
    UniformMap m_Uniforms;

public:
    Effect() : m_NextID(0), m_Bypass(false) {};
    virtual ~Effect() {};

    // uniform map accessors
    UniformMap& GetUniforms() { return m_Uniforms; }
    MaterialUniform& GetUniform(const std::string& name) {
        return m_Uniforms[name];
    }
    bool HasUniform(const std::string& name) {
        return m_Uniforms.find(name) != m_Uniforms.end();
    }
    void SetUniform(const MaterialUniform& uniform);
    void ClearUniforms() { m_Uniforms.clear(); }

    // bypass
    bool GetBypass() { return m_Bypass; }
    void SetBypass(bool bypass) { m_Bypass = bypass; }
    bool* GetBypassPtr() { return &m_Bypass; }

    // effect chain accessors
    size_t GetNextID() { return m_NextID; }
    Effect* Next();
    Effect* NextEnabled();

    // add effect to chain
    void Add(Effect* next) { m_NextID = next ? next->GetID() : 0; }
    // remove next effect from chain
    void RemoveNext() { m_NextID = 0; }

public:  // virtual, override in derived classes
    virtual Type GetType() { return Type::Base; }

public:  // chuck type names
	typedef std::unordered_map<Type, const std::string, EnumClassHash> CkTypeMap;
	static CkTypeMap s_CkTypeMap;
	static const char * CKName(Type type) { return s_CkTypeMap[type].c_str(); }
	virtual const char * myCkName() { return CKName(GetType()); }

};

// Effect that does nothing 
class PassThroughEffect : public Effect
{
public:
    PassThroughEffect() : Effect() {};
    virtual Type GetType() override { return Type::PassThrough; }
    virtual PassThroughEffect* Clone() override { 
        return new PassThroughEffect(*this); 
    }
};

// Color Inverse
class InverseEffect : public Effect
{
public:
    InverseEffect() : Effect() {
        // set mix
        SetUniform(MaterialUniform::CreateFloat(U_MIX, 1.0f));
    };
    virtual Type GetType() override { return Type::Invert; }
    virtual InverseEffect* Clone() override { return new InverseEffect(*this); }

    float GetMix() { return GetUniform(U_MIX).f; }
public:
    static const std::string U_MIX;
};

class MonoChromeEffect : public Effect
{
public:
    MonoChromeEffect() : Effect() {
        // set mix
        SetUniform(MaterialUniform::CreateFloat(U_MIX, 1.0f));
        // set color
        SetUniform(MaterialUniform::CreateFloat3(U_COLOR, 1.0f, 1.0f, 1.0f));
    };
    virtual Type GetType() override { return Type::Monochrome; }
    virtual MonoChromeEffect* Clone() override { return new MonoChromeEffect(*this); }

    float GetMix() { return GetUniform(U_MIX).f; }
    glm::vec3 GetColor() { 
        auto& color = GetUniform(U_COLOR); 
        return glm::vec3(color.f3[0], color.f3[1], color.f3[2]);
    }
public:
    static const std::string U_MIX;
    static const std::string U_COLOR;
};

// Custom
class CustomEffect : public Effect
{
private:
    std::string m_ScreenShaderString;
    bool m_IsPath;
    bool m_RebuildShader;
public:
    CustomEffect() 
        : Effect(), 
        m_ScreenShaderString(""),  // default to passthrough shader 
        m_IsPath(false), 
        m_RebuildShader(false) 
    {};
    virtual Type GetType() override { return Type::Custom; }
    virtual CustomEffect* Clone() override { return new CustomEffect(*this); }

    void SetScreenShader(const std::string& screenShaderString, bool isPath) { 
        m_ScreenShaderString = screenShaderString; 
        m_IsPath = isPath;
        m_RebuildShader = true;
    }
    const std::string& GetScreenShader() { return m_ScreenShaderString; }
    bool IsPath() { return m_IsPath; }
    bool GetRebuildShader() { return m_RebuildShader; }
    void SetRebuildShader(bool rebuildShader) { m_RebuildShader = rebuildShader; }
};

// Output effect
class OutputEffect : public Effect
{
public:
    OutputEffect() : Effect() {
        // initialize default uniforms

        // TODO: can we auto-extrapolate this from shader annotations?
        // if not the ChuGL API impl at least the DLL query and UI auto-gen

        // tone mapping
        SetUniform(MaterialUniform::CreateInt(U_TONEMAP, TONEMAP_ACES));

        // exposure
        SetUniform(MaterialUniform::CreateFloat(U_EXPOSURE, 1.0f));

        // gamma correction
        SetUniform(MaterialUniform::CreateFloat(U_GAMMA, 2.2f));    
    };
    virtual Type GetType() override { return Type::Output; }
    virtual OutputEffect* Clone() override { return new OutputEffect(*this); }\

    // uniform getters
    int GetTonemap() { return GetUniform(U_TONEMAP).i; }
    float GetGamma() { return GetUniform(U_GAMMA).f; }
    float GetExposure() { return GetUniform(U_EXPOSURE).f; }

public: // uniform names
    static const std::string U_GAMMA;
    static const std::string U_TONEMAP;
    static const std::string U_EXPOSURE;

public:  // uniform constants
    // tonemap methods
    static const int TONEMAP_NONE;
    static const int TONEMAP_LINEAR;
    static const int TONEMAP_REINHARD;
    static const int TONEMAP_CINEON;
    static const int TONEMAP_ACES;
    static const int TONEMAP_UNCHARTED;

};

class BloomEffect : public Effect
{
public:
    BloomEffect() : Effect() {
        // default strength
        SetUniform(MaterialUniform::CreateFloat(U_STRENGTH, .04f));
        // filter radius
        SetUniform(MaterialUniform::CreateFloat(U_RADIUS, .01f));
        // threshold
        SetUniform(MaterialUniform::CreateFloat(U_THRESHOLD, .8f));
        // threshold knee
        // SetUniform(MaterialUniform::CreateFloat(U_THRESHOLD_KNEE, .5f));
        // levels
        SetUniform(MaterialUniform::CreateInt(U_LEVELS, 5));
        // blend mode
        SetUniform(MaterialUniform::CreateInt(U_BLEND_MODE, BLEND_ADD));

        // karis mode
        SetUniform(MaterialUniform::CreateInt(U_KARIS_MODE, 0));
        // karis enabled
        SetUniform(MaterialUniform::CreateBool(U_KARIS_ENABLED, false));
    };
    virtual Type GetType() override { return Type::Bloom; }
    virtual BloomEffect* Clone() override { return new BloomEffect(*this); }

    float GetStrength() { return GetUniform(U_STRENGTH).f; }
    float GetRadius() { return GetUniform(U_RADIUS).f; }
    float GetThreshold() { return GetUniform(U_THRESHOLD).f; }
    int GetLevels() { return GetUniform(U_LEVELS).i; }
    int GetBlendMode() { return GetUniform(U_BLEND_MODE).i; }
    // float GetThresholdKnee() { return GetUniform(U_THRESHOLD_KNEE).f; }
    bool GetKarisEnabled() { return GetUniform(U_KARIS_ENABLED).b; }
    // int GetKarisMode() { return GetUniform(U_KARIS_MODE).i; }

public:  // uniform names
    static const std::string U_STRENGTH;
    static const std::string U_RADIUS;
    static const std::string U_THRESHOLD;
    // static const std::string U_THRESHOLD_KNEE;
    static const std::string U_LEVELS;
    static const std::string U_BLEND_MODE;

    static const std::string U_KARIS_ENABLED;
    static const std::string U_KARIS_MODE;


public:  // uniform constants
    // TODO: support blend modes
    static const t_CKUINT BLEND_ADD;
    static const t_CKUINT BLEND_MIX;
};

// TODO: consolidates a series of PP effects into a chain
// optimizing collapsable effects into a single shader where possible
// TODO: profile how expensive a single passthrough is first to see how necessary this is
// class CHGL_PP_EffectChain {};

}; // namespace PP