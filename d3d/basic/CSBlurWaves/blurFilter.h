// Performs a blur operation on the topmost mip level of an input texture.

#ifndef _INCGUARD_BLURFILTER_H
#define _INCGUARD_BLURFILTER_H

#include <Windows.h>
#include <xnamath.h>
#include "dxUtil.h"
#include "comPtr.h"

class BlurFilter
{
public:
	BlurFilter();
	~BlurFilter();

	ID3D11ShaderResourceView* GetBlurredOutput();

	// Generate Gaussian blur weights.
	void SetGaussianWeights(float sigma);

	// Manually specify blur weights.
	void SetWeights(const float weights[9]);

	/// The width and height should match the dimensions of the input texture to blur.
	/// It is OK to call Init() again to reinitialize the blur filter with a different 
	/// dimension or format.
	void Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format);

	/// Blurs the input texture blurCount times.  Note that this modifies the input texture, not a copy of it.
	void BlurInPlace(ID3D11DeviceContext* dc, ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, int blurCount);

private:

	uint32 m_width;
	uint32 m_height;
	DXGI_FORMAT m_format;

	ComPtr<ID3D11ShaderResourceView> m_blurredOutputTexSRV;
	ComPtr<ID3D11UnorderedAccessView> m_blurredOutputTexUAV;
};

#endif