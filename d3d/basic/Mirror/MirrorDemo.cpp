//---------------------------------------------------------------------------------------
//
// Demonstrates using the stencil buffer to mask out areas from being drawn to, 
// and to prevent "double blending."
//
// Controls:
//		Hold the left mouse button down and move the mouse to rotate.
//      Hold the right mouse button down to zoom in and out.
//
//      Press '1' - Lighting only render mode.
//      Press '2' - Texture render mode.
//      Press '3' - Fog render mode.
//
//		Move the skull left/right/up/down with 'A'/'D'/'W'/'S' keys.//
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

class MirrorApp : public DemoApp
{
public:
	explicit MirrorApp(HINSTANCE hInstance);
	~MirrorApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    void BuildRoomBuffers();
	void BuildSkullBuffers();

    ComPtr<ID3D11Buffer>           m_roomVB;
    ComPtr<ID3D11Buffer>           m_skullVB;
    ComPtr<ID3D11Buffer>           m_skullIB;

	ComPtr<ID3D11ShaderResourceView> m_floorDiffuseMapSRV;
	ComPtr<ID3D11ShaderResourceView> m_wallDiffuseMapSRV;
	ComPtr<ID3D11ShaderResourceView> m_mirrorDiffuseMapSRV;

    DirectionalLight m_dirLight[3];
	Material m_roomMat;
	Material m_skullMat;
	Material m_mirrorMat;
	Material m_shadowMat;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_roomWorld;
	XMFLOAT4X4 m_skullWorld;

	uint32 m_skullIndexCount;
    XMFLOAT3 m_skullTranslation;

    RenderOptions m_renderOptions;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	MirrorApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

MirrorApp::MirrorApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_roomVB(nullptr)
, m_skullVB(nullptr)
, m_skullIB(nullptr)
, m_floorDiffuseMapSRV(nullptr)
, m_wallDiffuseMapSRV(nullptr)
, m_mirrorDiffuseMapSRV(nullptr)
, m_skullIndexCount(0)
, m_skullTranslation(0.0f, 1.0f, -5.0f)
, m_renderOptions(RenderOptions::Textures)
{
    m_windowCaption = "Mirror Demo";
    m_enable4xMsaa = false;

    m_radius = 12.0f;
    m_theta = 1.24f*MathHelper::Pi;
    m_phi = 0.42*MathHelper::Pi;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_roomWorld, I);
    XMStoreFloat4x4(&m_skullWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

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

	m_roomMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_roomMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_roomMat.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

    m_skullMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_skullMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_skullMat.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	// Reflected material is transparent so it blends into mirror.
	m_mirrorMat.ambient  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_mirrorMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	m_mirrorMat.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	m_shadowMat.ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_shadowMat.diffuse  = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	m_shadowMat.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
}

MirrorApp::~MirrorApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool MirrorApp::Init()
{
    if(!DemoApp::Init())
		return false;

	return true;
}

void MirrorApp::InitGeometryBuffers()
{
    BuildRoomBuffers();
    BuildSkullBuffers();
}

void MirrorApp::BuildSkullBuffers()
{
    std::ifstream fin("Models/skull.txt");
    OC_ASSERT(fin.good());

	UINT vcount = 0;
	UINT tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount;
	fin >> ignore >> tcount;
	fin >> ignore >> ignore >> ignore >> ignore;
	
	std::vector<Vertex::Basic32> vertices(vcount);
	for(UINT i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

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

void MirrorApp::BuildRoomBuffers()
{
    // Create and specify geometry.  For this sample we draw a floor
	// and a wall with a mirror on it.  We put the floor, wall, and
	// mirror geometry in one vertex buffer.
	//
	//   |--------------|
	//   |              |
    //   |----|----|----|
    //   |Wall|Mirr|Wall|
	//   |    | or |    |
    //   /--------------/
    //  /   Floor      /
	// /--------------/

 
    //No indices buffer (some vertex are defined 2 times)
	Vertex::Basic32 v[30];

	// Floor: Observe we tile texture coordinates.
	v[0] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[1] = Vertex::Basic32(-3.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	
	v[3] = Vertex::Basic32(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f);
	v[4] = Vertex::Basic32( 7.5f, 0.0f,   0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f);
	v[5] = Vertex::Basic32( 7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f);

	// Wall: Observe we tile texture coordinates, and that we
	// leave a gap in the middle for the mirror.
	v[6]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[7]  = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[8]  = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	
	v[9]  = Vertex::Basic32(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[10] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f);
	v[11] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f);

	v[12] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[13] = Vertex::Basic32(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[14] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	
	v[15] = Vertex::Basic32(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f);
	v[16] = Vertex::Basic32(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f);
	v[17] = Vertex::Basic32(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f);

	v[18] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[19] = Vertex::Basic32(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[20] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	
	v[21] = Vertex::Basic32(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[22] = Vertex::Basic32( 7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f);
	v[23] = Vertex::Basic32( 7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f);

	// Mirror
	v[24] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[25] = Vertex::Basic32(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[26] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	
	v[27] = Vertex::Basic32(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[28] = Vertex::Basic32( 2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[29] = Vertex::Basic32( 2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * 30;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = v;
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_roomVB.GetAddressOf()));
}

void MirrorApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());

	HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/checkboard.dds", 0, 0, m_floorDiffuseMapSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/brick01.dds", 0, 0, m_wallDiffuseMapSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/ice.dds", 0, 0, m_mirrorDiffuseMapSRV.GetAddressOf(), 0 ));
}

void MirrorApp::InitVertexLayout() { }

void MirrorApp::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);

	// Switch the render mode based in key input.
	if( GetAsyncKeyState('1') & 0x8000 )
        m_renderOptions = RenderOptions::Lighting; 

	if( GetAsyncKeyState('2') & 0x8000 )
		m_renderOptions = RenderOptions::Textures; 

	if( GetAsyncKeyState('3') & 0x8000 )
		m_renderOptions = RenderOptions::TexturesAndFog; 

	// Allow user to move box.
	if( GetAsyncKeyState('A') & 0x8000 )
        m_skullTranslation.x -= 1.0f*dt;

	if( GetAsyncKeyState('D') & 0x8000 )
		m_skullTranslation.x += 1.0f*dt;

	if( GetAsyncKeyState('W') & 0x8000 )
		m_skullTranslation.y += 1.0f*dt;

	if( GetAsyncKeyState('S') & 0x8000 )
		m_skullTranslation.y -= 1.0f*dt;

	// Don't let user move below ground plane.
    m_skullTranslation.y = MathHelper::Max(m_skullTranslation.y, 0.0f);

	// Update the new world matrix.
	XMMATRIX skullRotate = XMMatrixRotationY(0.5f*MathHelper::Pi);
	XMMATRIX skullScale = XMMatrixScaling(0.45f, 0.45f, 0.45f);
	XMMATRIX skullOffset = XMMatrixTranslation(m_skullTranslation.x, m_skullTranslation.y, m_skullTranslation.z);
    XMStoreFloat4x4(&m_skullWorld, skullRotate*skullScale*skullOffset);
}

void MirrorApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Black));
    //Reset depth buffer to 1 and stencil buffer to 0
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::Basic32.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float blendFactor[] = {0.0f, 0.0f, 0.0f, 0.0f};

	uint32 stride = sizeof(Vertex::Basic32);
    uint32 offset = 0;

	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj  = view*proj;

    // Set per frame constants.
    Effects::BasicFX->SetDirLights(m_dirLight);
    Effects::BasicFX->SetEyePosW(m_camPosition);
   	Effects::BasicFX->SetFogColor(oc::Colors::Black);
	Effects::BasicFX->SetFogStart(2.0f);
	Effects::BasicFX->SetFogRange(40.0f);

    // Skull doesn't have texture coordinates, so we can't texture it.
	ID3DX11EffectTechnique* activeTech = nullptr;
	ID3DX11EffectTechnique* activeSkullTech= nullptr;

    switch(m_renderOptions)
	{
	case RenderOptions::Lighting:
		activeTech = Effects::BasicFX->Light3Tech;
		activeSkullTech = Effects::BasicFX->Light3Tech;
		break;
	case RenderOptions::Textures:
		activeTech = Effects::BasicFX->Light3TexTech;
		activeSkullTech = Effects::BasicFX->Light3Tech;
		break;
	case RenderOptions::TexturesAndFog:
		activeTech = Effects::BasicFX->Light3TexFogTech;
		activeSkullTech = Effects::BasicFX->Light3FogTech;
		break;
	}
 
    D3DX11_TECHNIQUE_DESC techDesc;

    // Draw the floor and walls to the back buffer as normal.
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_roomVB.GetAddressOf(), &stride, &offset);

        // Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_roomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_roomMat);

        // Floor
        Effects::BasicFX->SetDiffuseMap(m_floorDiffuseMapSRV.Get());
        pass->Apply(0, m_dxImmediateContext.Get());
		m_dxImmediateContext->Draw(6, 0);

		// Wall
		Effects::BasicFX->SetDiffuseMap(m_wallDiffuseMapSRV.Get());
		pass->Apply(0, m_dxImmediateContext.Get());
		m_dxImmediateContext->Draw(18, 6);
    }

    // Draw the skull to the back buffer as normal.
    activeSkullTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_skullVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        XMMATRIX world = XMLoadFloat4x4(&m_skullWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_skullMat);

        pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_skullIndexCount, 0, 0);
    }

    // Draw the mirror to the stencil buffer only
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_roomVB.GetAddressOf(), &stride, &offset);

        // Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_roomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());

        // Do not write to render target (disable writing to the back buffer by setting RenderTargetWriteMask = 0).
		m_dxImmediateContext->OMSetBlendState(RenderStates::NoRenderTargetWritesBS.Get(), blendFactor, 0xffffffff);

		// Render visible mirror pixels to stencil buffer (skull is drawed first for this, might occlude pixel on the mirror).
		// Do not write mirror depth to depth buffer at this point, otherwise it will occlude the reflection.
		m_dxImmediateContext->OMSetDepthStencilState(RenderStates::MarkMirrorDSS.Get(), 1);
		
		pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->Draw(6, 24);

        // At this point the visible part of the mirror is "marked" on the stencil buffer (with 1)
		// Restore states.
		m_dxImmediateContext->OMSetDepthStencilState(0, 0);
		m_dxImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
    }

    // Draw the skull reflection
    activeSkullTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_skullVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Reflect the skull by multiplying it's world with the reflection matrix
        XMVECTOR mirrorPlane = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
		XMMATRIX R = XMMatrixReflect(mirrorPlane);
        XMMATRIX world = XMLoadFloat4x4(&m_skullWorld) * R;
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_skullMat);

        // Reflect the lights too
        // Cache the old light directions, and reflect the light directions.
		XMFLOAT3 oldLightDirections[3];
		for(int i = 0; i < 3; ++i)
		{
			oldLightDirections[i] = m_dirLight[i].direction;

			XMVECTOR lightDir = XMLoadFloat3(&m_dirLight[i].direction);
			XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
			XMStoreFloat3(&m_dirLight[i].direction, reflectedLightDir);
		}

		Effects::BasicFX->SetDirLights(m_dirLight);

        // Cull clockwise triangles for reflection.
        // If not face normal will be inversed
        m_dxImmediateContext->RSSetState(RenderStates::CullClockwiseRS.Get());

		// Only draw reflection into visible mirror pixels as marked by the stencil buffer. 
        // Stencil test check that value of the stencil buffer is 1 (StencilRef) if not depth/stencil
        // test fail. See: http://msdn.microsoft.com/en-us/library/windows/desktop/bb205120(v=vs.85).aspx
		m_dxImmediateContext->OMSetDepthStencilState(RenderStates::DrawReflectionDSS.Get(), 1);
		pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_skullIndexCount, 0, 0);

		// Restore default states.
		m_dxImmediateContext->RSSetState(0);	
		m_dxImmediateContext->OMSetDepthStencilState(0, 0);	

        // Restore light directions.
		for(int i = 0; i < 3; ++i)
		{
			m_dirLight[i].direction = oldLightDirections[i];
		}

		Effects::BasicFX->SetDirLights(m_dirLight);
    }

	// Draw the mirror to the back buffer as usual but with transparency
	// blending so the reflection shows through.
	activeTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
		ID3DX11EffectPass* pass = activeTech->GetPassByIndex( p );

		m_dxImmediateContext->IASetVertexBuffers(0, 1, m_roomVB.GetAddressOf(), &stride, &offset);

		// Set per object constants.
		XMMATRIX world = XMLoadFloat4x4(&m_roomWorld);
		XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
		XMMATRIX worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
        Effects::BasicFX->SetMaterial(m_mirrorMat);
        Effects::BasicFX->SetDiffuseMap(m_mirrorDiffuseMapSRV.Get());

		// Mirror
		m_dxImmediateContext->OMSetBlendState(RenderStates::TransparentBS.Get(), blendFactor, 0xffffffff);
		pass->Apply(0, m_dxImmediateContext.Get());
		m_dxImmediateContext->Draw(6, 24);
	}

    // Draw the skull shadow
    activeSkullTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_skullVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Reflect the skull by multiplying it's world with the reflection matrix
        XMVECTOR shadowPlane = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane

        // Construct shadow matrix in this case Lw = 0 which mean the vector describe
        // the direction toward an infinitely far away light source.
        XMVECTOR toMainLight = -XMLoadFloat3(&m_dirLight[0].direction);
		XMMATRIX S =  XMMatrixShadow(shadowPlane, toMainLight);
		XMMATRIX shadowOffsetY = XMMatrixTranslation(0.0f, 0.001f, 0.0f);

        XMMATRIX world = XMLoadFloat4x4(&m_skullWorld)*S*shadowOffsetY;
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_shadowMat);

        // Only draw flattened triangle one time
		m_dxImmediateContext->OMSetDepthStencilState(RenderStates::NoDoubleBlendDSS.Get(), 0);
		pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_skullIndexCount, 0, 0);

		// Restore default states.
        m_dxImmediateContext->OMSetBlendState(0, blendFactor, 0xffffffff);
		m_dxImmediateContext->OMSetDepthStencilState(0, 0);	
	}

	HR(m_swapChain->Present(0, 0));
}

