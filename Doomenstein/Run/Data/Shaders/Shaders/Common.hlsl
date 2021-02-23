static const uint MAX_LIGHTS = 8;

struct light_t
{
	float3 worldPosition; 
    //padding 4 bytes after
	float3 color;
	float intensity;

    float3 direction;
    float directionFactor;

    float3 attenuation;
    float cosInnerAngle;

    float3 specAttenuation;
    float cosOuterAngle;
};

cbuffer time_constants : register(b0)
{
    float SYSTEM_TIME_SECONDS;
    float SYSTEM_TIME_DELTA_SECONDS;

    float GAMMA;
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

cbuffer light_constants : register(b3)
{
	float4 AMBIENT;
	light_t LIGHTS[MAX_LIGHTS];

    float SPECULAR_FACTOR;
    float SPECULAR_POWER;
    float DIFFUSE_FACTOR;
    float EMISSIVE_FACTOR;
    
    float3 FOG_COLOR;
    float FOG_NEAR;
    float FOG_FAR;
};

Texture2D<float4> tDiffuse : register(t0);
Texture2D<float4> tNormal : register(t1);
Texture2D<float4> tSpecular : register(t2);
Texture2D<float4> tEmissive : register(t3);

SamplerState sSampler : register(s0);

struct vs_input_t
{
    float3 position : POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD;

    float4 tangent : TANGENT;
    float3 normal : NORMAL;
};

struct v2f_t
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 uv : UV;

    float3 worldNormal : WORLD_NORMAL;
    float3 worldBitangent : BITANGENT;
    float3 worldTangent : TANGENT;
	float3 worldPosition : WORLD_POSITION;
};

v2f_t VertexFunction(vs_input_t input)
{
    v2f_t v2f = (v2f_t)0;

    v2f.color = input.color * TINT;
    v2f.uv = input.uv;

    //transform model space normal to world
    float4 tangent = float4(input.tangent.xyz, 0.f);
    float4 worldTangent = mul(MODEL, tangent);
    v2f.worldTangent = worldTangent.xyz;

    float4 normal = float4(input.normal,0.f);
    float4 worldNormal = mul(MODEL, normal);
    v2f.worldNormal = worldNormal.xyz;

    v2f.worldBitangent = cross(worldNormal.xyz, worldTangent.xyz) * input.tangent.w;    

    float4 modelPos = float4(input.position, 1.0f);
	float4 worldPos = mul(MODEL, modelPos);
    float4 cameraPos = mul(VIEW, worldPos);
    float4 clipPos = mul(PROJECTION, cameraPos);
    v2f.position = clipPos;
	v2f.worldPosition = worldPos.xyz;

    return v2f;
}
