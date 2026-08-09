#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

float g_fNoiseStrength = 0.3f; // 0~1
float g_fGlowStrength = 0.7f; // 0~3
float g_fLengthGlow = 0.3f; // 0~2

float g_fDissolve = 0;
float g_fUseNoise = 0;
float g_fUseDistortion = 0;
float g_fUseDissolve = 0;





cbuffer CB_SCROLL : register(b10)
{
	float g_fScrollOffset;
	float g_fAccumulationTime;
	uint g_iCurrentFrame;
	uint g_iFlipbookRows;
	uint g_iFlipbookColumns;
	float3 g_fPadding;
}
float2 ComputeFlipbookUV(float2 uv)
{
	uint rows = max(g_iFlipbookRows, 1u);
	uint columns = max(g_iFlipbookColumns, 1u);
	uint totalFrames = rows * columns;
	uint frameIndex = min(g_iCurrentFrame, totalFrames - 1u);

	uint columnIndex = frameIndex % columns;
	uint rowIndex = frameIndex / columns;

	float2 cellSize = 1.f / float2(columns, rows);
	float2 localUV = frac(uv);

	return (localUV + float2(columnIndex, rowIndex)) * cellSize;
}
struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0; // a에 나이 기반 페이드(fLifeRatio)가 실려 있음
    float4 vEmissive : COLOR1;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vUV : TEXCOORD0;
    float4 vColor : COLOR0;
    float4 vEmissive : COLOR1;
    float4 vScreenPos : TEXCOORD1;
};

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    Out.vPosition = mul(float4(In.vPosition, 1.f), g_matViewProj);
    Out.vUV = In.vUV;
    Out.vColor = In.vColor;
    Out.vEmissive = In.vEmissive;
    Out.vScreenPos = Out.vPosition;
    return Out;
}

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_BackgroundTex : register(t7);
Texture2D g_AnyTexture : register(t8);



struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
};
//
PS_OUT PSMain(VS_OUT In) : SV_TARGET
{
	PS_OUT Out = (PS_OUT) 0;

	
	float2 uv = In.vUV;
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, float2(uv.x, uv.y));
	float4 distortionTex = g_DistortionTexture.Sample(LinearWrap, float2(uv.x * 2, uv.y));
        
	if (all(tex.rgb < 0.1f))
		discard;
	float noise = g_NoiseTexture.Sample(LinearWrap, float2(uv.x * 2 + g_fScrollOffset, uv.y)).r;

	float center = 1 - smoothstep(0, 1, abs(uv.y - 0.5) * 2);

	center = pow(center, 0.8);		

	float glow = 1 + center * g_fGlowStrength;
    
	float lengthGlow = pow(In.vColor.a, 2);
	glow *= 1 + lengthGlow * g_fLengthGlow;
	if (g_fUseNoise > 0.5)
	{
		glow *= 1 + (noise - 0.5) * g_fNoiseStrength;
	}
    
	if (g_fUseDissolve > 0.5)
	{
		float progress = 1 - In.vColor.a;

		float alpha = smoothstep(0, 0.15, noise - progress);

		tex.a *= alpha;
	}
    
	float edge = 1 - smoothstep(0.5, 1, abs(uv.y - 0.5) * 2);
	tex.a *= edge;
    
	float4 color = tex * In.vColor;

	color.rgb *= glow;
	color.rgb *= In.vColor.a;

	color.rgb += In.vEmissive.rgb * In.vEmissive.a;
	color.a *= In.vColor.a;
	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;

    
	if (g_fUseDistortion > 0.5)
	{
		clip(color.a - 0.02);

    // 왜곡 텍스처에서 실제 방향 벡터를 샘플링 (스크롤도 같이 적용 가능)
		float2 distortionUV = float2(uv.x * 2 + g_fScrollOffset, uv.y);
		float2 distortionSample = g_DistortionTexture.Sample(LinearWrap, distortionUV).rg;

    // 0~1 범위를 -1~1로 remap 해서 양방향 왜곡이 되게
		float2 distortion = (distortionSample * 2.0f - 1.0f) * 5.f; // 0.05 = 왜곡 강도, 조절 필요

    // 트레일 알파가 강한 곳일수록 더 많이 왜곡되도록
		distortion *= color.a;

		float2 distortedUV = screenUV * float2(0.5, -0.5) + 0.5 + distortion;

		float4 background = g_BackgroundTex.Sample(LinearClamp, distortedUV);
        

    // 배경 굴절 위에 원본 트레일 색상을 얹어서 같이 보이게
		background.rgb += color.rgb;
		background.a = saturate(background.a + color.a);

		Out.vDiffuse = background;

		return Out;
	}
	else
	{
		Out.vDiffuse = color;
	}
	
	return Out;

}
PS_OUT PSPlayerDash(VS_OUT In) : SV_TARGET
{
	PS_OUT Out = (PS_OUT) 0;

	float2 uv = In.vUV;
	uv.x += g_fAccumulationTime * 3.33f;
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, float2(uv.x, uv.y * 3));
	float4 distortionTex = g_DistortionTexture.Sample(LinearWrap, float2(uv.x * 2, uv.y));
        
	if (all(tex.rgb < 0.1f))
		discard;
	float noise = g_NoiseTexture.Sample(LinearWrap, float2(uv.x * 2 + g_fScrollOffset, uv.y)).r;

	float center = 1 - smoothstep(0, 1, abs(uv.y - 0.5) * 2);

	center = pow(center, 0.8);

	float glow = 1 + center * g_fGlowStrength;
    
	float lengthGlow = pow(In.vColor.a, 2);
	glow *= 1 + lengthGlow * g_fLengthGlow;

	float edge = 1 - smoothstep(0.5, 1, abs(uv.y - 0.5) * 2);
	tex.a *= edge;
    
	float4 color = tex * In.vColor;

	color.rgb *= glow;
	color.rgb *= In.vColor.a;

	color.rgb += In.vEmissive.rgb * In.vEmissive.a * In.vColor.a;
	color.a *= In.vColor.a;

	Out.vDiffuse = color;
	return Out;
}
PS_OUT PSPlayerDash1(VS_OUT In) : SV_TARGET
{

	PS_OUT Out = (PS_OUT) 0;
	float2 flipbookUV = ComputeFlipbookUV(In.vUV);
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, flipbookUV);

	float3 emissive = In.vEmissive.rgb * In.vEmissive.a;

	float4 vFinalColor = float4(tex.rgb + emissive, tex.a);
	Out.vDiffuse = vFinalColor;

	return Out;

}

PS_OUT PSRemoveBlack(VS_OUT In)
{
    PS_OUT Out = (PS_OUT)0;

    float2 uv = In.vUV;

    // 트레일을 따라 서로 다른 위치가 좌우로 흔들림
    float wave1 = sin(uv.x * 8.0f + g_fAccumulationTime * 6.0f);
    float wave2 = sin(uv.x * 17.0f - g_fAccumulationTime * 9.0f);

    // V가 트레일 폭 방향
    uv.y += wave1 * 0.135f;
    uv.y += wave2 * 0.112f;

    float4 texColor =
        g_DiffuseTexture.Sample(LinearClamp, uv);

    float mask =
        max(texColor.r, max(texColor.g, texColor.b));

    clip(mask - 0.2f);

    float3 baseColor =
        texColor.rgb * In.vColor.rgb;

    float3 emissive =
        In.vEmissive.rgb *
        In.vEmissive.a *
        mask;

    Out.vDiffuse = float4(
        baseColor + emissive,
        mask * texColor.a * In.vColor.a
    );

    return Out;
}
PS_OUT PSMarble(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

    // 1) 위습 텍스처 샘플링 — 필요하면 UV를 시간에 따라 흘러가듯 스크롤
	float2 wispUV = In.vUV + float2(-g_fAccumulationTime * 0.3f, 0.f);
	float wispMask = g_DiffuseTexture.Sample(LinearWrap, wispUV).r; // 검은 배경(0) + 흰 줄기(1)


    // 3) 색은 인스턴스 이미시브 컬러로 (또는 In.vColor.rgb로 바꿔도 됨)
	float3 tintColor = In.vEmissive.rgb * In.vEmissive.a;

	float alpha = wispMask * In.vColor.a;
	float3 finalColor = tintColor * wispMask;

	Out.vDiffuse = float4(finalColor, alpha);
	return Out;
}
PS_OUT PSDarkWispTrail(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 uv = In.vUV;
	uv.x -= g_fAccumulationTime * 0.15f;

	float2 noiseUV1 = float2(uv.x * 1.5f - g_fAccumulationTime * 0.2f, uv.y * 2.f);
	float2 noiseUV2 = float2(uv.x * 3.f + g_fAccumulationTime * 0.13f, uv.y * 4.f);

	float2 noise1 = g_DistortionTexture.Sample(LinearWrap, noiseUV1).rg * 2.f - 1.f;
	float2 noise2 = g_DistortionTexture.Sample(LinearWrap, noiseUV2).rg * 2.f - 1.f;
	float2 distortion = noise1 * 0.7f + noise2 * 0.3f;

	float centerMask = 1.f - abs(uv.y * 2.f - 1.f);
	centerMask = saturate(centerMask);

	uv.x += distortion.x * 0.04f;
	uv.y += distortion.y * 0.12f * centerMask;

	

	float3 textureColor = g_DiffuseTexture.Sample(LinearWrap, In.vUV).rgb;
	float luminance = dot(textureColor, float3(0.299f, 0.587f, 0.114f));		

	float bodyMask = smoothstep(0.01f, 0.2f, luminance);
	float brightMask = smoothstep(0.4f, 0.85f, luminance);
	   
	float breakNoise = g_DistortionTexture.Sample(LinearWrap, noiseUV1 * 0.7f).b;
	float breakMask = smoothstep(0.45f, 0.75f, breakNoise);

	float alpha = bodyMask * breakMask * In.vColor.a;
	float3 blackColor = float3(0.001f, 0.001f, 0.002f);
	float3 emissiveColor = In.vEmissive.rgb * In.vEmissive.a;
	float3 finalColor = blackColor + emissiveColor * brightMask * 8.f;
	
	clip(alpha - 0.01f);

	Out.vDiffuse = float4(finalColor, alpha);
	return Out;
}
