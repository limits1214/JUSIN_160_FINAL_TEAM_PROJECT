
const static float PI = 3.14159265359f;

const static float3 AlbedoColor = { 1.f, 1.f, 1.f };

const static float NormalIntensity = 1.f;
const static float RoughnessIntensity = 1.f;
const static float MetallicIntensity = 1.f;
const static float AmbientIntensity = 1.f;
const static float SpecularIntensity = 1.f;

#define MAX_LIGHT_COUNT				16
#define MAX_SHADOW_LIGHT_COUNT      8

#define MAX_LIGHT_MAPCOUNT  6

#define LIGHT_DIRECTIONAL   0
#define LIGHT_POINT         1
#define LIGHT_SPOTLIGHT     2

#define MAX_REFLECTION_LOD  6.f

#define SCREENX 1280
#define SCREENY 720

#define POINTLIGHT_RESOLUTION 512
#define SPOTLIGHT_RESOLUTION  512

struct DirectionalLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float3 direction;
    float pad;
};

struct PointLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    
    float3 pos;
    float range;
    
    float3 att;
    float _pad;
};

struct SpotLight
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    
    float3 pos;
    float range;
    
    float3 direction;
    float spot;
    
    float3 att;
    float _pad;
};
struct DynamicLight
{
	float4x4 g_LightViewProj[MAX_LIGHT_MAPCOUNT];

    float3 LightDirection;
    float LightIntensity;
    float3 LightColor;
    float LightRange;

    float3 Position;
    uint LightType;

    float InnerAttanuation;
    float OuterAttanuation;
	
	int		ShadowSlot;
    float LightPadding;
};

struct EffectLight
{
	float3 Position;
	float LightIntensity;

	float3 LightColor;

	float InnerAttanuation;
	float OuterAttanuation;

	float3 LightPadding;
};

const static float alpha = 0;
struct Material
{
    float4 ambient;
    float4 diffuse;
    float4 specular;
    float4 reflect;
};

// 1. 오브젝트당 n회 갱신 (슬롯 b0)
cbuffer CB_PER_OBJECT : register(b0)
{
    matrix g_matWorld;
    matrix g_matWVP;
};

// 2. 프레임당 1회 갱신 (슬롯 b1)
cbuffer CB_PER_PASS : register(b1)
{
    matrix g_matView; // _float4x4와 1:1 대응
    matrix g_matProj;
    matrix g_matViewProj;
	
    matrix g_matInvView;
    matrix g_matInvProj;
    matrix g_matInvViewProj;
	
    matrix g_matShadowLightViewProj;
	
    float3 g_vCamPos;
	float  g_fDeltaTime;
    float3 g_vShadowLightDir;
	float  g_fTimeAccumulation;
};

cbuffer CB_BONES : register(b2)
{
    matrix g_BoneMatrices[512];
};

cbuffer CB_MATERIAL : register(b3)
{
    float3  EmissiveColor;
    float   EmissiveIntensity;
    
    float3  DissolveColor;
    float   DissolveIntensity;
    
    float   ObjectAlpha;
    
    float3  MaterialPadding;
}

cbuffer CB_LIGHT_BUFFER : register(b4) 
{
	DynamicLight	AffectedLight[MAX_LIGHT_COUNT];
    float4x4		g_InvViewProj;
    uint			LightCount;
    float3			LightPadding;
}



cbuffer CB_PER_UI : register(b7)
{
    float2 g_ui_texCoord;
    float2 g_ui_uvSize;
    float4 g_ui_color;
	float2 g_ui_texSize;
	float2 g_ui_quadSize;
	float4 g_ui_margins;
};

cbuffer CB_GPU_PART_ATTACHMENT : register(b9)
{
    float4x4 preTransform;
    uint gParentInstanceIndex;
    uint gParentBoneIndex;
    float2 gPartAttachmentPadding;
};


SamplerState LinearWrap                 : register(s0);
SamplerState LinearClamp                : register(s1);
SamplerState PointWrap                  : register(s2);
SamplerState PointClamp                 : register(s3);
SamplerState PointWrapNoMip             : register(s4);
SamplerState AnisotropicWrap            : register(s5);
SamplerState LinearBorder               : register(s7);

SamplerComparisonState ShadowSampler    : register(s6);
