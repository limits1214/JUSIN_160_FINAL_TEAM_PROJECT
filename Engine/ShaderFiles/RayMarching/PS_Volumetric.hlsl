#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D DepthTexture				: register(t0);
Texture2D SceneColorTexture			: register(t1);
Texture2D GodRayTexture				: register(t2);

const static float2		NoiseResolution = { 256.f, 256.f };
const static float		DepthThreshold	= { 25.f };
const static float		AnalyticSkyDistance = 1000.f;

const static float		FroxelDepthExponent = 1.5f;

const static float2		Offsets[5] =
{
	float2(0.f, 0.f), float2(-1.f, 0.f), float2(1.f, 0.f), float2(0.f, -1.f), float2(0.f, 1.f)
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
	float LinearDepth = saturate((_Depth - _Near) / max(_Far - _Near, 0.0001f));
	return pow(LinearDepth, 1.f / FroxelDepthExponent);
}

float4 CalculateAnalyticFog(float ViewDepth, float3 WorldPos)
{
	if (FogHeight <= 0.0001f)	return float4(0.f, 0.f, 0.f, 1.f);
	
	float HeightDistance = max(WorldPos.y - FogBaseHeight, 0.f);
	float HeightFactor	 = exp(-HeightDistance * FogHeightFallOff);
	float HeightLimit	 = 1.f - smoothstep(FogHeight * 0.8f, FogHeight, HeightDistance);

	float FogAmount = 1.f - exp(-ViewDepth * FogDensity * 0.1f * HeightFactor * HeightLimit);
	
	float FogDistanceFactor = smoothstep(FogStartPos, max(FogEndPos, FogStartPos + 0.0001f), ViewDepth);
	
	FogAmount *= FogDistanceFactor;
	
	return float4(FogColor * FogIntensity * FogAmount, 1.f - FogAmount);
}

float4 PSMain(float4 Position : SV_POSITION, float2 TexCoord : TEXCOORD0) : SV_TARGET
{
	float4	SceneColor	= SceneColorTexture.Sample(PointClamp, TexCoord);
	float	DepthTex = DepthTexture.Sample(PointClamp, TexCoord).r;
	
	float4	RayMarchedFog = GodRayTexture.SampleLevel(LinearClamp, TexCoord, 0);
	
	float3	WorldPos	= Convert_WorldPosByDepth(DepthTex, TexCoord).xyz;
	float	ViewDepth	= Convert_ViewZPosByDepth(DepthTex);
	
	if (DepthTex >= 1.f)	// 백버퍼 빈 공간
	{
		float3 FinalColor = SceneColor.rgb * RayMarchedFog.a + RayMarchedFog.rgb;
		return float4(FinalColor, SceneColor.a);
	}

	float4	AnalyticFog		= CalculateAnalyticFog(ViewDepth, WorldPos);
	float	AnalyticWeight	= smoothstep(AnalyticBlendStart, AnalyticBlendEnd, ViewDepth);
	
	float3	FinalScattering		= lerp(RayMarchedFog.rgb, AnalyticFog.rgb, AnalyticWeight);
	float	FinalTransmittance	= lerp(RayMarchedFog.a, AnalyticFog.a, AnalyticWeight);
	
	return float4(SceneColor.rgb * FinalTransmittance + FinalScattering, SceneColor.a);
}
