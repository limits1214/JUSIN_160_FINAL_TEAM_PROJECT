struct MAPMESH_INSTANCE_DATA
{
    float4x4 world;
	float4 windParams; // strength, speed, frequency, bendExponent
	float2 windHeightParams;
	uint windType;
	float padding;
};

struct MAPMESH_OCCLUSION_DATA
{
    float3 worldCenter;
    float padding0;
    float3 worldExtents;
    uint instanceIndex;
};

struct MAPMESH_CULL_META
{
    uint outputOffset;
    uint batchIndex;
};

StructuredBuffer<MAPMESH_INSTANCE_DATA> gInputInstances : register(t0);
StructuredBuffer<MAPMESH_OCCLUSION_DATA> gOcclusionData : register(t1);
Texture2D<float> gPrevHiz : register(t2);
StructuredBuffer<MAPMESH_CULL_META> gCullMeta : register(t3);
RWStructuredBuffer<MAPMESH_INSTANCE_DATA> gVisibleInstances : register(u0);
RWStructuredBuffer<uint> gBatchVisibleCounts : register(u1);

cbuffer CB_MapMeshGpuCull : register(b0)
{
    float4x4 gMatViewProj;
    float2 gScreenSize;
    float2 gHizSize;
    uint gInstanceCount;
    uint gMipCount;
    uint gUseHiz;
    float gHizBias;
};

bool ProjectBounds(MAPMESH_OCCLUSION_DATA bounds, out float2 minScreen, out float2 maxScreen, out float nearestDepth)
{
    minScreen = float2(3.402823e38f, 3.402823e38f);
    maxScreen = float2(-3.402823e38f, -3.402823e38f);
    nearestDepth = 1.f;

    [unroll]
    for (uint i = 0; i < 8; ++i)
    {
        float3 signVec = float3(
            ((i & 1) != 0) ? 1.f : -1.f,
            ((i & 2) != 0) ? 1.f : -1.f,
            ((i & 4) != 0) ? 1.f : -1.f);
        float3 corner = bounds.worldCenter + bounds.worldExtents * signVec;
        float4 clip = mul(float4(corner, 1.f), gMatViewProj);
        if (clip.w <= 0.0001f)
            return false;

        float3 ndc = clip.xyz / clip.w;
        if (ndc.z < 0.f || ndc.z > 1.f)
            return false;

        float2 screen;
        screen.x = (ndc.x * 0.5f + 0.5f) * gScreenSize.x;
        screen.y = (-ndc.y * 0.5f + 0.5f) * gScreenSize.y;
        minScreen = min(minScreen, screen);
        maxScreen = max(maxScreen, screen);
        nearestDepth = min(nearestDepth, ndc.z);
    }

    if (maxScreen.x < 0.f || maxScreen.y < 0.f || minScreen.x > gScreenSize.x || minScreen.y > gScreenSize.y)
        return false;

    minScreen = clamp(minScreen, float2(0.f, 0.f), gScreenSize - 1.f);
    maxScreen = clamp(maxScreen, float2(0.f, 0.f), gScreenSize - 1.f);
    return true;
}

uint SelectMip(float2 minScreen, float2 maxScreen)
{
    float2 rectSize = max(float2(1.f, 1.f), maxScreen - minScreen);
    float mipSize = max(rectSize.x, rectSize.y);
    uint selectedMip = 0;
    while (mipSize > 2.f && selectedMip + 1 < gMipCount)
    {
        mipSize *= 0.5f;
        ++selectedMip;
    }
    if (selectedMip > 0)
        --selectedMip;
    return selectedMip;
}

bool IsOccluded(float2 minScreen, float2 maxScreen, float nearestDepth)
{
    uint mip = SelectMip(minScreen, maxScreen);
    float2 mipSize = max(float2(1.f, 1.f), floor(gHizSize / exp2((float)mip)));
    float2 mipScale = mipSize / gScreenSize;
    uint2 maxValidTexel = uint2((uint)mipSize.x, (uint)mipSize.y) - uint2(1, 1);
    uint2 minTexel = min(uint2(floor(minScreen * mipScale)), maxValidTexel);
    uint2 maxTexel = min(uint2(floor(maxScreen * mipScale)), maxValidTexel);

    for (uint y = minTexel.y; y <= maxTexel.y; ++y)
    {
        for (uint x = minTexel.x; x <= maxTexel.x; ++x)
        {
            float hizDepth = gPrevHiz.Load(int3(x, y, mip));
            if (hizDepth >= nearestDepth - gHizBias)
                return false;
        }
    }
    return true;
}

[numthreads(64, 1, 1)]
void CSMain(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint index = dispatchThreadID.x;
    if (index >= gInstanceCount)
        return;

    bool visible = true;
    if (gUseHiz != 0)
    {
        float2 minScreen;
        float2 maxScreen;
        float nearestDepth;
        if (ProjectBounds(gOcclusionData[index], minScreen, maxScreen, nearestDepth) &&
            IsOccluded(minScreen, maxScreen, nearestDepth))
            visible = false;
    }

    if (visible)
    {
        MAPMESH_CULL_META meta = gCullMeta[index];
        uint localVisibleIndex;
        InterlockedAdd(gBatchVisibleCounts[meta.batchIndex], 1, localVisibleIndex);
        gVisibleInstances[meta.outputOffset + localVisibleIndex] = gInputInstances[index];
    }
}
