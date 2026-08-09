#include "../ShaderDefines.hlsl"

float4 Convert_WorldPosByDepth(float _Depth, float2 _TexCoord)
{
    // Depth = NDC   -> (InvProj) -> WorldSpace(InvView)
    float4 NDCWorldPos;
    
    // ViewSpace
    NDCWorldPos.x = _TexCoord.x * +2.f - 1.f;
    NDCWorldPos.y = _TexCoord.y * -2.f + 1.f;
    NDCWorldPos.z = _Depth;
    NDCWorldPos.w = 1.f;
    
    float4 WorldPos = mul(NDCWorldPos, g_matInvViewProj);
    
    return float4(WorldPos.xyz / WorldPos.w, 1.f);
}

float3x3 Make_TBNMatrix(float3 _Normal, float3 _Tangent)
{
    float3 Normal = normalize(_Normal);
    float3 Tangent = normalize(_Tangent);

    Tangent = normalize(Tangent - dot(Tangent, Normal) * Normal);
    
    float3 BiNormal = normalize(cross(Normal, Tangent));
    
    return float3x3(Tangent, BiNormal, Normal);
}
float3 Compute_WorldNormal(Texture2D _NormalTex, float2 _TexCoord, float4 _InNormal, float4 _InTangent)
{
    float3 LocalNormal = _NormalTex.Sample(LinearWrap, _TexCoord).rgb;
    LocalNormal = normalize(LocalNormal * 2.f - 1.f);
    float3x3 TBN = Make_TBNMatrix(_InNormal.xyz, _InTangent.xyz);

    float3 N = normalize(_InNormal.xyz);
    float3 T = normalize(_InTangent.xyz);
    
    T = normalize(T - dot(T, N) * N);
    float3 B = normalize(cross(N, T));
    
    float3 worldNormal = LocalNormal.x * T + LocalNormal.y * B + LocalNormal.z * N;

    return normalize(worldNormal);
}
float Convert_ViewZPosByDepth(float _Depth)
{
	return g_matProj._m32 / (_Depth - g_matProj._m22);
}

bool Compute_DynamicLight(float3 _WorldPosition, DynamicLight Light, out float3 L, out float3 Radiance)
{

    [branch]
    if (Light.LightType == LIGHT_DIRECTIONAL)   // Directional Light PBR
    {
		float3	NormalizedDirection = normalize(Light.LightDirection.xyz);
		L = -NormalizedDirection;
        Radiance = Light.LightColor * Light.LightIntensity;
    }
    else if (Light.LightType == LIGHT_POINT)    // Point Light PBR
    {
        float3	LightVector = Light.Position - _WorldPosition;
		float	DistanceSQ = dot(LightVector, LightVector);
		
		float	OuterRange = max(Light.OuterAttanuation, 0.001f);
		float	OuterRangeSQ = OuterRange * OuterRange;
		
		[branch]
		if (DistanceSQ >= OuterRangeSQ)	return false; // 빛이 안 닿는 구역
		
		float	InvDistance = rsqrt(max(DistanceSQ, 0.00001f));
		float	Distance = DistanceSQ * InvDistance;
		
		L = LightVector * InvDistance;
		
		float	InnerRange = clamp(Light.InnerAttanuation, 0.f, OuterRange);
		float	FadeRatio =saturate((Distance - InnerRange) / max(OuterRange - InnerRange, 0.001f));
		
		float	DistanceRatio = DistanceSQ / OuterRangeSQ;
		
		float	RangeFade = 1.f - smoothstep(0.f, 1.f, FadeRatio);
		RangeFade *= RangeFade;
		
		//float Attenuation = RangeFade / max(DistanceSQ + 1.f, 1.f);
		Radiance = Light.LightColor * Light.LightIntensity * RangeFade;
	}
    else if (Light.LightType == LIGHT_SPOTLIGHT)    // SpotLight Light PBR
    {
        float3	LightVector = Light.Position - _WorldPosition;
		float	LightRange = max(Light.LightRange, 0.001f);
		float	DistanceSQ = dot(LightVector, LightVector);
		float	RangeSQ = LightRange * LightRange;


		[branch]
		if (DistanceSQ > RangeSQ)
			return false; // 빛이 안 닿는 구역
		
		float	InvDistance =	rsqrt(max(DistanceSQ, 0.00001f));
		float	Distance = DistanceSQ * InvDistance;
		
		float	DistanceRatio = saturate(Distance / Light.LightRange);
		
		L = LightVector * InvDistance;
		
        // Decrease By Distance
		float3	NormalizedDirection = normalize(Light.LightDirection.xyz);
		float	DistanceFade = 1.f - smoothstep(0.f, 1.f, DistanceRatio);
		float	CosAngle = dot(-L, NormalizedDirection);
		float	ConeFade = smoothstep(Light.OuterAttanuation, Light.InnerAttanuation, CosAngle);
	
        Radiance = Light.LightColor * Light.LightIntensity * DistanceFade * ConeFade;
    }
    
    return true;
}

bool Compute_EffectLight(float3 _WorldPosition, EffectLight Light, out float3 L, out float3 Radiance)
{
	float3	LightVector = Light.Position - _WorldPosition;
	float	DistanceSQ = dot(LightVector, LightVector);
		
	float	OuterRange = max(Light.OuterAttanuation, 0.001f);
	float	InnerRange = min(Light.InnerAttanuation, OuterRange);
	float	OuterRangeSQ = OuterRange * OuterRange;
	
	[branch]
	if (DistanceSQ > OuterRangeSQ)
	{
		L		 = float3(0.f, 0.f, 0.f);
		Radiance = float3(0.f, 0.f, 0.f);
		
		return false;
	}
	
	float InvDistance = rsqrt(max(DistanceSQ, 0.00001f));
	float Distance = DistanceSQ * InvDistance;
	
	L = LightVector * InvDistance;
		
	float DistanceRatio = DistanceSQ / OuterRangeSQ;
		
	float RangeFade = 1.f - smoothstep(InnerRange, OuterRange, Distance);
	RangeFade *= RangeFade;
		
	float Attenuation = RangeFade / max(DistanceSQ + 1.f, 1.f);
		
	Radiance = Light.LightColor * Light.LightIntensity * Attenuation;
	
	return true;
}

float3 Apply_DissolveEffect(Texture2D _NoiseTex, float3 _BaseEmissive, float2 _TexCoord, float _EdgeWidth)
{
	float DissolveFactor = _NoiseTex.Sample(LinearWrap, _TexCoord).r - DissolveIntensity;
    clip(DissolveFactor);
	
    float   DissolveEdge = 1.f - smoothstep(0.f, _EdgeWidth, DissolveFactor);
    
	float3 DissolveEmissive = DissolveColor * DissolveEdge;
    
	return _BaseEmissive + DissolveEmissive;
}
