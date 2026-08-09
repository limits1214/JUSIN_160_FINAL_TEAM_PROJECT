#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define MAX_BONE_COUNT 512


struct GPU_ANIM_INSTANCE_DATA
{
	float4x4 WorldMatrix;

	uint iAnimIndex;
	uint iFlags;
	float fTrackPosition;
	uint iRootBoneIndex;

	uint iPrevAnimIndex;
	float fPrevTrackPosition;
	float fBlendWeight;
	uint bBlending;
};

struct GPU_SKIN_BONE_DESC
{
	float4x4 OffsetMatrix;

	uint iSkeletonBoneIndex;
	uint iPadding0;
	uint iPadding1;
	uint iPadding2;
};

struct VS_SHADOW_INSTANCED_IN
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float3 Tangent : TANGENT;
	float3 Binormal : BINORMAL;
	float2 TexCoord : TEXCOORD0;
	uint4 BlendIndices : BLENDINDICES;
	float4 BlendWeights : BLENDWEIGHT;
};

StructuredBuffer<GPU_ANIM_INSTANCE_DATA> gInstances : register(t6);
StructuredBuffer<float4x4> gCPUCombinedBoneMatrices : register(t7);
StructuredBuffer<GPU_SKIN_BONE_DESC> gSkinBones		: register(t8);

cbuffer CB_CPU_SKINNING_MESH : register(b5)
{
	uint gSkinBoneOffset;
	uint gVertexCount;
	uint gSkinBoneCount;
	/*----------- 광윤 추가 -----------*/
	uint iBonePaletteStride;
	/*---------------------------------*/
};

cbuffer CB_SHADOW : register(b11)
{
	uint	CurrentShadowLightIndex;
	uint	CurrentPointFaceIndex;
	uint	CurrentCascadeIndex;
	float	ShadowPadding;
}


float4 Compute_AnimModel_SkinnedPosition(VS_SHADOW_INSTANCED_IN IN, uint _InstancedID)
{
	//const uint InstanceBoneOffset = _InstancedID * 512;
	/*----------- 광윤 추가 -----------*/
	const uint InstanceBoneOffset = _InstancedID * iBonePaletteStride;
	/*---------------------------------*/
	
	float4 Weights = IN.BlendWeights;
	
	Weights.w = 1.0f -
        (Weights.x + Weights.y + Weights.z);

	const GPU_SKIN_BONE_DESC BoneX =
        gSkinBones[gSkinBoneOffset + IN.BlendIndices.x];

	const GPU_SKIN_BONE_DESC BoneY =
        gSkinBones[gSkinBoneOffset + IN.BlendIndices.y];

	const GPU_SKIN_BONE_DESC BoneZ =
        gSkinBones[gSkinBoneOffset + IN.BlendIndices.z];

	const GPU_SKIN_BONE_DESC BoneW =
        gSkinBones[gSkinBoneOffset + IN.BlendIndices.w];

	const float4x4 MatrixX =
        mul(
            BoneX.OffsetMatrix,
            gCPUCombinedBoneMatrices[
                InstanceBoneOffset +
                BoneX.iSkeletonBoneIndex]);

	const float4x4 MatrixY =
        mul(
            BoneY.OffsetMatrix,
            gCPUCombinedBoneMatrices[
                InstanceBoneOffset +
                BoneY.iSkeletonBoneIndex]);

	const float4x4 MatrixZ =
        mul(
            BoneZ.OffsetMatrix,
            gCPUCombinedBoneMatrices[
                InstanceBoneOffset +
                BoneZ.iSkeletonBoneIndex]);

	const float4x4 MatrixW =
        mul(
            BoneW.OffsetMatrix,
            gCPUCombinedBoneMatrices[
                InstanceBoneOffset +
                BoneW.iSkeletonBoneIndex]);

	const float4x4 SkinMatrix =
          MatrixX * Weights.x
        + MatrixY * Weights.y
        + MatrixZ * Weights.z
        + MatrixW * Weights.w;

	return mul(float4(IN.Position, 1.0f), SkinMatrix);
}


struct VS_IN
{
	float3 Position : POSITION;
};

struct VS_POINT_OUT
{
	float4 Position : SV_POSITION;
	float3 WorldPos : TEXCOORD0;
};
struct VS_FINAL_OUT
{
	float4 Position : SV_POSITION;
};

VS_FINAL_OUT VSMain_InstancedDirectional(VS_SHADOW_INSTANCED_IN IN, uint _InstancedID : SV_INSTANCEID)
{
	VS_FINAL_OUT OUT;
	
	const float4 SkinnedPosition = Compute_AnimModel_SkinnedPosition(IN, _InstancedID);
	
	float4 WorldPosition = mul(SkinnedPosition, gInstances[_InstancedID].WorldMatrix);
	
	uint LightViewProjIndex = 0;
	
	if (AffectedLight[CurrentShadowLightIndex].LightType == LIGHT_DIRECTIONAL) {
		LightViewProjIndex = CurrentCascadeIndex;
	}
	
	OUT.Position = mul(WorldPosition, AffectedLight[CurrentShadowLightIndex].g_LightViewProj[LightViewProjIndex]);
	
	return OUT;
}

VS_FINAL_OUT VSMain_Directional(VS_IN IN)
{
	VS_FINAL_OUT OUT;
	
	OUT.Position = mul(float4(IN.Position, 1.f), g_matWVP);
	
	return OUT;
}

VS_POINT_OUT VSMain_PointFace(VS_IN IN)
{
	VS_POINT_OUT OUT;
	
	float4 WorldPos = mul(float4(IN.Position, 1.f), g_matWorld);
	OUT.WorldPos = WorldPos.xyz;
	OUT.Position = mul(WorldPos, AffectedLight[CurrentShadowLightIndex].g_LightViewProj[CurrentPointFaceIndex]);

	return OUT;
}

VS_POINT_OUT VSMain_InstancedPointFace(VS_SHADOW_INSTANCED_IN IN, uint _InstancedID : SV_INSTANCEID)
{
	VS_POINT_OUT OUT;
	
	const float4 SkinnedPosition = Compute_AnimModel_SkinnedPosition(IN, _InstancedID);
	
	float4 WorldPosition = mul(SkinnedPosition, gInstances[_InstancedID].WorldMatrix);
	
	OUT.WorldPos = WorldPosition.xyz;
	OUT.Position = mul(WorldPosition, AffectedLight[CurrentShadowLightIndex].g_LightViewProj[CurrentPointFaceIndex]);
	
	return OUT;
}

float PSMain_Directional(VS_FINAL_OUT OUT) : SV_DEPTH
{
	return OUT.Position.z;
}
float PSMain_PointFace(VS_POINT_OUT OUT) : SV_DEPTH
{
	float3 LightToPixel = OUT.WorldPos - AffectedLight[CurrentShadowLightIndex].Position;
	float  Distance = length(LightToPixel);
	
	float  OuterRange = max(0.02f, AffectedLight[CurrentShadowLightIndex].OuterAttanuation);

	return saturate(Distance / OuterRange);
}
