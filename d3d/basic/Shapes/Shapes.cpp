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

class ShapesApp : public DemoApp
{
public:
	explicit ShapesApp(HINSTANCE hInstance);
	~ShapesApp();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

    ComPtr<ID3D11Buffer>           m_vertexBuffer;
    ComPtr<ID3D11Buffer>           m_indexBuffer;

    ComPtr<ID3DX11Effect> m_fx;
	ID3DX11EffectTechnique* m_tech; //Not a COM object
	ID3DX11EffectMatrixVariable* m_fxWorldViewProj; //Not a COM object

    ComPtr<ID3D11InputLayout>    m_inputLayout;
    ComPtr<ID3D11RasterizerState> m_wireframeRS;

    // Define transformations from local spaces to world space.
	XMFLOAT4X4 m_sphereWorld[10];
	XMFLOAT4X4 m_cylWorld[10];
	XMFLOAT4X4 m_boxWorld;
	XMFLOAT4X4 m_gridWorld;
	XMFLOAT4X4 m_centerSphere;

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
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	ShapesApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

ShapesApp::ShapesApp(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_vertexBuffer(nullptr)
, m_indexBuffer(nullptr)
, m_fx(nullptr)
, m_tech(nullptr)
, m_fxWorldViewProj(nullptr)
, m_inputLayout(nullptr)
, m_wireframeRS(nullptr)
{
    m_windowCaption = "Shapes Demo";

    m_radius = 15.0f;

	XMMATRIX I = XMMatrixIdentity();
    XMStoreFloat4x4(&m_gridWorld, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);

    XMMATRIX boxScale = XMMatrixScaling(2.0f, 1.0f, 2.0f);
	XMMATRIX boxOffset = XMMatrixTranslation(0.0f, 0.5f, 0.0f);
	XMStoreFloat4x4(&m_boxWorld, XMMatrixMultiply(boxScale, boxOffset));

	XMMATRIX centerSphereScale = XMMatrixScaling(2.0f, 2.0f, 2.0f);
	XMMATRIX centerSphereOffset = XMMatrixTranslation(0.0f, 2.0f, 0.0f);
	XMStoreFloat4x4(&m_centerSphere, XMMatrixMultiply(centerSphereScale, centerSphereOffset));

    for(int i = 0; i < 5; ++i)
	{
		XMStoreFloat4x4(&m_cylWorld[i*2+0], XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_cylWorld[i*2+1], XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i*5.0f));

		XMStoreFloat4x4(&m_sphereWorld[i*2+0], XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i*5.0f));
		XMStoreFloat4x4(&m_sphereWorld[i*2+1], XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i*5.0f));
	}
}

ShapesApp::~ShapesApp()
{
}

bool ShapesApp::Init()
{
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

void ShapesApp::InitGeometryBuffers()
{
    GeometryGenerator::MeshData box;
	GeometryGenerator::MeshData grid;
	GeometryGenerator::MeshData sphere;
	GeometryGenerator::MeshData cylinder;

	GeometryGenerator geoGen;
	geoGen.CreateBox(1.0f, 1.0f, 1.0f, box);
	geoGen.CreateGrid(20.0f, 30.0f, 60, 40, grid);
	geoGen.CreateSphere(0.5f, 20, 20, sphere);
	//geoGen.CreateGeosphere(0.5f, 2, sphere);
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

	std::vector<Vertex> vertices(totalVertexCount);

	XMFLOAT4 black(0.0f, 0.0f, 0.0f, 1.0f);

	UINT k = 0;
	for(size_t i = 0; i < box.vertices.size(); ++i, ++k)
	{
		vertices[k].pos   = box.vertices[i].position;
		vertices[k].color = black;
	}

	for(size_t i = 0; i < grid.vertices.size(); ++i, ++k)
	{
		vertices[k].pos   = grid.vertices[i].position;
		vertices[k].color = black;
	}

	for(size_t i = 0; i < sphere.vertices.size(); ++i, ++k)
	{
		vertices[k].pos   = sphere.vertices[i].position;
		vertices[k].color = black;
	}

	for(size_t i = 0; i < cylinder.vertices.size(); ++i, ++k)
	{
		vertices[k].pos   = cylinder.vertices[i].position;
		vertices[k].color = black;
	}

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * totalVertexCount;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.pSysMem = &vertices[0];
    HR(m_dxDevice->CreateBuffer(&vbd, &vinitData, m_vertexBuffer.GetAddressOf()));

	//
	// Pack the indices of all the meshes into one index buffer.
	//

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
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_indexBuffer.GetAddressOf()));
}

void ShapesApp::InitFX()
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

void ShapesApp::InitVertexLayout()
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

void ShapesApp::UpdateScene(float dt)
{
    DemoApp::UpdateScene(dt);
}

void ShapesApp::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_dxImmediateContext->IASetInputLayout(m_inputLayout.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    //Set wireframe mode
    m_dxImmediateContext->RSSetState(m_wireframeRS.Get());

	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_dxImmediateContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    m_dxImmediateContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	// Set constants
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = view*proj;
 
    D3DX11_TECHNIQUE_DESC techDesc;
    m_tech->GetDesc(&techDesc);
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
		// Draw the grid.
        XMMATRIX world = XMLoadFloat4x4(&m_gridWorld);
        m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>( &(world*viewProj) ));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_gridIndexCount, m_gridIndexOffset, m_gridVertexOffset);

		// Draw the box.
        world = XMLoadFloat4x4(&m_boxWorld);
        m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>( &(world*viewProj) ));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_boxIndexCount, m_boxIndexOffset, m_boxVertexOffset);

		// Draw the center sphere.
        world = XMLoadFloat4x4(&m_centerSphere);
        m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>( &(world*viewProj) ));
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        m_dxImmediateContext->DrawIndexed(m_sphereIndexCount, m_sphereIndexOffset, m_sphereVertexOffset);

        // Draw the cylinders.
		for(int i = 0; i < 10; ++i)
		{
            world = XMLoadFloat4x4(&m_cylWorld[i]);
            m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>( &(world*viewProj) ));
            m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(m_cylinderIndexCount, m_cylinderIndexOffset, m_cylinderVertexOffset);
		}

		// Draw the spheres.
		for(int i = 0; i < 10; ++i)
		{
            world = XMLoadFloat4x4(&m_sphereWorld[i]);
            m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>( &(world*viewProj) ));
            m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
            m_dxImmediateContext->DrawIndexed(m_sphereIndexCount, m_sphereIndexOffset, m_sphereVertexOffset);
		}
    }

	HR(m_swapChain->Present(0, 0));
}

