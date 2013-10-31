//***************************************************************************************
// Sky.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Simple class that renders a sky using a cube map.
//***************************************************************************************

#ifndef SKY_H
#define SKY_H

#include "dxUtil.h"
#include "comPtr.h"

class Camera;

class Sky
{
public:
	Sky(ID3D11Device* device, const std::string& cubemapFilename, float skySphereRadius);
	~Sky();

	ID3D11ShaderResourceView* getCubeMapSRV();

	void Draw(ID3D11DeviceContext* dc, const Camera& camera);

private:
	Sky(const Sky& rhs);
	Sky& operator=(const Sky& rhs);

	ComPtr<ID3D11Buffer> m_vb;
	ComPtr<ID3D11Buffer> m_ib;

	ComPtr<ID3D11ShaderResourceView> m_cubeMapSRV;

	uint32 m_indexCount;
};

#endif // SKY_H