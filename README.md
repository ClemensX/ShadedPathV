# ShadedPathV

Game Engine in Development!

ShadedPathV is a completely free C++ game engine built mainly on [Khronos standards](https://www.khronos.org/)

Some features:

- Rendering in multiple threads
- Each thread renders to its own backbuffer image, using its own set of thread local resources.
- Synchronization is done at presentation time, when the backbuffer image is copied to the app window
- Support VR games. (Probably via OpenXR)

## Current state

I am somewhat ok with thread model for now. Seems stable and flexible: Application can switch between non-threaded rendering and aribtrary number of rendering threads. But real test will be when more complex rendering code is available with objects and animation.

Still experimenting a lot with managing vulkan code in meaningful C++ classes. Especially for organizing shaders in an easy-to-use fashion and clear architecture.

Simple app with lines and debug texture: 
![Simple app with lines and debug texture](images/view001.PNG)
Red lines show the world size of 2 square km. Lines are drawn every 1m. White cross marks center. The texture is a debug texture that uses a different color on each mip level. Only mip level 0 is a real image with letters to identify if texture orientation is ok. (TL means top left...) Same texture was used for both squares, but the one in background is displayed with a higher mip level. While the camera moves further back you can check the transition between all the mip levels. On upper right you see simple FPS counter rendered with Dear ImGui.

![Same scene with camera moved back](images/view002.PNG)
Same scene with camera moved back. You see lines resembling floor level and ceiling (380 m apart). The textures are so small that they should use highest or 2nd highest mip level with 1x1 or 2x2 image size.

## TODO
Things finished and things to do. Both very small and very large things, just as they come to my mind. 

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
- [ ] image based tests
- [x] Thread pool for backbuffer rendering
- [x] dynamic lines for LineShader (added lines live only for one frame) in V 1.2 API
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

copy ktx.dll to executable path:
* vulkan\ShadedPathV\ShadedPathV\x64\Debug\ktx.dll

* **OpenXR**: install NuGet package OpenXR.Loader for all three projects in solution. If not found during compile or not displayed correctly: uninstall via NuGet Package Manager, then re-install

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
### Vulcan Device Coordinates (X,Y)

![Device Coordinates](DeviceCoordinates.drawio.svg)

### OpenXR

From the spec:
The OpenXR runtime must interpret the swapchain images in a clip space of positive Y pointing down, near Z plane at 0, and far Z plane at 1.

## Issues

* configure validation layers with Vulkan Configurator. Didn't succeed in configuring via app. Storing will enable debug config still active after configurator closes, but ALL Vulkan apps may be affected!
* Enable *Debug Output* option to see messages in debug console 

### Replay capture file

```C:\dev\vulkan>C:\dev\vulkan\libraries\vulkan\Bin\gfxrecon-replay.exe --paused gfxrecon_capture_frames_100_through_105_20211116T131643.gfxr```

### Integrate Renderdoc
* Download renderdoc
* Add this extension https://github.com/baldurk/renderdoc-contrib/tree/main/baldurk/whereismydraw
* See what it tells you

### Some arbitrary timings (we be moved elsewhere eventually)

All timings in [microseconds]
* [38000] Create/Copy small VertexBuffer (1 million times)
* [50] all w/o actual memcpy()

## Copyrights of used Components

* Dear ImGui: https://github.com/ocornut/imgui/blob/master/LICENSE.txt Copyright (c) 2014-2021 Omar Cornut
