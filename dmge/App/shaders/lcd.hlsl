//	Textures

Texture2D		g_texture0 : register(t0);
SamplerState	g_sampler0 : register(s0);

namespace s3d
{
	//	VS Output / PS Input
	
	struct PSInput
	{
		float4 position	: SV_POSITION;
		float4 color	: COLOR0;
		float2 uv		: TEXCOORD0;
	};
}

//	Constant Buffer

cbuffer PSConstants2D : register(b0)
{
	float4 g_colorAdd;
	float4 g_sdfParam;
	float4 g_sdfOutlineColor;
	float4 g_sdfShadowColor;
	float4 g_internal;
}

cbuffer RenderSetting : register(b1)
{
    float g_gamma;
    //float g_gridWidth;
}

static const float2 pixelSize = { 1.0 / 168.0, 1.0 / 144.0 };

//	Functions

float4 PS(s3d::PSInput input) : SV_TARGET
{
	float4 texColor = g_texture0.Sample(g_sampler0, input.uv);

	// Gamma
	
    texColor.rgb = pow(abs(texColor.rgb), 1.0 / g_gamma);
	
	// Grid

    //const float2 uv1 = fmod(input.uv.xy, pixelSize);
    //const float gridX = step((1.0 - g_gridWidth) * pixelSize.x, uv1.x);
    //const float gridY = step((1.0 - g_gridWidth) * pixelSize.y, uv1.y);
    //texColor.rgb *= (1 - gridX) * (1 - gridY) * 0.12 + 0.88;
	
	return (texColor * input.color) + g_colorAdd;
}
