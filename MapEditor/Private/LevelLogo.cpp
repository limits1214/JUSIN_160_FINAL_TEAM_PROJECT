#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelLoading.h"

#include "FlyCamera.h"

#include "ResCBuffer.h"
#include "UiCamera.h"
#include "LevelLogoLoader.h"

NS_USING(Client)

CLevelLogo::CLevelLogo()

{
}

CLevelLogo::~CLevelLogo()
{
}

HRESULT CLevelLogo::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	/*{
		CBackGround::UIOBJECT_DESC Desc{};
		Desc.sObjectTag = "BackGround";
		if (!(E::CGameInstance::Get().AddGameObjectToLayer("LEVEL_LOGO", "Prototype_GameObject_BackGround",
			"00_OBJECTS", &Desc)))
		{
			return E_FAIL;
		}
	}*/

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
				MSG_BOX("MSG_BOX_123");
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
				MSG_BOX("MSG_BOX_123_");
			}
			//E::CGameInstance::Get().SetActiveUICamera("UI");
		}
	}

	return S_OK;
}

void CLevelLogo::Update(E::_float fTimeDelta)
{
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

Engine::UPtr<CLevelLogo> CLevelLogo::Create()
{
	auto	pInstance = Engine::UPtr<CLevelLogo>(new CLevelLogo{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
	}

	return pInstance;
}

void CLevelLogo::Free()
{
	CLevelLogoLoader::UnLoad();
	CLevel::Free();
}
