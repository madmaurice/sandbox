//***************************************************************************************
// Camera by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "camera.h"
#include "mathHelper.h"

Camera::Camera()
	: m_position(0.0f, 0.0f, 0.0f), 
	  m_right(1.0f, 0.0f, 0.0f),
	  m_up(0.0f, 1.0f, 0.0f),
	  m_look(0.0f, 0.0f, 1.0f)
{
	setLens(0.25f*MathHelper::Pi, 1.0f, 1.0f, 1000.0f);
}

Camera::Camera(float x, float y, float z)
    : m_position(x, y, z),
	  m_right(1.0f, 0.0f, 0.0f),
	  m_up(0.0f, 1.0f, 0.0f),
	  m_look(0.0f, 0.0f, 1.0f)
{
}

Camera::~Camera()
{
}

XMVECTOR Camera::getPositionXM()const
{
	return XMLoadFloat3(&m_position);
}

XMFLOAT3 Camera::getPosition()const
{
	return m_position;
}

void Camera::setPosition(float x, float y, float z)
{
	m_position = XMFLOAT3(x, y, z);
}

void Camera::setPosition(const XMFLOAT3& v)
{
	m_position = v;
}

XMVECTOR Camera::getRightXM()const
{
	return XMLoadFloat3(&m_right);
}

XMFLOAT3 Camera::getRight()const
{
	return m_right;
}

XMVECTOR Camera::getUpXM()const
{
	return XMLoadFloat3(&m_up);
}

XMFLOAT3 Camera::getUp()const
{
	return m_up;
}

XMVECTOR Camera::getLookXM()const
{
	return XMLoadFloat3(&m_look);
}

XMFLOAT3 Camera::getLook()const
{
	return m_look;
}

float Camera::getNearZ()const
{
    return m_nearZ;
}

float Camera::getFarZ()const
{
	return m_farZ;
}

float Camera::getAspect()const
{
	return m_aspect;
}

float Camera::getFovY()const
{
	return m_fovY;
}

float Camera::getFovX()const
{
	float halfWidth = 0.5f*getNearWindowWidth();
	return 2.0f*atan(halfWidth / m_nearZ);
}

float Camera::getNearWindowWidth()const
{
	return m_aspect * m_nearWindowHeight;
}

float Camera::getNearWindowHeight()const
{
	return m_nearWindowHeight;
}

float Camera::getFarWindowWidth()const
{
	return m_aspect * m_farWindowHeight;
}

float Camera::getFarWindowHeight()const
{
	return m_farWindowHeight;
}

void Camera::setLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	m_fovY = fovY;
	m_aspect = aspect;
	m_nearZ = zn;
	m_farZ = zf;

	m_nearWindowHeight = 2.0f * m_nearZ * tanf( 0.5f*m_fovY );
	m_farWindowHeight  = 2.0f * m_farZ * tanf( 0.5f*m_fovY );

	XMMATRIX P = XMMatrixPerspectiveFovLH(m_fovY, m_aspect, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_proj, P);
}

void Camera::lookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&m_position, pos);
	XMStoreFloat3(&m_look, L);
	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
}

void Camera::lookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	lookAt(P, T, U);
}

XMMATRIX Camera::view()const
{
	return XMLoadFloat4x4(&m_view);
}

XMMATRIX Camera::proj()const
{
	return XMLoadFloat4x4(&m_proj);
}

XMMATRIX Camera::viewProj()const
{
	return XMMatrixMultiply(view(), proj());
}

void Camera::strafe(float d)
{
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(d); // become (d,d,d,d)
	XMVECTOR r = XMLoadFloat3(&m_right);
	XMVECTOR p = XMLoadFloat3(&m_position);
    // Do compenent wise multiplication of d and m_right, if m_right is (1,0,0,0)
    // this give (d,0,0,0). 
    // Each component of m_right is needed in the case of a rotated cam.
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, r, p));
}

void Camera::walk(float d)
{
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(d); // become (d,d,d,d)
	XMVECTOR l = XMLoadFloat3(&m_look);
	XMVECTOR p = XMLoadFloat3(&m_position);
	XMStoreFloat3(&m_position, XMVectorMultiplyAdd(s, l, p));
}

void Camera::pitch(float angle)
{
	// Rotate up and look vector about the right vector.
	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);

	XMStoreFloat3(&m_up,   XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));
}

void Camera::rotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis. (assume y axis = up vector)
	XMMATRIX R = XMMatrixRotationY(angle);

    // Up vector need to correspond with the up direction of the camera (it's like the head
    // of a person). If it's not aligned properly might have some glitch with the maths...
	XMStoreFloat3(&m_right,   XMVector3TransformNormal(XMLoadFloat3(&m_right), R));
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), R));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), R));
}

void Camera::updateViewMatrix()
{
    //TODO: use XMatrixLookAt built in?
    // A partir de la view matrix on peut aller chercher les elements (position, up, look) si je me souviens bien
	XMVECTOR R = XMLoadFloat3(&m_right);
	XMVECTOR U = XMLoadFloat3(&m_up);
	XMVECTOR L = XMLoadFloat3(&m_look);
	XMVECTOR P = XMLoadFloat3(&m_position);

	// Keep camera's axes orthogonal to each other and of unit length.
    // Necessary because after several rotations numerical errors can accumulate
	L = XMVector3Normalize(L);
	U = XMVector3Normalize(XMVector3Cross(L, R));

	// U, L already ortho-normal, so no need to normalize cross product.
	R = XMVector3Cross(U, L); 

	// Fill in the view matrix entries.
	float x = -XMVectorGetX(XMVector3Dot(P, R));
	float y = -XMVectorGetX(XMVector3Dot(P, U));
	float z = -XMVectorGetX(XMVector3Dot(P, L));

	XMStoreFloat3(&m_right, R);
	XMStoreFloat3(&m_up, U);
	XMStoreFloat3(&m_look, L);

    //Build the matrix
	m_view(0,0) = m_right.x; 
	m_view(1,0) = m_right.y; 
	m_view(2,0) = m_right.z; 
	m_view(3,0) = x;   

	m_view(0,1) = m_up.x;
	m_view(1,1) = m_up.y;
	m_view(2,1) = m_up.z;
	m_view(3,1) = y;  

	m_view(0,2) = m_look.x; 
	m_view(1,2) = m_look.y; 
	m_view(2,2) = m_look.z; 
	m_view(3,2) = z;   

	m_view(0,3) = 0.0f;
	m_view(1,3) = 0.0f;
	m_view(2,3) = 0.0f;
	m_view(3,3) = 1.0f;
}