#include "dxUtil.h"
#include "comPtr.h"

ID3D11ShaderResourceView* dxHelper::CreateTexture2DArraySRV(ID3D11Device* device, ID3D11DeviceContext* context,
		std::vector<std::string>& filenames,
		DXGI_FORMAT format,
		uint32 filter, 
		uint32 mipFilter)
{
    // 1- Create each texture from an individual file
    // 2- Create texture array
    // 3- Copy each individual texture into the elements of the texture array
    // 4- Create shader resource view of the texture array

	// Load the texture elements individually from file.  These textures
	// won't be used by the GPU (0 bind flags), they are just used to 
	// load the image data from file.  We use the STAGING usage so the
	// CPU can read the resource.
	uint32 size = filenames.size();

	std::vector<ComPtr<ID3D11Texture2D> > srcTex(size);
	for(uint32 i = 0; i < size; ++i)
	{
		D3DX11_IMAGE_LOAD_INFO loadInfo;

        loadInfo.Width  = D3DX11_FROM_FILE;
        loadInfo.Height = D3DX11_FROM_FILE;
        loadInfo.Depth  = D3DX11_FROM_FILE;
        loadInfo.FirstMipLevel = 0;
        loadInfo.MipLevels = D3DX11_FROM_FILE;
        loadInfo.Usage = D3D11_USAGE_STAGING;
        loadInfo.BindFlags = 0;
        loadInfo.CpuAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
        loadInfo.MiscFlags = 0;
        loadInfo.Format = format;
        loadInfo.Filter = filter;
        loadInfo.MipFilter = mipFilter;
		loadInfo.pSrcInfo  = 0;

        HR(D3DX11CreateTextureFromFile(device, filenames[i].c_str(), 
            &loadInfo, 0, (ID3D11Resource**)srcTex[i].GetAddressOf(), 0));
	}

	// Create the texture array.  Each element in the texture 
	// array has the same format/dimensions.
	D3D11_TEXTURE2D_DESC texElementDesc;
	srcTex[0]->GetDesc(&texElementDesc);

	D3D11_TEXTURE2D_DESC texArrayDesc;
	texArrayDesc.Width              = texElementDesc.Width;
	texArrayDesc.Height             = texElementDesc.Height;
	texArrayDesc.MipLevels          = texElementDesc.MipLevels;
	texArrayDesc.ArraySize          = size;
	texArrayDesc.Format             = texElementDesc.Format;
	texArrayDesc.SampleDesc.Count   = 1;
	texArrayDesc.SampleDesc.Quality = 0;
	texArrayDesc.Usage              = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags     = 0;
	texArrayDesc.MiscFlags          = 0;

	ComPtr<ID3D11Texture2D> texArray = nullptr;
    HR(device->CreateTexture2D( &texArrayDesc, 0, texArray.GetAddressOf()));

	// Copy individual texture elements into texture array.

	// for each texture element...
	for(uint32 texElement = 0; texElement < size; ++texElement)
	{
		// for each mipmap level...
		for(uint32 mipLevel = 0; mipLevel < texElementDesc.MipLevels; ++mipLevel)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			HR(context->Map(srcTex[texElement].Get(), mipLevel, D3D11_MAP_READ, 0, &mappedTex2D));

            //D3D11CalcSubresource calculate destination subresource to update
            //Formula is : ArraySlice * MipLevels + MipSlice
            //ArraySlice = texElement (column of the subresource)
            //MipSlice = mipLevel (row of the subresource)
            context->UpdateSubresource(texArray.Get(), 
				D3D11CalcSubresource(mipLevel, texElement, texElementDesc.MipLevels), //Calculate destination subresource to update
				0, mappedTex2D.pData, mappedTex2D.RowPitch, mappedTex2D.DepthPitch);

            context->Unmap(srcTex[texElement].Get(), mipLevel);
		}
	}	

	// Create a resource view to the texture array.
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texArrayDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	viewDesc.Texture2DArray.MostDetailedMip = 0;
	viewDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	viewDesc.Texture2DArray.FirstArraySlice = 0;
	viewDesc.Texture2DArray.ArraySize = size;

	ID3D11ShaderResourceView* texArraySRV = 0;
	HR(device->CreateShaderResourceView(texArray.Get(), &viewDesc, &texArraySRV));

	// We only need the resource view.
	return texArraySRV;
}

ID3D11ShaderResourceView* dxHelper::CreateRandomTexture1DSRV(ID3D11Device* device)
{
    //TODO
    return nullptr;
}
