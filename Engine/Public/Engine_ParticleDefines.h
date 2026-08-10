#pragma once

namespace Engine
{
	inline constexpr uint32_t INVALID_PARTICLE_OWNER_ID = 0;

	typedef struct STANDARD_PARAMS
	{
		bool bRandomPos = false;
		_float3 posMin = { 0,0,0 };
		_float3 posMax = { 0,0,0 };

		bool bRandomVel = false;
		_float3 velMin = { 0,0,0 };
		_float3 velMax = { 0,0,0 };

		uint32_t count = 1;
		_float3  position = {};
		_float3  velocity = {};
		_float3  originalVelocity = { 0.f,0.f, 0.f };

		_float   life = 1.f;
		bool bRandomSize = false;

		_float3   startSizeMin = { 1.f, 1.f, 1.f};
		_float3   startSizeMax = { 1.f, 1.f, 1.f};
		_float3   endSizeMin =	 { 1.f, 1.f, 1.f};
		_float3   endSizeMax =	 {1.f, 1.f, 1.f};

		_float3   fSize{ 1.f,1.f,1.f};
		_float3   fEndSize{ 1.f,1.f,1.f};
		bool bRandomRot = false;
		_float3 rotMin = { 0,0,0 };
		_float3 rotMax = { 0,0,0 };
		_float4   rotation = { 0.f, 0.f, 0.f, 0.f };
		_float4  color = { 1.f, 1.f, 1.f, 1.f };
		_float4  originalEmissive = { 1.f, 1.f, 1.f, 0.f };
		_float4  emissive = { 1.f, 1.f, 1.f, 0.f };
		_float4  endEmissive = { 1.f, 1.f, 1.f, 0.f };
		_bool    bLoop = false;
		_float   fSpawnInterval = 0.1f;
		_float	 fSpawnDelay = 0.f;
		uint32_t	iBehaviorType;
		_float fStopSizeTime = 0.f;
		_bool    bKeepRotate = false;
		_float3  rotationAxis= {};
		_float  rotationSpeed = {};
	}STANDARD_PARAMS;

	typedef struct BEAM_PARAMS
	{
		_float4  beamStart = {};
		_float4  beamEnd = {};
		_float4  color = { 1.f, 1.f, 1.f, 1.f };
		_float4  emissive = { 1.f, 1.f, 1.f, 0.f };
		_float4  endEmissive = { 1.f, 1.f, 1.f, 0.f };
		int      iDisplacementIterations = 6;
		_float   fDisplacementAmplitude = 2.5f;
		_float   fDisplacementDamping = 0.25f;
		_float   flickerTimeInverval = 0.25f;
		_float   beamDuration = 0.f;
		_float	 fSpawnDelay = 0.f;
		uint32_t ownerId = 0;
		int geometryType = 0;
		_float fGrowEndTime{};
		_float fStraightEndTime{};
		_float fHoldEndTime{};
		_float fFadeEndTime{};
		_float fBeamWidth{};
	

	}BEAM_PARAMS;

	struct BEAM_HANDLE
	{
		StringID groupTag;
		StringID typeTag;
		int32_t beamIndex = -1;

		bool IsValid() const
		{
			return beamIndex >= 0;
		}
	};
	constexpr uint32_t BEHAVIOR_NONE = 0;
	constexpr uint32_t BEHAVIOR_DISTORTION = 1 << 1;
	constexpr uint32_t BEHAVIOR_BILLBOARD = 1 << 2;
	constexpr uint32_t BEHAVIOR_GRAVITY = 1 << 3;
	constexpr uint32_t BEHAVIOR_CIRCLE_TO_WAVE = 1 << 4;
	constexpr uint32_t BEHAVIOR_SMOKE = 1 << 5;
	constexpr uint32_t BEHAVIOR_SMOKEJUMP = 1 << 6;
	constexpr uint32_t BEHAVIOR_SMOKEGV = 1 << 7;
	constexpr uint32_t BEHAVIOR_SMOKEGW = 1 << 8;
	constexpr uint32_t BEHAVIOR_LIGHTNING = 1 << 9;
	constexpr uint32_t BEHAVIOR_SIZESTOP = 1 << 10;
	// ============================================================
	// X-매크로: 필드 목록을 한 곳에서만 정의
	// X(타입, 이름, 기본값)
	// ============================================================

	/*
	* 	typedef struct tagParticle {
		_float3  position;
		_float   pad1;
		_float3  velocity;
		_float   life;
		_float   maxLife;
		_float   size;
		_float   startSize;
		_float   endSize;
		_float4  rotation;
		uint32_t alive;
		uint32_t loop;
		_float2  pad2;         // 추가 필요: loop→color (8바이트)
		_float4  color;

		_float4  originalEmissive, emissive, endEmissive;
		uint32_t frameIndex;
		uint32_t ownerID;
		uint32_t iBehaviorType = 0;
		_float pad3;
		_float3 originalPosition; // 원래 스폰 위치
		_float pad4;
		_float3 originalVelocity; // 원래 스폰 속도+ 방향
		_float pad5;
	}PARTICLE;
	*/
#define COMMON_PATTERN_FIELDS(X) \
    X(uint32_t, iBehaviorType, 0)

#define STAIRS_FIELDS(X) \
    X(_float3, vStartPos, _float3(0,0,0)) \
    X(uint32_t, iStepCount, 5) \
    X(_float, fStepWidth, 1.f) \
    X(_float, fStepHeight, 1.f) \
    X(_float, fStepDepth, 1.f) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0))\
   COMMON_PATTERN_FIELDS(X)

#define CIRCLE_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
    X(uint32_t, iCount, 12) \
	X(_bool, bRandomSize, false) \
	X(_float3, fSizeMin, _float3(0, 0, 0))\
	X(_float3, fSizeMax, _float3(0, 0, 0))\
    X(_float3, fSize,  _float3(1,1,1)) \
    X(_float3, fEndSize, _float3(1,1,1)) \
    X(_float, fLife, 1.f) \
	X(_bool, bRandomRot, false) \
    X(_float3, vMinRot, _float3(0,0,0)) \
    X(_float3, vMaxRot, _float3(0,0,0)) \
    X(_float3, vRotation, _float3(0,0,0)) \
	X(_bool, bRandomVel, false) \
	X(_float3, fVelMin, _float3(0,0,0))\
	X(_float3, fVelMax, _float3(0,0,0))\
	X(_float3, fVelocity, _float3(0,0,0))\
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0)) \
    X(_float, startIntensity, 0.f) \
    X(_float4, endEmissive, _float4(0,0,0,0)) \
	X(_float, endIntensity, 0.f) \
    X(_float, fYOffset, 0.f)\
    X(_bool, bStand, false)\
   COMMON_PATTERN_FIELDS(X)

#define CIRCLE_SPREAD_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
    X(uint32_t, iCount, 12) \
    X(_float3, fSize, _float3(1,1,1)) \
    X(_float3, fEndSize, _float3(1,1,1)) \
    X(_float, fLife, 1.f) \
	X(_float3, fVelocity, _float3(0,0,0))\
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0)) \
    X(_float, startIntensity, 0.f) \
    X(_float4, endEmissive, _float4(0,0,0,0)) \
    X(_float, endIntensity, 0.f) \
    X(_float, fSpawnDelay, 0.f)\
   COMMON_PATTERN_FIELDS(X)

#define SPIRAL_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
    X(uint32_t, iCount, 20) \
    X(_float, fHeightPerStep, 0.2f) \
    X(_float, fAngleStepDeg, 15.f) \
    X(_float3, fSize, _float3(1,1,1)) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0))\
   COMMON_PATTERN_FIELDS(X)



#define STRAIGHT_GROUND_FIELDS(X) \
    X(_float3, vStartPos, _float3(0,0,0)) \
    X(_bool, bRandomPos, false) \
    X(_float3, vMinPos, _float3(0,0,0)) \
    X(_float3, vMaxPos, _float3(0,0,0)) \
    X(uint32_t, iRow, 3) \
    X(uint32_t, iCol, 3) \
    X(_float, fOffsetX, 1.f) \
    X(_float, fOffsetZ, 1.f) \
    X(_bool, bRandomRot, false) \
    X(_float3, vMinRot, _float3(0,0,0)) \
    X(_float3, vMaxRot, _float3(0,0,0)) \
    X(_float3, vRotation, _float3(0,0,0)) \
    X(_float, fSpawnDelay, 0.1f) \
    X(_float3, fSize,_float3(1,1,1)) \
    X(_float, fLife, 1.f) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, startEmissive, _float4(0,0,0,0))\
    X(_float4, endEmissive, _float4(0,0,0,0))\
   COMMON_PATTERN_FIELDS(X)



#define SPAWN_S_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(uint32_t, iCount, 1) \
   COMMON_PATTERN_FIELDS(X)

#define SMOKE_FIELDS(X)\
	 X(uint32_t, iFlag, 0) \
	 X(_float3, vCenter, _float3(0,0,0)) \
    X(_float, fRadius, 3.f) \
	X(uint32_t, iCount, 12) \
    X(_float3, fSize,    _float3(1.f,1.f,1.f)) \
    X(_float3, fEndSize, _float3(1.f,1.f,1.f)) \
    X(_float, fLife, 1.f) \
	X(_float3, fVelocity, _float3(0,0,0))\
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float, fYOffset, 0.f)\
	X(_float, fSpeed, 0.f)\
	X(_float2,vRandRaidus, _float2(1.f,1.f)) \
	X(_float2,vRandSpeed,_float2(0.8f,1.2f))\
	X(_float2,vRandAlpha,_float2(1.f,1.f))\
	X(_float2,vRandAngle,_float2(-0.1f,0.1f))\
	X(_float2,vRandSize, _float2(0.8f,1.2f))\
	X(_float2,vRandLife, _float2(0.9f,1.1f))\
	X(_float2, vRandSpawn, _float2(1.f,1.f))\
	X(_float3,vRot,_float3(0,0,0))\
	X(uint32_t, iArray,1)\
	X(_float ,fSPawnDelay,0.f)\
COMMON_PATTERN_FIELDS(X)

#define LIGHTNING_STREIGHT(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(_float3, vRotation, _float3(0,0,0)) \
    X(uint32_t, iCount, 12) \
    X(_bool, bRandomSize, false) \
    X(_float3, fSize, _float3(1.f,1.f,1.f)) \
	X(_float3, fSizeMin, _float3(0, 0, 0))\
	X(_float3, fSizeMax, _float3(0, 0, 0))\
    X(_float3, fEndSize, _float3(1.f,1.f,1.f)) \
    X(_float, fLife, 1.f) \
    X(_bool, bRandomVel, false) \
	X(_float3, fVelocity, _float3(0,0,0))\
	X(_float3, fVelMin, _float3(0,0,0))\
	X(_float3, fVelMax, _float3(0,0,0))\
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0)) \
    X(_float, startIntensity, 0.f) \
    X(_float4, endEmissive, _float4(0,0,0,0)) \
	X(_float, endIntensity, 0.f) \
   COMMON_PATTERN_FIELDS(X)

#define CONE_FIELDS(X) \
    X(_float3, vCenter, _float3(0,0,0)) \
    X(uint32_t, iCount, 12) \
    X(_float, fAngleDegree, 1.f) \
    X(_float, fSpeedMin, 1.f) \
    X(_float, fSpeedMax, 1.f) \
    X(_float, fLife, 1.f) \
    X(_bool, bRandomSize, false) \
	X(_float3, fSize, _float3(1.f,1.f,1.f)) \
	X(_float3, fSizeMin, _float3(0, 0, 0))\
	X(_float3, fSizeMax, _float3(0, 0, 0))\
    X(_float3, fEndSize, _float3(1.f,1.f,1.f)) \
    X(_bool, bRandomRot, false) \
    X(_float3, vMinRot, _float3(0,0,0)) \
    X(_float3, vMaxRot, _float3(0,0,0)) \
    X(_float3, vRotation, _float3(0,0,0)) \
    X(_float4, color, _float4(1,1,1,1)) \
    X(_float4, emissive, _float4(0,0,0,0)) \
    X(_float, startIntensity, 0.f) \
    X(_float4, endEmissive, _float4(0,0,0,0)) \
	X(_float, endIntensity, 0.f) \
   COMMON_PATTERN_FIELDS(X)

// ============================================================
// struct 자동 생성 매크로
// ============================================================
#define DECLARE_PARAM_STRUCT(StructName, FIELD_LIST) \
struct StructName \
{ \
    FIELD_LIST(DECLARE_PARAM_FIELD) \
}; \

#define DECLARE_PARAM_FIELD(type, name, defaultVal) type name = defaultVal;

// 실제 struct 선언 (매크로 한 줄씩)
	struct SStairsParam { STAIRS_FIELDS(DECLARE_PARAM_FIELD) };
	struct SCircleParam { CIRCLE_FIELDS(DECLARE_PARAM_FIELD) };
	struct SCircleSpreadParam { CIRCLE_SPREAD_FIELDS(DECLARE_PARAM_FIELD) };
	struct SSpiralParam { SPIRAL_FIELDS(DECLARE_PARAM_FIELD) };
	struct SStraightGroundParam { STRAIGHT_GROUND_FIELDS(DECLARE_PARAM_FIELD) };
	struct SMOKE { SMOKE_FIELDS(DECLARE_PARAM_FIELD) };
	struct SLightning { LIGHTNING_STREIGHT(DECLARE_PARAM_FIELD) };
	struct SConeParam { CONE_FIELDS(DECLARE_PARAM_FIELD) };

#undef DECLARE_PARAM_FIELD


	//3. STRUCT 추가
	using PatternParamVariant = std::variant<SStairsParam, SCircleParam, SSpiralParam, SStraightGroundParam, SCircleSpreadParam, SMOKE, SLightning, SConeParam>;

	// 4. 콤보박스 등에서 쓸 이름 목록 (variant 인덱스와 순서 반드시 일치)
	inline constexpr const char* PATTERN_KIND_NAMES[] =
	{
		"Stairs", "Circle",  "Spiral", "StraightGround", "CircleToWave", "SMOKE", "SLightning", "Cone"
	};

	//5. 여기에 CASE 추가
	// 인덱스로 기본값 variant 생성 (콤보박스에서 종류 바꿀 때 사용)
	inline PatternParamVariant MakeDefaultPatternParam(int index)
	{
		switch (index)
		{
		case 0: return SStairsParam{};
		case 1: return SCircleParam{};
		case 2: return SSpiralParam{};
		case 3: return SStraightGroundParam{};
		case 4: return SCircleSpreadParam{};
		case 5: return SMOKE{};
		case 6: return SLightning{};
		case 7: return SConeParam{};
			  
		default: return SStairsParam{};
		}
	}


	//6. particleparmaImgui 로 이동
	enum class SPAWN_COMMAND_KIND { STANDARD, BEAM, PATTERN ,LIGHT };

	struct SPAWN_COMMAND
	{
		SPAWN_COMMAND_KIND sGroupTag_KindTag{};
		StringID sGroupTag{};
		StringID sTypeTag{};
		uint32_t ownerId = 0;
		std::variant<STANDARD_PARAMS, BEAM_PARAMS, PatternParamVariant, std::vector<PARTICLE_SPAWN_DATA>> params;
	};


}
