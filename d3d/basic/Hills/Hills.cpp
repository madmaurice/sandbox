//---------------------------------------------------------------------------------------
//
// Draw a basic terrain
//
//---------------------------------------------------------------------------------------

#include "demoApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include "geometryGenerator.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

class HillApp : public DemoApp
{
public:
	explicit HillApp(HINSTANCE hInstance);
	~HillApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    float GetHeight(float x, float z) const;

    ComPtr<ID3D11Buffer>           m_vertexBuffer;
    ComPtr<ID3D11Buffer>           m_indexBuffer;

    ComPtr<ID3DX11Effect> m_fx;
	ID3DX11EffectTechnique* m_tech; //Not a COM object
	ID3DX11EffectMatrixVariable* m_fxWorldViewProj; //Not a COM object

    ComPtr<ID3D11InputLayout>    m_inputLayout;
    uint32 m_gridIndexCount;

    XMFLOAT4X4 m_world;

	POINT m_lastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	HillApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

HillApp::HillApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_vertexBuffer(nullptr)
, m_indexBuffer(nullptr)
, m_fx(nullptr)
, m_tech(nullptr)
, m_fxWorldViewProj(nullptr)
, m_inputLayout(nullptr)
, m_gridIndexCount(0)

{
    m_windowCaption = "Hill Demo";

    m_radius = 200.0f;

	m_lastMousePos.x = 0;
	m_lastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_world, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

HillApp::~HillApp()
{
}

bool HillApp::Init()
{
    if(!DemoApp::Init())
		return false;

	return true;
}

void HillApp::InitGeometryBuffers()
{
   	GeometryGenerator::MeshData grid;
	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, grid);

	m_gridIndexCount = grid.indices.size();

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.

	std::vector<Vertex> vertices(grid.vertices.size());
	for(size_t i = 0; i < grid.vertices.size(); ++i)
	{
		XMFLOAT3 p = grid.vertices[i].position;

		p.y = GetHeight(p.x, p.z);

		vertices[i].pos   = p;
		
		// Color the vertex based on its height.
		if( p.y < -10.0f )
		{
			// Sandy beach color.
			vertices[i].color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
		}
		else if( p.y < 5.0f )
		{
			// Light yellow-green.
			vertices[i].color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
		}
		else if( p.y < 12.0f )
		{
			// Dark yellow-green.
			vertices[i].color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
		}
		else if( p.y < 20.0f )
		{
			// Dark brown.
			vertices[i].color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
		}
		else
		{
			// White snow.
			vertices[i].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		}
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * grid.vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_vertexBuffer.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * m_gridIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_indexBuffer.GetAddressOf()));
}

void HillApp::InitFX()
{
    std::ifstream fin("fx/color.fxo", std::ios::binary);
    OC_ASSERT(fin.good());

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
    HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 0, m_dxDevice.Get(), m_fx.GetAddressOf()));

	m_tech    = m_fx->GetTechniqueByName("ColorTech");
    m_fxWorldViewProj = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void HillApp::InitVertexLayout()
{
    // Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
    D3DX11_PASS_DESC passDesc;
    m_tech->GetPassByIndex(0)->GetDesc(&passDesc);
    HR(m_dxDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
        passDesc.IAInputSignatureSize, m_inputLayout.GetAddressOf()));
}

float HillApp::GetHeight(float x, float z) const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

void HillApp::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);
}

void HillApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_dxImmediateContext->IASetInputLayout(m_inputLayout.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_dxImmediateContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_dxImmediateContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX worldViewProj = world*view*proj;
 
    D3DX11_TECHNIQUE_DESC techDesc;
    m_tech->GetDesc(&techDesc);
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
		// Draw the grid.
		m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_gridIndexCount, 0, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

