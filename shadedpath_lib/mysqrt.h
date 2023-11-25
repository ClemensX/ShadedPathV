#pragma once

#if defined(OPENXR_AVAILABLE)
#if !defined(_MSC_VER)
#warning "--> OpenXR build mode activted!"
#else
#pragma message("--> OpenXR build mode activted!")
#endif
#else
#if !defined(_MSC_VER)
#warning "--> building without OpenXR"
#else
#pragma message("--> building without OpenXR")
#endif
#endif
namespace mathfunctions {
namespace detail {
double mysqrt(double x);
}
}