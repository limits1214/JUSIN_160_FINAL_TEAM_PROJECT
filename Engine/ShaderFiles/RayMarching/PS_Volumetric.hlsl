#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D DepthTexture				: register(t0);
Texture2D SceneColorTexture			: register(t1);
Texture3D VoxelAccumulatedTexture	: register(t2);
Texture2D BlueNoiseTexture			: register(t3);

const static float2 NoiseResolution = { 256.f, 256.f };
const static float	DepthThreshold = { 25.f };

const float2 Offsets[5] =
{
	float2(0.f, 0.f), float2(-1.f, 0.f), float2(1.f, 0.f), float2(0.f, -1.f), float2(0.f, 1.f)
};

cbuffer CB_FroxelConfig : register(b10)
{
	float3 FroxelGridSize;
	float NearZ;
	float FarZ;
	float2 ScreenResolution;
	
	float Padding;
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
	float	FogAnisotropyGA; // 전방 산란도
	
	float3	FogLightColor;
	float	FogAnisotropyGB; // 후방 산란도
	
	float	FogScatteringWeight; // 전방/후방 가중치
	
	float	FogBaseHeight;
	float	FogHeightFallOff;
	float	FogPadding;
};


float ViewDepthToFroxelZ(float _Depth, float _Near, float _Far)
{
	return log(max(_Depth, _Near) / _Near) / log(_Far / _Near);
}

float4 CalculateAnalyticFog(float ViewDepth, float3 WorldPos)
{
	if (FogHeight <= 0.0001f)	return float4(0.f, 0.f, 0.f, 1.f);
	
	float HeightDiffer = max(WorldPos.y - FogBaseHeight, 0.f);
	float HeightFactor = exp(-HeightDiffer * FogHeightFallOff);
	float HeightLimit = 1.f - smoothstep(FogHeight * 0.8f, FogHeight, HeightDiffer);

	float FogAmount = 1.f - exp(-ViewDepth * FogDensity * 0.1f * HeightFactor * HeightLimit);
	
	float FogDistanceFactor = smoothstep(FogStartPos, max(FogEndPos, FogStartPos + 0.0001f), ViewDepth);
	
	FogAmount *= FogDistanceFactor;
	
	return float4(FogColor * FogIntensity * FogAmount, 1.f - FogAmount);
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float4	SceneColor	= SceneColorTexture.Sample(PointClamp, TexCoord);
	float	DepthTex	= DepthTexture.Sample(PointClamp, TexCoord).r;
	
	float3	WorldPos	= Convert_WorldPosByDepth(DepthTex, TexCoord);
	float	ViewDepth	= Convert_ViewZPosByDepth(DepthTex);
	
	if (DepthTex >= 1.f)	// 백버퍼 빈 공간
	{
		float4 SkyFog = CalculateAnalyticFog(FarZ, WorldPos);
		return float4(SceneColor.rgb * SkyFog.a + SkyFog.rgb, SceneColor.a);
	}
	
	float4 FinalFog = float4(0.f, 0.f, 0.f, 0.f);
	
	float DepthGradient = abs(ddx(ViewDepth)) + abs(ddy(ViewDepth));
	
	if (DepthGradient < DepthThreshold)
	{
		float SampleFroxelZ = ViewDepthToFroxelZ(ViewDepth, NearZ, FarZ);
		SampleFroxelZ -= (0.5f / FroxelGridSize.z);

		float3 SampleTexCoord3D = float3(TexCoord, saturate(SampleFroxelZ));
		
		FinalFog = VoxelAccumulatedTexture.SampleLevel(LinearClamp, SampleTexCoord3D, 0);
	}
	else
	{
		float2	TexelSize = float2(1.f / ScreenResolution.x, 1.f / ScreenResolution.y);
		float4	AccumulatedFog = float4(0.f, 0.f, 0.f, 0.f);
		float	TotalWeight = 0.0001f;
		
		[unroll]
		for (uint i = 0; i < 5; ++i)
		{
			float2	OffsetUV	= TexCoord + Offsets[i] * TexelSize;	// 상하좌우 픽셀 위치
			float	SampleDepth = DepthTexture.SampleLevel(PointClamp, OffsetUV, 0).r;	// Offset 위치 깊이 샘플링
			float	SampleViewZ = Convert_ViewZPosByDepth(SampleDepth);	
			
			float	SampleFroxelZ = ViewDepthToFroxelZ(SampleViewZ, NearZ, FarZ);
			SampleFroxelZ -= (0.5f / FroxelGridSize.z);

			float3	SampleTexCoord3D = float3(OffsetUV, saturate(SampleFroxelZ));
			float4	SampleFog = VoxelAccumulatedTexture.SampleLevel(LinearClamp, SampleTexCoord3D, 0);
			
			float	SpatialWeight = exp(-(Offsets[i].x * Offsets[i].x + Offsets[i].y * Offsets[i].y) / 2.f);

			float	DepthDiffer = abs(ViewDepth - SampleViewZ);
			float	DepthWeight = exp(-DepthDiffer * 20.f);

			float	Weight = SpatialWeight * DepthWeight;
			
			AccumulatedFog += SampleFog * Weight;
			TotalWeight += Weight;
		}
		FinalFog = AccumulatedFog / TotalWeight;
	}
	
	//[unroll]
	//for (int x = -1; x <= 1; ++x)
	//{
	//	[unroll]
	//	for (int y = -1; y <= 1; ++y)
	//	{
	//		float2 OffsetUV = TexCoord + float2(x, y) * TexelSize;
	//		float SampleDepth = DepthTexture.SampleLevel(PointClamp, OffsetUV, 0).r;
	//		float SampleViewZ = Convert_ViewZPosByDepth(SampleDepth);
	//		
	//		float SampleFroxelZ = ViewDepthToFroxelZ(SampleViewZ, NearZ, MaxFroxelZDistance);
	//		SampleFroxelZ -= (0.5f / FroxelGridSize.z);
	//
	//		float3 SampleUVW = float3(OffsetUV, saturate(SampleFroxelZ));
	//		float4 SampleFog = VoxelAccumulatedTexture.SampleLevel(LinearClamp, SampleUVW, 0);
	//		
	//		float SpatialWeight = exp(-(x * x + y * y) / 2.f);
	//		
	//		float DepthDiff = abs(ViewDepth - SampleViewZ);
	//		float DepthWeight = exp(-DepthDiff * 20.0f);
	//
	//		float Weight = SpatialWeight * DepthWeight;
	//
	//		AccumulatedFog += SampleFog * Weight;
	//		TotalWeight += Weight;
	//	}
	//}
	
	//float4	FinalFog = AccumulatedFog / TotalWeight;
	
	//float	FroxelTexCoordZ = ViewDepthToFroxelZ(ViewDepth, NearZ, MaxFroxelZDistance);
	//float	CorrectedZ = FroxelTexCoordZ - (0.5f / FroxelGridSize.z);
	//
	//float2 NoiseUV = Position.xy / NoiseResolution;
	//float Jitter = BlueNoiseTexture.Sample(PointWrap, NoiseUV).r - 0.5f;
	//CorrectedZ += Jitter / FroxelGridSize.z;
	//
	//float3 FroxelTexCoord = float3(TexCoord, saturate(CorrectedZ));
	//
	//float4	FogTex = VoxelAccumulatedTexture.Sample(LinearClamp, FroxelTexCoord);

	//float4	GodRayTex = GodRayTexture.Sample(LinearClamp, TexCoord);

	float3 FinalScattering = float3(0.f, 0.f, 0.f);
	float FinalTransmittance = 1.f;
	
	if (ViewDepth > 200.0f)
	{
		float4	AnalyticFog = CalculateAnalyticFog(ViewDepth, WorldPos);
		float	BlendWeight = saturate((ViewDepth - 200.f) / (FarZ - 200.f));

		FinalScattering		= lerp(FinalFog.rgb, AnalyticFog.rgb, BlendWeight);
		FinalTransmittance	= lerp(FinalFog.a, AnalyticFog.a, BlendWeight);
	}
	else
	{
		FinalScattering = FinalFog.rgb;
		FinalTransmittance = FinalFog.a;
	}
	
	//float3 FinalScattering = FinalFog.rgb;
	//float FinalTransmittance = FinalFog.a;
	//
	//float DistanceFade = saturate((MaxFroxelZDistance - ViewDepth) / (MaxFroxelZDistance - 80.f));
	//FinalScattering *= DistanceFade;
	//FinalTransmittance = lerp(1.f, FinalTransmittance, DistanceFade);
	
	float3	FinalColor = SceneColor.rgb * FinalTransmittance + FinalScattering;

	return float4(FinalColor, SceneColor.a);
}
