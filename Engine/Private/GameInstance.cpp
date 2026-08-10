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
#include "Renderer.h"
#include "HizOcclusionCuller.h"
#include "ComConstantBuffer.h"
#include "FlyCamera.h"
#include "CameraObject.h"
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
#include "MapStaticModelLoader.h"
#include "MapManager.h"
#include "NavMeshManager.h"
#include "PhysXManager.h"
#include "NvClothManager.h"
#include "DbgLineRender.h"
#include "SerializeManager.h"
#include "PathPlaybackEditor.h"

#include "ComPxBoxCollider.h"
#include "ComPxCapsuleCollider.h"
#include "ComPxSphereCollider.h"
#include "ComPxCollider.h"
#include "ComPxRigidBody.h"
#include "ComPxTriMeshCollider.h"
#include "ComPxCharacterController.h"

#include "ParticleManager.h"
#include "Particle.h"

#include "ButtonComponent.h"
#include "TweenComponent.h"
#include "Model_Instance_Manager.h"

#include "GameInstanceInitLoader.h"

#include "MapMeshInstancingRenderer.h"

#include "EventManager.h"
#include "EffectManager.h"

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

	m_pNvClothManager = CNvClothManager::Create(
		ppDevice.Get(),
		ppContext.Get(),
		EngineDesc.eNvClothBackend);
	if (!m_pNvClothManager)
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

	

	m_pPrototypeManager = CPrototypeManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pPrototypeManager == nullptr)
	{
		return E_FAIL;
	}

	if (FAILED(CGameInstanceInitLoader::InitLoadStart()))
	{
		MSG_BOX("INIT LODER FAILED");
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

	m_pWorkerManager = CWorkerManager::Create("Normal", std::thread::hardware_concurrency());
	if (m_pWorkerManager == nullptr)
	{
		return E_FAIL;
	}

	m_pGameObjectManager = CGameObjectManager::Create();
	if (m_pGameObjectManager == nullptr)
	{
		return E_FAIL;
	}

	m_pGameObjectPoolManager = CGameObjectPoolManager::Create(
		m_pGameObjectManager.get());
	if (m_pGameObjectPoolManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("Start m_pRenderer()");
	m_pRenderer = CRenderer::Create(ppDevice.Get(), ppContext.Get());
	if (m_pRenderer == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pRenderer()");
	m_pHizOcclusionCuller = CHizOcclusionCuller::Create();
	if (m_pHizOcclusionCuller == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pHizOcclusionCuller");

	m_pCameraManager = CCameraManager::Create();
	if (m_pCameraManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pCameraManager");

	m_pColliderManager = CColliderManager::Create();
	if (m_pColliderManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pColliderManager");

	m_pAnimEdit_Manager = CAnimEdit_Manager::Create();
	if (m_pAnimEdit_Manager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pAnimEdit_Manager");

	m_pLightManager = CLightManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pLightManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pLightManager");

	m_pParticleManager = CParticleManager::Create();
	if (m_pParticleManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pParticleManager");

	m_pFontManager = CFontManager::Create(ppDevice.Get(), ppContext.Get());
	if (m_pFontManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pFontManager");

	m_pMapManager = CMapManager::Create();
	if (m_pMapManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pMapManager");

	m_pNavMeshManager = CNavMeshManager::Create();
	if (m_pNavMeshManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pNavMeshManager");

	m_pNodeEditor = CNodeEditor::Create();
	if (m_pNodeEditor == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pNodeEditor");

	m_pPhysXManager = CPhysXManager::Create();
	if (m_pPhysXManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pPhysXManager");


	m_pActionManager = CAction_Manager::Create();
	if (m_pActionManager == nullptr)
		return E_FAIL;
	LOG_MEMORY("End m_pActionManager");


	m_pSerializeManager = CSerializeManager::Create();
	if (m_pSerializeManager == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pSerializeManager");

	m_pPathPlaybackEditor = CPathPlaybackEditor::Create();
	if (m_pPathPlaybackEditor == nullptr)
		return E_FAIL;

	m_pModel_Instance_Manager = CModel_Instance_Manager::Create();
	if (m_pModel_Instance_Manager == nullptr) {
		return E_FAIL;
	}
	LOG_MEMORY("End m_pModel_Instance_Manager");

	m_pMapMeshInstancingRenderer = CMapMeshInstancingRenderer::Create();
	if (m_pMapMeshInstancingRenderer == nullptr)
	{
		return E_FAIL;
	}
	LOG_MEMORY("End m_pMapMeshInstancingRenderer");

	m_pEventManager = CEventManager::Create();
	if (m_pEventManager == nullptr)
	{
		return E_FAIL;
	}
	m_pEffectManager = CEffectManager::Create(m_pParticleManager.get(), m_pLightManager.get(), m_pSoundManager.get());
	if (m_pEffectManager == nullptr)
	{
		return E_FAIL;
	}
	
	return S_OK;
}

void CGameInstance::FixedUpdateEngine(_float fFixedTimeDelta)
{
	m_pPhysXManager->PrepareCCTInteractions(fFixedTimeDelta);

	{
		ZoneScopedN("GameObjectManager_FixedUpdate");
		m_pGameObjectManager->FixedUpdate(fFixedTimeDelta);
	}
	
	m_pPhysXManager->StepSimulation(fFixedTimeDelta);


	{
		ZoneScopedN("pNvClothManager_StepSimulation");
		if (m_pNvClothManager)
			m_pNvClothManager->StepSimulation(fFixedTimeDelta);
	}

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
		ZoneScopedN("GameObjectPoolManager_UpdateGUI");
		if (m_pGameObjectPoolManager)
			m_pGameObjectPoolManager->UpdateGUI();
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

	 m_pSoundManager->UpdateGUI();

	m_pNodeEditor->NodeEditorUpdate();
	m_pPhysXManager->UpdateGUI();
	if (m_pNvClothManager)
		m_pNvClothManager->UpdateGUI();

	m_pSerializeManager->UpdateGUI();
	if (m_pPathPlaybackEditor)
		m_pPathPlaybackEditor->UpdateGUI();

	m_pLuaManager->UpdateGUI();
	m_pEffectManager->UpdateGUI();
	//if (ImGui::Button("ShaderRebuild"))
	//{
	//	//TAG_RES_GRP_PERMANENT_SHADER
	//	if (auto resources = GetResource(TAG_RES_GRP_PERMANENT_SHADER))
	//	{
	//		for (auto& [res] : resources)
	//		{
	//			if (!res.empty())
	//			{
	//				res.front()->Unload();
	//				res.front()->Load();
	//			}
	//		}
	//	}
	//}
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

	{
		ZoneScopedN("EffectManager_Update");
		m_pEffectManager->Update(fTimeDelta);
	}

	// lua hot reload
	{
		ZoneScopedN("LuaManager_Update");
		m_pLuaManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("ShaderHotReload_Update");
		m_pResourceManager->UpdateShaderHotReload();
	}



	{
		ZoneScopedN("AnimEdit_Update");
		m_pAnimEdit_Manager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("ParticleManager_Update");
		m_pParticleManager->Update(fTimeDelta);
	}
	

	{
		ZoneScopedN("PhysXManager_Update");
		m_pPhysXManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("GameObjectManager_Update");
		m_pGameObjectManager->Update(fTimeDelta);
	}

	{
		ZoneScopedN("CameraManager_Update");
		m_pCameraManager->Update(fTimeDelta);
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
	{
		ZoneScopedN("Renderer_Update");
		m_pRenderer->Update(fTimeDelta);
	}

	m_pColliderManager->Update();

	m_pMapManager->Update(fTimeDelta);
	m_pMapMeshInstancingRenderer->Update();

	if (m_pNvClothManager)
		m_pNvClothManager->RenderDebug(*m_pDbgLineRender);
	m_pDbgLineRender->AddAxis(1.f, XMMatrixTranslation(1.3f, 1.2f, 0.f));
	m_pNavMeshManager->DrawDebug();

	AddRenderObject(RENDERGROUP::NONBLEND_INSTANCED, m_pModel_Instance_Manager.get());
	AddRenderObject(RENDERGROUP::EFFECT, m_pParticleManager.get());
	AddRenderObject(RENDERGROUP::COLLIDER, m_pDbgLineRender.get());

	// 모든 게임 오브젝트와 카메라의 LateUpdate가 끝난 뒤 활성 카메라 하나만 Listener 0에 반영한다.
	if (auto* pCamera = GetActiveCamera())
	{
		auto& cameraTransform = pCamera->GetTransform();
		_float3 vForward{};
		_float3 vUp{};
		XMStoreFloat3(&vForward, XMVector3Normalize(cameraTransform.GetState(STATE::LOOK)));
		XMStoreFloat3(&vUp, XMVector3Normalize(cameraTransform.GetState(STATE::UP)));

		m_pSoundManager->SetListenerAttributes(0, SOUND_LISTENER_DESC{
			.vPosition = cameraTransform.GetPosition(),
			.vVelocity = {},
			.vForward = vForward,
			.vUp = vUp
		});
	}

	// 최신 Listener/Emitter 값을 FMOD에 반영하고 종료된 SOUND_ID를 정리한다.
	{
		ZoneScopedN("SoundManager_Update");
		m_pSoundManager->Update();
	}
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

void CGameInstance::Release_Engine()
{
	m_pPathPlaybackEditor.reset();
	m_pMapMeshInstancingRenderer.reset();
	m_pNodeEditor.reset();
	m_pImguiManager.reset();
	m_pDInputManager.reset();
	m_pActionManager.reset();
	m_pAnimEdit_Manager.reset();
	m_pModel_Instance_Manager.reset();
	m_pGameObjectPoolManager.reset();
	if (m_pGameObjectManager)
	{
		m_pGameObjectManager->AllReset();
		//m_pGameObjectManager->FrameStart();
		m_pGameObjectManager->FrameEnd();
	}
	
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

	if(m_pResourceManager) m_pResourceManager->Release();
	m_pResourceManager.reset();
	m_pSoundManager.reset();

	m_pNavMeshManager.reset();
	m_pMapManager.reset();
	m_pPhysXManager.reset();
	m_pNvClothManager.reset();
	m_pEventManager.reset();
	m_pEffectManager.reset();
	m_pGraphicDevice.reset();
}

void CGameInstance::SetMouseFix(_bool mousefix)
{
	m_bMouseFix = mousefix;

	//if(m_bMouseFix)
	//	ShowCursor(FALSE);
}


void CGameInstance::FrameStart(_float fTimeDelta)
{
	{
		ZoneScopedN("InputManager_Update");
		m_pDInputManager->Update_InputDev();
	}
	m_iFrameCnt++;

	{
		ZoneScopedN("m_pLevelManager_FrameStart");
		m_pLevelManager->FrameStart(fTimeDelta);
	}

	{
		ZoneScopedN("m_pGameObjectManager_FrameStart");
		m_pGameObjectManager->FrameStart();
	}
	
	{
		ZoneScopedN("m_pColliderManager_FrameStart");
		m_pColliderManager->FrameStart();
	}



	{
		ZoneScopedN("GameObjectManager_PriorityUpdate");
		m_pGameObjectManager->PriorityUpdate(fTimeDelta);
	}
}
void CGameInstance::FrameEnd(_float fTimeDelta)
{
	m_pGameObjectManager->FrameEnd();
	m_pGameObjectPoolManager->FrameEnd();
	m_pLevelManager->FrameEnd(fTimeDelta);
	m_pEventManager->FrameEnd();

	m_pRenderer->FrameEnd();
	m_pMapMeshInstancingRenderer->FrameEnd();
	m_pModel_Instance_Manager->Clear_Frame();
	m_pColliderManager->FrameEnd();
	m_pDbgLineRender->FrameEnd();

}


#pragma region PARTICLE_MANAGER

HRESULT CGameInstance::LoadParticleJson(const std::string& strJsonPath) {
	return m_pParticleManager->LoadParticleJson(strJsonPath);
}
HRESULT CGameInstance::LoadParticlePresets(const std::string& strJsonPath) {
	return m_pParticleManager->LoadParticlePresets(strJsonPath);
}
CParticle* CGameInstance::GetParticle(const StringID& sGroupTag, const StringID& sTypeTag)
{
	return m_pParticleManager->GetParticle(sGroupTag,sTypeTag);
}
HRESULT CGameInstance::Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
	uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
	_bool bLoop, _float fSpawnInterval)
{
	return m_pParticleManager->Spawn(sGroupTag, sTypeTag, count, pSpawnData, bLoop, fSpawnInterval);
}

std::vector<SPAWN_COMMAND>  CGameInstance::Parse_Command(const std::string& strJsonFile)
{
	return m_pParticleManager->Parse_Command(strJsonFile);
}

uint32_t CGameInstance::Spawn(const std::vector<SPAWN_COMMAND>& templateCommands, const _float4x4& worldMat, _fvector endPos) {
	return m_pParticleManager->Spawn(templateCommands, worldMat ,endPos);
}
HRESULT CGameInstance::Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle)
{
	return m_pParticleManager->Add_Particle(sGroupTag, sTypeTag, std::move(particle));
}

std::vector<std::string> CGameInstance::Load_FilePath_ByExtension(const std::filesystem::path& _FolderPath, std::string_view _Extension)
{
	return m_pParticleManager->Load_FilePath_ByExtension(_FolderPath, _Extension);
}
HRESULT CGameInstance::Load_ParticleJsonPackage(const std::vector<std::string>& _FilePathPackage)
{
	return m_pParticleManager->Load_ParticleJsonPackage(_FilePathPackage);
}

void CGameInstance::TranslateOwner(uint32_t ownerId, const _float3& delta) {
	m_pParticleManager->TranslateOwner(ownerId, delta);
}

HRESULT CGameInstance::AddTrailPoint(const StringID& groupTag, const StringID& typeTag, const _float3& start, const _float3& end) {
	return m_pParticleManager->AddTrailPoint(groupTag, typeTag, start, end);
}
std::optional<BEAM_HANDLE> CGameInstance::SpawnBeam(const StringID& groupTag,const StringID& typeTag,const BEAM_PARAMS& params)
{
	return m_pParticleManager->SpawnBeam(groupTag,typeTag,params);
}
HRESULT CGameInstance::SetBeamPositions(const BEAM_HANDLE& handle,const _float4& start,const _float4& end)
{
	return m_pParticleManager->SetBeamPositions(handle,start,end);
}

HRESULT CGameInstance::StopBeam(const BEAM_HANDLE& handle)
{
	return m_pParticleManager->StopBeam(handle);
}
#pragma endregion

#pragma region EFFECT_MANAGER

EFFECT_INSTANCE_ID CGameInstance::PlayEffect(const std::string& sEffectName, const _float4x4& matWorld,
	_fvector vEndPosition, EFFECT_FINISHED_CALLBACK onFinsihed) {
	
	return m_pEffectManager->PlayEffect(sEffectName, matWorld, vEndPosition, onFinsihed);
}

void CGameInstance::StopEffect(EFFECT_INSTANCE_ID iEffectId) {
	m_pEffectManager->StopEffect(iEffectId);
}

void CGameInstance::SetEffectPosition(EFFECT_INSTANCE_ID iEffectId, const _float3& vPosition) {
	m_pEffectManager->SetEffectPosition(iEffectId, vPosition);

}

void CGameInstance::SetEffectWorldMatrix(EFFECT_INSTANCE_ID iEffectId, const _float4x4& colliderWorldMatrix) {
	m_pEffectManager->SetEffectWorldMatrix(iEffectId, colliderWorldMatrix);

}
void CGameInstance::SetBeamPositionsByOwner(EFFECT_INSTANCE_ID effectId, const _float3& start, const _float3& end) {
	m_pEffectManager->SetBeamPositionsByOwner(effectId, start, end);
}
void CGameInstance::ClearAllRunningEffect() {
	m_pEffectManager->ClearAllRunningEffect();
}



#pragma endregion

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
std::vector<SPtr<CResource>> CGameInstance::GetResource(const StringID& sGroupTag, const StringID& sResTag) const
{
	return m_pResourceManager->GetResource(sGroupTag, sResTag);
}
std::unordered_map<StringID, std::vector<SPtr<CResource>>> CGameInstance::GetResource(const StringID& sGroupTag) const
{
	return m_pResourceManager->GetResource(sGroupTag);
}

std::unordered_map<StringID, std::unordered_map<StringID, std::vector<SPtr<CResource>>>> CGameInstance::GetResources() const
{
	return m_pResourceManager->GetResources();
}

void CGameInstance::DelResource(const StringID& sGroupTag)
{
	m_pResourceManager->DelResource(sGroupTag);
}
void CGameInstance::DelResource(const StringID& sGroupTag, const StringID& sResTag)
{
	m_pResourceManager->DelResource(sGroupTag, sResTag);
}
std::vector<SPtr<CResource>> CGameInstance::GetResourcesByPath(const _string& sPath) const
{
	if (!m_pResourceManager) return {};
	return m_pResourceManager->GetResourcesByPath(sPath);
}
void CGameInstance::RemoveResourcePathLookup(const _string& sPath, SPtr<CResource> pRes)
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
uint32_t CGameInstance::GetCurrentLevelID() const
{
	return m_pLevelManager ? m_pLevelManager->GetCurrentLevelID() : CLevel::INVALID_LEVEL_ID;
}
void CGameInstance::RegisterLevelChangeFunc(const _string& ID, _Func func)
{
	m_pLevelManager->RegisterLevelChangeFunc(ID, func);
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
void CGameInstance::FontAddLateDraw3D(const std::string& fontTag, const std::wstring& text, _fmatrix matWVP, _fvector color, _float2 pivot)
{
	m_pFontManager->FontAddLateDraw3D(fontTag, text, matWVP, color, pivot);
}
void CGameInstance::Render3DFont()
{
	m_pFontManager->Render3DFont();
}
#pragma endregion


#pragma region WORKER_MANAGER
_bool CGameInstance::WorkerEnqueue(_string_view svTaskName, _Func func)
{
	return m_pWorkerManager->Enqueue(svTaskName, func);
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
void CGameInstance::DelPrototype(
	const StringID& sGroupTag, const StringID& sPrototypeTag)
{
	m_pPrototypeManager->DelPrototype(sGroupTag, sPrototypeTag);
}
std::vector<StringID> CGameInstance::GetPrototypeTags(const StringID& svGroupTag) const
{
	return m_pPrototypeManager->GetPrototypeTags(svGroupTag);
}
#pragma endregion


#pragma region GAMEOBJECT_MANAGER
void CGameInstance::GameObjectAllReset()
{
	m_pGameObjectManager->AllReset();
}

size_t CGameInstance::GameObjectResetLayers(
	std::span<const std::string_view> layerNames)
{
	return m_pGameObjectManager->ResetObjectsInLayers(layerNames);
}

size_t CGameInstance::GameObjectAllResetExceptLayers(
	std::span<const std::string_view> excludedLayerNames)
{
	return m_pGameObjectManager->ResetAllObjectsExceptLayers(excludedLayerNames);
}

inline CGameObject* CGameInstance::GetGameObjectByHandle(const CHandle& handle)
{
	return m_pGameObjectManager->GetGameObjectByHandle(handle);
}

const std::vector<std::pair<std::string, std::vector<CHandle>>>& CGameInstance::GetGameObjectLayers() const
{
	return m_pGameObjectManager->GetLayers();
}

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
HRESULT CGameInstance::RegistCinematicAsset(const SPtr<CCinematicAsset>& pAsset)
{
	return m_pCameraManager->RegistCinematicAsset(pAsset);
}
HRESULT CGameInstance::LoadCinematic(
	const std::string& CinematicName)
{
	return m_pCameraManager->LoadCinematic(CinematicName);
}
HRESULT CGameInstance::PlayCinematic(const StringID& CinematicID, const FCinematicPlayOptions& Options)
{
	return m_pCameraManager->PlayCinematic(CinematicID, Options);
}
HRESULT CGameInstance::PlayCinematic(const StringID& CinematicID, const CHandle& TargetHandle, const FCinematicPlayOptions& Options)
{
	return m_pCameraManager->PlayCinematic(CinematicID, TargetHandle, Options);
}
void CGameInstance::StopCinematic()
{
	m_pCameraManager->StopCinematic();
}
_bool CGameInstance::IsCinematicPlaying() const
{
	return m_pCameraManager->IsCinematicPlaying();
}
_float CGameInstance::GetCinematicPlayTime() const
{
	return m_pCameraManager->GetCinematicPlayTime();
}
void CGameInstance::SetCinematicCollisionQueryMask(uint32_t iQueryMask)
{
	if (m_pCameraManager == nullptr)
	{
		return;
	}

	m_pCameraManager->SetCinematicCollisionQueryMask(iQueryMask);
}
#pragma endregion


#pragma region RENDERER
HRESULT CGameInstance::AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject) {
	return m_pRenderer->AddRenderObject(eRenderGroup, pRenderObject);
}

_bool CGameInstance::IsOcclusionCulled(const IRenderable* pRenderObject)
{
	if (m_pHizOcclusionCuller == nullptr || m_pRenderer == nullptr || !m_pRenderer->HasPrevHizBuffer())
	{
		return false;
	}

	auto* pCamera = GetActiveCamera();
	if (pCamera == nullptr)
	{
		return false;
	}

	const _matrix matViewProj = pCamera->GetView() * pCamera->GetProj();
	return m_pHizOcclusionCuller->IsOcclusionCulledCPU(pRenderObject, matViewProj, m_vClientScreenSize);
}

const CHizBuffer* CGameInstance::GetPrevHizBuffer() const
{
	if (m_pRenderer == nullptr || !m_pRenderer->HasPrevHizBuffer())
	{
		return nullptr;
	}

	return m_pRenderer->GetPrevHizBuffer();
}
HRESULT	CGameInstance::Reset_DefaultShader(RENDERGROUP _Group) {
	return m_pRenderer->Reset_DefaultShader(_Group);
}

SPtr<CResDynamicTexture2D>	CGameInstance::Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {
	return m_pRenderer->Generate_RenderTarget(_sResTag, _Format, _BindFlags, _TexWidth, _TexHeight);
}
SPtr<CResDynamicTexture2D>	CGameInstance::Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth, uint32_t _TexHeight) {
	return m_pRenderer->Generate_DepthStencil_RenderTarget(_sResTag, _TexFormat, _DSVFormat, _SRVFormat, _TexWidth, _TexHeight);
}
SPtr<CResDynamicTexture2D>	CGameInstance::Generate_UnorderedAccessView(const StringID& _sResTag, DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth, uint32_t _TexHeight) {
	return m_pRenderer->Generate_UnorderedAccessView(_sResTag, _TexFormat, _BindFlags, _TexWidth, _TexHeight);
}
SPtr<CResViewPort>			CGameInstance::Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth, uint32_t _TexHeight) {
	return m_pRenderer->Generate_ViewPort(_sResTag, _TexWidth, _TexHeight);
}
HRESULT	CGameInstance::Generate_Texture2DArray(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount) {
	return m_pRenderer->Generate_Texture2DArray(_ShadowDSVList, _TextureArray, _SRV, _Resolution, _MaxLightCount);
}
HRESULT	CGameInstance::Generate_ShadowCubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount) {
	return m_pRenderer->Generate_ShadowCubeMap(_ShadowDSV, _TextureArray, _SRV, _Resolution, _MaxLightCount);
}
HRESULT	CGameInstance::Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	return m_pRenderer->Generate_ShadowTexture(_ShadowDSV, _Texture, _SRV, _ResolutionX, _ResolutionY);
}
HRESULT CGameInstance::Generate_ShadowMapOutput(ID3D11UnorderedAccessView** _ShadowUAV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _ShadowSRV, uint32_t _LTYPE, uint32_t _ResolutionX, uint32_t _ResolutionY) {
	return m_pRenderer->Generate_ShadowMapOutput(_ShadowUAV, _Texture, _ShadowSRV, _LTYPE, _ResolutionX, _ResolutionY);
}
VOID	CGameInstance::Render_ChromaticRing(XMVECTOR _WorldPosition, _float _Duration, _float _Scale) {
	m_pRenderer->Render_ChromaticRing(_WorldPosition, _Duration, _Scale);
}
VOID	CGameInstance::Set_ChromaticRingOpacity(_float _Opacity) { m_pRenderer->Set_ChromaticRingOpacity(_Opacity); }
VOID	CGameInstance::Apply_OutlineEffect(std::optional<CHandle> targetHandle) { m_pRenderer->Apply_OutlineEffect(targetHandle); }


#pragma endregion

#pragma region ANIMEDIT_MANAGER
HRESULT CGameInstance::SetupTestModel() {
	return m_pAnimEdit_Manager->SetupTestModel();
}
_string CGameInstance::GetAnimName(uint32_t iIndex, CHandle Handle)
{
	return m_pAnimEdit_Manager->GetAnimName(iIndex, Handle);
}
#pragma endregion

#pragma region MAP_MANAGER
HRESULT CGameInstance::SaveMap(const std::string& path)
{
	return m_pMapManager->SaveMap(path);
}
HRESULT CGameInstance::LoadMapResources(const std::string& path)
{
	auto modelLoad = IndexStaticModelsRequiredByMap(path, PATH_MAPEDITOR_STATIC_MODEL_DIR, TAG_RES_GRP_MAPEDITOR_STATIC_MODEL);
	m_pMapManager->SetMapModelResourceIndex(PATH_MAPEDITOR_STATIC_MODEL_DIR, TAG_RES_GRP_MAPEDITOR_STATIC_MODEL, std::move(modelLoad.modelPaths));
	return modelLoad.Succeeded() ? S_OK : E_FAIL;
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
std::vector<CHandle> CGameInstance::CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const
{
	return m_pMapManager->CollectMapMeshPickCandidates(rayOrigin, rayDirection);
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
void CGameInstance::ClearAllChunk()
{
	m_pMapManager->ClearAllChunk();
}
#pragma endregion

#pragma region LIGHT_MANAGER
std::optional<CHandle> CGameInstance::Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity) {
	return m_pLightManager->Add_DirectionalLight(_Direction, _Color, _Intensity);
}
std::optional<CHandle> CGameInstance::Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _InnerRange, _float _OuterRange) {
	return m_pLightManager->Add_PointLight(_Position, _Color, _Intensity, _InnerRange, _OuterRange);
}
std::optional<CHandle> CGameInstance::Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt) {
	return m_pLightManager->Add_SpotLight(_Position, _Color, _Intensity, _Range, _InnerAtt, _OuterAtt);
}
_bool CGameInstance::Remove_Light(const CHandle& hLight) {
	return m_pLightManager->Remove_Light(hLight);
}
size_t CGameInstance::Remove_PlacementLightGroup(
	std::string_view sGroup) {
	return m_pLightManager->
		Remove_PlacementLightGroup(sGroup);
}
void CGameInstance::SetActivePlacementLightGroup(
	std::string_view sGroup) {
	m_pLightManager->
		SetActivePlacementLightGroup(sGroup);
}
VOID	CGameInstance::Clear_DynamicLightList() {
	m_pLightManager->Clear_DynamicLightList();
}
HRESULT	CGameInstance::AddShadowRenderGroup(ACTORTYPE _ATYPE, IRenderable* pRenderObject) {
	return m_pLightManager->AddShadowRenderGroup(_ATYPE, pRenderObject);
}
HRESULT	CGameInstance::Render_ObjectShadow() {
	return m_pLightManager->Render_ObjectShadow();
}
HRESULT	CGameInstance::Render_ObjectNonShadow() {
	return m_pLightManager->Render_ObjectNonShadow();
}
HRESULT	CGameInstance::Initialize_EffectLight(uint32_t _PoolSize) {
	return m_pLightManager->Initialize_EffectLight(_PoolSize);
}
std::optional<CHandle> CGameInstance::Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color, _float _InnerRange, _float _OuterRange, _float _LifeTime, _float3 _Velocity) {
	return m_pLightManager->Allocate_EffectLight(_WorldPos, _Intensity, _Color, _InnerRange, _OuterRange, _LifeTime, _Velocity);
}
HRESULT	CGameInstance::Capture_ShadowMap() {
	return m_pLightManager->Capture_ShadowMap();
}
VOID	CGameInstance::Notify_StaticShadowSceneChanged(const BoundingBox& ChangedBounds) {
	m_pLightManager->Notify_StaticShadowSceneChanged(ChangedBounds);
}
VOID	CGameInstance::Bind_VolumetricLocalLightResources() {
	m_pLightManager->Bind_VolumetricLocalLightResources();
}
VOID	CGameInstance::UnBind_VolumetricLocalLightResources() {
	m_pLightManager->UnBind_VolumetricLocalLightResources();
}
_bool	CGameInstance::Evaluate_DirectionalLightCount() {
	return m_pLightManager->Evaluate_DirectionalLightCount();
}

XMMATRIX CGameInstance::Get_CascadeShadowViewProj(uint32_t _Index) {
	return m_pLightManager->Get_CascadeShadowViewProj(_Index);
}
XMFLOAT4 CGameInstance::Get_CascadeShadowSplits() {
	return m_pLightManager->Get_CascadeShadowSplits();
}
CSM_DATA& CGameInstance::Get_MainDirectionalLightData() {
	return m_pLightManager->Get_MainDirectionalLightData();
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
physx::PxControllerManager* CGameInstance::PxGetControllerManager() const
{
	return m_pPhysXManager->GetControllerManager();
}
//_bool CGameInstance::PxRayCast(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, PX_RAYCAST_RESULT& outResult) const
//{
//	return m_pPhysXManager->RayCast(vOrigin, vNormalizedDir, fMaxDistance, outResult);
//}
//_bool CGameInstance::PxRayCastMultiple(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, std::vector<PX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit) const
//{
//	return m_pPhysXManager->RayCastMultiple(vOrigin, vNormalizedDir, fMaxDistance, outVecResult, iMaxHit);
//}
#pragma endregion

#pragma endregion
#pragma region ANIMATIONEDTIOR_MANAGER
int32_t CGameInstance::GetAnimIndex(CHandle Handle)
{
	return m_pAnimEdit_Manager->GetAnimIndex(Handle);
}
#pragma endregion
#pragma region INSTANCE_MANAGER
void CGameInstance::Add_Instance(CComModelInstance* pModelInstance, CComAnimator* pAnimator, const _float4x4& WorldMatrix, uint32_t iFlags) {
	m_pModel_Instance_Manager->Add_Instance(pModelInstance, pAnimator, WorldMatrix, iFlags);
}

void CGameInstance::Add_Instance(CComStaticModelInstance* pModelInstance, const _float4x4& WorldMatrix, uint32_t iFlags) {
	m_pModel_Instance_Manager->Add_Instance(pModelInstance, WorldMatrix, iFlags);
}


void CGameInstance::Add_Instance(CComModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData) {

	m_pModel_Instance_Manager->Add_Instance(pModelInstance, InstanceData);
}

void CGameInstance::Add_Part_Instance(CComStaticModelInstance* pModelInstance, const GPU_PART_INSTANCE_DATA& InstanceData) {
	m_pModel_Instance_Manager->Add_Part_Instance(pModelInstance, InstanceData);
}

const std::vector<MODEL_INSTANCE_BATCH*>& CGameInstance::Get_ActiveBatches() const {
	return m_pModel_Instance_Manager->Get_ActiveBatches();
};
HRESULT CGameInstance::Render_ShadowInstanced(const ComPtr<ID3D11DeviceContext>& pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch, int32_t _PointFaceIndex) {
	return m_pModel_Instance_Manager->Render_ShadowInstanced(pContext.Get(), _LightHandle, _bStaticBatch, _PointFaceIndex);
}
_bool	CGameInstance::Has_ActiveDynamicShadowBatch() {
	return m_pModel_Instance_Manager->Has_ActiveDynamicShadowBatch();
}
#pragma endregion

#pragma region MAPMESH_INSTANCE_RENDER
HRESULT CGameInstance::PushMapObjectInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData, EMapMeshRenderFeature renderFeature)
{
	return m_pMapMeshInstancingRenderer->PushMapObjectInstance(pModel, renderFeature, instanceData, occlusionData);
}
// 인스턴싱 On/Off , 드로우 콜 GUI
_bool CGameInstance::IsInstancingEnabled()
{
	return m_pMapMeshInstancingRenderer->IsInstancingEnabled();
}
void CGameInstance::SetInstancingEnabled(_bool bEnabled)
{
	m_pMapMeshInstancingRenderer->SetInstancingEnabled(bEnabled);
}
const INSTANCING_STATS& CGameInstance::GetInstancingStats()
{
	return m_pMapMeshInstancingRenderer->GetInstancingStats();
}
_bool CGameInstance::IsDebugBoundsEnabled()
{
	return m_pMapMeshInstancingRenderer->IsDebugBoundsEnabled();
}
void CGameInstance::SetDebugBoundsEnabled(_bool bEnabled)
{
	return m_pMapMeshInstancingRenderer->SetDebugBoundsEnabled(bEnabled);
}
void CGameInstance::ClearMapMeshTextureCache()
{
	if (m_pMapMeshInstancingRenderer)
	{
		m_pMapMeshInstancingRenderer->ClearTextureCache();
	}
}
void CGameInstance::EraseMapMeshTextureCache(const SPtr<CResStaticModel>& model)
{
	if (m_pMapMeshInstancingRenderer)
	{
		m_pMapMeshInstancingRenderer->EraseTextureCache(model);
	}
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
