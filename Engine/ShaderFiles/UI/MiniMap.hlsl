#include "../ShaderDefines.hlsl"

Texture2D g_MaskTex : register(t0);   // t0: 프레임 (나침반)
Texture2D g_MinimapTex : register(t1);
Texture2D g_BattleZoneTex : register(t2);

cbuffer CB_MINIMAP : register(b10)
{
	float2 g_mapOffset;
	float  g_mapRotation;
	float  g_mapScale;
	uint   g_mapMode;
	float  g_smokeIntensity;
	float  g_smokeSpeed;
	float  g_smokeTime;
	uint   g_battleZoneCount;
	float3 g_battleZonePadding;
	float4 g_battleZones[8];
};

static const uint MINIMAP_MODE_WORLD_MAP = 0;
static const uint MINIMAP_MODE_DUNGEON_FOG = 1;

struct VS_IN
{
	float3 posL : POSITION;
	float2 uv : TEXCOORD;
};

struct PS_IN
{
	float4 posH : SV_POSITION;
	float2 uv : TEXCOORD;
};

PS_IN VSMain(VS_IN vin)
{
	PS_IN output;
	output.posH = mul(float4(vin.posL, 1.f), g_matWVP);
	output.uv = vin.uv;
	return output;
}

float GetLuminance(float3 color)
{
	return dot(color, float3(0.299f, 0.587f, 0.114f));
}

float3 ApplyBattleZones(float2 uv, float3 baseColor)
{
	const float3 battleZoneColor = float3(1.f, 0.025f, 0.015f);
	const uint zoneCount = min(g_battleZoneCount, 8u);

	for (uint i = 0; i < zoneCount; ++i)
	{
		const float2 centerUV = g_battleZones[i].xy;
		const float diameterUV = max(g_battleZones[i].z, 0.0001f);
		const float zoneOpacity = saturate(g_battleZones[i].w);
		const float2 zoneUV = (uv - centerUV) / diameterUV + 0.5f;

		if (all(zoneUV >= 0.f) && all(zoneUV <= 1.f))
		{
			const float zoneAlpha =
				g_BattleZoneTex.Sample(LinearClamp, zoneUV).a * zoneOpacity;
			baseColor = lerp(baseColor, battleZoneColor, zoneAlpha);
		}
	}

	return baseColor;
}

float4 RenderDungeonFog(float2 uv, float maskAlpha)
{
	const float time = g_smokeTime * g_smokeSpeed;

	float2 flowUV = uv * 1.15f + g_mapOffset +
		time * float2(0.008f, 0.004f);
	float2 flow = (g_MinimapTex.Sample(LinearWrap, flowUV).rg * 2.f - 1.f) * 0.035f;

	float2 smokeUV1 = uv * 0.82f + g_mapOffset +
		time * float2(-0.004f, 0.007f) + flow;
	float2 smokeUV2 = uv * 1.37f + g_mapOffset +
		time * float2(0.006f, -0.003f) - flow * 0.55f;

	float smoke1 = GetLuminance(g_MinimapTex.Sample(LinearWrap, smokeUV1).rgb);
	float smoke2 = GetLuminance(g_MinimapTex.Sample(LinearWrap, smokeUV2).rgb);
	float density = smoothstep(0.2f, 0.85f, smoke1 * 0.55f + smoke2 * 0.45f);
	density = saturate(density * g_smokeIntensity);

	const float3 darkColor = float3(0.035f, 0.045f, 0.055f);
	const float3 fogColor = float3(0.48f, 0.52f, 0.55f);
	float3 finalColor = lerp(darkColor, fogColor, density);
	finalColor = ApplyBattleZones(uv, finalColor);

	return float4(finalColor, maskAlpha * g_ui_color.a);
}

float4 PSMain(PS_IN input) : SV_Target
{
	float4 maskColor = g_MaskTex.Sample(LinearClamp, input.uv);

	// [마스킹 컷] 마스크 이미지의 알파(a) 값이 0에 가까우면 원 바깥 영역이므로 즉시 버림
	if (maskColor.a < 0.01f)
	{
		discard;
	}

	// 2. 미니맵 지도 UV 계산 (회전 및 스크롤)
	if (g_mapMode == MINIMAP_MODE_DUNGEON_FOG)
	{
		return RenderDungeonFog(input.uv, maskColor.a);
	}

	float2 mapUV = input.uv - float2(0.5f, 0.5f);
	mapUV *= g_mapScale;

	float cosAngle = cos(g_mapRotation);
	float sinAngle = sin(g_mapRotation);
	float2 rotatedUV;
	rotatedUV.x = mapUV.x * cosAngle - mapUV.y * sinAngle;
	rotatedUV.y = mapUV.x * sinAngle + mapUV.y * cosAngle;
	rotatedUV += float2(0.5f, 0.5f) + g_mapOffset;

	// 외곽 처리를 위해 7번 슬롯 Border 샘플러 사용
	float4 mapColor = g_MinimapTex.Sample(LinearBorder, rotatedUV);

	// 지도가 스크롤되어 맵의 텍스처 경계 바깥으로 밀려난 공백 영역(알파 0)도 버림
	if (mapColor.a < 0.01f)
	{
		discard;
	}

	// 3. 기존 명도 및 UI 컬러 연산 유지
	float brightness = dot(mapColor.rgb, float3(0.299, 0.587, 0.114));
	if (max(g_ui_color.r, max(g_ui_color.g, g_ui_color.b)) > 0.0f)
	{
		mapColor.rgb = g_ui_color.rgb * brightness;
	}

	// 4. 최종 출력 (원형 마스크의 알파와 UI 전체 알파 반영)
	// 원 경계면의 부드러운 안티앨리어싱을 위해 maskColor.a를 최종 알파에 곱해줍니다.
	mapColor.rgb = ApplyBattleZones(input.uv, mapColor.rgb);
	return float4(mapColor.rgb, maskColor.a * g_ui_color.a);
}
