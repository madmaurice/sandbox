#include "topicApp.h"
#include "mathHelper.h"


TopicApp::TopicApp(HINSTANCE hInstance)
: DXApp(hInstance) 
, m_cam(0.0f, 2.0f, -15.0f)
{
    m_windowCaption = "Topic Demo";
	
	m_lastMousePos.x = 0;
	m_lastMousePos.y = 0;
}

TopicApp::~TopicApp()
{
}

bool TopicApp::Init()
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

void TopicApp::UpdateScene(float dt)
{
    Sleep(16);

  	// Control the camera.
	if( GetAsyncKeyState('W') & 0x8000 )
		m_cam.walk(10.0f*dt);

	if( GetAsyncKeyState('S') & 0x8000 )
		m_cam.walk(-10.0f*dt);

	if( GetAsyncKeyState('A') & 0x8000 )
		m_cam.strafe(-10.0f*dt);

	if( GetAsyncKeyState('D') & 0x8000 )
		m_cam.strafe(10.0f*dt);
    
    m_cam.updateViewMatrix();
}

void TopicApp::OnResize()
{
	DXApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    m_cam.setLens(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
}

void TopicApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    m_lastMousePos.x = x;
    m_lastMousePos.y = y;

    SetCapture(m_hMainWnd);
}

void TopicApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    ReleaseCapture();
}

void TopicApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    //Rotate if left mouse button is pressed
    if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_lastMousePos.y));

		m_cam.pitch(dy); //Rotate on right vector (x axis)
		m_cam.rotateY(dx);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}