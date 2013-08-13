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
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class CrateDemo : public DemoApp
{
public:
	explicit CrateDemo(HINSTANCE hInstance);
	~CrateDemo();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    ComPtr<ID3D11Buffer>           m_boxVB;
    ComPtr<ID3D11Buffer>           m_boxIB;
 
    ComPtr<ID3D11ShaderResourceView> m_diffuseMapSRV;

	DirectionalLight m_dirLights[3];
	Material m_matBox;

	XMFLOAT4X4 m_texTransform;
	XMFLOAT4X4 m_boxWorld;

	int m_boxVertexOffset;
	UINT m_boxIndexOffset;
	UINT m_boxIndexCount;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	CrateDemo theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

CrateDemo::CrateDemo(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_boxVB(nullptr)
, m_boxIB(nullptr)
, m_diffuseMapSRV(nullptr)
, m_boxVertexOffset(0)
, m_boxIndexOffset(0)
, m_boxIndexCount(0)
{
    m_windowCaption = "Crate Demo";

    m_radius = 3.0f;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_boxWorld, I);
	XMStoreFloat4x4(&m_texTransform, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

    m_dirLights[0].ambient  = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_dirLights[0].diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_dirLights[0].specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
	m_dirLights[0].direction = XMFLOAT3(0.707f, -0.707f, 0.0f);
 
	m_dirLights[1].ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLights[1].diffuse  = XMFLOAT4(1.4f, 1.4f, 1.4f, 1.0f);
	m_dirLights[1].specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 16.0f);
	m_dirLights[1].direction = XMFLOAT3(-0.707f, 0.0f, 0.707f);

    m_dirLights[2].ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLights[2].diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLights[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLights[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	m_matBox.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_matBox.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_matBox.specular = XMFLOAT4(0.6f, 0.6f, 0.6f, 16.0f);
}

CrateDemo::~CrateDemo()
{
    Effects::DestroyAll();
    InputLayouts::DestroyAll();
}

bool CrateDemo::Init()
{
    if(!DemoApp::Init())
		return false;

	return true;
}

void CrateDemo::InitGeometryBuffers()
{
    GeometryGenerator::MeshData box;
    GeometryGenerator geoGen;
    geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

    m_boxVertexOffset = 0;
    m_boxIndexCount = box.indices.size();
    m_boxIndexOffset = 0;

    uint32 totalVertexCount = box.vertices.size();

    std::vector<Vertex::Basic32> vertices(totalVertexCount);

    size_t k = 0;
	for(size_t i = 0; i < box.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = box.vertices[i].position;
		vertices[k].Normal = box.vertices[i].normal;
		vertices[k].Tex    = box.vertices[i].texC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_boxVB.GetAddressOf()));

    std::vector<UINT> indices;
	indices.insert(indices.end(), box.indices.begin(), box.indices.end());

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * m_boxIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_boxIB.GetAddressOf()));
}


void CrateDemo::InitFX()
{    // Must init Effects first since InputLayouts depend on shader signatures.
    Effects::InitAll(m_dxDevice.Get());
    InputLayouts::InitAll(m_dxDevice.Get());

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
		"Textures/WoodCrate01.dds", 
        0, //Load info D3DX11_IMAGE_LOAD_INFO, texture loading option (width, miplevels, usage, format, filter...)
        0, //Used to spawn a new thread for the loading
        m_diffuseMapSRV.GetAddressOf(), 
        0 ));
}

void CrateDemo::InitVertexLayout()
{
}

void CrateDemo::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);
}

void CrateDemo::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;

    // Set per frame constants.
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = view*proj;

	Effects::BasicFX->SetDirLights(m_dirLights);
	Effects::BasicFX->SetEyePosW(m_camPosition);

    ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light2TexTech;
 
    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        //Draw the skull
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_boxVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_boxIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX world = XMLoadFloat4x4(&m_boxWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*view*proj;

        Effects::BasicFX->SetWorld(world);
        Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
        Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_matBox);

        Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&m_texTransform));
        Effects::BasicFX->SetDiffuseMap(m_diffuseMapSRV.Get());

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_boxIndexCount, m_boxIndexOffset, m_boxVertexOffset);
    }

	HR(m_swapChain->Present(0, 0));
}

