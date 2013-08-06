//---------------------------------------------------------------------------------------
//
// Basic app for various demo
//
//---------------------------------------------------------------------------------------

#ifndef _INCGUARD_DEMOAPP_H
#define _INCGUARD_DEMOAPP_H

#include "dxApp.h"

class DemoApp : public DXApp
{
public:
	explicit DemoApp(HINSTANCE hInstance);
	~DemoApp();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

    virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
    XMFLOAT4X4 m_view;
	XMFLOAT4X4 m_proj;

    XMFLOAT3 m_camPosition;
    XMFLOAT3 m_camTarget;

    float m_theta;
	float m_phi;
	float m_radius;

private:

    virtual void InitGeometryBuffers() =0;
	virtual void InitFX() =0;
	virtual void InitVertexLayout() =0;

	POINT m_lastMousePos;
};

#endif