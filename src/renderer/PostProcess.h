#pragma once

// TODO move into .cpp
#include "Shader.h"
#include "scenegraph/chugl_postprocess.h"
#include "ShaderCode.h"

// Abstract base class for post-processing effects
// Renderer impl of chugl_postprocess API

class Renderer;

class PostProcessEffect
{
protected:
    // PostProcessEffect* m_Next;
    PP::Effect* m_Effect;
    Shader* m_Shader;
public:  // factory
    static PostProcessEffect* Create(PP::Effect* chugl_effect) {
        const std::string& screenShaderVert = ShaderCode::PP_VERT;
        std::string screenShaderFrag;

        // get fragment shader code based on pp type
        switch (chugl_effect->GetType()) {
        case PP::Type::Base:
            throw std::runtime_error("Cannot create PostProcessEffect from base class");
        case PP::Type::PassThrough:
            screenShaderFrag = ShaderCode::PP_PASS_THROUGH;
            break;
        case PP::Type::Output:
            screenShaderFrag = ShaderCode::PP_OUTPUT;
            break;
        default:
            throw std::runtime_error("Unknown PP::Type");
        }

        // return effect
        return new PostProcessEffect(
            chugl_effect,
            new Shader(screenShaderVert, screenShaderFrag, false, false)
        );
    }
private:  // hidden ctor
    PostProcessEffect(
        PP::Effect* effect, Shader* shader
    ) : m_Effect(effect), m_Shader(shader) {
        // assert all not nulll
        assert(effect);
        assert(shader);
    }
public:

    virtual ~PostProcessEffect() {
        if (m_Shader) delete m_Shader;
    }

    virtual void Apply(Renderer& renderer);

    PP::Effect* GetChuglEffect() { return m_Effect; }

    // TODO: abstract fn to generate a GUI for this effect
    // TODO: timer
    // TODO: bypass

public: // static

};


class OutputEffect : public PostProcessEffect
{
public:

private:

};