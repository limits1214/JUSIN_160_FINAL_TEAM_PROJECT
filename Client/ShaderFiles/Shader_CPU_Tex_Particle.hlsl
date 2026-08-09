#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

cbuffer CB_TIMEACCUMULATION : register(b11)
{
	float g_fAccumulationTime;
	float3 _pad;
};
struct VS_IN
{
    // Per-Vertex - 쿼드 메쉬 로컬 좌표 (-0.5~0.5), UV
    float3 vPosition : POSITION;
    float2 vTexcoord : TEXCOORD0;

    // Per-Instance - VTX_PARTICLE_INSTANCED_DATA와 바이트 레이아웃 일치.
    // "INSTANCE_" 접두사가 있어야 CResVertexShader::Load()의 리플렉션이
    // 이 필드들을 슬롯 1(인스턴스 버퍼)로 인식한다.
    float4 vWorld0 : INSTANCE_WORLD0;
    float4 vWorld1 : INSTANCE_WORLD1;
    float4 vWorld2 : INSTANCE_WORLD2;
    float4 vWorld3 : INSTANCE_WORLD3;
    float4 vColor : INSTANCE_COLOR0;
	float4 vInstOriginalEmissive : INSTANCE_EMISSIVE0;
	float4 vInstEmissive : INSTANCE_EMISSIVE1;
	float4 vInstEndEmissive : INSTANCE_EMISSIVE2;
    float2 uvOffset : INSTANCE_UVOFFSET;
    float2 uvSize : INSTANCE_UVSIZE;
    float life : INSTANCE_LIFE; // 추가 
    float maxLife : INSTANCE_MAXLIFE; // 추가
    uint iBehaviorType : INSTANCE_BEHAVIORTYPE;
};




struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexcoord : TEXCOORD0;

    float4 vColor : TEXCOORD1;
    float4 vEmissive : TEXCOORD2;
    float4 vEndEmissive : TEXCOORD3;

    uint iBehaviorType : TEXCOORD4;
    float4 vScreenPos : TEXCOORD5;
    float3 vNormal : TEXCOORD6;
    float3 vTangent : TEXCOORD7;
    float3 vWorldPos : TEXCOORD8;
    float life : TEXCOORD9; 
    float maxLife : TEXCOORD10;
};
    

VS_OUT VSMain(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;
	
	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);

    // matWorld엔 회전이 없다 (C++ 쪽에서 Scale * Translation만 곱함).
    // 중심 위치/스케일만 뽑아내고, 회전은 여기서 카메라 축으로 직접 만든다 (빌보드).
    float3 vCenter = float3(matWorld._41, matWorld._42, matWorld._43);
    float3 vRow0 = float3(matWorld._11, matWorld._12, matWorld._13);
    float fScale = length(vRow0);
    float3 vRight, vUp;
    vRight = normalize(float3(matWorld._11, matWorld._12, matWorld._13));
    vUp = normalize(float3(matWorld._21, matWorld._22, matWorld._23));

    float scaleX = length(float3(matWorld._11, matWorld._12, matWorld._13));
    float scaleY = length(float3(matWorld._21, matWorld._22, matWorld._23));

    float3 vWorldPos =
    vCenter +
    vRight * In.vPosition.x * scaleX +
    vUp * In.vPosition.y * scaleY;
	
    Out.vPosition = mul(float4(vWorldPos, 1.f), g_matViewProj);
    Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;
    Out.vColor = In.vColor;
    Out.vEmissive = In.vInstEmissive;
    Out.vEndEmissive = In.vInstEndEmissive;
    Out.vScreenPos = Out.vPosition;
    Out.iBehaviorType = In.iBehaviorType;
    Out.vWorldPos = vWorldPos;
    Out.vTangent = vRight;
    Out.vNormal = normalize(cross(vRight, vUp));
    Out.life = In.life;
    Out.maxLife = In.maxLife;
    
    return Out;
}

Texture2D g_DiffuseTexture : register(t1);
Texture2D g_NormalTexture : register(t2);
Texture2D g_DistortionTexture : register(t3);
Texture2D g_NoiseTexture : register(t4);
Texture2D g_AnyTexture : register(t5);
Texture2D g_BackgroundTex : register(t7);

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;


};
PS_OUT PSMain(VS_OUT In)
{
 
    PS_OUT Out = (PS_OUT) 0;

    float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
    float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
	float4 noise = g_NoiseTexture.Sample(LinearWrap, In.vTexcoord);
    

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));

	//if (noise.r < ratio) 
	//	discard;
    if (all(texColor.rgb <= 0.03f))
        discard;
    float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
    
    if ((In.iBehaviorType & BEHAVIOR_DISTORTION) != 0)
    {
        clip(texColor.a - 0.02f);
        clip(In.vColor.a - 0.02f);

        float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
        screenUV.x = screenUV.x * 0.5f + 0.5f;
        screenUV.y = -screenUV.y * 0.5f + 0.5f;

        float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
        float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;

        float fEdgeMask = smoothstep(0.0f, 0.3f, texColor.a) *
                          (1.0f - smoothstep(0.3f, 0.9f, texColor.a));

        float distortionStrength = 0.05f * In.vColor.a * fEdgeMask;

        distortion *= distortionStrength;
        float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion);
        float3 finalRGB = lerp(distortedBackground.rgb, texColor.rgb, texColor.a);
		
        finalRGB += lerpedEmissive.rgb * lerpedEmissive.a;

        Out.vDiffuse = float4(finalRGB, 1.0f);
        return Out;
    }
 
    float4 vFinalColor = texColor * In.vColor;
    clip(vFinalColor.a - 0.02f);


	float3 FinalColor = vFinalColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;

    Out.vDiffuse = float4(FinalColor, vFinalColor.a);
    return Out;
}

PS_OUT SMOKE(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;	
	float2 NoiseUV = In.vTexcoord;
	NoiseUV += In.life * 0.2f;
	float2 noise = g_NoiseTexture.Sample(LinearWrap, NoiseUV).rg * 2.f - 1.f;
	
	float topMask = smoothstep(0.55f, 1.f, In.vTexcoord.y);
	
	float2 distrotedUV = In.vTexcoord + float2(0.03f, 0.01f) *topMask * 0.03f;
	distrotedUV.x += noise.x * 0.03f * topMask;
	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, distrotedUV);

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	
	float fadein = smoothstep(0.f, 0.18f, ratio);
	float fadeout = 1.0f - smoothstep(0.65f, 1.f, ratio);
	float lifealpha = fadein * fadeout;
	float fAlpha = texColor.r * In.vColor.a *  lifealpha;
	
	if (any(texColor.rgb > 0.6f))
		texColor.rgb *= 2.f;
	float3 FinalColor = lerp(texColor.rgb * In.vColor.rgb, float3(1, 1, 1), 0.8f);

	Out.vDiffuse = float4(FinalColor, saturate(fAlpha*1.4f));
	return Out;
}
PS_OUT PS_SMOKE_DEF(VS_OUT In)
{
 
	PS_OUT Out = (PS_OUT) 0;
	if ((In.iBehaviorType & BEHAVIOR_SMOKE) != 0)
	{
		float maskdel = g_NormalTexture.Sample(LinearWrap, In.vTexcoord).r;
		
		maskdel = smoothstep(0.35f, 0.85f, maskdel);
		float2 maskuv = In.vTexcoord * float2(0.55549, 0.45354);
		maskuv.y += In.life * 0.3f * In.maxLife * 0.3f;
		float mask = g_DiffuseTexture.Sample(LinearWrap, maskuv).r;
		
		float t = In.life / In.maxLife;
		float plusalpha = smoothstep(0.35f,0.85f,In.vTexcoord.y);
		float fAlpha = mask.r * maskdel * In.vColor.a * plusalpha;
		float3 color = lerp( float3(0.25f, 0.55f, 0.75f), float3(0.85f, 0.95f, 1.0f), mask);
		

		Out.vDiffuse = float4(In.vColor.rgb, saturate(fAlpha * 0.7f));
		return Out;
	}
	if ((In.iBehaviorType & BEHAVIOR_SMOKEGV) != 0)
	{
		float  mask = g_NormalTexture.Sample(LinearWrap, In.vTexcoord).r;
		
		float2 distoionUV = In.vTexcoord;
		distoionUV.x += In.life * 0.2f * In.maxLife * 0.2f;
		float2 distoion = g_DistortionTexture.Sample(LinearWrap, distoionUV).rg * 2.f -1.f;
		
		float2 wispsUV = In.vTexcoord * float2(0.549206f, 0.453968f);
		float bendMask = 1.f - smoothstep(0.45f, 1.f, In.vTangent.y);
		wispsUV += distoion * 0.02 * bendMask; 
		float4 wisps = g_DiffuseTexture.Sample(LinearWrap, wispsUV);
		
		
		float t = In.life / In.maxLife;
		
		float lifeFade = 1.f - smoothstep(0.65f, 0.8f, t);
		float smokeShape = pow(saturate(wisps.r),0.4f);
		float topFade = smoothstep(0.f, 0.3f, In.vTexcoord.y);
		
		float fAlpha = wisps.r * mask * In.vColor.a * lifeFade * topFade;
		Out.vDiffuse = float4(In.vColor.rgb * 1.4f, saturate(fAlpha * 0.34f));
		return Out;		
	}
	else if ((In.iBehaviorType & BEHAVIOR_SMOKEGW) != 0)
	{
		float mask = g_NormalTexture.Sample(LinearWrap, In.vTexcoord).r;
		
		
		float2 normalUV = In.vTexcoord;
		normalUV.y += In.life * 0.2f * In.maxLife * 0.2f;
		float normal = g_NormalTexture.Sample(LinearWrap, normalUV).r;
		
		float4 wisps = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
		
		float life = 1.f - (In.life * 0.02f);
		float alpha = saturate(life) * mask;
		if (wisps.r < 0.2f)
			discard;
		Out.vDiffuse = float4(wisps.rgb * 1.4f * In.vColor.rgb, alpha * 0.5f);
		return Out;
	}
	
	
	
	return Out;
}
PS_OUT RemoveBlack(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));

	float4 texColor = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);

	if (all(texColor.rgb < 0.2f))
		discard;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB, texColor.a * In.vColor.a);
	return Out;
}

PS_OUT RemoveBlackScrollXY(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
		    
	float2 uv = In.vTexcoord;
	
	float2 packedUV = uv;
	packedUV.x += In.life * 3.03f;
	//packedUV.y += In.life * 1.03f;
	//packedUV.y *= 3.f;
	
	float4 tex = g_DiffuseTexture.Sample(LinearWrap, float2(packedUV.x, packedUV.y));
	
	
	
	float ratio = saturate(1.0f - (In.life / max(In.maxLife, 0.0001f)));

	//float4 texColor = g_DiffuseTexture.Sample(LinearWrap, packedUV);

	float intensity = max(tex.r,(max(tex.g, tex.b)));
	float alpha = smoothstep(0.02f, 0.2f, intensity);

	float3 color = tex.rgb * In.vColor.rgb;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 finalRGB = color.rgb + lerpedEmissive.rgb * lerpedEmissive.a;

	Out.vDiffuse = float4(finalRGB, alpha * In.vColor.a);

	//if (all(texColor.rgb < 0.01f))
	//	discard;
	
	return Out;
}
PS_OUT PlayerDashSmoke2(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	static const float2 AtlasCount = float2(8.f, 8.f);

    // 이미 계산되어 전달된 8×8 Atlas UV
	float2 atlasUV = In.vTexcoord;

    // 현재 플립북 셀과 셀 내부 0~1 UV 복원
	float2 atlasPosition = atlasUV * AtlasCount;
	float2 cellIndex = floor(atlasPosition);
	float2 localUV = frac(atlasPosition);
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));


    /*
     * Normal은 플립북 Atlas UV가 아니라
     * 셀 내부 UV를 기준으로 별도로 스크롤한다.
     */
	float2 normalUV =
        localUV * float2(1.2f, 1.2f) +
        float2(-g_fAccumulationTime * 0.6f, 0.f);

	float2 distortion =
        g_NormalTexture.Sample(LinearWrap, normalUV).rg * 2.f - 1.f;

    // 강하게 적용하면 Smoke 형태가 망가지므로 작게 시작
	const float distortionStrength = 0.012f;

	float2 distortedLocalUV =
        localUV + distortion * distortionStrength;

    // 옆 플립북 프레임이 섞이는 것을 방지
	distortedLocalUV = clamp(
        distortedLocalUV,
        float2(0.02f, 0.02f),
        float2(0.98f, 0.98f)
    );

	
    // 왜곡된 셀 내부 UV를 다시 Atlas UV로 변환
	float2 distortedAtlasUV =
        (cellIndex + distortedLocalUV) / AtlasCount;
	//distortedAtlasUV.x =In.life * 0.03f;
    // SmokeMedium 플립북 샘플링
    // 가능하면 Diffuse 전용 Clamp 샘플러 권장


	float4 smokeTex =
        g_DiffuseTexture.Sample(LinearClamp, distortedAtlasUV);

    /*
     * WindNoise_D는 플립북 UV가 아닌 별도의 UV로 읽는다.
     * 대시 방향이 텍스처의 X축이라면 X 방향으로 스크롤.
     */
	float2 noiseUV =
        localUV * float2(1.5f, 1.f) +
        float2(-g_fAccumulationTime * 0.8f, 0.f);

	float noise =
        g_NoiseTexture.Sample(LinearWrap, noiseUV).r;

	float noiseMask =
        smoothstep(0.3f, 0.7f, noise);

    // 노이즈를 너무 강하게 곱하면 연기가 사라지므로 25%만 적용
	float breakup =
        lerp(1.f, noiseMask, 0.25f);

    // 알파 채널이 정상적으로 들어 있다면 이것을 사용
	float smokeMask = smokeTex.a;

    // 알파 채널이 항상 1이라면 위 라인 대신 아래 코드 사용
    // float smokeMask = max(
    //     smokeTex.r,
    //     max(smokeTex.g, smokeTex.b)
    // );

    // 파티클 생성/소멸 페이드
	float fadeIn = smoothstep(0.f, 0.08f, ratio);
	float fadeOut = 1.f - smoothstep(0.65f, 1.f, ratio);
	float lifeFade = fadeIn * fadeOut;

	float alpha =
        smokeMask *
        breakup *
        lifeFade *
        In.vColor.a;

    // Smoke 텍스처 RGB를 사용
	float3 color =
        smokeTex.rgb * In.vColor.rgb;

	float4 lerpedEmissive =
        lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB =
        color +
        lerpedEmissive.rgb * lerpedEmissive.a;

	Out.vDiffuse = float4(finalRGB, alpha);

    // 완전히 투명한 픽셀의 오버드로 감소
	clip(alpha - 0.001f);

	return Out;
}
PS_OUT PSOnlyForDistortion(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;


	float2 screenUV = In.vScreenPos.xy / In.vScreenPos.w;
	screenUV.x = screenUV.x * 0.5f + 0.5f;
	screenUV.y = -screenUV.y * 0.5f + 0.5f;

	float4 vDistortionColor = g_DistortionTexture.Sample(LinearWrap, In.vTexcoord);
	
	if (vDistortionColor.r <0.51f)
		discard;
	float2 distortion = vDistortionColor.rg * 2.0f - 1.0f;
	float distortionStrength = 0.15f * In.vColor.a ;

	distortion *= distortionStrength;
	float4 distortedBackground = g_BackgroundTex.Sample(LinearClamp, screenUV + distortion); 
	

	Out.vDiffuse = float4(distortedBackground.rgb, 1.0f);
	return Out;
	
}
PS_OUT PSSphereShield(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float3 normalSample = g_NormalTexture.Sample( LinearClamp,In.vTexcoord ).rgb;

	float3 sphereNormal =
        normalize(normalSample * 2.0f - 1.0f);

	float circleMask = g_DiffuseTexture.Sample(LinearClamp,In.vTexcoord).r;

	float3 viewDir =
        float3(0.f, 0.f, 1.f);

	float fresnel = 1.0f - saturate(dot(sphereNormal, viewDir));

	float rim = pow(fresnel, 5.0f);

	float3 rimColor =  float3(0.2f, 0.65f, 1.0f);

	float3 centerColor = float3(0.1f, 0.25f, 0.7f);

	float3 finalRGB =
        centerColor * circleMask * 0.3f +
        rimColor * rim * circleMask * 6.0f;

	float alpha =  saturate(circleMask * 0.25f + rim * circleMask);

	clip(circleMask - 0.01f);
	float3 color = In.vColor.rgb + In.vEmissive.rgb * In.vEmissive.a;	
	float finalAlpha = saturate(alpha - ratio);
	Out.vDiffuse = float4(finalRGB + color, finalAlpha);

	return Out;
}
PS_OUT BlindFlare(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float4 flareTex = g_DiffuseTexture.Sample(LinearClamp, In.vTexcoord);

	float2 centeredUV = In.vTexcoord * 2.0f - 1.0f;
	float distanceToCenter = length(centeredUV);
	float angle = atan2(centeredUV.y, centeredUV.x);

	float flareMask = dot(flareTex.rgb, float3(0.299f, 0.587f, 0.114f));
	flareMask = saturate(flareMask);

	float angleNoise = sin(angle * 7.0f + 0.5f);
	angleNoise += sin(angle * 13.0f - 1.7f) * 0.5f;
	angleNoise = angleNoise * 0.5f + 0.5f;
	angleNoise = smoothstep(0.25f, 0.85f, angleNoise);

	float radialFade = saturate(1.0f - distanceToCenter);
	float rayMask = angleNoise * radialFade;
	rayMask *= smoothstep(1.0f, 0.05f, distanceToCenter);

	float hue = angle / 6.283185f + 0.5f;
	float3 chromaColor;
	chromaColor.r = sin(hue * 6.283185f) * 0.5f + 0.5f;
	chromaColor.g = sin(hue * 6.283185f + 2.094f) * 0.5f + 0.5f;
	chromaColor.b = sin(hue * 6.283185f + 4.188f) * 0.5f + 0.5f;

	float3 warmColor = float3(1.0f, 0.42f, 0.08f);
	float3 rayColor = lerp(warmColor, chromaColor, 0.25f);

	float ratio = saturate( In.life / max(In.maxLife, 0.0001f));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float intensity = lerpedEmissive.a;

	float coreMask = pow(saturate(1.0f - distanceToCenter), 4.0f);
	float3 softFlare = warmColor * flareMask * intensity;
	float3 coloredRays = rayColor * rayMask * intensity * 0.7f;
	float3 whiteCore = float3(1.0f, 0.9f, 0.75f) * coreMask * intensity * 1.5f;

	float3 finalColor = (softFlare + coloredRays + whiteCore) * In.vColor.rgb;
	float finalAlpha = saturate(max(flareMask, rayMask) * In.vColor.a);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSPortal(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float2 flowUV1 = In.vTexcoord + float2(g_fAccumulationTime * 0.04f, -g_fAccumulationTime * 0.03f);
	float2 flowUV2 = In.vTexcoord * 1.4f + float2(-g_fAccumulationTime * 0.025f, g_fAccumulationTime * 0.035f);

	float2 flow1 = g_DistortionTexture.Sample(LinearWrap, flowUV1).rg * 2.0f - 1.0f;
	float2 flow2 = g_DistortionTexture.Sample(LinearWrap, flowUV2).rg * 2.0f - 1.0f;
	float2 flowDirection = (flow1 + flow2 * 0.5f) / 1.5f;

	float flowStrength = 1.06f; 
	float2 distortedUV = In.vTexcoord + flowDirection * flowStrength;

	float4 colorTex = g_DiffuseTexture.Sample(LinearWrap, distortedUV);
	float circleMask = g_AnyTexture.Sample(LinearClamp, In.vTexcoord).r;
	circleMask = smoothstep(0.05f, 0.2f, circleMask);

	float2 centeredUV = In.vTexcoord * 2.0f - 1.0f;
	float distanceToCenter = length(centeredUV);


	float luminance = dot(colorTex.rgb, float3(0.299f, 0.587f, 0.114f));

	float3 originalColor = colorTex.rgb;
	float3 saturatedColor = lerp(luminance.xxx, originalColor, 1.35f);
	float3 portalColor = max(saturatedColor, 0.0f);


	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 emissiveColor = lerpedEmissive.rgb * lerpedEmissive.a;

	float3 finalColor = portalColor * In.vColor.rgb;
	finalColor += originalColor * emissiveColor * circleMask;
	finalColor *= circleMask;

	float finalAlpha = circleMask * In.vColor.a * ObjectAlpha;

	clip(finalAlpha - 0.01f);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
PS_OUT PSCircleMask(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;


	float4 colorTex = g_DiffuseTexture.Sample(LinearWrap, In.vTexcoord);
	if (all(colorTex.rgb < 0.2f))
		discard;
	float circleMask = g_AnyTexture.Sample(LinearClamp, In.vTexcoord).r;
	circleMask = smoothstep(0.05f, 0.2f, circleMask);

	float2 centeredUV = In.vTexcoord * 2.0f - 1.0f;
	float distanceToCenter = length(centeredUV);
	


	float luminance = dot(colorTex.rgb, float3(0.299f, 0.587f, 0.114f));

	float3 originalColor = colorTex.rgb;
	float3 saturatedColor = lerp(luminance.xxx, originalColor, 1.35f);
	float3 portalColor = max(saturatedColor, 0.0f);


	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 emissiveColor = lerpedEmissive.rgb * lerpedEmissive.a;

	float3 finalColor = portalColor * In.vColor.rgb;
	finalColor += originalColor * emissiveColor * circleMask;
	finalColor *= circleMask;

	float finalAlpha = circleMask * In.vColor.a * ObjectAlpha;
	finalAlpha = pow(saturate(finalAlpha), 2.2f);
	
	clip(finalAlpha - 0.01f);

	Out.vDiffuse = float4(finalColor, finalAlpha);
	return Out;
}
