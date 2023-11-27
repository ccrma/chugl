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
    void SetUniform(const MaterialUniform& uniform);

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

// Output effect
class OutputEffect : public Effect
{
public:
    OutputEffect() : Effect() {
        // initialize default uniforms

        // TODO: can we auto-extrapolate this from shader annotations?
        // if not the ChuGL API impl at least the DLL query and UI auto-gen

        // tone mapping
        SetUniform(MaterialUniform::CreateInt(U_TONEMAP, TONEMAP_LINEAR));

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
    };
    virtual Type GetType() override { return Type::Bloom; }
    virtual BloomEffect* Clone() override { return new BloomEffect(*this); }

    float GetStrength() { return GetUniform(U_STRENGTH).f; }
    float GetRadius() { return GetUniform(U_RADIUS).f; }

public:  // uniform names
    static const std::string U_STRENGTH;
    static const std::string U_RADIUS;

public:  // uniform constants

};

// TODO: consolidates a series of PP effects into a chain
// optimizing collapsable effects into a single shader where possible
// TODO: profile how expensive a single passthrough is first to see how necessary this is
// class CHGL_PP_EffectChain {};

}; // namespace PP