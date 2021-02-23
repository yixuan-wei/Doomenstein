struct v2f_t
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;
};

struct vs_input_t
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;
};

cbuffer camera_constants : register(b1)
{
    float4x4 PROJECTION; 
    float4x4 VIEW;
    float3 CAM_POSITION;
    //padding 4 bytes after
};

cbuffer object_constants : register(b2)
{
	float4x4 MODEL;
	float4 TINT;
};

Texture2D<float4> tDiffuse : register(t0);
SamplerState sSampler : register(s0);

v2f_t VertexFunction(vs_input_t input)
{
    v2f_t v2f = (v2f_t)0;

    v2f.color = input.color;
    v2f.uv = input.uv;

    float4 modelPos = float4(input.position, 1.0f);
	float4 worldPos = mul(MODEL, modelPos);
    float4 cameraPos = mul(VIEW, worldPos);
    float4 clipPos = mul(PROJECTION, cameraPos);
    v2f.position = clipPos;

    return v2f;
}

//--------------------------------------------------------------------------------------
// Fragment Shader
float4 FragmentFunction( v2f_t input ) : SV_Target0
{    
	return float4(input.uv, 0.f,1.f);
}