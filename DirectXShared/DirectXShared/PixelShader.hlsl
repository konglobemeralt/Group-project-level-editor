struct VS_OUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
	float3 Normal : NORMAL;
	float3 WorldPos : POSITION;
};

cbuffer Light : register(b0)
{
	float3 lightPos;
	float4 lightColor;
}

cbuffer Color : register(b1)
{
	int4 texExist;
	float4 colors;
}

Texture2D txDiffuse : register(t0);
SamplerState stSampler : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
	float4 textureColor = float4(1.0, 1.0, 1.0, 1.0);
	float3 lightvec = (lightPos - input.WorldPos);
	lightvec = normalize(lightvec);

	float diffuse = saturate(dot(input.Normal, lightvec));

	if (texExist.x == 1)
	{
		textureColor = txDiffuse.Sample(stSampler, input.Tex) * (lightColor * diffuse);
		//textureColor = txDiffuse.Sample(stSampler, input.Tex);
	}
	else
	{
		textureColor = (lightColor * diffuse * colors);
		//textureColor = colors;
	}
	return textureColor;
}