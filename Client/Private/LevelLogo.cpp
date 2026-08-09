#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Level_Defines.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "BackGround.h"
#include "UiCamera.h"

#include "LevelLogoLoader.h"

// UI
#include "UIManager.h"
#include "UIController.h"
#include "VideoObject.h"

NS_USING(Client)

CLevelLogo::CLevelLogo()
	: CLevel{ ETOUI(LEVEL::LOGO) }
{
}

CLevelLogo::~CLevelLogo()
{
}

HRESULT CLevelLogo::Initialize()
{
	E::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});
	//Engine::CGameInstance::Get().GameObjectLayerInitialize(E::ETOUI(LEVEL_LOADING_LAYERS::END), LevelLoadingLayersToString);

	//{
	//	CBackGround::UIOBJECT_DESC Desc{};
	//	Desc.sObjectTag = "BackGround";
	//	if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_LOGO", "Prototype_GameObject_BackGround",
	//		"00_OBJECTS", &Desc)))
	//	{
	//		return E_FAIL;
	//	}
	//}

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

			// dynamic_cast vs static_cast benchmark 
			// dynamic_cast: 472ms, static_cast: 41ms
			if constexpr (false)
			{
				E::CGameObject* volatile val = E::CGameInstance::Get().GetGameObjectByHandle(uiCam.value());
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							sink = dynamic_cast<E::CUICamera*>(val);
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
				{
					auto start = std::chrono::high_resolution_clock::now();
					{
						E::CUICamera* volatile sink = nullptr;
						for (size_t i = 0; i < 10'000'000; ++i)
						{
							if (val->IsA(E::CUICamera::StaticType))
							{
								sink = static_cast<E::CUICamera*>(val);
							}
							else
							{
								sink = nullptr;
							}
						}
					}
					auto end = std::chrono::high_resolution_clock::now();
					auto cost = std::chrono::duration<double, std::milli>(end - start).count();
					MSG_BOX_STR(std::to_wstring(cost).c_str());
				}
			}
			{
				const auto* t = E::CGameInstance::GetConst().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());

				auto* t2 = E::CGameInstance::Get().GetGameObjectByHandleT<E::CUICamera>(uiCam.value());
			}
		}


	}

	GET_SINGLE(UIManager)->CreateFadeOut(1.f, 5.f);

	return S_OK;
}

void CLevelLogo::Update(E::_float fTimeDelta)
{
	if (!m_VideoQue)
	{
		m_Video = GET_SINGLE(UIManager)->LoadPrefab("LogoVideo").front();
		static_cast<CVideoObject*>(SafeGetOBJ(m_Video))->SetPath(L"./Resources/SampleClient/Textures/UI/Video/CIN_HL.avi");

		//CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
		//PlayFadeOutDelete(hBG);

		m_Logo = GET_SINGLE(UIManager)->LoadPrefab("Logo").front();
		SafeGetOBJ(m_Logo)->SetAlpha(0.f);
		PlayFadeInSacleUp(m_Logo, 9.f, 5.f);

		m_VideoQue = true;

		{
			m_SoundId = E::CGameInstance::Get().GetSoundManager()->Play2D("./Resources/SampleClient/Sound/UI/Hedwings.mp3", SOUND_PLAY_DESC{
					.sBusID = SOUND_BUS::UI,
					.fVolume = 0.3f,
					.fPitch = 1.f,
					.iPriority = 64,
					.bLoop = false
				});
		}
	}

	if(!m_ChangeScene)
		m_SceneChangeTimer += fTimeDelta;

	if (!m_isLogoDelete && m_SceneChangeTimer > 17.f)
	{
		PlayFadeOutDelete(m_Logo, 0.f, 3.f);
		m_isLogoDelete = true;
	}
		

	if (!m_ChangeScene && E::CGameInstance::Get().KeyDown(DIK_SPACE))
	{
		CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
		GetSafeUI(hBG)->SetAlpha(0.f);
		PlayFadeInChange(hBG);
		m_ChangeScene = true;
	}
	else if (!m_ChangeScene && m_SceneChangeTimer > 17.8f)
	{
		CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
		GetSafeUI(hBG)->SetAlpha(0.f);
		PlayFadeInChange(hBG);

		m_SceneChangeTimer = 0.f;
		m_ChangeScene = true;
	}
		

}

HRESULT CLevelLogo::Render()
{
	return S_OK;
}

void CLevelLogo::UpdateGUI()
{
	ImGui::Begin("LEVEL: CLevel_Logo");
	ImGui::End();
}

void CLevelLogo::FrameStart(E::_float fTimeDelta)
{

}

void CLevelLogo::PlayFadeOutDelete(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, 5.f,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad, 1.f);
}

void CLevelLogo::PlayFadeIn(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();
	_float scaleRatio = pBtn->GetScaleRatio();

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void CLevelLogo::PlayFadeInSacleUp(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();
	_float scaleRatio = pBtn->GetScaleRatio();

	pTween->PlayTween(0.8f, scaleRatio, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->SetScaleRatio(currentValue);
				pObj->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad, delay);

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void CLevelLogo::PlayFadeInChange(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle, this]() {
			Engine::CGameInstance::Get().ChangeLevel(
				CLevelLoading::Create(E::CGameInstance::Get().GetGraphicDevice(), E::CGameInstance::Get().GetGraphicDeviceContext(), LEVEL::CHARLES_ROOKWOOD));
			}, EEaseType::EaseOutQuad, delay);
}

HRESULT CLevelLogo::StopBGM()
{
	auto* pSoundManager = CGameInstance::Get().GetSoundManager();
	if (pSoundManager == nullptr ||
		!pSoundManager->FadeOutAndStop(m_SoundId, 1.f))
		return E_FAIL;

	m_SoundId = INVALID_SOUND_ID;

	return S_OK;
}

Engine::UPtr<CLevelLogo> CLevelLogo::Create()
{
	auto pInstance = Engine::UPtr<CLevelLogo>(new CLevelLogo{});
	pInstance->SetDeferredInitialization();
	return pInstance;
}

void CLevelLogo::Free()
{
	StopBGM();
	CLevel::Free();
}
