cbuffer worldMatrix : register(b0)
{
	matrix world;
}

cbuffer viewMatrix : register(b1)
{
	matrix view;
}
cbuffer projectionMatrix : register(b2)
{
	matrix projection;
}

struct VS_IN
{
	float3 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
	float3 Normal : NORMAL;
};

struct VS_OUT
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD;
	float3 Normal : NORMAL;
	float4 WorldPos : POSITION;
};

VS_OUT main(VS_IN input)
{
	VS_OUT output = (VS_OUT)0;

	output.Pos = mul(float4(input.Pos, 1.0f), mul(world, mul(view, projection)));
	//output.Pos = mul(float4(input.Pos, 1.0f), mul(world, view));
	output.Tex = input.Tex;
	output.Normal = mul(float4(input.Normal, 0.0f), world).xyz;
	output.WorldPos = mul (float4(input.Pos, 1.0f), world);

	return output;
}