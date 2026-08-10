#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D<float4> OriginalTexture	: register(t0);
Texture2D<float4> BlurPassTexture	: register(t1);

Texture2D<float4> LUT_Texture		: register(t2);
Texture2D<float4> FocusingTexture	: register(t3);

static const float	CenterWeight		= { 0.227027f };
static const float	BlurOffsets[2]		= { 1.3846154f, 3.2307692f };
static const float	BlurWeights[2]		= { 0.3162162f, 0.0702703f };

static const float	BrightThreshold		= { 0.60f };
static const float	BloomIntensity		= { 0.25f };

static const float	HalfBloomWeight		= { 0.60f };
static const float	QuarterBloomWeight	= { 0.40f };

static const float	ChromaticRing_Radius	 = { 0.32f };
static const float	ChromaticRing_Width		 = { 0.05f };
static const float	ChromaticRing_Smoothness = { 0.06f };

static const float  OutlineThickness = 1.f;
static const float4 OutlineColor = float4(1.f, 1.f, 1.f, 1.f);
// LUT ColorGrading Global Variable
static const float LUT_Size = 16.f;

// ToneMapping Global Variable
static const float3x3 AGX_InMatrix = float3x3(
    0.84242023, 0.07783290, 0.07974687,
    0.04561999, 0.84068596, 0.11369405,
    0.01731649, 0.07604894, 0.90663457
);

static const float3x3 AGX_OutMatrix = float3x3(
    1.19687984, -0.05289685, -0.14398300,
    -0.05886190, 1.15190313, -0.09304123,
    -0.01497570, -0.08150601, 1.09648171
);

static const float Min_Luminance = -12.47393f;
static const float Max_Luminance = 4.026069f;

static const float DistortionIntensity	= { 0.f }; // 왜곡 강도
static const float ChromaticIntensity	= { 0.f }; // 색수차 강도
static const float VignetteIntensity	= { 0.f }; // 비네팅 강도
static const float VignetteSmoothness	= { 0.f }; // 비네팅

RWTexture2D<float4> OUTPUT : register(u0);

cbuffer CB_POSTPROCESS : register(b10)
{
	float2 TexelSize;
	float2 _pad;
};

cbuffer CB_LENSFLARE : register(b11)
{
	float2	FlareCenterUV;
	float	FlareCurrentLifeTime;
	float	FlareMaxLifeTime;
	
	float	RingStartScale;
	float	RingEndScale;
	float	AspectRatio;
	float	RingBaseAlpha;
	
	float	RainbowSaturation;
	float	FlareEnabled;
	float2	TextureSize;
}

/////////////////////////// LensFlare Main Shader Function
float2 GetRingUV(float2 _TexCoord, float _Scale)
{
	float2 Delta = _TexCoord - FlareCenterUV;
	
	Delta.x *= AspectRatio;
	Delta /= max(_Scale, 0.001f);
	
	return Delta + 0.5f;
}

float IsInsideTexture(float2 _TexCoord)
{
	return step(0.0f, _TexCoord.x) * step(_TexCoord.x, 1.0f) * step(0.0f, _TexCoord.y) * step(_TexCoord.y, 1.0f);
}

float3 HSVToRGB(float3 _HSV)
{
	float3 P = abs(frac(_HSV.xxx + float3(0.f, 2.f / 3.f, 1.f / 3.f)) * 6.f - 3.f);

	return _HSV.z * lerp( float3(1.f, 1.f, 1.f), saturate(P - 1.f), _HSV.y);
}

float3 Make_ChromaticRing(float2 _TexCoord, out float _RingMask)
{
	float Radius = length(_TexCoord - 0.5f) * 2.f;
	
	float InnerRadius = ChromaticRing_Radius - ChromaticRing_Width;
	float OuterRadius = ChromaticRing_Radius + ChromaticRing_Width;

	float InnerMask = smoothstep(InnerRadius - ChromaticRing_Smoothness, InnerRadius + ChromaticRing_Smoothness, Radius);
	float OuterMask = 1.f - smoothstep(OuterRadius - ChromaticRing_Smoothness, OuterRadius + ChromaticRing_Smoothness, Radius);
	
	_RingMask = InnerMask * OuterMask;

	float RainbowRatio = saturate((Radius - InnerRadius) / max(OuterRadius - InnerRadius, 0.0001f));

	float Hue = lerp(2.f / 3.f, 0.f, RainbowRatio);

	return HSVToRGB(float3(Hue, saturate(RainbowSaturation), 1.f));
}

[numthreads(16, 16, 1)]
void CSMain_LensFlare(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= TextureSize.x || ID.y >= TextureSize.y)	return;
	
	float2	ScreenUV = (float2(ID.xy) + 0.5f) / TextureSize;
	float4  BackGroundColor = OriginalTexture.SampleLevel(LinearClamp, ScreenUV, 0);
	
	float	TimeRatio = saturate(FlareCurrentLifeTime / max(FlareMaxLifeTime, 0.0001f));

	if (FlareEnabled < 0.5f)
	{
		OUTPUT[ID.xy] = BackGroundColor;
		return;
	}
	
	float	EaseInValue = pow(TimeRatio, 2.f);

	float	RingScale = lerp(RingStartScale, RingEndScale, EaseInValue);
	
	float2	RingTexCoord = GetRingUV(ScreenUV, RingScale);
	
	float	ValidPixel = IsInsideTexture(RingTexCoord);

	float	ProceduralMask;
	float3	ChromaticMask	= Make_ChromaticRing(RingTexCoord, ProceduralMask) * ValidPixel;
	
	float	BaseWhiteValue	= min(ChromaticMask.r, min(ChromaticMask.g, ChromaticMask.b));
	float3	ChromaticOnly	= max(ChromaticMask - BaseWhiteValue.xxx, 0.f);
	
	float	Opacity			= (1.f - TimeRatio) * (1.f - TimeRatio) * RingBaseAlpha * 3.f;
	
	float3	FinalColor		= ChromaticOnly * ProceduralMask * Opacity;
	
	OUTPUT[ID.xy] = float4(BackGroundColor.rgb + FinalColor, BackGroundColor.a);
	return;
}

/////////////////////////// BLOOM Main Shader Function
float KarisWeight(float3 _Color)
{
	float3 Luminance = dot(_Color, float3(0.2126f, 0.7152f, 0.0722f));
	return 1.f / (1.f + max(Luminance, 0.0001f));
}

float3 CustomDownSampling(Texture2D _Texture, float2 _TexCoord, float2 _TexelSize)
{
	float2 SamplingOffset = _TexelSize * 0.5f; // Sampling Near Pixel
	
	float3 Color = 0.f;
	float ColorA = _Texture.SampleLevel(LinearClamp, _TexCoord + float2(-SamplingOffset.x, -SamplingOffset.y), 0).rgb;
	float ColorB = _Texture.SampleLevel(LinearClamp, _TexCoord + float2(+SamplingOffset.x, -SamplingOffset.y), 0).rgb;
	float ColorC = _Texture.SampleLevel(LinearClamp, _TexCoord + float2(-SamplingOffset.x, +SamplingOffset.y), 0).rgb;
	float ColorD = _Texture.SampleLevel(LinearClamp, _TexCoord + float2(+SamplingOffset.x, +SamplingOffset.y), 0).rgb;

	float WeightA = KarisWeight(ColorA);
	float WeightB = KarisWeight(ColorB);
	float WeightC = KarisWeight(ColorC);
	float WeightD = KarisWeight(ColorD);
	
	float3	WeightedColorSum = ColorA * WeightA + ColorB * WeightB + ColorC * WeightC + ColorD * WeightD;
	float	WeightSum = WeightA + WeightB + WeightC + WeightD;
	
	return WeightedColorSum / max(WeightSum, 0.0001f);
}

float3 DownSampling(Texture2D _Texture, float2 _TexCoord, float2 _TexelSize)
{
	float2 SamplingOffset = _TexelSize * 0.5f;

	float3 Color = 0.f;
	Color += _Texture.SampleLevel(LinearClamp, _TexCoord + float2(-SamplingOffset.x, -SamplingOffset.y), 0).rgb;
	Color += _Texture.SampleLevel(LinearClamp, _TexCoord + float2(+SamplingOffset.x, -SamplingOffset.y), 0).rgb;
	Color += _Texture.SampleLevel(LinearClamp, _TexCoord + float2(-SamplingOffset.x, +SamplingOffset.y), 0).rgb;
	Color += _Texture.SampleLevel(LinearClamp, _TexCoord + float2(+SamplingOffset.x, +SamplingOffset.y), 0).rgb;

	Color *= 0.25f;
	
	return min(Color, float3(50.0f, 50.0f, 50.0f));
}

float3 GaussianBlur(float2 _TexCoord, float2 _TexelDirection)
{
	float4 CenterPixel = BlurPassTexture.SampleLevel(LinearClamp, _TexCoord, 0) * CenterWeight;
	
	[unroll]
	for (int i = 0; i < 2; ++i)
	{
		const float2 Offset = _TexelDirection * BlurOffsets[i];
		CenterPixel += BlurPassTexture.SampleLevel(LinearClamp, _TexCoord + Offset, 0) * BlurWeights[i];
		CenterPixel += BlurPassTexture.SampleLevel(LinearClamp, _TexCoord - Offset, 0) * BlurWeights[i];
	}
	
	return CenterPixel.rgb;
}

float SoftKneeCurve(float _Luminance, float _Threshold, float _Knee)
{
	// make Transition by Threshold Softly
	float Soft = _Luminance - _Threshold + _Knee;
	Soft = clamp(Soft, 0.f, 2.f * _Knee);
	Soft = Soft * Soft / max(4.f * _Knee, 0.0001f);
	
	return Soft;
}

[numthreads(8, 8, 1)]
void CSMain_BrightPass(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY)	return;
	
	float2	TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY); 
	
	float3	DownSampledColor = CustomDownSampling(OriginalTexture, TexCoord, TexelSize);

	float	Luminance = dot(DownSampledColor, float3(0.2126f, 0.7152f, 0.0722f));
	
	float	SoftKneeCurveValue = SoftKneeCurve(Luminance, BrightThreshold, 0.2f);
	float	Contribution = max(SoftKneeCurveValue, Luminance - BrightThreshold) / max(Luminance, 0.0001f);

	OUTPUT[ID.xy] = float4(DownSampledColor * Contribution, 1.f);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_VerticalBlur(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	
	OUTPUT[ID.xy] = float4(GaussianBlur(TexCoord, float2(0.f, TexelSize.y)), 1.f);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_HorizontalBlur(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	
	OUTPUT[ID.xy] = float4(GaussianBlur(TexCoord, float2(TexelSize.x, 0.f)), 1.f);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_Combined(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	
	float3 OriginalColor	= OriginalTexture.SampleLevel(LinearClamp, TexCoord, 0).rgb;
	float3 BloomBlurColor	= BlurPassTexture.SampleLevel(LinearClamp, TexCoord, 0).rgb;
    
	OUTPUT[ID.xy] = float4(OriginalColor + BloomBlurColor * BloomIntensity, 1.f);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_UpSampling(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	
	float3 HalfBloom = OriginalTexture.SampleLevel(LinearClamp, TexCoord, 0).rgb;
	float3 QuarterBloom = BlurPassTexture.SampleLevel(LinearClamp, TexCoord, 0).rgb;
	
	OUTPUT[ID.xy] = float4(HalfBloom * HalfBloomWeight + QuarterBloom * QuarterBloomWeight, 1.f);
	return;
}

[numthreads(8, 8, 1)]
void CSMain_DownSampling(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY) return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
    
	OUTPUT[ID.xy] = float4(DownSampling(OriginalTexture, TexCoord, TexelSize), 1.f);
	return;
}

//////////////////////////////////////////////////////

/////////////////////////// PostProcess Filter Main Shader Function
// Distortion
float2 Distortion(float2 _UV)
{
	float2 Coord = _UV * 2.f - 1.f;
    
	float Coord2 = dot(Coord, Coord);
    
	float2 DistortedUV = Coord * (1.f + DistortionIntensity * Coord2);
    
	return (DistortedUV + 1.f) * 0.5f;
}

// Chromatic Aberration
float3 ChromaticAberration(float2 _UV)
{
	float2 UVFromCenter = _UV - 0.5f;
	float2 DistanceFromCenter = length(UVFromCenter);
    
	float2 Seperation = UVFromCenter * (DistanceFromCenter * ChromaticIntensity);
    
	float R = OriginalTexture.SampleLevel(LinearClamp, _UV - Seperation, 0).r;
	float G = OriginalTexture.SampleLevel(LinearClamp, _UV, 0).g;
	float B = OriginalTexture.SampleLevel(LinearClamp, _UV + Seperation, 0).b;

	return float3(R, G, B);
}

// Vignetting 
float3 Vignetting(float3 _Color, float2 _TexCoord)
{
	float2 UVFromCenter = _TexCoord - 0.5f;
    
	float DistanceFromCenter = dot(UVFromCenter, UVFromCenter);
	float Vignette = DistanceFromCenter * VignetteIntensity;
	Vignette = saturate(1.f - Vignette * VignetteSmoothness);
    
	return _Color * pow(Vignette, 2.f);
}

// LUT ColorGrading
float3 LUT_Filtering(float3 _Color)
{
	float3 Color = saturate(_Color);
    
	float BlueValue = Color.b * (LUT_Size - 1.f);
    
	float AdjustTile01 = floor(BlueValue);
	float AdjustTile02 = ceil(BlueValue);
    
	float2 LUT_UVOffset = Color.rg * ((LUT_Size - 1.f) / LUT_Size) + (0.5f / LUT_Size);
    
	float2 TexCoordA, TexCoordB;
	TexCoordA.x = (AdjustTile01 + LUT_UVOffset.x) / LUT_Size;
	TexCoordA.y = LUT_UVOffset.y;
    
	TexCoordB.x = (AdjustTile02 + LUT_UVOffset.x) / LUT_Size;
	TexCoordB.y = LUT_UVOffset.y;
    
	float3 LUT_ColorA = LUT_Texture.SampleLevel(LinearClamp, TexCoordA, 0).rgb;
	float3 LUT_ColorB = LUT_Texture.SampleLevel(LinearClamp, TexCoordB, 0).rgb;
    
    //float3 LUT_Mapping = _Color * ((LUT_Size - 1.f) / LUT_Size) + (0.5f / LUT_Size);
    //return LUT_Texture.Sample(SamplerClamp, LUT_Mapping);
    
	return lerp(LUT_ColorA, LUT_ColorB, frac(BlueValue));
}

// ToneMapping : Reinhard / ACESFilmic / AGXFilmic
float3 ToneMap_Reinhard(float3 _Color)
{
	return _Color.xyz / (_Color.xyz + 1.f);
}

float3 ToneMap_ACESFilm(float3 _Color)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
    
	return saturate((_Color * (a * _Color + b)) / (_Color * (c * _Color + d) + e));
}

float3 AGXFilmic(float3 _Color)
{
	//float3 X1 = _Color;
	//float3 X2 = X1 * X1;
	//float3 X3 = X2 * X1;
	//
	//return +0.155 * (1.0 - X1) * (1.0 - X1) * (1.0 - X1)
    //       + 1.019 * 3.0 * X1 * (1.0 - X1) * (1.0 - X1)
    //       + 1.385 * 3.0 * X2 * (1.0 - X1)
    //       + 1.000 * X3;
	
	float3 X = _Color;
	float3 X2 = _Color * X;
	float3 X4 = X2 * X2;

	return 15.5f * X4 * X2 - 40.14f * X4 * X + 31.96f * X4
         - 6.868f * X2 * X + 0.4298f * X2 + 0.1191f * X - 0.00232f;
}

float3 ToneMap_AGXFilm(float3 _Color)
{
	float3 AGXColor = mul(_Color, AGX_InMatrix);

	AGXColor = clamp(log2(AGXColor), Min_Luminance, Max_Luminance);
	
	float MaxEV = log2(Max_Luminance);
	float MinEV = log2(Min_Luminance);
	
	float3 LogColor = (AGXColor - MinEV) / (MaxEV - MinEV);
    
	float3 FilmColor = AGXFilmic(LogColor);
    
	return saturate(mul(FilmColor, AGX_OutMatrix));
}

////////////////////////////////////////////// OutLiner
float3 Render_ObjectEdge(float3 _Color, float2 _TexCoord)
{
	float CenterPixel = FocusingTexture.SampleLevel(LinearClamp, _TexCoord, 0).r;
	
	if (CenterPixel > 0.999f)
		return _Color;
	
	float2 MaskTexelSize = TexelSize * OutlineThickness;
	
	float RightPixel = FocusingTexture.SampleLevel(LinearClamp, _TexCoord + float2(MaskTexelSize.x, 0.f), 0).r;
	float LeftPixel = FocusingTexture.SampleLevel(LinearClamp, _TexCoord - float2(MaskTexelSize.x, 0.f), 0).r;
	float UpPixel = FocusingTexture.SampleLevel(LinearClamp, _TexCoord - float2(0.f, MaskTexelSize.y), 0).r;
	float DownPixel = FocusingTexture.SampleLevel(LinearClamp, _TexCoord + float2(0.f, MaskTexelSize.y), 0).r;

	float DepthEdge = abs(CenterPixel - RightPixel) + abs(CenterPixel - LeftPixel)
						+ abs(CenterPixel - UpPixel) + abs(CenterPixel - DownPixel);
	
	return lerp(_Color, OutlineColor.rgb, saturate(DepthEdge) * 50.f);
}

[numthreads(16, 16, 1)]
void CSMain_PostProcess(uint3 ID : SV_DispatchThreadID)
{
	[branch]
	if (ID.x >= SCREENX || ID.y >= SCREENY)
		return;
	
	float2 TexCoord = (float2(ID.xy) + 0.5f) / float2(SCREENX, SCREENY);
	
    // UV Distortion
	float2 DistortedCoord = Distortion(TexCoord);
    
    // Chromatic Aberration
	float3 FinalColor = ChromaticAberration(DistortedCoord);
    
    // ToneMapping
	FinalColor = ToneMap_ACESFilm(FinalColor);
    //FinalColor = ToneMap_Reinhard(FinalColor);
    //FinalColor = ToneMap_AGXFilm(FinalColor); // 일단 사용X
    
    // LUT ColorGrading
	FinalColor = LUT_Filtering(FinalColor);
    
    // Vignette
	FinalColor = Vignetting(FinalColor, TexCoord);
	
	FinalColor = pow(FinalColor, 1.f / 2.2f);
	
	// Edge Composite
	FinalColor = Render_ObjectEdge(FinalColor, DistortedCoord);
	
	OUTPUT[ID.xy] = float4(FinalColor, 1.f);
	return;
}

//////////////////////////////////////////////////////
