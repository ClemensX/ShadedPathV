#pragma once
class ShadedPathEngine
{
public:
    ShadedPathEngine() {
        Log("Engine c'tor\n");
    }

    virtual ~ShadedPathEngine() {
        Log("Engine destructor\n");
    };

    // initialize Vulkan
    void init();
    // exit Vulkan and free resources
    void shutdown();
private:
    GLFWwindow* window = nullptr;
    VkInstance vkInstance;

};

