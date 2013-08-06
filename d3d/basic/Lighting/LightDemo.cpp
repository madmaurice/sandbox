//---------------------------------------------------------------------------------------
//
// Draw Waves demo with lighting
//
//---------------------------------------------------------------------------------------

#include "demoApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include "geometryGenerator.h"
#include "lightHelper.h"
#include "waves.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT3 normal;
};

class LightDemo : public DemoApp
{
public:
	explicit LightDemo(HINSTANCE hInstance);
	~LightDemo();

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
    XMFLOAT3 GetHillNormal(float x, float z) const;

    ComPtr<ID3D11Buffer>           m_landVB;
    ComPtr<ID3D11Buffer>           m_landIB;
    ComPtr<ID3D11Buffer>           m_wavesVB;
    ComPtr<ID3D11Buffer>           m_wavesIB;

    ComPtr<ID3DX11Effect> m_fx;
	ID3DX11EffectTechnique* m_tech; //Not a COM object
	ID3DX11EffectMatrixVariable* m_fxWorldViewProj; //Not a COM object
    ID3DX11EffectMatrixVariable* m_fxWorld;
	ID3DX11EffectMatrixVariable* m_fxWorldInvTranspose;
	ID3DX11EffectVectorVariable* m_fxEyePosW;
	ID3DX11EffectVariable* m_fxDirLight;
	ID3DX11EffectVariable* m_fxPointLight;
	ID3DX11EffectVariable* m_fxSpotLight;
	ID3DX11EffectVariable* m_fxMaterial;

    ComPtr<ID3D11InputLayout>    m_inputLayout;

	Waves m_waves;
    DirectionalLight m_dirLight;
	PointLight m_pointLight;
	SpotLight m_spotLight;
	Material m_landMat;
	Material m_wavesMat;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_landWorld;
	XMFLOAT4X4 m_wavesWorld;

	uint32 m_landIndexCount;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	LightDemo theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

LightDemo::LightDemo(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_landVB(nullptr)
, m_landIB(nullptr)
, m_wavesVB(nullptr)
, m_wavesIB(nullptr)
, m_fx(nullptr)
, m_tech(nullptr)
, m_fxWorldViewProj(nullptr)
, m_fxWorld(nullptr)
, m_fxWorldInvTranspose(nullptr)
, m_fxEyePosW(nullptr)
, m_fxDirLight(nullptr)
, m_fxPointLight(nullptr)
, m_fxSpotLight(nullptr)
, m_fxMaterial(nullptr)
, m_inputLayout(nullptr)
{
    m_windowCaption = "Lighting Demo";

    m_radius = 200.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_landWorld, I);
    XMStoreFloat4x4(&m_wavesWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

    XMMATRIX wavesOffset = XMMatrixTranslation(0.0f, -3.0f, 0.0f);
	XMStoreFloat4x4(&m_wavesWorld, wavesOffset);

    // Directional light.
	m_dirLight.ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLight.diffuse  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);
 
	// Point light--position is changed every frame to animate in UpdateScene function.
	m_pointLight.ambient  = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pointLight.diffuse  = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pointLight.specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pointLight.att      = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_pointLight.range    = 35.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	m_spotLight.ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_spotLight.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_spotLight.specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_spotLight.att      = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_spotLight.spot     = 96.0f;
	m_spotLight.range    = 10000.0f;

	m_landMat.ambient  = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_landMat.diffuse  = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_landMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	m_wavesMat.ambient  = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	m_wavesMat.diffuse  = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	m_wavesMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);
}

LightDemo::~LightDemo()
{
}

bool LightDemo::Init()
{
    m_waves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

    if(!DemoApp::Init())
		return false;

	return true;
}

void LightDemo::InitGeometryBuffers()
{
    BuildLandBuffers();
    BuildWavesBuffers();
}

void LightDemo::BuildLandBuffers()
{
   	GeometryGenerator::MeshData land;
	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, land);

	m_landIndexCount = land.indices.size();

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.

	std::vector<Vertex> vertices(land.vertices.size());
	for(size_t i = 0; i < land.vertices.size(); ++i)
	{
		XMFLOAT3 p = land.vertices[i].position;

		p.y = GetHeight(p.x, p.z);

		vertices[i].pos   = p;
        vertices[i].normal = GetHillNormal(p.x, p.z);
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex) * land.vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_landVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(uint32) * m_landIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &land.indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_landIB.GetAddressOf()));
}

void LightDemo::BuildWavesBuffers()
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

float LightDemo::GetHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

XMFLOAT3 LightDemo::GetHillNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));
	
	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void LightDemo::InitFX()
{
    std::ifstream fin("fx/Lighting.fxo", std::ios::binary);
    OC_ASSERT(fin.good());

	fin.seekg(0, std::ios_base::end);
	int size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);
	std::vector<char> compiledShader(size);

	fin.read(&compiledShader[0], size);
	fin.close();
	
	HR(D3DX11CreateEffectFromMemory(&compiledShader[0], size, 
		0, m_dxDevice.Get(), m_fx.GetAddressOf()));

	m_tech    = m_fx->GetTechniqueByName("LightTech");
	m_fxWorldViewProj = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
    m_fxWorld             = m_fx->GetVariableByName("gWorld")->AsMatrix();
	m_fxWorldInvTranspose = m_fx->GetVariableByName("gWorldInvTranspose")->AsMatrix();
	m_fxEyePosW           = m_fx->GetVariableByName("gEyePosW")->AsVector();
	m_fxDirLight          = m_fx->GetVariableByName("gDirLight");
	m_fxPointLight        = m_fx->GetVariableByName("gPointLight");
	m_fxSpotLight         = m_fx->GetVariableByName("gSpotLight");
	m_fxMaterial          = m_fx->GetVariableByName("gMaterial");
}

void LightDemo::InitVertexLayout()
{
    // Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
    D3DX11_PASS_DESC passDesc;
    m_tech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(m_dxDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
		passDesc.IAInputSignatureSize, m_inputLayout.GetAddressOf()));
}

void LightDemo::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);

	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if( (m_timer.TotalTime() - t_base) >= 0.25f )
	{
		t_base += 0.25f;
 
		uint32 i = 5 + rand() % (m_waves.RowCount()-10);
		uint32 j = 5 + rand() % (m_waves.ColumnCount()-10);

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
		v[i].normal = m_waves.Normal(i);
	}

	m_dxImmediateContext->Unmap(m_wavesVB.Get(), 0);

	// Animate the lights.

	// Circle light over the land surface.
	m_pointLight.position.x = 70.0f*cosf( 0.2f*m_timer.TotalTime() );
	m_pointLight.position.z = 70.0f*sinf( 0.2f*m_timer.TotalTime() );
	m_pointLight.position.y = MathHelper::Max(GetHeight(m_pointLight.position.x, 
		m_pointLight.position.z), -3.0f) + 10.0f;


	// The spotlight takes on the camera position and is aimed in the
	// same direction the camera is looking.  In this way, it looks
	// like we are holding a flashlight.
    m_spotLight.position = m_camPosition;

    XMVECTOR pos = XMLoadFloat3(&m_camPosition);
    XMVECTOR target = XMLoadFloat3(&m_camTarget);
	XMStoreFloat3(&m_spotLight.direction, XMVector3Normalize(target - pos));
}

void LightDemo::DrawScene()
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
	XMMATRIX viewProj  = view*proj;

    // Set per frame constants.
	m_fxDirLight->SetRawValue(&m_dirLight, 0, sizeof(m_dirLight));
	m_fxPointLight->SetRawValue(&m_pointLight, 0, sizeof(m_pointLight));
	m_fxSpotLight->SetRawValue(&m_spotLight, 0, sizeof(m_spotLight));
	m_fxEyePosW->SetRawValue(&m_camPosition, 0, sizeof(m_camPosition));
 
    D3DX11_TECHNIQUE_DESC techDesc;
    m_tech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        //Draw the land
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_landVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_landIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        //Set per object constants
        XMMATRIX world = XMLoadFloat4x4(&m_landWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*viewProj;

		m_fxWorld->SetMatrix(reinterpret_cast<float*>(&world));
		m_fxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
		m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        m_fxMaterial->SetRawValue(&m_landMat, 0, sizeof(m_landMat));

        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_landIndexCount, 0, 0);

        //Draw the wave
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_wavesVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_wavesIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        world = XMLoadFloat4x4(&m_wavesWorld);
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world*viewProj;

		m_fxWorld->SetMatrix(reinterpret_cast<float*>(&world));
		m_fxWorldInvTranspose->SetMatrix(reinterpret_cast<float*>(&worldInvTranspose));
		m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));
        m_fxMaterial->SetRawValue(&m_wavesMat, 0, sizeof(m_wavesMat));

        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(3*m_waves.TriangleCount(), 0, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

