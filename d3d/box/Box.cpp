//---------------------------------------------------------------------------------------
//
// Draw a 3d box
//
//---------------------------------------------------------------------------------------

#include "dxApp.h"
#include "config.h"
#include "d3dx11Effect.h"
#include "mathHelper.h"
#include <d3dcompiler.h>
#include <iostream>
#include <sstream>

struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT4 color;
};

class BoxApp : public DXApp
{
public:
	explicit BoxApp(HINSTANCE hInstance);
	~BoxApp();

	virtual bool Init();
	virtual void OnResize();
	virtual void UpdateScene(float dt);
	virtual void DrawScene(); 

    virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);

private:

    void InitGeometryBuffers();
	void InitFX();
	void InitVertexLayout();

    ComPtr<ID3D11Buffer>           m_vertexBuffer;
    ComPtr<ID3D11Buffer>           m_indexBuffer;

    ComPtr<ID3DX11Effect> m_fx;
	ID3DX11EffectTechnique* m_tech; //Not a COM object
	ID3DX11EffectMatrixVariable* m_fxWorldViewProj; //Not a COM object

    ComPtr<ID3D11InputLayout>      m_inputLayout;

    XMFLOAT4X4 m_world;
	XMFLOAT4X4 m_view;
	XMFLOAT4X4 m_proj;

    float m_theta;
	float m_phi;
	float m_radius;

	POINT m_lastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
				   PSTR cmdLine, int showCmd)
{
    //Configure debugging stuff
    OC_DBG_CONFIG();

	BoxApp theApp(hInstance);
	
	if(!theApp.Init())
		return 0;
	
	return theApp.Run();
}

BoxApp::BoxApp(HINSTANCE hInstance)
: DXApp(hInstance) 
, m_vertexBuffer(nullptr)
, m_indexBuffer(nullptr)
, m_fx(nullptr)
, m_tech(nullptr)
, m_fxWorldViewProj(nullptr)
, m_inputLayout(nullptr)
, m_theta(1.5f*MathHelper::Pi)
, m_phi(0.25f*MathHelper::Pi)
, m_radius(5.0f)
{
    m_windowCaption = "Box Demo";
	
	m_lastMousePos.x = 0;
	m_lastMousePos.y = 0;

	XMMATRIX I = XMMatrixIdentity();
	XMStoreFloat4x4(&m_world, I);
	XMStoreFloat4x4(&m_view, I);
	XMStoreFloat4x4(&m_proj, I);
}

BoxApp::~BoxApp()
{
}

bool BoxApp::Init()
{
	if(!DXApp::Init())
		return false;

    // Better to init d3d stuff at the beginning (buffer...)
    // Device call are expensive, specially for creating stuff
	InitGeometryBuffers();
	InitFX();
	InitVertexLayout();

	return true;
}

void BoxApp::InitGeometryBuffers()
{
    // Create vertex buffer
    Vertex vertices[] =
    {
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), (const float*)&oc::Colors::White   },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), (const float*)&oc::Colors::Black   },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), (const float*)&oc::Colors::Red     },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), (const float*)&oc::Colors::Green   },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), (const float*)&oc::Colors::Blue    },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), (const float*)&oc::Colors::Yellow  },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), (const float*)&oc::Colors::Cyan    },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), (const float*)&oc::Colors::Magenta }
    };


    D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * 8;
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
	bd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA initData;
	ZeroMemory( &initData, sizeof(initData) );
    initData.pSysMem = vertices;
    HR(m_dxDevice->CreateBuffer( &bd, &initData, m_vertexBuffer.GetAddressOf()));

    // Create the index buffer
	uint32 indices[] = {
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3, 
		4, 3, 7
	};

	D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = sizeof(uint32) * 36;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;
    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.pSysMem = indices;
    HR(m_dxDevice->CreateBuffer(&ibd, &iinitData, m_indexBuffer.GetAddressOf()));
}

void BoxApp::InitFX()
{
	DWORD shaderFlags = 0;
#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3D10_SHADER_DEBUG;
	shaderFlags |= D3D10_SHADER_SKIP_OPTIMIZATION;
#endif
 
	ComPtr<ID3D10Blob> compiledShader(nullptr);
	ComPtr<ID3D10Blob> compilationMsgs(nullptr);
	HRESULT hr = D3DX11CompileFromFile("color.fx", 0, 0, 0, "fx_5_0", shaderFlags, 
        0, 0, compiledShader.GetAddressOf(), compilationMsgs.GetAddressOf(), 0);

	// compilationMsgs can store errors or warnings.
    if( compilationMsgs.Get() != nullptr )
	{
		MessageBoxA(0, (char*)compilationMsgs->GetBufferPointer(), 0, 0);
	}

	// Even if there are no compilationMsgs, check to make sure there were no other errors.
	if(FAILED(hr))
	{
		DXTrace(__FILE__, (DWORD)__LINE__, hr, "D3DX11CompileFromFile", true);
	}

	HR(D3DX11CreateEffectFromMemory(compiledShader->GetBufferPointer(), compiledShader->GetBufferSize(), 
        0, m_dxDevice.Get(), m_fx.GetAddressOf()));

	m_tech    = m_fx->GetTechniqueByName("ColorTech");
	m_fxWorldViewProj = m_fx->GetVariableByName("gWorldViewProj")->AsMatrix();
}

void BoxApp::InitVertexLayout()
{
    // Create the vertex input layout.
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Create the input layout
    // A vertex shader is associated with each pass, which give us the input signature from the pass.
    D3DX11_PASS_DESC passDesc;
    m_tech->GetPassByIndex(0)->GetDesc(&passDesc);
    HR(m_dxDevice->CreateInputLayout(vertexDesc, 2, passDesc.pIAInputSignature, 
        passDesc.IAInputSignatureSize, m_inputLayout.GetAddressOf()));
}

void BoxApp::OnResize()
{
	DXApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, GetAspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, P);
}

// Overload the  "<<" operators so that we can use cout to 
// output XMVECTOR objects.
std::ostream& operator<<(std::ostream& os, FXMVECTOR v)
{
	XMFLOAT4 dest;
	XMStoreFloat4(&dest, v);

	os << "(" << dest.x << ", " << dest.y << ", " << dest.z <<  ", " << dest.w << ")";
	return os;
}

void BoxApp::UpdateScene(float dt)
{
	// Build the view matrix.
    XMVECTOR pos    = MathHelper::ConvertSphericalToCartesian(m_radius, m_theta, m_phi);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up     = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    //Fix the up vector when the camera is "upside down"
    if (m_phi >= MathHelper::Pi && m_phi <= MathHelper::Pi *2)
	    up     = XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f);

    std::stringstream ss;
    ss << "Look at : "<< pos << std::endl;
    ss << "theta : "<< m_theta << std::endl;
    ss << "phi : "<< m_phi << std::endl;
    OutputDebugString(ss.str().c_str());
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);
}

void BoxApp::DrawScene()
{
    OC_ASSERT(m_dxImmediateContext.Get());
	OC_ASSERT(m_swapChain.Get());

    m_dxImmediateContext->ClearRenderTargetView(m_renderTargetView.Get(), reinterpret_cast<const float*>(&oc::Colors::LightSteelBlue));
    m_dxImmediateContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH|D3D11_CLEAR_STENCIL, 1.0f, 0);

    //Set vertex input layout and primitive topology
    m_dxImmediateContext->IASetInputLayout(m_inputLayout.Get());
    m_dxImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

  	UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_dxImmediateContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

    //The only formats allowed for index buffer data are 16-bit (DXGI_FORMAT_R16_UINT) and 32-bit (DXGI_FORMAT_R32_UINT) integers.
    m_dxImmediateContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // Set constants
	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX view  = XMLoadFloat4x4(&m_view);
	XMMATRIX proj  = XMLoadFloat4x4(&m_proj);
	XMMATRIX worldViewProj = world*view*proj;

	m_fxWorldViewProj->SetMatrix(reinterpret_cast<float*>(&worldViewProj));

    D3DX11_TECHNIQUE_DESC techDesc;
    m_tech->GetDesc( &techDesc );
    for(UINT p = 0; p < techDesc.Passes; ++p)
    {
        // Set the state contained in a pass to the device.
        m_tech->GetPassByIndex(p)->Apply(0, m_dxImmediateContext.Get());
        
		// 36 indices for the box.
		m_dxImmediateContext->DrawIndexed(36, 0, 0);
    }

    HR(m_swapChain->Present(0, 0));
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

    SetCapture(m_hMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if( (btnState & MK_LBUTTON) != 0 )
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_lastMousePos.x));
		float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_lastMousePos.y));

		// Update angles based on input to orbit camera around box.
		m_theta += dx;
		m_phi   += dy;

		// Restrict the angle m_phi.
		//m_phi = MathHelper::Clamp(m_phi, 0.1f, MathHelper::Pi-0.1f);

        // Retrict m_phi between 0 and 360 deg
        if (m_phi < 0.0f)
            m_phi += MathHelper::Pi * 2.0f;

        if (m_phi > MathHelper::Pi * 2.0f)
            m_phi -= MathHelper::Pi * 2.0f;
	}
	else if( (btnState & MK_RBUTTON) != 0 )
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f*static_cast<float>(x - m_lastMousePos.x);
		float dy = 0.005f*static_cast<float>(y - m_lastMousePos.y);

		// Update the camera radius based on input.
		m_radius += dx - dy;

		// Restrict the radius.
		m_radius = MathHelper::Clamp(m_radius, 3.0f, 15.0f);
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}