#include "vertex.h"
#include "effects.h"


const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc::PosNormal[2] = 
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
};

ComPtr<ID3D11InputLayout> InputLayouts::PosNormal = nullptr;

void InputLayouts::InitAll(ID3D11Device* device)
{
	// PosNormal

	D3DX11_PASS_DESC passDesc;
	Effects::BasicFX->Light1Tech->GetPassByIndex(0)->GetDesc(&passDesc);
	HR(device->CreateInputLayout(InputLayoutDesc::PosNormal, 2, passDesc.pIAInputSignature, 
        passDesc.IAInputSignatureSize, PosNormal.GetAddressOf()));
}

void InputLayouts::DestroyAll()
{
}

