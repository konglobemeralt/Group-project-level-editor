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
	float4 colors;
}

Texture2D txDiffuse : register(t0);
SamplerState stSampler : register(s0);

float4 main(VS_OUT input) : SV_TARGET
{
	float3 lightvec = (lightPos - input.WorldPos);
	lightvec = normalize(lightvec);

	float diffuse = saturate(dot(input.Normal, lightvec));
	//float4 textureColor = txDiffuse.Sample(stSampler, input.Tex);
	float4 textureColor = (lightColor * diffuse * colors);
		//float4 textureColor = colors;
		return textureColor;
}

//worldLightpos = normalize(lightPos - input.worldPos);

//		float4 PS_main(VS_OUT input) : SV_Target
//		{
//			float3 lightvec = (float3(0.0f, 0.0f, -2.0f) - input.WorldPos);
//			lightvec = normalize(lightvec);
//			
//			float4 color = float4(1.0f, 1.0f, 1.0f, 1.0f);
//
//			float diffuse = saturate(dot(input.Norm, lightvec));
//
//			float3 s = txDiffuse.Sample(sampAni, input.uv).xyz;
//			float4 outL =  (color * diffuse * float4(s, 1.0f));
//			//return float4(s, 1.0);
//			return outL;
//		}