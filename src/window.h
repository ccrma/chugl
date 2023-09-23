#pragma once
#include "renderer/Includes.h"

class Window
{
public:
	Window(int viewWidth = 2*640, int viewHeight = 2*480);
	// Window(int viewWidth = 3840, int viewHeight = 2160);
	~Window();

	void DisplayLoop();
	
	void Terminate() { glfwTerminate(); }

	// set and get
	inline GLFWwindow* GetWindow() { return m_Window; }
	inline float GetDeltaTime() const { return m_DeltaTime; }
	inline int GetViewWidth() { return m_ViewWidth; }
	inline int GetViewHeight() { return m_ViewHeight; }
	void SetViewSize(int width, int height);
private:
	// member vars
	GLFWwindow* m_Window;
	int m_ViewWidth, m_ViewHeight;
	float m_DeltaTime = 0.0f;

};
