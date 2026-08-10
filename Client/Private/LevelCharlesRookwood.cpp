#include "pch.h"
#include "LevelCharlesRookwood.h"
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

#include "MyMagicSquareStep.h"
#include "MyMagicSquareStepController.h"

#include "BridgeCRW.h"
#include "TmbGurdian.h"
#include "LightPlacementObject.h"
#include "ClientEvents.h"

NS_USING(Client)

CLevelCharlesRookwood::CLevelCharlesRookwood()
	: CLevel{ ETOUI(LEVEL::CHARLES_ROOKWOOD) }
{
}

CLevelCharlesRookwood::~CLevelCharlesRookwood()
{
}

HRESULT CLevelCharlesRookwood::Initialize()
{
	E::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	GET_SINGLE(UIManager)->CreateFadeOut(2.f, 3.f);

	if (FAILED(
		CGameInstance::Get().
			Initialize_EffectLight(15)))
	{
		return E_FAIL;
	}

	auto hPlayer = SpawnPlayer();
	if (!hPlayer)
	{
		MSG_BOX("Player Handle Failed To CLevelCharlesRookwood");
		return E_FAIL;
	}
	if (FAILED(SpawnPlayerCape(*hPlayer)))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().LoadMap("./Resources/json/MapSaved/Tomb12345", true)))
		return E_FAIL;

	if (FAILED(SpawnStaticCollision()))
		return E_FAIL;

	if (FAILED(SpawnFlyCamera()))
		return E_FAIL;

	if (FAILED(SpawnUICamera()))
		return E_FAIL;
	if (FAILED(SpawnPlayerCamera(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnBridge()))
		return E_FAIL;

	if (FAILED(SpawnMyMagicStepController()))
		return E_FAIL;

	if (FAILED(SpawnMonster(hPlayer)))
		return E_FAIL;

	if (FAILED(SpawnLightPlacement()))
		return E_FAIL;

	if (FAILED(SpawnSkyBox()))
		return E_FAIL;

	if (FAILED(PlayBGM()))
		return E_FAIL;

	SubscribePlayerDeath(*hPlayer);
	

	return S_OK;
}

void CLevelCharlesRookwood::Update(E::_float fTimeDelta)
{
	{
		if(!m_bCreatePlayScreenUI)
		{
			m_bCreatePlayScreenUI = true;
			CGameObject::GAMEOBJECT_DESC Desc{};
			Desc.sObjectTag = "UIController";

			auto uiControllerHandle = E::CGameInstance::Get().
				AddGameObjectToLayer(
					"LEVEL_CHARLES_ROOKWOOD",
					"Prototype_GameObject_UIController",
					"UIController",
					&Desc);
			GET_SINGLE(UIManager)->SetUIController(uiControllerHandle);

			if (uiControllerHandle)
			{
				E::CGameInstance::Get().EventPublish(
					FQuestUIGroupChanged{
						.Group = QUEST_UI_GROUP::ROOKWOOD_TRIAL_01,
						.Active = true
					});
			}
		}
	}

	GET_SINGLE(UIManager)->UpdateRootUIHandles();

	if (E::CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		//GET_SINGLE(UIManager)->CreateFadeInSceneChange(float delay = 0.f, float playtime = 5.f, LEVEL level = LEVEL::LOGO);
	}
}

HRESULT CLevelCharlesRookwood::Render()
{
	return S_OK;
}

void CLevelCharlesRookwood::UpdateGUI()
{
	ImGui::Begin("level: CharlesRookwood");

	ImGui::End();
}

void CLevelCharlesRookwood::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelCharlesRookwood> CLevelCharlesRookwood::Create()
{
	auto pInstance = Engine::UPtr<CLevelCharlesRookwood>(new CLevelCharlesRookwood{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

HRESULT CLevelCharlesRookwood::SpawnFlyCamera()
{
	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 1000.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				MSG_BOX("FailedToRegistCamera");
				return E_FAIL;
			}
			
		}
	}
	return S_OK;
}

HRESULT CLevelCharlesRookwood::SpawnUICamera()
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

HRESULT CLevelCharlesRookwood::SpawnPlayerCamera(std::optional<CHandle> hPlayer)
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
		LEVEL::CHARLES_ROOKWOOD,
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

std::optional<CHandle> CLevelCharlesRookwood::SpawnPlayer()
{
	CPlayer::DESC PlayerDesc{};
	PlayerDesc.sObjectTag = "Player";
	PlayerDesc.vInitialPosition = { -6.f, -215.f, 156.f };
	PlayerDesc.vInitialRotation = { 0.f, -90.f, 0.f };
	PlayerDesc.LevelTag = LEVEL::CHARLES_ROOKWOOD;
	PlayerDesc.tFilter = PX_FILTER_DESC{
		 .iLayer = ETOUI(COLLISION_LAYER::PLAYER_BODY),
		.iSimulationMask = PX_ALL_LAYERS,
		.iQueryMask =
			ETOUI(COLLISION_LAYER::WORLD_STATIC) |
			ETOUI(COLLISION_LAYER::MOVING_PLATFORM) |
			ETOUI(COLLISION_LAYER::ENEMY_BODY)
	};
	return  E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_Player,
		"03_Player",
		&PlayerDesc);
}

HRESULT CLevelCharlesRookwood::SpawnPlayerCape(CHandle hPlayer)
{
	CNvClothCape::DESC Desc{};
	Desc.sObjectTag = "NvClothCape";
	Desc.hTarget = hPlayer;
	Desc.sResourceGroup = LEVEL::CHARLES_ROOKWOOD;
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
		LEVEL::CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_NvClothCape,
		"03_Player",
		&Desc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelCharlesRookwood::SpawnMonster(std::optional<CHandle> hPlayer)
{
	// 레벨배치 엘리트몹 by SY
	{
		{
			//리트리트리트리트엘리트리트리트리트리
			CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
			TmbGurdianDesc.sObjectTag = "TmbGurdian";
			TmbGurdianDesc.TargetHandle = hPlayer.value();
			TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
			TmbGurdianDesc.vPos = _float3(-232.f, -227.f, -219.f);
			TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";
			TmbGurdianDesc.resBeHaviorMajor = "BTJSON";
			TmbGurdianDesc.resBeHaviorMinor = "TOMB_BT_GURDIANKNIGHT";
			TmbGurdianDesc.MonType = MONSTER_TYPE::ELITE;
			TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Sword);
			TmbGurdianDesc.WeaponResourceName = "Model_Resource_Sword";
			TmbGurdianDesc.vWeaponScale = _float3(100.f, 100.f, 100.f);
			TmbGurdianDesc.vScale = _float3(3.f, 3.f, 3.f);
			auto EliteTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

			if (!EliteTmb)
			{
				MSG_BOX("Create TmbGurdian Failed in Rookwood");
				return E_FAIL;
			}
		}

		{
			//리트리트리트리트엘리트리트리트리트리
			CTmbGurdian::TMBGURDIAN_DESC TmbGurdianDesc{};
			TmbGurdianDesc.sObjectTag = "TmbGurdian";
			TmbGurdianDesc.TargetHandle = hPlayer.value();
			TmbGurdianDesc.LevelTag = MagicEnumToStringView(LEVEL::CHARLES_ROOKWOOD);
			TmbGurdianDesc.vPos = _float3(-270.f, -227.f, -219.f);
			TmbGurdianDesc.ReSourceTag = "Model_Resource_TMBGurdian";
			TmbGurdianDesc.resBeHaviorMajor = "BTJSON";
			TmbGurdianDesc.resBeHaviorMinor = "TOMB_BT_GURDIANKNIGHT";
			TmbGurdianDesc.MonType = MONSTER_TYPE::ELITE;
			TmbGurdianDesc.WeaponProtoName = MagicEnumToStringView(PROTO_GAMEOBJECT::Prototype_GameObject_Sword);
			TmbGurdianDesc.WeaponResourceName = "Model_Resource_Sword";
			TmbGurdianDesc.vWeaponScale = _float3(100.f, 100.f, 100.f);
			TmbGurdianDesc.vScale = _float3(3.f, 3.f, 3.f);
			auto EliteTmb = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD, PROTO_GAMEOBJECT::Prototype_GameObject_TMBGurdian, "02_TmbGurdian", &TmbGurdianDesc);

			if (!EliteTmb)
			{
				MSG_BOX("Create TmbGurdian Failed in Rookwood");
				return E_FAIL;
			}
		}
	}
	return S_OK;
}


HRESULT CLevelCharlesRookwood::SpawnStaticCollision()
{
	auto handles = CGameInstance::Get()
		.GetPhysXManager()
		->CreateCollisionProxyObjectsFromFile(
			"Level_CharlesRookwood",
			"00_MapCollision");

	if (handles.empty())
		return E_FAIL;

	return S_OK;
}

HRESULT CLevelCharlesRookwood::SpawnLightPlacement()
{
	CLightPlacementObject::DESC desc{};
	desc.sObjectTag =
		"CharlesRookwoodLightPlacement";
	desc.sLightFileName =
		"RookWoodStage_LightProtoType_A";

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

HRESULT CLevelCharlesRookwood::SpawnMyMagicStepController()
{
	CMyMagicSquareStepController::DESC Desc{};
	Desc.ProtoMajorTag = LEVEL::CHARLES_ROOKWOOD;
	Desc.ProtoMinorTag = PROTO_GAMEOBJECT::Prototype_GameObject_MyMagicSquareStep;
	Desc.SpawnLayerName = "23_MyMagicSquareStep";
	Desc.ResMajorTag = LEVEL::CHARLES_ROOKWOOD;
	Desc.ResMinorTag = "Static_SquareStep_A_Resource";

	auto h = E::CGameInstance::Get().AddGameObjectToLayer(
		LEVEL::CHARLES_ROOKWOOD,
		PROTO_GAMEOBJECT::Prototype_GameObject_MyMagicSquareStepController,
		"22_MyMagicSquareStepController",
		&Desc);

	if (!h)
	{
		return E_FAIL;
	}

	float fRefY{230.f};
	// test
	{
		auto* pController = E::CGameInstance::Get()
			.GetGameObjectByHandleT<CMyMagicSquareStepController>(
				*h);
		if (!pController)
			return E_FAIL;
		{
			const StringID GroupID{ "MagicSquareGrid1" };
			CMyMagicSquareStepController::RECT_GROUP_DESC
				RectDesc{};
			// -212
			RectDesc.vStartPosition = { -245.f - 10.f, -230.f , 52.f - 59.f };
			RectDesc.iCountX = 10;
			RectDesc.iCountZ = 59;
			RectDesc.fSpacingX = 1.007f;
			RectDesc.fSpacingZ = 1.007f;

			if (!pController->RegistRectGroup( GroupID, RectDesc)
				//|| !pController->SpawnGroup(GroupID)
				)
				return E_FAIL;
		}

		//if(false)
		{
			const StringID GroupID{ "MagicSquareGrid2" };
			CMyMagicSquareStepController::RECT_GROUP_DESC
				RectDesc{};
			// -212
			RectDesc.vStartPosition = { -246.f - 8.f, -250.3f , -72.f - (1.007f * 35.f) };
			RectDesc.iCountX = 8;
			RectDesc.iCountZ = 35;
			RectDesc.fSpacingX = 1.007f;
			RectDesc.fSpacingZ = 1.007f;

			if (!pController->RegistRectGroup( GroupID, RectDesc)
				//|| !pController->SpawnGroup(GroupID)
				)
				return E_FAIL;
		}

		

		
		{
			const StringID CircleGroupID{
			"CreatureMagicCircleGrid" };
			CMyMagicSquareStepController::
				FILLED_CIRCLE_GROUP_DESC CircleDesc{};
			CircleDesc.vCenter = { -247.f - 3.5f, -250.3f , -72.f - (1.007f * 35.f) - (1.007f * 14.f) };
			CircleDesc.fRadius = 14.f;
			CircleDesc.fSpacing = 1.007f;
			//RiseDesc.eFillMode =
			//	CMyMagicSquareStepController::
			//	RISE_FILL_MODE::RADIAL;
			//RiseDesc.fStepTimingJitter = 0.08f;

			if (!pController->RegistFilledCircleGroup( CircleGroupID, CircleDesc)
				//|| !pController->SpawnGroup( CircleGroupID)
				)
				return E_FAIL;
		}

		
		{
			
			const StringID GroupID{ "MagicSquareGrid3" };
			CMyMagicSquareStepController::RECT_GROUP_DESC
				RectDesc{};
			// -212
			RectDesc.vStartPosition = { -246.f - 8.f, -250.4f , -72.f - (1.007f * 35.f) - (1.007f * 14.f * 2.f) - (1.007f * 34.f) + (1.007f)};
			RectDesc.iCountX = 8;
			RectDesc.iCountZ = 34;
			RectDesc.fSpacingX = 1.007f;
			RectDesc.fSpacingZ = 1.007f;

			if (!pController->RegistRectGroup( GroupID, RectDesc)
				//|| !pController->SpawnGroup(GroupID)
				)
				return E_FAIL;
		}

		
	}

	return S_OK;
}

HRESULT CLevelCharlesRookwood::SpawnSkyBox()
{
	CGameObject::GAMEOBJECT_DESC skyDesc{};
	skyDesc.sObjectTag = "SkyCloudyCube";
	if (!CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_SkyCloudyCube", "00_SKYBOX", &skyDesc))
	{
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLevelCharlesRookwood::PlayBGM()
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

HRESULT CLevelCharlesRookwood::StopBGM(_float fDuration)
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

void CLevelCharlesRookwood::SubscribePlayerDeath(const CHandle& hPlayer)
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

HRESULT CLevelCharlesRookwood::SpawnBridge()
{

	CBridgeCRW::DESC Desc{};

	auto hBridgeCRW = E::CGameInstance::Get().AddGameObjectToLayer(LEVEL::CHARLES_ROOKWOOD,PROTO_GAMEOBJECT::Prototype_GameObject_BridgeCRW,"BridgeCRW", &Desc);

	if (hBridgeCRW)
	{
		if (auto pObj = CGameInstance::Get().GetGameObjectByHandleT<CBridgeCRW>(*hBridgeCRW))
		{
			pObj->GetTransform().SetPosition(_float3{ -251.f, -242.f, -382.f });
		}
	}
	
	return S_OK;
}

void CLevelCharlesRookwood::Free()
{
	if (m_iPlayerDeathListenerID != 0)
	{
		CGameInstance::Get().EventUnsubscribe<FPlayerDied>(m_iPlayerDeathListenerID);
		m_iPlayerDeathListenerID = 0;
	}

	StopBGM();
	CLevel::Free();
}
