#include "pch.h"
#include "Renderer.hpp"

#include <vulkan/vulkan.hpp>

namespace CEE
{
	
#if defined(CEE_OS_WINDOWS)
	HINSTANCE Renderer::s_Connection = NULL;
#elif defined(CEE_WM_XCB)
	xcb_connection_t* Renderer::s_Connection = nullptr;
#endif
#if defined(CEE_OS_WINDOWS)
	Renderer::Renderer(HINSTANCE connection, Window* window)
	{
		s_Connection = connection;
		m_Window = window;
		InitalizeRenderer();
	}
#elif defined(CEE_WM_XCB)
	Renderer::Renderer(xcb_connection_t* connection, Window* window)
	{
		s_Connection = connection;
		m_Window = window;
		InitalizeRenderer();
	}
#endif

	void Renderer::InitalizeRenderer()
	{
		InitalizeInstance();
		InitalizeSurface();
		InitalizeDevice();
		InitalizeCommandBuffer();
		InitalizeSwapchain();
		InitalizeDepthBuffer();
		InitalizeUniformBuffer();
		InitalizePipelineLayout();
		InitalizeDescriptorSet();
		InitalizeRenderPass();
		InitalizeShaders();
		InitalizeFramebuffers();
		InitalizeVertexBuffer();
		InitalizeIndexBuffer();
		InitalizePipeline();
		InitalizeSyncronisation();
		m_Prepared = true;
	}
	
	void Renderer::InitalizeInstance()
	{
		uint32_t instanceExtensionCount = 0;
		uint32_t instanceLayerCount = 0;
		char const* const instacneValidationLayers[] = { "VK_LAYER_KHRONOS_validation" };
		
#if defined(NDEBUG)
		m_Validate = false;
#endif
		
		vk::Bool32 validationFound = VK_FALSE;
		if (m_Validate)
		{
			auto result = vk::enumerateInstanceLayerProperties(&instanceLayerCount, static_cast<vk::LayerProperties*>(nullptr));
		}
	}
	
	void Renderer::InitalizeSurface()
	{
		
	}
	
	void Renderer::InitalizeDevice()
	{
		
	}
	
	void Renderer::InitalizeCommandBuffer()
	{
		
	}
	
	void Renderer::InitalizeSwapchain()
	{
		
	}
	
	void Renderer::InitalizeDepthBuffer()
	{
		
	}
	
	void Renderer::InitalizeUniformBuffer()
	{
		
	}
	
	void Renderer::InitalizePipelineLayout()
	{
		
	}
	
	void Renderer::InitalizeDescriptorSet()
	{
		
	}
	
	void Renderer::InitalizeRenderPass()
	{
		
	}
	
	void Renderer::InitalizeShaders()
	{
		
	}
	
	void Renderer::InitalizeFramebuffers()
	{
		
	}
	
	void Renderer::InitalizeVertexBuffer()
	{
		
	}
	
	void Renderer::InitalizeIndexBuffer()
	{
		
	}
	
	void Renderer::InitalizePipeline()
	{
		
	}
	
	void Renderer::InitalizeSyncronisation()
	{
		
	}
	
}
