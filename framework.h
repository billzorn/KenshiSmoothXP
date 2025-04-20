#pragma once

#define WIN32_LEAN_AND_MEAN

// if this is not defined before including windows.h,
// the c++ standard library will break...
#ifndef NOMINMAX
# define NOMINMAX
#endif

#include <windows.h>
