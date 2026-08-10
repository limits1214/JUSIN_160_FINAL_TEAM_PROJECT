#include "pch.h"
#include "LevelLastBossRanrok.h"
#include "LevelLastBossRanrokLoader.h"
#include "GameInstance.h"
#include "Level_Defines.h"
#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "UIManager.h"

#include "Player.h"
#include "PlayerThirdPersonCamera.h"
#include "NvClothCape.h"
#include "UIController.h"

#include "EnderDragon.h"
#include "LightPlacementObject.h"
#include "ClientEvents.h"

NS_USING(Client)

CLevelLastBossRanrok::CLevelLastBossRanrok()
	: CLevel{ ETOUI(LEVEL::LAST_BOSS_RANROK) }
{
}

CLevelLastBossRanrok::~CLevelLastBossRanrok()
{
}

HRESULT CLevelLastBossRanrok::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
		});

	//GET_SINGLE(UIManager)->CreateFadeOut(2.f, 3.f);

	if (FAILED(CGameInstance::Get().Initialize_EffectLight(15)))
	{
		return E_FAIL;
	}

	auto hPlayer = SpawnPlayer();
	if (!hPlayer)
	{
		MSG_BOX("Player Handle Failed To CLevelLastBossRanrok");
		return E_FAIL;
	}

	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().LoadMap(CLevelLastBossRanrokLoader::MAP_PATH, true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
		return E_FAIL;

	if (FAILED(SpawnFlyCamera()))
		return E_FAIL;

	if (FAILED(SpawnUICamera()))
		return E_FAIL;
	if (FAILED(SpawnPlayerCamera(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnMonster(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnLightPlacement()))
		return E_FAIL;

	//if (FAILED(SpawnSkyBox()))
	//	return E_FAIL;

	//if (FAILED(PlayBGM()))
	//	return E_FAIL;

	//SubscribePlayerDeath(*hPlayer);


	return S_OK;
}

void CLevelLastBossRanrok::Update(E::_float fTimeDelta)
{
	{
		if (!m_bCreatePlayScreenUI)
		{
			m_bCreatePlayScreenUI = true;
			CGameObject::GAMEOBJECT_DESC Desc{};
			Desc.sObjectTag = "UIController";

			GET_SINGLE(UIManager)->SetUIController(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_LAST_BOSS_RANROK", "Prototype_GameObject_UIController",
				"UIController", &Desc));
		}
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();

	if (E::CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		//GET_SINGLE(UIManager)->CreateFadeInSceneChange(float delay = 0.f, float playtime = 5.f, LEVEL level = LEVEL::LOGO);
	}
}

HRESULT CLevelLastBossRanrok::Render()
{
	return S_OK;
}

void CLevelLastBossRanrok::UpdateGUI()
{
	ImGui::Begin("level: LastBossRanrok");

	ImGui::End();
}

void CLevelLastBossRanrok::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelLastBossRanrok> CLevelLastBossRanrok::Create()
{
	auto pInstance = Engine::UPtr<CLevelLastBossRanrok>(new CLevelLastBossRanrok{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

HRESULT CLevelLastBossRanrok::SpawnFlyCamera()
{
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { -50.f, 325.f, -25.f };
		Desc.vEye = { -50.f, 400.f, -200.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 5000.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
			if (FAILED(E::CGameInstance::Get().SetActiveCamera("FLY")))
			{
				MSG_BOX("FailedToActivateFlyCamera");
				return E_FAIL;
			}

		}
	}
	return S_OK;
}

HRESULT CLevelLastBossRanrok::SpawnUICamera()
{
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::ORTHOGRAPHIC;
		Desc.fNear = 0.f;
		Desc.fFar = 1.f;
		Desc.fWidth = g_iWinSizeX;
		Desc.fHeight = g_iWinSizeY;
		Desc.sObjectTag = "UICam";
		Desc.vEye = { 0.f, 0.f, -0.1f };

		if (auto uiCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_UICamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("UI", uiCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
		}
	}
	return S_OK;
}

HRESULT CLevelLastBossRanrok::SpawnPlayerCamera(std::optional<CHandle> hPlayer)
{
	if (!hPlayer) return E_FAIL;
	CPlayerThirdPersonCamera::DESC Desc{};
	Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
	Desc.vAt = { 10.f, 50.f, 10.f };
	Desc.vEye = { 10.f, 55.f, 5.f };
	Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
	Desc.fFovY = 65.f;
	Desc.fNear = 0.1f;
	Desc.fFar = 1000.f;
	Desc.sObjectTag = "PlayerCamera";
	Desc.hTarget = hPlayer.value();
	Desc.fYaw = -90.f;

	auto hPlayerCamera = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::LAST_BOSS_RANROK,
		PROTO_GAMEOBJECT::Prototype_GameObject_PlayerThirdPersonCamera,
		"101_CAMERA",
		&Desc);
	if (!hPlayerCamera || FAILED(E::CGameInstance::Get().RegistCamera(
		"PlayerCamera", *hPlayerCamera)))
	{
		return E_FAIL;
	}
	E::CGameInstance::Get().SetActiveCamera("PlayerCamera");
	return S_OK;
}

std::optional<CHandle> CLevelLastBossRanrok::SpawnPlayer()
{
	CPlayer::DESC PlayerDesc{};
	PlayerDesc.sObjectTag = "Player";
	PlayerDesc.vInitialPosition = { -25.f, 228.f, -150.f };
	PlayerDesc.vInitialRotation = { 0.f, 0.f, 0.f };
	PlayerDesc.LevelTag = LEVEL::LAST_BOSS_RANROK;
	PlayerDesc.tFilter = PX_FILTER_DESC{
		 .iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
			ETOUI(COLLISION_LAYER::ENEMY_BODY)
	};
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::LAST_BOSS_RANROK,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
}

HRESULT CLevelLastBossRanrok::SpawnPlayerCape(CHandle hPlayer)
{
	CNvClothCape::DESC Desc{};
	Desc.sObjectTag = "NvClothCape";
	Desc.hTarget = hPlayer;
	Desc.sResourceGroup = LEVEL::LAST_BOSS_RANROK;
	Desc.sModelResourceTag = "PLAYER_CAPE_MODEL_RESOURCE";
	Desc.sClothMeshResourceTag = "PLAYER_CAPE_CLOTH_RESOURCE";
	Desc.sTargetModelComponentTag = "ComCModelIntance";
	Desc.sAttachBoneName = "Spine3";
	Desc.vLocalPosition = { 0.05f, 0.08f, 0.f };

	E::CGameInstance::Get().JsonDeSerialize(
		"./Resources/NvCloth/CollisionRigs/ProfessorCape.nvclothcollision.json",
		Desc.tBodyCollisionRig,
		E::NVCLOTH_COLLISION_RIG_ROOT,
		false);

	if (!E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::LAST_BOSS_RANROK,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		"03_Player",
		&Desc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelLastBossRanrok::SpawnMonster(std::optional<CHandle> hPlayer)
{
	{
		CEnderDragon::DRAGON_DESC Dragon{};
		Dragon.sObjectTag = "Dragon";
		Dragon.TargetHandle = hPlayer.value();
		Dragon.LevelTag = MagicEnumToStringView(LEVEL::LAST_BOSS_RANROK);
		XMStoreFloat3(&Dragon.vPos, XMVectorSet(16.775f, 227.104f, -91.734f, 1.f));
		Dragon.ReSourceTag = "Model_Resource_Dragon";
		Dragon.resBeHaviorMajor = "BTJSON";
		Dragon.resBeHaviorMinor = "ENDERDRAGON";
		Dragon.MonType = MONSTER_TYPE::BOSS;
		auto pDragon = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::LAST_BOSS_RANROK, PROTO_GAMEOBJECT::Prototype_GameObject_Dragon, "02_Dragon", &Dragon);

		if (!pDragon)
		{
			MSG_BOX("Create Dragon Failed in Terrain");
			return E_FAIL;
		}

	}
	return S_OK;
}


HRESULT CLevelLastBossRanrok::SpawnStaticCollision()
{
	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_LastBossRanrok",
			"00_MapCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelLastBossRanrok::SpawnLightPlacement()
{
	CLightPlacementObject::DESC desc{};
	desc.sObjectTag =
		"LastBossRanrokLightPlacement";
	desc.sLightFileName =
		"Level_LastBossRanrok";

	return CGameInstance::Get().
		AddGameObjectToLayer(
			ES_EngineProtoMajorType::PERMANENT,
			ES_EngineProtoGameObject::
			Prototype_GameObject_LightPlacement,
			"Layer_LightPlacement",
			&desc)
		? S_OK
		: E_FAIL;
}


HRESULT CLevelLastBossRanrok::SpawnSkyBox()
{
	CGameObject::GAMEOBJECT_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelLastBossRanrok::PlayBGM()
{
	const _string sSoundPath = "./Resources/SampleClient/Sound/CharlesRookwood/CharlesRookwoodBgm.wav";
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr || !pSoundManager->Preload(sSoundPath))
		return E_FAIL;

	m_bmgID = pSoundManager->Play2D(sSoundPath,
		E::SOUND_PLAY_DESC{
			.sBusID = SOUND_BUS::BGM,
			.fVolume = 1.f,
			.fPitch = 1.f,
			.fFadeInDuration = 1.f,
			.iPriority = 64,
			.bLoop = true
		});
	if (m_bmgID == INVALID_SOUND_ID)
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelLastBossRanrok::StopBGM(_float fDuration)
{
	if (m_bmgID == INVALID_SOUND_ID)
		return S_OK;

	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr ||
		!pSoundManager->FadeOutAndStop(m_bmgID, fDuration))
		return E_FAIL;

	m_bmgID = INVALID_SOUND_ID;

	return S_OK;
}

void CLevelLastBossRanrok::SubscribePlayerDeath(const CHandle& hPlayer)
{
	m_hPlayer = hPlayer;
	m_iPlayerDeathListenerID = CGameInstance::Get().EventSubscribe<FPlayerDied>(
		m_hPlayer,
		[this](const FPlayerDied& Event)
		{
			if (Event.hPlayer != m_hPlayer)
				return;

			StopBGM(Event.fLevelBgmFadeDuration);
		});
}

void CLevelLastBossRanrok::Free()
{
	if (m_iPlayerDeathListenerID != 0)
	{
		CGameInstance::Get().EventUnsubscribe<FPlayerDied>(m_iPlayerDeathListenerID);
		m_iPlayerDeathListenerID = 0;
	}

	StopBGM();
	CLevel::Free();
}
