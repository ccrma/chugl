#pragma once

#include "chuck_def.h"
#include "chuck_dl.h"

#include <mutex>
#include <unordered_map>
#include <vector>

// TODO move into cpp
#include "imgui.h"
#include "glm/glm.hpp"


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
    Slider,
    Checkbox,
    Color3
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
    Element(Chuck_Object* event, const std::string& label = "") 
    : m_Event((Chuck_Event*)event), m_ReadDataLock(), m_Label(label) {}
    virtual ~Element() {}
    virtual void Draw() = 0;
    virtual Type GetType() { return Type::Element; }
	virtual const char * GetCkName() { return Manager::GetCkName( GetType() ); }

    void Broadcast() { Manager::Broadcast( m_Event ); }

    std::string& GetLabel() { return m_Label; }
    void SetLabel(const std::string& label) { m_Label = label; }

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

    virtual Type GetType() { return Type::Window; }

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
    int AddElement(Element* e) {
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

    virtual Type GetType() { return Type::Button; }

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

    virtual Type GetType() { return Type::Checkbox; }

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

private:
    bool m_ReadData;
    bool m_WriteData;
};

class Slider : public Element
{
public:
    Slider(
        Chuck_Object* event,
        float min = 0.0f,
        float max = 1.0f,
        float power = 1.0f
    ) : Element(event), m_Min(min), m_Max(max), m_Power(power), m_ReadData(min), m_WriteData(min) {
    }

    virtual Type GetType() { return Type::Slider; }

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

    virtual Type GetType() { return Type::Color3; }

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
private:
    float m_ReadData[3];
    float m_WriteData[3];
};


};  // end namespace GUI

