
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
void VS( float4 Pos : POSITION, float4 Color : COLOR,
         out float4 oPos : SV_POSITION, out float4 oColor : COLOR)
{
    oPos = Pos;
    oColor = Color;
}


//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 PS( float4 Pos : SV_POSITION, float4 Color : COLOR ) : SV_Target
{
    //return float4( 1.0f, 1.0f, 0.0f, 1.0f );    // Yellow, with Alpha = 1
    return Color;
}
