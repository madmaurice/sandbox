//***************************************************************************************
// Vertex.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Defines vertex structures and input layouts.
//***************************************************************************************

#ifndef _INCGUARD_VERTEX_H
#define _INCGUARD_VERTEX_H

#include "dxUtil.h"
#include "comPtr.h"

namespace Vertex
{
	struct Basic32
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 Tex;
	};
}

class InputLayoutDesc
{
public:
	// Init like const int A::a[4] = {0, 1, 2, 3}; in .cpp file.
	static const D3D11_INPUT_ELEMENT_DESC Basic32[3];
};

class InputLayouts
{
public:
	static void InitAll(ID3D11Device* device);
	static void DestroyAll();

    static ComPtr<ID3D11InputLayout> Basic32;
};

#endif // VERTEX_H
