// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// global defines to enable/disable features:
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

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#include <windows.h>

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
using namespace std;

// headers for used libraries
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <ktxvulkan.h>

// vulkan profile support (SDK 1.3 needed)
#include <vulkan/vulkan_profiles.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/quaternion.hpp>
using namespace glm;
//
// OpenXR Headers
//
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>

// global definitions and macros

inline void LogFile(const char* s) {
	static bool firstcall = true;
	ios_base::openmode mode;
	if (firstcall) {
		mode = ios::out;
		firstcall = false;
	}
	else {
		mode = ios::out | ios::app;
	}
	ofstream out("spe_run.log", mode);
	out << s;
	out.close();
}

#if defined(DEBUG) | defined(_DEBUG)
#define LogCond(y,x) if(y){Log(x)}
#define Log(x)\
{\
    wstringstream s1764;  s1764 << x; \
    OutputDebugString(s1764.str().c_str()); \
    stringstream s1765; s1765 << x; \
    LogFile(s1765.str().c_str()); \
}
#elif defined(LOGFILE)
#define Log(x)\
{\
	wstringstream s1764;  s1764 << x; \
	LogFile(s1764.str().c_str()); \
}
#else
#define Log(x)
#define LogCond(y,x)
#endif

#define LogF(x)\
{\
	stringstream s1764;  s1764 << x; \
	LogFile(s1764.str().c_str()); \
}

#define LogCondF(y,x) if(y){LogF(x)}

// Help with OpenXR sample code logging:
#define LogX(x) Log((x).c_str()<<endl)

inline void ErrorExt(string msg, const char* file, DWORD line)
{
	stringstream s;
	s << "ERROR " << msg << " ";
	s << file << " " << line << '\n';
	Log(s.str().c_str());
	//exit(0);
	s << "\n\nClick 'yes' to debug break and 'no' to hard exit.";
	int nResult = MessageBoxA(GetForegroundWindow(), s.str().c_str(), "Unexpected error encountered", MB_YESNO | MB_ICONERROR);
	if (nResult == IDYES)
		DebugBreak();
	else exit(0);
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

[[noreturn]] inline void ThrowXrResult(XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
	Throw(Fmt("XrResult failure [%s]", to_string(res)), originator, sourceLocation);
}

inline XrResult CheckXrResult(XrResult res, const char* originator = nullptr, const char* sourceLocation = nullptr) {
	if (XR_FAILED(res)) {
		ThrowXrResult(res, originator, sourceLocation);
	}

	return res;
}

#define THROW_XR(xr, cmd) ThrowXrResult(xr, #cmd, FILE_AND_LINE);
#define CHECK_XRCMD(cmd) CheckXrResult(cmd, #cmd, FILE_AND_LINE);
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
#include "Texture.h"
#include "ShaderBase.h"
#include "ClearShader.h"
#include "SimpleShader.h"
#include "LineShader.h"
#include "Shaders.h"
#include "ThreadResources.h"
#include "ui.h"
#include "ShadedPathEngine.h"
#include "World.h"
#include "SimpleApp.h"


#endif //PCH_H
