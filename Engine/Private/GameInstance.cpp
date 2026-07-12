#include "pch.h"
#include "GameInstance.h"

#include "TimeProvider.h"
#include "ImGuiManager.h"
#include "DInputManager.h"
#include "GraphicDevice.h"
#include "LevelManager.h"
#include "Level.h"
#include "SoundManager.h"
#include "FontManager.h"
#include "PrototypeManager.h"
#include "Prototype.h"
#include "ColliderManager.h"
#include "Collider.h"
#include "Renderer.h"
#include "ComConstantBuffer.h"
#include "FlyCamera.h"
#include "UICamera.h"
#include "ComBeHavior.h"
#include "AnimEdit_Manager.h"
#include "ComModelInstance.h"
#include "ComStaticModelInstance.h"
#include "ComAnimator.h"
#include "NodeEditor.h"
#include "Action_Manager.h"
#include "Light.h"
#include "ComCollider.h"
#include "MapMeshObject.h"
#include "MapManager.h"
#include "NavMeshManager.h"
#include "PhysXManager.h"
#include "DbgLineRender.h"
#include "SerializeManager.h"

#include "ComPxBoxCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCollider.h"
#include "ComPxRigidBody.h"
#include "ComPxTriMeshCollider.h"

#include "ComLuaScript.h"

#include "ParticleManager.h"
#include "Particle.h"

#include "ButtonComponent.h"
#include "TweenComponent.h"
NS_USING(Engine)

CGameInstance::CGameInstance()
{
}

CGameInstance::~CGameInstance()
{
}

HRESULT CGameInstance::InitializeEngine(const ENGINE_DESC& EngineDesc, ComPtr<ID3D11Device>& ppDevice, ComPtr<ID3D11DeviceContext>& ppContext)
{


	m_hWnd = EngineDesc.hWnd;
	m_vClientScreenSize.x = (float)EngineDesc.iWinSizeX;
	m_vClientScreenSize.y = (float)EngineDesc.iWinSizeY;


	m_pGraphicDevice = CGraphicDevice::Create(ppDevice, ppContext);
	if (m_pGraphicDevice == nullptr)
	{
		return E_FAIL;
	}

	m_pResourceManager = CResourceManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pResourceManager == nullptr)
	{
		return E_FAIL;
	}

	m_pSoundManager = CSoundManager::Create();
	if (m_pSoundManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(m_pGraphicDevice->ReadyDevice(EngineDesc.hWnd, EngineDesc.eWinMode, EngineDesc.iWinSizeX, EngineDesc.iWinSizeY)))
	{
		return E_FAIL;
	}

	m_pDbgLineRender = CDbgLineRender::Create(ppDevice.Get(), ppContext.Get());
	if (m_pDbgLineRender == nullptr)
	{
		return E_FAIL;
	}

	m_pLuaManager = CLuaManager::Create();
	if (m_pLuaManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(InitializeResources()))
	{
		return E_FAIL;
	}

	m_pPrototypeManager = CPrototypeManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pPrototypeManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(InitializePrototype()))
	{
		return E_FAIL;
	}

	m_pImguiManager = CImguiManager::Create(EngineDesc.hWnd, ppDevice.Get(), ppContext.Get());
	if (m_pImguiManager == nullptr)
	{
		return E_FAIL;
	}

	m_pLevelManager = CLevelManager::Create();
	if (m_pLevelManager == nullptr)
	{
		return E_FAIL;
	}

	m_pDInputManager = CDInputManager::Create(EngineDesc.hInstance, EngineDesc.hWnd);
	if (m_pDInputManager == nullptr)
	{
		return E_FAIL;
	}

	m_pTimeProvider = CTimeProvider::Create();
	if (m_pTimeProvider == nullptr)
	{
		return E_FAIL;
	}

	m_pWorkerManager = CWorkerManager::Create("Normal", 3);
	if (m_pWorkerManager == nullptr)
	{
		return E_FAIL;
	}

	m_pGameObjectManager = CGameObjectManager::Create();
	if (m_pGameObjectManager == nullptr)
	{
		return E_FAIL;
	}

	m_pRenderer = CRenderer::Create(ppDevice.Get(), ppContext.Get());
	if (m_pRenderer == nullptr)
	{
		return E_FAIL;
	}

	m_pCameraManager = CCameraManager::Create();
	if (m_pCameraManager == nullptr)
	{
		return E_FAIL;
	}

	m_pColliderManager = CColliderManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pColliderManager == nullptr)
	{
		return E_FAIL;
	}

	m_pAnimEdit_Manager = CAnimEdit_Manager::Create();
	if (m_pAnimEdit_Manager == nullptr)
	{
		return E_FAIL;
	}

	m_pLightManager = CLightManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pLightManager == nullptr)
	{
		return E_FAIL;
	}


	//m_pParticleManager = CParticleManager::Create(ppDevice.Get(), ppContext.Get());
	//if (m_pParticleManager == nullptr)
	//{
	//	return E_FAIL;
	//}
	m_pParticleManager = CParticleManager::Create();
	if (m_pParticleManager == nullptr)
	{
		return E_FAIL;
	}

	m_pFontManager = CFontManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pFontManager == nullptr)
	{
		return E_FAIL;
	}

	m_pMapManager = CMapManager::Create();
	if (m_pMapManager == nullptr)
	{
		return E_FAIL;
	}
	m_pNavMeshManager = CNavMeshManager::Create();
	if (m_pNavMeshManager == nullptr)
	{
		return E_FAIL;
	}
	m_pNodeEditor = CNodeEditor::Create();
	if (m_pNodeEditor == nullptr)
	{
		return E_FAIL;
	}
	m_pPhysXManager = CPhysXManager::Create();
	if (m_pPhysXManager == nullptr)
	{
		return E_FAIL;
	}

	m_pActionManager = CAction_Manager::Create();
	if (m_pActionManager == nullptr)
		return E_FAIL;



	m_pSerializeManager = CSerializeManager::Create();
	if (m_pSerializeManager == nullptr)
	{
		return E_FAIL;
	}


	return S_OK;
}

void CGameInstance::FixedUpdateEngine(_float fFixedTimeDelta)
{
	m_pGameObjectManager->FixedUpdate(fFixedTimeDelta);
	m_pPhysXManager->StepSimulation(fFixedTimeDelta);
}

void CGameInstance::UpdateGUI()
{
	ZoneScopedN("UpdateGUI");

	{
		ZoneScopedN("PrototypeManager_UpdateGUI");
		m_pPrototypeManager->UpdateGUI();
	}

	{
		ZoneScopedN("GameObjectManager_UpdateGUI");
		m_pGameObjectManager->UpdateGUI();
	}

	{
		ZoneScopedN("m_pAnimEdit_Manager_UpdateGUI");

		m_pAnimEdit_Manager->UpdateGUI();
	}


	m_pWorkerManager->UpdateGUI();

	m_pResourceManager->UpdateGUI();


	m_pCameraManager->UpdateGUI();

	m_pLevelManager->UpdateGUI();

	m_pColliderManager->UpdateGUI();

	m_pParticleManager->UpdateGUI();

	m_pLightManager->UpdateGUI();


	m_pRenderer->UpdateGUI();

	// 사운드 붙일때 부활
	// m_pSoundManager->UpdateGUI();

	m_pNodeEditor->NodeEditorUpdate();
	m_pPhysXManager->UpdateGUI();

	m_pSerializeManager->UpdateGUI();

	m_pLuaManager->UpdateGUI();
	if (ImGui::Button("ShaderRebuild"))
	{
		//TAG_RES_GRP_PERMANENT_SHADER
		if (auto resources = GetResource(TAG_RES_GRP_PERMANENT_SHADER))
		{
			for (auto& [_, res] : *resources)
			{
				if (!res.empty())
				{
					res.front()->Unload();
					res.front()->Load();
				}
			}
		}
	}
}

void CGameInstance::UpdateEngine(_float fTimeDelta)
{
	// TODO: 마우스 가두기 함수화하기
	{
		if (CGameInstance::Get().KeyDown(DIK_TAB))
		{

			m_bMouseFix = !m_bMouseFix;
			if (!m_bMouseFix)
			{
				ShowCursor(TRUE);
			}
			else
			{
				ShowCursor(FALSE);
			}
		}
		if (m_bMouseFix)
		{
			MouseFix();
		}
	}

	// 사운드 붙일때 부활
	if constexpr (false)
	{
		ZoneScopedN("SoundManager_Update");
		m_pSoundManager->Update();
	}

	// lua hot reload
	{
		m_pLuaManager->Update(fTimeDelta);
	}



	//m_pParticleManager->Update(fTimeDelta);
	m_pAnimEdit_Manager->Update(fTimeDelta);
	m_pParticleManager->Update(fTimeDelta);

	{
		m_pPhysXManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("GameObjectManager_Update");
		m_pGameObjectManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("GameObjectManager_LateUpdate");
		m_pGameObjectManager->LateUpdate(fTimeDelta);
	}

	{
		ZoneScopedN("LevelManager_Update");
		m_pLevelManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("LightManager_Update");
		m_pLightManager->Update(fTimeDelta);
	}

	m_pColliderManager->Update();
	//m_pGameObjectManager->PriorityUpdate(fTimeDelta);
	//m_pGameObjectManager->Update(fTimeDelta);
	//m_pGameObjectManager->LateUpdate(fTimeDelta);

	//m_pLevelManager->Update(fTimeDelta);


	m_pMapManager->Update(fTimeDelta);

	m_pDbgLineRender->AddAxis(1.f, XMMatrixTranslation(1.3f, 1.2f, 0.f));
	m_pNavMeshManager->DrawDebug();

	AddRenderObject(RENDERGROUP::BLEND, m_pParticleManager.get());
	AddRenderObject(RENDERGROUP::COLLIDER, m_pDbgLineRender.get());
	AddRenderObject(RENDERGROUP::COLLIDER, m_pColliderManager.get());
}


HRESULT CGameInstance::Draw()
{
	if (FAILED(m_pRenderer->Draw()))
	{
		return E_FAIL;
	}

#ifdef _DEBUG
	m_pMapManager->RenderDebugMapChunk();
#endif
	return S_OK;
}

//void CGameInstance::ClearResource(uint32_t iClearLevelIndex)
//{
//}

void CGameInstance::Release_Engine()
{
	CMapMeshObject::ReleaseInstancingResources(); // CMapMeshObject의 static 인스턴스 버퍼 해제
	m_pSoundManager.reset();
	m_pImguiManager.reset();
	m_pDInputManager.reset();
	m_pNodeEditor.reset();
	m_pActionManager.reset();
	m_pAnimEdit_Manager.reset();
	m_pGameObjectManager->AllReset();
	m_pLevelManager.reset();
	m_pColliderManager.reset();
	m_pParticleManager.reset();
	m_pWorkerManager.reset();
	m_pLightManager.reset();
	m_pCameraManager.reset();
	m_pPrototypeManager.reset();
	m_pGameObjectManager.reset();
	m_pLuaManager.reset();
	m_pRenderer.reset();
	m_pFontManager.reset();
	m_pSerializeManager.reset();
	m_pDbgLineRender.reset();
	m_pResourceManager.reset();
	m_pPhysXManager.reset();
	m_pNavMeshManager.reset();
	m_pMapManager.reset();
	m_pGraphicDevice.reset();
}


void CGameInstance::FrameStart(_float fTimeDelta)
{
	{
		ZoneScopedN("InputManager_Update");
		m_pDInputManager->Update_InputDev();
	}

	m_pLevelManager->FrameStart(fTimeDelta);
	m_pGameObjectManager->FrameStart();
	m_pColliderManager->FrameStart();



	{
		ZoneScopedN("GameObjectManager_PriorityUpdate");
		m_pGameObjectManager->PriorityUpdate(fTimeDelta);
	}
}
void CGameInstance::FrameEnd(_float fTimeDelta)
{
	m_pGameObjectManager->FrameEnd();
	m_pLevelManager->FrameEnd(fTimeDelta);

	m_pRenderer->FrameEnd();
	CMapMeshObject::ClearInstancingData();
	m_pColliderManager->FrameEnd();
	m_pDbgLineRender->FrameEnd();
}


#pragma region PARTICLE_MANAGER
HRESULT CGameInstance::Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
	uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
	_bool bLoop, _float fSpawnInterval)
{
	return m_pParticleManager->Spawn(sGroupTag, sTypeTag, count, pSpawnData, bLoop, fSpawnInterval);
}

HRESULT CGameInstance::Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle)
{
	return m_pParticleManager->Add_Particle(sGroupTag, sTypeTag, std::move(particle));
}
HRESULT CGameInstance::SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end,
	_float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping,
	_float fFlickerInterval, _float4 vColor, _float4 emissive, _float fDuration)
{
	return m_pParticleManager->SpawnRibbon(quantity, start, end, fDisplacementAmplitude, iDisplacementIterations, fDisplacementDamping, fFlickerInterval, vColor, emissive, fDuration);
}
#pragma endregion

#pragma region LUA_MANAGER
HRESULT CGameInstance::LuaScriptExecute(const std::string& script, const sol::environment& env, const std::string& chunkName)
{
	return m_pLuaManager->Execute(script, env, chunkName);
}
HRESULT CGameInstance::LuaScriptExecute(const std::string& script, const std::string& chunkName)
{
	return m_pLuaManager->Execute(script, chunkName);
}
sol::environment CGameInstance::LuaCreateEnvironment()
{
	return m_pLuaManager->CreateEnvironment();
}
sol::protected_function CGameInstance::LuaCacheFunction(const std::string& funcName)
{
	return m_pLuaManager->CacheFunction(funcName);
}
sol::protected_function CGameInstance::LuaCacheFunction(const sol::environment& env, const std::string& funcName)
{
	return m_pLuaManager->CacheFunction(env, funcName);
}

HRESULT CGameInstance::LuaCompile(const std::string& script)
{
	return m_pLuaManager->Compile(script);
}
bool CGameInstance::LuaIsEnvValid(const sol::environment& env) const
{
	return m_pLuaManager->IsEnvValid(env);
}
bool CGameInstance::LuaHasValue(const sol::environment& env, std::string_view name) const
{
	return m_pLuaManager->HasValue(env, name);
}
void CGameInstance::LuaRemoveValue(sol::environment& env, std::string_view name)
{
	m_pLuaManager->RemoveValue(env, name);
}
void CGameInstance::LuaEnvDump(const sol::environment& env) const
{
	m_pLuaManager->EnvDump(env);
}
void CGameInstance::LuaEnvClear(sol::environment& env)
{
	m_pLuaManager->EnvClear(env);
}
void CGameInstance::LuaRegisterComponent(const std::string& path, ILuaScriptRelodable* pComp)
{
	m_pLuaManager->RegisterComponent(path, pComp);
}
void CGameInstance::LuaUnregisterComponent(const std::string& path, ILuaScriptRelodable* pComp)
{
	m_pLuaManager->UnregisterComponent(path, pComp);
}
void CGameInstance::LuaRegisterExtension(std::function<void(sol::state&)> extensionFunc)
{
	m_pLuaManager->RegisterExtension(extensionFunc);
}
#pragma endregion
void CGameInstance::MouseFix() const
{
	RECT rect;
	GetClientRect(CGameInstance::Get().GetHwnd(), &rect);
	POINT center;
	center.x = (rect.right - rect.left) / 2;
	center.y = (rect.bottom - rect.top) / 2;

	ClientToScreen(CGameInstance::Get().GetHwnd(), &center);
	SetCursorPos(center.x, center.y);
}

HRESULT CGameInstance::InitializeResources()
{
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PASS, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_PASS) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_OBJECT, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_OBJECT) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_PARTICLE) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_SPAWN_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PARTICLE_SPAWN) })))
		{
			return E_FAIL;
		}
	}


	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_PER_UI) })))
		{
			return E_FAIL;
		}
	}
	if (auto res = CGameInstance::Get().AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, "CB_MATERIAL", E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_MATERIAL) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_INIT_PARTICLE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(CB_INIT_PARTICLE) })))
		{
			return E_FAIL;
		}
	}

	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_BUFFER, TAG_RES_CBUFFER_BONE, E::CResCBuffer::Create()))
	{
		if (FAILED(res->Load(E::CResCBuffer::CBUFFER_DESC{ .byteWidth = sizeof(_float4x4) * 512 })))
		{
			return E_FAIL;
		}
	}

	// LinearWrap
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(0, 1, res->GetSamplerState().GetAddressOf());
	}
	// LinearClamp
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_CLAMP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
				return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(1, 1, res->GetSamplerState().GetAddressOf());
	}
	// PointWrap
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;

		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

		samplerDesc.MinLOD = 0.f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		if (FAILED(res->Load(samplerDesc))) {
			return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(2, 1, res->GetSamplerState().GetAddressOf());
	}
	// PointClamp
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_CLAMP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
			.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(3, 1, res->GetSamplerState().GetAddressOf());
	}
	// PointWrapNoMip
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_POINT_WRAP_NOMIP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = 0.0f,
			}))) {
			return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(4, 1, res->GetSamplerState().GetAddressOf());
	}
	// AnisotropicWrap
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_ANISOTROPIC_WRAP, CResSamplerState::Create()))
	{
		if (FAILED(res->Load(D3D11_SAMPLER_DESC{
			.Filter = D3D11_FILTER_ANISOTROPIC,
			.AddressU = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressV = D3D11_TEXTURE_ADDRESS_WRAP,
			.AddressW = D3D11_TEXTURE_ADDRESS_WRAP,
			.MipLODBias = 0.f,
			.MaxAnisotropy = 16,
			.ComparisonFunc = D3D11_COMPARISON_NEVER,
			.MinLOD = 0,
			.MaxLOD = D3D11_FLOAT32_MAX,
			}))) {
			return E_FAIL;
		}
		GetGraphicDeviceContext()->PSSetSamplers(5, 1, res->GetSamplerState().GetAddressOf());
	}
	// ShadowSampler
	if (auto res = AddResourceT(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_SAHDOW, CResSamplerState::Create()))
	{
		D3D11_SAMPLER_DESC sampDesc{};
		sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_POINT;
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;

		sampDesc.BorderColor[0] = 1.f;
		sampDesc.BorderColor[1] = 1.f;
		sampDesc.BorderColor[2] = 1.f;
		sampDesc.BorderColor[3] = 1.f;

		sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		if (FAILED(res->Load(sampDesc)))
		{
			return E_FAIL;
		}

		GetGraphicDeviceContext()->PSSetSamplers(6, 1, res->GetSamplerState().GetAddressOf());
	}
	
	

	//ShaderFiles
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTex", "./ShaderFiles/QuadTex/QuadTex.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI", "./ShaderFiles/UI/QuadTexUI.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI", "./ShaderFiles/UI/QuadTexUI.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexFlipBook", "./ShaderFiles/UI/QuadTexFlipBook.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexFlipBook", "./ShaderFiles/UI/QuadTexFlipBook.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadCol", "./ShaderFiles/QuadCol/QuadCol.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_UpdateParticle", "./ShaderFiles/Particle/Shader_Particle_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_SpawnParticle", "./ShaderFiles/Particle/Shader_Particle_Spawn_Compute.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResComputeShader>(TAG_RES_GRP_PERMANENT_SHADER, "CS_InitParticle", "./ShaderFiles/Particle/Shader_CS_Init.hlsl"))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex", E::CResQuadTexBuffer::Create()))
	{
		if (FAILED(res->Load()))
		{
			return E_FAIL;
		}
	}


	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_BACKCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_FRONTCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_SOLID_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_RS_WIREFRAME_NOCULL, E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_WIREFRAME;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;

		res->Load(desc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BACKCULL_DEPTHBIAS", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_BACK;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = -100;
		desc.SlopeScaledDepthBias = -1.0f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_SOLID_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_FRONT;
		desc.DepthClipEnable = TRUE;
		res->Load(desc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "RS_ALPHATEST_BLOCK_VOXEL_SHADOW", E::CResRasterizerState::Create()))
	{
		D3D11_RASTERIZER_DESC desc{};
		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = D3D11_CULL_NONE;
		desc.DepthClipEnable = TRUE;
		desc.DepthBias = 10;
		desc.SlopeScaledDepthBias = 0.5f;
		desc.DepthBiasClamp = 0.0f;
		res->Load(desc);
	}


	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		res->Load(blendDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_BLEND_NONE", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = FALSE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		res->Load(blendDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		res->Load(blendDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "BS_ALPHA_BLEND_ADD", E::CResBlendState::Create()))
	{
		D3D11_BLEND_DESC blendDesc{};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		res->Load(blendDesc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHWRITE", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		res->Load(depthDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHWRITE", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthDesc.StencilEnable = FALSE;
		depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		res->Load(depthDesc);
	}
	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_DEPTHREAD", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = TRUE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		depthDesc.DepthFunc = D3D11_COMPARISON_LESS;
		depthDesc.StencilEnable = FALSE;
		depthDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
		depthDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
		res->Load(depthDesc);
	}

	if (auto res = AddResource(TAG_RES_GRP_PERMANENT_STATE, "DS_NO_DEPTHSTENCIL", E::CResDepthStencilState::Create()))
	{
		D3D11_DEPTH_STENCIL_DESC depthDesc{};
		depthDesc.DepthEnable = FALSE;
		depthDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
		depthDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		depthDesc.StencilEnable = FALSE;
		res->Load(depthDesc);
	}

	// Test Model Load
	// 오류나서 제거
	if (true)
	{
		//if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		//{
		//	if (FAILED(res->Load()))
		//	{
		//		return E_FAIL;
		//	}
		//}
		if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim", "./ShaderFiles/TestModel/Shader_VtxMesh_NonInstanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}

		if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelNonAnim_Instanced", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelNonAnim_Instanced", "./ShaderFiles/TestModel/Shader_VtxMesh_Instanced.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}


		if (auto res = AddResourceT<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
		if (auto res = AddResourceT<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_TestModelAnim", "./ShaderFiles/TestModel/Shader_VtxAnimMesh.hlsl"))
		{
			if (FAILED(res->Load()))
			{
				return E_FAIL;
			}
		}
	}


	// 텍스쳐 없는 경우 대비, 대체 텍스쳐
	{
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_DIFFUSE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Diffuse.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_NORMAL", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Normal.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_SMRO", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_SMRO.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_EMISSIVE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_Emissive.png")))
		{
			res->Load();
		}
		if (auto res = E::CGameInstance::Get().AddResource("DEFAULT_TEXTURE", "TEX_DEFAULT_WHITE", E::CResTexture2D::Create("./Resources/Engine/Texture/DefaultTexture/DefaultTex_White.png")))
		{
			res->Load();
		}
	}




	if (auto res = AddResourceT<E::CResModel>("TEST", "Model_Resource",
		CResModel::Create("./Resources/SampleClient/Models/Skeleton/Tomb_Protector/SK_Tomb_Protector.bin"))) {

		E::CResModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f) *XMMatrixRotationY(XMConvertToRadians(180.f));

		if (FAILED(res->Load(pDesc)))
		{
			return E_FAIL;
		}
	}

	/*if (auto res = AddResourceT<E::CResStaticModel>("TEST", "Static_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/OriginData/Static/HorseStatue.fbx"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

		if (FAILED(res->Load(pDesc)))
		{
			return E_FAIL;
		}
	}*/
	if (auto res = AddResourceT<E::CResStaticModel>("TEST", "Static_Model_Resource",
		CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_HorseStatue.bin"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

		if (FAILED(res->Load(pDesc)))
		{
			return E_FAIL;
		}
	}
	if (auto res = AddResourceT<E::CResStaticModel>(TAG_RES_GRP_MAPEDITOR_STATIC_MODEL, TAG_RES_MAPEDITOR_DEFAULT_STATIC_MODEL,
		CResStaticModel::Create("./Resources/SampleClient/Models/Static/SM_HorseStatue.bin"))) {

		E::CResStaticModel::DESC pDesc{};
		pDesc.PreTransformMatrix = XMMatrixScaling(1.f, 1.f, 1.f);

		if (FAILED(res->Load(pDesc)))
		{
			return E_FAIL;
		}
	}



	// 루아 리소스
	{
		
		auto res = AddResource(ES_EngineResMajorType::PERMANENT_LUA, ES_EngineResLuaScript::LUA_TEST, CResLuaScript::CreateAndLoad("./LuaFiles/SomeFolder/Hi.lua"));
	
	}

	return S_OK;
}
HRESULT CGameInstance::InitializePrototype()
{
	
	if (AddPrototype(ES_EngineProtoMajorType::PERMANENT, ES_EngineProtoComponent::Prototype_Component_Transform, CComTransform::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("PERMANENT", "Prototype_Component_ConstantBuffer", CComConstantBuffer::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("PERMANENT", "Prototype_Component_ModelInstance", CComModelInstance::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("PERMANENT", "Prototype_Component_StaticModelInstance", CComStaticModelInstance::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("PERMANENT", "Prototype_Component_Animator", CComAnimator::Create()))
	{
		return E_FAIL;
	}


	if (AddPrototype(ES_EngineProtoMajorType::CAMERAS, ES_EngineProtoGameObject::Prototype_GameObject_FlyCamera, CFlyCamera::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("CAMERAS", "Prototype_GameObject_UICamera", CUICamera::Create()))
	{
		return E_FAIL;
	}
	if (AddPrototype("BEHAVIOR", "Prototype_Component_BeHavior", CComBeHavior::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("COLLIDER", "Prototype_Component_Collider", CComCollider::Create()))
	{
		return E_FAIL;
	}

	if (AddPrototype("PERMANENT", "Prototype_GameObject_MapMeshObject", CMapMeshObject::Create()))
	{
		return E_FAIL;
	}

	

	// 피직스관련
	{
		
		if (AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxBoxCollider, CComPxBoxCollider::Create()))
		{
			return E_FAIL;
		}
		if (AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxCapsuleCollider, CComPxCapsuleCollider::Create()))
		{
			return E_FAIL;
		}
		if (AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxSphereCollider, CComPxSphereCollider::Create()))
		{
			return E_FAIL; 
		}
		if (AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxTriMeshCollider, CComPxTriMeshCollider::Create()))
		{
			return E_FAIL;
		}
		if (AddPrototype(ES_EngineProtoMajorType::PHYSX, ES_EngineProtoPhysXComponent::Prototype_Component_ComPxRigidBody, CComPxRigidBody::Create()))
		{
			return E_FAIL;
		}
	}

	// UI관련
	{
		if (AddPrototype(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", CButtonComponent::Create()))
		{
			return E_FAIL;
		}

		if (AddPrototype(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", TweenComponent::Create()))
		{
			return E_FAIL;
		}
	}

	// 루아
	{
		if (AddPrototype(ES_EngineProtoMajorType::LUA, ES_EngineProtoComponent::Prototype_Component_ComLuaScript, CComLuaScript::Create()))
		{
			return E_FAIL;
		}
	}

	//if (AddPrototype("CAMERAS", "Prototype_GameObject_PlayerCamera", CPlayerCamera::Create()))
	//{
	//	return E_FAIL;
	//}
	return S_OK;
}

#pragma region TIME_PROVIDER
_float CGameInstance::UpdateTimeProvider()
{
	return m_pTimeProvider->UpdateTimeProvider();
}
#pragma endregion

#pragma region IMGUI_MANAGER
void CGameInstance::ImguiNewFrame()
{
	m_pImguiManager->Update_Imgui();
}

void CGameInstance::ImguiEndFrameAndRender()
{
	m_pImguiManager->Render_Imgui();
}

_bool CGameInstance::ImguiWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	return m_pImguiManager->WinProc(hWnd, message, wParam, lParam);
}

_bool CGameInstance::ImguiGetActive() const
{
	return m_pImguiManager->Get_Active();
}

void CGameInstance::ImguiSetActive(_bool bActive)
{
	m_pImguiManager->Set_Active(bActive);
}

void CGameInstance::ImguiEnableDocking(_bool bEnableDocking, _bool bEnableViewports)
{
	m_pImguiManager->EnableDocking(bEnableDocking, bEnableViewports);
}
#pragma endregion


#pragma region RESOURCE_MANAGER
SPtr<CResource> CGameInstance::AddResource(const StringID& sGroupTag, const StringID& sResTag, _string_id eAssetType, const _string& sPath, void* pArg)
{
	return m_pResourceManager->AddResource(sGroupTag, sResTag, eAssetType, sPath, pArg);
}
SPtr<CResource> CGameInstance::AddResource(const StringID& sGroupTag, const StringID& sResTag, SPtr<CResource> pAsset)
{
	return m_pResourceManager->AddResource(sGroupTag, sResTag, pAsset);
}
const std::vector<SPtr<CResource>>* CGameInstance::GetResource(const StringID& sGroupTag, const StringID& sResTag) const
{
	return m_pResourceManager->GetResource(sGroupTag, sResTag);
}
const std::unordered_map<StringID, std::vector<SPtr<CResource>>>* CGameInstance::GetResource(const StringID& sGroupTag) const
{
	return m_pResourceManager->GetResource(sGroupTag);
}
const std::unordered_map<StringID, std::unordered_map<StringID, std::vector<SPtr<CResource>>>>& CGameInstance::GetResources() const
{
	return m_pResourceManager->GetResources();
}
HRESULT CGameInstance::LoadResource(const StringID& sGroupTag)
{
	return m_pResourceManager->LoadResource(sGroupTag);
}
HRESULT CGameInstance::LoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	return m_pResourceManager->LoadResource(sGroupTag, sResTag);
}
HRESULT CGameInstance::UnLoadResource(const StringID& sGroupTag)
{
	return m_pResourceManager->UnLoadResource(sGroupTag);
}
HRESULT CGameInstance::UnLoadResource(const StringID& sGroupTag, const StringID& sResTag)
{
	return m_pResourceManager->UnLoadResource(sGroupTag, sResTag);
}
void CGameInstance::DelResource(const StringID& sGroupTag)
{
	m_pResourceManager->DelResource(sGroupTag);
}
void CGameInstance::DelResource(const StringID& sGroupTag, const StringID& sResTag)
{
	m_pResourceManager->DelResource(sGroupTag, sResTag);
}
const std::vector<CResource*>* CGameInstance::GetResourcesByPath(const _string& sPath) const
{
	if (!m_pResourceManager) return nullptr;
	return m_pResourceManager->GetResourcesByPath(sPath);
}
void CGameInstance::RemoveResourcePathLookup(const _string& sPath, CResource* pRes)
{
	if (!m_pResourceManager) return;
	m_pResourceManager->RemovePathLookup(sPath, pRes);
}
#pragma endregion


#pragma region GRAPHIC_DEVICE
ComPtr<ID3D11Device> CGameInstance::GetGraphicDevice() const
{
	return m_pGraphicDevice->GetDevice();
}
ComPtr<ID3D11DeviceContext> CGameInstance::GetGraphicDeviceContext() const
{
	return  m_pGraphicDevice->GetContext();
}
ComPtr<ID3D11RenderTargetView> CGameInstance::GetBackBufferRTV() const
{
	return m_pGraphicDevice->GetBackBufferRTV();
}
ComPtr<ID3D11DepthStencilView> CGameInstance::GetBackBufferDSV() const
{
	return m_pGraphicDevice->GetBackBufferDSV();
}

ComPtr<ID3D11Texture2D> CGameInstance::GetBackBufferTexture() const
{
	return m_pGraphicDevice->GetBackBufferTexture();
}
HRESULT CGameInstance::ClearBackBufferView(const _float4* pClearColor)
{
	return m_pGraphicDevice->ClearBackBufferView(pClearColor);
}

HRESULT CGameInstance::ClearDepthStencilView()
{
	return m_pGraphicDevice->ClearDepthStencilView();
}

HRESULT CGameInstance::Present()
{
	ZoneScopedN("Present");
	return m_pGraphicDevice->Present();
}
#pragma endregion


#pragma region DINPUT_MANAGER
_bool CGameInstance::KeyPressing(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyPressing(byKeyID);
}
_bool CGameInstance::KeyUp(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyUp(byKeyID);
}
_bool CGameInstance::KeyDown(_ubyte byKeyID) const
{
	return m_pDInputManager->KeyDown(byKeyID);
}
int32_t CGameInstance::MouseMove(MOUSEMOVESTATE eMouseState) const
{
	return m_pDInputManager->MouseMove(eMouseState);
}
_bool CGameInstance::MousePressing(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MousePressing(eMouseState);
}
_bool CGameInstance::MouseUp(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MouseUp(eMouseState);
}
_bool CGameInstance::MouseDown(MOUSEKEYSTATE eMouseState) const
{
	return  m_pDInputManager->MouseDown(eMouseState);
}

#pragma endregion

#pragma region LEVEL_MANAGER
HRESULT CGameInstance::ChangeLevel(UPtr<CLevel> pNewLevel)
{
	return m_pLevelManager->ChangeLevel(std::move(pNewLevel));
}
HRESULT CGameInstance::ChangeLevel(const _string& ID)
{
	return m_pLevelManager->ChangeLevel(ID);
}
void CGameInstance::RegisterLevelChangeFunc(const _string& ID, _Func func)
{
	m_pLevelManager->RegisterLevelChangeFunc(ID, func);
}
#pragma endregion


#pragma region SOUND_MANAGER
HRESULT CGameInstance::CreateSound(const _string& sPath, FMOD_SOUND** ppSound)
{
	return m_pSoundManager->CreateSound(sPath, ppSound);
}

HRESULT CGameInstance::SoundAddChannel(const StringID& channelTag, const std::pair<StringID, StringID>& soundResources)
{
	return m_pSoundManager->AddChannel(channelTag, soundResources);
}

HRESULT CGameInstance::SoundPlay(const StringID& channelTag, _float fVolume, _float fPitch)
{
	return m_pSoundManager->Play(channelTag, fVolume, fPitch);
}

void CGameInstance::SoundStop(const StringID& channelTag)
{
	m_pSoundManager->Stop(channelTag);
}

void CGameInstance::SoundPause(const StringID& channelTag, _bool bPause)
{
	m_pSoundManager->Pause(channelTag, bPause);
}

_bool CGameInstance::SoundGetVolume(const StringID& channelTag, _float& fVolume)
{
	return m_pSoundManager->GetVolume(channelTag, fVolume);
}

_bool CGameInstance::SoundSetVolume(const StringID& channelTag, _float fVolume)
{
	return m_pSoundManager->SetVolume(channelTag, fVolume);
}

_bool CGameInstance::SoundIsPlaying(const StringID& channelTag) const
{
	return m_pSoundManager->IsPlaying(channelTag);
}

void CGameInstance::SoundSetPitch(const StringID& channelTag, float fPitchRatio)
{
	m_pSoundManager->SetPitch(channelTag, fPitchRatio);
}

#pragma endregion


#pragma region FONT_MANAGER
void CGameInstance::FontDraw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
	m_pFontManager->Draw(fontName, pText, vPosition, fScale, vColor, fRotation, vOrigin);
}
void CGameInstance::FontAddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale, _fvector vColor, _float fRotation, const _float2& vOrigin)
{
	m_pFontManager->AddLateDraw(eRenderGroup, fontName, pText, vPosition, fScale, vColor, fRotation, vOrigin);
}
_float2 CGameInstance::FontMeasureString(const StringID& fontName, const wchar_t* txt, float scale) const
{
	return m_pFontManager->MeasureString(fontName, txt, scale);
}
void CGameInstance::FontLateDraw(RENDERGROUP eRenderGroup)
{
	m_pFontManager->LateDraw(eRenderGroup);
}
#pragma endregion


#pragma region WORKER_MANAGER
void CGameInstance::WorkerEnqueue(_string_view svTaskName, _Func func)
{
	m_pWorkerManager->Enqueue(svTaskName, func);
}
#pragma endregion

#pragma region PROTOTYPE_MANAGER
HRESULT CGameInstance::AddPrototype(const StringID& svGroupTag, const StringID& svPrototypetag, UPtr<CPrototype> pPrototype)
{
	return m_pPrototypeManager->AddPrototype(svGroupTag, svPrototypetag, std::move(pPrototype));
}
UPtr<CPrototype> CGameInstance::ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg)
{
	return m_pPrototypeManager->ClonePrototype(svGroupTag, svPrototypetag, pArg);
}
void CGameInstance::DelPrototype(const StringID& sGroupTag)
{
	m_pPrototypeManager->DelPrototype(sGroupTag);
}
#pragma endregion


#pragma region GAMEOBJECT_MANAGER
void CGameInstance::GameObjectAllReset()
{
	m_pGameObjectManager->AllReset();
}

inline CGameObject* CGameInstance::GetGameObjectByHandle(const CHandle& handle)
{
	return m_pGameObjectManager->GetGameObjectByHandle(handle);
}
//const std::vector<CHandle>* CGameInstance::GetGameObjectLayer(std::string_view sLayerName) const
//{
//	return m_pGameObjectManager->GetLayer(sLayerName);
//}
//const std::vector<CHandle>* CGameInstance::GetGameObjectLayer(std::string_view sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg)
//{
//	return m_pGameObjectManager->GetLayer(sLayerName, iPrototypeLevelIndex, svPrototypeTag, pArg);
//}

const std::vector<std::pair<std::string, std::vector<CHandle>>>& CGameInstance::GetGameObjectLayers() const
{
	return m_pGameObjectManager->GetLayers();
}

//void CGameInstance::DelGameObjectLayer(std::string_view sLayerName)
//{
//	return m_pGameObjectManager->DelLayer(sLayerName);
//}
//std::optional<CHandle> CGameInstance::GetFreeHandle() const
//{
//	return m_pGameObjectManager->GetFreeHandle();
//}
#pragma endregion


#pragma region COLLIDER_MANAGER
void CGameInstance::AddColliderGroup(const StringID& groupTag, const CCollider* pCollider)
{
	m_pColliderManager->AddColliderGroup(groupTag, pCollider);
}
const std::vector<const CCollider*>* CGameInstance::GetColliderGroup(const StringID& groupTag) const
{
	return m_pColliderManager->GetColliderGroup(groupTag);
}
_bool CGameInstance::IntersectColl(const CCollider* pColl1, const CCollider* pColl2)
{
	return m_pColliderManager->IntersectColl(pColl1, pColl2);
}
const std::unordered_map<StringID, std::vector<const CCollider*>>* CGameInstance::GetColliders() const
{
	return m_pColliderManager->GetColliders();
}

#pragma endregion


#pragma region CAMERA_MANAGER
CCameraObject* CGameInstance::GetActiveCamera() const
{
	return m_pCameraManager->GetActiveCamera();
}
CCameraObject* CGameInstance::GetActiveCamera(const StringID& CameraID) const
{
	return m_pCameraManager->GetActiveCamera(CameraID);
}
HRESULT CGameInstance::SetActiveCamera(const StringID& CameraID)
{
	return m_pCameraManager->SetActiveCamera(CameraID);
}
CCameraObject* CGameInstance::GetCamera(const StringID& CameraID) const
{
	return m_pCameraManager->GetCamera(CameraID);
}
HRESULT CGameInstance::RegistCamera(const StringID& CameraID, const CHandle& handle)
{
	return m_pCameraManager->RegistCamera(CameraID, handle);
}

//const CCameraObject* CGameInstance::GetCameraObject(const StringID& GroupID) const
//{
//	return m_pCameraManager->GetCameraObject(GroupID);
//}
//HRESULT CGameInstance::SetCameraObject(const StringID& GroupID, const CHandle& handle)
//{
//	return m_pCameraManager->SetCameraObject(GroupID, handle);
//}

//CCameraObject* CGameInstance::GetActiveGameCamera() const
//{
//	return m_pCameraManager->GetActiveGameCamera();
//}
//
//HRESULT CGameInstance::SetActiveGameCamera(const StringID& CameraID)
//{
//	return m_pCameraManager->SetActiveGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveUICamera() const
//{
//	return m_pCameraManager->GetActiveUICamera();
//}
//
//HRESULT CGameInstance::SetActiveUICamera(const StringID& CameraID)
//{
//	return m_pCameraManager->SetActiveUICamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveGameCamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetActiveGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetActiveUICamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetActiveUICamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetGameCamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetGameCamera(CameraID);
//}
//
//CCameraObject* CGameInstance::GetUICamera(const StringID& CameraID) const
//{
//	return m_pCameraManager->GetUICamera(CameraID);
//}
//
//HRESULT CGameInstance::RegistGameCamera(const StringID& CameraID, const CHandle& handle)
//{
//	return m_pCameraManager->RegistGameCamera(CameraID, handle);
//}
//
//HRESULT CGameInstance::RegistUICamera(const StringID& CameraID, const CHandle& handle)
//{
//	return m_pCameraManager->RegistUICamera(CameraID, handle);
//}

#pragma endregion


#pragma region RENDERER
HRESULT CGameInstance::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject)
{
	return m_pRenderer->AddRenderObject(eRenderGroup, pRenderObject);
}
#pragma endregion

#pragma region ANIMEDIT_MANAGER
HRESULT CGameInstance::SetupTestModel() {
	return m_pAnimEdit_Manager->SetupTestModel();
}
#pragma endregion

#pragma region MAP_MANAGER
HRESULT CGameInstance::SaveMap(const std::string& path)
{
	return m_pMapManager->SaveMap(path);
}
HRESULT CGameInstance::LoadMap(const std::string& path, _bool clearBeforeLoad)
{
	return m_pMapManager->LoadMap(path, clearBeforeLoad);
}
HRESULT CGameInstance::LoadMapData(const std::string& path)
{
	return m_pMapManager->LoadMapData(path);
}
HRESULT CGameInstance::LoadMapChunk(const MAPCHUNK_COORD& coord)
{
	return m_pMapManager->LoadChunk(coord);
}
HRESULT CGameInstance::UnLoadMapChunk(const MAPCHUNK_COORD& coord)
{
	return m_pMapManager->UnLoadChunk(coord);
}
void CGameInstance::RebuildMapChunks()
{
	m_pMapManager->RebuildChunks();
}
HRESULT CGameInstance::RegisterMapMeshObjectToMapChunk(const CHandle& hObject)
{
	return m_pMapManager->RegisterMapMeshObject(hObject);
}
const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& CGameInstance::GetMapChunks() const
{
	return m_pMapManager->GetChunks();
}
const _float3& CGameInstance::GetMapChunkSize() const
{
	return m_pMapManager->GetChunkSize();
}
void CGameInstance::SetMapChunkStreaming(_bool enable)
{
	m_pMapManager->SetChunkStreaming(enable);
}
_bool CGameInstance::IsMapChunkStreaming() const
{
	return m_pMapManager->IsChunkStreaming();
}
#ifdef _DEBUG
void CGameInstance::SetDebugDrawMapChunk(_bool draw)
{
	return m_pMapManager->SetDebugDrawMapChunk(draw);
}
#endif
#pragma endregion

#pragma region LIGHT_MANAGER
VOID	CGameInstance::Bind_EnviromentLight() {
	m_pLightManager->Bind_EnviromentLight();
}
VOID	CGameInstance::Bind_DynamicLight() {
	m_pLightManager->Bind_DynamicLight();
}

VOID	CGameInstance::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
	m_pLightManager->Add_DirectionalLight(_Direction, _Color, _Intensity);
}
VOID	CGameInstance::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range) {
	m_pLightManager->Add_PointLight(_Position, _Color, _Intensity, _Range);
}
VOID	CGameInstance::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
	m_pLightManager->Add_SpotLight(_Position, _Color, _Intensity, _Range, _InnerAtt, _OuterAtt);
}
VOID	CGameInstance::Clear_DynamicLightList() {
	m_pLightManager->Clear_DynamicLightList();
}
#pragma endregion
#pragma endregion

#pragma region NODE_EDITOR
HRESULT	   CGameInstance::OpenBeHavior(CHandle Handle)
{
	return m_pNodeEditor->OpenBeHavior(Handle);
}
#pragma endregion

#pragma region NODE_EDITOR
HRESULT					CGameInstance::Add_Action_Prototype(NODEGROUP eType, const _string& strActionName, UPtr<class CBTRoot> pAction)
{
	return m_pActionManager->Add_Action_Prototype(eType, strActionName, std::move(pAction));
}
UPtr<class CBTRoot>	    CGameInstance::Show_ActioNode_List(NODEGROUP eType, uint32_t& iNode, ImVec2 vNodePos, CHandle Handle)
{
	return m_pActionManager->Show_ActioNode_List(eType, iNode, vNodePos, Handle);
}
void				CGameInstance::Show_Action_NodeWidget(CBTRoot* pNode)
{
	m_pActionManager->Show_Action_NodeWidget(pNode);
}
#pragma endregion

#pragma region PHYSX_MANAGER
//void CGameInstance::PxStepSimulation(float fFixedDeltaTime)
//{
//	m_pPhysXManager->StepSimulation(fFixedDeltaTime);
//}

physx::PxScene* CGameInstance::PxGetScene() const
{
	return m_pPhysXManager->GetScene();
}
physx::PxPhysics* CGameInstance::PxGetPhysics() const
{
	return m_pPhysXManager->GetPhysics();
}
_bool CGameInstance::PxRayCast(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, PHYSIX_RAYCAST_RESULT& outResult) const
{
	return m_pPhysXManager->RayCast(vOrigin, vNormalizedDir, fMaxDistance, outResult);
}
_bool CGameInstance::PxRayCastMultiple(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, std::vector<PHYSIX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit) const
{
	return m_pPhysXManager->RayCastMultiple(vOrigin, vNormalizedDir, fMaxDistance, outVecResult, iMaxHit);
}
#pragma endregion
UPtr<class CBTRoot>	    CGameInstance::Clone_Action(NODEGROUP eType, const _string& strActionName, void* pArg)
{
	return m_pActionManager->Clone_Action(eType, strActionName, pArg);
}
#pragma endregion
#pragma region ANIMATIONEDTIOR_MANAGER
int32_t CGameInstance::GetAnimIndex(CHandle Handle)
{
	return m_pAnimEdit_Manager->GetAnimIndex(Handle);
}
#pragma endregion
