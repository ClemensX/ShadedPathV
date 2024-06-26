// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// global defines to enable/disable features:
#define LOGFILE true
// set to true for logging render queue operations
// submit queue logging
#define LOG_QUEUE false
// render thread continuation logging:
#define LOG_RENDER_CONTINUATION false

#define LOG_FENCE false

// timer topics:
// FPS like
#define TIMER_DRAW_FRAME "DrawFrame"
#define TIMER_PRESENT_FRAME "PresentFrame"
#define TIMER_INPUT_THREAD "InputThread"
// part timers for sub-frame timings like presenting or buffer upload
#define TIMER_PART_BACKBUFFER_COPY_AND_PRESENT "PartPresentBackBuffer"
#define TIMER_PART_BUFFER_COPY "PartBufferCopy"

// add headers that you want to pre-compile here

// Dear ImGui headers:
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"


// Windows headers

#if defined(_WIN64)
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif

// c++ standard lib headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <optional>
#include <set>
#include <unordered_map>
#include <cstdint>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <array>
#include <functional>
#include <regex>
//using namespace std;

// headers for used libraries
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <ktxvulkan.h>

// vulkan profile support (SDK 1.3 needed)
#pragma warning( push, 1 )
#pragma warning(disable:6011)
#include <vulkan/vulkan_profiles.hpp>
#pragma warning( pop )

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

//
// OpenXR Headers
//

//#if defined(_WIN64)
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
//#endif

// global definitions and macros

#if !defined(_WIN64)
typedef unsigned long DWORD, *PDWORD, *LPDWORD;
typedef long long LONGLONG;
typedef unsigned int UINT;
#endif

#if defined(__APPLE__)

#include <libkern/OSByteOrder.h>
#define _byteswap_uint64(x) OSSwapInt64(x)

#endif


//{
//std::byte b;
//}

//using namespace std;
inline void LogFile(const char* s) {
	static bool firstcall = true;
	std::ios_base::openmode mode;
	if (firstcall) {
		mode = std::ios::out;
		firstcall = false;
	}
	else {
		mode = std::ios::out | std::ios::app;
	}
	std::ofstream out("spe_run.log", mode);
	out << s;
	out.close();
}


#if defined(DEBUG) | defined(_DEBUG)
#define LogCond(y,x) if(y){Log(x)}
#define Log(x)\
{\
    std::wstringstream s1764;  s1764 << x; \
    OutputDebugString(s1764.str().c_str()); \
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
}
#elif defined(LOGFILE)
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
}
#else
#define Log(x)
#define LogCond(y,x)
#endif

#define LogF(x)\
{\
	std::stringstream s1764;  s1764 << x; \
	LogFile(s1764.str().c_str()); \
}

#define LogCondF(y,x) if(y){LogF(x)}

// Help with OpenXR sample code logging:
#define LogX(x) Log((x).c_str()<<endl)

//#include Log("hugo");

inline void ErrorExt(std::string msg, const char* file, DWORD line)
{
	std::stringstream s;
	s << "ERROR " << msg << " ";
	s << file << " " << line << '\n';
	Log(s.str().c_str());
	//exit(0);
#if defined(_WIN64)
	s << "\n\nClick 'yes' to debug break and 'no' to hard exit.";
	int nResult = MessageBoxA(GetForegroundWindow(), s.str().c_str(), "Unexpected error encountered", MB_YESNO | MB_ICONERROR);
	if (nResult == IDYES)
		DebugBreak();
	else exit(0);
#else
    exit(0);
#endif
}

#define Error(x) ErrorExt((x), __FILE__,  (DWORD)__LINE__)

// OpenXR helpers
inline std::string Fmt(const char* fmt, ...) {
	va_list vl;
	va_start(vl, fmt);
	size_t size = std::vsnprintf(nullptr, 0, fmt, vl);
	va_end(vl);

	if (size != -1) {
		std::unique_ptr<char[]> buffer(new char[size + 1]);

		va_start(vl, fmt);
		size = std::vsnprintf(buffer.get(), size + 1, fmt, vl);
		va_end(vl);
		if (size != -1) {
			return std::string(buffer.get(), size);
		}
	}

	throw std::runtime_error("Unexpected vsnprintf failure");
}

#define CHK_STRINGIFY(x) #x
#define TOSTRING(x) CHK_STRINGIFY(x)
#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

[[noreturn]] inline void Throw(std::string failureMessage, const char* originator = nullptr, const char* sourceLocation = nullptr) {
	if (originator != nullptr) {
		failureMessage += Fmt("\n    Origin: %s", originator);
	}
	if (sourceLocation != nullptr) {
		failureMessage += Fmt("\n    Source: %s", sourceLocation);
	}
	Error(failureMessage.c_str());
	//throw std::logic_error(failureMessage);
}

#define THROW(msg) Throw(msg, nullptr, FILE_AND_LINE);
#define CHECK(exp)                                      \
    {                                                   \
        if (!(exp)) {                                   \
            Throw("Check failed", #exp, FILE_AND_LINE); \
        }                                               \
    }
#define CHECK_MSG(exp, msg)                  \
    {                                        \
        if (!(exp)) {                        \
            Throw(msg, #exp, FILE_AND_LINE); \
        }                                    \
    }

// convert error code to string:
inline std::optional<std::string> xr_to_string(XrInstance instance, XrResult res) {
	if (instance == nullptr) {
		return std::nullopt;
	}
	else {
		char buffer[XR_MAX_RESULT_STRING_SIZE];
		auto r = xrResultToString(instance, res, buffer);
		if (XR_SUCCESS == r) {
			return std::string(buffer);
		}
		else {
			return std::nullopt;
		}
	}
}

[[noreturn]] inline void ThrowXrResult(XrInstance instance, XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
	std::optional<std::string> resString = xr_to_string(instance, res);
	if (resString == std::nullopt) {
		Throw(Fmt("XrResult failure [%s]", std::to_string(res).c_str()), originator, sourceLocation);
	} else {
		Throw(Fmt("XrResult failure [%s]", resString.value().c_str()), originator, sourceLocation);
	}
}

inline XrResult CheckXrResult(XrInstance instance, XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
	if (XR_FAILED(res)) {
		ThrowXrResult(instance, res, originator, sourceLocation);
	}

	return res;
}

#define THROW_XR(xr, cmd) ThrowXrResult(xr, cmd, FILE_AND_LINE);
#define CHECK_XRCMD(cmd) CheckXrResult(instance, cmd, #cmd, FILE_AND_LINE);
#define CHECK_XRRESULT(res, cmdStr) CheckXrResult(res, cmdStr, FILE_AND_LINE);



// engine headers

#include "Files.h"
#include "GameTime.h"
#include "Util.h"
#include "Threads.h"
#include "Presentation.h"
#include "Camera.h"
#include "VR.h"
#include "GlobalRendering.h"
#include "VulkanResources.h"
#include "Texture.h"
#include "ShaderBase.h"
#include "ClearShader.h"
#include "SimpleShader.h"
#include "LineShader.h"
#include "pbrShader.h"
#include "CubeShader.h"
#include "BillboardShader.h"
#include "gltf.h"
#include "Object.h"
#include "Sound.h"
#include "ui.h"
#include "UIShader.h"
#include "Shaders.h"
#include "ThreadResources.h"
#include "ShadedPathEngine.h"
#include "World.h"
#include "SimpleApp.h"
#include "DeviceCoordApp.h"
#include "LineApp.h"
#include "gltfObjectsApp.h"
#include "GeneratedTexturesApp.h"
#include "BillboardDemo.h"
#include "LandscapeDemo1.h"
#include "LandscapeGenerator.h"

#endif //PCH_H
