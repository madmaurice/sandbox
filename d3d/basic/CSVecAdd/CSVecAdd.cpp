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
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>

struct Data
{
    XMFLOAT3 v1;
    XMFLOAT2 v2;
};

class CSVecAdd : public DemoApp
{
public:
	explicit CSVecAdd(HINSTANCE hInstance);
	~CSVecAdd();

	virtual bool Init();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

    void DoComputeWork();

private:

    virtual void InitGeometryBuffers();
	virtual void InitFX();
	virtual void InitVertexLayout();

	void BuildBuffersAndViews();

    ComPtr<ID3D11Buffer>           m_outputBuffer;
    ComPtr<ID3D11Buffer>           m_outputDebugBuffer;

	ComPtr<ID3D11ShaderResourceView> m_inputASRV;
	ComPtr<ID3D11ShaderResourceView> m_inputBSRV;
	ComPtr<ID3D11UnorderedAccessView> m_outputUAV;

	int32 m_numElements;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	CSVecAdd theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
    theApp.DoComputeWork();

	return 0;
}

CSVecAdd::CSVecAdd(HINSTANCE hInstance)
: DemoApp(hInstance) 
, m_outputBuffer(nullptr)
, m_outputDebugBuffer(nullptr)
, m_inputASRV(nullptr)
, m_inputBSRV(nullptr)
, m_outputUAV(nullptr)
, m_numElements(32)
{
    m_windowCaption = "CS Vector Add Demo";
}

CSVecAdd::~CSVecAdd()
{
    m_dxImmediateContext->ClearState();

    Effects::DestroyAll();
	InputLayouts::DestroyAll();
	RenderStates::DestroyAll();
}

bool CSVecAdd::Init()
{
    if(!DemoApp::Init())
		return false;

	return true;
}

void CSVecAdd::InitGeometryBuffers()
{
    BuildBuffersAndViews();
}

void CSVecAdd::InitFX()
{
    // Must init Effects first since InputLayouts depend on shader signatures.
	Effects::InitAll(m_dxDevice.Get());
	InputLayouts::InitAll(m_dxDevice.Get());
	RenderStates::InitAll(m_dxDevice.Get());
}

void CSVecAdd::BuildBuffersAndViews()
{
    //Initialize the input vectors data
    std::vector<Data> dataA(m_numElements);
	std::vector<Data> dataB(m_numElements);
	for(int i = 0; i < m_numElements; ++i)
	{
		dataA[i].v1 = XMFLOAT3(i, i, i);
		dataA[i].v2 = XMFLOAT2(i, 0);

		dataB[i].v1 = XMFLOAT3(-i, i, 0.0f);
		dataB[i].v2 = XMFLOAT2(0, -i);
	}

    // Inputs
    // Create a buffer to be bound as a shader input (D3D11_BIND_SHADER_RESOURCE).
	D3D11_BUFFER_DESC inputDesc;
    inputDesc.Usage = D3D11_USAGE_DEFAULT;
    inputDesc.ByteWidth = sizeof(Data) * m_numElements;
    inputDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    inputDesc.CPUAccessFlags = 0;
	inputDesc.StructureByteStride = sizeof(Data);
    inputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; //Structured buffer

    D3D11_SUBRESOURCE_DATA vinitDataA;
    vinitDataA.pSysMem = &dataA[0];

	ComPtr<ID3D11Buffer> bufferA = 0;
    HR(m_dxDevice->CreateBuffer(&inputDesc, &vinitDataA, bufferA.GetAddressOf()));

	D3D11_SUBRESOURCE_DATA vinitDataB;
    vinitDataB.pSysMem = &dataB[0];

	ComPtr<ID3D11Buffer> bufferB = 0;
    HR(m_dxDevice->CreateBuffer(&inputDesc, &vinitDataB, bufferB.GetAddressOf()));

    // Output
	// Create a read-write buffer the compute shader can write to (D3D11_BIND_UNORDERED_ACCESS).
	D3D11_BUFFER_DESC outputDesc;
    outputDesc.Usage = D3D11_USAGE_DEFAULT;
    outputDesc.ByteWidth = sizeof(Data) * m_numElements;
    outputDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS; //RW buffer
    outputDesc.CPUAccessFlags = 0;
	outputDesc.StructureByteStride = sizeof(Data);
    outputDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED; //Structured buffer

    HR(m_dxDevice->CreateBuffer(&outputDesc, 0, m_outputBuffer.GetAddressOf()));

    // The buffer for the copy of the results need to have STAGING usage and read access
    // Staging usage mean a resource that supports data transfer (copy) from the GPU to the CPU.
    // Create a system memory version of the buffer to read the results back from.
	outputDesc.Usage = D3D11_USAGE_STAGING; 
	outputDesc.BindFlags = 0;
	outputDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    HR(m_dxDevice->CreateBuffer(&outputDesc, 0, m_outputDebugBuffer.GetAddressOf()));


    //For SRV and UAV to a strucutured buffer the Format is unknown because there is no
    //DXGI format that correspond to our user defined type (Data in this case).
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX; //Could use D3D11_SRV_DIMENSION_BUFFER instead
	srvDesc.BufferEx.FirstElement = 0;
	srvDesc.BufferEx.Flags = 0;
	srvDesc.BufferEx.NumElements = m_numElements;

    //The SRV keep a reference on the buffer which mean we don't need the ID3D11Buffer anymore
    m_dxDevice->CreateShaderResourceView(bufferA.Get(), &srvDesc, m_inputASRV.GetAddressOf());
    m_dxDevice->CreateShaderResourceView(bufferB.Get(), &srvDesc, m_inputBSRV.GetAddressOf());


	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = m_numElements;

    m_dxDevice->CreateUnorderedAccessView(m_outputBuffer.Get(), &uavDesc, m_outputUAV.GetAddressOf());
}

void CSVecAdd::DoComputeWork()
{
    D3DX11_TECHNIQUE_DESC techDesc;

    Effects::VecAddFX->SetInputA(m_inputASRV.Get());
	Effects::VecAddFX->SetInputB(m_inputBSRV.Get());
    Effects::VecAddFX->SetOutput(m_outputUAV.Get());

    //Launch the compute shader
	Effects::VecAddFX->VecAddTech->GetDesc( &techDesc );
	for(UINT p = 0; p < techDesc.Passes; ++p)
	{
		ID3DX11EffectPass* pass = Effects::VecAddFX->VecAddTech->GetPassByIndex(p);
        pass->Apply(0, m_dxImmediateContext.Get());

		m_dxImmediateContext->Dispatch(1, 1, 1);
	}

    // Unbind the input textures from the CS for good housekeeping.
    // They are not needed anymore
	ID3D11ShaderResourceView* nullSRV[1] = { 0 };
    m_dxImmediateContext->CSSetShaderResources( 0, 1, nullSRV );

	// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
	// and a resource cannot be both an output and input at the same time.
	ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
	m_dxImmediateContext->CSSetUnorderedAccessViews( 0, 1, nullUAV, 0 );

	// Disable compute shader.
	m_dxImmediateContext->CSSetShader(0, 0, 0);

	std::ofstream fout("results.txt");

    // Copy the output buffer to system memory.
    m_dxImmediateContext->CopyResource(m_outputDebugBuffer.Get(), m_outputBuffer.Get());

	// Map the data for reading.
	D3D11_MAPPED_SUBRESOURCE mappedData; 
    m_dxImmediateContext->Map(m_outputDebugBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedData);

	Data* dataView = reinterpret_cast<Data*>(mappedData.pData);

	for(int i = 0; i < m_numElements; ++i)
	{
		fout << "(" << dataView[i].v1.x << ", " << dataView[i].v1.y << ", " << dataView[i].v1.z <<
			", " << dataView[i].v2.x << ", " << dataView[i].v2.y << ")" << std::endl;
	}

    m_dxImmediateContext->Unmap(m_outputDebugBuffer.Get(), 0);

	fout.close();
}

void CSVecAdd::InitVertexLayout() {}

void CSVecAdd::UpdateScene(float dt) {} 

void CSVecAdd::DrawScene()
{
    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Silver));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

	HR(m_swapChain->Present(0, 0));
}

