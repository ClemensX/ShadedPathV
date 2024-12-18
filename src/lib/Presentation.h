#pragma once

struct WindowInfo {
    int width = 800;
    int height = 600;
    const char* title = "Shaded Path Engine";
    GLFWwindow* glfw_window = nullptr;
};

class Presentation : public EngineParticipant
{
public:
	Presentation(ShadedPathEngine* s) {
		Log("Presentation c'tor\n");
        setEngine(s);
	};
	~Presentation();

	void createWindow(WindowInfo* winfo, int w, int h, const char* name,
		bool handleKeyEvents = true, bool handleMouseMoveEevents = true, bool handleMouseButtonEvents = true);

	// current input/output window
	WindowInfo* window = nullptr;

private:
    bool glfwInitCalled = false;
	// event callbacks, will be called from main thread (via Presentation::pollEvents):

	void callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods);
	void callbackMouseButton(GLFWwindow* window, int button, int action, int mods);
	void callbackCursorPos(GLFWwindow* window, double x, double y);
	static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
	// Input will be handled via one instance - application code needs to copy if needed, not referenced
	InputState inputState;
};

