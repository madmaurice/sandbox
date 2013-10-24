//***************************************************************************************
// RenderStates.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines render state objects.  
//***************************************************************************************

#ifndef _INCGUARD_RENDERSTATES_H
#define _INCGUARD_RENDERSTATES_H

#include "dxUtil.h"
#include "comPtr.h"

class RenderStates
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

	static ComPtr<ID3D11RasterizerState> WireframeRS;
	static ComPtr<ID3D11RasterizerState> NoCullRS;
    static ComPtr<ID3D11RasterizerState> CullClockwiseRS;

    //Blend states
	static ComPtr<ID3D11BlendState> AlphaToCoverageBS;
	static ComPtr<ID3D11BlendState> TransparentBS;
    static ComPtr<ID3D11BlendState> NoRenderTargetWritesBS;

    // Depth/stencil states
	static ComPtr<ID3D11DepthStencilState> MarkMirrorDSS;
	static ComPtr<ID3D11DepthStencilState> DrawReflectionDSS;
	static ComPtr<ID3D11DepthStencilState> NoDoubleBlendDSS;
    static ComPtr<ID3D11DepthStencilState> LessEqualDSS;
};

#endif // RENDERSTATES_H