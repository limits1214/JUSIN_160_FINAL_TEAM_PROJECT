#include "../ShaderHeader/SH_CommonFunction.hlsli"
											
Texture2D<float>		DepthTexture			: register(t0);
Texture2D<float>		BlueNoiseTexture		: register(t1);
Texture3D<float4>		VolumeTexture			: register(t2);

Texture3D<float>		VoxelDensityColor		: register(t3);
Texture3D<float4>		VoxelLightingColor		: register(t4);

Texture2DArray<float>	ShadowMapArray			: register(t5);

Texture2DArray<float>	DynamicShadowMaps		: register(t10);
TextureCubeArray<float> DynamicShadowCubeMaps	: register(t12);

RWTexture2D<float4>		OUTPUT					: register(u0);
RWTexture3D<float4>		OUTPUT3D				: register(u1);
RWTexture3D<float>		OUTPUT_DENSITY			: register(u2);

const static float2		SceneResolution			= { 1280.f, 720.f };
const static float2		NoiseResolution			= { 256.f, 256.f };

const static uint		GodRayMaxStep			= { 32 };
const static float		GodRayStrength			= { 1.5f };

static const float		SpotVolumetricShadowBias	= { 0.0001f };
static const float		PointVolumetricShadowBias	= { 0.002f };

static const float		LocalScatteringStrength = { 2.f };
static const float FroxelDepthExponent = 1.5f;
static const float2 PCFOffsets[4] =
{
	float2(-0.5f, -0.5f),
    float2(+0.5f, -0.5f),
    float2(-0.5f, +0.5f),
    float2(+0.5f, +0.5f)
};

cbuffer CB_FroxelConfig : register(b10)
{
	float3	FroxelGridSize;
	float	SliceDepthRatio;
	
	float2	FullScreenResolution;
	float2	HalfScreenResolution;
	
	float	NearZ;
	float	FarZ;
	float	AnalyticBlendStart;
	float	AnalyticBlendEnd;
};

cbuffer CB_VLFOG : register(b11)
{
	float3	FogColor;
	float	FogIntensity;
	   
	float3	FogCenterPos;
	float	FogHeight;
	
	float	FogStartPos;
	float	FogEndPos;
	float	FogDensity;
	float	FogNoiseScale;
	
	float3	FogLightDirection;
	float	FogAnisotropyGA;		// 전방 산란도
	
	float3	FogLightColor;
	float	FogAnisotropyGB;		// 후방 산란도
	
	float	FogScatteringWeight;	// 전방/후방 가중치
	
	float	FogBaseHeight;
	float	FogHeightFallOff;
	float	FogPadding;
};

cbuffer CB_CSM : register(b12)
{
	matrix	ShadowViewProj[4];
	float4	CascadeSplits;
	float2	ShadowMapSize;
	float2	ShadowBias;
};

float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far)
{
	float LinearDepth = saturate((_Depth - _Near) / max(_Far - _Near, 0.0001f));
	return pow(LinearDepth,1.f / FroxelDepthExponent);
}

float3 FroxelZToWorldPos(float3 _TexCoord)
{
	float ViewDepth = lerp(NearZ, FarZ, pow(saturate(_TexCoord.z), FroxelDepthExponent));
	
	float2 ScreenSpaceNDC;
	ScreenSpaceNDC.x = _TexCoord.x * +2.f - 1.f;
	ScreenSpaceNDC.y = _TexCoord.y * -2.f + 1.f;
	
	float3 ViewSpacePos;
	ViewSpacePos.x = ScreenSpaceNDC.x * ViewDepth / g_matProj[0][0];
	ViewSpacePos.y = ScreenSpaceNDC.y * ViewDepth / g_matProj[1][1];
	ViewSpacePos.z = ViewDepth;
	
	return mul(float4(ViewSpacePos, 1.f), g_matInvView).xyz;
}

float GetSliceDeltaZ(uint _ZSlice, float _MaxSlice, float _Near, float _Far)
{
	float Z0 = (float) _ZSlice / _MaxSlice;
	float Z1 = ((float) _ZSlice + 1.f) / _MaxSlice;

	float NearSlice = lerp(_Near, _Far, pow(Z0, FroxelDepthExponent));
	float FarSlice  = lerp(_Near, _Far, pow(Z1, FroxelDepthExponent));

	return FarSlice - NearSlice;
}

float Henyey_Greenstein_Phase(float _CosTheta, float _Anistropy)
{
	float Anistropy2 = _Anistropy * _Anistropy;
	float Denum = 1.f + Anistropy2 - 2.f * _Anistropy * _CosTheta;
	
	return (1.f - Anistropy2) / (4.f * PI * pow(max(Denum, 0.0001f), 1.5f)); 
}
float Henyey_Greenstein_DualPhase(float3 _RayDirection, float3 _FogLightDirection, float _FrontAnistropy, float _BackAnistropy, float k)
{
	float CosTheta = dot(_RayDirection, -_FogLightDirection);
	
	float PhaseValueA = Henyey_Greenstein_Phase(CosTheta, _FrontAnistropy);
	float PhaseValueB = Henyey_Greenstein_Phase(CosTheta, _BackAnistropy);
    
	return lerp(PhaseValueB, PhaseValueA, k);
}

float GetVolumeFogDensity(float3 _WorldPos)    
{
	if (FogHeight <= 0.0001f)	return 0.f;
	
	float	FogMaxHeight = max(0.f, _WorldPos.y - FogBaseHeight);

	float	HeightFactor = exp(-FogMaxHeight * FogHeightFallOff);
	
	float	HeightLimit = 1.f - smoothstep(FogHeight * 0.8f, FogHeight, FogMaxHeight);

	float3	NoiseTexCoord = (_WorldPos - FogCenterPos) * FogNoiseScale;
	float4	NoiseSet = VolumeTexture.SampleLevel(LinearWrap, NoiseTexCoord, 0.f);
    
    float	MainNoise	= NoiseSet.r;
    float	SubNoise	= NoiseSet.g * 0.5f + NoiseSet.b * 0.3f + NoiseSet.a * 0.2f;
    float	FinalNoise	= saturate(MainNoise * 0.7f + SubNoise * 0.3f);
	
	return	HeightFactor * FinalNoise * FogDensity * HeightLimit;
}

//float Compute_ShadowBrightness(float4 _Position)
//{
//    // ViewSpace Pos From ShadowCam
//    float4 ShadowSpacePos = mul(_Position, g_matShadowLightViewProj);
//    float2 ShadowMapUV;
//    ShadowMapUV.x = (ShadowSpacePos.x) * +0.5f + 0.5f;
//    ShadowMapUV.y = (ShadowSpacePos.y) * -0.5f + 0.5f;
//            
//    float DepthFromShadowCam = ShadowSpacePos.z;
//            
//    float ShadowBrightness = 1.f; // 최대 밝기 (1.f = 그림자가 안 지는 픽셀의 값)
//    
//    [branch]
//    if (ShadowMapUV.x >= 0.0f && ShadowMapUV.x <= 1.0f && ShadowMapUV.y >= 0.0f && ShadowMapUV.y <= 1.0f)
//    {
//        // Compare Depth (DepthFromShadowCam : ShadowMapTexture Depth)
//        // (DepthFromShadowCam < ShadowMapTexture Depth) : 1 ~ No Shadow
//        // (DepthFromShadowCam > ShadowMapTexture Depth) : 0 ~ Cascade Shadow
//        float ShadowFactor = ShadowMapTexture.SampleCmpLevelZero(ShadowSampler, ShadowMapUV, DepthFromShadowCam + 0.002f);
//
//        //lerp(0.15f, 1.0f, ShadowFactor);
//        ShadowBrightness = pow(ShadowFactor, 3.0f); // ShadowBrightness : 그림자의 밝기(대부분 1.f or 0.f)
//    }
//    return ShadowBrightness;
//}

float Compute_CascadeShadow(float3 _WorldPos)
{
	float4 ViewPos = mul(float4(_WorldPos, 1.f), g_matView);
	float ViewDepth = abs(ViewPos.z);
	
	if (CascadeSplits.w <= 0.f || ViewDepth >= CascadeSplits.w)	return 1.f;
	
	int CascadeIndex = 3;
	if		(ViewDepth < CascadeSplits.x)	CascadeIndex = 0;
	else if (ViewDepth < CascadeSplits.y)	CascadeIndex = 1;
	else if (ViewDepth < CascadeSplits.z)	CascadeIndex = 2;
	else if (ViewDepth < CascadeSplits.w)	CascadeIndex = 3;

	float4 ShadowSpacePos = mul(float4(_WorldPos, 1.f), ShadowViewProj[CascadeIndex]);
	ShadowSpacePos.xyz /= ShadowSpacePos.w;
	
	float2 ShadowMapUV;
	ShadowMapUV.x = ShadowSpacePos.x * +0.5f + 0.5f;
	ShadowMapUV.y = ShadowSpacePos.y * -0.5f + 0.5f;
	
	if (ShadowMapUV.x < 0.f || ShadowMapUV.x > 1.f ||
        ShadowMapUV.y < 0.f || ShadowMapUV.y > 1.f ||
		ShadowSpacePos.z > 1.f || ShadowSpacePos.z < 0.f)
	{
		return 1.f;
	}
	
	float CurrentDepth = ShadowSpacePos.z - ShadowBias.x;
	
	float2 TexelSize = 1.f / max(ShadowMapSize, float2(1.f, 1.f));

	float ShadowFactor = 0.f;
	
	ShadowFactor += ShadowMapArray.SampleCmpLevelZero(
    ShadowSampler,
    float3(ShadowMapUV + TexelSize * float2(-0.5f, -0.5f), CascadeIndex),
    CurrentDepth);

	ShadowFactor += ShadowMapArray.SampleCmpLevelZero(
    ShadowSampler,
    float3(ShadowMapUV + TexelSize * float2(+0.5f, -0.5f), CascadeIndex),
    CurrentDepth);

	ShadowFactor += ShadowMapArray.SampleCmpLevelZero(
    ShadowSampler,
    float3(ShadowMapUV + TexelSize * float2(-0.5f, +0.5f), CascadeIndex),
    CurrentDepth);

	ShadowFactor += ShadowMapArray.SampleCmpLevelZero(
    ShadowSampler,
    float3(ShadowMapUV + TexelSize * float2(+0.5f, +0.5f), CascadeIndex),
    CurrentDepth);
	
	return ShadowFactor * 0.25f;
}

float Compute_PointVolumetricShadow(float3 _WorldPos, uint _LightIndex)
{
	int ShadowSlotNumb = AffectedLight[_LightIndex].ShadowSlot;
	
	if (ShadowSlotNumb < 0 || ShadowSlotNumb >= MAX_SHADOW_LIGHT_COUNT) return 1.f;

	float3	LightToVoxel = _WorldPos - AffectedLight[_LightIndex].Position;
	float	DistanceSQ	 = dot(LightToVoxel, LightToVoxel);
	
	if (DistanceSQ <= 0.0001f) return 1.f;
	
	float OuterRange = max(AffectedLight[_LightIndex].OuterAttanuation, 0.02f);
	
	if (DistanceSQ >= OuterRange * OuterRange)	return 1.f;

	float InvDistance = rsqrt(DistanceSQ);
	float Distance = DistanceSQ * InvDistance;

	float3 SampleDirection = LightToVoxel * InvDistance;
	
	float CompareDepth = saturate(Distance / OuterRange - PointVolumetricShadowBias);
	
	float3 BaseUp = abs(SampleDirection.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);
	
	float3 TangentX = normalize(cross(SampleDirection, BaseUp));
	float3 TangentY = normalize(cross(SampleDirection, TangentX));
	
	float FilterRadius = 2.5f / POINTLIGHT_RESOLUTION;

	float ShadowFactor = 0.f;
	[unroll]
	for (uint i = 0; i < 4; ++i)
	{
		float2 Offset = PCFOffsets[i];

		float3 OffsetDirection = normalize(SampleDirection + TangentX * Offset.x * FilterRadius + TangentY * Offset.y * FilterRadius);
		
		ShadowFactor += DynamicShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(OffsetDirection, ShadowSlotNumb), CompareDepth);
	}

	return ShadowFactor * 0.25f;
	
}
float Compute_SpotVolumetricShadow(float3 _WorldPos, uint _LightIndex)
{
	int ShadowSlotNumb = AffectedLight[_LightIndex].ShadowSlot;
	
	if (ShadowSlotNumb < 0 || ShadowSlotNumb >= MAX_SHADOW_LIGHT_COUNT) return 1.f;

	float4 LightClipPos = mul(float4(_WorldPos, 1.f), AffectedLight[_LightIndex].g_LightViewProj[0]);
	if (LightClipPos.w <= 0.0001f) return 1.f;

	float3 LightNDC = LightClipPos.xyz / LightClipPos.w;
	
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
        LightNDC.y < -1.f || LightNDC.y > 1.f ||
        LightNDC.z < 0.f || LightNDC.z > 1.f) return 1.f;
	
	float2 ShadowUV;
	ShadowUV.x = LightNDC.x * 0.5f + 0.5f;
	ShadowUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float CompareDepth = saturate(LightNDC.z - SpotVolumetricShadowBias);

	float2 TexelSize = 1.f / float2(SPOTLIGHT_RESOLUTION, SPOTLIGHT_RESOLUTION);

	float ShadowFactor = 0.f;
	
	[unroll]
	for (uint i = 0; i < 4; i++)
	{
		float2 SampleUV = ShadowUV + TexelSize * PCFOffsets[i];
		ShadowFactor += DynamicShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, ShadowSlotNumb), CompareDepth);
	}

	return ShadowFactor * 0.25f;
}

float3 Compute_LocalScattering(float3 _WorldPos, float3 _RayDirection, float _Density)
{
	float3 LocalScattering = 0.f;
	
	[loop]
	for (uint i = 0; i < LightCount; ++i)
	{
		DynamicLight Light = AffectedLight[i];
		
		if (Light.LightType == LIGHT_DIRECTIONAL)	continue;
		
		if (Light.VolumetricIntensity <= 0.f)		continue;
		
		float3 L, Radiance;
		
		if (!Compute_DynamicLight(_WorldPos, Light, L, Radiance))	continue;

		float ShadowFactor = 1.f;
		
		if (Light.ShadowSlot >= 0)
		{
			if (Light.LightType == LIGHT_POINT)
			{
				ShadowFactor = Compute_PointVolumetricShadow(_WorldPos, i);
			}
			else
			{
				ShadowFactor = Compute_SpotVolumetricShadow(_WorldPos, i);
			}
		}
		
		float CosTheta = dot(_RayDirection, L);
		float Phase = Henyey_Greenstein_Phase(CosTheta, 0.f); //FogAnisotropyGA);
		
		LocalScattering += FogColor * Radiance * _Density * Phase * ShadowFactor * Light.VolumetricIntensity * LocalScatteringStrength;
	}
	
	return LocalScattering;
}

[numthreads(8, 8, 8)]
void CSMain_CellInjection(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y || ID.z >= (uint) FroxelGridSize.z)	return;
	
	float3	TexCoord = (float3(ID.xyz) + 0.5f) / FroxelGridSize;
	
	float3	VoxelWorldPos = FroxelZToWorldPos(TexCoord);
	float	FogDensity	  = GetVolumeFogDensity(VoxelWorldPos);
	
	float	FogDistance = abs(mul(float4(VoxelWorldPos, 1.f), g_matView).z);
	float	FogDistanceFactor = smoothstep(FogStartPos, max(FogEndPos, FogStartPos + 0.0001f), FogDistance);
	
	FogDensity *= FogDistanceFactor;

	OUTPUT_DENSITY[ID] = FogDensity;
	return;
}

[numthreads(8, 8, 8)]
void CSMain_LightIntegration(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y || ID.z >= (uint) FroxelGridSize.z)	return;
	
	float Density = VoxelDensityColor.Load(int4(ID.xyz, 0));

	if (Density <= 0.0001f)
	{
		OUTPUT3D[ID] = float4(0.f, 0.f, 0.f, 0.f);
		return;
	}
	

	//float2	NoiseUV = (float2(ID.xy + FrameOffset) + 0.5f) / NoiseResolution;
	//float	Jitter = BlueNoiseTexture.SampleLevel(PointWrap, NoiseUV, 0).r;
	//float	JitteredZ = (float) ID.z + 0.5f + (Jitter - 0.5f);
	//JitteredZ = clamp(JitteredZ, 0.5f, FroxelGridSize.z - 0.5f);
	
	//float3 TexCoord;
	//TexCoord.xy = (float2(ID.xy) + 0.5f) / FroxelGridSize.xy;
	//TexCoord.z	= JitteredZ / FroxelGridSize.z;
	float3 TexCoord = (float3(ID.xyz) + 0.5f) / FroxelGridSize;
	
	float3	VoxelWorldPos = FroxelZToWorldPos(TexCoord);
	float3	RayDirection  = normalize(VoxelWorldPos - g_vCamPos);
	
	float	PhaseValue = Henyey_Greenstein_DualPhase(RayDirection, FogLightDirection, FogAnisotropyGA, FogAnisotropyGB, FogScatteringWeight);
	
	float	ShadowBrightness	= Compute_CascadeShadow(VoxelWorldPos);
	
	float3	DirectScattering	= FogColor * FogLightColor * FogIntensity * Density * ShadowBrightness * PhaseValue * GodRayStrength;
	float3	AmbientScattering	= FogColor * Density * 0.002f;
	float3  LocalScattering		= Compute_LocalScattering(VoxelWorldPos, RayDirection, Density);
	
	float3	Scattering = DirectScattering + AmbientScattering + LocalScattering;
	float	Extinction = Density;

	OUTPUT3D[ID] = float4(Scattering, Extinction);
	return;
}

[numthreads(16, 16, 1)]
void CSMain_FroxelZAccumulation(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FroxelGridSize.x || ID.y >= (uint) FroxelGridSize.y)
		return;
	
	float3	AccumulatedScattering = float3(0.0f, 0.0f, 0.0f);
	float	AccumulatedTransmittance = 1.f;
	
	float	SliceRatio = SliceDepthRatio;
	float	NearSlice  = NearZ;

	[loop]
	for (uint z = 0; z < (uint) FroxelGridSize.z; ++z)
	{
		uint3	VoxelCoord = uint3(ID.xy, z);
		float4	LightingData = VoxelLightingColor[VoxelCoord];
		
		float3	Scattering = LightingData.rgb;
		float	Extinction = LightingData.a;
		
		float	FarSlice = NearSlice * SliceRatio;
		float	RayStepSize = FarSlice - NearSlice;
		
		float	StepExtinction = Extinction * RayStepSize;
		float	StepTransmittance = exp(-StepExtinction);
		
		float3 IntegratedScattering = float3(0.f, 0.f, 0.f);
		float WeightedScatteringDistance = 0.f;
		float ScatteringWeightSum = 0.f;
		
		if (Extinction > 0.00001f)
		{
			IntegratedScattering = (Scattering / Extinction) * (1.0f - StepTransmittance);
		}
		else
		{
			IntegratedScattering = Scattering * RayStepSize;
		}
		
		AccumulatedScattering += IntegratedScattering * AccumulatedTransmittance;
		AccumulatedTransmittance *= StepTransmittance;
		
		[branch]
		if (AccumulatedTransmittance < 0.001f)
		{
			AccumulatedTransmittance = 0.f;
		}
		
		OUTPUT3D[VoxelCoord] = float4(AccumulatedScattering, AccumulatedTransmittance);
		
		NearSlice = FarSlice;
	}
	
	return;
}

//[numthreads(16, 16, 1)]
//void CSMain_RayMarching(uint3 ID : SV_DispatchThreadID)
//{
//	if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y)
//		return;
//	
//	float2	TexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
//	float	Depth = DepthTexture.SampleLevel(PointClamp, TexCoord, 0).r;
//	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord);
//	float	PixelViewZ = abs(mul(float4(WorldPos, 1.0f), g_matView).z);
//	
//	float	FroxelZ = saturate(ViewDepthToFroxelZ(PixelViewZ, NearZ, FarZ));
//	float3	FroxelTexCoord = float3(TexCoord, FroxelZ);
//	
//	float4	LightingData = VoxelLightingColor.SampleLevel(LinearClamp, FroxelTexCoord, 0);
//	float3	AccumulatedScattering = LightingData.rgb;
//	float	AccumulatedTransmittance = LightingData.a;
//	
//	OUTPUT[ID.xy] = float4(AccumulatedScattering, AccumulatedTransmittance);
//	return;
//}

//[numthreads(16, 16, 1)]
//void CSMain_RayMarching_WithLoop(uint3 ID : SV_DispatchThreadID)
//{
//	if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y)
//		return;
//	
//	float2	TexCoord = (float2(ID.xy) + 0.5f) / ScreenResolution;
//	float	Depth = DepthTexture.SampleLevel(PointClamp, TexCoord, 0).r;
//	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord);
//	float	PixelViewZ = abs(mul(float4(WorldPos, 1.0f), g_matView).z);
//	
//	float	MaxFroxelZ = saturate(ViewDepthToFroxelZ(PixelViewZ, NearZ, FarZ));
//	float	MaxSlice = (uint) (MaxFroxelZ * FroxelGridSize.z);
//	
//	float3	AccumulatedScattering = float3(0.f, 0.f, 0.f);
//	float	AccumulatedTransmittance = 1.f;
//	
//	[loop]
//	for (uint i = 0; i < MaxSlice; ++i)
//	{
//		float3	FroxelTexCoord = float3(TexCoord, (i + 0.5f) / FroxelGridSize.z);
//		
//		float4	LightingData = VoxelLightingColor.SampleLevel(LinearClamp, FroxelTexCoord, 0);
//		float3	Scattering = LightingData.rgb;
//		float	Extinction = LightingData.a;
//
//		float	RayStepSize = GetSliceDeltaZ(i, FroxelGridSize.z, NearZ, FarZ);
//		
//		float	StepExtinction = Extinction * RayStepSize;
//		float	StepTransmittance = exp(-StepExtinction);
//		
//		float3 IntegratedScattering = float3(0.f, 0.f, 0.f);
//		if (Extinction > 0.0001f)
//		{
//			IntegratedScattering = (Scattering / Extinction) * (1.f - StepTransmittance);
//		}
//        
//		AccumulatedScattering += IntegratedScattering * AccumulatedTransmittance;
//		AccumulatedTransmittance *= StepTransmittance;
//		
//		if (AccumulatedTransmittance < 0.001f)
//		{
//			AccumulatedTransmittance = 0.f;
//			break;
//		}
//	}
//	float3 SceneColor = SceneColorTexture.SampleLevel(PointClamp, TexCoord, 0).rgb;
//	float3 FinalColor = (SceneColor * AccumulatedTransmittance) + AccumulatedScattering;
//	
//	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
//	return;
//}

[numthreads(16, 16, 1)]
void CSMain_RayMarching(uint3 ID : SV_DispatchThreadID)
{
	if (ID.x >= (uint) FullScreenResolution.x || ID.y >= (uint) FullScreenResolution.y)
		return;
	
	float2	TexCoord = (float2(ID.xy) + 0.5f) / FullScreenResolution;
	float	Depth = DepthTexture.SampleLevel(PointClamp, TexCoord, 0).r;
	float3	WorldPos = Convert_WorldPosByDepth(Depth, TexCoord).xyz;
	
	float	PixelViewZ = abs(mul(float4(WorldPos, 1.f), g_matView).z);
	
	float3	RayVector = WorldPos - g_vCamPos;
	float	RayLength = length(RayVector);
	
	if (RayLength <= 0.0001f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 0.f, 1.f);
		return;
	}
	float3	RayDirection = RayVector / RayLength;
	/*
 * Scene Depth 또는 볼류메트릭 최대 거리까지만 적분한다.
 */
	float MaxViewDepth =
    min(PixelViewZ, FarZ);

/*
 * View Z 간격을 실제 ray 이동 거리로 변환하기 위한 값.
 */
	float3 ViewRayDirection =
    mul(
        float4(RayDirection, 0.f),
        g_matView).xyz;

	float RayViewZ =
    max(
        abs(ViewRayDirection.z),
        0.0001f);

	float MaxRayDistance =
    MaxViewDepth / RayViewZ;

	float3 AccumulatedScattering =
    float3(0.f, 0.f, 0.f);

	float AccumulatedTransmittance =
    1.f;


[loop]
	for (uint z = 0;
     z < (uint) FroxelGridSize.z;
     ++z)
	{
		float NormalizedZ0 =
        (float) z /
        FroxelGridSize.z;

		float NormalizedZ1 =
        ((float) z + 1.f) /
        FroxelGridSize.z;

    /*
     * 현재 적용한 power depth distribution과 동일한 방식으로
     * 슬라이스의 View Z 범위를 구한다.
     */
		float SliceNear =
        lerp(
            NearZ,
            FarZ,
            pow(
                NormalizedZ0,
                FroxelDepthExponent));

    /*
     * 현재 오브젝트의 Scene Depth보다 뒤에 있는 슬라이스는
     * 적분할 필요가 없다.
     */
		if (SliceNear >= MaxViewDepth)
		{
			break;
		}

		float SliceFar =
        lerp(
            NearZ,
            FarZ,
            pow(
                NormalizedZ1,
                FroxelDepthExponent));

    /*
     * 마지막 슬라이스가 Scene Depth를 넘어가면
     * Scene Depth까지만 적분한다.
     */
		SliceFar =
        min(
            SliceFar,
            MaxViewDepth);

		float SampleViewDepth =
        (SliceNear + SliceFar) *
        0.5f;

    /*
     * View Depth를 현재 power froxel의 정규화된 Z로 변환한다.
     */
		float SampleFroxelZ =
        ViewDepthToFroxelZ(
            SampleViewDepth,
            NearZ,
            FarZ);

		float3 SampleFroxelTexCoord =
        float3(
            TexCoord,
            saturate(SampleFroxelZ));

		float4 LightingData =
        VoxelLightingColor.SampleLevel(
            LinearClamp,
            SampleFroxelTexCoord,
            0);

		float3 Scattering =
        LightingData.rgb;

		float Extinction =
        LightingData.a;


		float RayStepSize =
        (SliceFar - SliceNear) /
        RayViewZ;

		float StepTransmittance =
        exp(
            -Extinction *
            RayStepSize);

		float3 IntegratedScattering =
        float3(0.f, 0.f, 0.f);

		if (Extinction > 0.0001f)
		{
			IntegratedScattering =
            (Scattering / Extinction) *
            (1.f - StepTransmittance);
		}
		else
		{
			IntegratedScattering = Scattering * RayStepSize;
		}

		float3 StepContribution = IntegratedScattering * AccumulatedTransmittance;

		AccumulatedScattering +=StepContribution;
		AccumulatedTransmittance *= StepTransmittance;

		if (AccumulatedTransmittance < 0.001f) {
			AccumulatedTransmittance = 0.f;
			break;
		}
	}

	OUTPUT[ID.xy] = float4(AccumulatedScattering, AccumulatedTransmittance);
	return;
}
//
//
//[numthreads(16, 16, 1)]
//void CSMain_RayMarching_Optimized(uint3 ID : SV_DispatchThreadID)
//{
//	if (ID.x >= (uint) ScreenResolution.x || ID.y >= (uint) ScreenResolution.y)	return;
//	
//	float2	TexCoord	= (float2(ID.xy) + 0.5f) / ScreenResolution;
//	float	Depth		= DepthTexture.SampleLevel(PointClamp, TexCoord, 0).r;
//	float3	WorldPos	= Convert_WorldPosByDepth(Depth, TexCoord);
//	
//	float3	RayStart	= g_vCamPos;
//	float3	RayEnd		= WorldPos;
//	float3	RayVector	= RayEnd - RayStart;
//	float3	RayDirection = normalize(RayVector);
//	
//	float	StepSize	= length(RayVector) / (float) GodRayMaxStep;
//	
//	float	RayAccumulation = 0.f;
//	
//	float2	NoiseUV = (float2(ID.xy) + 0.5f) / NoiseResolution;
//	float	Jitter = BlueNoiseTexture.SampleLevel(PointWrap, NoiseUV, 0).r;
//	
//	[loop]
//	for (uint i = 0; i < GodRayMaxStep; ++i)
//	{
//		float	SampleDistance = (i + Jitter) * StepSize;
//		float3	SamplePosition = RayStart + RayDirection * SampleDistance;
//		
//		float	ShadowBrightness = Compute_CascadeShadow(SamplePosition);
//		RayAccumulation += ShadowBrightness;	
//	}
//	float FinalGodRay = RayAccumulation / (float) GodRayMaxStep;
//	
//	OUTPUT[ID.xy] = float4(FinalGodRay.rrr, 1.f);
//	return;
//}
