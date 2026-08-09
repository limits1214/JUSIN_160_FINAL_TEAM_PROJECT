#include "pch.h"
#include "LevelHogwartWorldLoader.h"

#include "GameInstance.h"
#include "Level_Defines.h"
#include "Client_Resources.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "Player_Weapon.h"
#include "Player_Magic_Bullet.h"
#include "NvClothCape.h"
#include "ResNvClothMesh.h"

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

// Client Terrain과 구분하기 위해 Engine Terrain 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"

NS_USING(Client)

std::future<bool> CLevelHogwartWorldLoader::Load()
{
	return E::CGameInstance::Get().WorkerEnqueueWithFuture(
		"LOADING_HogwartWorld",
		[]()
		{
			if (FAILED(E::CGameInstance::Get().LoadMapResources(MAP_PATH)))
				return false;

			if (!UILoad())
				return false;

			if (FAILED(E::CGameInstance::Get().LoadCinematic("AcientThunderAttack")))
				return false;

			if (auto texture = E::CGameInstance::Get().AddResource(
				LEVEL::HOGWART_WORLD,
				"TEX2D_Terrain_Tile0",
				E::CResTexture2D::Create(
					"./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
			{
				if (FAILED(texture->Load()))
					return false;
			}
			else
			{
				return false;
			}

			if (FAILED(E::CGameInstance::Get().AddPrototype(
				LEVEL::HOGWART_WORLD,
				PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
				E::CTerrain::Create())))
			{
				return false;
			}

			return SUCCEEDED(LoadPlayerResources());
		});
}

std::future<bool> CLevelHogwartWorldLoader::UnLoad()
{
	E::CGameInstance::Get().ClearAllRunningEffect();
	E::CGameInstance::Get().ClearAllChunk();
	E::CGameInstance::Get().GetNavMeshManager()->Clear();
	E::CGameInstance::Get().Clear_DynamicLightList();

	return E::CGameInstance::Get().WorkerEnqueueWithFuture(
		"UNLOADING_HogwartWorld",
		[]()
		{
			E::CGameInstance::Get().DelPrototype("MAPEDITOR");
			E::CGameInstance::Get().DelResource("MAPEDITOR");
			E::CGameInstance::Get().DelResource(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);
			E::CGameInstance::Get().DelPrototype("LEVEL_HOGWART_WORLD");
			E::CGameInstance::Get().DelResource("LEVEL_HOGWART_WORLD");
			E::CGameInstance::Get().DelPrototype(LEVEL::HOGWART_WORLD);
			E::CGameInstance::Get().DelResource(LEVEL::HOGWART_WORLD);
			return true;
		});
}

HRESULT CLevelHogwartWorldLoader::LoadPlayerResources()
{
	if (auto model = E::CGameInstance::Get().AddResourceT<E::CResModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_MODEL_RESROUCE",
		E::CResModel::Create(
			"./Resources/SampleClient/Models/Skeleton/professor/SK_professor.bin")))
	{
		E::CResModel::DESC desc{};
		desc.PreTransformMatrix =
			XMMatrixScaling(3.f, 3.f, 3.f) *
			XMMatrixRotationY(XMConvertToRadians(180.f)) *
			XMMatrixTranslation(0.f, -1.5f, 0.f);
		if (FAILED(model->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	if (FAILED(LoadPlayerCape()))
		return E_FAIL;

	if (auto weapon = E::CGameInstance::Get().AddResourceT<E::CResStaticModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_WEAPON_RESROUCE",
		E::CResStaticModel::Create(
			"./Resources/SampleClient/Models/Static/SM_Wand.bin")))
	{
		E::CResStaticModel::DESC desc{};
		desc.PreTransformMatrix = XMMatrixRotationX(XMConvertToRadians(-90.f)) * XMMatrixIdentity();
		if (FAILED(weapon->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	auto& gameInstance = E::CGameInstance::Get();
	if (FAILED(gameInstance.AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerWeapon,
		CPlayer_Weapon::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerMagicBullet,
			CPlayer_Magic_Bullet::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_Player,
			CPlayer::Create())) ||
		FAILED(gameInstance.AddPrototype(
			LEVEL::HOGWART_WORLD,
			PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
			CPlayerThirdPersonCamera::Create())))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelHogwartWorldLoader::LoadPlayerCape()
{
	constexpr char CAPE_MODEL_PATH[] =
		"./Resources/SampleClient/Models/Skeleton/clothes/SK_clothes.bin";
	const _matrix preTransform =
		XMMatrixScaling(3.f, 3.f, 3.f) *
		XMMatrixRotationY(XMConvertToRadians(180.f)) *
		XMMatrixTranslation(0.f, -1.5f, 0.f);

	if (auto model = E::CGameInstance::Get().AddResourceT<E::CResModel>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_CAPE_MODEL_RESOURCE",
		E::CResModel::Create(CAPE_MODEL_PATH)))
	{
		E::CResModel::DESC desc{};
		desc.PreTransformMatrix = preTransform;
		if (FAILED(model->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	if (auto cloth = E::CGameInstance::Get().AddResourceT<E::CResNvClothMesh>(
		LEVEL::HOGWART_WORLD,
		"PLAYER_CAPE_CLOTH_RESOURCE",
		E::CResNvClothMesh::Create(CAPE_MODEL_PATH)))
	{
		E::CResNvClothMesh::DESC desc{};
		desc.PreTransformMatrix = preTransform;
		desc.sSimulationAnchorBone = "Spine3";
		desc.iSimulationMeshIndex = 0;
		desc.iRenderMeshIndex = 1;
		desc.fWeldTolerance = 1.e-5f;
		desc.fFixedTopRatio = 0.1f;
		if (FAILED(cloth->Load(desc)))
			return E_FAIL;
	}
	else
	{
		return E_FAIL;
	}

	return E::CGameInstance::Get().AddPrototype(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		CNvClothCape::Create());
}

_bool CLevelHogwartWorldLoader::UILoad()
{
	namespace fs = std::filesystem;
	constexpr const char* UI_GROUP = "LEVEL_HOGWART_WORLD";
	const char* directories[] = {
		"./Resources/SampleClient/Textures/UI/UITexture/PlayScreen",
		"./Resources/SampleClient/Textures/UI/UITexture/SpellType",
		"./Resources/SampleClient/Textures/UI/UITexture/DeadScene",
		"./Resources/SampleClient/Textures/UI/UITexture/Cursor"
	};

	for (const char* directory : directories)
	{
		if (!fs::exists(directory) || !fs::is_directory(directory))
			continue;

		for (const auto& entry : fs::directory_iterator(directory))
		{
			if (!entry.is_regular_file() || entry.path().extension() != ".png")
				continue;

			const std::string tag = "TEX_" + entry.path().stem().string();
			auto texture = E::CGameInstance::Get().AddResource(
				UI_GROUP,
				tag,
				E::CResTexture2D::Create(entry.path().generic_string()));
			if (!texture || FAILED(texture->Load()))
				return false;
		}
	}

	auto& gameInstance = E::CGameInstance::Get();
	return
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_TextureUI", CTextureUI::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_EffectUI", CEffectUI::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_TextBox", CTextBox::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_Button", CButton::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_SpellMeter", CSpellMeter::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_HPBar", CHPBar::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_MiniMap", CMiniMap::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_UIController", CUIController::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_GameOverMask", CGameOverMask::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_VideoObject", CVideoObject::Create())) &&
		SUCCEEDED(gameInstance.AddPrototype(UI_GROUP, "Prototype_GameObject_Cursor", CCursor::Create()));
}
