#include "mainheader.h"

using namespace std;

Presentation::~Presentation()
{
	Log("Presentation destructor\n");
    if (glfwInitCalled) {
        glfwTerminate();
    }
}

void Presentation::createWindow(WindowInfo* winfo, int w, int h, const char* name,
    bool handleKeyEvents, bool handleMouseMoveEevents, bool handleMouseButtonEvents)
{
    if (!glfwInitCalled) {
        if (!glfwInit()) {
            Error("GLFW init failed\n");
        }
        glfwInitCalled = true;
    }
    // Get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

    // Get the video mode of the primary monitor
    const GLFWvidmode* videoMode = glfwGetVideoMode(primaryMonitor);

    // Retrieve the desktop size
    int desktopWidth = videoMode->width;
    int desktopHeight = videoMode->height;

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    auto window = glfwCreateWindow(w, h, name, nullptr, nullptr);
    winfo->glfw_window = window;
    if (engine->isDebugWindowPosition()) {
        // Set window position to right half near the top of the screen
        glfwSetWindowPos(window, desktopWidth / 2, 30);
    }
    // validate requested window size:
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    if (width != w || height != h) {
        Error("Could not create window with requested size");
    }
    // init callbacks: we assume that no other callback was installed (yet)
    if (handleKeyEvents) {
        // we need a static member function that can be registered with glfw:
        // static auto callback = bind(&Presentation::key_callbackMember, this, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5);
        // the above works, but can be done more elegantly with a lambda expression:
        static auto callback_static = [this](GLFWwindow* window, int key, int scancode, int action, int mods) {
            // because we have a this pointer we are now able to call a non-static member method:
            callbackKey(window, key, scancode, action, mods);
            };
        auto old = glfwSetKeyCallback(window,
            [](GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                // only static methods can be called here as we cannot change glfw function parameter list to include instance pointer
                callback_static(window, key, scancode, action, mods);
            }
        );
        assert(old == nullptr);
    }
    if (handleMouseMoveEevents) {
        static auto callback_static = [this](GLFWwindow* window, double xpos, double ypos) {
            callbackCursorPos(window, xpos, ypos);
            };
        auto old = glfwSetCursorPosCallback(window,
            [](GLFWwindow* window, double xpos, double ypos)
            {
                callback_static(window, xpos, ypos);
            }
        );
        assert(old == nullptr);
    }
    if (handleMouseButtonEvents) {
        static auto callback_static = [this](GLFWwindow* window, int button, int action, int mods) {
            callbackMouseButton(window, button, action, mods);
            };
        auto old = glfwSetMouseButtonCallback(window,
            [](GLFWwindow* window, int button, int action, int mods)
            {
                callback_static(window, button, action, mods);
            }
        );
        assert(old == nullptr);
    }
}

void Presentation::callbackKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    inputState.mouseButtonEvent = inputState.mouseMoveEvent = false;
    inputState.keyEvent = true;
    inputState.key = key;
    inputState.scancode = scancode;
    inputState.action = action;
    inputState.mods = mods;
    engine->app->handleInput(inputState);
}

void Presentation::callbackCursorPos(GLFWwindow* window, double xpos, double ypos)
{
    inputState.mouseButtonEvent = inputState.keyEvent = false;
    inputState.mouseMoveEvent = true;
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    inputState.pos.x = static_cast<float>(xpos / width);
    inputState.pos.y = static_cast<float>(ypos / height);
    engine->app->handleInput(inputState);
}

void Presentation::callbackMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    inputState.mouseMoveEvent = inputState.keyEvent = false;
    inputState.mouseButtonEvent = true;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        inputState.pressedLeft = action == GLFW_PRESS;
        inputState.pressedRight = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        inputState.pressedRight = action == GLFW_PRESS;
        inputState.pressedLeft = false;
    }
    inputState.stillPressedLeft = false;
    inputState.stillPressedRight = false;
    // Check if the left mouse button is still pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        //std::cout << "Left mouse button is still pressed." << std::endl;
        inputState.stillPressedLeft = true;
    }
    // Check if the right mouse button is still pressed
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
        //std::cout << "Right mouse button is still pressed." << std::endl;
        inputState.stillPressedRight = true;
    }
    engine->app->handleInput(inputState);
}
