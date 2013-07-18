//---------------------------------------------------------------------------------------
//
// This init DirectX only
//
//---------------------------------------------------------------------------------------

#include "dxApp.h"
#include "config.h"
#include <d3dcompiler.h>

namespace
{
    HRESULT CompileShaderFromFile(char* szFileName, const char* szEntryPoint, const char* szShaderModel, ID3DBlob** ppBlobOut)
    {
        HRESULT hr = S_OK;

        DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
        // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
        // Setting this flag improves the shader debugging experience, but still allows 
        // the shaders to be optimized and to run exactly the way they will run in 
        // the release configuration of this program.
        dwShaderFlags |= D3DCOMPILE_DEBUG;
    #endif

        ID3DBlob* pErrorBlob;
        hr = D3DX11CompileFromFile( szFileName, NULL, NULL, szEntryPoint, szShaderModel, 
            dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL );

        if( FAILED(hr) )
        {
            if( pErrorBlob != NULL )
                OutputDebugStringA( (char*)pErrorBlob->GetBufferPointer() );
            if( pErrorBlob ) pErrorBlob->Release();
            return hr;
        }
        if( pErrorBlob ) pErrorBlob->Release();

        return S_OK;
    }
}

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

class Triangle
{
public:
    Triangle();
    Vertex m_vertices[3];
};

Triangle::Triangle()
{
    m_vertices[0].pos = XMFLOAT3( 0.0f, 0.5f, 0.5f );
    m_vertices[0].color = (const float*)&oc::Colors::White;
    m_vertices[1].pos = XMFLOAT3( 0.5f, -0.5f, 0.5f );
    m_vertices[1].color = (const float*)&oc::Colors::White;
    m_vertices[2].pos = XMFLOAT3( -0.5f, -0.5f, 0.5f );
    m_vertices[2].color = (const float*)&oc::Colors::White;
}

class Square
{
public:
    Square();
    Vertex m_vertices[4];
};

Square::Square()
{
    m_vertices[0].pos = XMFLOAT3( -0.5f, 0.5f, 0.5f );
    m_vertices[0].color = (const float*)&oc::Colors::Green;
    m_vertices[1].pos = XMFLOAT3( 0.5f, 0.5f, 0.5f );
    m_vertices[1].color = (const float*)&oc::Colors::Cyan;
    m_vertices[2].pos = XMFLOAT3( 0.5f, -0.5f, 0.5f );
    m_vertices[2].color = (const float*)&oc::Colors::Red;
    m_vertices[3].pos = XMFLOAT3( -0.5f, -0.5f, 0.5f );
    m_vertices[3].color = (const float*)&oc::Colors::Yellow;
}
 
class InitDX : public DXApp
{
public:
	explicit InitDX(HINSTANCE hInstance);
	~InitDX();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

private:
    Triangle                       m_triangle;
    Square                         m_square;

    ComPtr<ID3D11Buffer>           m_vertexBuffer;
    ComPtr<ID3D11Buffer>           m_indexBuffer;
    ComPtr<ID3D11InputLayout>      m_inputLayout;
    ComPtr<ID3D11VertexShader>     m_vertexShader;
    ComPtr<ID3D11PixelShader>      m_pixelShader;
};



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	InitDX theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

InitDX::InitDX(HINSTANCE hInstance)
: DXApp(hInstance) 
, m_inputLayout(nullptr)
, m_vertexShader(nullptr)
, m_pixelShader(nullptr)
{
}

InitDX::~InitDX()
{
}

bool InitDX::Init()
{
	if(!DXApp::Init())
		return false;

    // Create vertex buffer
    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    //For triangle
    //bd.ByteWidth = sizeof(Vertex) * 3;
    bd.ByteWidth = sizeof(Vertex) * 4;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory( &initData, sizeof(initData) );
    //initData.pSysMem = m_triangle.m_vertices;
    initData.pSysMem = m_square.m_vertices;
    HR(m_dxDevice->CreateBuffer( &bd, &initData, m_vertexBuffer.GetAddressOf()));


    // Create the indice buffers for the square
    UINT indices[] = {
		0, 1, 2,
		0, 2, 3,
	};

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(UINT) * 6;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = indices;
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_indexBuffer.GetAddressOf()));


    // Compile the vertex shader
    ComPtr<ID3DBlob> pVSBlob = nullptr;
    HR(CompileShaderFromFile("shader.fx", "VS", "vs_4_0", pVSBlob.GetAddressOf()));

	// Create the vertex shader
    HR(m_dxDevice->CreateVertexShader( pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), NULL, m_vertexShader.GetAddressOf()));

	// Defined then create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
     	{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
 
    // Create the input layout to describe the input-buffer data for the input-assembler stage.
    // This map our vertex structure to the corresponding input of the vertex shader
    m_dxDevice->CreateInputLayout(vertexDesc, 2, pVSBlob->GetBufferPointer(),
        pVSBlob->GetBufferSize(), m_inputLayout.GetAddressOf());

    // Compile the pixel shader
	ComPtr<ID3DBlob> pPSBlob = nullptr;
    HR(CompileShaderFromFile("shader.fx", "PS", "ps_4_0", pPSBlob.GetAddressOf()));

    // Create pixel shader
    HR(m_dxDevice->CreatePixelShader( pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), NULL, m_pixelShader.GetAddressOf()));

	return true;
}

void InitDX::OnResize()
{
	DXApp::OnResize();
}

void InitDX::UpdateScene(float dt)
{
    // No scene :(
}

void InitDX::DrawScene()
{
    OC_ASSERT(m_dxImmediateContext.Get());
	OC_ASSERT(m_swapChain.Get());

    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::Blue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_dxImmediateContext->IASetInputLayout(m_inputLayout.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_dxImmediateContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
    //The only formats allowed for index buffer data are 16-bit (DXGI_FORMAT_R16_UINT) and 32-bit (DXGI_FORMAT_R32_UINT) integers.
    m_dxImmediateContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    m_dxImmediateContext->VSSetShader( m_vertexShader.Get(), NULL, 0 );
    m_dxImmediateContext->PSSetShader( m_pixelShader.Get(), NULL, 0 );

    m_dxImmediateContext->DrawIndexed(6, 0, 0); 

    //For triangle
    //m_dxImmediateContext->Draw(3, 0); 

	HR(m_swapChain->Present(0, 0));
}