//---------------------------------------------------------------------------------------
//
// Draw shapes with skull
//
//---------------------------------------------------------------------------------------

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

class LitSkullDemo : public DemoApp
{
public:
	explicit LitSkullDemo(HINSTANCE hInstance);
	~LitSkullDemo();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:
    void BuildShapeGeometryBuffers();
    void BuildSkullGeometryBuffers();

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    ComPtr<ID3D11Buffer>           m_shapesVB;
    ComPtr<ID3D11Buffer>           m_shapesIB;

    ComPtr<ID3D11Buffer>           m_skullVB;
    ComPtr<ID3D11Buffer>           m_skullIB;

    DirectionalLight m_dirLights[3];
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

    int m_boxVertexOffset;
	int m_gridVertexOffset;
	int m_sphereVertexOffset;
	int m_cylinderVertexOffset;

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

	LitSkullDemo theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

LitSkullDemo::LitSkullDemo(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_shapesVB(nullptr)
, m_shapesIB(nullptr)
, m_skullVB(nullptr)
, m_skullIB(nullptr)
, m_skullIndexCount(0)
, m_lightCount(0)
{
    m_windowCaption = "LitSkull Demo";

    m_radius = 15.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_gridWorld, I);
    XMStoreFloat4x4(&m_skullWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

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

    m_dirLights[0].ambient  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLights[0].diffuse  = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLights[0].specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLights[0].direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

	m_dirLights[1].ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLights[1].diffuse  = XMFLOAT4(0.20f, 0.20f, 0.20f, 1.0f);
	m_dirLights[1].specular = XMFLOAT4(0.25f, 0.25f, 0.25f, 1.0f);
	m_dirLights[1].direction = XMFLOAT3(-0.57735f, -0.57735f, 0.57735f);

	m_dirLights[2].ambient  = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLights[2].diffuse  = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLights[2].specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_dirLights[2].direction = XMFLOAT3(0.0f, -0.707f, -0.707f);

	m_gridMat.ambient  = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_gridMat.diffuse  = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_gridMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	m_cylinderMat.ambient  = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	m_cylinderMat.diffuse  = XMFLOAT4(0.7f, 0.85f, 0.7f, 1.0f);
	m_cylinderMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);

	m_sphereMat.ambient  = XMFLOAT4(0.1f, 0.2f, 0.3f, 1.0f);
	m_sphereMat.diffuse  = XMFLOAT4(0.2f, 0.4f, 0.6f, 1.0f);
	m_sphereMat.specular = XMFLOAT4(0.9f, 0.9f, 0.9f, 16.0f);

	m_boxMat.ambient  = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	m_boxMat.diffuse  = XMFLOAT4(0.651f, 0.5f, 0.392f, 1.0f);
	m_boxMat.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);

	m_skullMat.ambient  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_skullMat.diffuse  = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_skullMat.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
}

LitSkullDemo::~LitSkullDemo()
{
    Effects::DestroyAll();
    InputLayouts::DestroyAll();
}

bool LitSkullDemo::Init()
{
    if(!DemoApp::Init())
		return false;

    // Must init Effects first since InputLayouts depend on shader signatures.
    Effects::InitAll(m_dxDevice.Get());
    InputLayouts::InitAll(m_dxDevice.Get());

	return true;
}

void LitSkullDemo::InitGeometryBuffers()
{
    BuildShapeGeometryBuffers();
    BuildSkullGeometryBuffers();
}

void LitSkullDemo::BuildShapeGeometryBuffers()
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
	
	uint32 totalVertexCount = box.vertices.size() + 
		grid.vertices.size() + 
		sphere.vertices.size() +
		cylinder.vertices.size();

	uint32 totalIndexCount = m_boxIndexCount + 
        m_gridIndexCount + 
        m_sphereIndexCount + 
        m_cylinderIndexCount;

	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.

    std::vector<Vertex::PosNormal> vertices(totalVertexCount);

	uint32 k = 0;
	for(size_t i = 0; i < box.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = box.vertices[i].position;
        vertices[k].Normal = box.vertices[i].normal;
	}

	for(size_t i = 0; i < grid.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = grid.vertices[i].position;
        vertices[k].Normal = grid.vertices[i].normal;
	}

	for(size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = sphere.vertices[i].position;
        vertices[k].Normal = sphere.vertices[i].normal;
	}

	for(size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
	{
		vertices[k].Pos   = cylinder.vertices[i].position;
        vertices[k].Normal = cylinder.vertices[i].normal;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex::PosNormal) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_shapesVB.GetAddressOf()));

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
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_shapesIB.GetAddressOf()));
}

void LitSkullDemo::BuildSkullGeometryBuffers()
{
    std::ifstream fin("Models/skull.txt");
    OC_ASSERT(fin.good());
	
	uint32 vcount = 0;
	uint32 tcount = 0;
	std::string ignore;

	fin >> ignore >> vcount; //vertex
	fin >> ignore >> tcount; //triangle
	fin >> ignore >> ignore >> ignore >> ignore;
	
	std::vector<Vertex::PosNormal> vertices(vcount);
	for(uint32 i = 0; i < vcount; ++i)
	{
		fin >> vertices[i].Pos.x >> vertices[i].Pos.y >> vertices[i].Pos.z;
		fin >> vertices[i].Normal.x >> vertices[i].Normal.y >> vertices[i].Normal.z;
	}

	fin >> ignore;
	fin >> ignore;
	fin >> ignore;

	m_skullIndexCount = 3*tcount;
    std::vector<uint32> indices(m_skullIndexCount);
	for(uint32 i = 0; i < tcount; ++i)
	{
		fin >> indices[i*3+0] >> indices[i*3+1] >> indices[i*3+2];
	}

	fin.close();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(Vertex::PosNormal) * vcount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_skullVB.GetAddressOf()));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(uint32) * m_skullIndexCount;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
	iinitData.pSysMem = &indices[0];
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_skullIB.GetAddressOf()));
}

void LitSkullDemo::InitFX()
{
}

void LitSkullDemo::InitVertexLayout()
{
}

void LitSkullDemo::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);

	// Switch the number of lights based on key presses.
	if(GetAsyncKeyState('0') & 0x8000)
		m_lightCount = 0; 

	if(GetAsyncKeyState('1') & 0x8000)
		m_lightCount = 1; 

	if(GetAsyncKeyState('2') & 0x8000)
		m_lightCount = 2; 

	if(GetAsyncKeyState('3') & 0x8000)
		m_lightCount = 3; 
}

void LitSkullDemo::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(InputLayouts::PosNormal.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	uint32 stride = sizeof(Vertex::PosNormal);
    uint32 offset = 0;

    // Set per frame constants.
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = view*proj;

	Effects::BasicFX->SetDirLights(m_dirLights);
	Effects::BasicFX->SetEyePosW(m_camPosition);
 
	// Figure out which technique to use.
	ID3DX11EffectTechnique* activeTech = Effects::BasicFX->Light1Tech;
	switch(m_lightCount)
	{
	case 1:
		activeTech = Effects::BasicFX->Light1Tech;
		break;
	case 2:
		activeTech = Effects::BasicFX->Light2Tech;
		break;
	case 3:
		activeTech = Effects::BasicFX->Light3Tech;
		break;
	}
 
    D3DX11_TECHNIQUE_DESC techDesc;
    activeTech->GetDesc(&techDesc);
    for(uint32 p = 0; p < techDesc.Passes; ++p)
    {
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_shapesVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_shapesIB.Get(), DXGI_FORMAT_R32_UINT, 0);

		// Draw the grid.
        XMMATRIX world = XMLoadFloat4x4(&m_gridWorld);
        XMMATRIX worldInvTranspose = MathHelper::InverseTranspose(world);
        XMMATRIX worldViewProj = world*view*proj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(m_gridMat);

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_gridIndexCount, m_gridIndexOffset, m_gridVertexOffset);

		// Draw the box.
        world = XMLoadFloat4x4(&m_boxWorld);
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world*view*proj;

        Effects::BasicFX->SetWorld(world);
		Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
		Effects::BasicFX->SetWorldViewProj(worldViewProj);
		Effects::BasicFX->SetMaterial(m_boxMat);

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
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
            Effects::BasicFX->SetMaterial(m_cylinderMat);

            activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(m_cylinderIndexCount, m_cylinderIndexOffset, m_cylinderVertexOffset);
		}

		// Draw the spheres.
		for(int i = 0; i < 10; ++i)
		{
            world = XMLoadFloat4x4(&m_sphereWorld[i]);
            worldInvTranspose = MathHelper::InverseTranspose(world);
            worldViewProj = world*view*proj;

            Effects::BasicFX->SetWorld(world);
            Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
            Effects::BasicFX->SetWorldViewProj(worldViewProj);
            Effects::BasicFX->SetMaterial(m_sphereMat);

            activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(m_sphereIndexCount, m_sphereIndexOffset, m_sphereVertexOffset);
		}

        //Draw the skull
        m_dxImmediateContext->IASetVertexBuffers(0, 1, m_skullVB.GetAddressOf(), &stride, &offset);
        m_dxImmediateContext->IASetIndexBuffer(m_skullIB.Get(), DXGI_FORMAT_R32_UINT, 0);

        world = XMLoadFloat4x4(&m_skullWorld);
        worldInvTranspose = MathHelper::InverseTranspose(world);
        worldViewProj = world*view*proj;

        Effects::BasicFX->SetWorld(world);
        Effects::BasicFX->SetWorldInvTranspose(worldInvTranspose);
        Effects::BasicFX->SetWorldViewProj(worldViewProj);
        Effects::BasicFX->SetMaterial(m_skullMat);

        activeTech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_skullIndexCount, 0, 0);
    }

	HR(m_swapChain->Present(0, 0));
}

