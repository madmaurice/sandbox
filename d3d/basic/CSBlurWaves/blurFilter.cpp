
#include "blurFilter.h"
#include "effects.h"

BlurFilter::BlurFilter()
: m_blurredOutputTexSRV(nullptr)
, m_blurredOutputTexUAV(nullptr)
{
}

BlurFilter::~BlurFilter()
{
}

ID3D11ShaderResourceView* BlurFilter::GetBlurredOutput()
{
    return m_blurredOutputTexSRV.Get();
}

void BlurFilter::SetGaussianWeights(float sigma)
{
	float d = 2.0f*sigma*sigma;

	float weights[9];
	float sum = 0.0f;
	for(int i = 0; i < 8; ++i)
	{
		float x = (float)i;
		weights[i] = expf(-x*x/d);

		sum += weights[i];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for(int i = 0; i < 8; ++i)
	{
		weights[i] /= sum;
	}

	Effects::BlurFX->SetWeights(weights);
}

void BlurFilter::SetWeights(const float weights[9])
{
	Effects::BlurFX->SetWeights(weights);
}

void BlurFilter::Init(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	// Start fresh.
    m_blurredOutputTexSRV = nullptr;
	m_blurredOutputTexUAV = nullptr;

	m_width = width;
	m_height = height;
	m_format = format;

	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

    // Bind the resource as SRV and UAV
	D3D11_TEXTURE2D_DESC blurredTexDesc;
	blurredTexDesc.Width     = width;
	blurredTexDesc.Height    = height;
    blurredTexDesc.MipLevels = 1;
    blurredTexDesc.ArraySize = 1;
	blurredTexDesc.Format    = format;
	blurredTexDesc.SampleDesc.Count   = 1;
	blurredTexDesc.SampleDesc.Quality = 0;
    blurredTexDesc.Usage     = D3D11_USAGE_DEFAULT;
    blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    blurredTexDesc.CPUAccessFlags = 0;
    blurredTexDesc.MiscFlags      = 0;

	// Views save a reference to the texture so we can release our reference.
	ComPtr<ID3D11Texture2D> blurredTex = 0;
    HR(device->CreateTexture2D(&blurredTexDesc, 0, blurredTex.GetAddressOf()));

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = format;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
    HR(device->CreateShaderResourceView(blurredTex.Get(), &srvDesc, m_blurredOutputTexSRV.GetAddressOf()));

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
    HR(device->CreateUnorderedAccessView(blurredTex.Get(), &uavDesc, m_blurredOutputTexUAV.GetAddressOf()));
}

void BlurFilter::BlurInPlace(ID3D11DeviceContext* dc, 
							 ID3D11ShaderResourceView* inputSRV, 
	                         ID3D11UnorderedAccessView* inputUAV,
							 int blurCount)
{

    // Blur overview
    // We have 2 textures (A and B) and each texture has a SRV and a UAV.
    // Since this is a Gaussian blur it is separable.
    // 1- Bind SRV to A as input to the CS (for horizontal blur)
    // 2- Bind UAV to B as output to the CS.
    // 3- Dispatch thread group to perform horizontal blur. After this texture B store the horizontal blur.
    // 4- Bind SRV to B as input to the CS (for horizontal blur)
    // 5- Bind UAV to A as output to the CS.
    // 6- Dispatch thread group to perform vertical blur. After this texture A store the final blur.
    // In this case texture A is the texture received as input. 
    // Note: inputSRV and inputUAV both refer to the same texture (A).

	// Run the compute shader to blur the offscreen texture.
	for(int i = 0; i < blurCount; ++i)
	{
        // Since it's a Gaussian blur it can be seperated in 2 pass of 1D blur (horizontal and vertical).
        // This decrease the number of samples needed (texture fetch)
		// HORIZONTAL blur pass.
		D3DX11_TECHNIQUE_DESC techDesc;
		Effects::BlurFX->HorzBlurTech->GetDesc( &techDesc );
		for(uint32 p = 0; p < techDesc.Passes; ++p)
		{
			Effects::BlurFX->SetInputMap(inputSRV);
			Effects::BlurFX->SetOutputMap(m_blurredOutputTexUAV.Get());
			Effects::BlurFX->HorzBlurTech->GetPassByIndex(p)->Apply(0, dc);

			// How many groups do we need to dispatch to cover a row of pixels, where each
			// group covers 256 pixels (the 256 is defined in the ComputeShader).
            // There might be some extraneous threads but we take care of that in the shader with clamping check.
			uint32 numGroupsX = (uint32)ceilf(m_width / 256.0f);
			dc->Dispatch(numGroupsX, m_height, 1);
		}
	
		// Unbind the input texture from the CS for good housekeeping.
		ID3D11ShaderResourceView* nullSRV[1] = { 0 };
		dc->CSSetShaderResources( 0, 1, nullSRV );

		// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
		// and a resource cannot be both an output and input at the same time.
		ID3D11UnorderedAccessView* nullUAV[1] = { 0 };
		dc->CSSetUnorderedAccessViews( 0, 1, nullUAV, 0 );
	
		// VERTICAL blur pass.
		Effects::BlurFX->VertBlurTech->GetDesc( &techDesc );
		for(uint32 p = 0; p < techDesc.Passes; ++p)
		{
            //Blurred output was unbind from the CS we bind it again as SRV for the 2nd pass
            Effects::BlurFX->SetInputMap(m_blurredOutputTexSRV.Get());
			Effects::BlurFX->SetOutputMap(inputUAV);
			Effects::BlurFX->VertBlurTech->GetPassByIndex(p)->Apply(0, dc);

			// How many groups do we need to dispatch to cover a column of pixels, where each
			// group covers 256 pixels  (the 256 is defined in the ComputeShader).
			uint32 numGroupsY = (uint32)ceilf(m_height / 256.0f);
			dc->Dispatch(m_width, numGroupsY, 1);
		}
	
		dc->CSSetShaderResources( 0, 1, nullSRV );
		dc->CSSetUnorderedAccessViews( 0, 1, nullUAV, 0 );
	}

	// Disable compute shader.
	dc->CSSetShader(0, 0, 0);
}
