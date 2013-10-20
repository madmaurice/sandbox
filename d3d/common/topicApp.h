//---------------------------------------------------------------------------------------
//
// More advanced app for various demo (include 1st person camera)
//
//---------------------------------------------------------------------------------------

#ifndef _INCGUARD_TOPICAPP_H
#define _INCGUARD_TOPICAPP_H

#include "dxApp.h"
#include "camera.h"

class TopicApp : public DXApp
{
public:
	explicit TopicApp(HINSTANCE hInstance);
	virtual ~TopicApp();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene() =0; 

    virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

protected:
    Camera m_cam;

private:

    virtual void InitGeometryBuffers() =0;
	virtual void InitFX() =0;
	virtual void InitVertexLayout() =0;

	POINT m_lastMousePos;
};

#endif