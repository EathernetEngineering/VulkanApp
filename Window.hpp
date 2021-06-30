#ifndef _WINDOW_HPP
#define _WINDOW_HPP

#include <functional>

#include "Event.hpp"

#if defined(CEE_OS_WINDOWS)
#include <Windows.h>
#elif defined(CEE_WM_XCB)
#include <xcb/xcb.h>
#endif

namespace CEE {
	class Window {
	public:
		Window(void* connection, uint32_t width = 1280u, uint32_t height = 720u, const std::string& title = "Window");
		~Window();
		
		void PollEvents();

		inline uint32_t GetWidth()  const { return m_Width;  }
		inline uint32_t GetHeight() const { return m_Height; }
		
		/* EVENT SYSTEM */
		
	public:
		
		inline void SetKeyCallback(std::function<void(Window*, int, int, int, int)> callback) { m_KeyCallback = callback; }
		inline void SetCharCallback(std::function<void(Window*, uint32_t)> callback) { m_CharCallback = callback; }
		inline void setCursorPositionCallback(std::function<void(Window*, double, double)> callback) { m_CursorPositionCallback = callback; }
		inline void SetCursorEnterCallback(std::function<void(Window*, int)> callback) { m_CursorEnterCallback = callback; }
		inline void SetMouseButtonCallback(std::function<void(Window*, int, int, int)> callback) { m_MouseButtonCallback = callback; }
		inline void SetScrollCallback(std::function<void(Window*, double, double)> callback) { m_ScrollCallback = callback; }
		inline void SetDestroyWindowCallback(std::function<void(Window*)> callback) { m_DestroyWindowCallback = callback; }
		
		enum KeyAction {
			KEY_ACTION_UNSPECIFIED = 0,
			KEY_ACTION_PRESSED = 1 << 0,
			KEY_ACTION_RELEASED = 1 << 1,
			KEY_ACTION_REPEAT = 1 << 2
		};
		
	private:
		std::function<void(Window*, int, int, int, int)> m_KeyCallback;
		std::function<void(Window*, uint32_t)> m_CharCallback;
		std::function<void(Window*, double, double)> m_CursorPositionCallback;
		std::function<void(Window*, int)> m_CursorEnterCallback;
		std::function<void(Window*, int, int, int)> m_MouseButtonCallback;
		std::function<void(Window*, double, double)> m_ScrollCallback;
		std::function<void(Window*)> m_DestroyWindowCallback;
		
		static const int TranslateKey(uint32_t key);
		static const int TranslateState(uint32_t mods);
		
		/* END EVENT SYSTEM */
		
	private:
		uint32_t m_Width, m_Height;
		std::string m_Title;
		
	private:
#if defined(CEE_OS_WINDOWS)
		HWND m_Window;
	public:
		inline HWND GetNativeWindowPtr() { return m_Window; }
	private:
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
#elif defined(CEE_WM_XCB)
		xcb_screen_t* m_Screen;
		xcb_window_t m_Window;
	public:
		inline xcb_window_t GetNativeWindowPtr() { return m_Window; }
	private:
		xcb_atom_t m_WindowCloseAtom;
#endif
	
	private:
#if defined(CEE_OS_WINDOWS)
		static HINSTANCE s_Connection;
#elif defined(CEE_WM_XCB)
		static xcb_connection_t* s_Connection;
#endif
	};
}

#endif
