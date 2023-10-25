#pragma once

#include "chugl_pch.h"

struct GLFWwindow;
class Scene;
class Renderer;

class Window
{
public:
	Window(int viewWidth = 2*640, int viewHeight = 2*480);
	// Window(int viewWidth = 3840, int viewHeight = 2160);
	~Window();

	void DisplayLoop();

	// set and get
	inline GLFWwindow* GetWindow() { return m_Window; }
	inline float GetDeltaTime() const { return m_DeltaTime; }
	inline int GetViewWidth() { return m_ViewWidth; }
	inline int GetViewHeight() { return m_ViewHeight; }
	void SetViewSize(int width, int height);

	void UpdateState();
private:
	// member vars
	GLFWwindow* m_Window;
	int m_ViewWidth, m_ViewHeight;
	float m_DeltaTime = 0.0f;

public: // statics
	static std::unordered_map<GLFWwindow*, Window*> s_WindowMap;
	static Window* GetWindow(GLFWwindow* window) { return s_WindowMap[window]; }
	static void SetWindow(GLFWwindow* window, Window* w) { s_WindowMap[window] = w; }

public:  // default GGens
    static Renderer renderer;
    static Scene scene;
};
