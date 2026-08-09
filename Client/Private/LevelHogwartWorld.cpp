#include "pch.h"
#include "LevelHogwartWorld.h"

#include "GameInstance.h"
#include "LevelHogwartWorldLoader.h"
#include "FlyCamera.h"
#include "UiCamera.h"
#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "NvClothCape.h"
#include "UIController.h"
#include "UIManager.h"

// Client에도 같은 이름의 Terrain.h가 있으므로 Engine SDK 헤더를 명시한다.
#include "../../EngineSDK/Inc/Terrain.h"

NS_USING(Client)

CLevelHogwartWorld::CLevelHogwartWorld()
	: CLevel{ ETOUI(LEVEL::HOGWART_WORLD) }
{
}

HRESULT CLevelHogwartWorld::Initialize()
{
	auto& gameInstance = E::CGameInstance::Get();
	gameInstance.GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	if (FAILED(gameInstance.Initialize_EffectLight(15)))
		return E_FAIL;

	const auto hPlayer = SpawnPlayer();
	if (!hPlayer)
		return E_FAIL;

	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(gameInstance.LoadMap(CLevelHogwartWorldLoader::MAP_PATH, true)))
		return E_FAIL;

	if (FAILED(SpawnTerrain(*hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnFlyCamera()) ||
		FAILED(SpawnUICamera()) ||
		FAILED(SpawnPlayerCamera(*hPlayer)))
	{
		return E_FAIL;
	}

	if (FAILED(SpawnSkyBox()))
		return E_FAIL;

	gameInstance.Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);

	return S_OK;
}

void CLevelHogwartWorld::Update(E::_float fTimeDelta)
{
	UNREFERENCED_PARAMETER(fTimeDelta);

	if (!m_bCreatePlayScreenUI)
	{
		CGameObject::GAMEOBJECT_DESC desc{};
		desc.sObjectTag = "UIController";

		const auto hUIController = E::CGameInstance::Get().AddGameObjectToLayer(
			"LEVEL_HOGWART_WORLD",
			"Prototype_GameObject_UIController",
			"UIController",
			&desc);
		GET_SINGLE(UIManager)->SetUIController(hUIController);
		m_bCreatePlayScreenUI = hUIController.has_value();
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();
}

HRESULT CLevelHogwartWorld::Render()
{
	return S_OK;
}

void CLevelHogwartWorld::UpdateGUI()
{
	ImGui::Begin("Level: Hogwart World");
	ImGui::End();
}

std::optional<CHandle> CLevelHogwartWorld::SpawnPlayer()
{
	CPlayer::DESC desc{};
	desc.sObjectTag = "Player";
	// Hogsmeade 중심부의 Terrain 높이(약 48)보다 조금 위에서 시작한다.
	desc.vInitialPosition = { 200.f, 55.f, 80.f };
	desc.LevelTag = LEVEL::HOGWART_WORLD;
	desc.tFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM)
	};

	return E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&desc);
}

HRESULT CLevelHogwartWorld::SpawnPlayerCape(CHandle hPlayer)
{
	CNvClothCape::DESC desc{};
	desc.sObjectTag = "NvClothCape";
	desc.hTarget = hPlayer;
	desc.sResourceGroup = LEVEL::HOGWART_WORLD;
	desc.sModelResourceTag = "PLAYER_CAPE_MODEL_RESOURCE";
	desc.sClothMeshResourceTag = "PLAYER_CAPE_CLOTH_RESOURCE";
	desc.sTargetModelComponentTag = "ComCModelIntance";
	desc.sAttachBoneName = "Spine3";
	desc.vLocalPosition = { 0.05f, 0.08f, 0.f };

	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape.nvclothcollision.json",
		desc.tBodyCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);

	return E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		"03_Player",
		&desc)
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnTerrain(CHandle hPlayer)
{
	E::CTerrain::DESC desc{};
	desc.sObjectTag = "HogwartWorldTerrain";
	desc.textureGroup = MagicEnumToStringView(LEVEL::HOGWART_WORLD);
	desc.textureTag = "TEX2D_Terrain_Tile0";
	desc.tPhysicsFilter = PX_FILTER_DESC{
		.iLayer = ETOUI(COLLISION_LAYER::WORLD_STATIC),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask = PX_ALL_LAYERS
	};

	const auto hTerrain = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Terrain,
		"01_Terrain",
		&desc);
	if (!hTerrain)
		return E_FAIL;

	auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*hTerrain);
	if (!terrain)
		return E_FAIL;

	return terrain->LoadTerrain(CLevelHogwartWorldLoader::TERRAIN_PATH, hPlayer);
}

HRESULT CLevelHogwartWorld::SpawnFlyCamera()
{
	E::CCameraObject::CAMERA_DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	desc.vAt = { 200.f, 48.f, 80.f };
	desc.vEye = { 200.f, 65.f, 60.f };
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "FlyCam";

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		"CAMERAS", "Prototype_GameObject_FlyCamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("FLY", *hCamera))
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnUICamera()
{
	E::CCameraObject::CAMERA_DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
	desc.fNear = 0.f;
	desc.fFar = 1.f;
	desc.fWidth = g_iWinSizeX;
	desc.fHeight = g_iWinSizeY;
	desc.sObjectTag = "UICam";
	desc.vEye = { 0.f, 0.f, -0.1f };

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		"CAMERAS", "Prototype_GameObject_UICamera", "99_CAMERA", &desc);
	return hCamera && SUCCEEDED(E::CGameInstance::Get().RegistCamera("UI", *hCamera))
		? S_OK
		: E_FAIL;
}

HRESULT CLevelHogwartWorld::SpawnPlayerCamera(CHandle hPlayer)
{
	CPlayerThirdPersonCamera::DESC desc{};
	desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	desc.vAt = { 200.f, 55.f, 80.f };
	desc.vEye = { 200.f, 58.f, 73.f };
	desc.fAspect = g_iWinSizeX / static_cast<E::_float>(g_iWinSizeY);
	desc.fFovY = 75.f;
	desc.fNear = 0.1f;
	desc.fFar = 3000.f;
	desc.sObjectTag = "PlayerCamera";
	desc.hTarget = hPlayer;

	const auto hCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::HOGWART_WORLD,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&desc);
	if (!hCamera || FAILED(E::CGameInstance::Get().RegistCamera("PlayerCamera", *hCamera)))
		return E_FAIL;

	E::CGameInstance::Get().SetActiveCamera("PlayerCamera");
	return S_OK;
}

HRESULT CLevelHogwartWorld::SpawnSkyBox()
{
	CGameObject::GAMEOBJECT_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

UPtr<CLevelHogwartWorld> CLevelHogwartWorld::Create()
{
	auto instance = UPtr<CLevelHogwartWorld>(new CLevelHogwartWorld{});
	if (FAILED(instance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelHogwartWorld");
		return nullptr;
	}
	return instance;
}

void CLevelHogwartWorld::Free()
{
	CLevel::Free();
}
