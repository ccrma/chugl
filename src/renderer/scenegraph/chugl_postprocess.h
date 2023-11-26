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
    Output
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

        // gamma correction
        SetUniform(MaterialUniform::Create(U_GAMMA, 2.2f));
    };
    virtual Type GetType() override { return Type::Output; }
    virtual OutputEffect* Clone() override { return new OutputEffect(*this); }\

    // uniform getters
    float GetGamma() { return GetUniform(U_GAMMA).f; }

public: // uniform names
    static const std::string U_GAMMA;

};

// TODO: consolidates a series of PP effects into a chain
// optimizing collapsable effects into a single shader where possible
// TODO: profile how expensive a single passthrough is first to see how necessary this is
// class CHGL_PP_EffectChain {};

}; // namespace PP