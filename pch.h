#ifndef _PCH_H
#define _PCH_H

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <cstdio>
#include <sstream>

#include "base.hpp"

#if defined(CEE_WM_XCB)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(CEE_WM_WAYLAND)
#define VK_USE_PLATFORM_WAYLAND_KHR
#elif defined(CEE_WM_COCOA)
#define VK_USE_PLATFORM_COCOA_KHR
#elif defined(CEE_WM_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.hpp>

#endif
