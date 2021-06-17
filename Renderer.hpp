#ifndef _RENDERER_HPP
#define _RENDERER_HPP

#include "Window.hpp"

#if defined(CEE_OS_WINDOWS)
#include <Windows.h>
#elif defined(CEE_WM_XCB)
#include <xcb/xcb.h>
#endif

namespace CEE
{
	class Renderer
	{
	public:
#if defined(CEE_OS_WINDOWS)
		Renderer(HINSTANCE connection, Window* window);
#elif defined(CEE_WM_XCB)
		Renderer(xcb_connection_t* connection, Window* window);
#endif
		~Renderer();
		
	private:
		void InitalizeRenderer();
		
		void InitalizeInstance();
		void InitalizeSurface();
		void InitalizeDevice();
		void InitalizeCommandBuffer();
		void InitalizeSwapchain();
		void InitalizeDepthBuffer();
		void InitalizeUniformBuffer();
		void InitalizePipelineLayout();
		void InitalizeDescriptorSet();
		void InitalizeRenderPass();
		void InitalizeShaders();
		void InitalizeFramebuffers();
		void InitalizeVertexBuffer();
		void InitalizeIndexBuffer();
		void InitalizePipeline();
		void InitalizeSyncronisation();

	private:
#if defined(CEE_OS_WINDOWS)
		static HINSTANCE s_Connection;
#elif defined(CEE_WM_XCB)
		static xcb_connection_t* s_Connection;
#endif
		Window* m_Window;
		
		bool m_Prepared;
		bool m_Validate;
		
		vk::Instance m_Instance;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::Device m_Device;
	};
}

#endif
