#pragma once

#include "chugl_pch.h"
#include "imgui.h"

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
enum class Type : unsigned int 
{
    Element = 0,
    Window,
    Button,
    FloatSlider,
    IntSlider,
    Checkbox,
    Color3,
    Dropdown
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

    std::string& GetLabel() { return m_Label; }
    void SetLabel(const std::string& label);

protected:
    // void* m_ReadData;   // read by chuck thread, written to by render thread on widget update.
                        // not threadsafe, requires lock to read/write

    // void* m_WriteData;  // only ever written to by the Render thread, threadsafe
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
    void SetMin(float min) { m_Min = min; }

    float GetMax() { return m_Max; }
    void SetMax(float max) { m_Max = max; }

    float GetPower() { return m_Power; }
    void SetPower(float power) { m_Power = power; }

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
    void SetMin(float min) { m_Min = min; }

    float GetMax() { return m_Max; }
    void SetMax(float max) { m_Max = max; }

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
        if (data < 0) data = 0;
        if (data >= m_Options.size()) data = m_Options.size() - 1;
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


};  // end namespace GUI

