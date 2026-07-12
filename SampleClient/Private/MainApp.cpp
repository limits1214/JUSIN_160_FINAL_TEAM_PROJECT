#include "pch.h"

#include "Engine_Defines.h"

#include "MainApp.h"
#include "GameInstance.h"
#include "LevelLoading.h"
#include "Resources.h"
#include "Particle_Fire_CPU.h"
#include "Particle_Ribbon.h"
#include "BTMove.h"
#include "BTAnimation.h"
#include "Trail_Example.h"
#include "Particle_Fire_GPU.h"
#include "BTHeader_Definse.h"
#include <dxgi1_6.h>

NS_USING(Client)

CMainApp::CMainApp()
{
}

CMainApp::~CMainApp()
{
}

HRESULT CMainApp::Initialize()
{
	Engine::ENGINE_DESC EngineDesc{};
	EngineDesc.hWnd = g_hWnd;
	EngineDesc.hInstance = g_hInstance;
	EngineDesc.eWinMode = Engine::WINMODE::WIN;
	EngineDesc.iWinSizeX = g_iWinSizeX;
	EngineDesc.iWinSizeY = g_iWinSizeY;

	if (FAILED(CBaseApp::Initialize(EngineDesc)))
	{
		return E_FAIL;
	}

	

	if (true)
	{
		ComPtr<IDXGIDevice> dxgiDevice = nullptr;
		CGameInstance::Get().GetGraphicDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
		
		ComPtr<IDXGIAdapter> adapter = nullptr;
		dxgiDevice->GetAdapter(&adapter);

		ComPtr<IDXGIAdapter3> adapter3 = nullptr;
		adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&adapter3);

		DXGI_QUERY_VIDEO_MEMORY_INFO info{};
		adapter3->QueryVideoMemoryInfo(
			0,
			DXGI_MEMORY_SEGMENT_GROUP_LOCAL,
			&info);

		wchar_t buffer[256];
		swprintf_s(buffer, L"VRAM Usage : %.2f MB\n",
			info.CurrentUsage / 1024.0 / 1024.0);

		OutputDebugStringW(buffer);

	}
	LogMemoryUsage("AfterEngineInitialize");

	if (CBaseApp::StartLevel(CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO)))
	{
		return E_FAIL;
	}

	CGameInstance::Get().RegisterLevelChangeFunc("TO_LOGO", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Playground", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::PLAYGROUND));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_UIEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::UIEDITOR));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_ANIMEditor", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::ANIMEDITOR));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_LightMap", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LIGHTMAP));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Collider", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::COLLIDER));
		});

	CGameInstance::Get().RegisterLevelChangeFunc("TO_Physx", [=]() {
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::PHYSX));
		});

	// TODO   SampleClinet  초기 이니셜라이즈
	{
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_NOR_TEX", CResVertexShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
		{
			if (FAILED(res->Load()))
			{
				//MSG_BOX("");
				return E_FAIL;
			}
		}

		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex.hlsl")))
		{
			if (FAILED(res->Load()))
			{
				//MSG_BOX("");
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_NOR_TEX_UI", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex_UI.hlsl")))
		{
			if (FAILED(res->Load()))
			{
				//MSG_BOX("");
				return E_FAIL;
			}
		}
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_NOR_TEX_UI", CResPixelShader::Create("./ShaderFiles/Shader_VtxNorTex_UI.hlsl")))
		{
			if (FAILED(res->Load()))
			{
				//MSG_BOX("");
				return E_FAIL;
			}
		}
	
		Load_Particle_Resources();
	}

	{
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_MATERIAL", CResPhysXMaterial::Create(CResPhysXMaterial::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_BOX", CResPhysXBoxGeometry::Create(CResPhysXBoxGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_SHPERE", CResPhysXSphereGeometry::Create(CResPhysXSphereGeometry::DESC{}));
		CGameInstance::Get().AddResource("SAMPLE_CLIENT_PHYSIX", "TMP_GEO_CAPSULE", CResPhysXCapsuleGeometry::Create(CResPhysXCapsuleGeometry::DESC{}));

	}
	if (FAILED(Create_ActionNode()))
	{
		MSG_BOX("Failed Action Node To MainApp");
		return E_FAIL;
	}
	return S_OK;
}

HRESULT CMainApp::Load_Particle_Resources()
{
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_GPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_GPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_GPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_GPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_Structured_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_CPU_PARTICLE_MESH", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_CPU_PARTICLE_MESH", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Mesh_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_CPU_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_CPU_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_CPU_Tex_Particle.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_RIBBON_PARTICLE_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_RIBBON_PARTICLE_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Ribbon.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "VS_VTX_Trail_TEX", CResVertexShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_SHADER", "PS_VTX_Trail_TEX", CResPixelShader::Create("./ShaderFiles/Shader_Trail.hlsl")))
	{
		if (FAILED(res->Load()))
		{
			//MSG_BOX("");
			return E_FAIL;
		}
	}

	{
		//파티클 텍스쳐 로드
		if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_FLARE", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/VFX_T_RingFlare_D.png")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}
		if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_RIBBON", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/VFX_T_AncientMagicStreak_E.png")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				//return E_FAIL;
			}
		}
		if (auto res = E::CGameInstance::Get().AddResource("SAMPLE_CLINET_TEXTURE", "TEX_TRAIL", E::CResTexture2D::Create("./Resources/SampleClient/Textures/EffectParticle/trail.png")))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("");
				return E_FAIL;
			}
		}
		
		if (auto res = CGameInstance::Get().AddResourceT<E::CResStaticModel>("Rock1", "Static_Model_Resource",
			CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/SM_rock1.fbx"))) {

			E::CResStaticModel::DESC pDesc{};
			pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

			if (FAILED(res->Load(pDesc)))
			{
				return E_FAIL;
			}
		}

	}

	{
		//파티클 버퍼 생성
		if (auto res = CGameInstance::Get().AddResource("SAMPLE_CLIENT_PARTICLEBF", "VIBUF_ParticleQuad", CResQuadTexBuffer::Create()))
		{
			if (FAILED(res->Load()))
			{
				MSG_BOX("파티클 쿼드 버퍼 로드 실패");
				return E_FAIL;
			}
		}
	}

	{
		//파티클 객채들 생성
		CGameInstance::Get().Add_Particle("FIRE", "FIREBALL", CParticle_Fire_CPU::Create());
		CGameInstance::Get().Add_Particle("FIRE", "FIRESMOKE", CParticle_Fire_GPU::Create());
		CGameInstance::Get().Add_Particle("BEAM", "ATTACK", CParticle_Ribbon::Create());
		CGameInstance::Get().Add_Particle("TRAIL", "SLASH", CTrail_Example::Create());

	}
	return S_OK;
}

HRESULT CMainApp::Create_ActionNode()
{
	//프로토타입 이니셜라이즈랑 이름 맞출것
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ACTION,"BTMove", CBTMove::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ACTION, "BTTurnDirect", CBTTurnDirect::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ACTION, "BTTurnSlow", CBTTurnSlow::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ACTION, "BTOnlyFalse", CBTOnlyFalse::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ACTION, "BTOnlyTrue", CBTOnlyTrue::Create())))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::ANIMATION, "BTAnimation", CBTAnimation::Create())))
		return E_FAIL;

	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::DECORATOR, "BTDecSearch", CBTDecSearch::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::DECORATOR, "BTDecTimer", CBTDecTimer::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::DECORATOR, "BTDecLier", CBTDecLier::Create())))
		return E_FAIL;
	if (FAILED(CGameInstance::Get().Add_Action_Prototype(NODEGROUP::DECORATOR, "BTDecInvert", CBTDecInvert::Create())))
		return E_FAIL;

	return S_OK;
}



Engine::UPtr<CMainApp> CMainApp::Create()
{
	auto pInstance = Engine::UPtr<CMainApp>(new CMainApp{});

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CMainApp");
	}

	return pInstance;
}
