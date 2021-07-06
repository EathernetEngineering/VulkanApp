cmake_minimum_required(VERSION 3.2)

project(VulkanApp VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD_REQUIRED 17)

add_executable(VulkanApp main.cpp Application.cpp Window.cpp Renderer.cpp Application.hpp
	Window.hpp Renderer.hpp Shader.cpp Shader.hpp Camera.cpp Camera.hpp base.hpp)

find_package(Vulkan REQUIRED)
target_link_libraries(VulkanApp ${Vulkan_LIBRARIES})

if(WIN32)
set(CEE_OS_WINDOWS ON)
set(CEE_WM_WIN32 ON)
elseif(UNIX AND NOT APPLE)
set(CEE_OS_LINUX ON)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})
find_package(XCB)
find_package(X11)
find_package(Wayland)
if(XCB_FOUND)
target_link_libraries(VulkanApp xcb)
set(CEE_WM_XCB ON)
elseif(X11_FOUND)
set(CEE_WM_X11 ON)
elseif(Wayland_FOUND)
set(CEE_WM_WAYLAND ON)
endif()
else()
set(CEE_OS_MAC ON)
set(CEE_WM_COCOA ON)
endif()

add_subdirectory(vendor/shaderc)

configure_file(config.h.in config.h)

target_include_directories(VulkanApp PRIVATE build/ ${Vulkan_INCLUDE_DIRS} vendor/glm vendor/shaderc/libshaderc/include)
target_link_libraries(VulkanApp shaderc)
target_precompile_headers(VulkanApp PRIVATE pch.h)
