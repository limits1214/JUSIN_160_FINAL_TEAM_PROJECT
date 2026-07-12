#include "../ShaderDefines.hlsl"
// Color LUT + ToneMapping + Vignetting + Noise

// 지금까지 씬에 그려진 텍스쳐 바인딩
Texture2D SceneColorTexture : register(t0);
Texture2D LUT_Texture       : register(t1); // LUT 필터 3D 텍스쳐

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
static const float Min_Luminance = 0.00018442211f;
static const float Max_Luminance = 16.f;

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
    
    float R = SceneColorTexture.Sample(LinearClamp, _UV - Seperation).r;
    float G = SceneColorTexture.Sample(LinearClamp, _UV).g;
    float B = SceneColorTexture.Sample(LinearClamp, _UV + Seperation).b;

    return float3(R, G, B);
}

// Vignetting 
float3 Vignetting(float3 _Color, float2 _UV)
{
    float2 UVFromCenter = _UV - 0.5f;
    
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
    
    float3 LUT_ColorA = LUT_Texture.Sample(LinearClamp, TexCoordA).rgb;
    float3 LUT_ColorB = LUT_Texture.Sample(LinearClamp, TexCoordB).rgb;
    
    //float3 LUT_Mapping = _Color * ((LUT_Size - 1.f) / LUT_Size) + (0.5f / LUT_Size);
    //return LUT_Texture.Sample(SamplerClamp, LUT_Mapping);
    
    return lerp(LUT_ColorA, LUT_ColorA, frac(BlueValue));
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
    float3 X1 = _Color;
    float3 X2 = X1 * X1;
    float3 X3 = X2 * X1;

    return +0.155 * (1.0 - X1) * (1.0 - X1) * (1.0 - X1)
           + 1.019 * 3.0 * X1 * (1.0 - X1) * (1.0 - X1)
           + 1.385 * 3.0 * X2 * (1.0 - X1)
           + 1.000 * X3;
}

float3 ToneMap_AGXFilm(float3 _Color)
{
    float3 AGXColor = mul(_Color, AGX_InMatrix);

    AGXColor = clamp(AGXColor, Min_Luminance, Max_Luminance);
    float3 LogColor = (log2(AGXColor) - log2(Min_Luminance)) / (log2(Max_Luminance) - log2(Min_Luminance));
    
    float3 FilmColor = AGXFilmic(LogColor);
    
    return mul(FilmColor, AGX_OutMatrix);
}

struct PS_IN
{
    float4 Position : SV_POSITION;
    float2 TexCoord : TEXCOORD0;
};
struct PS_OUT
{
    vector Diffuse : SV_TARGET0;
};

PS_OUT PSMain(PS_IN IN)
{
    PS_OUT OUT;
    
    // UV Distortion
    float2 DistortedCoord = Distortion(IN.TexCoord);
    
    // Chromatic Aberration
    float3 FinalColor = ChromaticAberration(DistortedCoord);
    
    // ToneMapping
    //FinalColor = ToneMap_ACESFilm(FinalColor);
    FinalColor = ToneMap_Reinhard(FinalColor);
    //FinalColor = ToneMap_AGXFilm(FinalColor); // 일단 사용X
    
    // LUT ColorGrading
    FinalColor = LUT_Filtering(FinalColor);
    
    // Vignette
    FinalColor = Vignetting(FinalColor, IN.TexCoord);
    OUT.Diffuse = float4(pow(FinalColor, 1.f / 2.2f), 1.f);
    
    return OUT;
}
