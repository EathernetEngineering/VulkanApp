#ifndef _APPLICATION_HPP
#define _APPLICATION_HPP

#include "Window.hpp"
#include "Renderer.hpp"
#include "Camera.hpp"

namespace CEE {
	class Application
	{
	public:
		Application(int arg, char** argv);
		~Application();
		
	public:
		int Run();
		
	private:
#if defined(CEE_OS_WINDOWS)
		static HINSTANCE s_Connection;
#elif defined(CEE_WM_XCB)
		static xcb_connection_t* s_Connection;
#endif
		
	private:
		Window* m_Window = nullptr;
		bool m_Running = false;
		
		Renderer* m_Renderer = nullptr;

		Camera m_Camera;
		
	private:
		static Application* s_Instance;
	};
}

#endif
