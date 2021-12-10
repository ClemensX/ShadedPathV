# ShadedPathV

## TODO
Course of action should be like this

- [x] \(done via themed timer) re-use old fps counter
- [x] Decouple Swap chain and backbuffer image rendering
- [x] backbuffer image saving
- [x] adapt backbuffer image size during rendering to window size
- [ ] image based tests
- [ ] Thread pool for backbuffer rendering
- [ ] optimze thread performance
- [ ] vr view
- [ ] asset loading (library)
- [ ] Shaders
- [ ] vr controllers
- [ ] animation
- [ ] Demos
- [ ] Games

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

* renderThreadContinue: atomic_flag
* queue: FIFO queue (host controlled)
* presentFence: VkFence
* inFlightFence: VkFence

| **Thread Synchronization**  | Queue Submit Thread | Render Threads |
| -------------             | ------ | ------ |
| renderThreadContinue set + notify               |                  | renderThreadContinue->wait() |
|                                                 |                  | drawFrame() |
|                                                 | queue.pop()      |             |
| presentFence was created in set mode            |                  | vkWaitForFences(presentFence) |
|                                                 |                  | vkReset |
|                                                 |                  | create graphics command buffers |
|                                                 |                  | queue.push() |
|                                                 | vkQueueSubmit(inFlightFence) | |
|                                                 |                  | vkWaitForFences(presentFence) |
|                                                 | vkWaitForFence(inFlightFence) |
|                                                 | vkReset |
|                                                 | vkAcquireNextImageKHR(swapChain) |
|                                                 | copy back buffer image to swapChain image |
|                                                 | vkQueueSubmit(presentFence) | |
|                                                 |                  | vkReset |
| Validation Error: VkFence is simultaneously used | vkQueueSubmit(presentFence) | |
|                                                 |                  | drawFrame() |
|                                                 |                  | vkWaitForFences(presentFence) |
|                                                 | vkQueuePresentKHR() |
|                                                 | renderThreadContinue set + notify
|                                                 | queue.pop()      |             |

## Issues

* configure validation layers with Vulkan Configurator. Didn't succeed in configuring via app. Storing will enable debug config still active after configurator closes, but ALL Vulkan apps may be affected!
* Enable *Debug Output* option to see messages in debug console 

### Replay capture file

```C:\dev\vulkan>C:\dev\vulkan\libraries\vulkan\Bin\gfxrecon-replay.exe --paused gfxrecon_capture_frames_100_through_105_20211116T131643.gfxr```

### Integrate Renderdoc
* Download renderdoc
* Add this extension https://github.com/baldurk/renderdoc-contrib/tree/main/baldurk/whereismydraw
* See what it tells you

