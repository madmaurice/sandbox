//---------------------------------------------------------------------------------------
//
// Draw Waves demo with blending
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
#include "waves.h"
#include "effects.h"
#include "renderStates.h"
#include "vertex.h"
#include "blurFilter.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

enum RenderOptions
{
	Lighting = 0,
	Textures = 1,
	TexturesAndFog = 2
};

class BlurApp : public DemoApp
{
public:
	explicit BlurApp(HINSTANCE hInstance);
	~BlurApp();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

	void UpdateWaves();
    void DrawWrapper();
	void DrawScreenQuad();

	void BuildCrateBuffers();
    void BuildLandBuffers();
	void BuildWavesBuffers();
    void BuildScreenQuadGeometryBuffers();
	void BuildOffscreenViews();

    float GetHeight(float x, float z) const;
    XMFLOAT3 GetHillNormal(float x, float z) const;

    ComPtr<ID3D11Buffer>           m_landVB;
    ComPtr<ID3D11Buffer>           m_landIB;

    ComPtr<ID3D11Buffer>           m_wavesVB;
    ComPtr<ID3D11Buffer>           m_wavesIB;

    ComPtr<ID3D11Buffer>           m_boxVB;
    ComPtr<ID3D11Buffer>           m_boxIB;

    ComPtr<ID3D11Buffer>           m_screenQuadVB;
    ComPtr<ID3D11Buffer>           m_screenQuadIB;

	ComPtr<ID3D11ShaderResourceView> m_grassMapSRV;
	ComPtr<ID3D11ShaderResourceView> m_wavesMapSRV;
	ComPtr<ID3D11ShaderResourceView> m_boxMapSRV;

    //Off screen texture resources
    ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;
	ComPtr<ID3D11UnorderedAccessView> m_offscreenUAV;
	ComPtr<ID3D11RenderTargetView> m_offscreenRTV;

    BlurFilter m_blur;
	Waves m_waves;

	Material m_landMat;
	Material m_wavesMat;
	Material m_boxMat;

    DirectionalLight m_dirLight[3];

    // Define textures transform
    XMFLOAT4X4 m_grassTexTransform;
    XMFLOAT4X4 m_waterTexTransform;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_landWorld;
	XMFLOAT4X4 m_wavesWorld;
	XMFLOAT4X4 m_boxWorld;

	uint32 m_landIndexCount;


	XMFLOAT2 m_waterTexOffset;

    RenderOptions m_renderOptions;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	BlurApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

BlurApp::BlurApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_landVB(nullptr)
, m_landIB(nullptr)
, m_wavesVB(nullptr)
, m_wavesIB(nullptr)
, m_boxVB(nullptr)
, m_boxIB(nullptr)
, m_screenQuadVB(nullptr)
, m_screenQuadIB(nullptr)
, m_grassMapSRV(nullptr)
, m_wavesMapSRV(nullptr)
, m_boxMapSRV(nullptr)
, m_offscreenSRV(nullptr)
, m_offscreenUAV(nullptr)
, m_offscreenRTV(nullptr)
, m_landIndexCount(0)
, m_waterTexOffset(0.0f, 0.0f)
, m_renderOptions(RenderOptions::Textures)
{
    m_windowCaption = "Blur Demo";
    
    //Enable 4X MSAA for alpha-to-coverage
    m_enable4xMsaa = false;

    m_radius = 80.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_landWorld, I);
    XMStoreFloat4x4(&m_wavesWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

    XMMATRIX boxScale = XMMatrixScaling(15.0f, 15.0f, 15.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(8.0f, 5.0f, -15.0f);
	XMStoreFloat4x4(&m_boxWorld, boxScale*boxOffset);

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
	m_wavesMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	m_wavesMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 32.0f);

	m_boxMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_boxMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_boxMat.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

BlurApp::~BlurApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool BlurApp::Init()
{
    m_waves.Init(160, 160, 1.0f, 0.03f, 5.0f, 0.3f);

    if(!DemoApp::Init())
		return false;

	return true;
}

void BlurApp::InitGeometryBuffers()
{
    BuildCrateBuffers();
    BuildLandBuffers();
    BuildWavesBuffers();
    BuildScreenQuadGeometryBuffers();
	BuildOffscreenViews();
}

void BlurApp::BuildCrateBuffers()
{
    GeometryGenerator::MeshData box;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	std::vector<Vertex::Basic32> vertices(box.vertices.size());

	for(UINT i = 0; i < box.vertices.size(); ++i)
	{
		vertices[i].Pos    = box.vertices[i].position;
		vertices[i].Normal = box.vertices[i].normal;
		vertices[i].Tex    = box.vertices[i].texC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * box.vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_boxVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * box.indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &box.indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_boxIB.GetAddressOf()));
}

void BlurApp::BuildLandBuffers()
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

void BlurApp::BuildWavesBuffers()
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

void BlurApp::BuildScreenQuadGeometryBuffers()
{
    GeometryGenerator::MeshData quad;

	GeometryGenerator geoGen;
	geoGen.CreateFullscreenQuad(quad);

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	std::vector<Vertex::Basic32> vertices(quad.vertices.size());

	for(UINT i = 0; i < quad.vertices.size(); ++i)
	{
		vertices[i].Pos    = quad.vertices[i].position;
		vertices[i].Normal = quad.vertices[i].normal;
		vertices[i].Tex    = quad.vertices[i].texC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * quad.vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_screenQuadVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * quad.indices.size();
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &quad.indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_screenQuadIB.GetAddressOf()));
}

void BlurApp::BuildOffscreenViews()
{
    // We call this function everytime the window is resized so that the render target is a quarter
	// the client area dimensions.  So Release the previous views before we create new ones.
	m_offscreenSRV = nullptr;
	m_offscreenRTV = nullptr;
	m_offscreenUAV = nullptr;

	D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width     = m_windowWidth;
    texDesc.Height    = m_windowHeight;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count   = 1;  
	texDesc.SampleDesc.Quality = 0;  
	texDesc.Usage          = D3D11_USAGE_DEFAULT;
	texDesc.BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	texDesc.CPUAccessFlags = 0; 
	texDesc.MiscFlags      = 0;

	ComPtr<ID3D11Texture2D> offscreenTex = 0;
    HR(m_dxDevice->CreateTexture2D(&texDesc, 0, offscreenTex.GetAddressOf()));

	// Null description means to create a view to all mipmap levels using 
	// the format the texture was created with.
    HR(m_dxDevice->CreateShaderResourceView(offscreenTex.Get(), 0, m_offscreenSRV.GetAddressOf()));
	HR(m_dxDevice->CreateRenderTargetView(offscreenTex.Get(), 0, m_offscreenRTV.GetAddressOf()));
	HR(m_dxDevice->CreateUnorderedAccessView(offscreenTex.Get(), 0, m_offscreenUAV.GetAddressOf()));
}

float BlurApp::GetHeight(float x, float z)const
{
	return 0.3f*( z*sinf(0.1f*x) + x*cosf(0.1f*z) );
}

XMFLOAT3 BlurApp::GetHillNormal(float x, float z)const
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

void BlurApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());

	HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/grass.dds", 0, 0, m_grassMapSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/water2.dds", 0, 0, m_wavesMapSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/WireFence.dds", 0, 0, m_boxMapSRV.GetAddressOf(), 0 ));
}

void BlurApp::InitVertexLayout() {}

void BlurApp::OnResize()
{
    DemoApp::OnResize();

    // Recreate the resources that depend on the client area size.
    // Both resources need to be the same size

    // Rebuild offscreen texture
	BuildOffscreenViews();

    //Rebuild texture used by the blur implementation
    m_blur.Init(m_dxDevice.Get(), m_windowWidth, m_windowHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
}

void BlurApp::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);

	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if( (m_timer.TotalTime() - t_base) >= 0.1f )
	{
		t_base += 0.1f;
 
		uint32 i = 5 + rand() % (m_waves.RowCount()-10);
		uint32 j = 5 + rand() % (m_waves.ColumnCount()-10);

		float r = MathHelper::RandF(0.5f, 1.0f);

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

	// Switch the render mode based in key input.
	if( GetAsyncKeyState('1') & 0x8000 )
        m_renderOptions = RenderOptions::Lighting; 

	if( GetAsyncKeyState('2') & 0x8000 )
		m_renderOptions = RenderOptions::Textures; 

	if( GetAsyncKeyState('3') & 0x8000 )
		m_renderOptions = RenderOptions::TexturesAndFog; 
}

void BlurApp::DrawWrapper()
{
	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj  = view*proj;

    float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;

    // Set per frame constants.
    Effects::BasicFX->SetDirLights(m_dirLight);
    Effects::BasicFX->SetEyePosW(m_camPosition);
   	Effects::BasicFX->SetFogColor(oc::Colors::Silver);
	Effects::BasicFX->SetFogStart(25.0f);
	Effects::BasicFX->SetFogRange(200.0f);

   	ID3DX11EffectTechnique* boxTech = nullptr;
    ID3DX11EffectTechnique* landAndWavesTech = nullptr;

    switch(m_renderOptions)
	{
	case RenderOptions::Lighting:
		boxTech = Effects::BasicFX->Light3Tech;
		landAndWavesTech = Effects::BasicFX->Light3Tech;
		break;
	case RenderOptions::Textures:
		boxTech = Effects::BasicFX->Light3TexAlphaClipTech;
		landAndWavesTech = Effects::BasicFX->Light3TexTech;
		break;
	case RenderOptions::TexturesAndFog:
		boxTech = Effects::BasicFX->Light3TexAlphaClipFogTech;
		landAndWavesTech = Effects::BasicFX->Light3TexFogTech;
		break;
	}
 
    D3DX11_TECHNIQUE_DESC techDesc;
    boxTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        //Draw the box with alpha clipping
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_boxVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_boxIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_boxWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_boxMat);
		Effects::BasicFX->SetDiffuseMap(m_boxMapSRV.Get());

        //Disable backface culling to render the crate properly (else the back of the crate won't be rendered)
        m_dxImmediateContext->RSSetState(RenderStates::NoCullRS.Get());
        boxTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
		m_dxImmediateContext->DrawIndexed(36, 0, 0);

		// Restore default render state.
		m_dxImmediateContext->RSSetState(0);
    }

    //Draw the hills and water with texture and fog (no alpha clipping needed)
    landAndWavesTech->GetDesc(&techDesc);
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

        landAndWavesTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
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

        m_dxImmediateContext->OMSetBlendState(RenderStates::TransparentBS.Get(), blendFactor, 0xffffffff);
        landAndWavesTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(3*m_waves.TriangleCount(), 0, 0);

		// Restore default blend state
		m_dxImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
    }
}

void BlurApp::DrawScreenQuad()
{
    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 
	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;
 
	XMMATRIX identity = XMMatrixIdentity();
 
	ID3DX11EffectTechnique* texOnlyTech = Effects::BasicFX->Light0TexTech;
	D3DX11_TECHNIQUE_DESC techDesc;

	texOnlyTech->GetDesc( &techDesc );
	for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_screenQuadVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_screenQuadIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Identity matrix is needed as world since screen quad is already in NDC space
		Effects::BasicFX->SetWorld(identity);
		Effects::BasicFX->SetWorldInvTranspose(identity);
		Effects::BasicFX->SetWorldViewProj(identity);
		Effects::BasicFX->SetTexTransform(identity);
        Effects::BasicFX->SetDiffuseMap(m_offscreenSRV.Get());

        texOnlyTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
		m_dxImmediateContext->DrawIndexed(6, 0, 0);
    }
}

void BlurApp::DrawScene()
{
    //Overview
    // 1- Set the OM render target to an offscreen texture
    // 2- Draw the whole scene as usual
    // 3- Set the OM render target as usual (back buffer)
    // 4- Apply the blur to the offscreen texture
    // 5- Draw the offscreen texture on the back buffer

    // Render to our offscreen texture.  Note that we can use the same depth/stencil buffer
	// we normally use since our offscreen texture matches the dimensions.  
    // Set render target to offscreen texture
    m_dxImmediateContext->OMSetRenderTargets(1, m_offscreenRTV.GetAddressOf(), m_depthStencilView.Get());

    m_dxImmediateContext->ClearRenderTargetView(m_offscreenRTV.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Draw the scene to the offscreen texture
	DrawWrapper();

	// Restore the back buffer.  The offscreen render target will serve as an input into
	// the compute shader for blurring, so we must unbind it from the OM stage before we
	// can use it as an input into the compute shader.
    m_dxImmediateContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	//m_blur.SetGaussianWeights(2.0f);
    m_blur.BlurInPlace(m_dxImmediateContext.Get(), m_offscreenSRV.Get(), m_offscreenUAV.Get(), 2);

	// Draw fullscreen quad with texture of blurred scene on it.
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    // Draw offscreen texture on the back buffer
	DrawScreenQuad();


	HR(m_swapChain->Present(0, 0));
}

