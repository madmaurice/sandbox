//***************************************************************************************
// RenderStates.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "RenderStates.h"

ComPtr<ID3D11RasterizerState> RenderStates::WireframeRS = nullptr;
ComPtr<ID3D11RasterizerState> RenderStates::NoCullRS    = nullptr;
	 
ComPtr<ID3D11BlendState>      RenderStates::AlphaToCoverageBS = nullptr;
ComPtr<ID3D11BlendState>      RenderStates::TransparentBS     = nullptr;

void RenderStates::InitAll(ID3D11Device* device)
{
	// WireframeRS
	D3D11_RASTERIZER_DESC wireframeDesc;
	ZeroMemory(&wireframeDesc, sizeof(D3D11_RASTERIZER_DESC));
	wireframeDesc.FillMode = D3D11_FILL_WIREFRAME;
	wireframeDesc.CullMode = D3D11_CULL_BACK;
	wireframeDesc.FrontCounterClockwise = false;
	wireframeDesc.DepthClipEnable = true;

    HR(device->CreateRasterizerState(&wireframeDesc, WireframeRS.GetAddressOf()));

	// NoCullRS
    // Disable backface culling (useful to render transparent objects)
	D3D11_RASTERIZER_DESC noCullDesc;
	ZeroMemory(&noCullDesc, sizeof(D3D11_RASTERIZER_DESC));
	noCullDesc.FillMode = D3D11_FILL_SOLID;
	noCullDesc.CullMode = D3D11_CULL_NONE;
	noCullDesc.FrontCounterClockwise = false;
	noCullDesc.DepthClipEnable = true;

    HR(device->CreateRasterizerState(&noCullDesc, NoCullRS.GetAddressOf()));

	// AlphaToCoverageBS
	D3D11_BLEND_DESC alphaToCoverageDesc = {0};
	alphaToCoverageDesc.AlphaToCoverageEnable = true;
	alphaToCoverageDesc.IndependentBlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].BlendEnable = false;
	alphaToCoverageDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HR(device->CreateBlendState(&alphaToCoverageDesc, AlphaToCoverageBS.GetAddressOf()));

	// TransparentBS
	D3D11_BLEND_DESC transparentDesc = {0};
	transparentDesc.AlphaToCoverageEnable = false;
	transparentDesc.IndependentBlendEnable = false;

	transparentDesc.RenderTarget[0].BlendEnable = true;
	transparentDesc.RenderTarget[0].SrcBlend       = D3D11_BLEND_SRC_ALPHA; // F = (as, as, as)
	transparentDesc.RenderTarget[0].DestBlend      = D3D11_BLEND_INV_SRC_ALPHA; // F = (1-as, 1-as, 1-as) 
	transparentDesc.RenderTarget[0].BlendOp        = D3D11_BLEND_OP_ADD;
	transparentDesc.RenderTarget[0].SrcBlendAlpha  = D3D11_BLEND_ONE; // Falpha = 1
	transparentDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO; // Falpha = 0
	transparentDesc.RenderTarget[0].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
    transparentDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    HR(device->CreateBlendState(&transparentDesc, TransparentBS.GetAddressOf()));
}

void RenderStates::DestroyAll()
{
}