//---------------------------------------------------------------------------------------
//
// Draw shapes with skull
//
//---------------------------------------------------------------------------------------
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "demoApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include "geometryGenerator.h"
#include "lightHelper.h"
#include "effects.h"
#include "vertex.h"
#include "renderStates.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class TessellationApp : public DemoApp
{
public:
	explicit TessellationApp(HINSTANCE hInstance);
	~TessellationApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    ComPtr<ID3D11Buffer>           m_quadPatchVB;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	TessellationApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

TessellationApp::TessellationApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_quadPatchVB(nullptr)
{
    m_windowCaption = "Tesselation Demo";

    m_radius = 80.0f;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

TessellationApp::~TessellationApp()
{
    m_dxImmediateContext->ClearState();

    Effects::DestroyAll();
    InputLayouts::DestroyAll();
    RenderStates::DestroyAll();
}

bool TessellationApp::Init()
{
    if(!DemoApp::Init())
		return false;

	return true;
}

void TessellationApp::InitGeometryBuffers()
{
    //Build quad patch
    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(XMFLOAT3) * 4;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

    // C0 --------- C1
    //    |       |
    //    |       |
    // C3 --------- C2
	XMFLOAT3 vertices[4] = 
	{
		XMFLOAT3(-10.0f, 0.0f, +10.0f),
		XMFLOAT3(+10.0f, 0.0f, +10.0f),
		XMFLOAT3(-10.0f, 0.0f, -10.0f),
		XMFLOAT3(+10.0f, 0.0f, -10.0f)
	};

    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = vertices;
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_quadPatchVB.GetAddressOf()));
}


void TessellationApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
    Effects::InitAll(m_dxDevice.Get());
    InputLayouts::InitAll(m_dxDevice.Get());
    RenderStates::InitAll(m_dxDevice.Get());
}

void TessellationApp::InitVertexLayout()
{
}

void TessellationApp::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);
}

void TessellationApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Set the topology as a point patch with 4 control points (quad)
    m_dxImmediateContext->IASetInputLayout(InputLayouts::Pos.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	uint32 stride = sizeof(Vertex::Pos);
    uint32 offset = 0;

    // Set per frame constants.
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = view*proj;

    Effects::TessellationFX->SetEyePosW(m_camPosition);
	Effects::TessellationFX->SetFogColor(oc::Colors::Silver);
	Effects::TessellationFX->SetFogStart(15.0f);
	Effects::TessellationFX->SetFogRange(175.0f);

    ID3DX11EffectTechnique* activeTech = Effects::TessellationFX->TessTech;
 
    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_quadPatchVB.GetAddressOf(), &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMMatrixIdentity();
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::TessellationFX->SetWorld(world);
		Effects::TessellationFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::TessellationFX->SetWorldViewProj(worldViewProj);
		Effects::TessellationFX->SetTexTransform(XMMatrixIdentity());
		//Effects::TessellationFX->SetMaterial(0);
		Effects::TessellationFX->SetDiffuseMap(0);

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());

        m_dxImmediateContext->RSSetState(RenderStates::WireframeRS.Get());
		m_dxImmediateContext->Draw(4, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

