//---------------------------------------------------------------------------------------
//
// Utility stuff for DirectX
//
//---------------------------------------------------------------------------------------

#ifndef _INCGUARD_DXUTIL_H
#define _INCGUARD_DXUTIL_H

#include <d3dx11.h>
#include <xnamath.h>
#include <dxerr.h>
#include <vector>
#include "types.h"

#if defined(DEBUG) || defined(_DEBUG)
	#ifndef HR
	#define HR(x)                                              \
	{                                                          \
		HRESULT hr = (x);                                      \
		if(FAILED(hr))                                         \
		{                                                      \
			DXTrace(__FILE__, (DWORD)__LINE__, hr, #x, true); \
            __debugbreak();                                   \
		}                                                     \
	}
	#endif

#else
	#ifndef HR
	#define HR(x) (x)
	#endif
#endif 

//---------------------------------------------------------------------------------------
// Utility classes.
//---------------------------------------------------------------------------------------

class dxHelper
{
public:
	///<summary>
	/// 
	/// Does not work with compressed formats.
	///</summary>
	static ID3D11ShaderResourceView* CreateTexture2DArraySRV( ID3D11Device* device, ID3D11DeviceContext* context, 
        std::vector<std::string>& filenames, 
        DXGI_FORMAT format = DXGI_FORMAT_FROM_FILE, 
        uint32 filter = D3DX11_FILTER_NONE, 
        uint32 mipFilter = D3DX11_FILTER_LINEAR);

	static ID3D11ShaderResourceView* CreateRandomTexture1DSRV(ID3D11Device* device);
};

namespace oc
{
    namespace Colors
    {
        XMGLOBALCONST XMVECTORF32 IndianRed = { 0.804f, 0.361f, 0.361f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightCoral = { 0.941f, 0.502f, 0.502f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Salmon = { 0.980f, 0.502f, 0.447f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkSalmon = { 0.914f, 0.588f, 0.478f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightSalmon = { 1.0f, 0.627f, 0.478f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Crimson = { 0.863f, 0.078f, 0.235f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
        XMGLOBALCONST XMVECTORF32 FireBrick = { 0.698f, 0.133f, 0.133f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkRed = { 0.545f, 0.0f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Pink = {1.0f, 0.753f, 0.796f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightPink = {1.0f, 0.714f, 0.747f, 1.0f};
        XMGLOBALCONST XMVECTORF32 HotPink = {1.0f, 0.412f, 0.706f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DeepPink = {1.0f, 0.078f, 0.576f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumVioletRed = {0.780f, 0.082f, 0.522f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PaleVioletRed = {0.859f, 0.439f, 0.576f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Coral = {1.0f, 0.627f, 0.478f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Tomato = {1.0f, 0.388f, 0.278f, 1.0f};
        XMGLOBALCONST XMVECTORF32 OrangeRed = {1.0f, 0.271f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkOrange = {1.0f, 0.549f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Orange = {1.0f, 0.647f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Gold = {1.0f, 0.843f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Yellow = {1.0f, 1.0f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightYellow = {1.0f, 1.0f, 0.878f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LemonChion = {0.980f, 0.980f, 0.804f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightGoldenRodYellow = {0.980f, 0.980f, 0.824f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PapayaWhip = {1.0f, 0.937f, 0.835f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Moccasin = {1.0f, 0.894f, 0.710f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PeachPu = {1.0f, 0.855f, 0.725f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PaleGoldenRod = {0.933f, 0.910f, 0.667f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Khaki = {0.941f, 0.902f, 0.549f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkKhaki = {0.741f, 0.718f, 0.420f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Lavender = {0.902f, 0.902f, 0.980f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Thistle = {0.847f, 0.749f, 0.847f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Plum = {0.867f, 0.627f, 0.867f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Violet = {0.933f, 0.510f, 0.933f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Orchid = {0.655f, 0.439f, 0.839f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumOrchid = {0.729f, 0.333f, 0.827f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumPurple = {0.576f, 0.439f, 0.859f, 1.0f};
        XMGLOBALCONST XMVECTORF32 BlueViolet = {0.541f, 0.169f, 0.886f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkViolet = {0.580f, 0.0f, 0.827f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkOrchid = {0.600f, 0.196f, 0.800f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkMagenta = {0.545f, 0.0f, 0.545f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Purple = {0.502f, 0.0f, 0.502f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Indigo = {0.294f, 0.0f, 0.510f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SlateBlue = {0.416f, 0.353f, 0.804f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkSlateBlue = {0.282f, 0.239f, 0.545f, 1.0f};
        XMGLOBALCONST XMVECTORF32 GreenYellow = {0.678f, 1.0f, 0.184f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Chartreuse = {0.498f, 1.0f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LawnGreen = {0.486f, 0.988f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Lime = { 0.0f, 1.0f, 0.0f, 1.0f };
        XMGLOBALCONST XMVECTORF32 LimeGreen = {0.196f, 0.804f, 0.196f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PaleGreen = {0.596f, 0.984f, 0.596f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightGreen = {0.565f, 0.933f, 0.565f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumSpringGreen = {0.0f, 0.980f, 0.604f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SpringGreen = {0.0f, 1.0f, 0.498f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumSeaGreen = {0.235f, 0.702f, 0.443f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SeaGreen = {0.180f, 0.545f, 0.341f, 1.0f};
        XMGLOBALCONST XMVECTORF32 ForestGreen = {0.133f, 0.545f, 0.133f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Green = {0.0f, 0.502f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkGreen = {0.0f, 0.392f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 YellowGreen = {0.604f, 0.804f, 0.196f, 1.0f};
        XMGLOBALCONST XMVECTORF32 OliveDrab = {0.420f, 0.557f, 0.137f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Olive = {0.502f, 0.502f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkOliveGreen = {0.333f, 0.420f, 0.184f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumAquamarine = {0.400f, 0.804f, 0.667f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkSeaGreen = {0.561f, 0.737f, 0.561f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightSeaGreen = {0.125f, 0.698f, 0.667f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkCyan = {0.0f, 0.545f, 0.545f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Teal = {0.0f, 0.502f, 0.502f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Aqua = {0.0f, 1.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightCyan = {0.878f, 1.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PaleTurquoise = {0.686f, 0.933f, 0.933f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Aquamarine = {0.498f, 1.0f, 0.831f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Turquoise = {0.251f, 0.878f, 0.816f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumTurquoise = {0.282f, 0.820f, 0.800f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkTurquoise = {0.0f, 0.808f, 0.820f, 1.0f};
        XMGLOBALCONST XMVECTORF32 CadetBlue = {0.373f, 0.620f, 0.627f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SteelBlue = {0.275f, 0.510f, 0.706f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightSteelBlue = {0.690f, 0.769f, 0.871f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightBlue = {0.678f, 0.847f, 0.902f, 1.0f};
        XMGLOBALCONST XMVECTORF32 PowderBlue = {0.690f, 0.878f, 0.902f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SkyBlue = {0.529f, 0.808f, 0.922f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightSkyBlue = {0.529f, 0.808f, 0.980f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DeepSkyBlue = {0.0f, 0.749f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DodgerBlue = {0.118f, 0.565f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 CornflowerBlue = {0.392f, 0.584f, 0.929f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumSlateBlue = {0.482f, 0.408f, 0.933f, 1.0f};
        XMGLOBALCONST XMVECTORF32 RoyalBlue = {0.255f, 0.412f, 0.882f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Blue = {0.0f, 0.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MediumBlue = {0.0f, 0.0f, 0.804f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkBlue = {0.0f, 0.0f, 0.545f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Navy = {0.0f, 0.0f, 0.502f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MidnightBlue = {0.098f, 0.098f, 0.439f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Cornsilk = {1.0f, 0.973f, 0.863f, 1.0f};
        XMGLOBALCONST XMVECTORF32 BlanchedAlmond = {1.0f, 0.922f, 0.804f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Bisque = {1.0f, 0.894f, 0.769f, 1.0f};
        XMGLOBALCONST XMVECTORF32 NavajoWhite = {1.0f, 0.871f, 0.678f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Wheat = {0.961f, 0.871f, 0.702f, 1.0f};
        XMGLOBALCONST XMVECTORF32 BurlyWood = {0.871f, 0.722f, 0.529f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Tan = {0.824f, 0.706f, 0.549f, 1.0f};
        XMGLOBALCONST XMVECTORF32 RosyBrown = {0.737f, 0.561f, 0.561f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SandyBrown = {0.957f, 0.643f, 0.376f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Goldenrod = {0.855f, 0.647f, 0.125f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkGoldenrod = {0.722f, 0.525f, 0.043f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Peru = {0.804f, 0.522f, 0.247f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Chocolate = {0.824f, 0.412f, 0.118f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SaddleBrown = {0.545f, 0.271f, 0.075f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Sienna = {0.627f, 0.322f, 0.176f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Brown = {0.647f, 0.165f, 0.165f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Maroon = {0.502f, 0.0f, 0.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 White = { 1.0f, 1.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Snow = {1.0f, 0.980f, 0.980f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Honeydew = {0.941f, 1.0f, 0.941f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MintCream = {0.961f, 1.0f, 0.980f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Azure = {0.941f, 1.0f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 AliceBlue = {0.941f, 0.973f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 GhostWhite = {0.973f, 0.973f, 1.0f, 1.0f};
        XMGLOBALCONST XMVECTORF32 WhiteSmoke = {0.961f, 0.961f, 0.961f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SeaShell = {1.0f, 0.961f, 0.933f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Beige = {0.961f, 0.961f, 0.863f, 1.0f};
        XMGLOBALCONST XMVECTORF32 OldLace = {0.992f, 0.961f, 0.902f, 1.0f};
        XMGLOBALCONST XMVECTORF32 FloralWhite = {1.0f, 0.980f, 0.941f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Ivory = {1.0f, 1.0f, 0.941f, 1.0f};
        XMGLOBALCONST XMVECTORF32 AntiqueWhite = {0.980f, 0.922f, 0.843f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Linen = {0.980f, 0.941f, 0.902f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LavenderBlush = {1.0f, 0.941f, 0.961f, 1.0f};
        XMGLOBALCONST XMVECTORF32 MistyRose = {1.0f, 0.894f, 0.882f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Gainsboro = {0.863f, 0.863f, 0.863f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightGrey = {0.827f, 0.827f, 0.827f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Silver = {0.753f, 0.753f, 0.753f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkGrey = {0.663f, 0.663f, 0.633f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Gray = {0.502f, 0.502f, 0.502f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DimGray = {0.412f, 0.412f, 0.412f, 1.0f};
        XMGLOBALCONST XMVECTORF32 LightSlateGray = {0.467f, 0.533f, 0.600f, 1.0f};
        XMGLOBALCONST XMVECTORF32 SlateGray = { 0.439f, 0.502f, 0.565f, 1.0f};
        XMGLOBALCONST XMVECTORF32 DarkSlateGray = {0.184f, 0.310f, 0.310f, 1.0f};
        XMGLOBALCONST XMVECTORF32 Black = { 0.0f, 0.0f, 0.0f, 1.0f};
    }
}

#endif // _INCGUARD_DXUTIL_H