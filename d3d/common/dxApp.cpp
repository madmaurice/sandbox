#include "dxApp.h"
#include "dxUtil.h"
#include "config.h"
#include <WindowsX.h>
#include <sstream>

namespace
{
	// Used to forward Windows messages from a global window
	// procedure to our member function window procedure because we cannot
	// assign a member function to WNDCLASS::lpfnWndProc.
	DXApp* gd3dApp = 0;
}

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return gd3dApp->MsgProc(hwnd, msg, wParam, lParam);
}

DXApp::DXApp(HINSTANCE hInstance)
:	m_hAppInst(hInstance),
    m_windowCaption("DX11 Application"),
    m_dxDriverType(D3D_DRIVER_TYPE_HARDWARE),
    m_windowWidth(800),
    m_windowHeight(600),
    m_enable4xMsaa(false),
    m_hMainWnd(0),
    m_appPaused(false),
    m_minimized(false),
    m_maximized(false),
    m_resizing(false),
    m_4xMsaaQuality(0),
 
    m_dxDevice(nullptr),
    m_dxImmediateContext(nullptr),
	m_swapChain(nullptr),
    m_depthStencilBuffer(nullptr),
    m_renderTargetView(nullptr),
    m_depthStencilView(nullptr)
{
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));

	// Get a pointer to the application object so we can forward 
	// Windows messages to the object's window procedure through
	// the global window procedure.
	gd3dApp = this;
}

DXApp::~DXApp()
{
    if( m_dxImmediateContext.Get() )
    {
        //This method resets any device context to the default settings. 
        //This sets all input/output resource slots, shaders, input layouts, predications, scissor rectangles, 
        //depth-stencil state, rasterizer state, blend state, sampler state, and viewports to NULL. 
        //The primitive topology is set to UNDEFINED.
		m_dxImmediateContext->ClearState();
    }
}

HINSTANCE DXApp::GetAppInstance() const
{
    return m_hAppInst;
}

HWND DXApp::GetWindowHandle() const
{
	return m_hMainWnd;
}

float DXApp::GetAspectRatio() const
{
    return static_cast<float>(m_windowWidth) / m_windowHeight;
}

int DXApp::Run()
{
	MSG msg = {0};
 
	m_timer.Reset();

	while(msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if(PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
		{
            TranslateMessage( &msg );
            DispatchMessage( &msg );
		}
		// Otherwise, do animation/game stuff.
		else
        {	
			m_timer.Tick();

			if( !m_appPaused)
			{
				CalculateFrameStats();
				UpdateScene(m_timer.DeltaTime());	
				DrawScene();
			}
			else
			{
				Sleep(100);
			}
        }
    }

	return (int)msg.wParam;
}

bool DXApp::Init()
{
	if(!InitMainWindow())
		return false;

	if(!InitDirect3D())
		return false;

	return true;
}

bool DXApp::InitMainWindow()
{
	WNDCLASS wc;
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = MainWndProc; 
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = m_hAppInst;
	wc.hIcon         = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor       = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName  = 0;
	wc.lpszClassName = "DXWndClassName"; // Name of the class (for registration)

    // Registers a window class for subsequent use in calls to the CreateWindow or CreateWindowEx function
	if( !RegisterClass(&wc) )
	{
		MessageBox(0, "RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = { 0, 0, m_windowWidth, m_windowHeight };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width  = R.right - R.left;
	int height = R.bottom - R.top;

    m_hMainWnd = CreateWindow("DXWndClassName", m_windowCaption.c_str(), 
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, m_hAppInst, 0); 
	if( !m_hMainWnd )
	{
		MessageBox(0, "CreateWindow Failed.", 0, 0);
		return false;
	}

    ShowWindow(m_hMainWnd, SW_SHOW);

    // The UpdateWindow function updates the client area of the specified window by sending a 
    // WM_PAINT message to the window if the window's update region is not empty. 
	UpdateWindow(m_hMainWnd);

	return true;
}

bool DXApp::InitDirect3D()
{
	// Create the device and device context.
	UINT createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL featureLevel;

	HRESULT hr = D3D11CreateDevice(
			0,                 // default adapter
            m_dxDriverType,
			0,                 // no software device
			createDeviceFlags, 
			0,                 // default feature level array (select greatest supported -> DX11)
            0,                 // number of feature level in the array above 
			D3D11_SDK_VERSION, //SDK version
            m_dxDevice.GetAddressOf(),  //Created device
			&featureLevel,              //Selected feature level
            m_dxImmediateContext.GetAddressOf());  //Device context

    //NOTE: To create a deffered context call CreateDefferredContext
    //Used for multi-threading, basically the main thread has an immediate context and
    //the worker thread have deferred context

	if( FAILED(hr) )
	{
		MessageBox(0, "D3D11CreateDevice Failed.", 0, 0);
		return false;
	}

	if( featureLevel != D3D_FEATURE_LEVEL_11_0 )
	{
		MessageBox(0, "Direct3D Feature Level 11 unsupported.", 0, 0);
		return false;
	}

	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.
    HR(m_dxDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m_4xMsaaQuality));
    OC_ASSERT( m_4xMsaaQuality > 0 );

	// Fill out a DXGI_SWAP_CHAIN_DESC to describe our swap chain.

	DXGI_SWAP_CHAIN_DESC sd;
    //Describe the back buffer
    sd.BufferDesc.Width  = m_windowWidth;
    sd.BufferDesc.Height = m_windowHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //Texture format of the back buffer ( 8 bits rgb + alpha)

    // ScanlineOrdering : DXGI_MODE_SCANLINE_ORDER
    // This indicate the method the raster uses to create an image on a surface.
    //DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED  
    //DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE 
    //DXGI_MODE_SCANLINE_ORDER_UPPER_FIELD_FIRST 
    //DXGI_MODE_SCANLINE_ORDER_LOWER_FIELD_FIRST
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

    // Scaling : DXGI_MODE_SCALING 
    // Flags indicating how an image is stretched to fit a given monitor's resolution.
    //DXGI_MODE_SCALING_UNSPECIFIED
    //DXGI_MODE_SCALING_CENTERED  = no scaling 
    //DXGI_MODE_SCALING_STRETCHED
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; 

    //Multisampling (anti-aliasing), all DX11 device support at least 4X multisampling for all render target format
	// Use 4X MSAA? 
	if( m_enable4xMsaa )
	{
		sd.SampleDesc.Count   = 4;
		sd.SampleDesc.Quality = m_4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		sd.SampleDesc.Count   = 1;
		sd.SampleDesc.Quality = 0;
	}

    // Describes the surface usage and CPU access options for the back buffer. 
    // The back buffer can be used for shader input or render-target output.
	sd.BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount  = 1;
	sd.OutputWindow = m_hMainWnd;
	sd.Windowed     = true;
    //Options for handling pixels in a display surface after calling IDXGISwapChain1::Present1.
	sd.SwapEffect   = DXGI_SWAP_EFFECT_DISCARD;
    //Options for swap-chain behavior.
	sd.Flags        = 0;

	// To correctly create the swap chain, we must use the IDXGIFactory that was
	// used to create the device.  If we tried to use a different IDXGIFactory instance
	// (by calling CreateDXGIFactory), we get an error: "IDXGIFactory::CreateSwapChain: 
	// This function is being called with a device from a different IDXGIFactory."
	ComPtr<IDXGIDevice> dxgiDevice = nullptr;
    HR(m_dxDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgiDevice.GetAddressOf()));
	      
	ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
    HR(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)dxgiAdapter.GetAddressOf()));

	ComPtr<IDXGIFactory> dxgiFactory = nullptr;
    HR(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgiFactory.GetAddressOf()));

    HR(dxgiFactory->CreateSwapChain(m_dxDevice.Get(), &sd, m_swapChain.GetAddressOf()));
	
	// The remaining steps that need to be carried out for d3d creation
	// also need to be executed every time the window is resized.  So
	// just call the OnResize method here to avoid code duplication.
	OnResize();

	return true;
}

void DXApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if( (m_timer.TotalTime() - timeElapsed) >= 1.0f )
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::ostringstream outs;   
		outs.precision(6);
        outs << m_windowCaption << "    "
			 << "FPS: " << fps << "    " 
			 << "Frame Time: " << mspf << " (ms)";
        SetWindowText(m_hMainWnd, outs.str().c_str());
		
		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void DXApp::OnResize()
{
    OC_ASSERT(m_dxImmediateContext.Get());
    OC_ASSERT(m_dxDevice.Get());
	OC_ASSERT(m_swapChain.Get());

	// Release the old views, as they hold references to the buffers we
	// will be destroying.  Also release the old depth/stencil buffer.
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_depthStencilBuffer.Reset();

	// Resize the swap chain and recreate the render target view.
    // NOTE: If we do not resize the buffers the display will be stretched to the window size
    HR(m_swapChain->ResizeBuffers(1, m_windowWidth, m_windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM, 0));
	ComPtr<ID3D11Texture2D> backBuffer;
    HR(m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf())));
    HR(m_dxDevice->CreateRenderTargetView(backBuffer.Get(), 0, m_renderTargetView.GetAddressOf()));

	// Create the depth/stencil buffer and view.
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	
    depthStencilDesc.Width     = m_windowWidth;
    depthStencilDesc.Height    = m_windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format    = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Use 4X MSAA? --must match swap chain MSAA values.
	if( m_enable4xMsaa )
	{
		depthStencilDesc.SampleDesc.Count   = 4;
		depthStencilDesc.SampleDesc.Quality = m_4xMsaaQuality-1;
	}
	// No MSAA
	else
	{
		depthStencilDesc.SampleDesc.Count   = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}

	depthStencilDesc.Usage          = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0; 
	depthStencilDesc.MiscFlags      = 0;

    HR(m_dxDevice->CreateTexture2D(&depthStencilDesc, 0, m_depthStencilBuffer.GetAddressOf()));
    HR(m_dxDevice->CreateDepthStencilView(m_depthStencilBuffer.Get(), 0, m_depthStencilView.GetAddressOf()));

	// Bind the render target view and depth/stencil view to the pipeline.
    m_dxImmediateContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Set the viewport transform.
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
    m_viewport.Width    = static_cast<float>(m_windowWidth);
    m_viewport.Height   = static_cast<float>(m_windowHeight);
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;

	m_dxImmediateContext->RSSetViewports(1, &m_viewport);
}

LRESULT DXApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch( msg )
	{
	// WM_ACTIVATE is sent when the window is activated or deactivated.  
	// We pause the game when the window is deactivated and unpause it 
	// when it becomes active.  
	case WM_ACTIVATE:
		if( LOWORD(wParam) == WA_INACTIVE )
		{
			m_appPaused = true;
			m_timer.Stop();
		}
		else
		{
			m_appPaused = false;
			m_timer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
        m_windowWidth  = LOWORD(lParam);
        m_windowHeight = HIWORD(lParam);
        if(m_dxDevice.Get())
		{
			if( wParam == SIZE_MINIMIZED )
			{
				m_appPaused = true;
                m_minimized = true;
                m_maximized = false;
			}
			else if( wParam == SIZE_MAXIMIZED )
			{
				m_appPaused = false;
				m_minimized = false;
				m_maximized = true;
				OnResize();
			}
			else if( wParam == SIZE_RESTORED )
			{
				// Restoring from minimized state?
				if( m_minimized )
				{
					m_appPaused = false;
					m_minimized = false;
					OnResize();
				}

				// Restoring from maximized state?
                else if( m_maximized )
				{
					m_appPaused = false;
                    m_maximized = false;
					OnResize();
				}
                else if( m_resizing )
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		m_appPaused = true;
        m_resizing  = true;
		m_timer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		m_appPaused = false;
        m_resizing  = false;
		m_timer.Start();
		OnResize();
		return 0;
 
	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses 
	// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
        // Don't beep when we alt-enter.
        return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200; 
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}



