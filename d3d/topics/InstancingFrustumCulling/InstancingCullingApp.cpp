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

struct InstanceData
{
    XMFLOAT4X4 World;
    XMFLOAT4 Color;
};

class InstancingCullingApp : public TopicApp
{
public:
	explicit InstancingCullingApp(HINSTANCE hInstance);
	~InstancingCullingApp();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

	void BuildSkullBuffers();
	void BuildInstancedBuffer();

    ComPtr<ID3D11Buffer>           m_skullVB;
    ComPtr<ID3D11Buffer>           m_skullIB;
    ComPtr<ID3D11Buffer>           m_instancedBuffer;

   	XNA::AxisAlignedBox m_skullbox;
	XNA::Frustum m_camFrustum;

    uint32 m_visibleObjectCount;

    std::vector<InstanceData> m_instancedData;

    bool m_frustumCullingEnabled;

    DirectionalLight m_dirLight[3];
    Material m_skullMat;

    // Define transformations from local spaces to world space.
	XMFLOAT4X4 m_skullWorld;

	uint32 m_skullIndexCount;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	InstancingCullingApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

InstancingCullingApp::InstancingCullingApp(HINSTANCE hInstance)
: TopicApp(hInstance) 
, m_skullVB(nullptr)
, m_skullIB(nullptr)
, m_skullIndexCount(0)
, m_visibleObjectCount(0)
, m_frustumCullingEnabled(true)
{
    m_windowCaption = "Culling Demo";
    m_enable4xMsaa = false;

    m_cam.setPosition(0.0f, 2.0f, -15.0f);

    XMMATRIX I = XMMatrixIdentity();

    XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
    XMStoreFloat4x4(&m_skullWorld, XMMatrixMultiply(skullScale, skullOffset));

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

    m_skullMat.ambient  = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);
	m_skullMat.diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_skullMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
}

InstancingCullingApp::~InstancingCullingApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool InstancingCullingApp::Init()
{
    if(!TopicApp::Init())
		return false;

	return true;
}

void InstancingCullingApp::OnResize()
{
    TopicApp::OnResize();

    // Build the frustum from the projection matrix in view space.
    ComputeFrustumFromProjection(&m_camFrustum, &m_cam.proj());
}

void InstancingCullingApp::InitGeometryBuffers()
{
    BuildSkullBuffers();
    BuildInstancedBuffer();
}

void InstancingCullingApp::BuildSkullBuffers()
{
    std::ifstream fin("Models/skull.txt");
    OC_ASSERT(fin.good());

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;

    // Get min and max vector (for bounding box)
	XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
	XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
	
    XMVECTOR vMin = XMLoadFloat3(&vMinf3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxf3);
	std::vector<Vertex::Basic32> vertices(vcount);
	for(UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;

        XMVECTOR P = XMLoadFloat3(&vertices[i].Pos);
		
		vMin = XMVectorMin(vMin, P);
		vMax = XMVectorMax(vMax, P);
	}

    // 15.2.2
    XMStoreFloat3(&m_skullbox.Center, 0.5f*(vMin+vMax));
	XMStoreFloat3(&m_skullbox.Extents, 0.5f*(vMax-vMin));

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

    m_skullIndexCount = 3*tcount;
	std::vector<UINT> indices(m_skullIndexCount);
	for(UINT i = 0; i < tcount; ++i)
	{
		fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
	}

	fin.close();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * vcount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_skullVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * m_skullIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_skullIB.GetAddressOf()));
}

void InstancingCullingApp::BuildInstancedBuffer()
{
    // Construct all the instanced data on the system side (C++)
    // To be copied to the dynamic buffer on frame update

    // 5 x 5 x 5 grid of skull
    const int n = 5;
    m_instancedData.resize(n*n*n);
	
    // Scene is 200 x 200 x 200
	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;
	
	float x = -0.5f*width;
	float y = -0.5f*height;
	float z = -0.5f*depth;
	float dx = width / (n-1);
	float dy = height / (n-1);
	float dz = depth / (n-1);
	for(int k = 0; k < n; ++k)
	{
		for(int i = 0; i < n; ++i)
		{
			for(int j = 0; j < n; ++j)
			{
				// Position instanced along a 3D grid.
                m_instancedData[k*n*n + i*n + j].World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					x+j*dx, y+i*dy, z+k*dz, 1.0f); //translation only
				
				// Random color.
                m_instancedData[k*n*n + i*n + j].Color.x = MathHelper::RandF(0.0f, 1.0f);
				m_instancedData[k*n*n + i*n + j].Color.y = MathHelper::RandF(0.0f, 1.0f);
				m_instancedData[k*n*n + i*n + j].Color.z = MathHelper::RandF(0.0f, 1.0f);
				m_instancedData[k*n*n + i*n + j].Color.w = 1.0f;
			}
		}
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.ByteWidth = sizeof(InstanceData) * m_instancedData.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

    HR(m_dxDevice->CreateBuffer(&vbd, 0, m_instancedBuffer.GetAddressOf()));
}

void InstancingCullingApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());
}

void InstancingCullingApp::InitVertexLayout() { }

void InstancingCullingApp::UpdateScene(float dt)
{
    TopicApp::UpdateScene(dt);

	// Switch culling
	if( GetAsyncKeyState('1') & 0x8000 )
        m_frustumCullingEnabled = true;

	if( GetAsyncKeyState('2') & 0x8000 )
		m_frustumCullingEnabled = false;

    //Perform culling
    m_visibleObjectCount = 0;

    if(m_frustumCullingEnabled)
	{
        // Get matrix to transform view to world
        // Needed to transform frustum to instanced skulls local coordinate (for frustum test)
		XMVECTOR detView = XMMatrixDeterminant(m_cam.view());
		XMMATRIX invView = XMMatrixInverse(&detView, m_cam.view());
	
		D3D11_MAPPED_SUBRESOURCE mappedData; 
        m_dxImmediateContext->Map(m_instancedBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

		InstanceData* dataView = reinterpret_cast<InstanceData*>(mappedData.pData);

        // For each instanced data check if the bounding box is in the frustum
        // and if it's the case copy it to the dynamic buffer
        for(UINT i = 0; i < m_instancedData.size(); ++i)
		{
			XMMATRIX W = XMLoadFloat4x4(&m_instancedData[i].World);
			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);

			// View space to the object's local space.
			XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);
		
			// Decompose the matrix into its individual parts.
			XMVECTOR scale;
			XMVECTOR rotQuat;
			XMVECTOR translation;
			XMMatrixDecompose(&scale, &rotQuat, &translation, toLocal);

			// Transform the camera frustum from view space to the object's local space.
			XNA::Frustum localspaceFrustum;
            XNA::TransformFrustum(&localspaceFrustum, &m_camFrustum, XMVectorGetX(scale), rotQuat, translation);

			// Perform the box/frustum intersection test in local space.
            if(XNA::IntersectAxisAlignedBoxFrustum(&m_skullbox, &localspaceFrustum) != 0)
			{
				// Write the instance data to dynamic VB of the visible objects.
                dataView[m_visibleObjectCount++] = m_instancedData[i];
			}
		}

        m_dxImmediateContext->Unmap(m_instancedBuffer.Get(), 0);
	}
	else // No culling enabled, draw all objects.
	{
		D3D11_MAPPED_SUBRESOURCE mappedData; 
        m_dxImmediateContext->Map(m_instancedBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

		InstanceData* dataView = reinterpret_cast<InstanceData*>(mappedData.pData);

		for(UINT i = 0; i < m_instancedData.size(); ++i)
		{
            dataView[m_visibleObjectCount++] = m_instancedData[i];
		}

        m_dxImmediateContext->Unmap(m_instancedBuffer.Get(), 0);
	}

	std::stringstream outs;   
	outs.precision(6);
	outs << "Instancing and Culling Demo" << 
		"    " << m_visibleObjectCount << 
        " objects visible out of " << m_instancedData.size();
    m_windowCaption = outs.str();
}

void InstancingCullingApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    //Reset depth buffer to 1 and stencil buffer to 0
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::InstancedBasic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Use 2 input slots, one for the vertex data and the other for the instanced data
    uint32 stride[2] = {sizeof(Vertex::Basic32), sizeof(InstanceData)};
    uint32 offset[2] = {0, 0};

    ID3D11Buffer* vbs[2] = {m_skullVB.Get(), m_instancedBuffer.Get()};
    /* test...
    ID3D11Buffer** test = (ID3D11Buffer**) ::operator new(sizeof(ID3D11Buffer*) *2);
    *test = m_skullVB.Get();
    *(test +1) = m_instancedBuffer.Get();
    */

	// Set constants
    XMMATRIX view  = m_cam.view();
    XMMATRIX proj  = m_cam.proj();
    XMMATRIX viewProj  = m_cam.viewProj();

    // Set per frame constants.
    Effects::InstancedBasicFX->SetDirLights(m_dirLight);
    Effects::InstancedBasicFX->SetEyePosW(m_cam.getPosition());

    // Skull doesn't have texture coordinates, so we can't texture it.
    ID3DX11EffectTechnique* activeTech = Effects::InstancedBasicFX->Light3Tech;

    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);

    // Draw the skulls with instancing
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 2, vbs, stride, offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX world = XMLoadFloat4x4(&m_skullWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);

        Effects::InstancedBasicFX->SetWorld(world);
		Effects::InstancedBasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::InstancedBasicFX->SetViewProj(viewProj);
        Effects::InstancedBasicFX->SetMaterial(m_skullMat);

        pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexedInstanced(m_skullIndexCount, m_visibleObjectCount, 0, 0, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

