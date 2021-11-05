// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here

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
#include <cstdint>
#include <algorithm>
using namespace std;

// headers for used libraries
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

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

inline void ErrorExt(string msg, const char* file, DWORD line)
{
	stringstream s;
	s << "ERROR " << msg << " ";
	s << file << " " << line << '\n';
	Log(s.str().c_str());
	//exit(0);
	s << "\n\nClick 'yes' to debug break and 'no' to hard exit.";
	int nResult = MessageBox(GetForegroundWindow(), L"Click 'yes' to debug break or 'no' to hard exit.", L"Unexpected error encountered", MB_YESNO | MB_ICONERROR);
	if (nResult == IDYES)
		DebugBreak();
	else exit(0);
}

#define Error(x) ErrorExt((x), __FILE__,  (DWORD)__LINE__)

// engine headers

#include "Util.h"
#include "GlobalRendering.h"
#include "ShadedPathEngine.h"
#include "SimpleApp.h"


#endif //PCH_H
