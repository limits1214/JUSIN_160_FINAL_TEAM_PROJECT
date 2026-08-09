#include "../ShaderHeader/SH_CommonFunction.hlsli"

Texture2D g_DiffuseTexture : register(t0);
Texture2D g_NormalTexture : register(t1);
Texture2D g_SMROTexture : register(t2);
Texture2D g_EmissiveTexture : register(t3);

Texture2D DefaultNoiseTexture : register(t13);
static const float DissolveEdgeWidth = 0.025f;

struct VS_IN
{
	float3 vPosition : POSITION;
	float3 vNormal : NORMAL;
	float3 vTangent : TANGENT;
	float3 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;
	
	float4 iWorld0 : INSTANCE_WORLD0;
	float4 iWorld1 : INSTANCE_WORLD1;
	float4 iWorld2 : INSTANCE_WORLD2;
	float4 iWorld3 : INSTANCE_WORLD3;
	
	float4	iWindParams : INSTANCE_WIND_PARAMS; // strength, speed, frequency, bendExponent
	float2	iWindHeightParams : INSTANCE_WIND_HEIGHT; // local influence start Y, inverse influence height
	uint	iWindType : INSTANCE_WIND_TYPE;
};

struct VS_OUT
{
	float4 vPosition : SV_POSITION;
	float4 vNormal : NORMAL;
	float4 vTangent : TANGENT;
	float4 vBinormal : BINORMAL;
	float2 vTexcoord : TEXCOORD0;
	float4 vWorldPos : TEXCOORD1;
	float4 vProjPos : TEXCOORD2;
};

static const uint WIND_NONE = 0u;
static const uint WIND_GRASS = 1u;
static const uint WIND_TREE = 2u;


VS_OUT VSMain(VS_IN In)
{
	VS_OUT Out;

	float4x4 matWorld = float4x4(In.iWorld0, In.iWorld1, In.iWorld2, In.iWorld3);

	float strength = In.iWindParams.x;
	float speed = In.iWindParams.y;
	float frequency = In.iWindParams.z;
	float bendExponent = max(In.iWindParams.w, 0.01f);

	float localMinY = In.iWindHeightParams.x;
	float inverseHeight = In.iWindHeightParams.y;

    // 높이 정보가 아직 세팅되지 않은 오브젝트용 임시 fallback
	float heightWeight = inverseHeight > 0.f ? saturate((In.vPosition.y - localMinY) * inverseHeight) : saturate(In.vPosition.y);

	float bendWeight = pow(heightWeight, bendExponent);

	float4 worldPosition = mul(float4(In.vPosition, 1.f), matWorld);
	float2 windDirection = normalize(float2(1.f, 0.35f));
	float2 objectWorldPosition = In.iWorld3.xz;
	float randomPhase = frac(sin(dot(objectWorldPosition, float2(12.9898f, 78.233f))) * 43758.5453f) * 6.2831853f;

	float spatialPhase = dot(worldPosition.xz, float2(0.17f, 0.11f)) * frequency;

	float phase = g_fTimeAccumulation * speed + randomPhase + spatialPhase;

	float primaryWave = sin(phase) + sin(phase * 2.17f + 1.3f) * 0.35f;

	float detailWave = sin(phase * 4.71f + In.vPosition.y * 2.f);

	float displacement = 0.f;

	if (In.iWindType == WIND_GRASS)
	{
		displacement = strength * bendWeight * (primaryWave + detailWave * 0.18f);
	}
	else if (In.iWindType == WIND_TREE)
	{
		displacement = strength * bendWeight * (primaryWave * 0.35f + detailWave * 0.08f);
	}

	worldPosition.xz += windDirection * displacement;

	float4 worldNormal = normalize(mul(float4(In.vNormal, 0.f), matWorld));

	float4 worldTangent = normalize(mul(float4(In.vTangent, 0.f), matWorld));

	float4 worldBinormal = normalize(mul(float4(In.vBinormal, 0.f), matWorld));

	float4 viewPosition = mul(worldPosition, g_matView);

	Out.vPosition = mul(viewPosition, g_matProj);
	Out.vNormal = worldNormal;
	Out.vTangent = worldTangent;
	Out.vBinormal = worldBinormal;
	Out.vTexcoord = In.vTexcoord;
	Out.vWorldPos = worldPosition;
	Out.vProjPos = Out.vPosition;

	return Out;
}
