#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D g_DiffuseTexture	: register(t0);
Texture2D g_NormalTexture	: register(t1);
Texture2D g_SMROTexture		: register(t2);
Texture2D g_EmissiveTexture : register(t3);

Texture2D DefaultNoiseTexture : register(t13);
static const float DissolveEdgeWidth = 0.025f;

struct GPU_ANIM_INSTANCE_DATA
{
    float4x4 WorldMatrix;
    uint iAnimIndex;
    uint iFlags;
    float fTrackPosition;
    uint RootBoneIndex;
    uint iPrevAnimIndex;
    float fPrevTrackPosition;
    float fBlendWeight;
    uint bBlending;
};



StructuredBuffer<GPU_ANIM_INSTANCE_DATA> g_AnimationInstances : register(t6);
StructuredBuffer<float4x4> g_FinalBoneMatrices : register(t7);
struct GPU_PART_INSTANCE_DATA { float4x4 WorldMatrix; uint iParentInstanceIndex; uint iParentBoneIndex;	bool bAttacth; float bPad; };
StructuredBuffer<GPU_PART_INSTANCE_DATA> g_PartInstances : register(t8);


struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float2 vTexcoord : TEXCOORD0;
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

VS_OUT VSMain(VS_IN In, uint instanceId : SV_InstanceID)
{
    VS_OUT Out;
	const GPU_PART_INSTANCE_DATA part = g_PartInstances[instanceId];
    const uint boneOffset = part.iParentInstanceIndex * 512;
    const float4x4 boneMatrix = g_FinalBoneMatrices[boneOffset + part.iParentBoneIndex];
    const float4x4 worldMatrix = g_AnimationInstances[part.iParentInstanceIndex].WorldMatrix;
    const float4x4 attachmentWorldMatrix = mul(boneMatrix, worldMatrix);

	float4x4 partAttachmentWorldMatrix;
	if (part.bAttacth == true)
	{
		
		partAttachmentWorldMatrix = mul(part.WorldMatrix, attachmentWorldMatrix);
	}
	else
	{
		partAttachmentWorldMatrix = part.WorldMatrix;
	}
    float4 localPos = mul(float4(In.vPosition, 1.f), preTransform);
    float4 vWorldPosition = mul(localPos, partAttachmentWorldMatrix);

    Out.vPosition = mul(vWorldPosition, g_matViewProj);
    Out.vNormal = normalize(mul(float4(In.vNormal, 0.f), partAttachmentWorldMatrix));
    Out.vTangent = normalize(mul(float4(In.vTangent, 0.f), partAttachmentWorldMatrix));
    Out.vBinormal = normalize(mul(float4(In.vBinormal, 0.f), partAttachmentWorldMatrix));
    Out.vTexcoord = In.vTexcoord;
    Out.vWorldPos = vWorldPosition;
    Out.vProjPos = Out.vPosition;
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
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vSMRO : SV_TARGET2;
    float4 vEmissive : SV_TARGET3;
};


PS_OUT PSMain(PS_IN In)
{
    PS_OUT Out;

    float4 fDiffuse = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord) * float4(AlbedoColor, ObjectAlpha);
	if (fDiffuse.a == 0.0f) discard;
	
	float3 fNormal			= Compute_WorldNormal(g_NormalTexture, In.vTexcoord, In.vNormal, In.vTangent) * NormalIntensity;
    float3 fMRO				= g_SMROTexture.Sample(LinearWrap, In.vTexcoord);
    
    float fFinalMetallic	= fMRO.r * MetallicIntensity;
    float fFinalRoughness	= fMRO.g * RoughnessIntensity;
    float fFinalAO			= fMRO.b * AmbientIntensity;
    
    float3 fEmissive = g_EmissiveTexture.Sample(LinearWrap, In.vTexcoord).rgb * EmissiveColor * EmissiveIntensity;
    
	float3 fFinalEmissive = Apply_DissolveEffect(DefaultNoiseTexture, fEmissive, In.vTexcoord, DissolveEdgeWidth);
    
	Out.vDiffuse	= fDiffuse;
    Out.vNormal		= float4(fNormal * 0.5f + 0.5f, 1.f);
    Out.vSMRO		= float4(fFinalMetallic, fFinalRoughness, fFinalAO, 1.f);
    Out.vEmissive	= float4(fEmissive, 1.f);
    
	return Out;
}
