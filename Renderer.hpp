#ifndef _RENDERER_HPP
#define _RENDERER_HPP

#include "Window.hpp"
#include "Shader.hpp"
#include "Camera.hpp"

#if defined(CEE_OS_WINDOWS)
#include <Windows.h>
#elif defined(CEE_WM_XCB)
#include <xcb/xcb.h>
#endif

#include <glm/glm.hpp>

namespace CEE
{
	typedef struct SwapchainResources {
		vk::Image image;
		vk::ImageView view;
	} SwapchainResources;

	typedef struct DepthBuffer {
		vk::Format format = vk::Format::eD16Unorm;

		vk::Image image;
		vk::DeviceMemory memory;
		vk::ImageView view;
	} DepthBuffer;

	typedef struct UniformBuffer {
		vk::Buffer buffer;
		vk::DeviceMemory deviceMemory;
		vk::MemoryRequirements memoryRequirements;

		vk::DescriptorBufferInfo bufferInfo;

		uint8_t* cpuMemoryPtr;
	} UniformBuffer;

	typedef struct VertexBuffer {
		vk::Buffer buffer;
		vk::DeviceMemory deviceMemory;
		vk::DescriptorBufferInfo bufferInfo;

		uint8_t* cpuMemoryPtr;
	} VertexBuffer;

	typedef struct IndexBuffer {
		vk::Buffer buffer;
		vk::DeviceMemory deviceMemory;
		vk::DescriptorBufferInfo bufferInfo;
		vk::IndexType indexType;

		uint8_t* cpuMemoryPtr;
	} IndexBuffer;

	typedef struct Vertex {
		glm::vec4 position;
		glm::vec4 color;
		glm::vec3 normal;
	} Vertex;

	typedef struct RendererCapabilities {
		const size_t maxIndices;
		const size_t maxVertices;

		RendererCapabilities(size_t maxIndices)
			: maxIndices(maxIndices), maxVertices((maxIndices * 4) / 6)
		{ }
	} RendererCapabilities;

	typedef struct RendererStatistics {
		uint32_t drawCalls;
		
		size_t indices;
		size_t vertices;

		size_t quads;
	} RendererStatistics;

	class Renderer
	{
	public:
#if defined(CEE_OS_WINDOWS)
		Renderer(HINSTANCE connection, Window* window, RendererCapabilities capabilities);
#elif defined(CEE_WM_XCB)
		Renderer(xcb_connection_t* connection, Window* window, RendererCapabilities capabilities);
#endif
		~Renderer();

		void BeginScene(Camera& camera);
		void EndScene();

		void DrawQuad(glm::vec2 translation, glm::vec2 scale, float rotationAngle, glm::vec4 color);
		
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

		void Resize();

		const bool GetMemoryTypeFromProperties(uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask, uint32_t* typeIndex);

	private:
#if defined(CEE_OS_WINDOWS)
		static HINSTANCE s_Connection;
#elif defined(CEE_WM_XCB)
		static xcb_connection_t* s_Connection;
#endif

		Window* m_Window;
		
		const RendererCapabilities m_Capabilities;
		bool m_Prepared;
		bool m_Validate;

		RendererStatistics m_Statistics;
		
		vk::Instance m_Instance;
		vk::PhysicalDevice m_PhysicalDevice;
		vk::Device m_Device;
		vk::PhysicalDeviceProperties m_PhysicalDeviceProperties;
		vk::PhysicalDeviceMemoryProperties m_PhysicalDeviceMemoryProperties;
		uint32_t m_PhysicalDeviceCount = 1;
		uint32_t m_PreferredPhysicalDevice = 0;

		std::vector<char const*> m_EnabledLayerNames;
		std::vector<char const*> m_EnabledExtensionNames;

		vk::SurfaceKHR m_Surface;

		uint32_t m_QueueFamilyCount;
		std::unique_ptr<vk::QueueFamilyProperties[]> m_QueueFamilyProperties;
		uint32_t m_GraphicsQueueFamilyIndex = UINT32_MAX, m_PresentQueueFamilyIndex = UINT32_MAX;
		bool m_SeperatePresentQueue = false;

		vk::CommandPool m_CommandPool;
		vk::CommandBuffer m_CommandBuffer;

		vk::Queue m_GraphicsQueue, m_PresentQueue;

		vk::Format m_Format;
		vk::Extent2D m_SwapchainExtent;

		vk::SwapchainKHR m_Swapchain;
		uint32_t m_SwapchainImageCount = 0;
		std::unique_ptr<SwapchainResources[]> m_SwapchainResources;
		uint32_t m_CurrentBuffer;

		DepthBuffer m_DepthBuffer;
		UniformBuffer m_MvpBuffer;

		uint32_t m_DescriptorSetCount;
		std::unique_ptr<vk::DescriptorSetLayout[]> m_DescriptorSetLayouts;
		vk::DescriptorPool m_DescriptorPool;
		std::unique_ptr<vk::DescriptorSet[]> m_DescriptorSets;

		vk::PipelineLayout m_PipelineLayout;
		vk::Pipeline m_Pipeline;

		vk::RenderPass m_RenderPass;

		std::unique_ptr<Shader> m_Shader;

		std::unique_ptr<vk::Framebuffer[]> m_Framebuffers;

		VertexBuffer m_VertexBuffer;
		vk::VertexInputBindingDescription m_VertexInputBindingDescription;
		vk::VertexInputAttributeDescription m_VertexInputAttributeDescriptions[3];

		IndexBuffer m_IndexBuffer;

		vk::Viewport m_Viewport;
		vk::Rect2D m_ScissorRect;

		vk::Semaphore m_ImageAcquiredSemaphore;
		vk::Fence m_Fence;

		glm::mat4 m_Model, m_View, m_Projection;

		std::vector<Vertex> m_Vertices;
		
		vk::PolygonMode m_PolygonMode;
	};
}

#endif
