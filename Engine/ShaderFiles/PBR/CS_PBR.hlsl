#include "../ShaderHeader/SH_CommonFunction.hlsli"

#define PCF_ADDITIONAL_COUNT	3
#define PCF_EVALUATE_COUNT		5
#define MAX_EFFECTLIGHT_COUNT	15		

RWTexture2D<float4> OUTPUT : register(u0);

// Base Texture
Texture2D<float4>		AlbedoMap				: register(t0);
Texture2D<float4>		NormalMap				: register(t1);
Texture2D<float4>		SMROMap					: register(t2);
Texture2D<float4>		EmissiveMap				: register(t3);
Texture2D<float4>		AmbientMap				: register(t4);
Texture2D<float>		DepthMap				: register(t5);

// Image Based Lighting
TextureCube				IrradianceMap			: register(t6);		// Enviroment Light
TextureCube				PreFilterMap			: register(t7);					
Texture2D<float4>		LookUpTableMap			: register(t8);		// BRDF LUT

// Shadow Texture
Texture2DArray<float>	StaticShadowMaps		: register(t9);		// Directional Static
Texture2DArray<float>	DynamicShadowMaps		: register(t10);	// Directional Dynamic

TextureCubeArray<float> StaticShadowCubeMaps	: register(t11);	// Point Static
TextureCubeArray<float> DynamicShadowCubeMaps	: register(t12);	// Point Dynamic

Texture2DArray<float>	CSMShadowMaps			: register(t13); // Directional Light

static const float		ShadowSmoothness		= 1.5f;
static const float		ShadowBrightness		= 0.f;
static const float		PointShadowDepthBias	= 0.002f;
static const float		SpotShadowDepthBias		= 0.00001f;

static const float		EnviromentIntensity		= 0.00f;			// 환경광 밝기
static const float		FillLightBrightness		= 0.25f;			// 등지는 영역의 밝기
static const float		DirectLightBrightness	= 0.60f;			// 빛받는 영역의 밝기

static const float2		PoissonDisk_EightTab[8] =		
{
	float2(0.000000f, 0.000000f),
	float2(0.440453f, 0.639399f),
	float2(-0.757088f, 0.349334f),
	float2(-0.419418f, -0.616039f),
	float2(0.574619f, -0.715851f),
	
	float2(0.527837f, -0.085868f),
	float2(-0.040062f, 0.536087f),
	float2(-0.670445f, -0.179949f)
};
static const float2		PoissonDisk_FourTab[4] =
{	
	float2(-0.326, -0.406), float2(-0.840, 0.074), float2(-0.696, 0.457), float2(-0.203, 0.621)
};
static const float2 RandomRotationCS[8] =
{
	float2(1.00000000f, 0.00000000f),
    float2(0.70710678f, 0.70710678f),
    float2(0.00000000f, 1.00000000f),
    float2(-0.70710678f, 0.70710678f),
    float2(-1.00000000f, 0.00000000f),
    float2(-0.70710678f, -0.70710678f),
    float2(0.00000000f, -1.0000000f),
    float2(0.70710678f, -0.70710678f)
};

cbuffer CB_EFFECT_LIGHT : register(b11)
{
	EffectLight ELightList[MAX_EFFECTLIGHT_COUNT];

	uint	ELightCount;
	float3	ELightPadding;
};
cbuffer CB_CSM : register(b12)
{
	matrix ShadowViewProj[4];
	float4 CascadeSplits;
	float2 ShadowMapSize;
	float2 ShadowBias;
};

uint Hash_ShadowPixel(uint2 PixelPos)
{
	uint Hash = PixelPos.x * 0x8DA6B343u;
	Hash ^= PixelPos.y * 0xD8163841u;

	Hash ^= Hash >> 16;
	Hash *= 0x7FEB352Du;
	Hash ^= Hash >> 15;

	return Hash;
}

float2x2 Get_RandomNoise(uint2 _PixelPos)
{
	uint RotationIndex = Hash_ShadowPixel(_PixelPos) & 7u;

	float2 CosSin = RandomRotationCS[RotationIndex];

	return float2x2(CosSin.x, -CosSin.y, CosSin.y, CosSin.x);
}

float DistributionGGX(float3 N, float3 H, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    
    float NDH = max(0.f, dot(N, H));
    
    float Num = R2;
    float Denom = ((NDH * NDH) * (R2 - 1.0) + 1.0);
    Denom = PI * Denom * Denom;
	
    return Num / max(0.000001f, Denom);
}
float VisibilitySmithJointGGX(float NDY, float NDL, float _Roughness)
{
    float R = _Roughness * _Roughness;
    float R2 = R * R;
    
    float lambdaV = NDL * sqrt(max((-NDY * R2 + NDY) * NDY + R2, 0.001f));
    float lambdaL = NDY * sqrt(max((-NDL * R2 + NDL) * NDL + R2, 0.001f));
    
    float Denom = lambdaV + lambdaL;
    return Denom > 0.0f ? 0.5f / Denom : 0.0f;
}
float3 FresnelSchlick(float CTH, float3 MBR)
{
    float ClampCTH = clamp(CTH, 0.0f, 1.0f);
    return MBR + (1.0 - MBR) * pow(clamp(1.0 - ClampCTH, 0.0, 1.0), 5.0);
}
float MergeShadowMap(int _ShadowSlot, float2 _SamplerUV, float _CurrentPixelDepth)
{
	return DynamicShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
}
float MergeShadowCubeMap(int _ShadowSlot, float3 _SamplerUV, float _CurrentPixelDepth)
{
	return DynamicShadowCubeMaps.SampleCmpLevelZero(ShadowSampler, float4(_SamplerUV, _ShadowSlot), _CurrentPixelDepth);
}
float Compute_SmoothShadow(float4 _WorldPos, float2x2 _RandomRotMat, float2 _SamplingRange, int _ShadowSlot, uint _LightIndex)
{
	float4	LightPos = mul(float4(_WorldPos.xyz, 1.f), AffectedLight[_LightIndex].g_LightViewProj[0]);
	float3	LightNDC = LightPos.xyz * rcp(LightPos.w);
	
	[branch]
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
	 	LightNDC.y < -1.f || LightNDC.y > 1.f ||
		LightNDC.z < 0.f  || LightNDC.z > 1.f)		return 1.f;

	float2 ShadowMapUV;
	ShadowMapUV.x = LightNDC.x * +0.5f + 0.5f;
	ShadowMapUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float CurrentPixelDepth = saturate(LightNDC.z - SpotShadowDepthBias);
	float ShadowFactor = 0.f;
	
    [unroll]
	for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
		float2 SampleUV = ShadowMapUV + (RotatedOffset * _SamplingRange);
		
		ShadowFactor += MergeShadowMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
	}
	
	[branch]		// Every Direction ShadowFactor 0.f
	if (ShadowFactor <= 0.0001f)	return ShadowBrightness;
	
	[branch]		// Every Direction ShadowFactor 1.f
	if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f) return 1.f;
	
	[unroll]
	for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
        
		float2 SampleUV = ShadowMapUV + (RotatedOffset * _SamplingRange);
		
		ShadowFactor += MergeShadowMap(_ShadowSlot, SampleUV, CurrentPixelDepth);
	}

	return lerp(ShadowBrightness, 1.f, saturate(ShadowFactor * (1.f / (PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT))));
}

float Compute_PointShadow(float4 _WorldPos, float2x2 _RandomRotMat, int _ShadowSlot, uint _LightIndex)
{
	float3	LightToPixel = _WorldPos.xyz - AffectedLight[_LightIndex].Position;
	
	float	DistanceSQ = max(dot(LightToPixel, LightToPixel), 0.000001f);
	
	float	InvDistance = rsqrt(DistanceSQ);
	float	Distance	= DistanceSQ * InvDistance;
	float3	Direction	= LightToPixel * InvDistance;
	
	float CurrentPixelDepth = Distance / max(0.02f, AffectedLight[_LightIndex].OuterAttanuation);
	CurrentPixelDepth -= PointShadowDepthBias;
	
	float3 BaseUP = abs(Direction.z) < 0.999f ? float3(0.f, 0.f, 1.f) : float3(1.f, 0.f, 0.f);

	float3 TangentX = normalize(cross(Direction, BaseUP));
	float3 TangentY = normalize(cross(Direction, TangentX));
    
	float FilterRadius = (ShadowSmoothness / POINTLIGHT_RESOLUTION);
	
	float ShadowFactor = 0.f;

    [unroll]
	for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
    {
		float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
        
        float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
		float3 SampleDirection = Direction + Offset3D;
		
		ShadowFactor += MergeShadowCubeMap(_ShadowSlot, SampleDirection, CurrentPixelDepth);
	}
	
	[branch]		// Every Direction ShadowFactor 0.f
	if (ShadowFactor <= 0.0001f)	return ShadowBrightness;
	
	[branch]		// Every Direction ShadowFactor 1.f
	if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f) return 1.f;
	
	[unroll]
	for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
	{
		float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
        
		float3 Offset3D = (TangentX * RotatedOffset.x + TangentY * RotatedOffset.y) * FilterRadius;
        
		float3 SampleDirection = Direction + Offset3D;
		
		ShadowFactor += MergeShadowCubeMap(_ShadowSlot, SampleDirection, CurrentPixelDepth);
	}
	
	return lerp(ShadowBrightness, 1.f, saturate(ShadowFactor * (1.f / (PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT))));
}

float Compute_CascadeShadow(float4 _WorldPos, float2x2 _RandomRotMat)
{
	float4	ViewPos = mul(float4(_WorldPos.xyz, 1.f), g_matView);
	float	ViewDepth = abs(ViewPos.z);
	
	if (CascadeSplits.w <= 0.f || ViewDepth >= CascadeSplits.w)	return 1.f;
	
	int		CascadeIndex;
	if		(ViewDepth < CascadeSplits.x)	CascadeIndex = 0;
	else if (ViewDepth < CascadeSplits.y)	CascadeIndex = 1;
	else if (ViewDepth < CascadeSplits.z)	CascadeIndex = 2;
	else 	CascadeIndex = 3;
	
	float4 LightPos = mul(float4(_WorldPos.xyz, 1.f), ShadowViewProj[CascadeIndex]);
	
	if (abs(LightPos.w) <= 0.00001f)	return 1.f;
	
	float3 LightNDC = LightPos.xyz / LightPos.w;
	
	if (LightNDC.x < -1.f || LightNDC.x > 1.f ||
		LightNDC.y < -1.f || LightNDC.y > 1.f ||
		LightNDC.z < 0.f  || LightNDC.z > 1.f)		return 1.f;

	float2 ShadowMapUV;
	ShadowMapUV.x = LightNDC.x * +0.5f + 0.5f;
	ShadowMapUV.y = LightNDC.y * -0.5f + 0.5f;
	
	float	CurrentDepth = LightNDC.z - ShadowBias.x;
	
	float2	SamplingRange = ShadowSmoothness / max(ShadowMapSize, float2(1.f, 1.f));
	float	ShadowFactor = 0.f;
	
	[branch]
	if (CascadeIndex <= 1)
	{
		[unroll]
		for (int i = 0; i < PCF_EVALUATE_COUNT; ++i)
		{
			float2 RotatedOffset = mul(PoissonDisk_EightTab[i], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, (float) CascadeIndex), CurrentDepth);
		}
		
		[branch]		// Every Direction ShadowFactor 0.f
		if (ShadowFactor <= 0.0001f)	return ShadowBrightness;
	
		[branch]		// Every Direction ShadowFactor 1.f
		if (ShadowFactor >= (float) PCF_EVALUATE_COUNT - 0.0001f) return 1.f;
		
		[unroll]
		for (int j = PCF_EVALUATE_COUNT; j < PCF_EVALUATE_COUNT + PCF_ADDITIONAL_COUNT; ++j)
		{
			float2 RotatedOffset = mul(PoissonDisk_EightTab[j], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, (float) CascadeIndex), CurrentDepth);
		}
		
		ShadowFactor *= 0.125f;
	}
	else
	{
		[unroll]
		for (int i = 0; i < PCF_EVALUATE_COUNT - 1; ++i)
		{
			float2 RotatedOffset = mul(PoissonDisk_FourTab[i], _RandomRotMat);
		
			float2 SampleUV = ShadowMapUV + RotatedOffset * SamplingRange;
			ShadowFactor += CSMShadowMaps.SampleCmpLevelZero(ShadowSampler, float3(SampleUV, (float) CascadeIndex), CurrentDepth);
		}
		ShadowFactor *= 0.25f;
	}
	
	return lerp(ShadowBrightness, 1.f, ShadowFactor);
}

float3 Compute_EnviromentLight(float3 N, float3 V, float3 _Albedo, float _Roughness, float _Metallic, float3 MBR)
{
	N = normalize(N);
	V = normalize(V);
	
	float Roughness = saturate(_Roughness);
	float Metallic  = saturate(_Metallic);
	
	float ReverseRoughness = 1.f - Roughness;
	float3 Fresnel = max(float3(ReverseRoughness, ReverseRoughness, ReverseRoughness), MBR);
	
	float NDV = saturate(dot(N, V));
	float3 F = MBR + (Fresnel - MBR) * pow(1.f - NDV, 5.f);
	
	float3 KS = F;
	float3 KD = (1.f - KS) * (1.f - Metallic);
	
	float3 Irradiance = IrradianceMap.SampleLevel(LinearClamp, N, 0.f).rgb;
	
	float3 DiffuseAmbient = KD * Irradiance * _Albedo;
	float3 R = reflect(-V, N);
	
	float3 PreFilteredMap = PreFilterMap.SampleLevel(LinearClamp, R, Roughness * MAX_REFLECTION_LOD).rgb;
	
	float2 BRDF = LookUpTableMap.SampleLevel(LinearClamp, float2(NDV, Roughness), 0.f).rg;
	
	float3 SpecularAmbient = PreFilteredMap * (F * BRDF.x + BRDF.y);
	
	return DiffuseAmbient + SpecularAmbient;
}

[numthreads(16, 16, 1)]
void CSMain_NonBlend(uint3 ID : SV_DispatchThreadID)
{
	uint ScreenWidth;
	uint ScreenHeight;
	DepthMap.GetDimensions(ScreenWidth, ScreenHeight);
	
	[branch]
	if (ID.x >= ScreenWidth || ID.y >= ScreenHeight)	return;
	int3 PixelCoord = int3(ID.xy, 0);
	
	float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(ScreenWidth, ScreenHeight);
	float	Depth	= DepthMap.Load(PixelCoord); // 해당 픽셀 깊이 계산

	[branch]
    if (Depth >= 1.f)
    {
        OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
        return;
    }
	
    float4	DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);
	
	float3	WorldNormal = normalize(NormalMap.Load(PixelCoord).rgb * 2.f - 1.f);

	float3	AlbedoTex = AlbedoMap.Load(PixelCoord).rgb;
    float3	Albedo = pow(AlbedoTex.rgb, 2.2f);
	
	float3	MultipleTex = SMROMap.Load(PixelCoord).rgb;
    float	Metallic = MultipleTex.r;
	float	Roughness = clamp(MultipleTex.g, 0.15f, 1.f);

    float3	V = normalize(g_vCamPos - DepthWorld.xyz);
    float	NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
    float3	MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
    float3	LightAccumulation = float3(0.f, 0.f, 0.f);
	
	float2x2 RandomNoiseMatrix = Get_RandomNoise(ID.xy);
	
	float2	SamplingRange = 1.f / float2(POINTLIGHT_RESOLUTION, POINTLIGHT_RESOLUTION) * ShadowSmoothness;
	
	[loop]
    for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3	H = normalize(V + L);
				float	D = DistributionGGX(WorldNormal, H, Roughness);
				float3	F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.f - kS) * (1.f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				 
				float ShadowFactor = 1.f;
				int ShadowSlot = AffectedLight[i].ShadowSlot;
				
				uint LightType = AffectedLight[i].LightType;
				
				[branch]
				if (LightType == LIGHT_DIRECTIONAL)
				{
					ShadowFactor = Compute_CascadeShadow(DepthWorld, RandomNoiseMatrix);
				}
				else if (ShadowSlot >= 0 && ShadowSlot < MAX_SHADOW_LIGHT_COUNT)
				{
					if (LightType == LIGHT_POINT)
					{
						ShadowFactor = Compute_PointShadow(DepthWorld, RandomNoiseMatrix, ShadowSlot, i);
					}
					else
					{
						ShadowFactor = Compute_SmoothShadow(DepthWorld, RandomNoiseMatrix, SamplingRange, ShadowSlot, i);
					}
				}
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL * ShadowFactor;
			}
		}
	}

	float3 EffectAccumulation = float3(0.f, 0.f, 0.f);
	
	[loop]
	for (uint j = 0; j < ELightCount; ++j)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_EffectLight(DepthWorld.xyz, ELightList[j], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				
				EffectAccumulation += (Diffuse + Specular) * Radiance * NDL;
			}
		}
	}
	//float3	BaseEmissive = EmissiveMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3	BaseEmissive = EmissiveMap.Load(PixelCoord).rgb;
    
	//float	AmbientOcclusion = AmbientMap.SampleLevel(LinearWrap, TexCoord, 0.f).r;
	float	AmbientOcclusion = AmbientMap.Load(PixelCoord).r;
	float3	Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3	EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;		// Enviroment Light
	
	float3	FillLighting	= Albedo * (1.f - Metallic) * FillLightBrightness;		// Shadow Face
	float3	DirectLighting	= LightAccumulation * DirectLightBrightness;			// Light Face
	float3	EffectLighting = EffectAccumulation * DirectLightBrightness; // Light Face
	
	float3 FinalColor = EnviromentLight + FillLighting + DirectLighting + EffectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
    return;
}

[numthreads(16, 16, 1)]
void CSMain_Blend(uint3 ID : SV_DispatchThreadID)
{
	uint ScreenWidth;
	uint ScreenHeight;
	DepthMap.GetDimensions(ScreenWidth, ScreenHeight);
	
	[branch]
	if (ID.x >= ScreenWidth || ID.y >= ScreenHeight)
		return;
	//[branch]
	//if (ID.x >= SCREENX || ID.y >= SCREENY) return; // 스레드가 해상도 넘어가면 출력X
	int3 PixelCoord = int3(ID.xy, 0);
	
	//float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(ScreenWidth, ScreenHeight);
    //float	Depth = DepthMap.SampleLevel(LinearWrap, TexCoord, 0.f).r; // 해당 픽셀 깊이 계산
	float Depth = DepthMap.Load(PixelCoord); // 해당 픽셀 깊이 계산

	[branch]
	if (Depth >= 1.f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
		return;
	}
	
	float4 DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);
	
    //float3 WorldNormal = normalize(NormalMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * 2.f - 1.f);
	float3 WorldNormal = normalize(NormalMap.Load(PixelCoord).rgb * 2.f - 1.f);
	
    //float3 AlbedoTex = AlbedoMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float4 AlbedoTex = AlbedoMap.Load(PixelCoord);
	float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

    //float3 MultipleTex = SMROMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3 MultipleTex = SMROMap.Load(PixelCoord).rgb;
	float Metallic = MultipleTex.r;
	float Roughness = clamp(MultipleTex.g, 0.15f, 1.f);
    //float   Ambient     = MultipleTex.b;

	float3 V = normalize(g_vCamPos - DepthWorld.xyz);
	float NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);
	
	float2x2 RandomNoiseMatrix = Get_RandomNoise(ID.xy);
	
	float2 SamplingRange = 1.f / float2(POINTLIGHT_RESOLUTION, POINTLIGHT_RESOLUTION) * ShadowSmoothness;
	
	[loop]
	for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				 
				float ShadowFactor = 1.f;
				int ShadowSlot = AffectedLight[i].ShadowSlot;
				
				[branch]
				if (ShadowSlot >= 0 && ShadowSlot < MAX_SHADOW_LIGHT_COUNT)
				{
					[branch]
					if		(AffectedLight[i].LightType == LIGHT_DIRECTIONAL)
					{
						ShadowFactor = Compute_CascadeShadow(DepthWorld, RandomNoiseMatrix);
					}
					else if (AffectedLight[i].LightType == LIGHT_POINT)
					{
						ShadowFactor = Compute_PointShadow(DepthWorld, RandomNoiseMatrix, ShadowSlot, i);
					}
					else
					{
						ShadowFactor = Compute_SmoothShadow(DepthWorld, RandomNoiseMatrix, SamplingRange, ShadowSlot, i);
					}
				}
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL * ShadowFactor;
			}
		}
	}
	float3 BaseEmissive = EmissiveMap.Load(PixelCoord).rgb;
    
	float	AmbientOcclusion = AmbientMap.Load(PixelCoord).r;
	float3	Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3	EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity; // Enviroment Light
	
	float3	FillLighting = Albedo * (1.f - Metallic) * FillLightBrightness; // Shadow Face
	float3	DirectLighting = LightAccumulation * DirectLightBrightness; // Light Face
	
	float3	FinalColor = EnviromentLight + FillLighting + DirectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, AlbedoTex.a);
	return;
}

[numthreads(16, 16, 1)]
void CSMain_NonShadow(uint3 ID : SV_DispatchThreadID)
{
	uint ScreenWidth;
	uint ScreenHeight;
	DepthMap.GetDimensions(ScreenWidth, ScreenHeight);
	
	[branch]
	if (ID.x >= ScreenWidth || ID.y >= ScreenHeight)
		return;
	//[branch]
	//if (ID.x >= SCREENX || ID.y >= SCREENY) return; // 스레드가 해상도 넘어가면 출력X
	int3 PixelCoord = int3(ID.xy, 0);
	
	//float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(ScreenWidth, ScreenHeight);
    //float	Depth = DepthMap.SampleLevel(LinearWrap, TexCoord, 0.f).r; // 해당 픽셀 깊이 계산
	float Depth = DepthMap.Load(PixelCoord); // 해당 픽셀 깊이 계산
	
	[branch]
	if (Depth >= 1.f)
	{
		OUTPUT[ID.xy] = float4(0.f, 0.f, 1.f, 1.f);
		return;
	}

	float4 DepthWorld = Convert_WorldPosByDepth(Depth, TexCoord);

	//float3 WorldNormal = normalize(NormalMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb * 2.f - 1.f);
	float3 WorldNormal = normalize(NormalMap.Load(PixelCoord).rgb * 2.f - 1.f);
	
	//float3 AlbedoTex = AlbedoMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3 AlbedoTex = AlbedoMap.Load(PixelCoord).rgb;
	float3 Albedo = pow(AlbedoTex.rgb, 2.2f);

	//float3 MultipleTex = SMROMap.SampleLevel(LinearWrap, TexCoord, 0.f).rgb;
	float3 MultipleTex = SMROMap.Load(PixelCoord).rgb;
	float Metallic = MultipleTex.r;
	float Roughness = clamp(MultipleTex.g, 0.15f, 1.f);
    //float   Ambient     = MultipleTex.b;

	float3 V = normalize(g_vCamPos - DepthWorld.xyz);
	float NDV = max(dot(WorldNormal, V), 0.0001f);
    
    // Metallic Material Based Reflection
	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, Metallic);
	
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);
	
	[unroll]
	for (uint i = 0; i < LightCount; ++i)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_DynamicLight(DepthWorld.xyz, AffectedLight[i], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				
				LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
			}
		}
	}
	float3 EffectAccumulation = float3(0.f, 0.f, 0.f);
	
	[loop]
	for (uint j = 0; j < ELightCount; ++j)
	{
		float3 L, Radiance;
		[branch]
		if (Compute_EffectLight(DepthWorld.xyz, ELightList[j], L, Radiance))
		{
			float RawNDL = dot(WorldNormal, L);
			[branch]
			if (RawNDL > 0.f)
			{
				float NDL = clamp(RawNDL, 0.f, 1.f);

				float3 H = normalize(V + L);
				float D = DistributionGGX(WorldNormal, H, Roughness);
				float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);

				float V_Spec = VisibilitySmithJointGGX(NDV, NDL, Roughness);

				float3 Specular = D * F * V_Spec / 10.f;

				float3 kS = F;
				float3 kD = (1.0f - kS) * (1.0f - Metallic);
				float3 Diffuse = kD * Albedo / PI;
				
				EffectAccumulation += (Diffuse + Specular) * Radiance * NDL;
			}
		}
	}
	float3 BaseEmissive = EmissiveMap.Load(PixelCoord).rgb;
    
	float  AmbientOcclusion = AmbientMap.Load(PixelCoord).r;
	float3 Ambient = Compute_EnviromentLight(WorldNormal, V, Albedo, Roughness, Metallic, MBR);
	
	float3 EnviromentLight = Ambient * AmbientOcclusion * EnviromentIntensity;		// Enviroment Light
	
	float3 FillLighting = Albedo * (1.f - Metallic) * FillLightBrightness;			// Shadow Face
	float3 DirectLighting = LightAccumulation * DirectLightBrightness;				// Light Face  
	float3 EffectLighting = EffectAccumulation * DirectLightBrightness;				// Light Face
	
	float3 FinalColor = EnviromentLight + FillLighting + DirectLighting + EffectLighting + BaseEmissive;
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
	return;
}
