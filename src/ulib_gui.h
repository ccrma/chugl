#pragma once

#include "chugl_pch.h"
#include "imgui.h"
#include "imgui/misc/cpp/imgui_stdlib.h"

/*=====================================================

Architecture Overview: ----

On the query side, all UI_XXX types extend the Chuck Event class.

All UI widgets extend a base Element class, that contains 
a pointer to their corresponding ChucK event.

All UI widgets are added to a Window class, which represents
a single ImGUI window and contains a list of child elements, which are
renderered in the order they are added.

The GUI manager class holds a list of all windows, and is responsible
for providing threadsafe access to the list of windows, and for drawing.

Thje GUI Manager exposes a draw() function that is called in the `draw_imgui()`
function in `window.cpp`

Data: ---- 

Each widget stores 2 copies of its data, in fields called `m_ReadData` and `m_WriteData`.
`m_ReadData` is read by the chuck thread, and written to by the render thread.
`m_WriteData` is read by the render thread when the widget is drawn, and written to by the chuck thread.


Thread Safety: ----

There are 2 tiers of locks. 1 global lock, accessed via Manager::GetWindowLock(), which must
be grabbed when:
- renderer is drawing, ie walking the list of windows
- chuck thread is adding a new window or adding an element to an existing window
- chuck thread is writing to an element's data

There is also a lock for each element, accessed via Element::m_ReadDataLock, which must be grabbed when:
- audio thread wants to access a widget's m_ReadData
- the widget has been modified in the UI, and the render thread is copying m_WriteData to m_ReadData

On widget update: ----

When a widget is updated via the user interacting with the UI, all widgets take the following steps:
- grab the local m_ReadDataLock
- copy m_WriteData to m_ReadData
- unlock the local m_ReadDataLock
- broadcast the corresponding chuck event

Performance: ----

The purpose of having these 2 tiers of locks is to reduce contention on the global lock.
Reasoning: a UI window needs to be drawn every frame, but rarely needs to be modified / update data.
Having the per-widget m_ReadDataLock allows the chuck audio thread to read a widget's data without
having to grab the global lock, which is grabbed by the render thread every frame and will be held for a 
duration that scales with the complexity of the UI.
The m_ReadDataLock, in contrast, is only held for the duration it takes to read/copy a single widget's data field.

Setting widget values in code: ----

Lastly, the chuck thread may want to occasionally set widget values in code (e.g. to set a default value).
Such cases should be rare, e.g. a default value will only need to be set once per widget before the UI is drawn.
In this case, the chuck thread has to grab the global lock.

======================================================*/

t_CKBOOL init_chugl_gui(Chuck_DL_Query *QUERY);

// GUI classes ================================================================

namespace GUI 
{

// forward decls
class Window; 

struct EnumClassHash
{
	template <typename T>
	std::size_t operator()(T t) const
	{
		return static_cast<std::size_t>(t);
	}
};

//-----------------------------------------------------------------------------
// name: Type
// desc: Type enums for GUI elements
//-----------------------------------------------------------------------------
enum class Type : t_CKUINT 
{
    Element = 0,
    Window,
    Button,
    FloatSlider,
    IntSlider,
    Checkbox,
    Color3,
    Dropdown,
    Text,
    InputText,
    Custom
};

//-----------------------------------------------------------------------------
// name: Manager 
// desc: Static class that holds all GUI elements
//-----------------------------------------------------------------------------
class Manager 
{
private:  // member vars
    static std::vector<Window*> s_Windows;
    static std::mutex s_WindowsLock;  // lock used to access s_Windows
                                      // grabbed by audio thread when creating new UI windows or adding elements to existing windows
                                      // grabbed by render thread every frame during imgui draw
    static CBufferSimple* s_SharedEventQueue;  // locksafe circular buffer for GUI events
    static const Chuck_DL_Api* s_CKAPI;        // reference to chugins runtime API
    static Chuck_VM* s_CKVM;               // reference to chuck VM

    typedef std::unordered_map<Type, const std::string, EnumClassHash> CkTypeMap;
    static CkTypeMap s_CkTypeMap;

public:
    static void DrawGUI();  // draws all windows and their children

    // chuck API setters
    static void SetEventQueue(CBufferSimple* queue) { s_SharedEventQueue = queue; }
    static void SetCKAPI( const Chuck_DL_Api* api ) { s_CKAPI = api; }
    static void SetCKVM( Chuck_VM* vm ) { s_CKVM = vm; }

    // event fns
    static void Broadcast( Chuck_Event* ckevent ) {
        s_CKAPI->vm->queue_event(s_CKVM, ckevent, 1, s_SharedEventQueue);
    }

    // threadsafe add window to manager
    static void AddWindow(Window* window);

    // getters
    static std::mutex& GetWindowLock() { return s_WindowsLock; }
    static CBufferSimple* GetEventQueue() { return s_SharedEventQueue; }
	static const char * GetCkName(Type t) { return s_CkTypeMap[t].c_str(); }

    // static std::string& GetCkName();

};

//-----------------------------------------------------------------------------
// name: Element
// desc: abstract GUI element base 
//-----------------------------------------------------------------------------
class Element
{
public:
    Element(Chuck_Object* event, const std::string& label = " ") 
    : m_Event((Chuck_Event*)event), m_ReadDataLock(), m_Label(label) {}
    virtual ~Element() {}
    virtual void Draw() = 0; // assume we are holding Manager::GetWindowLock() when this is called
    virtual Type GetType() { return Type::Element; }
	virtual const char * GetCkName() { return Manager::GetCkName( GetType() ); }

    void Broadcast() { Manager::Broadcast( m_Event ); }

    virtual std::string& GetLabel() { 
        // threadsafe assuming 
        // 1. SetLabel grabs window manager lock
        // 2. Render thread also grabs this lock when drawing (ie reading label)
        // 3. label is only ever set by audio-thread via UI_Element.text(...) API
        return m_Label; 
    }
    virtual void SetLabel(const std::string& label);

protected:
    std::string m_Label;  // name of element

    std::mutex m_ReadDataLock;  // lock to provide read/write access to m_ReadData
                                // grabbed by audio thread to access readData
                                // grabbed by render thread when imgui widget has new value, to copy from m_WriteData to m_ReadData

    Chuck_Event* m_Event;       // event to signal chuck thread that m_ReadData has been updated
                                // and is ready to be read
    

};

class Window : public Element
{
public:
    Window(Chuck_Object* event) 
        : Element(event), m_Open(true) {}

    virtual Type GetType() override { return Type::Window; }

    virtual void Draw() override {
        if (!m_Open) return;  // window closed

        if (!ImGui::Begin(m_Label.c_str(), &m_Open)) goto cleanup;

        // window is open, draw rest of elements in DFS order
        for (auto& e : m_Elements) {
            e->Draw();
        }

cleanup:    
        ImGui::End();
    }

    // threadsafe add an element to the window. returns number of elements in window after add
    size_t AddElement(Element* e) {
        // grab the window lock (in case render thread is currently drawing this window)
        // NOTE: could prob improve performance by having a separate lock for each window
        // but prob won't matter for UI construction
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Elements.push_back(e);
        return m_Elements.size();
    }
    

private:
    bool m_Open;
    std::vector<Element*> m_Elements;
};


class Button : public Element
{
public:
    Button(
        Chuck_Object* event,
        const ImVec2& size = ImVec2(0,0)
    ) :  Element(event), m_Size(size) {}

    virtual Type GetType() override { return Type::Button; }

    virtual void Draw() override {
        if (ImGui::Button(m_Label.c_str(), m_Size)) {
            // broadcast chuck event
            Broadcast();
        }
    }

private:
    ImVec2 m_Size;
};

class Checkbox : public Element
{
public:
    Checkbox(
        Chuck_Object* event
    ) : Element(event), m_ReadData(false), m_WriteData(false) {}

    virtual Type GetType() override { return Type::Checkbox; }

    virtual void Draw() override {
        if (ImGui::Checkbox(m_Label.c_str(), &m_WriteData)) {
            // lock
            std::unique_lock<std::mutex> lock(m_ReadDataLock);
            // copy
            m_ReadData = m_WriteData;
            // unlock
            lock.unlock();
            // broadcast chuck event
            Broadcast();
        }
    }

    bool GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_ReadData;
    }

    void SetData(bool data) {
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // set
        m_WriteData = data;
        // copy to readData
        m_ReadData = m_WriteData;
    }

private:
    bool m_ReadData;
    bool m_WriteData;
};

class FloatSlider : public Element
{
public:
    FloatSlider(
        Chuck_Object* event,
        float min = 0.0f,
        float max = 1.0f,
        float power = 1.0f
    ) : Element(event), m_Min(min), m_Max(max), m_Power(power), m_ReadData(min), m_WriteData(min) {
    }

    virtual Type GetType() override { return Type::FloatSlider; }

    virtual void Draw() override {
        if (ImGui::SliderFloat(m_Label.c_str(), &m_WriteData, m_Min, m_Max, "%.3f", m_Power)) {
            // lock
            std::unique_lock<std::mutex> lock(m_ReadDataLock);
            // copy
            m_ReadData = m_WriteData;
            // unlock
            lock.unlock();
            // broadcast chuck event
            Broadcast();
        }
    }

    float GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_ReadData;
    }

    void SetData(float data) {
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // set with bounds check
        m_WriteData = glm::clamp(data, m_Min, m_Max);
        // copy to read data
        m_ReadData = m_WriteData;
    }

    float GetMin() { return m_Min; }
    void SetMin(float min) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Min = min; 
    }

    float GetMax() { return m_Max; }
    void SetMax(float max) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Max = max; 
    }

    float GetPower() { return m_Power; }
    void SetPower(float power) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Power = power; 
    }

private:
    float m_Min;
    float m_Max;
    float m_Power;
    float m_ReadData;
    float m_WriteData;
};

class IntSlider : public Element
{
public:
    IntSlider(
        Chuck_Object* event,
        int min = 0,
        int max = 10
    ) : Element(event), m_Min(min), m_Max(max), m_ReadData(min), m_WriteData(min) {
    }

    virtual Type GetType() override { return Type::FloatSlider; }

    virtual void Draw() override {
        if (ImGui::SliderInt(m_Label.c_str(), &m_WriteData, m_Min, m_Max)) {
            // lock
            std::unique_lock<std::mutex> lock(m_ReadDataLock);
            // copy
            m_ReadData = m_WriteData;
            // unlock
            lock.unlock();
            // broadcast chuck event
            Broadcast();
        }
    }

    float GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_ReadData;
    }

    void SetData(int data) {
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // set with bounds check
        m_WriteData = glm::clamp(data, m_Min, m_Max);
        // copy to read data
        m_ReadData = m_WriteData;
    }

    float GetMin() { return m_Min; }
    void SetMin(float min) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Min = min; 
    }

    float GetMax() { return m_Max; }
    void SetMax(float max) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Max = max; 
    }

private:
    int m_Min;
    int m_Max;
    int m_ReadData;
    int m_WriteData;
};


class Color3 : public Element
{
public:
    Color3(
        Chuck_Object* event
    ) : Element(event) {
        m_ReadData[0] = 0.0f;
        m_ReadData[1] = 0.0f;
        m_ReadData[2] = 0.0f;
        m_WriteData[0] = 0.0f;
        m_WriteData[1] = 0.0f;
        m_WriteData[2] = 0.0f;
    }

    virtual Type GetType() override { return Type::Color3; }

    virtual void Draw() override {
        if (ImGui::ColorEdit3(m_Label.c_str(), &m_WriteData[0])) {
            // lock
            std::unique_lock<std::mutex> lock(m_ReadDataLock);
            // copy
            m_ReadData[0] = m_WriteData[0];
            m_ReadData[1] = m_WriteData[1];
            m_ReadData[2] = m_WriteData[2];
            // unlock
            lock.unlock();
            // broadcast chuck event
            Broadcast();
        }
    }

    glm::vec3 GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return glm::vec3(m_ReadData[0], m_ReadData[1], m_ReadData[2]);
    }

    void SetData(t_CKVEC3& data) {
        // lock (to modify writeData need to lock UI-wide window lock)
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // bounds check, clamp all values between 0 and 1
        m_WriteData[0] = glm::clamp(data.x, 0.0, 1.0);
        m_WriteData[1] = glm::clamp(data.y, 0.0, 1.0);
        m_WriteData[2] = glm::clamp(data.z, 0.0, 1.0);
        // copy to readData
        m_ReadData[0] = m_WriteData[0];
        m_ReadData[1] = m_WriteData[1];
        m_ReadData[2] = m_WriteData[2];
    }

private:
    float m_ReadData[3];
    float m_WriteData[3];
};

class Dropdown : public Element
{
public:
    Dropdown(
        Chuck_Object* event
    ) : Element(event), m_WriteData(0), m_ReadData(0) {}

    virtual Type GetType() override { return Type::Color3; }

    virtual void Draw() override {
        if (m_Options.empty()) return;  // no options to draw

        if (ImGui::BeginCombo(m_Label.c_str(), m_Options[m_WriteData].c_str()))
        {
            for (int n = 0; n < m_Options.size(); n++)
            {
                const bool is_selected = (m_WriteData == n);
                // item selected
                if (ImGui::Selectable(m_Options[n].c_str(), is_selected)) {
                    m_WriteData = n;
                    // lock
                    std::unique_lock<std::mutex> lock(m_ReadDataLock);
                    // copy
                    m_ReadData = m_WriteData;
                    // unlock
                    lock.unlock();
                    // broadcast chuck event
                    Broadcast();
                }

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    int GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        return m_ReadData;
    }
    int SetData(int data) {
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // bounds check
        if (data >= m_Options.size()) data = m_Options.size() - 1;
        if (data < 0) data = 0;
        m_WriteData = data;
        // copy to read data too
        m_ReadData = m_WriteData;
        return data;
    }

    void SetOptions(const std::vector<std::string>& options) { 
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_Options = options; 
    }

private:
    // data is the index of the selected option
    int m_ReadData;
    int m_WriteData;
    std::vector<std::string> m_Options;
};



/* 
UI_Text API 

Supported ImGUI text functionality: 
    Text(const char* fmt, ...)                                      IM_FMTARGS(1); // formatted text
    TextColored(const ImVec4& col, const char* fmt, ...)            IM_FMTARGS(2); // shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor();
    TextWrapped(const char* fmt, ...)                               IM_FMTARGS(1); // shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();. Note that this won't work on an auto-resizing window if there's no other widgets to extend the window width, yoy may need to set a size using SetNextWindowSize().
    BulletText(const char* fmt, ...)                                IM_FMTARGS(1); // shortcut for Bullet()+Text()
    SeparatorText(const char* label);                               // currently: formatted text with an horizontal line

    // TODO: how do we reconcile this with label...
    // have both .text() and .label()
    // LabelText currently unsupported
    LabelText(const char* label, const char* fmt, ...)              IM_FMTARGS(2); // display text+label aligned the same way as value+label widgets

Uses Element base class m_Label to store text.

Locking:
Because text is not an interactive UI element, if it is ever set, it's probably going to 
be from the chuck thread. Therefore to reduce lock contention, the m_ReadDataLock is 
repurposed here to be grabbed by the chuck thread when setting the text, and by the render
thread every frame when drawing
- there is no read/write data that needs copying, so m_ReadDataLock doesn't need to serve its original 
  purpose
- render thread has to grab the m_ReadDataLock every frame instead of only when the data is modified
- to change text, chuck thread only has to grab the m_ReadDataLock instead of the global window lock,
  making the act of changing text less expensive
- net tradeoff: the cost of drawing the UI is a tiny bit more expensive for the render thread
  but the cost of changing text is a lot less expensive for audio thread
*/

class Text : public Element
{
public:
    // text mode enums
    enum Mode : t_CKUINT {
        Default = 0,
        Bullet = 1,
        Separator = 2
    };
public:
    Text(
        Chuck_Object* event
    ) : Element(event), m_Color({1.0, 1.0, 1.0, 1.0}),
        m_Wrap(true), m_Mode(Mode::Default), m_Text(" ")
     {}

    virtual Type GetType() override { return Type::Text; }

    virtual void Draw() override {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);

        // push color state
        ImGui::PushStyleColor(ImGuiCol_Text, m_Color);

        // push wrap to window edge
        if (m_Wrap) ImGui::PushTextWrapPos(0.0f);

        // draw text
        if (m_Mode == Mode::Default)             ImGui::Text("%s", m_Text.c_str());
        else if (m_Mode == Mode::Bullet)         ImGui::BulletText("%s", m_Text.c_str());
        else if (m_Mode == Mode::Separator)      ImGui::SeparatorText(m_Text.c_str());

        // pop wrap state
        if (m_Wrap) ImGui::PopTextWrapPos();

        // pop color state
        ImGui::PopStyleColor();
    }

    const std::string& GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_Text;
    }

    void SetData(const std::string& text) {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // set new text
        m_Text = text.empty() ? " " : text;
    }

    void SetColor(const glm::vec4& color) {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // set new color
        m_Color = {color.r, color.g, color.b, color.a};
    }

    glm::vec4 GetColor() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return {m_Color.x, m_Color.y, m_Color.z, m_Color.w};
    }

    void SetWrap(bool wrap) {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // set new wrap
        m_Wrap = wrap;
    }

    bool GetWrap() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_Wrap;
    }

    void SetMode(Mode mode) {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // set new mode
        m_Mode = mode;
    }

    Mode GetMode() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_Mode;
    }

public: // static const modes
    static const t_CKUINT DefaultText;
    static const t_CKUINT BulletText;
    static const t_CKUINT SeparatorText;

private:
    // std::mutex m_ReadDataLock;  // lock to provide read/write access to m_ReadData
    std::string m_Text;
    ImVec4 m_Color;
    bool m_Wrap;
    Mode m_Mode;
};

// Custom element class
// Currently used by PostProcessors to generate custom GUI controls
// Assumed to be READ ONLY to chuck thread, does not offer any way to set data
template <typename T>
class Custom : public Element
{
public:
    Custom(
        Chuck_Object* event,
        const std::function<void(Custom*, T&)>& drawFn,
        T data
    ) :  Element(event), m_DrawFn(drawFn), m_Data(data) {}

    virtual Type GetType() override { return Type::Custom; }

    virtual void Draw() override {
        // no need to lock because only ever accessed by render thread
        m_DrawFn(this, m_Data);
    }
private:
    // custom draw function which takes current Custom element as argument
    std::function<void(Custom*, T&)> m_DrawFn;
    T m_Data;
};

class InputText : public Element
{
public:
    InputText(
        Chuck_Object* event
    ) : Element(event), m_ReadData(" "), m_WriteData(" "),
    m_MultiLineInputSize(-1.0, 16.0f),
    m_IsMultiLine(false), m_BroadcastOnEnter(false)
    {}

    virtual Type GetType() override { return Type::InputText; }

    virtual void Draw() override {
        // draw label
        ImGui::Text("%s", m_Label.c_str());

        ImGuiInputTextFlags input_text_flags = (
            m_BroadcastOnEnter ? ImGuiInputTextFlags_EnterReturnsTrue : 0
            // | ImGuiInputTextFlags_AllowTabInput  // TODO: tabs are bugged on GText rendering
            // | ImGuiInputTextFlags_EscapeClearsAll
        );
        // draw text
        bool inputTextReturn = false;
        if (m_IsMultiLine) {
			// get input box size as multiple of text line height
            float inputWidth = m_MultiLineInputSize.x <= 0 ? ImGui::GetWindowWidth() : ImGui::GetTextLineHeight() * m_MultiLineInputSize.x;
            float inputHeight = m_MultiLineInputSize.y <= 0 ? ImGui::GetWindowHeight() : ImGui::GetTextLineHeight() * m_MultiLineInputSize.y;
            inputTextReturn = ImGui::InputTextMultiline(
                m_Label.c_str(), &m_WriteData, ImVec2(inputWidth, inputHeight), input_text_flags
            );
        } else {
            inputTextReturn = ImGui::InputText(m_Label.c_str(), &m_WriteData, input_text_flags);
        }

        if (inputTextReturn) {
            // lock
            std::unique_lock<std::mutex> lock(m_ReadDataLock);
            // copy
            m_ReadData = m_WriteData;
            // unlock
            lock.unlock();
            // broadcast chuck event
            Broadcast();
        }
    }

    const std::string& GetData() {
        // lock
        std::lock_guard<std::mutex> lock(m_ReadDataLock);
        // return
        return m_ReadData;
    }

    void SetData(const std::string& text) {
        // lock
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        // set new text
        m_WriteData = text.empty() ? " " : text;
        // copy to read data
        m_ReadData = m_WriteData;
    }

    // only ever read by render thread, safe to read without lock by audio thread
    bool GetMultiLine() { return m_IsMultiLine; }

    // set by audio thread, read by render thread, therefore needs lock
    void SetMultiLine(bool multiLine) { 
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_IsMultiLine = multiLine; 
    }

    glm::vec2 GetMultiLineInputSize() { return m_MultiLineInputSize; }
    void SetMultiLineInputSize(const glm::vec2& size) { 
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_MultiLineInputSize = size; 
    }

    bool GetBroadcastOnEnter() { return m_BroadcastOnEnter; }
    void SetBroadcastOnEnter(bool broadcastOnEnter) { 
        std::lock_guard<std::mutex> lock(Manager::GetWindowLock());
        m_BroadcastOnEnter = broadcastOnEnter; 
    }

private:
    glm::vec2 m_MultiLineInputSize;  // size of text input if in multiline mode
    bool m_IsMultiLine;
    bool m_BroadcastOnEnter;
    std::string m_ReadData;
    std::string m_WriteData;
};

};  // end namespace GUI