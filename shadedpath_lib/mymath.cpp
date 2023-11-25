#include "mymath.h"
#if defined(USE_MYMATH)
#include "mysqrt.h"
#endif
#include <cmath>


namespace mathfunctions {
double sqrt(double x)
{
  // TODO 9: If USE_MYMATH is defined, use detail::mysqrt.
  // Otherwise, use std::sqrt.
#if defined(USE_MYMATH)
  return detail::mysqrt(x);
#else
  return std::sqrt(x);
#endif
}
}