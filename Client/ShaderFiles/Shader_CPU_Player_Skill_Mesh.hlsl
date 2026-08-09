#include "../../Engine/ShaderFiles/Particle/Particle_Common_Struct_Func.hlsl"

#define MAX_LIGHT_COUNT     8
#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2
cbuffer CB_TIMEACCUMULATION : register(b11)
{
	float g_fAccumulationTime;
	float3 _pad;
};
struct VS_IN
{
	float3 vPosition : POSITION;
	float3 vNormal : NORMAL;
	float3 vTangent : TANGENT;
	float3 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;

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

	float life : INSTANCE_LIFE;
	float maxLife : INSTANCE_MAXLIFE;

	uint iBehaviorType : INSTANCE_BEHAVIORTYPE;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float2 vTexcoord : TEXCOORD0;
	float3 vWorldPos : TEXCOORD1;

	float life : TEXCOORD2;
	float maxLife : TEXCOORD3;

	nointerpolation uint iBehaviorType : TEXCOORD4;

	float4 vScreenPos : TEXCOORD5;
	float4 vColor : COLOR0;

	float3 vNormal : NORMAL0;
	float3 vTangent : TANGENT0;
	float3 vBinormal : BINORMAL0;

	float4 vEmissive : EMISSIVE0;
	float4 vEndEmissive : EMISSIVE1;
};

VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out = (VS_OUT) 0;

	float4x4 matWorld = float4x4(In.vWorld0, In.vWorld1, In.vWorld2, In.vWorld3);
	float4 worldPosition = mul(float4(In.vPosition, 1.f), matWorld);

	Out.vPosition = mul(worldPosition, g_matViewProj);
	Out.vWorldPos = worldPosition.xyz;
	Out.vScreenPos = Out.vPosition;
	Out.vTexcoord = In.uvOffset + In.vTexcoord * In.uvSize;

	float3x3 world3x3 = (float3x3) matWorld;

	Out.vNormal = normalize(mul(In.vNormal, world3x3));
	Out.vTangent = normalize(mul(In.vTangent, world3x3));
	Out.vBinormal = normalize(mul(In.vBinormal, world3x3));

	Out.vColor = In.vColor;
	Out.vEmissive = In.vInstEmissive;
	Out.vEndEmissive = In.vInstEndEmissive;

	Out.life = In.life;
	Out.maxLife = In.maxLife;
	Out.iBehaviorType = In.iBehaviorType;

	return Out;
}

Texture2D AlbedoMap : register(t0);
Texture2D NormalMap : register(t1);
Texture2D SMROMap : register(t2);
Texture2D EmissiveMap : register(t3);
Texture2D NoiseMap : register(t5);
Texture2D DistortionMap : register(t6);
Texture2D g_BackgroundTex : register(t7);
Texture2D AnyTextureMap : register(t8);

struct PS_OUT
{
	float4 vDiffuse : SV_TARGET0;
};

PS_OUT PSMain(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float4 AlbedoTex = AlbedoMap.Sample(LinearWrap, In.vTexcoord);
	AlbedoTex *= In.vColor;

	clip(AlbedoTex.a - 0.05f);

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float3 Albedo = pow(max(AlbedoTex.rgb, 0.f), 2.2f);

	float3 WorldNormal = Compute_WorldNormal(NormalMap, In.vTexcoord, In.vNormal, In.vTangent);
	WorldNormal = normalize(WorldNormal * NormalIntensity);

	float3 V = normalize(g_vCamPos - In.vWorldPos);
	float NDV = max(dot(WorldNormal, V), 0.f);

	float3 SMRO = SMROMap.Sample(LinearWrap, In.vTexcoord).rgb;
	float fMetallic = SMRO.r * MetallicIntensity;
	float fRoughness = SMRO.g * RoughnessIntensity;
	float fAmbient = SMRO.b * AmbientIntensity;

	float3 MBR = lerp(float3(0.04f, 0.04f, 0.04f), Albedo, fMetallic);
	float3 LightAccumulation = float3(0.f, 0.f, 0.f);

    [unroll(MAX_LIGHT_COUNT)]
	for (int i = 0; i < LightCount; ++i)
	{
		float3 L;
		float3 Radiance;

		if (!Compute_DynamicLight(AffectedLight[i], In.vWorldPos, L, Radiance))
			continue;

		float RawNDL = dot(WorldNormal, L);

		if (RawNDL <= 0.f)
			continue;

		float NDL = saturate(RawNDL);
		float3 H = normalize(V + L);

		float D = DistributionGGX(WorldNormal, H, fRoughness);
		float3 F = FresnelSchlick(max(dot(H, V), 0.f), MBR);
		float V_Spec = VisibilitySmithJointGGX(NDV, NDL, fRoughness);

		float3 Specular = D * F * V_Spec * SpecularIntensity;
		float3 kS = F;
		float3 kD = (1.f - kS) * (1.f - fMetallic);
		float3 Diffuse = kD * Albedo / PI;

		LightAccumulation += (Diffuse + Specular) * Radiance * NDL;
	}

	float3 texEmissive = EmissiveMap.Sample(LinearWrap, In.vTexcoord).rgb;
	texEmissive = pow(max(texEmissive, 0.f), 2.2f);

	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 instanceEmissive = lerpedEmissive.rgb * lerpedEmissive.a;

	float3 constantAmbient = Albedo * 0.05f * fAmbient;
	float3 finalColor = constantAmbient + LightAccumulation + texEmissive + instanceEmissive;

	Out.vDiffuse = float4(finalColor, AlbedoTex.a);

	return Out;
}

PS_OUT PSAccio(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;
	
	float2 diffuseUV =
        In.vTexcoord * float2(1.f, 1.f) + float2(0, g_fAccumulationTime * 0.2f);
	
	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));

	
	float4 texColor = AlbedoMap.Sample(LinearWrap, diffuseUV);

	if (all(texColor.rgb < 0.2f))
		discard;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);

	float3 finalRGB = texColor.rgb * In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;
	
	Out.vDiffuse = float4(finalRGB, texColor.a * In.vColor.a);
	return Out;
}

PS_OUT PSDepulso(VS_OUT In)
{
	PS_OUT Out = (PS_OUT) 0;

	float ratio = saturate(In.life / max(In.maxLife, 0.0001f));
	float dist = In.vTexcoord.y; // V: 구멍(0) ~ 바깥테두리(1)

	float bandWidth = 0.1f; // 링(띠)의 두께 — 값 조절 가능
	float edgeSoft = 0.05f;

    // 바깥 경계: 0에서 시작해서 1을 넘어(1+bandWidth)까지 이동 -> 끝에서 자연스럽게 사라짐
	float outerRadius = lerp(0.f, 1.f + bandWidth, ratio);
    // 안쪽 경계: 바깥보다 bandWidth만큼 뒤처져서 따라옴
	float innerRadius = outerRadius - bandWidth;

	float outerEdge = 1.f - smoothstep(outerRadius - edgeSoft, outerRadius, dist);
	float innerEdge = smoothstep(innerRadius - edgeSoft, innerRadius, dist);
	float alpha = outerEdge * innerEdge;

	float noise = AlbedoMap.Sample(LinearWrap, In.vTexcoord).r;
	alpha *= noise * In.vColor.a;
	float4 lerpedEmissive = lerp(In.vEmissive, In.vEndEmissive, ratio);
	float3 finalColor = In.vColor.rgb + lerpedEmissive.rgb * lerpedEmissive.a;		
	Out.vDiffuse = float4(finalColor, alpha);
	clip(alpha - 0.01f);

	return Out;
}
