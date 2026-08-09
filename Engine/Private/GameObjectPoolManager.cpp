#include "pch.h"
#include "GameObjectPoolManager.h"

#include "GameObject.h"
#include "GameObjectManager.h"

NS_USING(Engine)

CGameObjectPoolManager::CGameObjectPoolManager()
{
}

CGameObjectPoolManager::~CGameObjectPoolManager()
{
}

HRESULT CGameObjectPoolManager::Initialize(
	CGameObjectManager* pGameObjectManager)
{
	if (!pGameObjectManager)
		return E_FAIL;

	m_pGameObjectManager = pGameObjectManager;
	return S_OK;
}

size_t CGameObjectPoolManager::HANDLE_HASH::operator()(
	const CHandle& hHandle) const noexcept
{
	size_t iSeed = std::hash<size_t>{}(hHandle.GetIndex());
	iSeed ^= std::hash<uint32_t>{}(hHandle.GetGeneration()) +
		0x9e3779b9u + (iSeed << 6) + (iSeed >> 2);
	return iSeed;
}

_bool CGameObjectPoolManager::RegisterPool(
	const StringID& siPoolKey,
	POOL_DESC tDesc)
{
	if (!m_pGameObjectManager || !tDesc.fnCreate ||
		m_Pools.contains(siPoolKey))
	{
		return false;
	}

	if (tDesc.iMaxCount != 0 &&
		tDesc.iPrewarmCount > tDesc.iMaxCount)
	{
		return false;
	}

	if (tDesc.eExhaustPolicy == EXHAUST_POLICY::GROW &&
		tDesc.iGrowCount == 0)
	{
		return false;
	}

	auto [iter, bInserted] = m_Pools.emplace(
		siPoolKey,
		POOL{ .tDesc = std::move(tDesc) });
	if (!bInserted)
		return false;

	POOL& tPool = iter->second;
	if (GrowPool(
		siPoolKey,
		tPool,
		tPool.tDesc.iPrewarmCount) !=
		tPool.tDesc.iPrewarmCount)
	{
		UnregisterPool(siPoolKey);
		return false;
	}

	return true;
}

_bool CGameObjectPoolManager::UnregisterPool(
	const StringID& siPoolKey)
{
	auto poolIter = m_Pools.find(siPoolKey);
	if (poolIter == m_Pools.end())
		return false;

	POOL& tPool = poolIter->second;
	ReleaseAll(siPoolKey);

	for (const CHandle& hObject : tPool.AvailableHandles)
	{
		if (CGameObject* pObject =
			m_pGameObjectManager->GetGameObjectByHandle(hObject))
		{
			pObject->SetPendingDestroyCascade();
		}
		m_HandleStates.erase(hObject);
	}

	m_Pools.erase(poolIter);
	return true;
}

std::optional<CHandle> CGameObjectPoolManager::Acquire(
	const StringID& siPoolKey,
	void* pArg)
{
	auto poolIter = m_Pools.find(siPoolKey);
	if (poolIter == m_Pools.end() || !m_pGameObjectManager)
		return std::nullopt;

	POOL& tPool = poolIter->second;
	RemoveInvalidAvailableHandles(siPoolKey, tPool);

	if (tPool.AvailableHandles.empty())
	{
		switch (tPool.tDesc.eExhaustPolicy)
		{
		case EXHAUST_POLICY::GROW:
			GrowPool(siPoolKey, tPool, tPool.tDesc.iGrowCount);
			break;

		case EXHAUST_POLICY::RECYCLE_OLDEST:
			if (!tPool.ActiveHandles.empty())
				Release(tPool.ActiveHandles.front());
			break;

		case EXHAUST_POLICY::FAIL:
		default:
			break;
		}
	}

	while (!tPool.AvailableHandles.empty())
	{
		const CHandle hObject = tPool.AvailableHandles.back();
		tPool.AvailableHandles.pop_back();

		auto stateIter = m_HandleStates.find(hObject);
		CGameObject* pObject =
			m_pGameObjectManager->GetGameObjectByHandle(hObject);
		if (stateIter == m_HandleStates.end() ||
			stateIter->second.siPoolKey != siPoolKey ||
			stateIter->second.bActive || !pObject ||
			pObject->GetPendingDestroy())
		{
			RemoveHandleRecord(hObject, tPool);
			continue;
		}

		if (!pObject->AcquireFromPool(pArg))
		{
			pObject->ReleaseToPool();
			tPool.AvailableHandles.push_back(hObject);
			return std::nullopt;
		}

		stateIter->second.bActive = true;
		tPool.ActiveHandles.push_back(hObject);
		return hObject;
	}

	return std::nullopt;
}

_bool CGameObjectPoolManager::Release(const CHandle& hObject)
{
	auto stateIter = m_HandleStates.find(hObject);
	if (stateIter == m_HandleStates.end() ||
		!stateIter->second.bActive || !m_pGameObjectManager)
	{
		return false;
	}

	auto poolIter = m_Pools.find(stateIter->second.siPoolKey);
	if (poolIter == m_Pools.end())
	{
		m_HandleStates.erase(stateIter);
		return false;
	}

	POOL& tPool = poolIter->second;
	const auto activeIter = std::find(
		tPool.ActiveHandles.begin(),
		tPool.ActiveHandles.end(),
		hObject);
	if (activeIter != tPool.ActiveHandles.end())
		tPool.ActiveHandles.erase(activeIter);

	CGameObject* pObject =
		m_pGameObjectManager->GetGameObjectByHandle(hObject);
	if (!pObject || pObject->GetPendingDestroy())
	{
		RemoveHandleRecord(hObject, tPool);
		return false;
	}

	pObject->ReleaseToPool();
	stateIter->second.bActive = false;
	tPool.AvailableHandles.push_back(hObject);
	return true;
}

size_t CGameObjectPoolManager::ReleaseAll(
	const StringID& siPoolKey)
{
	auto poolIter = m_Pools.find(siPoolKey);
	if (poolIter == m_Pools.end())
		return 0;

	size_t iReleasedCount = 0;
	POOL& tPool = poolIter->second;
	// [LSY] Release()의 반복 탐색 없이 활성 목록을 한 번만 순회한다.
	while (!tPool.ActiveHandles.empty())
	{
		const CHandle hObject = tPool.ActiveHandles.back();
		tPool.ActiveHandles.pop_back();

		auto stateIter = m_HandleStates.find(hObject);
		CGameObject* pObject = m_pGameObjectManager
			? m_pGameObjectManager->GetGameObjectByHandle(hObject)
			: nullptr;
		if (stateIter == m_HandleStates.end() ||
			stateIter->second.siPoolKey != siPoolKey ||
			!stateIter->second.bActive || !pObject ||
			pObject->GetPendingDestroy())
		{
			RemoveHandleRecord(hObject, tPool);
			continue;
		}

		pObject->ReleaseToPool();
		stateIter->second.bActive = false;
		tPool.AvailableHandles.push_back(hObject);
		++iReleasedCount;
	}

	return iReleasedCount;
}

void CGameObjectPoolManager::ClearAllPools()
{
	while (!m_Pools.empty())
		UnregisterPool(m_Pools.begin()->first);
}

void CGameObjectPoolManager::FrameEnd()
{
	if (!m_pGameObjectManager)
		return;

	for (auto& [siPoolKey, tPool] : m_Pools)
	{
		RemoveInvalidAvailableHandles(siPoolKey, tPool);
		std::erase_if(
			tPool.ActiveHandles,
			[this, &tPool](const CHandle& hObject)
			{
				const CGameObject* pObject =
					m_pGameObjectManager->GetGameObjectByHandle(hObject);
				if (pObject && !pObject->GetPendingDestroy())
					return false;

				RemoveHandleRecord(hObject, tPool);
				return true;
			});
	}
}

void CGameObjectPoolManager::UpdateGUI()
{
	if (!ImGui::Begin("GameObject_Pool_Manager"))
	{
		ImGui::End();
		return;
	}

	size_t iTotalObjectCount = 0;
	size_t iActiveObjectCount = 0;
	size_t iAvailableObjectCount = 0;
	for (const auto& [_, tPool] : m_Pools)
	{
		iTotalObjectCount += tPool.iTotalCount;
		iActiveObjectCount += tPool.ActiveHandles.size();
		iAvailableObjectCount += tPool.AvailableHandles.size();
	}

	ImGui::Text("Pools: %zu", m_Pools.size());
	ImGui::Text(
		"Objects: %zu Total | %zu Active | %zu Available",
		iTotalObjectCount,
		iActiveObjectCount,
		iAvailableObjectCount);
	ImGui::Separator();

	ImGui::Checkbox("Enable Pool Search", &m_bEnablePoolSearch);
	if (m_bEnablePoolSearch)
		ImGui::InputText("Pool Key", m_szPoolSearch, sizeof(m_szPoolSearch));
	ImGui::Checkbox("Show Handle Details", &m_bShowHandleDetails);
	ImGui::Separator();

	std::optional<StringID> siReleaseAllRequest{};
	for (const auto& [siPoolKey, tPool] : m_Pools)
	{
		const _char* szPoolKey = siPoolKey.GetDbgStr();
		if (m_bEnablePoolSearch && m_szPoolSearch[0] != '\0' &&
			std::string_view{ szPoolKey }.find(m_szPoolSearch) ==
			std::string_view::npos)
		{
			continue;
		}

		ImGui::PushID(szPoolKey);
		const size_t iTrackedCount =
			tPool.ActiveHandles.size() + tPool.AvailableHandles.size();
		std::string sPoolLabel = std::string{ szPoolKey } + " (" +
			std::to_string(tPool.ActiveHandles.size()) + " Active, " +
			std::to_string(tPool.AvailableHandles.size()) + " Available)";

		if (iTrackedCount != tPool.iTotalCount)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.f, 0.35f, 0.35f, 1.f });

		const _bool bPoolOpened = ImGui::TreeNode(sPoolLabel.c_str());

		if (iTrackedCount != tPool.iTotalCount)
			ImGui::PopStyleColor();

		if (bPoolOpened)
		{
			const _char* szExhaustPolicy = "Unknown";
			switch (tPool.tDesc.eExhaustPolicy)
			{
			case EXHAUST_POLICY::GROW:
				szExhaustPolicy = "Grow";
				break;
			case EXHAUST_POLICY::FAIL:
				szExhaustPolicy = "Fail";
				break;
			case EXHAUST_POLICY::RECYCLE_OLDEST:
				szExhaustPolicy = "Recycle Oldest";
				break;
			default:
				break;
			}

			ImGui::Text("Prewarm Count: %zu", tPool.tDesc.iPrewarmCount);
			ImGui::Text("Grow Count: %zu", tPool.tDesc.iGrowCount);
			if (tPool.tDesc.iMaxCount == 0)
				ImGui::TextUnformatted("Max Count: Unlimited");
			else
				ImGui::Text("Max Count: %zu", tPool.tDesc.iMaxCount);
			ImGui::Text("Exhaust Policy: %s", szExhaustPolicy);
			ImGui::Text(
				"Tracked: %zu Total | %zu Active | %zu Available",
				tPool.iTotalCount,
				tPool.ActiveHandles.size(),
				tPool.AvailableHandles.size());

			if (iTrackedCount != tPool.iTotalCount)
			{
				ImGui::TextColored(
					ImVec4{ 1.f, 0.35f, 0.35f, 1.f },
					"Invalid count state: handle lists contain %zu entries.",
					iTrackedCount);
			}

			if (!tPool.ActiveHandles.empty() && ImGui::Button("Release All"))
				siReleaseAllRequest = siPoolKey;

			if (m_bShowHandleDetails)
			{
				auto DrawHandleList = [this](
					const _char* szLabel,
					const auto& Handles,
					_bool bExpectedActive)
				{
					std::string sLabel = std::string{ szLabel } + " (" +
						std::to_string(Handles.size()) + ")";
					if (!ImGui::TreeNode(sLabel.c_str()))
						return;

					for (const CHandle& hObject : Handles)
					{
						const auto stateIter = m_HandleStates.find(hObject);
						const CGameObject* pObject = m_pGameObjectManager ?
							m_pGameObjectManager->GetGameObjectByHandle(hObject) : nullptr;
						const _bool bValid =
							stateIter != m_HandleStates.end() &&
							stateIter->second.bActive == bExpectedActive &&
							pObject && !pObject->GetPendingDestroy();

						if (!bValid)
							ImGui::PushStyleColor(
								ImGuiCol_Text,
								ImVec4{ 1.f, 0.35f, 0.35f, 1.f });

						if (pObject)
						{
							ImGui::Text(
								"%s Index: %zu | Gen: %u | Type: %.*s | Tag: %.*s",
								bValid ? "" : "[Invalid]",
								hObject.GetIndex(),
								hObject.GetGeneration(),
								static_cast<int>(pObject->GetTypeString().size()),
								pObject->GetTypeString().data(),
								static_cast<int>(pObject->GetObjectTag().size()),
								pObject->GetObjectTag().data());
						}
						else
						{
							ImGui::Text(
								"[Invalid] Index: %zu | Gen: %u | Object not found",
								hObject.GetIndex(),
								hObject.GetGeneration());
						}

						if (!bValid)
							ImGui::PopStyleColor();
					}

					ImGui::TreePop();
				};

				DrawHandleList("Active Handles", tPool.ActiveHandles, true);
				DrawHandleList("Available Handles", tPool.AvailableHandles, false);
			}

			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	if (siReleaseAllRequest)
		ReleaseAll(*siReleaseAllRequest);

	ImGui::End();
}

_bool CGameObjectPoolManager::HasPool(
	const StringID& siPoolKey) const
{
	return m_Pools.contains(siPoolKey);
}

size_t CGameObjectPoolManager::GetTotalCount(
	const StringID& siPoolKey) const
{
	const auto iter = m_Pools.find(siPoolKey);
	return iter != m_Pools.end() ? iter->second.iTotalCount : 0;
}

size_t CGameObjectPoolManager::GetActiveCount(
	const StringID& siPoolKey) const
{
	const auto iter = m_Pools.find(siPoolKey);
	return iter != m_Pools.end() ? iter->second.ActiveHandles.size() : 0;
}

size_t CGameObjectPoolManager::GetAvailableCount(
	const StringID& siPoolKey) const
{
	const auto iter = m_Pools.find(siPoolKey);
	return iter != m_Pools.end() ? iter->second.AvailableHandles.size() : 0;
}

size_t CGameObjectPoolManager::GrowPool(
	const StringID& siPoolKey,
	POOL& tPool,
	size_t iRequestedCount)
{
	size_t iCreatedCount = 0;
	while (iCreatedCount < iRequestedCount)
	{
		if (tPool.tDesc.iMaxCount != 0 &&
			tPool.iTotalCount >= tPool.tDesc.iMaxCount)
		{
			break;
		}

		const auto hObject = tPool.tDesc.fnCreate();
		if (!hObject || m_HandleStates.contains(*hObject))
			break;

		CGameObject* pObject =
			m_pGameObjectManager->GetGameObjectByHandle(*hObject);
		if (!pObject || pObject->GetPendingDestroy())
			break;

		pObject->ReleaseToPool();
		tPool.AvailableHandles.push_back(*hObject);
		m_HandleStates.emplace(
			*hObject,
			HANDLE_STATE{
				.siPoolKey = siPoolKey,
				.bActive = false });
		++tPool.iTotalCount;
		++iCreatedCount;
	}

	return iCreatedCount;
}

void CGameObjectPoolManager::RemoveInvalidAvailableHandles(
	const StringID& siPoolKey,
	POOL& tPool)
{
	std::erase_if(
		tPool.AvailableHandles,
		[this, &siPoolKey, &tPool](const CHandle& hObject)
		{
			const auto stateIter = m_HandleStates.find(hObject);
			const CGameObject* pObject =
				m_pGameObjectManager->GetGameObjectByHandle(hObject);
			const _bool bInvalid =
				stateIter == m_HandleStates.end() ||
				stateIter->second.siPoolKey != siPoolKey ||
				stateIter->second.bActive || !pObject ||
				pObject->GetPendingDestroy();

			if (bInvalid)
				RemoveHandleRecord(hObject, tPool);

			return bInvalid;
		});
}

void CGameObjectPoolManager::RemoveHandleRecord(
	const CHandle& hObject,
	POOL& tPool)
{
	m_HandleStates.erase(hObject);
	if (tPool.iTotalCount > 0)
		--tPool.iTotalCount;
}

UPtr<CGameObjectPoolManager> CGameObjectPoolManager::Create(
	CGameObjectManager* pGameObjectManager)
{
	auto pInstance = ToUPtr(new CGameObjectPoolManager{});
	if (FAILED(pInstance->Initialize(pGameObjectManager)))
	{
		MSG_BOX("Failed to Create: CGameObjectPoolManager");
		return nullptr;
	}

	return pInstance;
}

void CGameObjectPoolManager::Free()
{
	ClearAllPools();
	m_HandleStates.clear();
	m_pGameObjectManager = nullptr;

	CEngineBase::Free();
}
