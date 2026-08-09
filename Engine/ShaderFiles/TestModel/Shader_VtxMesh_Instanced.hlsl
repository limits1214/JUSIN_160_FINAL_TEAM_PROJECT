#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D g_DiffuseTexture  : register(t0);
Texture2D g_NormalTexture   : register(t1);
Texture2D g_SMROTexture     : register(t2);
Texture2D g_EmissiveTexture : register(t3);

Texture2D DefaultNoiseTexture : register(t13);
static const float DissolveEdgeWidth = 0.025f;

struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
    float4 iWorld0 : INSTANCE_WORLD0;
    float4 iWorld1 : INSTANCE_WORLD1;
    float4 iWorld2 : INSTANCE_WORLD2;
    float4 iWorld3 : INSTANCE_WORLD3;
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

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out;
    
    float4x4 matWV, matWVP;
    float4x4 matWorld = float4x4(In.iWorld0, In.iWorld1, In.iWorld2, In.iWorld3);
    
    matWV = mul(matWorld, g_matView);
    matWVP = mul(matWV, g_matProj);
    
    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), matWorld));
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), matWorld));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), matWorld));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = mul(float4(In.vPosition, 1.f), matWorld);
    Out.vProjPos = Out.vPosition;
    return Out;
}

struct PS_IN
{
    float4 vPosition    : SV_POSITION;
    float4 vNormal      : NORMAL;
    float4 vTangent     : TANGENT;
    float4 vBinormal    : BINORMAL;
    float2 vTexcoord    : TEXCOORD0;
    float4 vWorldPos    : TEXCOORD1;
    float4 vProjPos     : TEXCOORD2;
};

struct PS_OUT
{
    vector vDiffuse     : SV_TARGET0;
    vector vNormal      : SV_TARGET1;
    vector vSMRO        : SV_TARGET2;
    vector vEmissive    : SV_TARGET3;
};

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT Out;
    
    float4 fDiffuse     = g_DiffuseTexture.Sample(LinearWrap, IN.vTexcoord) * float4(AlbedoColor, ObjectAlpha);
    
    if (fDiffuse.a == 0.0f) discard;
    
    float3 fNormal      = Compute_WorldNormal(g_NormalTexture, IN.vTexcoord, IN.vNormal, IN.vTangent) * NormalIntensity;
    float3 fMRO         = g_SMROTexture.Sample(LinearWrap, IN.vTexcoord);
    
    float fFinalMetallic    = fMRO.r * MetallicIntensity;
    float fFinalRoughness   = fMRO.g * RoughnessIntensity;
    float fFinalAO          = fMRO.b * AmbientIntensity;

    float3 fEmissive = g_EmissiveTexture.Sample(LinearWrap, IN.vTexcoord).r * EmissiveColor * EmissiveIntensity;
    
    float3 fFinalEmissive = Apply_DissolveEffect(DefaultNoiseTexture, fEmissive, IN.vTexcoord, DissolveEdgeWidth);

	Out.vDiffuse	= fDiffuse;
    Out.vNormal     = float4(fNormal * 0.5f + 0.5f, 1.f);
    Out.vSMRO       = float4(fFinalMetallic, fFinalRoughness, fFinalAO, 1.f);
	Out.vEmissive	= float4(fFinalEmissive, 1.f);
    
    return Out;
}
