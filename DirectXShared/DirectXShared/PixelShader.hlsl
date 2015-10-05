struct VS_OUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
	float3 Normal : NORMAL;
};

Texture2D txDiffuse : register(t0);
SamplerState stSampler : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
	//float4 textureColor = txDiffuse.Sample(stSampler, input.Tex);
	float4 textureColor = float4(0.5, 0.5, 0.5, 1.0);
	return textureColor;
}