add_library(shadedpathv STATIC OBJECT
    BillboardDemo.cpp
    BillboardShader.cpp
    ClearShader.cpp
    CubeShader.cpp
    DeviceCoordApp.cpp
    Files.cpp
    GameTime.cpp
    GeneratedTexturesApp.cpp
    GlobalRendering.cpp
    gltf.cpp
    gltfObjectsApp.cpp

    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_impl_glfw.cpp
    imgui/imgui_impl_vulkan.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    LandscapeDemo1.cpp
    LandscapeGenerator.cpp
    LineApp.cpp
    LineShader.cpp
    # miniaudio/extras/stb_vorbis.
    Object.cpp
    PBRShader.cpp
    pch.cpp
    Presentation.cpp
    ShadedPathEngine.cpp
    ShaderBase.cpp
    Shaders.cpp
    SimpleApp.cpp
    SimpleShader.cpp
    Sound.cpp
    Texture.cpp
    ThreadResources.cpp
    Threads.cpp
    ui.cpp
    UIShader.cpp
    Util.cpp
    VR.cpp
    VulkanResources.cpp
    World.cpp

)
target_compile_features(shadedpathv PUBLIC cxx_std_20)
target_include_directories(shadedpathv PUBLIC .)
target_precompile_headers(shadedpathv PUBLIC pch.h)
