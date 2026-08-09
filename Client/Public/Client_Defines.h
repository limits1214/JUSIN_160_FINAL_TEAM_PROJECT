#pragma once

#include <cstdint>
#include "Level_Defines.h"
namespace Client
{
	static const unsigned int	g_iWinSizeX{ 1280 };
	static const unsigned int	g_iWinSizeY{ 720 };

	enum class SOUND_BUS
	{
		BGM,
		SFX,
		VOICE,
		UI,
		AMBIENCE,
		END
	};

	enum class COLLISION_LAYER : uint32_t
	{
		NONE = 0,				// 어떤 물리 레이어에도 속하지 않음
		DEFAULT = 1u << 0,		// 레이어를 지정하지 않은 기존 Collider의 호환용 기본값
		WORLD_STATIC = 1u << 1,	// 지형, 건물, 벽처럼 움직이지 않는 월드 충돌체
		WORLD_DYNAMIC = 1u << 2,	// 상자, 가구처럼 물리로 움직일 수 있는 월드 오브젝트
		PLAYER_BODY = 1u << 3,	// 플레이어 본체 및 Character Controller
		ENEMY_BODY = 1u << 4,	// 적 캐릭터의 본체 충돌체
		NPC_BODY = 1u << 5,		// 비전투 NPC의 본체 충돌체
		PLAYER_HITBOX = 1u << 6,	// 플레이어 근접 공격이나 주문의 공격 판정
		ENEMY_HITBOX = 1u << 7,	// 적 공격의 공격 판정
		PLAYER_PROJECTILE = 1u << 8,	// 플레이어가 발사한 투사체
		ENEMY_PROJECTILE = 1u << 9,	// 적이 발사한 투사체
		TRIGGER = 1u << 10,		// 포탈, 퀘스트, 체크포인트 등 범용 진입 영역
		INTERACTION = 1u << 11,	// 상호작용 가능한 오브젝트를 감지하는 영역
		SENSOR = 1u << 12,		// AI 시야, 탐지, 근접 센서 영역
		RAGDOLL = 1u << 13,		// 래그돌 본과 다른 물리 객체 사이의 충돌
		CLOTH_COLLIDER = 1u << 14,	// 망토·의상 시뮬레이션에 제공할 충돌체
		DEBRIS = 1u << 15,		// 파편과 장식용 소형 물리 오브젝트
		MOVING_PLATFORM = 1u << 16,	// 플레이어, 몬스터와 동적 물체가 올라탈 수 있는 움직이는 발판
		PLAYER_HURTBOX = 1u << 17,	// 적 공격에 피격되는 플레이어의 부위별 판정
		ENEMY_HURTBOX = 1u << 18,	// 플레이어 공격에 피격되는 적의 부위별 판정
	};


	enum class PROTO_GAMEOBJECT
	{
		Prototype_GameObject_DebugPlayer,
		Prototype_GameObject_Player,
		Prototype_GameObject_DebugPlayerThirdPersonCamera,
		Prototype_GameObject_TriggerCRW_SpawnStep,
		Prototype_GameObject_TriggerCRW_StairStep,
		Prototype_GameObject_TriggerCRW_SpawnStep2,
		Prototype_GameObject_TriggerCRW_SpawnStep3,
		Prototype_GameObject_TriggerCRW_SpawnStep4,
		Prototype_GameObject_TriggerCRW_DeSpawnStep,
		Prototype_GameObject_TriggerCRW_DeSpawnStep2,
		Prototype_GameObject_TriggerCRW_DeSpawnStep3,
		Prototype_GameObject_TriggerCRW_DeSpawnStep4,
		Prototype_GameObject_TriggerCRW_BridgeBring,
		Prototype_GameObject_TriggerCRW_BridgeFix,
		Prototype_GameObject_TriggerCRW_ToBoss,
		Prototype_GameObject_TriggerCRW_SpawnMonster1,
		Prototype_GameObject_MyMagicSquareStep,
		Prototype_GameObject_MyMagicSquareStepController,
		Prototype_GameObject_BridgeCRW,
		Prototype_GameObject_BossTMB,
		Prototype_GameObject_TMBGurdian,
		Prototype_GameObject_TmbGurdianDead,
		Prototype_GameObject_BossWeapon,
		Prototype_GameObject_Dragon,
		Prototype_GameObject_Dragon_FireBall,
		Prototype_GameObject_Dragon_Breath,
		Prototype_GameObject_Axe,
		Prototype_GameObject_Sword,
		Prototype_GameObject_Mace,
		Prototype_GameObject_PlayerThirdPersonCamera,
		Prototype_GameObject_PlayerWeapon,
		Prototype_GameObject_Terrain,
		Prototype_GameObject_OilBarrel,
		Prototype_GameObject_RagdollTest,
		Prototype_GameObject_TestPathPlayback,
		Prototype_GameObject_NvClothCape,
		Prototype_GameObject_PlayerMagicBullet,
		Prototype_GameObject_BossStarBurst,
		Prototype_GameObject_BossBall,
		Prototype_GameObject_TombBossBullet,
	};

	enum class PROTO_COMPONENT
	{

	};

	enum class TURN { LEFT_45, LEFT_90, LEFT_135, LEFT_180, RIGHT_45, RIGHT_90, RIGHT_135, RIGHT_180, END };
	enum class ATTMON { SLOT0, SLOT1, SLOT2, SLOT3, SLOT4, SLOT5, SLOT6, SKIP, SLOT7, SLOT8, SLOT9,END };
	enum class EFFMON {EFFSLOT0, EFFSLOT1, EFFSLOT2, EFFSLOT3, EFFSLOT4, EFFSLOT5, EFFSLOT6, EFFSLOT7, END};
	enum class PLAYER_SKILL_TYPE { DEFAULT, ATTACK, ACCIO, DEPULSO , DESCENDO, ACIENT_LIGHTNING, PROTEGO,END};
	enum class MONSTER_TYPE{NORMAL,ELITE, BOSS};

enum class PARTES { WEAPON,EFFECT, END };
}

template <>
struct magic_enum::customize::enum_range<Client::COLLISION_LAYER>
{
	static constexpr bool is_flags = true;
};

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;

#define DECLARE_SINGLE(classname)				\
private:										\
	classname() { }								\
public:											\
	static classname* GetInstance()				\
	{											\
		static classname s_instance;			\
												\
		return &s_instance;						\
	}

#define GET_SINGLE(classname) classname::GetInstance()
