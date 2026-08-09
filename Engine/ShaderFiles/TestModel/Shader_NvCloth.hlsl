#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SMROTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);
ByteAddressBuffer g_NvClothParticles : register(t9);

Texture2D DefaultNoiseTexture : register(t13);
static const float DissolveEdgeWidth = 0.025f;

struct VS_IN
{
    float2 vTexcoord : TEXCOORD0;
    uint3 vParticleIndices : PARTICLE_INDICES;
    float3 vBarycentric : CLOTH_BARYCENTRIC;
    float3 vPositionOffset : CLOTH_POSITION_OFFSET;
    float3 vNormalFrame : CLOTH_NORMAL_FRAME;
    float4 vTangentFrame : CLOTH_TANGENT_FRAME;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

float3 LoadNvClothParticle(uint iParticleIndex)
{
    return asfloat(
        g_NvClothParticles.Load4(
            iParticleIndex * 16u)).xyz;
}

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    const float3 p0 =
        LoadNvClothParticle(In.vParticleIndices.x);
    const float3 p1 =
        LoadNvClothParticle(In.vParticleIndices.y);
    const float3 p2 =
        LoadNvClothParticle(In.vParticleIndices.z);

    const float3 vEdge0 = p1 - p0;
    const float3 vEdge1 = p2 - p0;
    const float3 vFrameTangent =
        vEdge0 * rsqrt(max(dot(vEdge0, vEdge0), 1.e-12f));
    const float3 vRawNormal =
        cross(vEdge0, vEdge1);
    const float3 vFrameNormal =
        vRawNormal *
        rsqrt(max(dot(vRawNormal, vRawNormal), 1.e-12f));
    const float3 vRawBinormal =
        cross(vFrameNormal, vFrameTangent);
    const float3 vFrameBinormal =
        vRawBinormal *
        rsqrt(max(dot(vRawBinormal, vRawBinormal), 1.e-12f));

    const float3 vSurfacePosition =
        p0 * In.vBarycentric.x +
        p1 * In.vBarycentric.y +
        p2 * In.vBarycentric.z;
    const float3 vClothPosition =
        vSurfacePosition +
        vFrameTangent * In.vPositionOffset.x +
        vFrameBinormal * In.vPositionOffset.y +
        vFrameNormal * In.vPositionOffset.z;

    const float3 vClothNormal = normalize(
        vFrameTangent * In.vNormalFrame.x +
        vFrameBinormal * In.vNormalFrame.y +
        vFrameNormal * In.vNormalFrame.z);
    float3 vClothTangent =
        vFrameTangent * In.vTangentFrame.x +
        vFrameBinormal * In.vTangentFrame.y +
        vFrameNormal * In.vTangentFrame.z;
    vClothTangent = normalize(
        vClothTangent -
        vClothNormal *
        dot(vClothNormal, vClothTangent));
    const float3 vClothBinormal =
        normalize(cross(vClothNormal, vClothTangent)) *
        In.vTangentFrame.w;

    const float4 vWorldPosition =
        mul(float4(vClothPosition, 1.f), g_matWorld);
    Out.vPosition = mul(vWorldPosition, mul(g_matView, g_matProj));
    Out.vNormal = normalize(
        mul(float4(vClothNormal, 0.f), g_matWorld));
    Out.vTangent = normalize(
        mul(float4(vClothTangent, 0.f), g_matWorld));
    Out.vBinormal = normalize(
        mul(float4(vClothBinormal, 0.f), g_matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorldPosition;
    Out.vProjPos = Out.vPosition;
    return Out;
}
/*----------- 광윤 수정 -----------*/
struct VS_SHADOW_OUT
{
    float4 vPosition : SV_POSITION;
    float4 vWorldPos : POSITION;
	float2 vTexcoord : TEXCOORD0;
};
/*---------------------------------*/
VS_SHADOW_OUT VSShadow(VS_IN In)
{
    VS_SHADOW_OUT Out;
    const float3 p0 =
        LoadNvClothParticle(In.vParticleIndices.x);
    const float3 p1 =
        LoadNvClothParticle(In.vParticleIndices.y);
    const float3 p2 =
        LoadNvClothParticle(In.vParticleIndices.z);

    const float3 vEdge0 = p1 - p0;
    const float3 vEdge1 = p2 - p0;
    const float3 vFrameTangent =
        vEdge0 * rsqrt(max(dot(vEdge0, vEdge0), 1.e-12f));
    const float3 vRawNormal =
        cross(vEdge0, vEdge1);
    const float3 vFrameNormal =
        vRawNormal *
        rsqrt(max(dot(vRawNormal, vRawNormal), 1.e-12f));
    const float3 vRawBinormal =
        cross(vFrameNormal, vFrameTangent);
    const float3 vFrameBinormal =
        vRawBinormal *
        rsqrt(max(dot(vRawBinormal, vRawBinormal), 1.e-12f));

    const float3 vSurfacePosition =
        p0 * In.vBarycentric.x +
        p1 * In.vBarycentric.y +
        p2 * In.vBarycentric.z;
    const float3 vClothPosition =
        vSurfacePosition +
        vFrameTangent * In.vPositionOffset.x +
        vFrameBinormal * In.vPositionOffset.y +
        vFrameNormal * In.vPositionOffset.z;

    Out.vWorldPos = mul(
        float4(vClothPosition, 1.f),
        g_matWorld);
    Out.vPosition = mul(
        Out.vWorldPos,
        mul(g_matView, g_matProj));
	/*----------- 광윤 수정 -----------*/
	Out.vTexcoord = In.vTexcoord;
	/*---------------------------------*/
    return Out;
}
/*----------- 광윤 수정 -----------*/
//struct VS_POINT_SHADOW_OUT
//{
//    float4 vWorldPos : POSITION;
//};

struct VS_POINT_SHADOW_OUT
{
	float4 Position : SV_POSITION;
	float3 WorldPos : POSITION;
	float2 vTexcoord : TEXCOORD0;
};
/*---------------------------------*/
VS_POINT_SHADOW_OUT VSPointShadow(VS_IN In)
{
    VS_POINT_SHADOW_OUT Out;
    const float3 p0 =
        LoadNvClothParticle(In.vParticleIndices.x);
    const float3 p1 =
        LoadNvClothParticle(In.vParticleIndices.y);
    const float3 p2 =
        LoadNvClothParticle(In.vParticleIndices.z);

    const float3 vEdge0 = p1 - p0;
    const float3 vEdge1 = p2 - p0;
    const float3 vFrameTangent =
        vEdge0 * rsqrt(max(dot(vEdge0, vEdge0), 1.e-12f));
    const float3 vRawNormal =
        cross(vEdge0, vEdge1);
    const float3 vFrameNormal =
        vRawNormal *
        rsqrt(max(dot(vRawNormal, vRawNormal), 1.e-12f));
    const float3 vRawBinormal =
        cross(vFrameNormal, vFrameTangent);
    const float3 vFrameBinormal =
        vRawBinormal *
        rsqrt(max(dot(vRawBinormal, vRawBinormal), 1.e-12f));

    const float3 vSurfacePosition =
        p0 * In.vBarycentric.x +
        p1 * In.vBarycentric.y +
        p2 * In.vBarycentric.z;
    const float3 vClothPosition =
        vSurfacePosition +
        vFrameTangent * In.vPositionOffset.x +
        vFrameBinormal * In.vPositionOffset.y +
        vFrameNormal * In.vPositionOffset.z;

	const float4 LocalPosition = float4(vClothPosition, 1.f);

	const float4 WorldPosition = mul(LocalPosition, g_matWorld);
	
	/*----------- 광윤 수정 -----------*/
	Out.Position = mul(LocalPosition, g_matWVP);
	Out.WorldPos = WorldPosition.xyz;
	Out.vTexcoord = In.vTexcoord;
    //Out.vWorldPos = mul(
    //    float4(vClothPosition, 1.f),
    //    g_matWorld);
	/*---------------------------------*/
	
    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float4 vNormal : NORMAL;
    float4 vTangent : TANGENT;
    float4 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 vWorldPos : TEXCOORD1;
    float4 vProjPos : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse : SV_TARGET0;
    vector vNormal : SV_TARGET1;
    vector vSMRO : SV_TARGET2;
    vector vEmissive : SV_TARGET3;
};

PS_OUT PSMain(PS_IN In)
{
    PS_OUT Out;
    const float4 fDiffuse =
        g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord) *
        float4(AlbedoColor, ObjectAlpha);
    if (fDiffuse.a == 0.f)
        discard;

    const float3 fNormal = Compute_WorldNormal(
        g_NormalTexture,
        In.vTexcoord,
        In.vNormal,
        In.vTangent) * NormalIntensity;
    const float3 fMRO =
        g_SMROTexture.Sample(
            LinearWrap,
            In.vTexcoord).rgb;
    const float3 fEmissive =
        g_EmissiveTexture.Sample(LinearWrap, In.vTexcoord).rgb *
        EmissiveColor * EmissiveIntensity;

	Out.vDiffuse = fDiffuse;		// 광윤 추가 -> Alpha값 적용위해서 변경
    Out.vNormal = float4(fNormal * 0.5f + 0.5f, 1.f);
    Out.vSMRO = float4(
        fMRO.r * MetallicIntensity,
        fMRO.g * RoughnessIntensity,
        fMRO.b * AmbientIntensity,
        1.f);
    Out.vEmissive = float4(
        Apply_DissolveEffect(
            DefaultNoiseTexture,
            fEmissive,
            In.vTexcoord,
            DissolveEdgeWidth),
        1.f);
    return Out;
}
