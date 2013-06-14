//---------------------------------------------------------------------------------------
//
// Simple DirectX application class.  
//
//---------------------------------------------------------------------------------------

#ifndef _INCGUARD_DXAPP_H
#define _INCGUARD_DXAPP_H

#include "dxUtil.h"
#include "timer.h"
#include <string>
#include <comPtr.h>

class DXApp
{
public:
	DXApp(HINSTANCE hInstance);
	virtual ~DXApp();
	
	HINSTANCE GetAppInstance() const;
	HWND      GetWindowHandle() const;
	float     GetAspectRatio() const;
	
	int Run();
 
	virtual bool Init();
	virtual void OnResize(); 
	virtual void UpdateScene(float dt) = 0;
	virtual void DrawScene() = 0; 
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y){ }
	virtual void OnMouseUp(WPARAM btnState, int x, int y)  { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y){ }

protected:
	bool InitMainWindow();
	bool InitDirect3D();

	void CalculateFrameStats();

private:
    void Pause();
    void Resume();

protected:

    // Windows members
	HINSTANCE m_hAppInst;
	HWND      m_hMainWnd;
	bool      m_appPaused;
	bool      m_minimized;
	bool      m_maximized;
	bool      m_resizing;

	Timer m_timer;

    //DirectX members
	UINT      m_4xMsaaQuality;
	ComPtr<ID3D11Device> m_dxDevice;
	ComPtr<ID3D11DeviceContext> m_dxImmediateContext;
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D11Texture2D> m_depthStencilBuffer;
	ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;
	D3D11_VIEWPORT m_viewport;

	// Derived class should set these in derived constructor to customize starting values.
	std::string m_windowCaption;
	D3D_DRIVER_TYPE m_dxDriverType;
	int m_windowWidth;
	int m_windowHeight;
	bool m_enable4xMsaa;
};

#endif // INCGUARD
