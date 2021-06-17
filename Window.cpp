#include "pch.h"
#include "Window.hpp"

namespace CEE {

#if defined(CEE_OS_WINDOWS)
	HINSTANCE Window::s_Connection = NULL;
#elif defined(CEE_WM_XCB)
	xcb_connection_t* Window::s_Connection = nullptr;
#endif	

	Window::Window(void* connection, uint32_t width, uint32_t height, const std::string& title)
		: m_Width(width), m_Height(height), m_Title(title)
	{
#if defined(CEE_OS_WINDOWS)
		s_Connection = *(HINSTANCE*)(connection);
		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.hCursor = LoadCursor(s_Connection, IDC_ARROW);
		wc.hInstance = s_Connection;
		wc.hIcon = LoadIcon(s_Connection, IDI_APPLICATION);
		wc.lpszClassName = "VulkanAppClass";
		wc.lpszMenuName = NULL;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = NULL;
		wc.cbWndExtra = NULL;
		if (FAILED(RegisterClassEx(&wc)))
		{
			fprintf(stderr, "Failed to register class.\n");
		}

		RECT wr = { 0, 0, static_cast<LONG>(m_Width), static_cast<LONG>(m_Height) };
		AdjustWindowRectEx(&wr, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);

		m_Window = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, wc.lpszClassName, m_Title.c_str(),
								  WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
								  wr.right - wr.left, wr.bottom - wr.top,
								  NULL, NULL, s_Connection, this);
		if (!m_Window)
		{
			fprintf(stderr, "Failed to create window.\n\tError code: %u\n", (uint32_t)(uint16_t)GetLastError());
		}
		ShowWindow(m_Window, SW_NORMAL);
#elif defined(CEE_WM_XCB)
		s_Connection = (xcb_connection_t*)(connection);
		m_Screen = xcb_setup_roots_iterator(xcb_get_setup(s_Connection)).data;
		m_Window = xcb_generate_id(s_Connection);
		xcb_create_window(s_Connection, XCB_COPY_FROM_PARENT, m_Window, m_Screen->root,
						  0, 0, m_Width, m_Height, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT,
						  m_Screen->root_visual, XCB_CW_BACK_PIXEL, &m_Screen->white_pixel);
		xcb_intern_atom_cookie_t windowProtocolsAtomCookie = xcb_intern_atom(s_Connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_reply_t* windowProtocolsAtomReply = xcb_intern_atom_reply(s_Connection, windowProtocolsAtomCookie, 0);
		xcb_intern_atom_cookie_t windowCloseAtomCookie = xcb_intern_atom(s_Connection, 0, 16, "WM_DELETE_WINDOW");
		xcb_intern_atom_reply_t* windowCloseAtomReply = xcb_intern_atom_reply(s_Connection, windowCloseAtomCookie, 0);
		
		xcb_change_property(s_Connection, XCB_PROP_MODE_REPLACE, m_Window, windowProtocolsAtomReply->atom, 4, 32, 1, &windowCloseAtomReply->atom);
		m_WindowCloseAtom = windowCloseAtomReply->atom;
		
		xcb_map_window(s_Connection, m_Window);
		xcb_flush(s_Connection);
#endif
	}
	
	Window::~Window()
	{
#if defined(CEE_WM_XCB)
		xcb_destroy_window(s_Connection, m_Window);
#endif
	}
	
	void Window::PollEvents()
	{
#if defined(CEE_WM_XCB)
		xcb_generic_event_t* xcbEvent = xcb_poll_for_event(s_Connection);
		while (xcbEvent)
		{
			switch (xcbEvent->response_type & ~0x80)
			{
				case XCB_KEY_PRESS:
					if(m_KeyCallback) {
						xcb_key_press_event_t* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(xcbEvent);
						int key = TranslateKey(keyEvent->detail);
						int mods = TranslateState(keyEvent->state);
						m_KeyCallback(this, keyEvent->detail, key, KEY_ACTION_PRESSED, mods);
					}
					break;
					
				case XCB_KEY_RELEASE:
					if(m_KeyCallback) {
						xcb_key_press_event_t* keyEvent = reinterpret_cast<xcb_key_press_event_t*>(xcbEvent);
						int key = TranslateKey(keyEvent->detail);
						int mods = TranslateState(keyEvent->state);
						m_KeyCallback(this, keyEvent->detail, key, KEY_ACTION_RELEASED, mods);
					}
					break;
					
				case XCB_CLIENT_MESSAGE:
					{
						if (((xcb_client_message_event_t*)xcbEvent)->data.data32[0] == m_WindowCloseAtom)
						{
							if (m_DestroyWindowCallback)
							{
								m_DestroyWindowCallback(this);
							}
						}
					}
			}
			free(xcbEvent);
			xcbEvent = xcb_poll_for_event(s_Connection);
		}
#elif defined(CEE_OS_WINDOWS)
		MSG msg;
		while (PeekMessage(&msg, m_Window, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
#endif
	}
	
	const int Window::TranslateKey(uint32_t key)
	{
		return key;
	}
	
	const int Window::TranslateState(uint32_t state)
	{
		return state;
	}

#if defined(CEE_OS_WINDOWS)
	LRESULT Window::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		Window* pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		switch (message)
		{
			case WM_KEYDOWN:
				{
					if (pWindow->m_KeyCallback)
					{
						int key = TranslateKey(wParam);
						int mods = TranslateState(0);
						pWindow->m_KeyCallback(pWindow, (int)(uint8_t)(lParam >> 16), key, KEY_ACTION_PRESSED, mods);
					}
				}
				return 0;

			case WM_KEYUP:
				{
					if (pWindow->m_KeyCallback)
					{
						int key = TranslateKey(wParam);
						int mods = TranslateState(0);
						pWindow->m_KeyCallback(pWindow, (int)(uint8_t)(lParam >> 16), key, KEY_ACTION_RELEASED, mods);
					}
				}
				return 0;

			case WM_CREATE:
				{
					LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
					SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
				}
				return 0;

			case WM_DESTROY:
				{
					if (pWindow->m_DestroyWindowCallback) pWindow->m_DestroyWindowCallback(pWindow);
					PostQuitMessage(0);
				}
				return 0;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
#endif
}
