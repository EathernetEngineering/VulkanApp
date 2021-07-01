#include "pch.h"
#include "Renderer.hpp"

#include <vulkan/vulkan.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace CEE
{
	
	constexpr Vertex g_QuadVertices[] = {
		{ { -0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  0.5f,  0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ {  0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f, -0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, -1.0f } }
	};

#if defined(CEE_OS_WINDOWS)
	HINSTANCE Renderer::s_Connection = NULL;
#elif defined(CEE_WM_XCB)
	xcb_connection_t* Renderer::s_Connection = nullptr;
#endif
#if defined(CEE_OS_WINDOWS)
	Renderer::Renderer(HINSTANCE connection, Window* window, RendererCapabilities capabilities)
		: m_Capabilities(capabilities)
	{
		s_Connection = connection;
		m_Window = window;
		InitalizeRenderer();
	}
#elif defined(CEE_WM_XCB)
	Renderer::Renderer(xcb_connection_t* connection, Window* window, RendererCapabilities capabilities)
		: m_Capabilities(capabilities)
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
		memset(&m_Statistics, 0, sizeof(RendererStatistics));
		m_Prepared = true;
	}

	Renderer::~Renderer()
	{
		m_Device.unmapMemory(m_MvpBuffer.deviceMemory);
		m_Device.destroySemaphore(m_ImageAcquiredSemaphore, nullptr);
		m_Device.destroyFence(m_Fence, nullptr);
		m_Device.destroyPipeline(m_Pipeline, nullptr);
		m_Device.destroyDescriptorPool(m_DescriptorPool, nullptr);
		m_Shader.reset(nullptr);
		m_Device.destroyBuffer(m_IndexBuffer.buffer, nullptr);
		m_Device.freeMemory(m_IndexBuffer.deviceMemory, nullptr);
		m_Device.destroyBuffer(m_VertexBuffer.buffer, nullptr);
		m_Device.freeMemory(m_VertexBuffer.deviceMemory, nullptr);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
			m_Device.destroyFramebuffer(m_Framebuffers[i], nullptr);
		m_Device.destroyRenderPass(m_RenderPass, nullptr);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
			m_Device.destroyDescriptorSetLayout(m_DescriptorSetLayouts[i], nullptr);
		m_Device.destroyPipelineLayout(m_PipelineLayout, nullptr);
		m_Device.destroyBuffer(m_MvpBuffer.buffer, nullptr);
		m_Device.freeMemory(m_MvpBuffer.deviceMemory, nullptr);
		m_Device.destroyImageView(m_DepthBuffer.view, nullptr);
		m_Device.destroyImage(m_DepthBuffer.image, nullptr);
		m_Device.freeMemory(m_DepthBuffer.memory, nullptr);
		vk::CommandBuffer commandBuffers[] =
		{
			m_CommandBuffer
		};
		m_Device.freeCommandBuffers(m_CommandPool, sizeof(commandBuffers) / sizeof(commandBuffers[0]), commandBuffers);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
			m_Device.destroyImageView(m_SwapchainResources[i].view, nullptr);
		m_Device.destroySwapchainKHR(m_Swapchain, nullptr);
		m_Device.destroyCommandPool(m_CommandPool, nullptr);
		m_Device.waitIdle();
		m_Device.destroy(nullptr);
		m_Instance.destroySurfaceKHR(m_Surface, nullptr);
		m_Instance.destroy();
	}
	
	void Renderer::InitalizeInstance()
	{
		uint32_t instanceExtensionCount = 0;
		uint32_t instanceLayerCount = 0;
		std::vector<char const*> instanceValidationLayers = { "VK_LAYER_KHRONOS_validation" };
		
#if defined(NDEBUG)
		m_Validate = false;
#else
		m_Validate = true;
#endif
		
		vk::Bool32 validationFound = VK_FALSE;
		if (m_Validate)
		{
			auto result = vk::enumerateInstanceLayerProperties(&instanceLayerCount, static_cast<vk::LayerProperties*>(nullptr));
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate instance layer properties");
			if (instanceLayerCount > 0)
			{
				std::unique_ptr<vk::LayerProperties[]> instanceLayers(new vk::LayerProperties[instanceLayerCount]);
				result = vk::enumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.get());
				CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate instance layer properties");

				validationFound = VK_TRUE;
				for (uint32_t i = 0; i < instanceValidationLayers.size(); i++)
				{
					bool found = false;
					for (uint32_t j = 0; j < instanceLayerCount; j++)
					{
						if (strcmp(instanceValidationLayers[i], instanceLayers[j].layerName) != 0)
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						fprintf(stderr, "Cannot find Layer %s\n", instanceValidationLayers[i]);
						validationFound = VK_FALSE;
					}
				}
			}
			if (validationFound == VK_TRUE)
			{
				m_EnabledLayerNames = instanceValidationLayers;
			}
		}

		vk::Bool32 surfaceExtFound = VK_FALSE;
		vk::Bool32 platformExtFound = VK_FALSE;

		auto result = vk::enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, static_cast<vk::ExtensionProperties*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate instance extension properties");

		if (instanceExtensionCount > 0)
		{
			std::unique_ptr<vk::ExtensionProperties[]> extenstionProperties(new vk::ExtensionProperties[instanceExtensionCount]);
			result = vk::enumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, extenstionProperties.get());
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate instance extension properties");

			for (uint32_t i = 0; i < instanceExtensionCount; i++)
			{
				if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, extenstionProperties[i].extensionName))
				{
					surfaceExtFound = VK_TRUE;
					m_EnabledExtensionNames.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
				}
#if defined(CEE_OS_WINDOWS)
				if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, extenstionProperties[i].extensionName))
				{
					platformExtFound = VK_TRUE;
					m_EnabledExtensionNames.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
				}
#elif defined(CEE_WM_XCB)
				if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, extenstionProperties[i].extensionName))
				{
					platformExtFound = VK_TRUE;
					m_EnabledExtensionNames.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
				}
#endif
				CEE_ASSERT(m_EnabledExtensionNames.size() < 64);
			}
		}

		CEE_ASSERT(surfaceExtFound && platformExtFound);

		auto const appInfo = vk::ApplicationInfo()
			.setApiVersion(VK_MAKE_API_VERSION(0, 1, 2, 0))
			.setApplicationVersion(0)
			.setEngineVersion(0)
			.setPEngineName("Vulkan Engine")
			.setPApplicationName("Vulkan Application");

		auto instanceCreateInfo = vk::InstanceCreateInfo()
			.setPApplicationInfo(&appInfo).setFlags(vk::InstanceCreateFlags()).setPNext(nullptr);
		
		instanceCreateInfo.setEnabledLayerCount(static_cast<uint32_t>(m_EnabledLayerNames.size()));
		if (m_EnabledLayerNames.size() > 0)
			instanceCreateInfo.setPpEnabledLayerNames(m_EnabledLayerNames.data());
		
		instanceCreateInfo.setEnabledExtensionCount(static_cast<uint32_t>(m_EnabledExtensionNames.size()));
		if (m_EnabledExtensionNames.size() > 0)
			instanceCreateInfo.setPpEnabledExtensionNames((char const* const*)m_EnabledExtensionNames.data());

		result = vk::createInstance(&instanceCreateInfo, nullptr, &m_Instance);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create instance");

		result = m_Instance.enumeratePhysicalDevices(&m_PhysicalDeviceCount, static_cast<vk::PhysicalDevice*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate physical devices");
		CEE_ASSERT_WITH_MESSAGE(m_PhysicalDeviceCount > 0, "No physical devices present");

		std::unique_ptr<vk::PhysicalDevice[]> physialDevices(new vk::PhysicalDevice[m_PhysicalDeviceCount]);
		result = m_Instance.enumeratePhysicalDevices(&m_PhysicalDeviceCount, physialDevices.get());

		if (m_PreferredPhysicalDevice > m_PhysicalDeviceCount - 1)
		{
			fprintf(stderr, "Physical device %u is set as the preferred physical device, however is not present\n"
					"\t Constinuing with GPU 0.\n", m_PreferredPhysicalDevice);
			m_PreferredPhysicalDevice = 0;
		}

		m_PhysicalDevice = physialDevices[m_PreferredPhysicalDevice];

		m_PhysicalDevice.getProperties(&m_PhysicalDeviceProperties);

		char const* DeviceTypeNames[]
		{
			"Unknown type",
			"Integrated GPU",
			"Descrete GPU",
			"Virtual GPU",
			"CPU"
		};

		uint32_t version = m_PhysicalDeviceProperties.apiVersion;
		printf("Pyhsical device properties:\n"
			   "\tDevice Name: %s\n"
			   "\tDevice Type: %s\n"
			   "\tApi Version: %u.%u.%u\n",
			   m_PhysicalDeviceProperties.deviceName.data(),
			   DeviceTypeNames[(uint32_t)m_PhysicalDeviceProperties.deviceType],
			   VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version));

		m_PhysicalDevice.getMemoryProperties(&m_PhysicalDeviceMemoryProperties);
	}
	
	void Renderer::InitalizeSurface()
	{
#if defined(CEE_OS_WINDOWS)
		auto const surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR()
			.setHinstance(s_Connection).setHwnd(m_Window->GetNativeWindowPtr());

		auto result = m_Instance.createWin32SurfaceKHR(&surfaceCreateInfo, nullptr, &m_Surface);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create WIN32 surface.");;
#elif defined(CEE_WM_XCB)
		auto const surfaceCreateInfo = vk::XcbSurfaceCreateInfoKHR()
			.setConnection(s_Connection).setWindow(m_Window->GetNativeWindowPtr());

		auto result = m_Instance.createXcbSurfaceKHR(&surfaceCreateInfo, nullptr, &m_Surface);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create XCB surface.");;
#endif
	}
	
	void Renderer::InitalizeDevice()
	{
		m_EnabledExtensionNames.clear();

		vk::Bool32 swapchainExtensionFound = VK_FALSE;
		uint32_t deviceExtensionCount = 0;
		
		auto result = m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, static_cast<vk::ExtensionProperties*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate device extensions.");
		if (deviceExtensionCount > 0)
		{
			std::unique_ptr<vk::ExtensionProperties[]> deviceExtensions(new vk::ExtensionProperties[deviceExtensionCount]);
			result = m_PhysicalDevice.enumerateDeviceExtensionProperties(nullptr, &deviceExtensionCount, deviceExtensions.get());
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to enumerate device extensions.");

			for (uint32_t i = 0; i < deviceExtensionCount; i++)
			{
				if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, deviceExtensions[i].extensionName))
				{
					swapchainExtensionFound = VK_TRUE;
					m_EnabledExtensionNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
				}
			}
			CEE_ASSERT(m_EnabledExtensionNames.size() < 64);
		}

		CEE_ASSERT(swapchainExtensionFound == VK_TRUE);

		auto deviceQueueCreateInfo = vk::DeviceQueueCreateInfo();
		m_PhysicalDevice.getQueueFamilyProperties(&m_QueueFamilyCount, static_cast<vk::QueueFamilyProperties*>(nullptr));
		CEE_ASSERT(m_QueueFamilyCount > 0);

		m_QueueFamilyProperties.reset(new vk::QueueFamilyProperties[m_QueueFamilyCount]);
		m_PhysicalDevice.getQueueFamilyProperties(&m_QueueFamilyCount, m_QueueFamilyProperties.get());
		CEE_ASSERT(m_QueueFamilyCount > 0);

		std::unique_ptr<vk::Bool32[]> supportsPresent(new vk::Bool32[m_QueueFamilyCount]);
		for (uint32_t i = 0; i < m_QueueFamilyCount; i++)
		{
			result = m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface, &supportsPresent[i]);
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface supports present");
		}

		for (uint32_t i = 0; i < m_QueueFamilyCount; i++)
		{
			if (m_QueueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics)
			{
				if (m_GraphicsQueueFamilyIndex == UINT32_MAX)
				{
					m_GraphicsQueueFamilyIndex = i;
				}
				if (supportsPresent[i] == VK_TRUE)
				{
					m_GraphicsQueueFamilyIndex = i;
					m_PresentQueueFamilyIndex = i;
				}
			}
		}
		if (m_PresentQueueFamilyIndex == UINT32_MAX)
		{
			for (uint32_t i = 0; i < m_GraphicsQueueFamilyIndex; i++)
			{
				if (supportsPresent[i] == VK_TRUE)
				{
					m_PresentQueueFamilyIndex = i;
					break;
				}
			}
			m_SeperatePresentQueue = true;
		}
		if (m_GraphicsQueueFamilyIndex == UINT32_MAX || m_PresentQueueFamilyIndex == UINT32_MAX)
		{
			if (m_GraphicsQueueFamilyIndex == UINT32_MAX)
				fprintf(stderr, "Failed to find graphics queue index.\n\tTerminating Program.\t");
			else
				fprintf(stderr, "Failed to find present queue index.\n\tTerminating Program.\t");
			exit(-1);
		}

		float const priorities[] = { 1.0f };
		const vk::DeviceQueueCreateInfo deviceQueueCreateInfos[] = {
			vk::DeviceQueueCreateInfo()
				.setPQueuePriorities(priorities)
				.setQueueCount(1)
				.setQueueFamilyIndex(m_GraphicsQueueFamilyIndex),
			vk::DeviceQueueCreateInfo()
				.setPQueuePriorities(priorities)
				.setQueueCount(1)
				.setQueueFamilyIndex(m_PresentQueueFamilyIndex)
		};
		auto deviceCreateInfo = vk::DeviceCreateInfo()
			.setPQueueCreateInfos(deviceQueueCreateInfos)
			.setEnabledExtensionCount(static_cast<uint32_t>(m_EnabledExtensionNames.size()))
			.setPpEnabledExtensionNames(m_EnabledExtensionNames.data())
			.setEnabledLayerCount(0)
			.setPpEnabledLayerNames(nullptr)
			.setPEnabledFeatures(nullptr);
		if (m_SeperatePresentQueue) deviceCreateInfo.setQueueCreateInfoCount(2);
		else deviceCreateInfo.setQueueCreateInfoCount(1);
		
		result = m_PhysicalDevice.createDevice(&deviceCreateInfo, nullptr, &m_Device);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create device.");
	}
	
	void Renderer::InitalizeCommandBuffer()
	{
		auto const commandPoolCreateInfo = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(m_GraphicsQueueFamilyIndex)
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		auto result = m_Device.createCommandPool(&commandPoolCreateInfo, nullptr, &m_CommandPool);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create command pool.");

		auto commandBufferAllocateInfo = vk::CommandBufferAllocateInfo()
			.setCommandBufferCount(1)
			.setCommandPool(m_CommandPool);

		result = m_Device.allocateCommandBuffers(&commandBufferAllocateInfo, &m_CommandBuffer);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to allocate command buffer.");

		auto const beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse).setPInheritanceInfo(nullptr);
		result = m_CommandBuffer.begin(&beginInfo);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to start recording command buffer.");

		m_Device.getQueue(m_GraphicsQueueFamilyIndex, 0, &m_GraphicsQueue);
		if (m_SeperatePresentQueue) m_Device.getQueue(m_PresentQueueFamilyIndex, 0, &m_PresentQueue);
		else m_PresentQueue = m_GraphicsQueue;
	}
	
	void Renderer::InitalizeSwapchain()
	{
		uint32_t formatCount = 0;
		auto result = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface, &formatCount, static_cast<vk::SurfaceFormatKHR*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface formats.");

		std::unique_ptr<vk::SurfaceFormatKHR[]> surfaceFormats(new vk::SurfaceFormatKHR[formatCount]);
		result = m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface, &formatCount, surfaceFormats.get());
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface formats.");

		if (formatCount == 1 && surfaceFormats[0].format == vk::Format::eUndefined)
		{
			m_Format = vk::Format::eR8G8B8A8Unorm;
		}
		else
		{
			CEE_ASSERT(formatCount > 0);
			m_Format = surfaceFormats[0].format;
		}

		vk::SurfaceCapabilitiesKHR surfaceCapabilities;
		result = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface, &surfaceCapabilities);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface capabilities.");

		uint32_t presentModeCount = 0;
		result = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface, &presentModeCount, static_cast<vk::PresentModeKHR*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface present modes.");

		std::unique_ptr<vk::PresentModeKHR[]> presentModes(new vk::PresentModeKHR[presentModeCount]);
		result = m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface, &presentModeCount, presentModes.get());
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get surface present modes.");

		vk::Extent2D swapchainExtent;
		if (surfaceCapabilities.currentExtent.width == UINT32_MAX)
		{
			swapchainExtent.width = m_Window->GetWidth();
			swapchainExtent.height = m_Window->GetHeight();
			if (swapchainExtent.width < surfaceCapabilities.minImageExtent.width)
				swapchainExtent.width = surfaceCapabilities.minImageExtent.width;
			else if (swapchainExtent.width > surfaceCapabilities.maxImageExtent.width)
				swapchainExtent.width = surfaceCapabilities.maxImageExtent.width;
			if (swapchainExtent.height < surfaceCapabilities.minImageExtent.height)
				swapchainExtent.height = surfaceCapabilities.minImageExtent.height;
			else if (swapchainExtent.height > surfaceCapabilities.maxImageExtent.height)
				swapchainExtent.height = surfaceCapabilities.maxImageExtent.height;
		}
		else
			swapchainExtent = surfaceCapabilities.currentExtent;
		m_SwapchainExtent = swapchainExtent;

		vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
		uint32_t desiredNumberOfSwapchainImages = surfaceCapabilities.minImageCount;

		vk::SurfaceTransformFlagBitsKHR preTransform;
		if (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity)
		{
			preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
		}
		else
		{
			preTransform = surfaceCapabilities.currentTransform;
		}

		vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		vk::CompositeAlphaFlagBitsKHR compositeFlags[] = {
			vk::CompositeAlphaFlagBitsKHR::eOpaque,
			vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
			vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
			vk::CompositeAlphaFlagBitsKHR::eInherit
		};
		for (uint32_t i = 0; i < sizeof(compositeFlags) / sizeof(compositeFlags[0]); i++)
		{
			if (surfaceCapabilities.supportedCompositeAlpha & compositeFlags[i])
			{
				compositeAlpha = compositeFlags[i];
				break;
			}
		}

		auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR()
			.setSurface(m_Surface)
			.setMinImageCount(desiredNumberOfSwapchainImages)
			.setImageFormat(m_Format)
			.setImageExtent(m_SwapchainExtent)
			.setPreTransform(preTransform)
			.setCompositeAlpha(compositeAlpha)
			.setImageArrayLayers(1)
			.setPresentMode(presentMode)
			.setOldSwapchain(nullptr)
			.setClipped(true)
			.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr);
		if (m_SeperatePresentQueue)
		{
			uint32_t queueFamilyIndices[2] = { m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex };
			swapchainCreateInfo.setImageSharingMode(vk::SharingMode::eConcurrent);
			swapchainCreateInfo.setQueueFamilyIndexCount(2);
			swapchainCreateInfo.setPQueueFamilyIndices(queueFamilyIndices);
		}

		result = m_Device.createSwapchainKHR(&swapchainCreateInfo, nullptr, &m_Swapchain);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create swapchain.");

		result = m_Device.getSwapchainImagesKHR(m_Swapchain, &m_SwapchainImageCount, static_cast<vk::Image*>(nullptr));
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get swapchain images.");

		std::unique_ptr<vk::Image[]> swapchainImages(new vk::Image[m_SwapchainImageCount]);
		result = m_Device.getSwapchainImagesKHR(m_Swapchain, &m_SwapchainImageCount, swapchainImages.get());
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to get swapchain images.");

		m_SwapchainResources.reset(new SwapchainResources[m_SwapchainImageCount]);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
		{
			m_SwapchainResources[i].image = swapchainImages[i];
		}
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
		{
			auto const swapchainImageViewCreateInfo = vk::ImageViewCreateInfo()
				.setImage(m_SwapchainResources[i].image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(m_Format)
				.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))
				.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

			result = m_Device.createImageView(&swapchainImageViewCreateInfo, nullptr, &m_SwapchainResources[i].view);
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create swapchain image view.");
		}
		m_CurrentBuffer = 0;
	}
	
	void Renderer::InitalizeDepthBuffer()
	{
		m_DepthBuffer.format = vk::Format::eD16Unorm;
		vk::ImageTiling tiling;
		vk::FormatProperties formatProperties;
		m_PhysicalDevice.getFormatProperties(m_DepthBuffer.format, &formatProperties);
		if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			tiling = vk::ImageTiling::eLinear;
		else if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
			tiling = vk::ImageTiling::eOptimal;
		else
		{
			CEE_ASSERT_WITH_MESSAGE(false, "vk::Format::eD16Unorm unsupported\n\tTry other depth options?\n");
		}

		auto const imageCreateInfo = vk::ImageCreateInfo()
			.setImageType(vk::ImageType::e2D)
			.setFormat(m_DepthBuffer.format)
			.setExtent(vk::Extent3D(m_SwapchainExtent.width, m_SwapchainExtent.height, 1))
			.setMipLevels(1)
			.setArrayLayers(1)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setTiling(tiling)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setSharingMode(vk::SharingMode::eExclusive);

		auto result = m_Device.createImage(&imageCreateInfo, nullptr, &m_DepthBuffer.image);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create depth buffer image.");

		vk::MemoryRequirements memoryRequirements;
		m_Device.getImageMemoryRequirements(m_DepthBuffer.image, &memoryRequirements);

		uint32_t memoryTypeIndex;
		bool pass = GetMemoryTypeFromProperties(memoryRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal, &memoryTypeIndex);
		CEE_ASSERT_WITH_MESSAGE(pass, "Required memory type for depth buffer not supported.");

		auto const memoryAllocateInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memoryRequirements.size)
			.setMemoryTypeIndex(memoryTypeIndex);

		result = m_Device.allocateMemory(&memoryAllocateInfo, nullptr, &m_DepthBuffer.memory);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess , "Unable to allocate memory for depth buffer.");

		m_Device.bindImageMemory(m_DepthBuffer.image, m_DepthBuffer.memory, 0);

		auto const imageViewCreateInfo = vk::ImageViewCreateInfo()
			.setImage(m_DepthBuffer.image)
			.setFormat(m_DepthBuffer.format)
			.setComponents(vk::ComponentMapping(vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA))
			.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1))
			.setViewType(vk::ImageViewType::e2D);

		result = m_Device.createImageView(&imageViewCreateInfo, nullptr, &m_DepthBuffer.view);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to craete image view for depth buffer.");
	}

	
	
	void Renderer::InitalizeUniformBuffer()
	{
		// MVP uniform buffer.
		{
			m_Model = glm::identity<glm::mat4>();
			m_View = glm::identity<glm::mat4>();
			m_Projection = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

			glm::mat4 mvp = m_Model * m_View * m_Projection;

			auto const unifromBufferCreateInfo = vk::BufferCreateInfo()
				.setPQueueFamilyIndices(nullptr)
				.setQueueFamilyIndexCount(0)
				.setSharingMode(vk::SharingMode::eExclusive)
				.setSize(sizeof(glm::mat4))
				.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

			auto result = m_Device.createBuffer(&unifromBufferCreateInfo, nullptr, &m_MvpBuffer.buffer);
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create MVP uniform buffer.");

			m_Device.getBufferMemoryRequirements(m_MvpBuffer.buffer, &m_MvpBuffer.memoryRequirements);

			vk::MemoryPropertyFlags typeBits = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
			uint32_t typeIndex;
			bool pass = GetMemoryTypeFromProperties(m_MvpBuffer.memoryRequirements.memoryTypeBits, typeBits, &typeIndex);
			CEE_ASSERT_WITH_MESSAGE(pass, "Required memory type for MVP uniform buffer not supported.");

			auto const memoryAllocateInfo = vk::MemoryAllocateInfo()
				.setAllocationSize(m_MvpBuffer.memoryRequirements.size)
				.setMemoryTypeIndex(typeIndex);

			result = m_Device.allocateMemory(&memoryAllocateInfo, nullptr, &m_MvpBuffer.deviceMemory);
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to allocate memory for MVP uniform buffer.");

			result = m_Device.mapMemory(m_MvpBuffer.deviceMemory, 0, m_MvpBuffer.memoryRequirements.size, vk::MemoryMapFlags(), reinterpret_cast<void**>(&m_MvpBuffer.cpuMemoryPtr));
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to map memory for MVP uniform buffer.");

			memcpy(m_MvpBuffer.cpuMemoryPtr, &mvp, sizeof(mvp));

			m_Device.bindBufferMemory(m_MvpBuffer.buffer, m_MvpBuffer.deviceMemory, 0);

			m_MvpBuffer.bufferInfo.setBuffer(m_MvpBuffer.buffer).setOffset(0).setRange(sizeof(mvp));
		}
		// Lighting uniform buffer.
		{

		}
	}
	
	void Renderer::InitalizePipelineLayout()
	{
		const vk::DescriptorSetLayoutBinding layoutBindings[]
		{
			vk::DescriptorSetLayoutBinding()
				.setBinding(0).setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1).setStageFlags(vk::ShaderStageFlagBits::eVertex)
				.setPImmutableSamplers(nullptr),
			vk::DescriptorSetLayoutBinding()
				.setBinding(1).setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)
				.setPImmutableSamplers(nullptr)
		};

		auto const descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo()
			.setBindingCount(sizeof(layoutBindings) / sizeof(layoutBindings[0]))
			.setPBindings(layoutBindings);

		m_DescriptorSetLayouts.reset(new vk::DescriptorSetLayout[1]);
		auto result = m_Device.createDescriptorSetLayout(&descriptorSetLayoutCreateInfo, nullptr, m_DescriptorSetLayouts.get());
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to create descriptor set layout.");

		auto const pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo()
			.setPushConstantRangeCount(0)
			.setPPushConstantRanges(nullptr)
			.setSetLayoutCount(1)
			.setPSetLayouts(m_DescriptorSetLayouts.get());

		result = m_Device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to create pipeline laytout");
	}
	
	void Renderer::InitalizeDescriptorSet()
	{
		m_DescriptorSetCount = 1;

		vk::DescriptorPoolSize typeCounts[] =
		{
			vk::DescriptorPoolSize().setDescriptorCount(2).setType(vk::DescriptorType::eUniformBuffer)
		};
		auto const descriptorPoolCreateInfo = vk::DescriptorPoolCreateInfo()
			.setPoolSizeCount(1)
			.setPPoolSizes(typeCounts)
			.setMaxSets(m_DescriptorSetCount);

		auto result = m_Device.createDescriptorPool(&descriptorPoolCreateInfo, nullptr, &m_DescriptorPool);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to create descriptor pool.");

		vk::DescriptorSetAllocateInfo const descriptorSetAllocateInfo[] =
		{
			vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(m_DescriptorPool)
				.setDescriptorSetCount(m_DescriptorSetCount)
				.setPSetLayouts(m_DescriptorSetLayouts.get())
		};

		m_DescriptorSets.reset(new vk::DescriptorSet[m_DescriptorSetCount]);
		result = m_Device.allocateDescriptorSets(descriptorSetAllocateInfo, m_DescriptorSets.get());
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to allocate memory for descriptor sets.");

		const vk::WriteDescriptorSet writeDescriptorSets[] =
		{
			vk::WriteDescriptorSet()
			.setDstSet(m_DescriptorSets[0])
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setPBufferInfo(&m_MvpBuffer.bufferInfo)
			.setDstArrayElement(0)
			.setDstBinding(0)
		};
		m_Device.updateDescriptorSets((sizeof(writeDescriptorSets) / sizeof(writeDescriptorSets[0])), writeDescriptorSets, 0, nullptr);
	}
	
	void Renderer::InitalizeRenderPass()
	{
		vk::AttachmentDescription attachmentDescriptions[] =
		{
			vk::AttachmentDescription()
				.setFormat(m_Format)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eStore)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setFinalLayout(vk::ImageLayout::ePresentSrcKHR),
			vk::AttachmentDescription()
				.setFormat(m_DepthBuffer.format)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setLoadOp(vk::AttachmentLoadOp::eClear)
				.setStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
				.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
		};

		auto const colorReference = vk::AttachmentReference()
			.setAttachment(0).setLayout(vk::ImageLayout::eColorAttachmentOptimal);
		auto const depthReference = vk::AttachmentReference()
			.setAttachment(1).setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

		auto const subpassDescription = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
			.setInputAttachmentCount(0)
			.setPInputAttachments(nullptr)
			.setColorAttachmentCount(1)
			.setPColorAttachments(&colorReference)
			.setPResolveAttachments(nullptr)
			.setPDepthStencilAttachment(&depthReference)
			.setPreserveAttachmentCount(0)
			.setPPreserveAttachments(nullptr);

		auto const subpassDependency = vk::SubpassDependency()
			.setSrcSubpass(VK_SUBPASS_EXTERNAL)
			.setDstSubpass(0)
			.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
			.setSrcAccessMask(vk::AccessFlags())
			.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

		auto const RenderPassCreateInfo = vk::RenderPassCreateInfo()
			.setAttachmentCount(sizeof(attachmentDescriptions) / sizeof(attachmentDescriptions[0]))
			.setPAttachments(attachmentDescriptions)
			.setSubpassCount(1)
			.setPSubpasses(&subpassDescription)
			.setDependencyCount(1)
			.setPDependencies(&subpassDependency);

		auto result = m_Device.createRenderPass(&RenderPassCreateInfo, nullptr, &m_RenderPass);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "failed to create render pass.");
	}
	
	void Renderer::InitalizeShaders()
	{
		m_Shader = std::make_unique<Shader>(&m_Device);

		auto result = m_Shader->CompileShadersFromFiles("../res/shaders/basic.vert", "../res/shaders/basic.frag");
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to compile shaders");
	}
	
	void Renderer::InitalizeFramebuffers()
	{
		vk::ImageView attachments[2];
		attachments[1] = m_DepthBuffer.view;

		auto const FrameBufferCreateInfo = vk::FramebufferCreateInfo()
			.setRenderPass(m_RenderPass)
			.setAttachmentCount(2)
			.setPAttachments(attachments)
			.setWidth(m_SwapchainExtent.width)
			.setHeight(m_SwapchainExtent.height)
			.setLayers(1);

		m_Framebuffers.reset(new vk::Framebuffer[m_SwapchainImageCount]);
		CEE_ASSERT(m_Framebuffers != NULL);

		vk::Result result = vk::Result::eErrorUnknown;
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
		{
			attachments[0] = m_SwapchainResources[i].view;
			result = m_Device.createFramebuffer(&FrameBufferCreateInfo, nullptr, &m_Framebuffers[i]);
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create framebuffer.");
		}
	}
	
	void Renderer::InitalizeVertexBuffer()
	{
		auto const vertexBufferCreateInfo = vk::BufferCreateInfo()
			.setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
			.setSize(m_Capabilities.maxVertices * sizeof(Vertex))
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setSharingMode(vk::SharingMode::eExclusive);

		auto result = m_Device.createBuffer(&vertexBufferCreateInfo, nullptr, &m_VertexBuffer.buffer);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create vertex buffer.");

		vk::MemoryRequirements memoryRequirements;
		m_Device.getBufferMemoryRequirements(m_VertexBuffer.buffer, &memoryRequirements);

		uint32_t memoryTypeIndex;
		vk::MemoryPropertyFlags typeBits = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bool pass = GetMemoryTypeFromProperties(memoryRequirements.memoryTypeBits, typeBits, &memoryTypeIndex);
		CEE_ASSERT_WITH_MESSAGE(pass, "Required memory type for vertex buffer not supported.");

		auto const vertexBufferAllocateInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memoryRequirements.size)
			.setMemoryTypeIndex(memoryTypeIndex);

		result = m_Device.allocateMemory(&vertexBufferAllocateInfo, nullptr, &m_VertexBuffer.deviceMemory);
		CEE_ASSERT_WITH_MESSAGE(pass, "No mappable, coherant memory.");

		result = m_Device.mapMemory(m_VertexBuffer.deviceMemory, 0, memoryRequirements.size, vk::MemoryMapFlags(), (void**)&m_VertexBuffer.cpuMemoryPtr);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to map memory for vertex buffer.");

		//memset(&m_VertexBuffer.cpuMemoryPtr, 0, memoryRequirements.size);

		m_Device.bindBufferMemory(m_VertexBuffer.buffer, m_VertexBuffer.deviceMemory, 0);

		m_VertexInputBindingDescription.setBinding(0).setInputRate(vk::VertexInputRate::eVertex).setStride(sizeof(Vertex));

		m_VertexInputAttributeDescriptions[0]
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(0);
		m_VertexInputAttributeDescriptions[1]
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32A32Sfloat)
			.setOffset(sizeof(float) * 4);
		m_VertexInputAttributeDescriptions[2]
			.setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset((sizeof(float) * 4) + (sizeof(float) * 4));

		m_VertexBuffer.bufferInfo.setBuffer(m_VertexBuffer.buffer).setOffset(0).setRange(memoryRequirements.size);
	}
	
	void Renderer::InitalizeIndexBuffer()
	{
		auto const indexBufferCreateInfo = vk::BufferCreateInfo()
			.setSize(m_Capabilities.maxIndices * sizeof(uint16_t))
			.setUsage(vk::BufferUsageFlagBits::eIndexBuffer)
			.setQueueFamilyIndexCount(0)
			.setPQueueFamilyIndices(nullptr)
			.setSharingMode(vk::SharingMode::eExclusive);

		auto result = m_Device.createBuffer(&indexBufferCreateInfo, nullptr, &m_IndexBuffer.buffer);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create index buffer.");

		vk::MemoryRequirements memoryRequirements;
		m_Device.getBufferMemoryRequirements(m_IndexBuffer.buffer, &memoryRequirements);

		uint32_t typeIndex;
		vk::MemoryPropertyFlags typeBits = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
		bool pass = GetMemoryTypeFromProperties(memoryRequirements.memoryTypeBits, typeBits, &typeIndex);
		CEE_ASSERT_WITH_MESSAGE(pass, "No mappable, coherant memory.");

		auto const indexBufferAllocateInfo = vk::MemoryAllocateInfo()
			.setAllocationSize(memoryRequirements.size)
			.setMemoryTypeIndex(typeIndex);
		result = m_Device.allocateMemory(&indexBufferAllocateInfo, nullptr, &m_IndexBuffer.deviceMemory);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to allocate memory for index buffer.");

		result = m_Device.mapMemory(m_IndexBuffer.deviceMemory, 0, memoryRequirements.size, vk::MemoryMapFlags(), (void**)&m_IndexBuffer.cpuMemoryPtr);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, {"Failed to map memory for index buffer.");

		{
			uint16_t* indices = (uint16_t*)malloc(memoryRequirements.size);
			CEE_ASSERT(indices != NULL);

			for (uint16_t i = 0; i < m_Capabilities.maxIndices - 6; i += 6)
			{
				indices[i + 0] = i + 0;
				indices[i + 1] = i + 1;
				indices[i + 2] = i + 2;

				indices[i + 3] = i + 2;
				indices[i + 4] = i + 3;
				indices[i + 5] = i + 0;
			}
			memcpy(m_IndexBuffer.cpuMemoryPtr, indices, memoryRequirements.size);
			free(indices);
		}
		m_Device.unmapMemory(m_IndexBuffer.deviceMemory);
		m_IndexBuffer.cpuMemoryPtr = nullptr;

		m_IndexBuffer.bufferInfo.setBuffer(m_IndexBuffer.buffer).setOffset(0).setRange(m_Capabilities.maxIndices * sizeof(uint16_t));
		m_IndexBuffer.indexType = vk::IndexType::eUint16;

		m_Device.bindBufferMemory(m_IndexBuffer.buffer, m_IndexBuffer.deviceMemory, 0);
	}
	
	void Renderer::InitalizePipeline()
	{
		vk::DynamicState stateEnables[2];
		memset(stateEnables, 0, sizeof(stateEnables));
		auto dynamicState = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStateCount(0).setPDynamicStates(stateEnables);

		auto const vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptionCount(1)
			.setPVertexBindingDescriptions(&m_VertexInputBindingDescription)
			.setVertexAttributeDescriptionCount(3)
			.setPVertexAttributeDescriptions(m_VertexInputAttributeDescriptions);

		auto const inputAssemblyStateCreateInfo = vk::PipelineInputAssemblyStateCreateInfo()
			.setPrimitiveRestartEnable(VK_FALSE)
			.setTopology(vk::PrimitiveTopology::eTriangleList);

		auto const rasterizationStateCreateInfo = vk::PipelineRasterizationStateCreateInfo()
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthClampEnable(VK_FALSE)
			.setRasterizerDiscardEnable(VK_FALSE)
			.setDepthBiasEnable(VK_FALSE)
			.setDepthBiasClamp(0)
			.setDepthBiasConstantFactor(0)
			.setDepthBiasSlopeFactor(0)
			.setLineWidth(1.0f);

		vk::PipelineColorBlendAttachmentState const colorBlendAttachmentStates[] = {
			vk::PipelineColorBlendAttachmentState()
			.setColorWriteMask(vk::ColorComponentFlags(0xF))
			.setBlendEnable(VK_FALSE)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcColorBlendFactor(vk::BlendFactor::eZero)
			.setDstColorBlendFactor(vk::BlendFactor::eZero)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
			.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
		};

		auto const colorBlendStateCreateInfo = vk::PipelineColorBlendStateCreateInfo()
			.setAttachmentCount(1)
			.setPAttachments(colorBlendAttachmentStates)
			.setLogicOpEnable(VK_FALSE)
			.setLogicOp(vk::LogicOp::eNoOp)
			.setBlendConstants(std::array<float, 4>({ 1.0f, 1.0f, 1.0f, 1.0f }));

		auto const viewportStateCreateInfo = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setScissorCount(1)
			.setPViewports(nullptr)
			.setPScissors(nullptr);
		stateEnables[dynamicState.dynamicStateCount++] = vk::DynamicState::eViewport;
		stateEnables[dynamicState.dynamicStateCount++] = vk::DynamicState::eScissor;

		auto const depthStencilStateCreateInfo = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(VK_TRUE)
			.setDepthWriteEnable(VK_TRUE)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
			.setDepthBoundsTestEnable(VK_FALSE)
			.setMinDepthBounds(0.0f)
			.setMaxDepthBounds(0.0f)
			.setStencilTestEnable(VK_FALSE)
			.setBack(vk::StencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, {}, {}, {}))
			.setFront(vk::StencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways, {}, {}, {}));

		auto const multisampleStateCreateInfo = vk::PipelineMultisampleStateCreateInfo()
			.setPSampleMask(nullptr)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setSampleShadingEnable(VK_FALSE)
			.setAlphaToCoverageEnable(VK_FALSE)
			.setAlphaToOneEnable(VK_FALSE)
			.setMinSampleShading(0.0f);

		vk::PipelineShaderStageCreateInfo shaderStageCreateInfo[] = {
			vk::PipelineShaderStageCreateInfo()
			.setModule(m_Shader->GetVertexModule())
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setPSpecializationInfo(nullptr),
			vk::PipelineShaderStageCreateInfo()
			.setModule(m_Shader->GetFragmentModule())
			.setPName("main")
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setPSpecializationInfo(nullptr)
		};

		auto const graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo()
			.setLayout(m_PipelineLayout)
			.setBasePipelineIndex(0)
			.setBasePipelineHandle(nullptr)
			.setPVertexInputState(&vertexInputStateCreateInfo)
			.setPInputAssemblyState(&inputAssemblyStateCreateInfo)
			.setPColorBlendState(&colorBlendStateCreateInfo)
			.setPRasterizationState(&rasterizationStateCreateInfo)
			.setPTessellationState(nullptr)
			.setPMultisampleState(&multisampleStateCreateInfo)
			.setPDynamicState(&dynamicState)
			.setPViewportState(&viewportStateCreateInfo)
			.setPDepthStencilState(&depthStencilStateCreateInfo)
			.setStageCount(2)
			.setPStages(shaderStageCreateInfo)
			.setRenderPass(m_RenderPass)
			.setSubpass(0);

		auto result = m_Device.createGraphicsPipelines(nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &m_Pipeline);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create graphics pipeline.");

		m_Viewport = vk::Viewport()
			.setWidth((float)m_SwapchainExtent.width)
			.setHeight((float)m_SwapchainExtent.height)
			.setX(0)
			.setY(0)
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		m_ScissorRect = vk::Rect2D(vk::Offset2D(0, 0), m_SwapchainExtent);
	}
	
	void Renderer::InitalizeSyncronisation()
	{
		auto const imageAcquiredSemaphoreCreateInfo = vk::SemaphoreCreateInfo()
			.setFlags({});

		auto result = m_Device.createSemaphore(&imageAcquiredSemaphoreCreateInfo, nullptr, &m_ImageAcquiredSemaphore);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create image acquired semaphore.");

		auto const fenceCreateInfo = vk::FenceCreateInfo();
		result = m_Device.createFence(&fenceCreateInfo, nullptr, &m_Fence);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to create fence.");
	}

	const bool Renderer::GetMemoryTypeFromProperties(uint32_t typeBits, vk::MemoryPropertyFlags requirementsMask, uint32_t* typeIndex)
	{
		for (uint32_t i = 0; i < m_PhysicalDeviceMemoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) == 1)
			{
				if ((m_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & requirementsMask) == requirementsMask)
				{
					*typeIndex = i;
					return true;
				}
			}
			typeBits >>= 1;
		}
		return false;
	}

	void Renderer::Resize()
	{
		if (!m_Prepared)
			return;

		m_Device.destroyPipeline(m_Pipeline, nullptr);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
		{
			m_Device.destroyFramebuffer(m_Framebuffers[i], nullptr);
		}
		m_Device.destroyImage(m_DepthBuffer.image, nullptr);
		m_Device.destroyImageView(m_DepthBuffer.view, nullptr);
		m_Device.freeMemory(m_DepthBuffer.memory);
		for (uint32_t i = 0; i < m_SwapchainImageCount; i++)
		{
			m_Device.destroyImageView(m_SwapchainResources[i].view, nullptr);
		}
		m_Device.destroySwapchainKHR(m_Swapchain, nullptr);

		InitalizeSwapchain();
		InitalizeDepthBuffer();
		InitalizeFramebuffers();
		InitalizePipeline();
	}

	void Renderer::BeginScene(Camera& camera)
	{
		if (!m_Prepared)
			return;

		m_CommandBuffer.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		auto const beginInfo = vk::CommandBufferBeginInfo()
			.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse)
			.setPInheritanceInfo(nullptr);

		auto result = m_CommandBuffer.begin(&beginInfo);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to begin recording commands.");

		m_View = camera.GetTransformationMatrix();
		glm::mat4 mvp = m_Model * m_View * m_Projection;
		memcpy(m_MvpBuffer.cpuMemoryPtr, &mvp, sizeof(mvp));

		result = m_Device.acquireNextImageKHR(m_Swapchain, UINT64_MAX, m_ImageAcquiredSemaphore, nullptr, &m_CurrentBuffer);
		if (result == vk::Result::eErrorOutOfDateKHR)
		{
			Resize();
		}
		else if (result == vk::Result::eSuboptimalKHR)
		{
			// Do nothing.
		}
		else if (result == vk::Result::eErrorSurfaceLostKHR)
		{
			m_Instance.destroySurfaceKHR(m_Surface, nullptr);
			InitalizeSurface();
			Resize();
		}
		else
		{
			CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to acquire next image.");
		}

		vk::ClearValue clearValues[] = {
			vk::ClearValue().setColor(vk::ClearColorValue(std::array<float, 4>({ 0.2f, 0.0f, 0.8f, 1.0f }))),
			vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0))
		};
		auto const renderPassBeginInfo = vk::RenderPassBeginInfo()
			.setRenderPass(m_RenderPass)
			.setFramebuffer(m_Framebuffers[m_CurrentBuffer])
			.setRenderArea(vk::Rect2D(vk::Offset2D(0.0f, 0.0f), m_SwapchainExtent))
			.setClearValueCount(sizeof(clearValues) / sizeof(clearValues[0]))
			.setPClearValues(clearValues);

		m_CommandBuffer.beginRenderPass(&renderPassBeginInfo, vk::SubpassContents::eInline);

		m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_Pipeline);
		m_CommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_PipelineLayout, 0,
			m_DescriptorSetCount, m_DescriptorSets.get(), 0, nullptr);

		m_CommandBuffer.setViewport(0, 1, &m_Viewport);
		m_CommandBuffer.setScissor(0, 1, &m_ScissorRect);
	}
	
	void Renderer::EndScene()
	{
		if (!m_Prepared)
			return;

		vk::DeviceSize offsets[] = { 0 };

		memcpy(m_VertexBuffer.cpuMemoryPtr, m_Vertices.data(), m_Vertices.size() * sizeof(Vertex));

		m_CommandBuffer.bindVertexBuffers(0, 1, &m_VertexBuffer.buffer, offsets);
		m_CommandBuffer.bindIndexBuffer(m_IndexBuffer.buffer, 0, m_IndexBuffer.indexType);

		m_CommandBuffer.drawIndexed(m_Statistics.indices, 1, 0, 0, 0);

		m_CommandBuffer.endRenderPass();

		m_CommandBuffer.end();

		const vk::CommandBuffer commandBuffers[] =
		{
			m_CommandBuffer
		};

		vk::PipelineStageFlags pipelineStageFlags = vk::PipelineStageFlagBits::eColorAttachmentOutput;
		auto const submitInfo = vk::SubmitInfo()
			.setWaitSemaphoreCount(1)
			.setPWaitSemaphores(&m_ImageAcquiredSemaphore)
			.setPWaitDstStageMask(&pipelineStageFlags)
			.setCommandBufferCount(sizeof(commandBuffers) / sizeof(commandBuffers[0]))
			.setPCommandBuffers(commandBuffers)
			.setSignalSemaphoreCount(0)
			.setPSignalSemaphores(nullptr);

		auto result = m_GraphicsQueue.submit(1, &submitInfo, m_Fence);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to submit render command buffer to graphics queue.");

		auto const present = vk::PresentInfoKHR()
			.setSwapchainCount(1)
			.setPSwapchains(&m_Swapchain)
			.setPImageIndices(&m_CurrentBuffer)
			.setPResults(nullptr)
			.setWaitSemaphoreCount(0)
			.setPWaitSemaphores(nullptr);

		do {
			result = m_Device.waitForFences(1, &m_Fence, VK_TRUE, 10000000000);
		} while (result == vk::Result::eTimeout);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to wait for fences.");
		result = m_PresentQueue.presentKHR(&present);
		CEE_ASSERT_WITH_MESSAGE((uint32_t)result >= 0, "Failed to present.");
		result = m_Device.resetFences(1, &m_Fence);
		CEE_ASSERT_WITH_MESSAGE(result == vk::Result::eSuccess, "Failed to reset fence.");

		memset(&m_Statistics, 0, sizeof(RendererStatistics));
		m_Vertices.clear();
	}

	void Renderer::DrawQuad(glm::vec2 translation, glm::vec2 scale, float rotationAngle)
	{
		glm::mat4 transformation = glm::scale(glm::identity<glm::mat4>(), glm::vec3(scale, 1.0f));
		transformation = glm::rotate(transformation, rotationAngle, glm::vec3(0.0f, 0.0f, 1.0f));
		transformation = glm::translate(transformation, glm::vec3(translation, 0.0f));
		for (uint32_t i = 0; i < 4; i++)
		{
			Vertex vertex;
			vertex.position = g_QuadVertices[i].position * transformation;
			vertex.color = g_QuadVertices[i].color;
			vertex.normal = g_QuadVertices[i].position;
			m_Vertices.push_back(vertex);
		}
		m_Statistics.verices += 4;
		m_Statistics.indices += 6;
		m_Statistics.quads++;
	}
}
