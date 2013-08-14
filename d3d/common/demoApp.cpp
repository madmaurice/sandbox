#include "demoApp.h"
#include "mathHelper.h"
#include <sstream>

DemoApp::DemoApp(HINSTANCE hInstance)
: DXApp(hInstance) 
, m_camPosition(0.0f, 0.0f, 0.0f)
, m_camTarget(0.0f, 0.0f, 0.0f)
, m_theta(1.5f*MathHelper::Pi)
, m_phi(0.25f*MathHelper::Pi)
, m_radius(5.0f)
{
    m_windowCaption = "Box Demo";
	
	m_lastMousePos.x = 0;
	m_lastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

DemoApp::~DemoApp()
{
}

bool DemoApp::Init()
{
	if(!DXApp::Init())
		return false;

    // Better to init d3d stuff at the beginning (buffer...)
    // Device call are expensive, specially for creating stuff
	InitGeometryBuffers();
	InitFX();
	InitVertexLayout();

	return true;
}

void DemoApp::DrawScene() { }

void DemoApp::OnResize()
{
	DXApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

void DemoApp::UpdateScene(float dt)
{
    Sleep(16);

	// Build the view matrix.
    XMVECTOR pos    = MathHelper::ConvertSphericalToCartesian(m_radius, m_theta, m_phi);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMStoreFloat3(&m_camPosition, pos);
    XMStoreFloat3(&m_camTarget, target);

	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);
}

void DemoApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

    SetCapture(m_hMainWnd);
}

void DemoApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void DemoApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_lastMousePos.y));

		// Update angles based on input to orbit camera around box.
		m_theta += dx;
		m_phi   += dy;

		// Restrict the angle m_phi.
		m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi-0.1f);
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.2f*static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.2f*static_cast<float>(y - m_lastMousePos.y);

		// Update the camera radius based on input.
		m_radius += dx - dy;

		// Restrict the radius.
		m_radius = MathHelper::Clamp(m_radius, 3.0f, 500.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}