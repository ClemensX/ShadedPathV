#include "mainheader.h"

using namespace std;

void UI::init(ShadedPathEngine* engine)
{
    this->engine = engine;
    if (!enabled)
        return;
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Create Descriptor Pool
    {
        VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        if (vkCreateDescriptorPool(engine->globalRendering.device, &pool_info, nullptr, &g_DescriptorPool) != VK_SUCCESS) {
            Error("Cannot create DescriptorPool for DearImGui");
        }
    }
    // create render pass
    {
        // attachment
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = engine->globalRendering.ImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;//VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        // subpasses and attachment references
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        // subpasses
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // render pass
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(engine->globalRendering.device, &renderPassInfo, nullptr, &imGuiRenderPass) != VK_SUCCESS) {
            Error("failed to create render pass!");
        }
    }

    // Setup Platform/Renderer backends
    WindowInfo* winfo = engine->presentation.windowInfo;
    ImGui_ImplGlfw_InitForVulkan(winfo->glfw_window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = engine->globalRendering.vkInstance;
    init_info.PhysicalDevice = engine->globalRendering.physicalDevice;
    init_info.Device = engine->globalRendering.device;
    init_info.QueueFamily = engine->globalRendering.familyIndices.graphicsFamily.value();
    init_info.Queue = engine->globalRendering.graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = winfo->imageCount - 1;
    init_info.ImageCount = winfo->imageCount;
    init_info.RenderPass = imGuiRenderPass;
    //init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info);

    // upload fonts to GPU
    //VkCommandBuffer command_buffer = engine->globalRendering.beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    //engine->globalRendering.endSingleTimeCommands(command_buffer);
}

void UI::update()
{
    if (!enabled)
        return;
    unique_lock<mutex> lock(monitorMutex);
    beginFrame();
    //ImGui::ShowDemoWindow();
    buildUI();
    endFrame();
    uiRenderAvailable = true;
}

void UI::render(FrameResources* fr, UISubShader* pf)
{
    if (!enabled)
        return;
    unique_lock<mutex> lock(monitorMutex);
    if (!uiRenderAvailable) {
        return;
    }
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pf->commandBuffer);
}

void UI::beginFrame()
{
    if (!enabled)
        return;
    WindowInfo* winfo = engine->presentation.windowInfo;
    ImGui_ImplVulkan_NewFrame();
    //ImGui_ImplGlfw_NewFrame(winfo->width, winfo->height);
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UI::endFrame()
{
    if (!enabled)
        return;
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Render();
}

void UI::buildUI()
{
    if (!enabled)
        return;
    // switch between imgui demo window and our own
    bool showImguiDemo = false;
    if (!showImguiDemo) {
        bool open = false;
        bool* p_open = NULL;//&open; // no close button

        // displayed text:
        bool enableMouseTracking = false; // switch for displaying mouse pos in overlay
        //string fps("60.0");
        string fps = engine->fpsCounter.getFPSAsString();
        //string appname("SimpleAp");
        string appname = engine->appname;
        appname = "ShadedPath " + appname + " FPS: " + fps;

        int corner = 1;
        ImGuiIO& io = ImGui::GetIO();
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
        if (corner != -1)
        {
            const float PAD = 0.0f; // disable padding space between borders and Overlay
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
            ImVec2 work_size = viewport->WorkSize;
            ImVec2 window_pos, window_pos_pivot;
            window_pos.x = (corner & 1) ? (work_pos.x + work_size.x - PAD) : (work_pos.x + PAD);
            window_pos.y = (corner & 2) ? (work_pos.y + work_size.y - PAD) : (work_pos.y + PAD);
            window_pos_pivot.x = (corner & 1) ? 1.0f : 0.0f;
            window_pos_pivot.y = (corner & 2) ? 1.0f : 0.0f;
            ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            window_flags |= ImGuiWindowFlags_NoMove;
        }
        ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
        if (ImGui::Begin("ShadedPathV", p_open, window_flags))
        {
            ImGui::Text(appname.c_str());
            if (enableMouseTracking) {
                ImGui::Separator();
                if (ImGui::IsMousePosValid())
                    ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
                else
                    ImGui::Text("Mouse Position: <invalid>");
            }
            engine->app->buildCustomUI();
            if (ImGui::BeginPopupContextWindow())
            {
                if (ImGui::MenuItem("Custom", NULL, corner == -1)) corner = -1;
                if (ImGui::MenuItem("Top-left", NULL, corner == 0)) corner = 0;
                if (ImGui::MenuItem("Top-right", NULL, corner == 1)) corner = 1;
                if (ImGui::MenuItem("Bottom-left", NULL, corner == 2)) corner = 2;
                if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
                if (p_open && ImGui::MenuItem("Close")) *p_open = false;
                ImGui::EndPopup();
            }
        }
        ImGui::End();
    }
    else {
        ImGui::ShowDemoWindow();
    }
}

UI::~UI()
{
    if (!enabled)
        return;
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyRenderPass(engine->globalRendering.device, imGuiRenderPass, nullptr);
    vkDestroyDescriptorPool(engine->globalRendering.device, g_DescriptorPool, nullptr);
}