#pragma once
namespace Engine
{	
	typedef struct tagConstantBufferPerObject
	{
		_float4x4 matWorld{};
		_float4x4 matWVP{};
	} CB_PER_OBJECT;
	static_assert(sizeof(CB_PER_OBJECT) % 16 == 0);

	typedef struct tagConstantBufferPerPass
	{
		//DIRECTIONAL_LIGHT dirLight{};
		_float4x4	matView{};            // 뷰 행렬
		_float4x4	matProj{};            // 투영 행렬 (Perspective 또는 Ortho)
		_float4x4	matViewProj{};        // 곱해진 행렬 (VS에서 연산 절약)

		_float4x4	matInvView{};			// 뷰 역행렬 (빌보드 계산이나 월드 좌표 복원용)
		_float4x4	matInvProj{};
		_float4x4	matInvViewProj{};

		_float4x4	matShadowLightViewProj{};

		_float3		vCamPos{};
		_float		fDeltaTime{};

		_float3		vShadowLightDir{};
		_float		fTimeAccumulation{};
	} CB_PER_PASS;
	static_assert(sizeof(CB_PER_PASS) % 16 == 0);

	typedef struct tagConstantBufferObjectMaterial
	{
		_float3  EmissiveColor;
		_float   EmissiveIntensity;

		_float3  DissolveColor;
		_float   DissolveIntensity;

		_float	 ObjectAlpha;

		_float3  ObjectPadding;

	} CB_MATERIAL;
	static_assert(sizeof(CB_MATERIAL) % 16 == 0);

	typedef struct tagConstantBufferLight
	{
		DYNAMIC_LIGHT	AffectedLight[MAX_NORMAL_LIGHT_RENDER_COUNT];

		XMFLOAT4X4		g_InvViewProj{};
		uint32_t		LightCount{};
		_float3			LightPadding{};
	} CB_LIGHT;
	static_assert(sizeof(CB_LIGHT) % 16 == 0);

	typedef struct tagConstantBufferEffectLight
	{
		EFFECT_LIGHT	EffectLight[MAX_EFFECT_LIGHT_RENDER_COUNT];

		uint32_t		LightCount{};
		_float3			LightPadding{};
	} CB_EFFECT_LIGHT;
	static_assert(sizeof(CB_EFFECT_LIGHT) % 16 == 0);

	typedef struct tagConstantBufferShadow
	{
		uint32_t	CurrentShadowLightIndex{};
		uint32_t	CurrentPointFaceIndex{};
		uint32_t	CurrentCascadeIndex{};
		_float		ShadowPadding{};
	} CB_SHADOW;
	static_assert(sizeof(CB_SHADOW) % 16 == 0);

	typedef struct tagConstantBufferPerUI
	{
		_float2  texCoord{};
		_float2  uvSize{};	// 그릴사이즈
		_float4  color{ 0.f, 0.f, 0.f, 1.f };
		_float2 texSize{};  // 원본 텍스처의 픽셀 크기 (Width, Height)
		_float2 quadSize{}; // 텍스처의 현제 사이즈
		_float4 margins{};
	} CB_PER_UI;
	static_assert(sizeof(CB_PER_UI) % 16 == 0);

	typedef struct CB_PART_ATTACHMENT
	{
		_float4x4 m_preTransform;
		uint32_t gParentInstanceIndex;
		uint32_t gParentBoneIndex;
		_float2  gPartAttachmentPadding;
	}CB_PART_ATTACHMENT;
	static_assert(sizeof(CB_PART_ATTACHMENT) % 16 == 0);

	typedef struct tagInitParticle
	{
		uint32_t g_iMaxParticles;
		_float3 pad;
	}CB_INIT_PARTICLE;
	static_assert(sizeof(CB_INIT_PARTICLE) % 16 == 0);

	typedef struct CB_ParticleUpdate
	{
		_float    g_fTimeDelta;
		uint32_t g_iNumInstances;
		uint32_t g_iFlipbookRows;
		uint32_t g_iFlipbookColumns;
		uint32_t g_iTotalFrames;
		_float	g_fTime;
		_float2    g_fPadding2;   // 16바이트 정렬 맞추려고 패딩 조정 필요
	} CB_PER_PARTICLE;
	static_assert(sizeof(CB_PER_PARTICLE) % 16 == 0);

	typedef struct CB_SCROLL
	{
		_float    g_fScrollOffset;
		_float    g_fAccumulationTime;
		uint32_t    g_iCurrentFrame;
		uint32_t    g_iFlipbookRows;
		uint32_t    g_iFlipbookColumns;
		_float3    g_fPadding;   // 16바이트 정렬 맞추려고 패딩 조정 필요
	} CB_SCROLL;
	static_assert(sizeof(CB_SCROLL) % 16 == 0);

	typedef struct CB_ParticleSpawn
	{
		uint32_t    g_iSpawnCount;
		uint32_t	g_iMaxParticles;
		_float2     pad;
		//PARTICLE_SPAWN_DATA  g_SpawnData[MAX_SPAWN_PER_CALL];
	}CB_PARTICLE_SPAWN;
	static_assert(sizeof(CB_PARTICLE_SPAWN) % 16 == 0);

	typedef struct CB_RibbonParticle
	{
		uint32_t    g_iSpawnCount;
		_float3     pad;
		PARTICLE_SPAWN_DATA  g_SpawnData[MAX_SPAWN_PER_CALL];
	}CB_RIBBON_PARTICLE;
	static_assert(sizeof(CB_RIBBON_PARTICLE) % 16 == 0);

	struct CB_OWNER_OPERATION
	{
		uint32_t iTargetOwnerID = 0;
		uint32_t iMaxParticles = 0;
		_float2 vPadding{};
		_float4x4 matDelta{};
		_float4 vColor{};
		_float4 vEmissive{};
	};
	static_assert(sizeof(CB_OWNER_OPERATION) % 16 == 0);

	typedef struct CB_SpellMeter
	{
		float fAmount;
		float fDistSpeed;
		float fDistStrength;
		float fTime;

		_float4 vFillColor;
		_float4 vEmptyColor;
		_float4 vRippleColor;
		_float4 vWispyColor;
	}CB_SPELLMETER;
	static_assert(sizeof(CB_RIBBON_PARTICLE) % 16 == 0);

	struct CB_TRAIL_OPTION 
	{
		float g_fNoiseStrength; // 0~1
		float g_fDistortion; // 0~0.1
		float g_fGlowStrength; // 0~3
		float g_fLengthGlow; // 0~2

		float g_fDissolve;
		float g_fUseNoise;
		float g_fUseDistortion;
		float g_fUseDissolve;
	};
	static_assert(sizeof(CB_TRAIL_OPTION) % 16 == 0);

	typedef struct CB_Minimap
	{
		_float2	mapOffset;
		float	mapRotation;
		float	mapScale;
		uint32_t mapMode;
		float smokeIntensity;
		float smokeSpeed;
		float smokeTime;
		uint32_t battleZoneCount;
		_float3 battleZonePadding;
		_float4 battleZones[8];
	}CB_MINIMAP;
	static_assert(sizeof(CB_MINIMAP) % 16 == 0);

	struct CB_BLOOM
	{
		_float2	g_TexelSize;
		_float2	g_padding;
	};
	static_assert(sizeof(CB_BLOOM) % 16 == 0);

	struct CB_FROXEL
	{
		_float3	g_fFroxelGridSize;
		_float	g_fNearZ;

		_float	g_fFarZ;
		_float2	g_fScreenResolution;
		_float  g_fSliceDepthRatio;

		_float2	g_fHalfScreenResolution;
		_float2	FroxelPadding1;
	};
	static_assert(sizeof(CB_FROXEL) % 16 == 0);

	struct CB_VLFOG
	{
		_float3 g_fFogColor;
		_float	g_fFogIntensity;

		_float3 g_fFogCenterPos;
		_float  g_fFogHeight;

		_float	g_fFogStartPos;
		_float	g_fFogEndPos;
		_float	g_fFogDensity;
		_float	g_fFogNoiseScale;

		_float3	g_fFogLightDirection;
		_float	g_fFogAnisotropyGA;

		_float3	g_fFogLightColor;
		_float	g_fFogAnisotropyGB;

		_float	g_fFogScatteringWeight;
		_float	g_fFogBaseHeight;
		_float	g_fFogHeightFallOff;
		_float	g_fFogPadding;
	};
	static_assert(sizeof(CB_VLFOG) % 16 == 0);

	struct CB_CSM
	{
		_matrix g_mShadowViewProj[4];
		_float4 g_fCascadeSplits;
		_float2 g_fShadowMapSize;
		_float2 g_fShadowBias;
	};
	static_assert(sizeof(CB_CSM) % 16 == 0);

	struct CB_LENSFLARE
	{
		_float2	FlareCenterUV;
		float	FlareCurrentLifeTime;
		float	FlareMaxLifeTime;

		float	RingStartScale;
		float	RingEndScale;
		float	AspectRatio;
		float	RingBaseAlpha;

		float	RainbowSaturation;
		float	FlareEnabled;
		_float2	TextureSize;
	};
	static_assert(sizeof(CB_LENSFLARE) % 16 == 0);
}
