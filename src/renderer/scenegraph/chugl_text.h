#pragma once

#include "chugl_pch.h"
#include "SceneGraphObject.h"

class CHGL_Text: public SceneGraphObject
{
public:
    struct Params {
        std::string text;
        std::string fontPath;
        bool dirty;
        glm::vec4 color;
        glm::vec2 controlPoints;
        float lineSpacing;

        Params() : text(""), fontPath(""), dirty(false), color(1.0), controlPoints(0.5f), lineSpacing(1.0f) {}
    };
public:
    CHGL_Text() {}
    virtual ~CHGL_Text() {}

    virtual bool IsText() override { return true; }

    virtual CHGL_Text* Clone() override { return new CHGL_Text(*this); }

    const Params& GetParams() { return m_Params; }
    void SetParams(const Params& params) { m_Params = params; }

    void SetText(const std::string& text) { m_Params.text = text; }
    const std::string& GetText() { return m_Params.text; }

    void SetFontPath(const std::string& fontPath) { m_Params.fontPath = fontPath; }
    const std::string& GetFontPath() { return m_Params.fontPath; }

    bool GetDirty() { return m_Params.dirty; }
    void SetDirty(bool dirty) { m_Params.dirty = dirty; }

    const glm::vec4& GetColor() { return m_Params.color; }
    void SetColor(const glm::vec4& color) { m_Params.color = color; }

    const glm::vec2& GetControlPoints() { return m_Params.controlPoints; }
    void SetControlPoints(const glm::vec2& controlPoints) { m_Params.controlPoints = controlPoints; }

    float GetLineSpacing() { return m_Params.lineSpacing; }
    void SetLineSpacing(float lineSpacing) { m_Params.lineSpacing = lineSpacing; }

private:
    Params m_Params;

public:  // chuck type names
	// typedef std::unordered_map<Type, const std::string, EnumClassHash> CkTypeMap;
	// static CkTypeMap s_CkTypeMap;
	// static const char * CKName(Type type) { return s_CkTypeMap[type].c_str(); }
	// virtual const char * myCkName() { return CKName(GetType()); }

	static const char * CKName() { return "GText"; }
	virtual const char * myCkName() { return "GText"; }
};