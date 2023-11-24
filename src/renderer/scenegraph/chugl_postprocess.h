#pragma once

#include "chugl_pch.h"
#include "SceneGraphNode.h"

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
protected:
    size_t m_NextID;
    bool m_Bypass;
public:
    Effect() : m_NextID(0), m_Bypass(false) {};
    virtual ~Effect() {};

    bool GetBypass() { return m_Bypass; }
    void SetBypass(bool bypass) { m_Bypass = bypass; }

    size_t GetNextID() { return m_NextID; }
    Effect* Next();
    Effect* NextEnabled();

    void Add(Effect* next) { m_NextID = next ? next->GetID() : 0; }
    void Remove() { m_NextID = 0; }

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
    struct Params {
        float gamma;
    };
    Params m_Params;
public:
    OutputEffect(Params params) : Effect(), m_Params(params) {};
    virtual Type GetType() override { return Type::Output; }
    virtual OutputEffect* Clone() override { return new OutputEffect(*this); }
};

// TODO: consolidates a series of PP effects into a chain
// optimizing collapsable effects into a single shader where possible
// TODO: profile how expensive a single passthrough is first to see how necessary this is
// class CHGL_PP_EffectChain {};

}; // namespace PP