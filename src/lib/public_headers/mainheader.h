#pragma once

// global defines to enable/disable features:
#define LOGFILE true
// set to true for logging render queue operations
// submit queue logging
#define LOG_QUEUE false
// render thread continuation logging:
#define LOG_RENDER_CONTINUATION false
// render thread fence logging:
#define LOG_FENCE false
// global update threads logging
#define LOG_GLOBAL_UPDATE false

// timer topics:
// FPS like (will display FPS value in loginfo() output)
#define TIMER_DRAW_FRAME "DrawFrame"
#define TIMER_PRESENT_FRAME "PresentFrame"
#define TIMER_INPUT_THREAD "InputThread"
// part timers for sub-frame timings like presenting or buffer upload
#define TIMER_PART_BACKBUFFER_COPY_AND_PRESENT "PartPresentBackBuffer"
#define TIMER_PART_BUFFER_COPY "PartBufferCopy"
#define TIMER_PART_GLOBAL_UPDATE "PartGlobalUpdate"
#define TIMER_PART_OPENXR "PartOpenXR"

// Windows headers

#if defined(_WIN64)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#endif

// c++ standard lib headers
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
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
#include <bitset>
//using namespace std;

// headers for used libraries
#define VK_ENABLE_BETA_EXTENSIONS
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_beta.h>
#include <vulkan/vulkan.hpp>
//#define GLFW_INCLUDE_VULKAN

// vulkan profile support (SDK 1.3 needed)
#pragma warning( push, 1 )
#pragma warning(disable:6011)
#include <vulkan/vulkan_profiles.hpp>
#pragma warning( pop )

// Dear ImGui headers:
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#include <GLFW/glfw3.h>
// glfw has included vulkan/vulkan.h
#include "ktxvulkan.h"

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

#if defined(OPENXR_AVAILABLE)
#define XR_USE_GRAPHICS_API_VULKAN
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#endif

// global definitions and macros

#if !defined(_WIN64)
typedef unsigned long DWORD, *PDWORD, *LPDWORD;
typedef long long LONGLONG;
typedef unsigned int UINT;
#endif

#if defined(__APPLE__)

#include <libkern/OSByteOrder.h>
#define _byteswap_uint64(x) OSSwapInt64(x)
//#include <MoltenVK/mvk_config.h>
#endif


//{
//std::byte b;
//}

//using namespace std;
inline void LogFile(const char* s) {
	static bool firstcall = true; // exceptional use of local static field: reuse same log file for many engine runs in same process
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
#if defined(_WIN64)
#define Log(x)\
{\
    std::stringstream s1768;  s1768 << x; \
    std::string str = s1768.str(); \
    std::wstring wstr(str.begin(), str.end()); \
    std::wstringstream wss(wstr); \
    OutputDebugString(wss.str().c_str()); \
    LogFile(str.c_str()); \
}
#elif defined(__APPLE__)
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
	printf("%s", s1765.str().c_str()); \
}
#else
#define Log(x)\
{\
    std::stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
}
#endif
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
	int size = std::vsnprintf(nullptr, 0, fmt, vl);
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
	throw std::logic_error(failureMessage);
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

#if defined(OPENXR_AVAILABLE)
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
	}
	else {
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

#include "DebugOutput.h"
#include "OpenXRDebugUtils.h"
#endif

// global const
double static const PI = 3.14159265358979323846;
double static const PI2 = PI * 2.0;
double static const PI_half = PI / 2.0;
double static const PI_quarter = PI / 4.0;

// engine headers

#include "GlobalDef.h"
#include "Global.h"
#include "EngineParticipant.h"
#include "Files.h"
#include "GameTime.h"
#include "Util.h"
#include "ShadedPathEngine.h"
#include "Threads.h"
#include "Camera.h"
#include "VR.h"
#include "Texture.h"
#include "GlobalRendering.h"
#include "GlobalUpdate.h"
#include "VulkanResources.h"
//#include "ShaderBase.h"
//#include "ClearShader.h"
//#include "SimpleShader.h"
//#include "LineShader.h"
//#include "pbrShader.h"
//#include "CubeShader.h"
//#include "BillboardShader.h"
//#include "TerrainShader.h"
//#include "gltf.h"
//#include "Object.h"
////#include "Sound.h"
//#include "ui.h"
//#include "UIShader.h"
//#include "Shaders.h"
//#include "ThreadResources.h"
//#include "ShadedPathEngine.h"
//#include "structs.h"
//#include "Path.h"
//#include "World.h"
//#include "Game.h"

//#include "EngineParticipant.h"
//#include "Files.h"
//#include "GameTime.h"
//#include "Util.h"
//#include "Threads.h"
//#include "Presentation.h"
//#include "Camera.h"
//#include "VR.h"
//#include "Texture.h"
//#include "GlobalRendering.h"
//#include "GlobalUpdate.h"
//#include "VulkanResources.h"
//#include "ShaderBase.h"
//#include "ClearShader.h"
//#include "SimpleShader.h"
//#include "LineShader.h"
//#include "pbrShader.h"
//#include "CubeShader.h"
//#include "BillboardShader.h"
//#include "TerrainShader.h"
//#include "gltf.h"
//#include "Object.h"
//#include "Sound.h"
//#include "ui.h"
//#include "UIShader.h"
//#include "Shaders.h"
//#include "ThreadResources.h"
//#include "ShadedPathEngine.h"
//#include "structs.h"
//#include "Path.h"
//#include "World.h"
//#include "Game.h"
