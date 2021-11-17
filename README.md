# ShadedPathV

### TODO
Course of action should be like this
1. (done via themed timer) re-use old fps counter
1. Decouple Swap chain and backbuffer image rendering
2. backbuffer image saving
3. image based tests
4. Thread pool for backbuffer rendering
5. optimze thread performance
6. vr view
7. asset loading (library)
8. Shaders
9. vr controllers
10. animation
11. Demos
12. Games

### Issues

* configure validation layers with Vulkan Configurator. Didn't succeed in configuring via app. Storing will enable debug config still active after configurator closes, but ALL Vulkan apps may be affected!
* Enable *Debug Output* option to see messages in debug console 

### Replay capture file

```C:\dev\vulkan>C:\dev\vulkan\libraries\vulkan\Bin\gfxrecon-replay.exe --paused gfxrecon_capture_frames_100_through_105_20211116T131643.gfxr```


