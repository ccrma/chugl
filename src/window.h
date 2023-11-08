#pragma once

#include "chugl_pch.h"

struct GLFWwindow;
class Scene;
class Renderer;

class Window
{
public:
	Window(int windowWidth = s_DefaultWindowWidth, int windowHeight = s_DefaultWindowHeight);
	~Window();

	void DisplayLoop();

	// set and get
	GLFWwindow* GetWindow() { return m_Window; }
	float GetDeltaTime() const { return m_DeltaTime; }
	int GetFrameWidth() { return m_FrameWidth; }
	int GetFrameHeight() { return m_FrameHeight; }
	int GetWindowWidth() { return m_WindowWidth; }
	int GetWindowHeight() { return m_WindowHeight; }
	void SetWindowSize(int width, int height);
	void SetFrameSize(int width, int height);

	void UpdateState(Scene& scene);
private:
	// member vars
	GLFWwindow* m_Window;
	int m_WindowWidth, m_WindowHeight;
	int m_FrameWidth, m_FrameHeight;
	float m_DeltaTime = 0.0f;

public: // statics
	static std::unordered_map<GLFWwindow*, Window*> s_WindowMap;
	static Window* GetWindow(GLFWwindow* window) { return s_WindowMap[window]; }
	static void SetWindow(GLFWwindow* window, Window* w) { s_WindowMap[window] = w; }

	static int s_DefaultWindowWidth, s_DefaultWindowHeight;

public:  // default GGens
    static Renderer renderer;
    static Scene scene;
};
