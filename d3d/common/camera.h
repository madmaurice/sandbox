//***************************************************************************************
// Camera.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Simple first person style camera class that lets the viewer explore the 3D scene.
//   -It keeps track of the camera coordinate system relative to the world space
//    so that the view matrix can be constructed.  
//   -It keeps track of the viewing frustum of the camera so that the projection
//    matrix can be obtained.
//***************************************************************************************

#ifndef _INCGUARD_CAMERA_H
#define _INCGUARD_CAMERA_H

#include "dxUtil.h"

class Camera
{
public:
	Camera();
	Camera(float x, float y, float z);
	~Camera();

	// Get/Set world camera position.
	XMVECTOR getPositionXM() const;
	XMFLOAT3 getPosition() const;
	void setPosition(float x, float y, float z);
	void setPosition(const XMFLOAT3& v);
	
	// Get camera basis vectors.
	XMVECTOR getRightXM() const;
	XMFLOAT3 getRight() const;
	XMVECTOR getUpXM() const;
	XMFLOAT3 getUp() const;
	XMVECTOR getLookXM() const;
	XMFLOAT3 getLook() const;

	// Get frustum properties.
	float getNearZ() const;
	float getFarZ() const;
	float getAspect() const;
	float getFovY() const;
	float getFovX() const;

	// Get near and far plane dimensions in view space coordinates.
	float getNearWindowWidth() const;
	float getNearWindowHeight() const;
	float getFarWindowWidth() const;
	float getFarWindowHeight() const;
	
	// Set frustum.
	void setLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void lookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp);
	void lookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up);

	// Get View/Proj matrices.
	XMMATRIX view() const;
	XMMATRIX proj() const;
	XMMATRIX viewProj() const;

	// Strafe/Walk the camera a distance d.
	void strafe(float d);
	void walk(float d);

	// Rotate the camera.
	void pitch(float angle);
	void rotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void updateViewMatrix();

private:

	// Camera coordinate system with coordinates relative to world space.
	XMFLOAT3 m_position;
	XMFLOAT3 m_right;
	XMFLOAT3 m_up;
	XMFLOAT3 m_look;

	// Cache frustum properties.
	float m_nearZ;
	float m_farZ;
	float m_aspect;
	float m_fovY;
	float m_nearWindowHeight;
	float m_farWindowHeight;

	// Cache View/Proj matrices.
	XMFLOAT4X4 m_view;
	XMFLOAT4X4 m_proj;
};

#endif // CAMERA_H