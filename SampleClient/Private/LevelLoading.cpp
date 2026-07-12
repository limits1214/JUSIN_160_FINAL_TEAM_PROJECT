#include "pch.h"

#include "LevelLoading.h"
#include "GameInstance.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "LevelLogo.h"
#include "BackGround.h"
#include "LevelPlayground.h"
#include "Terrain.h"
#include "Particle.h"
#include "TestModel.h"
#include "Test_StaticModel.h"
#include "LevelUIEditor.h"
#include "LevelAnimEditor.h"
#include "LevelLightMap.h"
#include "LevelCollider.h"
#include "TestCollider.h"
#include "LevelPhysX.h"
#include "TestPhysX.h"
#include "TestPhysXTerrain.h"
#include "TestPhysXBox.h"
#include "TestPhysXBall.h"
#include "TestPhysXCapsule.h"
#include "LightObject.h"
#include "CTexUI.h"
#include "FlipBook.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Button.h"

#include "TestGob.h"
NS_USING(Client)

CLevelLoading::CLevelLoading(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex) noexcept
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
	, m_eNextLevelIndex(eNextLevelIndex)
{
}

CLevelLoading::~CLevelLoading()
{
}

HRESULT CLevelLoading::Initialize()
{
	Engine::CGameInstance::Get().GameObjectAllReset();


	return S_OK;
}

void CLevelLoading::Update(E::_float fTimeDelta)
{
	if (!m_bThreadStart)
	{
		m_bThreadStart = true;

		ThreadStart();
	}

	LoadingCheck();

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
	case LEVEL::PLAYGROUND:
		pNewLevel = CLevelPlayground::Create();
		break;
	case LEVEL::UIEDITOR:
		pNewLevel = CLevelUIEditor::Create();
		break;
	case LEVEL::ANIMEDITOR:
		pNewLevel = CLevelAnimEditor::Create();
		break;
	case LEVEL::COLLIDER:
		pNewLevel = CLevelCollider::Create();
		break;
	case LEVEL::LIGHTMAP:
		pNewLevel = CLevelLightMap::Create();
		break;
	case LEVEL::PHYSX:
		pNewLevel = CLevelPhysX::Create();
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

void CLevelLoading::ThreadStart()
{
	switch (m_eNextLevelIndex)
	{
	case LEVEL::LOGO:
	{
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LOGO", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_LOGO", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_LOGO", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}
	}
	break;
	case LEVEL::PLAYGROUND:
	{
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_LightObject", CLightObject::Create())))
		{
			return;
		}
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PLAYGROUND", [this]()
			{

				if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						//MSG_BOX("");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Terrain", CTerrain::Create())))
				{
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Gobline", CTestGob::Create())))
				{
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_TestModel", CTestModel::Create())))
				{
				   return false;
				}


				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				//if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_LightObject", CLightObject::Create())))
				//{
				//	int a = 0;
				//	//return false;
				//}



				return  true;
			});
	}
	break;
	case LEVEL::UIEDITOR:
		/* Texture */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_SHM", E::CResTexture2D::Create("./Resources/SampleClient/Textures/SHM.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_MAP", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/T_Map_OverlandPaper_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Back_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Back_4k.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Ready_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Ready_4k.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "TEX_UI_T_NurtureMeterDiamond_Outer_4k", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_NurtureMeterDiamond_Outer_4k.png")))
		{
			res->Load();
		}
		/* Mask */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "MASK_UI_T_ButtonFlameTopClamp", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/TexUI/UI_T_ButtonFlameTopClamp.png")))
		{
			res->Load();
		}

		/* FlipBook */
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_LoadingWidget_Flame", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_LoadingWidget_Flame.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_LoadingWidget_Houses", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_LoadingWidget_Houses.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFXSmokeSim_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFXSmokeSim_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_T_ItemSpark_8x8_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_T_ItemSpark_8x8_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_T_PopVFX_8x8_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_T_PopVFX_8x8_D.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_VFX_BlinkingStars", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/UI_T_VFX_BlinkingStars.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_UI_T_MagicEffect1", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/FlipBook/UI_T_MagicEffect1.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("LEVEL_UIEDITOR", "Flipbook_UI_T_SmokeWispy_D", E::CResTexture2D::Create("./Resources/SampleClient/Textures/UI/FlipBook/UI_T_SmokeWispy_D.png")))
		{
			res->Load();
		}
		
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_UIEDITOR", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_BackGround", CBackGround::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_TextureUI", CTextureUI::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_EffectUI", CEffectUI::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_TextBox", CTextBox::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_UIEDITOR", "Prototype_GameObject_Button", CButton::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});

	
		break;
	case LEVEL::ANIMEDITOR:
	{
		//if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_LightObject", CLightObject::Create())))
		//{
		//	int a = 0;
		//	//return false;
		//}
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_ANIM", [this]()
			{
		
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_TestModel", CTestModel::Create())))
				{
					return false;
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_TEST", "Prototype_GameObject_TestStaticModel", CTest_StaticModel::Create())))
				{
					return false;
				}

				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});
	}
		break;
	case LEVEL::COLLIDER:
	{
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_COLLIDER", [this]()
			{
				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_COLLIDER", "Prototype_GameObject_TestCollider", CTestCollider::Create())))
				{
					return false;
				}
				//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				return  true;
			});
	}
	break;
	case LEVEL::LIGHTMAP:
	{
		//"LIGHT", "Prototype_GameObject_TestModel"
		//"LIGHT", "Prototype_GameObject_Terrain"
		//"SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0" p
		//"LIGHT", "Prototype_GameObject_LightObject"
		//"SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain"
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}
		if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_LightObject", CLightObject::Create())))
		{
			int a = 0;
			//return false;
		}
		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_LIGHTMAP", [this]()
			{
				if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						//MSG_BOX("");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_Terrain", CTerrain::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("LIGHT", "Prototype_GameObject_TestModel", CTestModel::Create())))
				{
					return false;
				}
				
				return  true;
			});
	}
	break;
	case LEVEL::PHYSX:
	{
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_TEX", "TEX2D_Terrain_Tile0", CResTexture2D::Create("./Resources/SampleClient/Textures/Terrain/Tile0.dds")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}



		m_futLoadFinish = E::CGameInstance::Get().WorkerEnqueueWithFuture("LOADING_PHYSX", [this]()
			{
				if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_BUFFER", "VIBUFFER_Terrain", CResTerrainVIBuffer::Create("./Resources/SampleClient/Textures/Terrain/Height.bmp")))
				{
					if (FAILED(res->Load(CResTerrainVIBuffer::DESC{})))
					{
						//MSG_BOX("");
						return false;
					}
				}

				if (FAILED(E::CGameInstance::Get().AddPrototype("LEVEL_PLAYGROUND", "Prototype_GameObject_Terrain", CTerrain::Create())))
				{
					return false;
				}
				if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysX", CTestPhysX::Create())))
				{
					return false;
				}
				//TestPhysXTerrain
				if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXTerrain", CTestPhysXTerrain::Create())))
				{
					return false;
				}

				//TestPhysXBox
				if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBox", CTestPhysXBox::Create())))
				{
					return false;
				}

				//TestPhysXBall
				if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXBall", CTestPhysXBall::Create())))
				{
					return false;
				}

				//TestPhysXCapsule
				if (FAILED(E::CGameInstance::Get().AddPrototype("SAMPLE_CLIENT_PHYSX", "Prototype_GameObject_TestPhysXCapsule", CTestPhysXCapsule::Create())))
				{
					return false;
				}
				return  true;
			});
	}
	break;
	default:
		m_bLoadEnd = true;
		break;
	}

}

void CLevelLoading::LoadingCheck()
{
	if (m_futLoadFinish.valid())
	{
		if (m_futLoadFinish.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			m_bLoadEnd = m_futLoadFinish.get();

			if (!m_bLoadEnd)
			{
				MSG_BOX("LOADING FAILD");
			}
		}
	}
}



Engine::UPtr<CLevelLoading> CLevelLoading::Create(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext, LEVEL eNextLevelIndex)
{
	auto	pInstance = Engine::UPtr<CLevelLoading>(new CLevelLoading(pDevice, pContext, eNextLevelIndex));

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevelLoading");
		return nullptr;
	}

	return pInstance;
}

void CLevelLoading::Free()
{
	CLevel::Free();
}
