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
#include "sky.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

class CubeMapApp : public TopicApp
{
public:
	explicit CubeMapApp(HINSTANCE hInstance);
	~CubeMapApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    void BuildShapeBuffers();
	void BuildSkullBuffers();

    std::unique_ptr<Sky> m_sky;

    ComPtr<ID3D11Buffer>           m_shapeVB;
    ComPtr<ID3D11Buffer>           m_shapeIB;

    ComPtr<ID3D11Buffer>           m_skullVB;
    ComPtr<ID3D11Buffer>           m_skullIB;

    ComPtr<ID3D11Buffer>           m_skySphereVB;
    ComPtr<ID3D11Buffer>           m_skySphereIB;

	ComPtr<ID3D11ShaderResourceView> m_floorTexSRV;
	ComPtr<ID3D11ShaderResourceView> m_stoneTexSRV;
	ComPtr<ID3D11ShaderResourceView> m_brickTexSRV;

    DirectionalLight m_dirLight[3];
	Material m_gridMat;
	Material m_boxMat;
	Material m_cylinderMat;
	Material m_sphereMat;
	Material m_skullMat;

    // Define transformations from local spaces to world space.
    XMFLOAT4X4 m_sphereWorld[10];
    XMFLOAT4X4 m_cylWorld[10];
	XMFLOAT4X4 m_boxWorld;
	XMFLOAT4X4 m_gridWorld;
	XMFLOAT4X4 m_skullWorld;

    int32 m_boxVertexOffset;
	int32 m_gridVertexOffset;
	int32 m_sphereVertexOffset;
	int32 m_cylinderVertexOffset;

    uint32 m_boxIndexOffset;
	uint32 m_gridIndexOffset;
	uint32 m_sphereIndexOffset;
	uint32 m_cylinderIndexOffset;

    uint32 m_boxIndexCount;
	uint32 m_gridIndexCount;
	uint32 m_sphereIndexCount;
	uint32 m_cylinderIndexCount;

	uint32 m_skullIndexCount;

	uint32 m_lightCount;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	CubeMapApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

CubeMapApp::CubeMapApp(HINSTANCE hInstance)
: TopicApp(hInstance) 
, m_sky(nullptr)
, m_shapeVB(nullptr)
, m_shapeIB(nullptr)
, m_skullVB(nullptr)
, m_skullIB(nullptr)
, m_floorTexSRV(nullptr)
, m_stoneTexSRV(nullptr)
, m_brickTexSRV(nullptr)
, m_skullIndexCount(0)
, m_lightCount(3)
{
    m_windowCaption = "CubeMap Demo";
    m_enable4xMsaa = false;

    m_cam.setPosition(0.0f, 2.0f, -15.0f);

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_gridWorld, I);

	XMMATRIX boxScale = XMMatrixScaling(3.0f, 1.0f, 3.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
    XMStoreFloat4x4(&m_boxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX skullScale = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX skullOffset = XMMatrixTranslation(0.0f, 1.0f, 0.0f);
    XMStoreFloat4x4(&m_skullWorld, XMMatrixMultiply(skullScale, skullOffset));

    for(int i = 0; i < 5; ++i)
	{
        XMStoreFloat4x4(&m_cylWorld[i*2+0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_cylWorld[i*2+1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&m_sphereWorld[i*2+0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_sphereWorld[i*2+1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}

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

	m_gridMat.ambient  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_gridMat.diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_gridMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
    m_gridMat.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); //No reflection

	m_cylinderMat.ambient  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cylinderMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cylinderMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_cylinderMat.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); //No reflection

	m_sphereMat.ambient  = XMFLOAT4(0.2f, 0.3f, 0.4f, 1.0f);
	m_sphereMat.diffuse  = XMFLOAT4(0.3f, 0.3f, 0.4f, 1.0f);
	m_sphereMat.specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);
    m_sphereMat.reflect = XMFLOAT4(0.4f, 0.4f, 0.4f, 1.0f);

	m_boxMat.ambient  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_boxMat.diffuse  = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_boxMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_boxMat.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); //No reflection

	m_skullMat.ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_skullMat.diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_skullMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
    m_skullMat.reflect = XMFLOAT4(0.5f, 0.5f, 0.5f, 16.0f);
}

CubeMapApp::~CubeMapApp()
{
    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool CubeMapApp::Init()
{
    if(!TopicApp::Init())
		return false;

	return true;
}

void CubeMapApp::InitGeometryBuffers()
{
    BuildShapeBuffers();
    BuildSkullBuffers();
}

void CubeMapApp::BuildSkullBuffers()
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

void CubeMapApp::BuildShapeBuffers()
{
    GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20, cylinder);

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
	m_boxVertexOffset      = 0;
	m_gridVertexOffset     = box.vertices.size();
	m_sphereVertexOffset   = m_gridVertexOffset + grid.vertices.size();
	m_cylinderVertexOffset = m_sphereVertexOffset + sphere.vertices.size();

	// Cache the index count of each object.
	m_boxIndexCount      = box.indices.size();
	m_gridIndexCount     = grid.indices.size();
	m_sphereIndexCount   = sphere.indices.size();
	m_cylinderIndexCount = cylinder.indices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	m_boxIndexOffset      = 0;
	m_gridIndexOffset     = m_boxIndexCount;
	m_sphereIndexOffset   = m_gridIndexOffset + m_gridIndexCount;
	m_cylinderIndexOffset = m_sphereIndexOffset + m_sphereIndexCount;
	
	uint32 totalVertexCount = 
		box.vertices.size() + 
		grid.vertices.size() + 
		sphere.vertices.size() +
		cylinder.vertices.size();

	uint32 totalIndexCount = 
		m_boxIndexCount + 
		m_gridIndexCount + 
		m_sphereIndexCount +
		m_cylinderIndexCount;

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	std::vector<Vertex::Basic32> vertices(totalVertexCount);

	UINT k = 0;
	for(size_t i = 0; i < box.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = box.vertices[i].position;
		vertices[k].Normal = box.vertices[i].normal;
		vertices[k].Tex    = box.vertices[i].texC;
	}

	for(size_t i = 0; i < grid.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = grid.vertices[i].position;
		vertices[k].Normal = grid.vertices[i].normal;
		vertices[k].Tex    = grid.vertices[i].texC;
	}

	for(size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = sphere.vertices[i].position;
		vertices[k].Normal = sphere.vertices[i].normal;
		vertices[k].Tex    = sphere.vertices[i].texC;
	}

	for(size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos    = cylinder.vertices[i].position;
		vertices[k].Normal = cylinder.vertices[i].normal;
		vertices[k].Tex    = cylinder.vertices[i].texC;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::Basic32) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_shapeVB.GetAddressOf()));

	// Pack the indices of all the meshes into one index buffer.
	std::vector<uint32> indices;
	indices.insert(indices.end(), box.indices.begin(), box.indices.end());
	indices.insert(indices.end(), grid.indices.begin(), grid.indices.end());
	indices.insert(indices.end(), sphere.indices.begin(), sphere.indices.end());
	indices.insert(indices.end(), cylinder.indices.begin(), cylinder.indices.end());

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(uint32) * totalIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = &indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_shapeIB.GetAddressOf()));
}

void CubeMapApp::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());

    m_sky.reset(new Sky(m_dxDevice.Get(), "Textures/grasscube1024.dds", 5000.0f));

	HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/floor.dds", 0, 0, m_floorTexSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/stone.dds", 0, 0, m_stoneTexSRV.GetAddressOf(), 0 ));

    HR(D3DX11CreateShaderResourceViewFromFile(m_dxDevice.Get(), 
        "Textures/bricks.dds", 0, 0, m_brickTexSRV.GetAddressOf(), 0 ));
}

void CubeMapApp::InitVertexLayout() { }

void CubeMapApp::UpdateScene(float dt)
{
    TopicApp::UpdateScene(dt);

	// Switch the number of lights
	if( GetAsyncKeyState('1') & 0x8000 )
        m_lightCount = 1;

	if( GetAsyncKeyState('2') & 0x8000 )
		m_lightCount = 2;

	if( GetAsyncKeyState('3') & 0x8000 )
		m_lightCount = 3;
}

void CubeMapApp::DrawScene()
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
    Effects::BasicFX->SetCubeMap(m_sky->getCubeMapSRV());

    // Skull doesn't have texture coordinates, so we can't texture it.
	ID3DX11EffectTechnique* activeTexTech = nullptr;
	ID3DX11EffectTechnique* activeSkullTech= nullptr;
	ID3DX11EffectTechnique* activeReflectTech = nullptr;

    switch(m_lightCount)
	{
    case 1:
        activeTexTech = Effects::BasicFX->Light1TexTech;
		activeSkullTech = Effects::BasicFX->Light1ReflectTech;
		activeReflectTech = Effects::BasicFX->Light1ReflectTech;
		break;
	case 2:
		activeTexTech = Effects::BasicFX->Light2TexTech;
		activeSkullTech = Effects::BasicFX->Light2ReflectTech;
		activeReflectTech = Effects::BasicFX->Light2ReflectTech;
		break;
	case 3:
		activeTexTech = Effects::BasicFX->Light3TexTech;
		activeSkullTech = Effects::BasicFX->Light3ReflectTech;
		activeReflectTech = Effects::BasicFX->Light3ReflectTech;
		break;
	}

    XMMATRIX world;
	XMMATRIX worldInvTranspose;
	XMMATRIX worldViewProj;
 
    // Draw the grid, cylinders, and box without any cubemap reflection.
    D3DX11_TECHNIQUE_DESC techDesc;
    activeTexTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeTexTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_shapeVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_shapeIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        // Draw the grid
        world = XMLoadFloat4x4(&m_gridWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*view*proj;
		
		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetTexTransform(XMMatrixScaling(6.0f, 8.0f, 1.0f));
		Effects::BasicFX->SetMaterial(m_gridMat);
        Effects::BasicFX->SetDiffuseMap(m_floorTexSRV.Get());

        pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_gridIndexCount, m_gridIndexOffset, m_gridVertexOffset);

        //Draw the box
        world = XMLoadFloat4x4(&m_boxWorld);
		worldInvTranspose = MathHelper::InverseTranspose(world);
		worldViewProj = world*view*proj;

		Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
		Effects::BasicFX->SetMaterial(m_boxMat);
        Effects::BasicFX->SetDiffuseMap(m_stoneTexSRV.Get());

        activeTexTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_boxIndexCount, m_boxIndexOffset, m_boxVertexOffset);

        // Draw the cylinders.
		for(int i = 0; i < 10; ++i)
		{
            world = XMLoadFloat4x4(&m_cylWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world*view*proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
            Effects::BasicFX->SetMaterial(m_cylinderMat);
            Effects::BasicFX->SetDiffuseMap(m_brickTexSRV.Get());

            pass->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(m_cylinderIndexCount, m_cylinderIndexOffset, m_cylinderVertexOffset);
		}
    }

  	// Draw the spheres with cubemap reflection (reflect technique activate reflection).
	activeReflectTech->GetDesc( &techDesc );
	for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
		// Draw the spheres.
		for(int i = 0; i < 10; ++i)
		{
			world = XMLoadFloat4x4(&m_sphereWorld[i]);
			worldInvTranspose = MathHelper::InverseTranspose(world);
			worldViewProj = world*view*proj;

			Effects::BasicFX->SetWorld(world);
			Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
			Effects::BasicFX->SetWorldViewProj(worldViewProj);
			Effects::BasicFX->SetTexTransform(XMMatrixIdentity());
			Effects::BasicFX->SetMaterial(m_sphereMat);
            Effects::BasicFX->SetDiffuseMap(m_stoneTexSRV.Get());

            activeReflectTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
			m_dxImmediateContext->DrawIndexed(m_sphereIndexCount, m_sphereIndexOffset, m_sphereVertexOffset);
		}
	}

    // Draw the skull
    activeSkullTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        ID3DX11EffectPass* pass = activeSkullTech->GetPassByIndex( p );

        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_skullVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        world = XMLoadFloat4x4(&m_skullWorld);
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world*viewProj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_skullMat);

        pass->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_skullIndexCount, 0, 0);
    }

    // Draw the sky
    m_sky->Draw(m_dxImmediateContext.Get(), m_cam);

    // restore default states, as the SkyFX changes them in the effect file.
	m_dxImmediateContext->RSSetState(0);
	m_dxImmediateContext->OMSetDepthStencilState(0, 0);

	HR(m_swapChain->Present(0, 0));
}

