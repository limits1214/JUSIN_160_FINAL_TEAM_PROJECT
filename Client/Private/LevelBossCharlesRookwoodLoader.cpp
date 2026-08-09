#include "pch.h"
#include "LevelBossCharlesRookwoodLoader.h"

#include "Level_Defines.h"

#include "GameInstance.h"
#include "BackGround.h"

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

#include "DebugPlayer.h"
#include "DebugPlayerThirdPersonCamera.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"
#include "BossTMB.h"
#include "BossMace.h"
#include "StarBurst.h"
#include "MonEffectBall.h"
NS_USING(Client)

std::future<bool> CLevelBossCharlesRookwoodLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_BossCharlesRookwood", []()
		{
			// Map Load
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
			{
				return false;
			}
			UILoad();

			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("Lightning")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("TombBossIntro")))
			{
				return false;
			}

			// 디버그 플레이어 프로토타입 등록
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayer, CDebugPlayer::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayer");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_DebugPlayerThirdPersonCamera, CDebugPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_DebugPlayerThirdPersonCamera");
				return false;
			}

			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}
			if (FAILED(LoadPlayerCape()))
				return false;


			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("CHARLES_ROOKWOOD Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}

			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_Player");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}
			if (FAILED(MonsterLoad_InWorker()))
			{
				MSG_BOX("MonsterLoad Failed");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
			{
				MSG_BOX("CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerWeapon");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_PlayerMagicBullet");
				return false;
			}

			return  true;
		});
}

HRESULT CLevelBossCharlesRookwoodLoader::LoadPlayerCape()
{
	constexpr char CAPE_MODEL_PATH[] =
		"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
	const _matrix CapePreTransform =
		XMMatrixScaling(3.f, 3.f, 3.f) *
		XMMatrixRotationY(XMConvertToRadians(180.f)) *
		XMMatrixTranslation(0.f, -1.5f, 0.f);

	if (auto res = CGameInstance::Get().AddResourceT<CResModel>(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		"PLAYER_CAPE_MODEL_RESOURCE",
		CResModel::Create(CAPE_MODEL_PATH)))
	{
		CResModel::DESC Desc{};
		Desc.PreTransformMatrix = CapePreTransform;
		if (FAILED(res->Load(Desc)))
		{
			MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed PLAYER_CAPE_MODEL_RESOURCE");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResourceT<CResNvClothMesh>(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
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
			MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed PLAYER_CAPE_CLOTH_RESOURCE");
			return E_FAIL;
		}
	}

	if (FAILED(E::CGameInstance::Get().AddPrototype(
		LEVEL::BOSS_CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		CNvClothCape::Create())))
	{
		MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_NvClothCape");
		return E_FAIL;
	}

	return S_OK;
}

std::future<bool> CLevelBossCharlesRookwoodLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().ClearAllRunningEffect();

	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();

	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_BossCharlesRookwood", []()
		{
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");   E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);

			CGameInstance::Get().DelPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD);

			CGameInstance::Get().DelResource("MODEL");
			CGameInstance::Get().DelResource(LEVEL::BOSS_CHARLES_ROOKWOOD);
			return true;
		});
}

HRESULT CLevelBossCharlesRookwoodLoader::MonsterLoad_InWorker()
{
	
	{
		//TombBos
		{
			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "Model_Resource_TombBoss",
				CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
			{
				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Model_Resource_TombBoss");
					return E_FAIL;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, CBossTMB::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_BossTMB");
				return E_FAIL;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossStarBurst, CBoss_StarBurst::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_BossStarBurst");
				return E_FAIL;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossBall, CMonEffectBall::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_CMonEffectBall");
				return E_FAIL;
			}
		}

		//Weapon
		{
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::BOSS_CHARLES_ROOKWOOD, "Model_Resource_BossWeapon",
				CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_BossWeapon.bin"))) 
			{
				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Static_Model_Resource_BossWeapon");
					return E_FAIL;
				}
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::BOSS_CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_BossWeapon, CBossMace::Create())))
			{
				MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_BossWeapon");
				return E_FAIL;
			}
		}
	}
	return S_OK;
}

_bool CLevelBossCharlesRookwoodLoader::UILoad()
{
	/**********************UI********************/
	{
		{
			namespace fs = std::filesystem;

			const char* targetDirectories[] = {
				"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
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

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_BOSS_CHARLES_ROOKWOOD", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_BOSS_CHARLES_ROOKWOOD", "Prototype_GameObject_Cursor", CCursor::Create())))
		{
			return false;
		}
	}
}
