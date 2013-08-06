//---------------------------------------------------------------------------------------
//
// Draw some basic shapes
//
//---------------------------------------------------------------------------------------

#include "demoApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include "geometryGenerator.h"
#include "waves.h"
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

class WavesDemo : public DemoApp
{
public:
	explicit WavesDemo(HINSTANCE hInstance);
	~WavesDemo();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    void BuildLandBuffers();
	void BuildWavesBuffers();

    float GetHeight(float x, float z) const;

    ComPtr<ID3D11Buffer>           m_landVB;
    ComPtr<ID3D11Buffer>           m_landIB;
    ComPtr<ID3D11Buffer>           m_wavesVB;
    ComPtr<ID3D11Buffer>           m_wavesIB;

    ComPtr<ID3DX11Effect> m_fx;
	ID3DX11EffectTechnique* m_tech; //Not a COM object
	ID3DX11EffectMatrixVariable* m_fxWorldViewProj; //Not a COM object

    ComPtr<ID3D11InputLayout>    m_inputLayout;
    ComPtr<ID3D11RasterizerState> m_wireframeRS;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_gridWorld;
	XMFLOAT4X4 m_wavesWorld;

	uint32 m_gridIndexCount;

	Waves m_waves;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	WavesDemo theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

WavesDemo::WavesDemo(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_landVB(nullptr)
, m_landIB(nullptr)
, m_wavesVB(nullptr)
, m_wavesIB(nullptr)
, m_fx(nullptr)
, m_tech(nullptr)
, m_fxWorldViewProj(nullptr)
, m_inputLayout(nullptr)
, m_wireframeRS(nullptr)
{
    m_windowCaption = "Waves Demo";

    m_radius = 200.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_gridWorld, I);
    XMStoreFloat4x4(&m_wavesWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

WavesDemo::~WavesDemo()
{
}

bool WavesDemo::Init()
{
    m_waves.Init(200, 200, 0.8f, 0.03f, 3.25f, 0.4f);

    if(!DemoApp::Init())
		return false;

    D3D11_RASTERIZER_DESC wireFrameDesc;
    ZeroMemory(&wireFrameDesc, sizeof(D3D11_RASTERIZER_DESC));
    wireFrameDesc.FillMode = D3D11_FILL_WIREFRAME;
    wireFrameDesc.CullMode = D3D11_CULL_BACK;
    wireFrameDesc.FrontCounterClockwise = false;
	wireFrameDesc.DepthClipEnable = true;

    HR(m_dxDevice->CreateRasterizerState(&wireFrameDesc, m_wireframeRS.GetAddressOf()));

	return true;
}

void WavesDemo::InitGeometryBuffers()
{
    BuildLandBuffers();
    BuildWavesBuffers();
}

void WavesDemo::BuildLandBuffers()
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
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_landVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(uint32) * m_gridIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &grid.indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_landIB.GetAddressOf()));
}

void WavesDemo::BuildWavesBuffers()
{
    // Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DYNAMIC; //Dynamic instead of Immutable
    vbd.ByteWidth = sizeof(Vertex) * m_waves.VertexCount();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //Write access since it's dynamic
    vbd.MiscFlags = 0;

    //No data is passed because we will set it at each frame
    HR(m_dxDevice->CreateBuffer(&vbd, 0, m_wavesVB.GetAddressOf()));

	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.
	std::vector<uint32> indices(3*m_waves.TriangleCount()); // 3 indices per face

	// Iterate over each quad.
	uint32 m = m_waves.RowCount();
	uint32 n = m_waves.ColumnCount();
	int k = 0;
	for(uint32 i = 0; i < m-1; ++i)
	{
		for(uint32 j = 0; j < n-1; ++j)
		{
			indices[k]   = i*n+j;
			indices[k+1] = i*n+j+1;
			indices[k+2] = (i+1)*n+j;

			indices[k+3] = (i+1)*n+j;
			indices[k+4] = i*n+j+1;
			indices[k+5] = (i+1)*n+j+1;

			k += 6; // next quad
		}
	}

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(uint32) * indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_wavesIB.GetAddressOf()));
}

float WavesDemo::GetHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

void WavesDemo::InitFX()
{
    std::ifstream fin("fx/color.fxo", std::ios::binary);
    OC_ASSERT(fin.good());

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 
		0, m_dxDevice.Get(), m_fx.GetAddressOf()));

	m_tech    = m_fx->GetTechniqueByName("ColorTech");
	m_fxWorldViewProj = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void WavesDemo::InitVertexLayout()
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

void WavesDemo::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);

	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if( (m_timer.TotalTime() - t_base) >= 0.25f )
	{
		t_base += 0.25f;
 
		uint32 i = 5 + rand() % 190;
		uint32 j = 5 + rand() % 190;

		float r = MathHelper::RandF(1.0f, 2.0f);

		m_waves.Disturb(i, j, r);
	}

	m_waves.Update(dt);

	// Update the wave vertex buffer with the new solution.
	
	D3D11_MAPPED_SUBRESOURCE mappedData;
	HR(m_dxImmediateContext->Map(m_wavesVB.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData));

	Vertex* v = reinterpret_cast<Vertex*>(mappedData.pData);
	for(UINT i = 0; i < m_waves.VertexCount(); ++i)
	{
		v[i].pos = m_waves[i];
		v[i].color = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	}

	m_dxImmediateContext->Unmap(m_wavesVB.Get(), 0);
}

void WavesDemo::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_dxImmediateContext->IASetInputLayout(m_inputLayout.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32 stride = sizeof(Vertex);
    uint32 offset = 0;

	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
 
    D3DX11_TECHNIQUE_DESC techDesc;
    m_tech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        //Draw the land
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_landVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_landIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX world = XMLoadFloat4x4(&m_gridWorld);
		XMMATRIX worldViewProj = world*view*proj;
		m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_gridIndexCount, 0, 0);

        //Draw the wave

        //Set wireframe mode
        m_dxImmediateContext->RSSetState(m_wireframeRS.Get());

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_wavesVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_wavesIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        world = XMLoadFloat4x4(&m_wavesWorld);
		worldViewProj = world*view*proj;
		m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(3*m_waves.TriangleCount(), 0, 0);

        //Restore default
        m_dxImmediateContext->RSSetState(0);
    }

	HR(m_swapChain->Present(0, 0));
}

