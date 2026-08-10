#include "pch.h"
#include "LevelLastBossRanrokLoader.h"
#include "GameInstance.h"

#include "Player.h"
#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"
#include "PlayerThirdPersonCamera.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"
#include "Level_Defines.h"

// UI
#include "UIController.h"
#include "EffectUI.h"
#include "TextureUI.h"
#include "Button.h"
#include "TextBox.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "MiniMap.h"
#include "GameOverMask.h"
#include "VideoObject.h"
#include "Cursor.h"


#include "EnderDragon.h"
#include "BossMace.h"
#include "EnderDragon_State.h"
#include "EdgFireBall.h"
#include "EdgBreath.h"
#include "EdgPulse.h"

#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
NS_USING(Client)

std::future<bool> CLevelLastBossRanrokLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture(
		"LOADING_LastBossRanrok",
		[]()
		{
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
				return false;

			if (!UILoad_InWorker())
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().LoadCinematic("Lightning")))
			{
				return false;
			}

			if (FAILED(LoadPlayer_InWorker()))
			{
				return false;
			}

			if (FAILED(LoadPlayerCape_InWorker()))
			{
				return false;
			}

			if (FAILED(MonsterLoad_InWorker()))
			{
				MSG_BOX("Create Failed Monster in LOADING_LastBossRanrok");
				return false;
			}

			return true;
		});
}

HRESULT CLevelLastBossRanrokLoader::LoadPlayer_InWorker()
{
	if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(CURR_LEVEL, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
		if (FAILED(res->Load(pDesc))) {
			MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_MODEL_RESROUCE");
			return false;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(CURR_LEVEL, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixScaling(1.f, 1.f, 1.f);
		if (FAILED(res->Load(pDesc))) {
			MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_WEAPON_RESROUCE");
			return false;
		}
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		CURR_LEVEL, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
	{
		MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
		return false;
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		CURR_LEVEL, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
	{
		MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerThirdPersonCamera");
		return false;
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		CURR_LEVEL, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
	{
		MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerWeapon");
		return false;
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		CURR_LEVEL, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
	{
		MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerMagicBullet");
		return false;
	}
	return S_OK;
}

HRESULT CLevelLastBossRanrokLoader::LoadPlayerCape_InWorker()
{
	constexpr char CAPE_MODEL_PATH[] =
		"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
	const _matrix CapePreTransform =
		XMMatrixScaling(3.f, 3.f, 3.f) *
		XMMatrixRotationY(XMConvertToRadians(180.f)) *
		XMMatrixTranslation(0.f, -1.5f, 0.f);

	if (auto res = CGameInstance::Get().AddResourceT<CResModel>(
		CURR_LEVEL,
		"PLAYER_CAPE_MODEL_RESOURCE",
		CResModel::Create(CAPE_MODEL_PATH)))
	{
		CResModel::DESC Desc{};
		Desc.PreTransformMatrix = CapePreTransform;
		if (FAILED(res->Load(Desc)))
		{
			MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_CAPE_MODEL_RESOURCE");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<CResNvClothMesh>(
		CURR_LEVEL,
		"PLAYER_CAPE_CLOTH_RESOURCE",
		CResNvClothMesh::Create(CAPE_MODEL_PATH)))
	{
		CResNvClothMesh::DESC Desc{};
		Desc.PreTransformMatrix = CapePreTransform;
		Desc.sSimulationAnchorBone = "Spine3";
		Desc.iSimulationMeshIndex = 0;
		Desc.iRenderMeshIndex = 1;
		Desc.fWeldTolerance = 1.e-5f;
		Desc.fFixedTopRatio = 0.1f;
		if (FAILED(res->Load(Desc)))
		{
			MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_CAPE_CLOTH_RESOURCE");
			return E_FAIL;
		}
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		CURR_LEVEL,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		CNvClothCape::Create())))
	{
		MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_NvClothCape");
		return E_FAIL;
	}
	{
		_float4x4 mat;
		XMStoreFloat4x4(&mat, XMMatrixIdentity());
		mat._41 = -250.f;
		mat._42 = -250.f;
		mat._43 = -600.f;
		CGameInstance::Get().PlayEffect("Portal", mat);
	}

	return S_OK;
}

std::future<bool> CLevelLastBossRanrokLoader::UnLoad()
{
	LOG_MEMORY("start");

	// 메인스레드 MAP해제
	E::CGameInstance::Get().ClearAllRunningEffect();
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_CharlesRookwood", []()
		{
			// 워커스레드 MAP 해제
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			CGameInstance::Get().DelPrototype(CURR_LEVEL);

			return true;
		});
}

_bool CLevelLastBossRanrokLoader::UILoad_InWorker()
{
	/**********************UI********************/
	{
		{
			namespace fs = std::filesystem;

			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellType",
				"./Resources/SampleClient/Textures/UI/UITexture/SpellSlot",
				"./Resources/SampleClient/Textures/UI/UITexture/DeadScene",
				"./Resources/SampleClient/Textures/UI/UITexture/Cursor"
			};

			// 배열을 순회하며 기존 로직을 한 번만 작성하여 처리합니다.
			for (const auto& targetDir : targetDirectories)
			{
				if (fs::exists(targetDir) && fs::is_directory(targetDir))
				{
					for (const auto& entry : fs::directory_iterator(targetDir))
					{
						if (entry.is_regular_file() && entry.path().extension() == ".png")
						{
							std::string fileName = entry.path().stem().string();
							std::string resTag = "TEX_" + fileName;
							std::string fullPath = entry.path().generic_string();

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LAST_BOSS_RANROK", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_Cursor", CCursor::Create())))
		{
			return false;
		}
	}
	return true;
}
HRESULT CLevelLastBossRanrokLoader::MonsterLoad_InWorker()
{
	//Dragon	
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::LAST_BOSS_RANROK, "Model_Resource_Dragon",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon.bin"))) {

			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.8f, 1.8f, 1.8f) * XMMatrixRotationY(XMConvertToRadians(180.f));

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LAST_BOSS_RANROK Failed Model_Resource_Dragon");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::LAST_BOSS_RANROK, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon, CEnderDragon::Create())))
		{
			MSG_BOX("LAST_BOSS_RANROK Failed Prototype_GameObject_Dragon");
			return E_FAIL;
		}
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::LAST_BOSS_RANROK, "Prototype_Component_Dragon_FSM", CEnderDragon_State::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::LAST_BOSS_RANROK, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, CEdgFireBall::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::LAST_BOSS_RANROK, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Breath, CEdgBreath::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::LAST_BOSS_RANROK, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Pulse, CEdgPulse::Create()))) return E_FAIL;

	}

	return S_OK;
}
