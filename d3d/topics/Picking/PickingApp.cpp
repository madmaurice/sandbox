//---------------------------------------------------------------------------------------
//
// Simple 1st person camera
//
//---------------------------------------------------------------------------------------

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "topicApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include "geometryGenerator.h"
#include "lightHelper.h"
#include "effects.h"
#include "renderStates.h"
#include "vertex.h"
#include "xnacollision.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class PickingApp : public TopicApp
{
public:
	explicit PickingApp(HINSTANCE hInstance);
	~PickingApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

    virtual void OnMouseDown(WPARAM btnState, int x, int y);

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    void BuildMeshGeometryBuffers();
    void Pick(int sx, int sy);

    ComPtr<ID3D11Buffer>           m_meshVB;
    ComPtr<ID3D11Buffer>           m_meshIB;

    // Keep system memory copies of the Mesh geometry for picking
    std::vector<Vertex::Basic32> m_meshVertices;
    std::vector<uint32> m_meshIndices;

    XNA::AxisAlignedBox m_meshBox;

    DirectionalLight m_dirLight[3];
	Material m_meshMat;
	Material m_pickedTriangleMat;

    // Define transformations from local spaces to world space.
	XMFLOAT4X4 m_meshWorld;

	uint32 m_meshIndexCount;
	uint32 m_pickedTriangle;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	PickingApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

PickingApp::PickingApp(HINSTANCE hInstance)
: TopicApp(hInstance) 
, m_meshVB(nullptr)
, m_meshIB(nullptr)
, m_meshIndexCount(0)
, m_pickedTriangle(-1)
{
    m_windowCaption = "Picking Demo";
    m_enable4xMsaa = false;

    m_cam.setPosition(0.0f, 2.0f, -15.0f);

    XMMATRIX meshScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX meshOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
	XMStoreFloat4x4(&m_meshWorld, XMMatrixMultiply(meshScale, meshOffset));

    // Directional light.
    m_dirLight[0].ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLight[0].diffuse  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight[0].direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	m_dirLight[1].ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLight[1].diffuse  = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	m_dirLight[1].specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	m_dirLight[1].direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	m_dirLight[2].ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLight[2].diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLight[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLight[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

    m_meshMat.ambient  = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_meshMat.diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_meshMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

    m_pickedTriangleMat.ambient  = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
	m_pickedTriangleMat.diffuse  = XMFLOAT4(0.0f, 0.8f, 0.4f, 1.0f);
	m_pickedTriangleMat.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
}

PickingApp::~PickingApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool PickingApp::Init()
{
    if(!TopicApp::Init())
		return false;

	return true;
}

void PickingApp::InitGeometryBuffers()
{
    BuildMeshGeometryBuffers();
}

void PickingApp::BuildMeshGeometryBuffers()
{
    std::ifstream fin("Models/car.txt");
    OC_ASSERT(fin.good());

	uint32 vcount = 0;
	uint32 tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

    // Min and max are for constructing the bounding box
    XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
	
	XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
	
    m_meshVertices.resize(vcount);
	for(uint32 i = 0; i < vcount; ++i)
	{
        fin >> m_meshVertices[i].Pos.x >> m_meshVertices[i].Pos.y >> m_meshVertices[i].Pos.z;
		fin >> m_meshVertices[i].Normal.x >> m_meshVertices[i].Normal.y >> m_meshVertices[i].Normal.z;

        XMVECTOR P = XMLoadFloat3(&m_meshVertices[i].Pos);
		
		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

    // Set the AABB
    XMStoreFloat3(&m_meshBox.Center, 0.5f*(vMin+vMax));
	XMStoreFloat3(&m_meshBox.Extents, 0.5f*(vMax-vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

    m_meshIndexCount = 3*tcount;
    m_meshIndices.resize(m_meshIndexCount);
	for(uint32 i = 0; i < tcount; ++i)
	{
        fin >> m_meshIndices[i*3+0] >> m_meshIndices[i*3+1] >> m_meshIndices[i*3+2];
	}

	fin.close();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &m_meshVertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_meshVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * m_meshIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &m_meshIndices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_meshIB.GetAddressOf()));
}

void PickingApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());
}

void PickingApp::InitVertexLayout() { }

void PickingApp::UpdateScene(float dt)
{
    TopicApp::UpdateScene(dt);
}

void PickingApp::OnMouseDown(WPARAM btnState, int x, int y)
{
    if( (btnState & MK_LBUTTON) != 0 )
	{
        TopicApp::OnMouseDown(btnState, x, y);
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		Pick(x, y);
	}
}

void PickingApp::Pick(int sx, int sy)
{
    XMMATRIX P = m_cam.proj();

	// Compute picking ray in view space.

    // Convert coordinate to [-1, 1]
    float pointX = (+2.0f*sx/m_windowWidth  - 1.0f);
    float pointY = (-2.0f*sy/m_windowHeight + 1.0f);

    // Adjust the points using the projection matrix to account for the aspect ratio of the viewport.
    float vx = pointX/P(0,0);
    float vy = pointY/P(1,1);

    // TODO : check this (rayDir with vx and vy...)
    // Ray definition in view space.
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir    = XMVectorSet(vx, vy, 1.0f, 0.0f);

	// Tranform ray to local space of Mesh.
	XMMATRIX V = m_cam.view();
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(V), V);

	XMMATRIX W = XMLoadFloat4x4(&m_meshWorld);
	XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

	XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);

    // TODO : check this (rayDir Normal?)
    rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
	rayDir = XMVector3TransformNormal(rayDir, toLocal);

	// Make the ray direction unit length for the intersection tests.
	rayDir = XMVector3Normalize(rayDir);

	// If we hit the bounding box of the Mesh, then we might have picked a Mesh triangle,
	// so do the ray/triangle tests.
	//
	// If we did not hit the bounding box, then it is impossible that we hit 
	// the Mesh, so do not waste effort doing ray/triangle tests.

	// Assume we have not picked anything yet, so init to -1.
    m_pickedTriangle = -1;
	float tmin = 0.0f;
    if(XNA::IntersectRayAxisAlignedBox(rayOrigin, rayDir, &m_meshBox, &tmin))
	{
		// Find the nearest ray/triangle intersection.
		tmin = MathHelper::Infinity;
        for(UINT i = 0; i < m_meshIndices.size()/3; ++i)
		{
			// Indices for this triangle.
			UINT i0 = m_meshIndices[i*3+0];
			UINT i1 = m_meshIndices[i*3+1];
			UINT i2 = m_meshIndices[i*3+2];

			// Vertices for this triangle.
            XMVECTOR v0 = XMLoadFloat3(&m_meshVertices[i0].Pos);
			XMVECTOR v1 = XMLoadFloat3(&m_meshVertices[i1].Pos);
			XMVECTOR v2 = XMLoadFloat3(&m_meshVertices[i2].Pos);

			// We have to iterate over all the triangles in order to find the nearest intersection.
			float t = 0.0f;
			if(XNA::IntersectRayTriangle(rayOrigin, rayDir, v0, v1, v2, &t))
			{
				if( t < tmin )
				{
					// This is the new nearest picked triangle.
					tmin = t;
                    m_pickedTriangle = i;
				}
			}
		}
	}
}

void PickingApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    //Reset depth buffer to 1 and stencil buffer to 0
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;

	// Set constants
    XMMATRIX view  = m_cam.view();
    XMMATRIX proj  = m_cam.proj();
    XMMATRIX viewProj  = m_cam.viewProj();

    // Set per frame constants.
    Effects::BasicFX->SetDirLights(m_dirLight);
    Effects::BasicFX->SetEyePosW(m_cam.getPosition());

    // Skull doesn't have texture coordinates, so we can't texture it.
    ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light3Tech;

    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );

        if( GetAsyncKeyState('1') & 0x8000 )
            m_dxImmediateContext->RSSetState(RenderStates::WireframeRS.Get());

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_meshVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_meshIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Draw the grid
        XMMATRIX world = XMLoadFloat4x4(&m_meshWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(m_meshMat);

        pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_meshIndexCount, 0, 0);

        // Restore default
        m_dxImmediateContext->RSSetState(0);

        if(m_pickedTriangle != -1)
		{
			// Change depth test from < to <= so that if we draw the same triangle twice, it will still pass
			// the depth test.  This is because we redraw the picked triangle with a different material
			// to highlight it.  
            m_dxImmediateContext->OMSetDepthStencilState(RenderStates::LessEqualDSS.Get(), 0);

            Effects::BasicFX->SetMaterial(m_pickedTriangleMat);
            pass->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(3, 3*m_pickedTriangle, 0);

			// restore default
			m_dxImmediateContext->OMSetDepthStencilState(0, 0);
		}
    }

	HR(m_swapChain->Present(0, 0));
}

