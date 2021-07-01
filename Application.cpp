#include "pch.h"
#include "Application.hpp"
#include <iostream>

namespace CEE
{
#if defined(CEE_OS_WINDOWS)
	HINSTANCE Application::s_Connection = NULL;
#elif defined(CEE_WM_XCB)
	xcb_connection_t* Application::s_Connection = nullptr;
#endif	
	
	Application* Application::s_Instance = nullptr;
	
	CEE::Application::Application(int arg, char** argv)
	{
		if (s_Instance != nullptr)
		{
			fprintf(stderr, "Only one instance of CEE::Application class allowed.\n");
		}
		else s_Instance = this;

#if defined(CEE_OS_WINDOWS)
		s_Connection = GetModuleHandle(NULL);
#elif defined(CEE_WM_XCB)
		s_Connection = xcb_connect(nullptr, nullptr);
		if (xcb_connection_has_error(s_Connection))
		{
			fprintf(stderr, "XCB connection has error.\n");
		}
#endif

#if defined(CEE_OS_WINDOWS)
		m_Window = new Window((void*)(&s_Connection), 1280, 720, "Vulkan App");
		m_Renderer = new Renderer(s_Connection, m_Window, RendererCapabilities(9996));
#elif defined(CEE_WM_XCB)
		m_Window = new Window((void*)s_Connection, 1280, 720, "Vulkan App");
		m_Renderer = new Renderer(s_Connection, m_Window, RendererCapabilities(9996));
#endif
		
		m_Window->SetDestroyWindowCallback([this](Window* window){
				if (m_Window == window) m_Running = false;
		});
	}
	
	Application::~Application()
	{
		delete m_Window;
#if defined(CEE_WM_XCB)
		xcb_disconnect(s_Connection);
#endif
		s_Instance = nullptr;
	}
	
	int Application::Run()
	{
		m_Running = true;
		while (m_Running)
		{
			m_Renderer->BeginScene(m_Camera);
			m_Renderer->DrawQuad({ 0.0f, 0.0f }, { 0.5f, 0.5f }, 0.0f);
			m_Renderer->EndScene();
			m_Window->PollEvents();
		}
		return 0;
	}
}

