#define DLLLIB
#include "rwgui.h"
#include <iostream>
#include <windowsx.h>
#include <string>
#include <d2d1.h>

#pragma warning(disable : 4244 4311 4312 4838 4244 4251)

int Application::Run(HINSTANCE hInstance)
{
	appInstance = hInstance;
	WNDCLASSEX windowClass;
	ZeroMemory(&windowClass, sizeof(WNDCLASSEX));
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
	windowClass.hInstance = hInstance;
	windowClass.lpfnWndProc = (WNDPROC)OnWindowProcStatic;
	windowClass.lpszClassName = GetWindowClassName();
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClassEx(&windowClass);

	GUIResources::Get();

	Bounds bounds = GetDefaultWindowBounds();
	RECT rect = { bounds.Pos.x, bounds.Pos.y, bounds.Size.x, bounds.Size.y};
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, false);
	WindowHandler = CreateWindow(GetWindowClassName(), GetApplicationTitle(), WS_POPUP, bounds.Pos.x, bounds.Pos.y, rect.right-rect.left, rect.bottom-rect.top, 0, 0, hInstance, 0);
	if (!WindowHandler) return 1;
	LONG lStyle = GetWindowLong(WindowHandler, GWL_STYLE);
	lStyle &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
	SetWindowLong(WindowHandler, GWL_STYLE, lStyle);
	LONG lExStyle = GetWindowLong(WindowHandler, GWL_EXSTYLE);
	lExStyle &= ~(WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE);
	SetWindowLong(WindowHandler, GWL_EXSTYLE, lExStyle);
	ShowWindow(WindowHandler, SW_SHOW);
	int result = SetWindowLongPtr(WindowHandler, GWLP_USERDATA, (LONG_PTR)this);

	Renderer = new RWD2D();
	if (!Renderer || !Renderer->Init(this)) return 1;

	BuildPages(Pages);
	for (ApplicationPage* page : Pages)
	{
		page->SetApp(this);
	}
	OnInit();
	


	MSG msg;
	while (!bShouldExit)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			if (!Update()) return 1;
		}
		
	}
	return 0;
}


void Application::Stop()
{
	OnStop();
	bShouldExit = true;
}

LRESULT Application::OnWindowProcStatic(HWND pWindowHandler, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
	Application* target = (Application*)GetWindowLongPtr(pWindowHandler, GWLP_USERDATA);
	if (target == nullptr) return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	return target->OnWindowProc(pWindowHandler,uMsg,wparam,lparam);
}

LRESULT Application::OnWindowProc(HWND pWindowHandler, UINT uMsg, WPARAM wparam, LPARAM lparam)
{
	Bounds bounds = GetDefaultWindowBounds();
	static ID2D1SolidColorBrush* solidBrush = nullptr;
	Drawable* drawable = nullptr;
	Vector2D pt(0,0);
	RECT rc;
	switch (uMsg)
	{
	case WM_KEYDOWN:
		OnKeyPressed(wparam & 0xFF);
		return DefWindowProc(pWindowHandler,uMsg,wparam,lparam);
	case WM_LBUTTONDOWN:
		OnKeyPressed(VK_LBUTTON);
		return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	case WM_LBUTTONUP:
		OnKeyReleased(VK_LBUTTON);
		return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	case WM_RBUTTONDOWN:
		OnKeyPressed(VK_RBUTTON);
		return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	case WM_RBUTTONUP:
		OnKeyReleased(VK_RBUTTON);
		return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	case WM_KEYUP:
		OnKeyReleased(wparam & 0xFF);
		return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
	case WM_NCHITTEST:
		pt = Vector2D(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
		GetWindowRect(this->GetWindowHandler(), &rc);
		pt.x = pt.x - rc.left;
		pt.y = pt.y - rc.top;
		/* Debug */
		/*
		Renderer->GetRenderTarget()->BeginDraw();
		if (solidBrush == nullptr)
		{
			Renderer->GetRenderTarget()->CreateSolidColorBrush(Color(1.0f, 0, 0).ToD2D1ColorF(), &solidBrush);
		}
		Renderer->GetRenderTarget()->FillRectangle(Bounds(pt, Vector2D(32, 32)).ToD2DRect(), solidBrush);
		Renderer->GetRenderTarget()->EndDraw();
		*/
		drawable = GetDrawableAtPosition(pt);
		currentHoveredDrawable = drawable;
		if (drawable) return drawable->GetDrawableNCObjectType();
		return HTCLIENT;
	case WM_SIZE:
		if (Renderer) Renderer->Resize();
		return 0;
	case WM_ERASEBKGND:
		return 0;
	case WM_PAINT:
		if (Renderer) Renderer->Update();
		return 0;
	case WM_DESTROY:
		Stop();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(pWindowHandler, uMsg, wparam, lparam);
}

void Application::OnDraw(RWD2D * d2d, ID2D1HwndRenderTarget * renderTarget)
{
	if (activePage < Pages.size()) Pages[activePage]->Draw(d2d, renderTarget);
}


void Application::OnKeyPressed(char key)
{
	Input[key] = true;
	if (key == VK_LBUTTON)
	{
		Drawable* currentHovered = GetCurrentHoveredDrawable();
		if (currentHovered != nullptr)
		{
			currentHovered->OnMousePress();
			currentPressedDrawable = currentHovered;
		}
	}
}

void Application::OnKeyReleased(char key)
{
	Input[key] = false;
	if (key == VK_LBUTTON)
	{
		if (currentPressedDrawable != nullptr && currentPressedDrawable == GetCurrentHoveredDrawable())
		{
			currentPressedDrawable->OnMouseClick();
			currentPressedDrawable = nullptr;
		}
		if (currentHoveredDrawable != nullptr) currentHoveredDrawable->OnMousePress();
	}
}

Drawable * Application::GetDrawableAtPosition(Vector2D Position)
{
	if (GetActivePage() == nullptr) return nullptr;
	return GetActivePage()->GetDrawableAtPosition(Position);
}

void Application::SetActivePage(int pageId)
{
	if (pageId < Pages.size()) activePage = pageId;
}

int Application::GetActivePageID()
{
	return activePage;
}

ApplicationPage * Application::GetActivePage()
{
	if (activePage < Pages.size()) return Pages[activePage];
	return nullptr;
}


void ApplicationPage::SetApp(Application* nApp)
{
	this->App = nApp;
}

char * Application::GetApplicationTitle()
{
	return GetApplicationName();
}



Bounds Application::GetDefaultWindowBounds()
{
	return Bounds(200,200,800,600);
}

HWND Application::GetWindowHandler()
{
	return this->WindowHandler;
}

char * Application::GetWindowClassName()
{
	return GetApplicationName();
}

Bounds Application::GetCurrentWindowBounds()
{
	ID2D1HwndRenderTarget* rt = Renderer->GetRenderTarget();
	if (rt == nullptr) return Bounds(0, 0, 0, 0);
	return Bounds(0,0,rt->GetSize().width,rt->GetSize().height);
}

GUIResources::GUIResources()
{
	CURSOR_ARROW = LoadCursor(NULL, IDC_ARROW);
	CURSOR_WAIT = LoadCursor(NULL, IDC_WAIT);
}