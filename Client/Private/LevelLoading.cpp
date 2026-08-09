#include "pch.h"
#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "LevelLogo.h"
#include "LevelLogoLoader.h"
#include "LevelCharlesRookwood.h"
#include "LevelCharlesRookwoodLoader.h"

#include "LevelBossCharlesRookwood.h"
#include "LevelBossCharlesRookwoodLoader.h"
#include "LevelHogwartWorld.h"
#include "LevelHogwartWorldLoader.h"

#include "UIManager.h"
#include "UICamera.h"
#include "FlyCamera.h"

#include "LevelTerrain.h"
#include "LevelTerrainLoader.h"

#include "LevelUIEditor.h"
#include "LevelUIEditorLoader.h"

#include "LevelLastBossRanrok.h"
#include "LevelLastBossRanrokLoader.h"
NS_USING(Client)

CLevelLoading::CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept
	: CLevel{ ETOUI(LEVEL::LOADING) }
	, m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_eNextLevelIndex(eNextLevelIndex)
{
	const uint32_t iCurrentLevelID = Engine::CGameInstance::Get().GetCurrentLevelID();
	if (iCurrentLevelID != Engine::CLevel::INVALID_LEVEL_ID)
	{
		m_ePreviousLevelIndex = static_cast<LEVEL>(iCurrentLevelID);
	}
}

CLevelLoading::~CLevelLoading()
{
}

bool CLevelLoading::IsLevelChangeLocked() const
{
	return m_ePhase != PHASE::COMPLETE && m_ePhase != PHASE::FAILED;
}

HRESULT CLevelLoading::Initialize()
{
	LOG_MEMORY("CLevelLoading::Initialize");

	Engine::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	GET_SINGLE(UIManager)->CreateFadeOut(1.f, 2.f);

	{
		E::CCameraObject::CAMERA_DESC Desc{};
		Desc.eProj = E::CCameraObject::PROJ::PERSPECTIVE;
		Desc.vAt = { 0.f, 0.f, 0.f };
		Desc.vEye = { 0.f, 0.f, -5.f };
		Desc.fAspect = { g_iWinSizeX / (E::_float)g_iWinSizeY };
		Desc.fFovY = 75.f;
		Desc.fNear = 0.1f;
		Desc.fFar = 100.f;
		Desc.sObjectTag = "FlyCam";

		if (auto flyCam = E::CGameInstance::Get().AddGameObjectToLayer("CAMERAS", "Prototype_GameObject_FlyCamera",
			"99_CAMERA", &Desc))
		{
			if (FAILED(E::CGameInstance::Get().RegistCamera("FLY", flyCam.value())))
			{
				int x = 0;
			}
			E::CGameInstance::Get().SetActiveCamera("FLY");
		}
	}

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
				int x = 0;
			}

			{
				const auto* t = E::CGameInstance::GetConst().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());

				auto* t2 = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());
			}
		}
	}

	return S_OK;
}

void CLevelLoading::Update(E::_float fTimeDelta)
{
	// UI LOADING SCENE CREATE
	if (!m_bLoadUiResource)
	{
		switch (m_eNextLevelIndex)
		{
		case LEVEL::LOGO :
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon1");
			m_bLoadUiResource = true;
			break;
		case LEVEL::CHARLES_ROOKWOOD:
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon1");
			m_bLoadUiResource = true;
			break;
		case LEVEL::BOSS_CHARLES_ROOKWOOD:
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon2");
			m_bLoadUiResource = true;
			break;
		case LEVEL::HOGWART_WORLD:
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon1");
			m_bLoadUiResource = true;
			break;
		case LEVEL::LAST_BOSS_RANROK:
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon2");
			m_bLoadUiResource = true;
			break;
		default:
			GET_SINGLE(UIManager)->LoadPrefab("LoadingDungeon2");
			m_bLoadUiResource = true;
			break;
		}
	}
	GET_SINGLE(UIManager)->UpdateRootUIHandles();

	switch (m_ePhase)
	{
	case PHASE::READY:
		StartUnload();
		break;
	case PHASE::UNLOADING:
		CheckUnload();
		break;
	case PHASE::LOADING:
		CheckLoad();
		break;
	default:
		break;
	}
}

HRESULT CLevelLoading::Render()
{
	return S_OK;
}

void CLevelLoading::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevelLoading");
	ImGui::End();
}

void CLevelLoading::FrameEnd(E::_float fTimeDelta)
{
	if (m_bLoadEnd)
	{
		LoadEnd();
		return;
	}
}

HRESULT CLevelLoading::LoadEnd()
{
	Engine::UPtr<CLevel>	pNewLevel{};
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
		pNewLevel = CLevelLogo::Create();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		pNewLevel = CLevelCharlesRookwood::Create();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		pNewLevel = CLevelBossCharlesRookwood::Create();
		break;
	case LEVEL::TERRAIN:
		pNewLevel = CLevelTerrain::Create();
		break;
	case LEVEL::HOGWART_WORLD:
		pNewLevel = CLevelHogwartWorld::Create();
		break;
	case LEVEL::UIEDITOR:
		pNewLevel = CLevelUIEditor::Create();
		break;
	case LEVEL::LAST_BOSS_RANROK:
		pNewLevel = CLevelLastBossRanrok::Create();
		break;
	}
	assert(pNewLevel);

	if (FAILED(Engine::CGameInstance::Get().ChangeLevel(std::move(pNewLevel))))
	{
		MSG_BOX("ChangeLevelFailed in loading");
		return E_FAIL;
	}
	return S_OK;
}

void CLevelLoading::StartUnload()
{
	if (!m_ePreviousLevelIndex || *m_ePreviousLevelIndex == LEVEL::LOADING)
	{
		StartLoad();
		return;
	}

	m_ePhase = PHASE::UNLOADING;
	switch (*m_ePreviousLevelIndex)
	{
	case LEVEL::LOGO:
		m_futUnloadFinish = CLevelLogoLoader::UnLoad();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		m_futUnloadFinish = CLevelCharlesRookwoodLoader::UnLoad();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		m_futUnloadFinish = CLevelBossCharlesRookwoodLoader::UnLoad();
		break;
	case LEVEL::TERRAIN:
		m_futUnloadFinish = CLevelTerrainLoader::UnLoad();
		break;
	case LEVEL::HOGWART_WORLD:
		m_futUnloadFinish = CLevelHogwartWorldLoader::UnLoad();
		break;
	case LEVEL::UIEDITOR:
		m_futUnloadFinish = CLevelUIEditorLoader::UnLoad();
		break;
	case LEVEL::LAST_BOSS_RANROK:
		m_futUnloadFinish = CLevelLastBossRanrokLoader::UnLoad();
		break;
	default:
		StartLoad();
		break;
	}
}

void CLevelLoading::CheckUnload()
{
	if (!m_futUnloadFinish.valid())
		return;

	if (m_futUnloadFinish.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
		return;

	if (!m_futUnloadFinish.get())
	{
		m_ePhase = PHASE::FAILED;
		MSG_BOX("UNLOADING FAILED");
		return;
	}

	StartLoad();
}

void CLevelLoading::StartLoad()
{
	m_ePhase = PHASE::LOADING;
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
		m_futLoadFinish = CLevelLogoLoader::Load();
		break;
	case LEVEL::CHARLES_ROOKWOOD:
		m_futLoadFinish = CLevelCharlesRookwoodLoader::Load();
		break;
	case LEVEL::BOSS_CHARLES_ROOKWOOD:
		m_futLoadFinish = CLevelBossCharlesRookwoodLoader::Load();
		break;
	case LEVEL::TERRAIN:
		m_futLoadFinish = CLevelTerrainLoader::Load();
		break;
	case LEVEL::HOGWART_WORLD:
		m_futLoadFinish = CLevelHogwartWorldLoader::Load();
		break;
	case LEVEL::UIEDITOR:
		m_futLoadFinish = CLevelUIEditorLoader::Load();
		break;
	case LEVEL::LAST_BOSS_RANROK:
		m_futLoadFinish = CLevelLastBossRanrokLoader::Load();
		break;
	default:
		m_ePhase = PHASE::COMPLETE;
		m_bLoadEnd = true;
		break;
	}

}

void CLevelLoading::CheckLoad()
{
	if (m_futLoadFinish.valid())
	{
		if (m_futLoadFinish.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			if (!m_futLoadFinish.get())
			{
				m_ePhase = PHASE::FAILED;
				MSG_BOX("LOADING FAILED");
				return;
			}

			m_ePhase = PHASE::COMPLETE;
			m_bLoadEnd = true;
		}
	}
}

Engine::UPtr<CLevelLoading> CLevelLoading::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex)
{
	auto pInstance = Engine::UPtr<CLevelLoading>(new CLevelLoading(pDevice, pContext, eNextLevelIndex));
	pInstance->SetDeferredInitialization();
	return pInstance;
}

void CLevelLoading::Free()
{
	LOG_MEMORY("CLevelLoading::Free");
	CLevel::Free();
}
