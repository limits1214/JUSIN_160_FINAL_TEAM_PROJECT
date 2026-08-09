#pragma once

#include "Engine_Defines.h"
#include "Handle.h"
#include <deque>

NS_BEGIN(Engine)

class CGameObjectManager;

class ENGINE_DLL CGameObjectPoolManager final : public CEngineBase
{
public:
	enum class EXHAUST_POLICY : uint8_t
	{
		GROW,
		FAIL,
		RECYCLE_OLDEST
	};

	struct POOL_DESC
	{
		size_t iPrewarmCount{ 8 };
		size_t iGrowCount{ 4 };
		size_t iMaxCount{ 32 }; // 0이면 최대 개수 제한 없음
		EXHAUST_POLICY eExhaustPolicy{ EXHAUST_POLICY::GROW };
		std::function<std::optional<CHandle>()> fnCreate{};
	};

private:
	CGameObjectPoolManager();
	~CGameObjectPoolManager() override;

public:
	CGameObjectPoolManager(const CGameObjectPoolManager&) = delete;
	CGameObjectPoolManager& operator=(const CGameObjectPoolManager&) = delete;

public:
	_bool RegisterPool(const StringID& siPoolKey, POOL_DESC tDesc);
	_bool UnregisterPool(const StringID& siPoolKey);
	std::optional<CHandle> Acquire(
		const StringID& siPoolKey,
		void* pArg = nullptr);
	_bool Release(const CHandle& hObject);
	size_t ReleaseAll(const StringID& siPoolKey);
	void ClearAllPools();
	void FrameEnd();
	void UpdateGUI();

	_bool HasPool(const StringID& siPoolKey) const;
	size_t GetTotalCount(const StringID& siPoolKey) const;
	size_t GetActiveCount(const StringID& siPoolKey) const;
	size_t GetAvailableCount(const StringID& siPoolKey) const;

private:
	HRESULT Initialize(CGameObjectManager* pGameObjectManager);

	struct HANDLE_HASH
	{
		size_t operator()(const CHandle& hHandle) const noexcept;
	};

	struct HANDLE_STATE
	{
		StringID siPoolKey{};
		_bool bActive{};
	};

	struct POOL
	{
		POOL_DESC tDesc{};
		std::vector<CHandle> AvailableHandles{};
		std::deque<CHandle> ActiveHandles{};
		size_t iTotalCount{};
	};

	using POOL_CONTAINER = std::unordered_map<StringID, POOL>;
	using HANDLE_STATE_CONTAINER =
		std::unordered_map<CHandle, HANDLE_STATE, HANDLE_HASH>;

	size_t GrowPool(
		const StringID& siPoolKey,
		POOL& tPool,
		size_t iRequestedCount);
	void RemoveInvalidAvailableHandles(
		const StringID& siPoolKey,
		POOL& tPool);
	void RemoveHandleRecord(
		const CHandle& hObject,
		POOL& tPool);

private:
	CGameObjectManager* m_pGameObjectManager{};
	POOL_CONTAINER m_Pools{};
	HANDLE_STATE_CONTAINER m_HandleStates{};
	_bool m_bEnablePoolSearch{};
	_bool m_bShowHandleDetails{};
	_char m_szPoolSearch[128]{};

public:
	static UPtr<CGameObjectPoolManager> Create(
		CGameObjectManager* pGameObjectManager);

private:
	void Free() override;
};

NS_END
