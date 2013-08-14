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
#include "effects.h"
#include "vertex.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class TexturedWavesApp : public DemoApp
{
public:
	explicit TexturedWavesApp(HINSTANCE hInstance);
	~TexturedWavesApp();

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

	ComPtr<ID3D11ShaderResourceView> m_grassMapSRV;
	ComPtr<ID3D11ShaderResourceView> m_wavesMapSRV;

	Waves m_waves;

	Material m_landMat;
	Material m_wavesMat;

    DirectionalLight m_dirLight[3];

    // Define textures transform
    XMFLOAT4X4 m_grassTexTransform;
    XMFLOAT4X4 m_waterTexTransform;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_landWorld;
	XMFLOAT4X4 m_wavesWorld;

	uint32 m_landIndexCount;

	XMFLOAT2 m_waterTexOffset;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	TexturedWavesApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

TexturedWavesApp::TexturedWavesApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_landVB(nullptr)
, m_landIB(nullptr)
, m_wavesVB(nullptr)
, m_wavesIB(nullptr)
, m_grassMapSRV(nullptr)
, m_wavesMapSRV(nullptr)
, m_landIndexCount(0)
, m_waterTexOffset(0.0f, 0.0f)
{
    m_windowCaption = "TexturedWaves Demo";

    m_radius = 200.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_landWorld, I);
    XMStoreFloat4x4(&m_wavesWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

    // Scale the grass texture 5 times (wrap)
    XMMATRIX grassTexScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);
	XMStoreFloat4x4(&m_grassTexTransform, grassTexScale);

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

	m_landMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_landMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_landMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	m_wavesMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_wavesMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_wavesMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);
}

TexturedWavesApp::~TexturedWavesApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
}

bool TexturedWavesApp::Init()
{
    m_waves.Init(160, 160, 1.0f, 0.03f, 3.25f, 0.4f);

    if(!DemoApp::Init())
		return false;

	return true;
}

void TexturedWavesApp::InitGeometryBuffers()
{
    BuildLandBuffers();
    BuildWavesBuffers();
}

void TexturedWavesApp::BuildLandBuffers()
{
   	GeometryGenerator::MeshData land;
	GeometryGenerator geoGen;

	geoGen.CreateGrid(160.0f, 160.0f, 50, 50, land);

	m_landIndexCount = land.indices.size();

	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.

	std::vector<Vertex::Basic32> vertices(land.vertices.size());
	for(size_t i = 0; i < land.vertices.size(); ++i)
	{
		XMFLOAT3 p = land.vertices[i].position;

		p.y = GetHeight(p.x, p.z);

		vertices[i].Pos   = p;
        vertices[i].Normal = GetHillNormal(p.x, p.z);
        vertices[i].Tex = land.vertices[i].texC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::Basic32) * land.vertices.size();
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

void TexturedWavesApp::BuildWavesBuffers()
{
    // Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DYNAMIC; //Dynamic instead of Immutable
    vbd.ByteWidth = sizeof(Vertex::Basic32) * m_waves.VertexCount();
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

float TexturedWavesApp::GetHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

XMFLOAT3 TexturedWavesApp::GetHillNormal(float x, float z)const
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

void TexturedWavesApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());

	HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/grass.dds", 0, 0, m_grassMapSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/water2.dds", 0, 0, m_wavesMapSRV.GetAddressOf(), 0 ));
}

void TexturedWavesApp::InitVertexLayout()
{
}

void TexturedWavesApp::UpdateScene(float dt)
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

    Vertex::Basic32* v = reinterpret_cast<Vertex::Basic32*>(mappedData.pData);
	for(UINT i = 0; i < m_waves.VertexCount(); ++i)
	{
		v[i].Pos = m_waves[i];
		v[i].Normal = m_waves.Normal(i);

        // Derive tex-coords in [0,1] from position.
		v[i].Tex.x  = 0.5f + m_waves[i].x / m_waves.Width();
		v[i].Tex.y  = 0.5f - m_waves[i].z / m_waves.Depth();
	}

	m_dxImmediateContext->Unmap(m_wavesVB.Get(), 0);

	// Animate water texture coordinates.

	// Tile water texture.
	XMMATRIX wavesScale = XMMatrixScaling(5.0f, 5.0f, 0.0f);

	// Translate texture over time (this animate the texture)
	m_waterTexOffset.y += 0.05f*dt;
	m_waterTexOffset.x += 0.1f*dt;	
	XMMATRIX wavesOffset = XMMatrixTranslation(m_waterTexOffset.x, m_waterTexOffset.y, 0.0f);

	// Combine scale and translation.
	XMStoreFloat4x4(&m_waterTexTransform, wavesScale*wavesOffset);
}

void TexturedWavesApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;

	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj  = view*proj;

    // Set per frame constants.
    Effects::BasicFX->SetDirLights(m_dirLight);
    Effects::BasicFX->SetEyePosW(m_camPosition);
 
    ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light3TexTech;
    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        //Draw the land
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_landVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_landIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        //Set per object constants
        XMMATRIX world = XMLoadFloat4x4(&m_landWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&m_grassTexTransform));
        Effects::BasicFX->SetMaterial(m_landMat);
        Effects::BasicFX->SetDiffuseMap(m_grassMapSRV.Get());

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_landIndexCount, 0, 0);

        //Draw the wave
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_wavesVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_wavesIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        world = XMLoadFloat4x4(&m_wavesWorld);
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetTexTransform(XMLoadFloat4x4(&m_waterTexTransform));
        Effects::BasicFX->SetMaterial(m_wavesMat);
        Effects::BasicFX->SetDiffuseMap(m_wavesMapSRV.Get());

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(3*m_waves.TriangleCount(), 0, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

