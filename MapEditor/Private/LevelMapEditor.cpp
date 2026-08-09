#include "pch.h"
#include "LevelLogo.h"
#include "GameInstance.h"
#include "LevelMapEditor.h"
#include "FlyCamera.h"
#include "ResCBuffer.h"
#include "UiCamera.h"


#include "TestGuizmo.h"
#include "MapMeshObject.h"
#include "Terrain.h"
#include "LevelMapEditorLoader.h"

NS_USING(Client)

CLevelMapEditor::CLevelMapEditor()

{
}

CLevelMapEditor::~CLevelMapEditor()
{
}

HRESULT CLevelMapEditor::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllResetExceptLayers({
		"00_ENGINE_CINEMATIC_CAMERA"
	});

	//{
	//	CMapMeshObject::MAP_MESH_OBJECT_DESC Desc{};
	//	Desc.sObjectTag = "DefaultMapMeshObject";
	//	Desc.modelGroupTag = E::TAG_RES_GRP_MAPEDITOR_STATIC_MODEL;
	//	Desc.modelResTag = E::TAG_RES_MAPEDITOR_DEFAULT_STATIC_MODEL;
	//	if (auto hObject = E::CGameInstance::Get().AddGameObjectToLayer("PERMANENT", "Prototype_GameObject_MapMeshObject",
	//		E::MAPMESHOBJECTLAYER, &Desc))
	//	{
	//		m_SelectedObject = hObject.value();
	//	}
	//	else
	//	{
	//		return E_FAIL;
	//	}
	//}

	// Terrain
	{
		E::CTerrain::DESC Desc{};
		Desc.sObjectTag = "Terrain";
		//Desc.heightMapPath = "./Resources/SampleClient/Textures/Terrain/Height.bmp";
		Desc.textureGroup = "MAPEDITOR_TERRAIN_TILE";
		Desc.textureTag = "Tile0";
		Desc.chunkQuadCount = 150;
		Desc.vertexSpacing = 1.f;
		Desc.heightScale = 0.1f;

		if (!E::CGameInstance::Get().AddGameObjectToLayer(
			"MAPEDITOR",
			"Prototype_GameObject_Terrain",
			"01_Terrain",
			&Desc))
		{
			return E_FAIL;
		}
	}


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

	m_pMapEditorGUI = CMapEditorGUI::Create(&m_SelectedObject);
	if (m_pMapEditorGUI == nullptr)
	{
		return E_FAIL;
	}

	if (E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Light", CLight::Create()))	return E_FAIL;		// 월드에 전역조명 추가
	CGameInstance::Get().Add_DirectionalLight({ 1.f, -1.f, 1.f }, { 1.f, 1.f, 1.f }, 10.f);
	//CGameInstance::Get().Add_PointLight({ 1.f, -1.f, 1.f }, { 1.f, 0.f, 0.f }, 30.f, 10.f);

	return S_OK;
}

void CLevelMapEditor::Update(E::_float fTimeDelta)
{
}

HRESULT CLevelMapEditor::Render()
{
	return S_OK;
}

void CLevelMapEditor::UpdateGUI()
{
	m_pMapEditorGUI->UpdateGUI(0.f);
}

void CLevelMapEditor::FrameStart(E::_float fTimeDelta)
{

}

Engine::UPtr<CLevelMapEditor> CLevelMapEditor::Create()
{
	auto	pInstance = Engine::UPtr<CLevelMapEditor>(new CLevelMapEditor{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_MapEditor");
	}

	return pInstance;
}

void CLevelMapEditor::Free()
{
	CLevelMapEditorLoader::UnLoad();
	CLevel::Free();
}
