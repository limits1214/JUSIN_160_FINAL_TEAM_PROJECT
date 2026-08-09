#pragma once
#include "Engine_Defines.h"
#include "GameObject.h"
#include "Handle.h"
#include "Slot.h"
#include <span>
#include <unordered_set>

NS_BEGIN(Engine)

class ENGINE_DLL CGameObjectManager final : public CEngineBase
{
public:
	CGameObjectManager(const CGameObjectManager&) = delete;
	CGameObjectManager& operator=(const CGameObjectManager& rhs) = delete;

private:
	CGameObjectManager();
	~CGameObjectManager() override;

private:
	HRESULT Initialize();

public:
	void UpdateGUI();
	void UpdateGUIDrawTreeNode( CGameObject* handle);
private:
	char m_GUISearchFilter[256] = {};
	_bool m_bGUIEnableSearchInput{ false };
	_bool m_bGUIShowInvalidLayerHandles{ false };
	bool MatchesGUIFilter(std::string_view sText) const;
	bool MatchesLayerObjectFilter(std::string_view sLayerName, CGameObject* pObj) const;
	std::string GetGameObjectDebugLabel(CGameObject* pObj) const;
	std::string GetInvalidLayerHandleDebugText(const CHandle& handle) const;
	bool MatchesFilter(CGameObject* pObj) const;

public:
	void FrameStart();
	void FrameEnd();

public:
	std::optional<CHandle> GetFreeHandle() ;

public:
	CGameObject* GetGameObjectByHandle(const CHandle& handle) { return const_cast<CGameObject*>(_GetGameObjectByHandle(handle)); }
	const CGameObject* GetGameObjectByHandle(const CHandle& handle) const { return _GetGameObjectByHandle(handle); }
	template<typename T> T* GetGameObjectByHandleT(const CHandle& handle);
	template<typename T> const T* GetGameObjectByHandleT(const CHandle& handle) const;
	std::optional<CHandle> GetHandleByGameObject(CGameObject* pObj) const;
private:
	const CGameObject* _GetGameObjectByHandle(const CHandle& handle ) const;

public:
	std::optional<CHandle> AddGameObjectToLayer(const StringID& siProtoGroupTag, const StringID& siPrototypeTag, std::string_view sLayerName, void* pArg);
	const std::vector<CHandle>* GetLayer(std::string_view sLayerName) const;
	const std::vector<CHandle>* GetLayer(std::string_view sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg) ;
	const std::vector<std::pair<std::string, std::vector<CHandle>>>& GetLayers() const { return m_Layers; }
	template<typename T> T* GetFirstGameObjectByLayer(std::string_view sLayerName);
	void DelLayer(std::string_view sLayerName);

public:
	void AllReset();
	size_t ResetObjectsInLayers(std::span<const std::string_view> layerNames);
	size_t ResetAllObjectsExceptLayers(std::span<const std::string_view> excludedLayerNames);

private:
	enum class RESET_LAYER_MODE : uint8_t
	{
		INCLUDE_ONLY,
		EXCLUDE
	};

	// [LSY] 레이어 이름과 모드에 따라 일괄 제거할 오브젝트를 PendingDestroy 상태로 예약한다.
	// INCLUDE_ONLY는 전달한 레이어만 제거하고, EXCLUDE는 전달한 레이어를 제외한 나머지를 제거한다.
	// 존재하지 않는 레이어 이름은 무시하며, 반환값은 이번 호출에서 새로 제거 예약된 오브젝트 수다.
	// 실제 오브젝트 해제와 레이어 컨테이너 정리는 안전한 FrameEnd 시점에 일괄 처리한다.
	size_t RequestResetByLayers(
		std::span<const std::string_view> layerNames,
		RESET_LAYER_MODE eMode);
	_bool DestroyAllPendingObjects();
	_bool DestroyPendingObjectsInTree();
	void RemoveInvalidLayerHandles();
	_bool RemoveEmptyResetTargetLayers(
		const std::unordered_set<std::string>& resetTargetLayers);

	_bool m_bBatchResetPending{ false };
	std::unordered_set<std::string> m_PendingResetTargetLayers{};

public:
	void FixedUpdate(_float fTimeDelta);
	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

private:
	std::vector<CSlot<CGameObject>> m_Objects{};
	std::vector<size_t> m_FreeSlots{};

private:
	struct StringHash {
		using is_transparent = void;  // 이게 핵심

		size_t operator()(std::string_view sv) const {
			return std::hash<std::string_view>{}(sv);
		}
	};

	struct StringEqual {
		using is_transparent = void;  // 이것도 필요

		bool operator()(std::string_view a, std::string_view b) const {
			return a == b;
		}
	};

	std::vector<std::pair<std::string, std::vector<CHandle>>> m_Layers{};
	std::unordered_map<std::string, size_t, StringHash, StringEqual> m_LookupLayers{};
	void SortLayer();

private:
	std::vector<CGameObject*> m_TreePreparation{};
	std::vector<CGameObject*> m_Tree{};
	std::vector<CGameObject*> m_DFSReserved{};
	_bool m_bTreeReBuild{ true };


public:
	static UPtr< CGameObjectManager> Create();

public:
	void Free() override;
};

NS_END

inline const Engine::CGameObject* Engine::CGameObjectManager::_GetGameObjectByHandle(const CHandle& handle) const
{
	size_t idx = handle.GetIndex();
	if (idx >= m_Objects.size())
	{
		return nullptr;
	}

	const auto& slot = m_Objects[idx];
	if (!slot.IsOccupied())
	{
		return nullptr;
	}
	if (slot.GetGeneration() != handle.GetGeneration())
	{
		return nullptr;
	}

	return slot.Get();
}

template<typename T>
inline T* Engine::CGameObjectManager::GetGameObjectByHandleT(const CHandle& handle)
{
	const CGameObject* obj = _GetGameObjectByHandle(handle);

	if (!obj)
	{
		return nullptr;
	}

	
	if (!obj->Is<T>())
	{
		return nullptr;
	}
	
	return const_cast<T*>(static_cast<const T*>(obj));
}


template<typename T>
inline const T* Engine::CGameObjectManager::GetGameObjectByHandleT(const CHandle& handle) const
{
	const CGameObject* obj = _GetGameObjectByHandle(handle);

	if (!obj)
	{
		return nullptr;
	}

	if (!obj->Is<T>())
	{
		return nullptr;
	}

	return static_cast<const T*>(obj);
}

template<typename T>
inline T* Engine::CGameObjectManager::GetFirstGameObjectByLayer(std::string_view sLayerName)
{
	auto* pLayer = GetLayer(sLayerName);
	if (pLayer->empty())
	{
		return nullptr;
	}
	T* pObj = GetGameObjectByHandleT<T>(pLayer->front());
	if (!pObj)
	{
		return nullptr;
	}
	return pObj;
}
