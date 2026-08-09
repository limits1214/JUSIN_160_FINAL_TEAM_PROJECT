#include "pch.h"
#include "LevelTerrainLoader.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "Terrain.h"
#include "Client_Resources.h"
#include "OilBarrel.h"
#include "TestPathPlaybackObject.h"
#include "RagdollTest.h"
#include "TombBossBullet.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
#include "TmbGurdian.h"
#include "TmbGurdianDead.h"
#include "GurdianWeapon.h"
#include "BossTMB.h"
#include "StarBurst.h"
#include "EnderDragon.h"
#include "BossMace.h"
#include "EnderDragon_State.h"
#include "EdgFireBall.h"
#include "EdgBreath.h"
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

NS_USING(Client)

std::future<bool> CLevelTerrainLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_TERRAIN", []()
		{
			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
			{
				return false;
			}
			if (FAILED(E::CGameInstance::Get().LoadCinematic("Lightning")))
			{
				return false;
			}

			UILoad();
			// oilbarrel
			{
				if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Static_OilBarrel_Resource",
					CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_oil_barrel_0001.bin"))) {

					E::CResStaticModel::DESC pDesc{};
					pDesc.PreTransformMatrix = XMMatrixScaling(300.f, 300.f, 300.f);

					if (FAILED(res->Load(pDesc)))
					{
						MSG_BOX("TERRAIN Failed Static_OilBarrel_Resource");
						//return false;
					}
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype(
					LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_OilBarrel, COilBarrel::Create())))
				{
					MSG_BOX("TERRAIN Failed Prototype_GameObject_OilBarrel");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_RagdollTest,
				CRagdollTest::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_RagdollTest");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_TestPathPlayback,
				CTestPathPlaybackObject::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_TestPathPlayback");
				return false;
			}

			{
				constexpr char CAPE_MODEL_PATH[] =
					"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
				const _matrix CapePreTransform =
					XMMatrixScaling(3.f, 3.f, 3.f) *
					XMMatrixRotationY(
						XMConvertToRadians(180.f)) *
					XMMatrixTranslation(0.f, -1.5f, 0.f);

				if (auto res =
					CGameInstance::Get().
					AddResourceT<CResModel>(
						LEVEL::TERRAIN,
						"PLAYER_CAPE_MODEL_RESOURCE",
						CResModel::Create(
							CAPE_MODEL_PATH)))
				{
					CResModel::DESC Desc{};
					Desc.PreTransformMatrix =
						CapePreTransform;
					if (FAILED(res->Load(Desc)))
					{
						MSG_BOX(
							"TERRAIN Failed PLAYER_CAPE_MODEL_RESOURCE");
						return false;
					}
				}

				if (auto res =
					CGameInstance::Get().
					AddResourceT<CResNvClothMesh>(
						LEVEL::TERRAIN,
						"PLAYER_CAPE_CLOTH_RESOURCE",
						CResNvClothMesh::Create(
							CAPE_MODEL_PATH)))
				{
					CResNvClothMesh::DESC Desc{};
					Desc.PreTransformMatrix =
						CapePreTransform;
					Desc.sSimulationAnchorBone =
						"Spine3";
					Desc.iSimulationMeshIndex = 0;
					Desc.iRenderMeshIndex = 1;
					Desc.fWeldTolerance = 1.e-5f;
					Desc.fFixedTopRatio = 0.1f;
					if (FAILED(res->Load(Desc)))
					{
						MSG_BOX(
							"TERRAIN Failed PLAYER_CAPE_CLOTH_RESOURCE");
						return false;
					}
				}

				if (FAILED(
					E::CGameInstance::Get().
					AddPrototype(
						LEVEL::TERRAIN,
						PROTO_GAMEOBJECT::
							Prototype_GameObject_NvClothCape,
						CNvClothCape::Create())))
				{
					MSG_BOX(
						"TERRAIN Failed Prototype_GameObject_NvClothCape");
					return false;
				}
			}

			// terrain
			{
				if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
				{
					if (FAILED(res->Load()))
					{
						MSG_BOX("");
						//return E_FAIL;
					}
				}
				if (auto res = CGameInstance::Get().AddResource(LEVEL::TERRAIN, "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						MSG_BOX("LEVEL::TERRAIN Failed VIBUFFER_Terrain ");
						//return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Terrain, CTerrain::Create())))
				{
					MSG_BOX("TERRAIN Failed Prototype_GameObject_Terrain");
					return false;
				}
			}


			if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "PLAYER_MODEL_RESROUCE", CResModel::Create("./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin"))) {

				E::CResModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f)) * XMMatrixTranslation(0.f, -1.5f, 0.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_MODEL_RESROUCE");
					return false;
				}
			}
		
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "PLAYER_WEAPON_RESROUCE", CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Wand.bin"))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixScaling(1.f, 1.f, 1.f);
				if (FAILED(res->Load(pDesc))) {
					MSG_BOX("TERRAIN Failed PLAYER_WEAPON_RESROUCE");
					return false;
				}
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Player, CPlayer::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_Player");
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera, CPlayerThirdPersonCamera::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerThirdPersonCamera");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon, CPlayer_Weapon::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerWeapon");
				return false;
			}
			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet, CPlayer_Magic_Bullet::Create())))
			{
				MSG_BOX("TERRAIN Failed Prototype_GameObject_PlayerMagicBullet");
				return false;
			}
			MonsterLoad_InWorker();
			// 워커 스레드 종료
			return  true;
		});
}

std::future<bool> CLevelTerrainLoader::UnLoad()
{
	LOG_MEMORY("start");
	E::CGameInstance::Get().ClearAllRunningEffect();
	LOG_MEMORY("end");
	return E::CGameInstance::Get().WorkerEnqueueWithFuture("UNLOADING_TERRAIN", []()
		{
			CGameInstance::Get().DelPrototype(LEVEL::TERRAIN);
			CGameInstance::Get().DelResource(LEVEL::TERRAIN);

			return true;
		});
}

_bool CLevelTerrainLoader::UILoad()
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

							if (auto res = E::CGameInstance::Get().AddResource("LEVEL_TERRAIN", resTag, E::CResTexture2D::Create(fullPath)))
							{
								res->Load();
							}
						}
					}
				}
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_TextBox", CTextBox::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_Button", CButton::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_SpellMeter", CSpellMeter::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_HPBar", CHPBar::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_MiniMap", CMiniMap::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_UIController", CUIController::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_GameOverMask", CGameOverMask::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_VideoObject", CVideoObject::Create())))
		{
			return false;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TERRAIN", "Prototype_GameObject_Cursor", CCursor::Create())))
		{
			return false;
		}
	}
	return true;
}
HRESULT CLevelTerrainLoader::MonsterLoad_InWorker()
{
	//TombBos
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_TombBoss",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin")))
		{
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TombBoss");
				return E_FAIL;
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossTMB, CBossTMB::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_BossTMB");
			return E_FAIL;
		}

		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_BossWeapon",
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
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossWeapon, CBossMace::Create())))
		{
			MSG_BOX("BOSS_CHARLES_ROOKWOOD Failed Prototype_GameObject_BossWeapon");
			return E_FAIL;
		}
	}
	//BOSSSTAR
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_BossStarBurst, CBoss_StarBurst::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_BossStarBurst");
			return E_FAIL;
		}
	}
	//BOSSBULLET
	{
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN,
			PROTO_GAMEOBJECT::Prototype_GameObject_TombBossBullet, CTombBossBullet::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_TombBossBullet");
			return E_FAIL;
		}
	}
	//TombGurDian
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_TMBGurdian",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Grunt/SK_Tomb_Grunt.bin")))
		{
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Model_Resource_TMBGurdian");
				return E_FAIL;
			}
		}

		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, CTmbGurdian::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TMBGurdian");
			return false;
		}

	}
	//TombGurDianDead
	{
		for (uint32_t i = 0; i < 13; ++i)
		{
			std::string path = "./Resources/SampleClient/Models/Static/SM_Med_" + std::to_string(i) + ".bin";
			StringID resTag = "Static_Med_Debris_" + std::to_string(i);
			if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, resTag,
				CResStaticModel::Create(path))) {

				E::CResStaticModel::DESC pDesc{};
				pDesc.PreTransformMatrix = XMMatrixIdentity();

				if (FAILED(res->Load(pDesc)))
				{
					MSG_BOX("LEVEL_CREATURE Failed Static_Med_Debris");
				}
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(
			LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_TmbGurdianDead, CTmbGurdianDead::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_TmbGurdianDead");
			return false;
		}
	}
	//TombWeapon
	{
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_Mace",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Mace.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Mace_Model_Resource");
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>(LEVEL::TERRAIN, "Model_Resource_Sword",
			CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_Tomb_Sword.bin"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(0.01f, 0.01f, 0.01f);

			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("LEVEL_CREATURE Failed Static_Sword_Model_Resource");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Mace, CGurdianWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Mace");
			return E_FAIL;
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Sword, CGurdianWeapon::Create())))
		{
			MSG_BOX("LEVEL_CREATURE Failed Prototype_GameObject_Sword");
			return E_FAIL;
		}

	}
	//Dragon	
	{ 
		if (auto res = CGameInstance::Get().AddResourceT<E::CResModel>(LEVEL::TERRAIN, "Model_Resource_Dragon",
			CResModel::Create("./Resources/SampleClient/Models/Skeleton/Dragon/SK_Dragon.bin"))) {
		
			E::CResModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(3.f, 3.f, 3.f) * XMMatrixRotationY(XMConvertToRadians(180.f));
		
			if (FAILED(res->Load(pDesc)))
			{
				MSG_BOX("TERRAIN Failed Model_Resource_Dragon");
				return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon, CEnderDragon::Create())))
		{
			MSG_BOX("TERRAIN Failed Prototype_GameObject_Dragon");
			return E_FAIL;
		}
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, "Prototype_Component_Dragon_FSM",CEnderDragon_State::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_FireBall, CEdgFireBall::Create()))) return E_FAIL;
		if (FAILED(CGameInstance::Get().AddPrototype(LEVEL::TERRAIN, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon_Breath, CEdgBreath::Create()))) return E_FAIL;

	}

	return S_OK;
}
