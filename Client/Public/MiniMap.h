#pragma once

#include "UITex.h"
#include "Client_Defines.h"
#include "UI_Structs.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
class CCameraObject;
NS_END

NS_BEGIN(Client)

class CMiniMap final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CMiniMap, E::CUITex)

private:
	CMiniMap();
	~CMiniMap() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	void SetMiniMapProfile(const MINIMAP_PROFILE& profile);
	void AddBattleZone(const BATTLE_ZONE_INFO& battleZone);
	void AddObjective(MINIMAP_OBJECTIVE_INFO objective);
	_bool SetObjectiveActive(const std::string& key, _bool active);
	_bool SetContentGroupActive(QUEST_UI_GROUP group, _bool active);

private:
	virtual void PlayEffect(uint32_t uiState);

private:
	CComConstantBuffer* m_pComCBufferPerUI = nullptr;
	CComConstantBuffer* m_pMinimapCBuffer = nullptr;
	CButtonComponent* m_pComCButton = nullptr;

	_float3 m_cameraLook{0.f, 0.f, 1.f};
	_float3 m_playerLook{1.f, 0.f, 1.f};

	_float2 tMapOffset{};
	_float tRotation{ 0.f};
	_float tScale{0.6f};
	_float m_fSmokeTime{};
	_float3 m_vPreviousPlayerPosition{};
	_bool m_bHasPreviousPlayerPosition{ false };
	MINIMAP_PROFILE m_MiniMapProfile{};
	uint32_t m_iConfiguredLevel{ static_cast<uint32_t>(-1) };
	uint32_t m_iObjectiveInitializedLevel{ static_cast<uint32_t>(-1) };

	_bool m_SearchPlayerIcon{false};
	CHandle m_hPlayerIcon{};

	static constexpr size_t MONSTER_MARKER_COUNT = 20;
	static constexpr _float MONSTER_MARKER_SIZE = 18.f;
	static constexpr _float MONSTER_DETECTION_RADIUS = 60.f;
	static constexpr _float MONSTER_SEARCH_INTERVAL = 0.1f;
	static constexpr _float MINIMAP_BORDER_PADDING = 4.3f;
	static constexpr _float MINIMAP_ICON_ALPHA_RATIO = 2.5f;
	static constexpr size_t MAX_BATTLE_ZONE_COUNT = 8;

	_bool m_bMonsterMarkerPoolInitialized{ false };
	_bool m_bBattleZoneInitialized{ false };
	_float m_fMonsterSearchAcc{ MONSTER_SEARCH_INTERVAL };
	std::vector<CHandle> m_vMonsterMarkerHandles;
	std::vector<CHandle> m_vNearbyMonsterHandles;
	std::vector<BATTLE_ZONE_INFO> m_vBattleZones{};
	std::array<_float4, MAX_BATTLE_ZONE_COUNT> m_BattleZoneShaderData{};
	uint32_t m_iVisibleBattleZoneCount{};
	std::vector<MINIMAP_OBJECTIVE_INFO> m_vObjectives;

private:
	void SearchPlayerIcon();
	void SetPlayerIconRot(_float rot);
	void CalcDir();
	void InitializeMonsterMarkerPool();
	void InitializeBattleZone();
	void InitializeObjectives();
	void InitializeObjectiveMarkers(MINIMAP_OBJECTIVE_INFO& objective);
	void RefreshNearbyMonsters(E::CGameObject* pPlayer);
	void UpdateMonsterMarkers(E::_float fTimeDelta, E::CGameObject* pPlayer);
	void HideMonsterMarkers();
	void SetMonsterMarkerVisible(E::CUIObject* pMarker, _bool bVisible);
	void ConfigureDefaultProfile();
	void UpdateWorldMapOffset(const _float3& playerPosition);
	void UpdateFogMovementOffset(const _float3& playerPosition);
	void UpdateBattleZones(const _float3& playerPosition);
	void UpdateObjectiveMarkers(
		_float fTimeDelta,
		const _float3& playerPosition,
		E::CCameraObject* camera);
	void HideObjectiveMarkers();
	_bool IsObjectiveActive(MINIMAP_OBJECTIVE_INFO& objective,
		_float distanceSq) const;
	void SetObjectiveMarkerVisible(E::CUIObject* marker, _bool visible);
	void SetObjectivePhaseVisible(
		OBJECTIVE_VISUAL_PHASE& phase, _bool visible,
		_float fTimeDelta);
	void SetScreenObjectivePhaseVisible(
		OBJECTIVE_VISUAL_PHASE& phase, _float targetAlpha,
		_float fTimeDelta);
	_float UpdateScreenObjectiveMarkerPosition(
		OBJECTIVE_VISUAL_PHASE& phase,
		const _float3& worldPosition,
		E::CCameraObject* camera);

	void InitRookwoodBattleZone();
	void InitBossRookwoodBattleZone();
	void InitRookwoodObjectives();

private:
	CUIObject* SafeGetOBJ(CHandle pHandle);
	void PlayFadeIn(CHandle pHandle, float delay = 0.f, float playtime = 5.f);
	void PlayFadeOut(CHandle pHandle, float delay = 0.f, float playtime = 5.f);
public:
	static E::UPtr<CMiniMap> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
