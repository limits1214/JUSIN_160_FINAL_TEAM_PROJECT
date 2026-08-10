#include "MiniMap.h"
#include "pch.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "Level_Defines.h"
#include "PlayerThirdPersonCamera.h"
#include "TextureUI.h"
#include "Monster.h"

NS_USING(Client)

CMiniMap::CMiniMap()
{
}

CMiniMap::~CMiniMap()
{
}

HRESULT CMiniMap::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CMiniMap::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		CComConstantBuffer::DESC mDesc{};
		mDesc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_MiniMap" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerMiniMap", &mDesc, &m_pMinimapCBuffer)))
		{
			return E_FAIL;
		}; 


		/* Component */
		CComponent::DESC CDesc{};
		CDesc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::MINIMAP);
	ConfigureDefaultProfile();
	return S_OK;
}

void CMiniMap::PriorityUpdate(E::_float fTimeDelta)
{
	InitializeMonsterMarkerPool();
	InitializeBattleZone();
	InitializeObjectives();
	SearchPlayerIcon();
}

void CMiniMap::Update(E::_float fTimeDelta)
{
	ConfigureDefaultProfile();

	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
	{
		m_bHasPreviousPlayerPosition = false;
		m_iVisibleBattleZoneCount = 0;
		HideMonsterMarkers();
		HideObjectiveMarkers();
		return;
	}

	m_fSmokeTime = fmodf(m_fSmokeTime + fTimeDelta, 10000.f);

	CUIObject::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}

	auto* pCamera = Cast<CPlayerThirdPersonCamera>(
		E::CGameInstance::Get().GetActiveCamera("PlayerCamera"));
	if (!pCamera)
	{
		m_bHasPreviousPlayerPosition = false;
		m_iVisibleBattleZoneCount = 0;
		HideMonsterMarkers();
		HideObjectiveMarkers();
		return;
	}

	auto* pPlayer = E::CGameInstance::Get().GetGameObjectByHandle(
		pCamera->GetTargetHandle());
	if (!pPlayer)
	{
		m_bHasPreviousPlayerPosition = false;
		m_iVisibleBattleZoneCount = 0;
		HideMonsterMarkers();
		HideObjectiveMarkers();
		return;
	}

	UpdateFogMovementOffset(pPlayer->GetTransform().GetPosition());
	UpdateWorldMapOffset(pPlayer->GetTransform().GetPosition());
	UpdateBattleZones(pPlayer->GetTransform().GetPosition());
	UpdateMonsterMarkers(fTimeDelta, pPlayer);
	// PlayerCamera는 플레이어 위치/방향 조회에 사용하고,
	// 월드 좌표 투영은 실제 렌더링 중인 활성 카메라를 사용한다.
	UpdateObjectiveMarkers(
		fTimeDelta,
		pPlayer->GetTransform().GetPosition(),
		E::CGameInstance::Get().GetActiveCamera());

	XMVECTOR cameraLook = XMVectorSetY(
		pCamera->GetTransform().GetState(STATE::LOOK), 0.f);
	XMVECTOR playerLook = XMVectorSetY(
		pPlayer->GetTransform().GetState(STATE::LOOK), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(cameraLook)) < 0.000001f ||
		XMVectorGetX(XMVector3LengthSq(playerLook)) < 0.000001f)
		return;

	cameraLook = XMVector3Normalize(cameraLook);
	playerLook = XMVector3Normalize(playerLook);

	XMStoreFloat3(&m_cameraLook, cameraLook);
	XMStoreFloat3(&m_playerLook, playerLook);

	CalcDir();
}

void CMiniMap::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CMiniMap::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_MiniMap");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_MiniMap");
	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	{
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		const _float renderAlpha =
			m_MiniMapProfile.Mode == MINIMAP_MODE::DUNGEON_FOG ?
			std::clamp(m_UIINFO.Alpha * m_MiniMapProfile.FogAlphaMultiplier,
				0.f, 1.f) :
			m_UIINFO.Alpha;
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, renderAlpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::UI), 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		E::CB_MINIMAP minimapBuffer{};
		minimapBuffer.mapOffset = tMapOffset;
		minimapBuffer.mapRotation = tRotation;
		minimapBuffer.mapScale = tScale;
		minimapBuffer.mapMode = static_cast<uint32_t>(m_MiniMapProfile.Mode);
		minimapBuffer.smokeIntensity = m_MiniMapProfile.SmokeIntensity;
		minimapBuffer.smokeSpeed = m_MiniMapProfile.SmokeSpeed;
		minimapBuffer.smokeTime = m_fSmokeTime;
		minimapBuffer.battleZoneCount = m_iVisibleBattleZoneCount;
		for (uint32_t i = 0; i < m_iVisibleBattleZoneCount; ++i)
			minimapBuffer.battleZones[i] = m_BattleZoneShaderData[i];

		// 미니맵 전용 컴포넌트나 버퍼 오브젝트를 통해 MapDiscard 진행
		m_pMinimapCBuffer->MapDiscard(pContext, &minimapBuffer, sizeof(minimapBuffer));

		pContext->VSSetConstantBuffers(10, 1, m_pMinimapCBuffer->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(10, 1, m_pMinimapCBuffer->GetAdressOfBuffer());
	}

	{
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};
				cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(ETOUI(B_SLOTNUMBER::PER_OBJECT), 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& frameSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, "TEX_UI_T_HUD_MiniMap_Fade");
		const auto& minimapSrv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_MiniMapProfile.TextureTag);
		

		ID3D11ShaderResourceView* srvs[3] = {
			frameSrv->GetSRV().Get(),      // t0: 프레임
			minimapSrv->GetSRV().Get(),   // t1: 맵
			frameSrv->GetSRV().Get(),      // t2: battle-zone alpha mask
		};

		pContext->PSSetShaderResources(0, 3, srvs);
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);

	ID3D11Buffer* nullBuffer = nullptr;
	pContext->VSSetConstantBuffers(10, 1, &nullBuffer);
	pContext->PSSetConstantBuffers(10, 1, &nullBuffer);

	return S_OK;
}

void CMiniMap::SetMiniMapProfile(const MINIMAP_PROFILE& profile)
{
	m_MiniMapProfile = profile;
	m_UIINFO.Restag = profile.TextureTag;
	tScale = profile.MapScale;
	tMapOffset = {};
	m_bHasPreviousPlayerPosition = false;
}

void CMiniMap::AddBattleZone(const BATTLE_ZONE_INFO& battleZone)
{
	m_vBattleZones.push_back(battleZone);
}

void CMiniMap::AddObjective(MINIMAP_OBJECTIVE_INFO objective)
{
	InitializeObjectiveMarkers(objective);
	m_vObjectives.push_back(std::move(objective));
}

_bool CMiniMap::SetObjectiveActive(const std::string& key, _bool active)
{
	auto iter = std::find_if(
		m_vObjectives.begin(), m_vObjectives.end(),
		[&key](const MINIMAP_OBJECTIVE_INFO& objective)
		{
			return objective.Key == key;
		});

	if (iter == m_vObjectives.end())
		return false;

	iter->ManualActive = active;
	return true;
}

_bool CMiniMap::SetContentGroupActive(
	QUEST_UI_GROUP group, _bool active)
{
	if (group == QUEST_UI_GROUP::NONE ||
		group == QUEST_UI_GROUP::END)
		return false;

	_bool found = false;
	for (auto& battleZone : m_vBattleZones)
	{
		if (battleZone.Group != group)
			continue;

		battleZone.Enabled = active;
		found = true;
	}

	for (auto& objective : m_vObjectives)
	{
		if (objective.Group != group)
			continue;

		objective.Enabled = active;
		if (!active)
			objective.ProximityActive = false;
		found = true;
	}

	return found;
}

void CMiniMap::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	if (uiState & ETOUI(UI_STATE::APPEAR))
	{
		ClearEffectTweens();
		if (Appear) Appear(this);
	}

	if (uiState & ETOUI(UI_STATE::DISAPPEAR))
	{
		ClearEffectTweens();
		if (Disappear) Disappear(this);
	}

	if (m_bInputLocked)
		return;
}

void CMiniMap::SearchPlayerIcon()
{
	if (m_SearchPlayerIcon)
		return;

	for (auto pHandle : m_vChildren)
	{
		Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(pHandle);
		if (!pUI)
			continue;

		const UI_INFO& pInfo = pUI->GetUIInfo();
		if (pInfo.Restag == "TEX_UI_T_HUD_MiniMap_PlayerBlip")
		{
			m_hPlayerIcon = pHandle;
			m_SearchPlayerIcon = true;
			return;
		}
	}
}

void CMiniMap::SetPlayerIconRot(_float rot)
{
	Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(m_hPlayerIcon);
	if (!pUI)
	{
		m_SearchPlayerIcon = false;
		return;
	}

	pUI->SetLocalRot(rot);
	pUI->GetUIInfo().Rot = m_UIINFO.Rot + rot;
	pUI->CalcUICoord();
}

void CMiniMap::CalcDir()
{
	const _float cameraYaw = atan2f(m_cameraLook.x, m_cameraLook.z);
	const _float playerYaw = atan2f(m_playerLook.x, m_playerLook.z);

	// UI 좌표는 Y축이 아래를 향하므로 카메라 yaw와 같은 부호로 회전해야
	// 카메라 전방이 미니맵 위쪽을 향한다.
	m_UIINFO.Rot = XMConvertToDegrees(cameraYaw);
	CalcUICoord();

	// CUIObject combines child rotation as parent Rot + child LocalRot.
	SetPlayerIconRot(XMConvertToDegrees(-playerYaw));
}

void CMiniMap::InitializeMonsterMarkerPool()
{
	if (m_bMonsterMarkerPoolInitialized)
		return;

	m_bMonsterMarkerPoolInitialized = true;
	m_vMonsterMarkerHandles.reserve(MONSTER_MARKER_COUNT);

	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(
			E::CGameInstance::Get().GetCurrentLevelID())).data();

	for (size_t i = 0; i < MONSTER_MARKER_COUNT; ++i)
	{
		CTextureUI::UIOBJECT_DESC desc{};
		desc.sObjectTag = "MiniMap_MonsterMarker_" + std::to_string(i);
		desc.Name = desc.sObjectTag;
		desc.fX = m_UIINFO.fX;
		desc.fY = m_UIINFO.fY;
		desc.fSizeX = MONSTER_MARKER_SIZE;
		desc.fSizeY = MONSTER_MARKER_SIZE;
		desc.fAlpha = 0.f;
		desc.ResTag = "TEX_UI_T_MiniMap_AuthorityFigure";
		desc.ResWeight = m_UIINFO.Weight + 1;
		desc.UIType = ETOUI(UI_TYPE::TEXUI);

		auto markerHandle = E::CGameInstance::Get().AddGameObjectToLayer(
			currentLevel,
			"Prototype_GameObject_TextureUI",
			"Layer_UI",
			&desc);
		if (!markerHandle)
			continue;

		auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(*markerHandle);
		if (!pMarker)
			continue;

		pMarker->SetParent(GetHandle());
		pMarker->SetLocalPos({ 0.f, 0.f });
		pMarker->SetColor({ 1.f, 0.12f, 0.08f });
		pMarker->GetUIInfo().WeightOffset = 5;
		SetMonsterMarkerVisible(pMarker, false);

		AddChildren(*markerHandle);
		m_vMonsterMarkerHandles.push_back(*markerHandle);
	}
}

void CMiniMap::InitializeBattleZone()
{
	if (m_bBattleZoneInitialized)
		return;

	m_bBattleZoneInitialized = true;

	const uint32_t currentLevelID = E::CGameInstance::Get().GetCurrentLevelID();

	switch (currentLevelID)
	{
	case ETOUI(LEVEL::CHARLES_ROOKWOOD):
		InitRookwoodBattleZone();
		break;
	default:
		break;
	}
}

void CMiniMap::InitializeObjectives()
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	if (m_iObjectiveInitializedLevel == currentLevelID)
		return;

	HideObjectiveMarkers();
	m_vObjectives.clear();
	m_iObjectiveInitializedLevel = currentLevelID;

	switch (currentLevelID)
	{
	case ETOUI(LEVEL::CHARLES_ROOKWOOD):
		InitRookwoodObjectives();
		break;
	default:
		break;
	}
}

void CMiniMap::InitializeObjectiveMarkers(
	MINIMAP_OBJECTIVE_INFO& objective)
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	if (objective.LevelID != static_cast<uint32_t>(-1) &&
		objective.LevelID != currentLevelID)
		return;

	const std::string currentLevel = _string("LEVEL_") +
		MagicEnumToStringView(static_cast<LEVEL>(currentLevelID)).data();

	for (size_t i = 0; i < objective.VisualPhases.size(); ++i)
	{
		auto& phase = objective.VisualPhases[i];
		CTextureUI::UIOBJECT_DESC desc{};
		desc.sObjectTag = "MiniMap_Objective_" + objective.Key +
			"_" + std::to_string(i);
		desc.Name = desc.sObjectTag;
		desc.fX = m_UIINFO.fX;
		desc.fY = m_UIINFO.fY;
		desc.fSizeX = phase.IconSize;
		desc.fSizeY = phase.IconSize;
		desc.fAlpha = 0.f;
		desc.ResTag = phase.TextureTag;
		desc.ResWeight = m_UIINFO.Weight + phase.WeightOffset;
		desc.UIType = ETOUI(UI_TYPE::TEXUI);

		auto markerHandle = E::CGameInstance::Get().AddGameObjectToLayer(
			currentLevel,
			phase.PrototypeTag,
			"Layer_UI",
			&desc);
		if (!markerHandle)
			continue;

		auto* marker = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(*markerHandle);
		if (!marker)
			continue;

		phase.MarkerHandle = *markerHandle;
		marker->SetParent(GetHandle());
		marker->SetLocalPos({ 0.f, 0.f });
		marker->SetColor(phase.TintColor);
		marker->GetUIInfo().WeightOffset = phase.WeightOffset;
		marker->SetAlphaRatio(0.f);
		SetObjectiveMarkerVisible(marker, false);
		AddChildren(*markerHandle);

		if (!phase.ShowScreenMarker)
			continue;

		CTextureUI::UIOBJECT_DESC screenDesc{};
		screenDesc.sObjectTag = "Screen_Objective_" + objective.Key +
			"_" + std::to_string(i);
		screenDesc.Name = screenDesc.sObjectTag;
		screenDesc.fX = 0.f;
		screenDesc.fY = 0.f;
		screenDesc.fSizeX = phase.ScreenMarkerSize;
		screenDesc.fSizeY = phase.ScreenMarkerSize;
		screenDesc.fAlpha = 0.f;
		screenDesc.ResTag = phase.TextureTag;
		screenDesc.ResWeight = phase.ScreenMarkerWeight;
		screenDesc.UIType = ETOUI(UI_TYPE::TEXUI);

		auto screenMarkerHandle = E::CGameInstance::Get().
			AddGameObjectToLayer(
				currentLevel,
				phase.PrototypeTag,
				"Layer_UI",
				&screenDesc);
		if (!screenMarkerHandle)
			continue;

		auto* screenMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(*screenMarkerHandle);
		if (!screenMarker)
			continue;

		phase.ScreenMarkerHandle = *screenMarkerHandle;
		screenMarker->SetColor(phase.TintColor);
		SetObjectiveMarkerVisible(screenMarker, false);
	}
}

void CMiniMap::RefreshNearbyMonsters(E::CGameObject* pPlayer)
{
	m_vNearbyMonsterHandles.clear();
	if (!pPlayer || m_vMonsterMarkerHandles.empty())
		return;

	std::vector<E::PX_OVERLAP_RESULT> results{};
	const E::PX_OVERLAP_DESC overlapDesc{
		.tGeometry = {
			.eType = E::PX_QUERY_GEOMETRY_TYPE::SPHERE,
			.fRadius = MONSTER_DETECTION_RADIUS
		},
		.tPose = {
			.vPosition = pPlayer->GetTransform().GetPosition()
		},
		.tFilter = {
			.iQueryMask = ETOUI(COLLISION_LAYER::ENEMY_BODY)
		}
	};

	if (!E::CGameInstance::Get().GetPhysXManager()->
		OverlapMultiple(overlapDesc, results,
			static_cast<uint32_t>(MONSTER_MARKER_COUNT * 2)))
		return;

	for (const auto& result : results)
	{
		auto* pMonster = Cast<CMonster>(result.pGameObject);
		if (!pMonster || pMonster->GetPendingDestroy() ||
			pMonster->Get_CurrentHp() <= 0)
			continue;

		const CHandle monsterHandle = pMonster->GetHandle();
		if (std::find(m_vNearbyMonsterHandles.begin(),
			m_vNearbyMonsterHandles.end(), monsterHandle) !=
			m_vNearbyMonsterHandles.end())
			continue;

		m_vNearbyMonsterHandles.push_back(monsterHandle);
		if (m_vNearbyMonsterHandles.size() >=
			m_vMonsterMarkerHandles.size())
			break;
	}
}

void CMiniMap::UpdateMonsterMarkers(
	E::_float fTimeDelta, E::CGameObject* pPlayer)
{
	if (!m_bMonsterMarkerPoolInitialized || !pPlayer)
	{
		HideMonsterMarkers();
		return;
	}

	m_fMonsterSearchAcc += fTimeDelta;
	if (m_fMonsterSearchAcc >= MONSTER_SEARCH_INTERVAL)
	{
		m_fMonsterSearchAcc = 0.f;
		RefreshNearbyMonsters(pPlayer);
	}

	const _float3& playerPos = pPlayer->GetTransform().GetPosition();
	const _float mapRadius =
		0.5f * std::min(m_UIINFO.SizeX, m_UIINFO.SizeY);
	const _float markerRadius =
		0.5f * sqrtf(MONSTER_MARKER_SIZE * MONSTER_MARKER_SIZE * 2.f);
	const _float innerRadius = std::max(
		0.f, mapRadius - markerRadius - MINIMAP_BORDER_PADDING);
	const _float pixelPerWorldUnit =
		innerRadius / MONSTER_DETECTION_RADIUS;
	const _float detectionRadiusSq =
		MONSTER_DETECTION_RADIUS * MONSTER_DETECTION_RADIUS;

	for (size_t i = 0; i < m_vMonsterMarkerHandles.size(); ++i)
	{
		auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(m_vMonsterMarkerHandles[i]);
		if (!pMarker)
			continue;

		if (i >= m_vNearbyMonsterHandles.size())
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		auto* pMonster = E::CGameInstance::Get().
			GetGameObjectByHandleT<CMonster>(m_vNearbyMonsterHandles[i]);
		if (!pMonster || pMonster->GetPendingDestroy() ||
			pMonster->Get_CurrentHp() <= 0)
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		const _float3& monsterPos = pMonster->GetTransform().GetPosition();
		const _float dx = monsterPos.x - playerPos.x;
		const _float dz = monsterPos.z - playerPos.z;
		const _float distanceSq = dx * dx + dz * dz;

		if (distanceSq > detectionRadiusSq || innerRadius <= 0.f)
		{
			SetMonsterMarkerVisible(pMarker, false);
			continue;
		}

		pMarker->SetLocalPos({
			dx * pixelPerWorldUnit,
			-dz * pixelPerWorldUnit
		});

		XMVECTOR monsterLook = XMVectorSetY(
			pMonster->GetTransform().GetState(STATE::LOOK), 0.f);
		if (XMVectorGetX(XMVector3LengthSq(monsterLook)) > 0.000001f)
		{
			monsterLook = XMVector3Normalize(monsterLook);
			pMarker->SetLocalRot(XMConvertToDegrees(-atan2f(
				XMVectorGetX(monsterLook),
				XMVectorGetZ(monsterLook))));
		}

		SetMonsterMarkerVisible(pMarker, true);
	}
}

void CMiniMap::HideMonsterMarkers()
{
	for (const CHandle markerHandle : m_vMonsterMarkerHandles)
	{
		if (auto* pMarker = E::CGameInstance::Get().
			GetGameObjectByHandleT<CTextureUI>(markerHandle))
		{
			SetMonsterMarkerVisible(pMarker, false);
		}
	}
}

void CMiniMap::SetMonsterMarkerVisible(
	E::CUIObject* pMarker, _bool bVisible)
{
	if (!pMarker)
		return;

	pMarker->SetAlphaRatio(
		bVisible ? MINIMAP_ICON_ALPHA_RATIO : 0.f);
	pMarker->SetAlpha(bVisible ? m_UIINFO.Alpha : 0.f);
	pMarker->SetActive(bVisible);
}

void CMiniMap::UpdateObjectiveMarkers(
	_float fTimeDelta,
	const _float3& playerPosition,
	E::CCameraObject* camera)
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	const _float mapRadius =
		0.5f * std::min(m_UIINFO.SizeX, m_UIINFO.SizeY);
	const _float pixelPerWorldUnit =
		mapRadius / MONSTER_DETECTION_RADIUS;

	for (auto& objective : m_vObjectives)
	{
		const _float deltaX = objective.WorldPosition.x - playerPosition.x;
		const _float deltaZ = objective.WorldPosition.z - playerPosition.z;
		const _float distanceSq = deltaX * deltaX + deltaZ * deltaZ;
		OBJECTIVE_VISUAL_PHASE* selectedPhase = nullptr;
		const _bool isCurrentLevel =
			objective.LevelID == static_cast<uint32_t>(-1) ||
			objective.LevelID == currentLevelID;
		if (!objective.Enabled || !isCurrentLevel)
			objective.ProximityActive = false;

		if (objective.Enabled && isCurrentLevel &&
			IsObjectiveActive(objective, distanceSq))
		{
			const _float distance = sqrtf(distanceSq);
			for (auto& phase : objective.VisualPhases)
			{
				const _float hysteresis =
					std::max(0.f, phase.DistanceHysteresis);
				const _float minDistance = phase.DesiredVisible ?
					std::max(0.f, phase.MinDistance - hysteresis) :
					phase.MinDistance + hysteresis;
				const _float maxDistance = phase.DesiredVisible ?
					phase.MaxDistance + hysteresis :
					std::max(0.f, phase.MaxDistance - hysteresis);
				const _bool aboveMin = distance >= minDistance;
				const _bool belowMax = phase.MaxDistance <= 0.f ||
					distance < maxDistance;
				if (aboveMin && belowMax)
				{
					selectedPhase = &phase;
					break;
				}
			}
		}

		for (auto& phase : objective.VisualPhases)
		{
			const _bool isSelected = &phase == selectedPhase;
			SetObjectivePhaseVisible(
				phase, isSelected, fTimeDelta);

			const _float screenMarkerTargetAlpha =
				isSelected && phase.ShowScreenMarker ?
				UpdateScreenObjectiveMarkerPosition(
					phase, objective.WorldPosition, camera) :
				0.f;
			SetScreenObjectivePhaseVisible(
				phase, screenMarkerTargetAlpha, fTimeDelta);
		}

		// FadeOut 중에는 selectedPhase에서 제외되지만 Tween이 끝날 때까지
		// 아이콘은 활성 상태이므로 미니맵 회전 상쇄를 계속 갱신한다.
		for (auto& phase : objective.VisualPhases)
		{
			auto* phaseMarker = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(phase.MarkerHandle);
			if (!phaseMarker || !phaseMarker->GetActive())
				continue;

			phaseMarker->SetLocalRot(-m_UIINFO.Rot);
			phaseMarker->GetUIInfo().Rot = 0.f;
			phaseMarker->CalcUICoord();
		}

		if (!selectedPhase)
			continue;

		auto* marker = E::CGameInstance::Get().
			GetGameObjectByHandleT<E::CUIObject>(
				selectedPhase->MarkerHandle);
		if (!marker)
			continue;

		_float2 localPosition{
			deltaX * pixelPerWorldUnit,
			-deltaZ * pixelPerWorldUnit
		};
		const _float markerRadius =
			0.5f * sqrtf(selectedPhase->IconSize *
				selectedPhase->IconSize * 2.f);
		const _float innerRadius = std::max(
			0.f, mapRadius - markerRadius - MINIMAP_BORDER_PADDING);
		const _float positionLength = sqrtf(
			localPosition.x * localPosition.x +
			localPosition.y * localPosition.y);
		if (positionLength > innerRadius && positionLength > 0.000001f)
		{
			const _float clampRatio = innerRadius / positionLength;
			localPosition.x *= clampRatio;
			localPosition.y *= clampRatio;
		}

		marker->SetLocalPos(localPosition);
		marker->SetLocalRot(-m_UIINFO.Rot);
		marker->GetUIInfo().Rot = 0.f;
		marker->CalcUICoord();
	}
}

void CMiniMap::HideObjectiveMarkers()
{
	for (auto& objective : m_vObjectives)
	{
		objective.ProximityActive = false;
		for (auto& phase : objective.VisualPhases)
		{
			phase.DesiredVisible = false;
			phase.MarkerAlphaRatio = 0.f;
			if (auto* marker = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(phase.MarkerHandle))
			{
				SetObjectiveMarkerVisible(marker, false);
			}

			phase.ScreenMarkerDesiredVisible = false;
			phase.ScreenMarkerAlpha = 0.f;
			if (auto* screenMarker = E::CGameInstance::Get().
				GetGameObjectByHandleT<E::CUIObject>(
					phase.ScreenMarkerHandle))
			{
				SetObjectiveMarkerVisible(screenMarker, false);
			}
		}
	}
}

_bool CMiniMap::IsObjectiveActive(
	MINIMAP_OBJECTIVE_INFO& objective,
	_float distanceSq) const
{
	if (objective.AutoActivateDistance > 0.f)
	{
		const _float exitDistance = objective.AutoActivateDistance +
			std::max(0.f, objective.ActivationHysteresis);
		const _float threshold = objective.ProximityActive ?
			exitDistance : objective.AutoActivateDistance;
		objective.ProximityActive = distanceSq <= threshold * threshold;
	}
	else
	{
		objective.ProximityActive = false;
	}

	const _bool isNear = objective.ProximityActive;

	switch (objective.ActiveRule)
	{
	case OBJECTIVE_ACTIVE_RULE::MANUAL:
		return objective.ManualActive;
	case OBJECTIVE_ACTIVE_RULE::PROXIMITY:
		return isNear;
	case OBJECTIVE_ACTIVE_RULE::MANUAL_OR_PROXIMITY:
		return objective.ManualActive || isNear;
	case OBJECTIVE_ACTIVE_RULE::MANUAL_AND_PROXIMITY:
		return objective.ManualActive && isNear;
	default:
		return false;
	}
}

void CMiniMap::SetObjectiveMarkerVisible(
	E::CUIObject* marker, _bool visible)
{
	if (!marker)
		return;
	if (auto* tween = marker->GetTweenCom())
		tween->ClearTweens();

	marker->SetAlphaRatio(
		visible ? MINIMAP_ICON_ALPHA_RATIO : 0.f);
	marker->SetAlpha(visible ? 1.f : 0.f);
	marker->SetActive(visible);
}

void CMiniMap::SetObjectivePhaseVisible(
	OBJECTIVE_VISUAL_PHASE& phase, _bool visible,
	_float fTimeDelta)
{
	auto* marker = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(phase.MarkerHandle);
	if (!marker)
		return;

	phase.DesiredVisible = visible;

	const _float targetRatio = visible ?
		MINIMAP_ICON_ALPHA_RATIO : 0.f;
	const _float fadeTime = visible ?
		phase.FadeInTime : phase.FadeOutTime;
	const _float ratioStep = fadeTime > 0.f ?
		MINIMAP_ICON_ALPHA_RATIO *
		std::max(0.f, fTimeDelta) / fadeTime :
		MINIMAP_ICON_ALPHA_RATIO;

	if (visible)
	{
		marker->SetActive(true);
		phase.MarkerAlphaRatio = std::min(
			targetRatio, phase.MarkerAlphaRatio + ratioStep);
	}
	else
	{
		phase.MarkerAlphaRatio = std::max(
			targetRatio, phase.MarkerAlphaRatio - ratioStep);
	}

	marker->SetAlphaRatio(phase.MarkerAlphaRatio);
	marker->SetAlpha(m_UIINFO.Alpha * phase.MarkerAlphaRatio);

	if (!visible && phase.MarkerAlphaRatio <= 0.f)
		marker->SetActive(false);
}

void CMiniMap::SetScreenObjectivePhaseVisible(
	OBJECTIVE_VISUAL_PHASE& phase, _float targetAlpha,
	_float fTimeDelta)
{
	if (!phase.ShowScreenMarker)
		return;

	auto* marker = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(
			phase.ScreenMarkerHandle);
	if (!marker)
		return;

	targetAlpha = std::clamp(targetAlpha, 0.f, 1.f);
	const _bool visible = targetAlpha > 0.f;
	phase.ScreenMarkerDesiredVisible = visible;

	const _bool increasing =
		phase.ScreenMarkerAlpha < targetAlpha;
	const _float fadeTime = increasing ?
		phase.FadeInTime : phase.FadeOutTime;
	const _float alphaStep = fadeTime > 0.f ?
		std::max(0.f, fTimeDelta) / fadeTime : 1.f;

	if (visible)
		marker->SetActive(true);

	if (increasing)
	{
		phase.ScreenMarkerAlpha = std::min(
			targetAlpha, phase.ScreenMarkerAlpha + alphaStep);
	}
	else
	{
		phase.ScreenMarkerAlpha = std::max(
			targetAlpha, phase.ScreenMarkerAlpha - alphaStep);
	}

	marker->SetAlphaRatio(1.f);
	marker->SetAlpha(phase.ScreenMarkerAlpha);

	if (!visible && phase.ScreenMarkerAlpha <= 0.f)
		marker->SetActive(false);
}

_float CMiniMap::UpdateScreenObjectiveMarkerPosition(
	OBJECTIVE_VISUAL_PHASE& phase,
	const _float3& worldPosition,
	E::CCameraObject* camera)
{
	if (!camera)
		return 0.f;

	auto* marker = E::CGameInstance::Get().
		GetGameObjectByHandleT<E::CUIObject>(
			phase.ScreenMarkerHandle);
	if (!marker)
		return 0.f;

	const _float3 markerWorldPosition{
		worldPosition.x + phase.ScreenMarkerWorldOffset.x,
		worldPosition.y + phase.ScreenMarkerWorldOffset.y,
		worldPosition.z + phase.ScreenMarkerWorldOffset.z
	};
	const _matrix view = camera->GetView();
	const _matrix projection = camera->GetProj();
	const _vector clipPosition = XMVector4Transform(
		XMVectorSet(
			markerWorldPosition.x,
			markerWorldPosition.y,
			markerWorldPosition.z,
			1.f),
		view * projection);
	const _float2 screenSize =
		E::CGameInstance::Get().GetClientScreenSize();
	if (screenSize.x <= 0.f || screenSize.y <= 0.f)
		return 0.f;

	const _vector projected = XMVector3Project(
		XMLoadFloat3(&markerWorldPosition),
		0.f,
		0.f,
		screenSize.x,
		screenSize.y,
		0.f,
		1.f,
		projection,
		view,
		XMMatrixIdentity());

	_float3 screenPosition{};
	XMStoreFloat3(&screenPosition, projected);
	const _float halfSize = phase.ScreenMarkerSize * 0.5f;
	const _bool isOnScreen =
		screenPosition.z >= 0.f && screenPosition.z <= 1.f &&
		screenPosition.x >= halfSize &&
		screenPosition.x <= screenSize.x - halfSize &&
		screenPosition.y >= halfSize &&
		screenPosition.y <= screenSize.y - halfSize;
	if (isOnScreen && XMVectorGetW(clipPosition) > 0.f)
	{
		marker->SetPos({ screenPosition.x, screenPosition.y });
		marker->CalcUICoord();
		return 1.f;
	}

	const _float clipW = XMVectorGetW(clipPosition);
	const _float safeW = std::max(fabsf(clipW), 0.0001f);
	_float ndcX = XMVectorGetX(clipPosition) / safeW;
	_float ndcY = XMVectorGetY(clipPosition) / safeW;

	// abs(w)를 사용하면 카메라 뒤쪽에서도 목표의 좌우 방향을 유지한다.

	_float2 screenDirection{
		ndcX * screenSize.x * 0.5f,
		-ndcY * screenSize.y * 0.5f
	};
	if (fabsf(screenDirection.x) < 0.0001f &&
		fabsf(screenDirection.y) < 0.0001f)
	{
		screenDirection.y = screenSize.y * 0.5f;
	}

	const _float edgePadding = std::max(
		0.f, phase.ScreenMarkerEdgePadding);
	const _float edgeHalfWidth = std::max(
		0.f, screenSize.x * 0.5f - halfSize - edgePadding);
	const _float edgeHalfHeight = std::max(
		0.f, screenSize.y * 0.5f - halfSize - edgePadding);
	const _float scaleX = fabsf(screenDirection.x) > 0.0001f ?
		edgeHalfWidth / fabsf(screenDirection.x) : FLT_MAX;
	const _float scaleY = fabsf(screenDirection.y) > 0.0001f ?
		edgeHalfHeight / fabsf(screenDirection.y) : FLT_MAX;
	const _float edgeScale = std::min(scaleX, scaleY);

	marker->SetPos({
		screenSize.x * 0.5f + screenDirection.x * edgeScale,
		screenSize.y * 0.5f + screenDirection.y * edgeScale
	});
	marker->CalcUICoord();
	return std::clamp(
		phase.ScreenMarkerOffscreenAlpha, 0.f, 1.f);
}

void CMiniMap::ConfigureDefaultProfile()
{
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	if (m_iConfiguredLevel == currentLevelID)
		return;

	m_iConfiguredLevel = currentLevelID;
	MINIMAP_PROFILE profile{};
	profile.Mode = MINIMAP_MODE::DUNGEON_FOG;
	profile.TextureTag = "TEX_UI_T_MiniMapSmoke";
	profile.MapScale = 1.f;
	profile.SmokeIntensity = 0.9f;
	profile.SmokeSpeed = 8.f;
	profile.FogAlphaMultiplier = 1.75f;
	profile.PlayerScrollScale = 0.015f;

	SetMiniMapProfile(profile);
}

void CMiniMap::UpdateWorldMapOffset(const _float3& playerPosition)
{
	if (m_MiniMapProfile.Mode != MINIMAP_MODE::WORLD_MAP)
	{
		return;
	}

	const _float worldWidth =
		m_MiniMapProfile.WorldMaxXZ.x - m_MiniMapProfile.WorldMinXZ.x;
	const _float worldDepth =
		m_MiniMapProfile.WorldMaxXZ.y - m_MiniMapProfile.WorldMinXZ.y;
	if (fabsf(worldWidth) <= 0.000001f ||
		fabsf(worldDepth) <= 0.000001f)
	{
		tMapOffset = {};
		return;
	}

	const _float normalizedX = std::clamp(
		(playerPosition.x - m_MiniMapProfile.WorldMinXZ.x) / worldWidth,
		0.f, 1.f);
	const _float normalizedZ = std::clamp(
		(playerPosition.z - m_MiniMapProfile.WorldMinXZ.y) / worldDepth,
		0.f, 1.f);

	const _float mapU = m_MiniMapProfile.UVMin.x +
		(m_MiniMapProfile.UVMax.x - m_MiniMapProfile.UVMin.x) * normalizedX;
	const _float mapV = m_MiniMapProfile.UVMax.y +
		(m_MiniMapProfile.UVMin.y - m_MiniMapProfile.UVMax.y) * normalizedZ;

	tMapOffset = { mapU - 0.5f, mapV - 0.5f };
}

void CMiniMap::UpdateFogMovementOffset(const _float3& playerPosition)
{
	if (m_MiniMapProfile.Mode != MINIMAP_MODE::DUNGEON_FOG)
	{
		m_bHasPreviousPlayerPosition = false;
		return;
	}

	if (!m_bHasPreviousPlayerPosition)
	{
		m_vPreviousPlayerPosition = playerPosition;
		m_bHasPreviousPlayerPosition = true;
		return;
	}

	const _float deltaX = playerPosition.x - m_vPreviousPlayerPosition.x;
	const _float deltaZ = playerPosition.z - m_vPreviousPlayerPosition.z;
	m_vPreviousPlayerPosition = playerPosition;

	// Ignore teleports and large level-transition position changes.
	if (deltaX * deltaX + deltaZ * deltaZ > 100.f)
		return;

	tMapOffset.x = fmodf(tMapOffset.x +
		deltaX * m_MiniMapProfile.PlayerScrollScale, 1.f);
	tMapOffset.y = fmodf(tMapOffset.y -
		deltaZ * m_MiniMapProfile.PlayerScrollScale, 1.f);
}

void CMiniMap::UpdateBattleZones(const _float3& playerPosition)
{
	m_iVisibleBattleZoneCount = 0;
	m_BattleZoneShaderData.fill({});

	const _float mapSize = std::min(m_UIINFO.SizeX, m_UIINFO.SizeY);
	if (mapSize <= 0.000001f)
		return;

	const _float mapRadius = mapSize * 0.5f;
	const _float innerRadius = std::max(
		0.f, mapRadius - MINIMAP_BORDER_PADDING);
	if (innerRadius <= 0.f)
		return;

	const _float pixelPerWorldUnit =
		innerRadius / MONSTER_DETECTION_RADIUS;
	const uint32_t currentLevelID =
		E::CGameInstance::Get().GetCurrentLevelID();
	const uint32_t allLevels = static_cast<uint32_t>(-1);

	for (const BATTLE_ZONE_INFO& battleZone : m_vBattleZones)
	{
		if (m_iVisibleBattleZoneCount >= MAX_BATTLE_ZONE_COUNT)
			break;
		if (!battleZone.Enabled)
			continue;
		if (battleZone.LevelID != allLevels &&
			battleZone.LevelID != currentLevelID)
			continue;

		const _float deltaX = battleZone.Center.x - playerPosition.x;
		const _float deltaZ = battleZone.Center.z - playerPosition.z;
		const _float visibleDistanceSq =
			battleZone.VisibleDistance * battleZone.VisibleDistance;
		if (deltaX * deltaX + deltaZ * deltaZ > visibleDistanceSq)
			continue;

		const _float localX = deltaX * pixelPerWorldUnit;
		const _float localY = -deltaZ * pixelPerWorldUnit;
		const _float centerU = 0.5f + localX / m_UIINFO.SizeX;
		const _float centerV = 0.5f + localY / m_UIINFO.SizeY;
		const _float diameterUV = std::max(
			0.001f,
			battleZone.WorldRadius * 2.f * pixelPerWorldUnit / mapSize);

		m_BattleZoneShaderData[m_iVisibleBattleZoneCount++] = {
			centerU,
			centerV,
			diameterUV,
			std::clamp(battleZone.Alpha, 0.f, 1.f)
		};
	}
}

void CMiniMap::InitRookwoodBattleZone()
{
	BATTLE_ZONE_INFO trial01{};
	trial01.Group = QUEST_UI_GROUP::ROOKWOOD_TRIAL_01;
	trial01.Enabled = false;
	trial01.Center = { -178.f, 0.f, 160.f };
	trial01.WorldRadius = 40.f;
	trial01.VisibleDistance = 60.f;
	trial01.Alpha = 0.25f;
	trial01.LevelID = ETOUI(LEVEL::CHARLES_ROOKWOOD);
	AddBattleZone(trial01);

	BATTLE_ZONE_INFO trial02{};
	trial02.Group = QUEST_UI_GROUP::ROOKWOOD_TRIAL_02;
	trial02.Enabled = false;
	trial02.Center = { -252.f, 0.f, -120.f };
	trial02.WorldRadius = 20.f;
	trial02.VisibleDistance = 60.f;
	trial02.Alpha = 0.25f;
	trial02.LevelID = ETOUI(LEVEL::CHARLES_ROOKWOOD);
	AddBattleZone(trial02);

	BATTLE_ZONE_INFO trial03{};
	trial03.Group = QUEST_UI_GROUP::ROOKWOOD_TRIAL_03;
	trial03.Enabled = false;
	trial03.Center = { -254.f, 0.f, -210.f };
	trial03.WorldRadius = 30.f;
	trial03.VisibleDistance = 60.f;
	trial03.Alpha = 0.25f;
	trial03.LevelID = ETOUI(LEVEL::CHARLES_ROOKWOOD);
	AddBattleZone(trial03);
}

void CMiniMap::InitBossRookwoodBattleZone()
{
}

void CMiniMap::InitRookwoodObjectives()
{
	MINIMAP_OBJECTIVE_INFO objective{};
	objective.Group = QUEST_UI_GROUP::ROOKWOOD_TRIAL_01;
	objective.Enabled = false;
	objective.Key = "Rookwood_MainMission_Test";
	objective.WorldPosition = { -178.f, -226.f, 160.f };
	objective.LevelID = ETOUI(LEVEL::CHARLES_ROOKWOOD);
	objective.ActiveRule = OBJECTIVE_ACTIVE_RULE::PROXIMITY;
	objective.AutoActivateDistance = 150.f;
	objective.ActivationHysteresis = 5.f;
	objective.VisualPhases.push_back({
		.MinDistance = 10.f,
		.MaxDistance = 0.f,
		.TextureTag = "TEX_UI_T_MiniMap_MainMission",
		.PrototypeTag = "Prototype_GameObject_TextureUI",
		.IconSize = 36.f,
		.TintColor = { 1.f, 0.78f, 0.04f },
		.DistanceHysteresis = 1.f,
		.ShowScreenMarker = true,
		.ScreenMarkerSize = 42.f,
		.ScreenMarkerWorldOffset = { 0.f, 2.5f, 0.f },
		.ScreenMarkerWeight = 100
	});

	AddObjective(std::move(objective));
}

CUIObject* CMiniMap::SafeGetOBJ(CHandle pHandle)
{
	if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle))
		return E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(pHandle);

	return nullptr;
}

void CMiniMap::PlayFadeIn(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	if (!pBtn)
		return;
	auto pTween = pBtn->GetTweenCom();
	if (!pTween)
		return;

	//pBtn->SetInputLcok(true);

	// 미니맵 마커는 자식 UI라서 Alpha가 매 프레임
	// ParentAlpha * AlphaRatio로 다시 계산된다.
	// 따라서 Alpha가 아니라 AlphaRatio를 보간해야 첫 표시도 정상적으로 페이드된다.
	pBtn->SetAlphaRatio(0.f);
	pBtn->SetAlpha(0.f);

	pTween->PlayTween(0.f, MINIMAP_ICON_ALPHA_RATIO, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlphaRatio(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void CMiniMap::PlayFadeOut(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	if (!pBtn)
		return;
	auto pTween = pBtn->GetTweenCom();
	if (!pTween)
		return;

	pBtn->SetInputLcok(true);

	const _float originAlphaRatio = pBtn->GetAlphaRatio();

	pTween->PlayTween(originAlphaRatio, 0.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlphaRatio(currentValue);
		}, [pHandle]() {
			if (auto* marker = E::CGameInstance::Get().
				GetGameObjectByHandleT<CUIObject>(pHandle))
			{
				marker->SetAlpha(0.f);
				marker->SetAlphaRatio(0.f);
				marker->SetActive(false);
			}
		}, EEaseType::EaseOutQuad, delay);
}

E::UPtr<CMiniMap> CMiniMap::Create()
{
	auto pInstance = E::ToUPtr(new CMiniMap{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CMiniMap");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CMiniMap::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CMiniMap{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMiniMap");
		return nullptr;
	}

	return pInstance;
}
