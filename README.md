# ShadedPathV

Game Engine in Development!

ShadedPathV is a completely free C++ game engine built mainly on [Khronos standards](https://www.khronos.org/)

Some features:

- Rendering in multiple threads
- Each thread renders to its own backbuffer image, using its own set of thread local resources.
- Synchronization is done at presentation time, when the backbuffer image is copied to the app window
- Support VR games via OpenXR

## Cube Maps

1. https://jaxry.github.io/panorama-to-cubemap/
1. ```toktx --genmipmap --uastc 3 --zcmp 18 --assign_oetf linear --assign_primaries none --verbose --t2 --cubemap cube.ktx2 px.png nx.png py.png ny.png pz.png nz.png```

## Current State (Q2 / 2022)

### PBR Shader

As a first step we can now parse glTF files and render objects with just the base texture. No Lighting.

![pbr with only base texture](images/pbr01.png)
![pbr with only base texture](images/pbr02.png)

### Texture Loading from glTF Files
Implemented texture workflow for reading glTF files:
1. Downloaded texture in glTF format (e.g. from Sketchfab) usually have simple .pga or .jpg textures with no mipmaps.
1. We decided to go for KTX 2 texture containers with supercompressed format VK_FORMAT_BC7_SRGB_BLOCK. It seems to be the only texture compression format that has wide adoption.
1. This solves two issues: texture size on GPU is greatly reduced and we can include mipmaps offline when preparing the glTF files.
1. We use gltf-tranform like this to prepare glTF files for engine use:

   ```gltf-transform uastc WaterBottle.glb bottle2.glb --level 4 --zstd 18 --verbose```
   
   * KTX + Basis UASTC texture compression
   * level 4 is highest quality  - maybe use lower for development
   * zstd 18 is default level for Zstandard supercompression
   * gltf-transform creates mipmaps by default. use --filter to change default filter **lanczos4**

1. Decoding details are in **texture.cpp**. Workflow is like this (all methods from KTX library):
   * the engine checks format support at startup like this: ```vkGetPhysicalDeviceFormatProperties(engine->global.physicalDevice, VK_FORMAT_BC7_SRGB_BLOCK, &fp);```
   * after loading binary texture data into CPU memory: **ktxTexture_CreateFromMemory()**
   * check that we have the right texture type: ```kTexture->classId == class_id::ktxTexture2_c``` If this fails we have read a KTX 1 type texture which we cannot handle.
   * check supercompression with ```ktxTexture2_NeedsTranscoding();```
   * inflate UASTC and transcode to GPU block-compressed format: ```ktxTexture2_TranscodeBasis(texture, KTX_TTF_BC7_RGBA, 0);```
   * last step is to upload to GPU: ```ktxTexture2_VkUploadEx()```

## Q1 / 2022

I am somewhat ok with thread model for now. Seems stable and flexible: Application can switch between non-threaded rendering and aribtrary number of rendering threads. But real test will be when more complex rendering code is available with objects and animation.

Still experimenting a lot with managing vulkan code in meaningful C++ classes. Especially for organizing shaders in an easy-to-use fashion and clear architecture.

Simple app with lines and debug texture: 
![Simple app with lines and debug texture](images/view001.PNG)
Red lines show the world size of 2 square km. Lines are drawn every 1m. White cross marks center. The texture is a debug texture that uses a different color on each mip level. Only mip level 0 is a real image with letters to identify if texture orientation is ok. (TL means top left...) Same texture was used for both squares, but the one in background is displayed with a higher mip level. While the camera moves further back you can check the transition between all the mip levels. On upper right you see simple FPS counter rendered with Dear ImGui.

![Same scene with camera moved back](images/view002.PNG)
Same scene with camera moved back. You see lines resembling floor level and ceiling (380 m apart). The textures are so small that they should use highest or 2nd highest mip level with 1x1 or 2x2 image size.

## TODO
Things finished and things to do. Both very small and very large things, just as they come to my mind. 

- [ ] Cube maps (needed for PBR environment maps)
- [ ] Environment maps
- [ ] Rest of PBR stages
- [x] PBR Shader (simple: only base texture display, no lighting)
- [x] PBR object loading from glTF files (vertices with pos and text coord, textures)
- [x] Include KTX texture loading in PBR shader
- [x] \(done via themed timer) re-use old fps counter (still needs fixing - values too high?)
- [x] Decouple Swap chain and backbuffer image rendering
- [x] backbuffer image saving
- [x] adapt backbuffer image size during rendering to window size
- [x] fix renderThreadContinue->wait() not waiting correctly (atomic_flag not suitable)
- [x] fix no shutdown for > 1 render threads
- [x] add *Release* version to the current *Debug* config in VS
- [x] TextureStore to read and organize KTX textures
- [x] Include *Dear ImGui* with standard Demo UI
- [x] UI: FPS Counter
- [x] Find assets by looking for 'data' folder up the whole path, starting at .exe location
- [ ] image based tests
- [x] Thread pool for backbuffer rendering
- [x] dynamic lines for LineShader (added lines live only for one frame) in V 1.2 API
- [ ] fix line shader backbuffer2 image wrong format in stereo mode if no dynamic add lines
- [x] check for vulkan profile support: VP_KHR_roadmap_2022 level 1 (requires Feb 2022 drivers, only checked for nvdia)
- [ ] Switch to V 1.3 API and get rid of framebuffer and renderpasses
- [ ] LineText Shader with coordinate system display and dynamic text
- [ ] finalize thread architecture
- [ ] optimze thread performance
- [ ] vr view
- [ ] asset loading (library)
- [ ] Shaders
- [ ] vr controllers
- [ ] animation
- [ ] Demos
- [ ] Games

## Dependencies

copy to ../libraries:

* glf
* glm
* ktx
* vulkan
* tinygltf

copy ktx.dll to executable path:
* vulkan\ShadedPathV\ShadedPathV\x64\Debug\ktx.dll

* **OpenXR**: install NuGet package OpenXR.Loader for all three projects in solution. If not found during compile or not displayed correctly: uninstall via NuGet Package Manager, then re-install

Use Khronos OpenXR sdk directly for VS 2022:
 * clone https://github.com/KhronosGroup/OpenXR-SDK.git
 * in cmd: 
 ```
 mkdir build\win64
 cd build\win64
 cmake -G "Visual Studio 17" ..\..
 ```
 * open solution in VS 2022 at OpenXR-SDK\build\win64\OPENXR.sln and build
 * loader lib and pdb file will be here: \OpenXR-SDK\build\win64\src\loader\Debug
 * include folder here: \OpenXR-SDK\include\openxr
 * Add include directory for all ShadedPath Projects by right clicking, Properties, then in the VC++ Directories property page, add the path to the library header file(s) to Include Directories.
 * for ShadedPathVTest and ShadedPathV add library path similar to include directory

## Stereo Mode

Activate stereo mode from client with one of these:
* engine.enableVR()
* engine.enableStereo()

Stereo mode will enable all shaders to draw twice, for left and right eye. All internale instances are named without qualifier for single view mode / left eye. And with **2** added to the name for right eye. E.g. for line shader framebuffer:

* VkFramebuffer ThreadResources.framebufferLine (for left eye or single view)
* VkFramebuffer ThreadResources.framebufferLine2 (for right eye)

Only left eye will be shown in presentation window unless double view is activated with **engine.enableStereoPresentation()**

## Formats

To decide formats to use we can run the engine in presentation mode and get a list of all supported swap chain formats and presentation modes. On my Laptop and PC I get list below. We decided for the formats in **bold**

### Swap Chain Color Fomat and Space
| Format | VkFormat | Color Space | VkColorSpaceKHR |
| --- | --- | --- | --- |
| 44 | VK_FORMAT_B8G8R8A8_UNORM | 0 | VK_COLOR_SPACE_SRGB_NONLINEAR_KHR |
| **50** | **VK_FORMAT_B8G8R8A8_SRGB** | **0** | **VK_COLOR_SPACE_SRGB_NONLINEAR_KHR** |
| 64 | VK_FORMAT_A2B10G10R10_UNORM_PACK32 | 0 | VK_COLOR_SPACE_SRGB_NONLINEAR_KHR |

### Presentation mode

| Mode | VkPresentModeKHR |
| --- | --- | 
| 0 | VK_PRESENT_MODE_IMMEDIATE_KHR |
| **1** | **VK_PRESENT_MODE_MAILBOX_KHR** | 
| 2 | VK_PRESENT_MODE_FIFO_KHR | 
| 3 | VK_PRESENT_MODE_FIFO_RELAXED_KHR | 

### Thread Model

* renderThreadContinue: ThreadsafeWaitingQueue<> (host controlled)
* queue: FIFO queue (host controlled)
* presentFence: VkFence
* inFlightFence: VkFence

| Remarks                                         | Queue Submit Thread | Render Threads |
| -------------                                   | ------           | ------ |
| renderThreadContinue push()                     |                  | renderThreadContinue->pop() |
|                                                 |                  | drawFrame() |
|                                                 | queue.pop()      |             |
| presentFence was created in set mode            |                  | vkWaitForFences(presentFence) |
|                                                 |                  | vkReset |
|                                                 |                  | create graphics command buffers |
|                                                 |                  | queue.push() |
|                                                 |                  | renderThreadContinue->pop() |
|                                                 | vkQueueSubmit(inFlightFence) | |
|                                                 | vkWaitForFence(inFlightFence) |
|                                                 | vkReset |
|                                                 | vkAcquireNextImageKHR(swapChain) |
|                                                 | copy back buffer image to swapChain image |
|                                                 | vkQueueSubmit(presentFence) | |
|                                                 | vkQueuePresentKHR() |
|                                                 | renderThreadContinue push()
|                                                 |                  | vkWaitForFences(presentFence) |
|                                                 |                  | vkReset |
|                                                 |                  | drawFrame() |
|                                                 |                  | queue.push() |
|                                                 | queue.pop()      |             |
|                                                 |                  | renderThreadContinue->pop() |
|                                                 | vkQueueSubmit(inFlightFence) | |

## Coordinate Systems
### Right Handed Coordinate System

![right handed](images/right_handed_small.png)

Right-handed coordinate system (positive z towards camera) used for **ShadedPath Engine, OpenXR,** and **glTF**. (picture taken from [here](https://www.khronos.org/assets/uploads/developers/library/2018-siggraph/04-OpenXR-SIGGRAPH_Aug2018.pdf))

### OpenXR

From the spec (Chapter 2.16):
The OpenXR runtime must interpret the swapchain images in a clip space of positive Y pointing down, near Z plane at 0, and far Z plane at 1.

### Vulcan Device Coordinates (X,Y)

![Device Coordinates](DeviceCoordinates.drawio.svg)

This means right handed with x (-1 to 1) to the right, y (-1 to 1) top to bottom and z (0 to 1) into the screen. See app **DeviceCoordApp** for details.

## Issues

* configure validation layers with Vulkan Configurator. Didn't succeed in configuring via app. Storing will enable debug config still active after configurator closes, but ALL Vulkan apps may be affected!
* Enable *Debug Output* option to see messages in debug console 

### Replay capture file

```C:\dev\vulkan>C:\dev\vulkan\libraries\vulkan\Bin\gfxrecon-replay.exe --paused gfxrecon_capture_frames_100_through_105_20211116T131643.gfxr```

### Integrate Renderdoc
* Download renderdoc
* Add this extension https://github.com/baldurk/renderdoc-contrib/tree/main/baldurk/whereismydraw
* See what it tells you

### glTF Model Handling

Online Model Viewer (drag-and-drop):

```https://gltf-viewer.donmccurdy.com/```


Enable compression and create mipmaps:

```gltf-transform uastc WaterBottle.glb bottle2.glb --level 4 --zstd 18 --verbose```

### Some arbitrary timings (we be moved elsewhere eventually)

All timings in [microseconds]
* [38000] Create/Copy small VertexBuffer (1 million times)
* [50] all w/o actual memcpy()

## Copyrights of used Components

* Dear ImGui: https://github.com/ocornut/imgui/blob/master/LICENSE.txt Copyright (c) 2014-2021 Omar Cornut
* Coordinate system image: By Watchduck - Own work, CC BY-SA 4.0, https://commons.wikimedia.org/w/index.php?curid=58162563
* Coordinate system image: https://www.khronos.org/assets/uploads/developers/library/2018-siggraph/04-OpenXR-SIGGRAPH_Aug2018.pdf
* tinygltf
  * json.hpp : Licensed under the MIT License http://opensource.org/licenses/MIT. Copyright (c) 2013-2017 Niels Lohmann http://nlohmann.me.
  * stb_image : Public domain.
  * catch : Copyright (c) 2012 Two Blue Cubes Ltd. All rights reserved. Distributed under the Boost Software License, Version 1.0.
  * RapidJSON : Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip. All rights reserved. http://rapidjson.org/
  * dlib(uridecode, uriencode) : Copyright (C) 2003 Davis E. King Boost Software License 1.0. http://dlib.net/dlib/server/server_http.cpp.html
