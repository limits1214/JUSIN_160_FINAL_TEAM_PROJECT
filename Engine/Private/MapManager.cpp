#include "pch.h"
#include "MapManager.h"
#include "MapMeshObject.h"
#include <fstream>
#include <filesystem>

#include "CameraObject.h"
#include "CollFrustum.h"
#include "MapStaticModelLoader.h"
#include "OctreeNode.h"
NS_USING(Engine)

struct Engine::PENDING_CHUNK_APPLY_STATE
{
	PENDING_CHUNK_LOAD_RESULT result{};
	size_t nextObjectIndex{};
	_bool initialized{};
};

namespace
{
	//constexpr _float3 DEFAULT_MAP_CHUNK_SIZE{ 150.f, 150.f, 150.f };

	_bool IsSameChunkSize(const _float3& lhs, const _float3& rhs)
	{
		return std::fabs(lhs.x - rhs.x) <= FLT_EPSILON
			&& std::fabs(lhs.y - rhs.y) <= FLT_EPSILON
			&& std::fabs(lhs.z - rhs.z) <= FLT_EPSILON;
	}

	std::string ChunkFileName(const MAPCHUNK_COORD& coord)
	{
		return std::to_string(coord.x) + "_" + std::to_string(coord.y) + "_" + std::to_string(coord.z) + ".json";
	}

	nlohmann::ordered_json MakeCoordJson(const MAPCHUNK_COORD& coord)
	{
		return nlohmann::ordered_json
		{
			{"x", coord.x},
			{"y", coord.y},
			{"z", coord.z}
		};
	}

	MAPCHUNK_COORD ReadCoordJson(const nlohmann::ordered_json& coordJson)
	{
		return MAPCHUNK_COORD
		{
			coordJson["x"].get<int64_t>(),
			coordJson["y"].get<int64_t>(),
			coordJson["z"].get<int64_t>()
		};
	}

	nlohmann::ordered_json MakeWindJson(const WIND_DESC& windDesc)
	{
		return nlohmann::ordered_json
		{
			{ "type", static_cast<uint32_t>(windDesc.type) },
			{ "strength", windDesc.strength },
			{ "speed", windDesc.speed },
			{ "frequency", windDesc.frequency },
			{ "bendExponent", windDesc.bendExponent },
			{ "heightStart", windDesc.heightStart },
			{ "heightEnd", windDesc.heightEnd }
		};
	}

	WIND_DESC ReadWindJson(const nlohmann::ordered_json& objectJson)
	{
		WIND_DESC windDesc{};
		if (!objectJson.contains("wind") || !objectJson["wind"].is_object())
			return windDesc;

		const auto& windJson = objectJson["wind"];
		const uint32_t windType = windJson.value("type", static_cast<uint32_t>(EWindType::None));
		if (windType <= static_cast<uint32_t>(EWindType::Tree))
			windDesc.type = static_cast<EWindType>(windType);
		windDesc.strength = windJson.value("strength", windDesc.strength);
		windDesc.speed = windJson.value("speed", windDesc.speed);
		windDesc.frequency = windJson.value("frequency", windDesc.frequency);
		windDesc.bendExponent = windJson.value("bendExponent", windDesc.bendExponent);
		windDesc.heightStart = windJson.value("heightStart", windDesc.heightStart);
		windDesc.heightEnd = windJson.value("heightEnd", windDesc.heightEnd);
		return windDesc;
	}

	nlohmann::ordered_json MakeObjectJson(CMapMeshObject* pMeshObj, const std::string& layerName, const MAPCHUNK_COORD& coord)
	{
		const auto& pTransform = pMeshObj->GetTransform();
		const auto& pos = pTransform.GetPosition();
		const auto& quat = pTransform.GetQuaternion();
		const auto& scale = pTransform.GetScale();

		return nlohmann::ordered_json
		{
			{"type", "MapMeshObject"},
			{"objectTag", std::string(pMeshObj->GetObjectTag())},
			{"protoGroup", "PERMANENT"},
			{"prototype", "Prototype_GameObject_MapMeshObject"},
			{"modelGroup", pMeshObj->GetModelResourceGroup()},
			{"model", pMeshObj->GetModelResourceTag()},
			{"layer", layerName},
			{"position", { pos.x, pos.y, pos.z }},
			{"rotation", { quat.x, quat.y, quat.z, quat.w }},
			{"scale", { scale.x, scale.y, scale.z }},
			{"wind", MakeWindJson(pMeshObj->GetWindDesc())},
			{"chunk", MakeCoordJson(coord)}
		};
	}

	std::optional<MAP_MESH_OBJECT_LOAD_DESC> MakeMapMeshLoadDesc(const nlohmann::ordered_json& objectJson)
	{
		if (!objectJson.contains("type") || objectJson["type"] != "MapMeshObject")
			return std::nullopt;

		MAP_MESH_OBJECT_LOAD_DESC desc{};
		desc.objectTag = objectJson["objectTag"];
		desc.protoGroup = objectJson.value("protoGroup", "PERMANENT");
		desc.prototype = objectJson.value("prototype", "Prototype_GameObject_MapMeshObject");
		desc.modelGroup = objectJson["modelGroup"];
		desc.model = objectJson["model"];
		desc.layer = objectJson["layer"];

		const auto& pos = objectJson["position"];
		const auto& rot = objectJson["rotation"];
		const auto& scale = objectJson["scale"];

		desc.position = _float3{ pos[0], pos[1], pos[2] };
		desc.rotation = _float4{ rot[0], rot[1], rot[2], rot[3] };
		desc.scale = _float3{ scale[0], scale[1], scale[2] };
		desc.windDesc = ReadWindJson(objectJson);

		return desc;
	}

	_bool HasHandle(const std::vector<CHandle>& handles, const CHandle& target)
	{
		return std::find(handles.begin(), handles.end(), target) != handles.end();
	}

	_bool CanAutoUnload(const MAPCHUNK& chunk)
	{
		return chunk.loadState == EChunkLoadState::Loaded
			&& chunk.saveState != EChunkSaveState::Unsaved;
	}

	_bool CanAutoLoad(const MAPCHUNK& chunk)
	{
		return chunk.loadState == EChunkLoadState::Unloaded
			&& chunk.saveState != EChunkSaveState::Unsaved;
	}

	std::optional<CHandle> CreateMapMeshObjectFromJson(const nlohmann::ordered_json& objectJson)
	{
		if (!objectJson.contains("type") || objectJson["type"] != "MapMeshObject")
		{
			return std::nullopt;
		}

		const std::string objectTag = objectJson["objectTag"];
		const std::string modelGroup = objectJson["modelGroup"];
		const std::string model = objectJson["model"];
		const std::string layer = objectJson["layer"];

		E::CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectTag;
		desc.modelGroupTag = modelGroup;
		desc.modelResTag = model;
		desc.protoGroupTag = objectJson.value("protoGroup", "PERMANENT");
		desc.prototypeTag = objectJson.value("prototype", "Prototype_GameObject_MapMeshObject");
		desc.windDesc = ReadWindJson(objectJson);

		auto hObject = E::CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			layer,
			&desc);
		if (!hObject.has_value())
		{
			return std::nullopt;
		}

		auto* newObj = E::CGameInstance::Get().GetGameObjectByHandle(hObject.value());
		if (newObj == nullptr)
		{
			return std::nullopt;
		}

		const auto& pos = objectJson["position"];
		const auto& rot = objectJson["rotation"];
		const auto& scale = objectJson["scale"];

		auto& newObjTransform = newObj->GetTransform();
		newObjTransform.SetPosition(XMVectorSet(pos[0], pos[1], pos[2], 1.f));
		newObjTransform.SetQuaternion(_float4{ rot[0], rot[1], rot[2], rot[3] });
		newObjTransform.SetScale(_float3{ scale[0], scale[1], scale[2] });

		/*----------- 광윤 추가 -----------*/
		newObjTransform.Update();
		/*---------------------------------*/

		return hObject;
	}

}

#ifdef _DEBUG
std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> CMapManager::_batch = nullptr;
std::unique_ptr<DirectX::BasicEffect> CMapManager::_effect = nullptr;
ComPtr<ID3D11InputLayout> CMapManager::_inputLayout = nullptr;
#endif

CMapManager::CMapManager()
{
}

CMapManager::~CMapManager()
{
#ifdef _DEBUG
	//-------DebugDraw-------
	_batch.reset();
	_effect.reset();
	_inputLayout.Reset();
	//-----------------------
#endif
}

HRESULT CMapManager::Initialize()
{

#ifdef _DEBUG
	// DebugDraw
	// static 객체들이 아직 생성되지 않았다면 최초 1회 생성
	if (_batch == nullptr)
	{
		// Batch 생성
		_batch = std::make_unique<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>>(CGameInstance::Get().GetGraphicDeviceContext().Get());

		// Effect 생성 및 설정
		_effect = std::make_unique<DirectX::BasicEffect>(CGameInstance::Get().GetGraphicDevice().Get());
		_effect->SetVertexColorEnabled(true); // 정점 색상 사용 활성화

		// InputLayout 생성 (BasicEffect의 셰이더 정보와 VertexPositionColor 구조를 연결)
		void const* shaderByteCode;
		size_t byteCodeLength;
		_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

		HRESULT hr = CGameInstance::Get().GetGraphicDevice()->CreateInputLayout(
			DirectX::VertexPositionColor::InputElements,
			DirectX::VertexPositionColor::InputElementCount,
			shaderByteCode,
			byteCodeLength,
			_inputLayout.GetAddressOf()
		);

		assert(SUCCEEDED(hr));
	}
#endif

	return S_OK;
}

void CMapManager::PriorityUpdate(_float fTimeDelta)
{

}

// 저장해놨던 Chunk들을 심리스 로딩/언로딩
// (저장해놓은 Chunk들만 작동하기때문에 에디터에서 실시간으로 Rebuild한 Cunk는 심리스의 적용대상이 되지않음)

void CMapManager::Update(_float fTimeDelta)
{
	ProcessDeferredModelReleases();
	ProcessLoadedChunkResults();

	if (m_Chunks.empty())
	{
		return;
	}

	CCameraObject* pCamera = CGameInstance::Get().GetActiveCamera();
	if (pCamera == nullptr)
	{
		return;
	}

	const auto* pFrustumCollider = pCamera->GetFrustumCollider();
	if (pFrustumCollider == nullptr)
		return;

	const auto loadChunks = GetChunksAroundCamera(pCamera, STREAM_LOAD_RADIUS);
	const auto retainedChunks = GetChunksAroundCamera(pCamera, STREAM_UNLOAD_RADIUS);
	const auto& boundingFrustum = pFrustumCollider->GetBoundingFrustum();

	CullLoadedChunksByCameraFrustum(retainedChunks, boundingFrustum);

	if (m_bChunkStreaming && !m_sMapRootPath.empty())
	{
		UnloadChunksOutsideRange(retainedChunks);
		RequestNeededChunkLoads(loadChunks);
	}

    //  이전 프레임에서 예약한 리소스 해제
    //               ↓
    //  워커가 완료한 청크를 월드에 조금씩 반영
    //               ↓
    //  카메라 주변 5×5×5 청크 계산
    //  카메라 주변 7×7×7 유지 영역 계산
    //               ↓
    //  7×7×7 밖의 청크 언로드
    //               ↓
    //  5×5×5 안에서 가장 가까운 청크 하나 로드 요청
}

std::vector<MAPCHUNK_COORD> CMapManager::GetChunksAroundCamera(const CCameraObject* pCamera, int64_t radius) const
{

	const auto& pos = pCamera->GetTransform().GetPosition();
	const MAPCHUNK_COORD cameraChunkCoord = WorldToChunkCoord(pos);

	const size_t diameter = static_cast<size_t>(radius * 2 + 1);
	std::vector<MAPCHUNK_COORD> chunks;
	chunks.reserve(diameter * diameter * diameter);

	// radius 2 = 5x5x5 load range, radius 3 = 7x7x7 retention range.
	for (int64_t y = -radius; y <= radius; ++y)
	{
		for (int64_t z = -radius; z <= radius; ++z)
		{
			for (int64_t x = -radius; x <= radius; ++x)
			{
				chunks.push_back(
					MAPCHUNK_COORD
					{
						cameraChunkCoord.x + x,
						cameraChunkCoord.y + y,
						cameraChunkCoord.z + z
					});
			}
		}
	}

	return chunks;
}

void CMapManager::UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& neededChunks)
{
	auto isNeededChunk = [&neededChunks](const MAPCHUNK_COORD& coord)
		{
			return std::find(neededChunks.begin(), neededChunks.end(), coord) != neededChunks.end();
		};

	// 모든 Chunk들 중 주변3*3*3 Chunk가 아니라면 UnLoad
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (CanAutoUnload(chunk) && !isNeededChunk(coord))
		{
			UnLoadChunk(coord);
		}
	}
}

void CMapManager::RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& neededChunks)
{
	if (m_AsyncChunkLoadsInFlight.load(std::memory_order_acquire) != 0)
		return;

	std::vector<MAPCHUNK_COORD> prioritizedChunks = neededChunks;
	if (const auto* camera = CGameInstance::Get().GetActiveCamera())
	{
		const MAPCHUNK_COORD cameraCoord = WorldToChunkCoord(camera->GetTransform().GetPosition());

		std::sort(prioritizedChunks.begin(), prioritizedChunks.end(),
			[&cameraCoord](const MAPCHUNK_COORD& lhs, const MAPCHUNK_COORD& rhs)
			{
				const auto DistanceSquared = [&cameraCoord](const MAPCHUNK_COORD& coord)
					{
						const int64_t dx = coord.x - cameraCoord.x;
						const int64_t dy = coord.y - cameraCoord.y;
						const int64_t dz = coord.z - cameraCoord.z;
						return dx * dx + dy * dy + dz * dz;
					};
				return DistanceSquared(lhs) < DistanceSquared(rhs);
			});
	}

	for (const auto& coord : prioritizedChunks)
	{
		auto iter = m_Chunks.find(coord);
		if (iter == m_Chunks.end())
		{
			continue;
		}

		if (CanAutoLoad(iter->second))
		{
			RequestLoadChunkAsync(coord);
			//LoadChunk(coord);
			return;
		}
	}
}

void CMapManager::CullLoadedChunksByCameraFrustum(const std::vector<MAPCHUNK_COORD>& neededChunks, const BoundingFrustum& boundingFrustum)
{
	for (const auto& coord : neededChunks)
	{
		auto iter = m_Chunks.find(coord);
		if (iter == m_Chunks.end())
		{
			continue;
		}

		if (iter->second.loadState != EChunkLoadState::Loaded)
		{
			continue;
		}

		const auto& selectedChunk = iter->second;
		const BoundingBox& cullingBounds = selectedChunk.octreeNode
			? selectedChunk.octreeNode->GetCullingBoundingBox()
			: selectedChunk.bounds;

		if (!boundingFrustum.Intersects(cullingBounds))
		{
			continue;
		}

		if (const auto& octreeNode = selectedChunk.octreeNode)
		{
			octreeNode->OctreeFrustumCull(boundingFrustum);
		}
	}
}

void CMapManager::LateUpdate(_float fTimeDelta)
{

}

void CMapManager::ClearAllChunk()
{
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	QueueAllChunkModelReleases();
	m_Chunks.clear();
}

void CMapManager::SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, std::unordered_map<std::string, std::filesystem::path> modelPaths)
{
	std::lock_guard<std::mutex> lock(m_MapModelResourceIndexMutex);
	m_MapModelStaticRoot = staticModelRoot;
	m_MapModelResourceGroup = resourceGroup;
	m_MapModelPaths = std::move(modelPaths);
}

HRESULT CMapManager::EnsureModelResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key)
{
	if (auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag))
	{
		if (model->GetState() == CResource::STATE::LOADED)
			return S_OK;
		if (model->GetState() != CResource::STATE::UNLOAD && model->GetState() != CResource::STATE::LOADFAIL)
			return E_FAIL;
	}

	std::filesystem::path staticModelRoot;
	std::filesystem::path modelPath;
	{
		std::lock_guard<std::mutex> lock(m_MapModelResourceIndexMutex);
		if (key.group != m_MapModelResourceGroup)
			return E_FAIL;

		const auto pathIter = m_MapModelPaths.find(key.tag);
		if (pathIter == m_MapModelPaths.end())
			return E_FAIL;

		staticModelRoot = m_MapModelStaticRoot;
		modelPath = pathIter->second;
	}
	return LoadMapStaticModelFile(
		modelPath,
		staticModelRoot,
		key.group,
		nullptr,
		key.tag)
		? S_OK
		: E_FAIL;
}

HRESULT CMapManager::AcquireChunkModelResources(MAPCHUNK& chunk, const std::vector<MAP_MESH_OBJECT_LOAD_DESC>& objects)
{
	std::lock_guard<std::mutex> resourceLock(m_ModelResourceLoadMutex);
	std::unordered_set<MAP_MODEL_RESOURCE_KEY, MAP_MODEL_RESOURCE_KEY_HASH> uniqueModels;
	for (const auto& object : objects)
	{
		if (!object.modelGroup.empty() && !object.model.empty())
			uniqueModels.insert({ object.modelGroup, object.model });
	}

	std::vector<MAP_MODEL_RESOURCE_KEY> loadedForThisChunk;
	for (const auto& key : uniqueModels)
	{
		const auto existing = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		const bool wasAlreadyLoaded = existing && existing->GetState() == CResource::STATE::LOADED;

		if (FAILED(EnsureModelResourceLoaded(key)))
		{
			for (const auto& candidate : loadedForThisChunk)
			{
				auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(candidate.group, candidate.tag);
				if (!model)
					continue;

				CGameInstance::Get().EraseMapMeshTextureCache(model);
				model->Unload();
				CGameInstance::Get().DelResource(candidate.group, candidate.tag);
			}
			return E_FAIL;
		}

		if (!wasAlreadyLoaded)
			loadedForThisChunk.push_back(key);
	}

	chunk.modelResources.clear();
	chunk.modelResources.reserve(uniqueModels.size());
	for (const auto& key : uniqueModels)
	{
		++m_ModelChunkRefCounts[key];
		chunk.modelResources.push_back(key);
	}

	return S_OK;
}

HRESULT CMapManager::PreloadChunkModelResources(PENDING_CHUNK_LOAD_RESULT& result)
{
	ZoneScopedN("ChunkResourcePreloadWorker");
	std::unordered_set<MAP_MODEL_RESOURCE_KEY, MAP_MODEL_RESOURCE_KEY_HASH> uniqueModels;
	for (const auto& object : result.objects)
	{
		if (!object.modelGroup.empty() && !object.model.empty())
			uniqueModels.insert({ object.modelGroup, object.model });
	}

	std::lock_guard<std::mutex> resourceLock(m_ModelResourceLoadMutex);
	std::vector<MAP_MODEL_RESOURCE_KEY> loadedForThisResult;
	for (const auto& key : uniqueModels)
	{
		const auto existing = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		const bool wasAlreadyLoaded = existing && existing->GetState() == CResource::STATE::LOADED;

		if (FAILED(EnsureModelResourceLoaded(key)))
		{
			for (const auto& loadedKey : loadedForThisResult)
			{
				auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(loadedKey.group, loadedKey.tag);
				if (!model)
					continue;

				model->Unload();
				CGameInstance::Get().DelResource(loadedKey.group, loadedKey.tag);
			}
			return E_FAIL;
		}

		if (!wasAlreadyLoaded)
			loadedForThisResult.push_back(key);
	}

	result.modelResources.assign(uniqueModels.begin(), uniqueModels.end());
	{
		std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
		for (const auto& key : result.modelResources)
			++m_PendingModelRefCounts[key];
	}
	return S_OK;
}

HRESULT CMapManager::AcquirePreloadedChunkModelResources(MAPCHUNK& chunk, const PENDING_CHUNK_LOAD_RESULT& result)
{
	for (const auto& key : result.modelResources)
	{
		auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		if (!model || model->GetState() != CResource::STATE::LOADED)
		{
			ReleasePendingModelResources(result);
			return E_FAIL;
		}
	}

	{
		std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
		for (const auto& key : result.modelResources)
		{
			auto pendingIter = m_PendingModelRefCounts.find(key);
			if (pendingIter == m_PendingModelRefCounts.end())
				continue;
			if (pendingIter->second > 1)
				--pendingIter->second;
			else
				m_PendingModelRefCounts.erase(pendingIter);
		}
	}

	chunk.modelResources = result.modelResources;
	for (const auto& key : chunk.modelResources)
		++m_ModelChunkRefCounts[key];
	return S_OK;
}

void CMapManager::ReleasePendingModelResources(const PENDING_CHUNK_LOAD_RESULT& result)
{
	{
		std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
		for (const auto& key : result.modelResources)
		{
			auto pendingIter = m_PendingModelRefCounts.find(key);
			if (pendingIter == m_PendingModelRefCounts.end())
				continue;
			if (pendingIter->second > 1)
				--pendingIter->second;
			else
				m_PendingModelRefCounts.erase(pendingIter);
		}
	}

	m_DeferredUnusedModelReleases.insert(m_DeferredUnusedModelReleases.end(), result.modelResources.begin(), result.modelResources.end());
}

void CMapManager::QueueChunkModelRelease(MAPCHUNK& chunk)
{
	if (chunk.modelResources.empty())
		return;

	m_DeferredModelReleases.push_back(std::move(chunk.modelResources));
	chunk.modelResources.clear();
}

void CMapManager::QueueAllChunkModelReleases()
{
	for (auto& [coord, chunk] : m_Chunks)
		QueueChunkModelRelease(chunk);
}

void CMapManager::ProcessDeferredModelReleases()
{
	if (m_DeferredModelReleases.empty() && m_DeferredUnusedModelReleases.empty())
		return;

	std::unique_lock<std::mutex> resourceLock(m_ModelResourceLoadMutex, std::try_to_lock);
	if (!resourceLock.owns_lock())
		return;

	auto releases = std::move(m_DeferredModelReleases);
	m_DeferredModelReleases.clear();
	auto candidates = std::move(m_DeferredUnusedModelReleases);
	m_DeferredUnusedModelReleases.clear();

	for (const auto& chunkResources : releases)
	{
		for (const auto& key : chunkResources)
		{
			const auto refIter = m_ModelChunkRefCounts.find(key);
			if (refIter == m_ModelChunkRefCounts.end())
				continue;

			if (refIter->second > 1)
			{
				--refIter->second;
				continue;
			}

			m_ModelChunkRefCounts.erase(refIter);
			candidates.push_back(key);
		}
	}

	for (const auto& key : candidates)
	{
		if (m_ModelChunkRefCounts.contains(key))
			continue;

		{
			std::lock_guard<std::mutex> pendingLock(m_PendingModelRefMutex);
			if (m_PendingModelRefCounts.contains(key))
				continue;
		}

		auto model = CGameInstance::Get().GetResourceFirst<CResStaticModel>(key.group, key.tag);
		if (!model)
			continue;

		CGameInstance::Get().EraseMapMeshTextureCache(model);
		model->Unload();
		CGameInstance::Get().DelResource(key.group, key.tag);
	}
}

HRESULT CMapManager::SaveMap(const std::string& path)
{
	RebuildChunks();

	const std::filesystem::path mapDir(path);
	const std::filesystem::path chunkDir = mapDir / "chunks";
	std::error_code ec;
	std::filesystem::create_directories(chunkDir, ec);
	if (ec)
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson = {};
	rootJson["version"] = 1;
	rootJson["chunkSize"] = { m_vChunkSize.x, m_vChunkSize.y, m_vChunkSize.z };
	rootJson["chunks"] = nlohmann::ordered_json::array();
	rootJson["objects"] = nlohmann::ordered_json::array();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	std::vector<MAPCHUNK_COORD> originallyUnloadedChunks;
	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.loadState == EChunkLoadState::Unloaded && chunk.saveState != EChunkSaveState::Unsaved)
		{
			originallyUnloadedChunks.push_back(coord);
			if (FAILED(/*RequestLoadChunkAsync(coord)*/LoadChunk(coord))) // Synchronous load while saving.
			{
				return E_FAIL;
			}
		}
	}

	for (auto& [coord, chunk] : m_Chunks)
	{
		const std::string fileName = ChunkFileName(coord);
		const std::filesystem::path relativePath = std::filesystem::path("chunks") / fileName;
		const std::filesystem::path chunkPath = mapDir / relativePath;

		chunk.filePath = relativePath.generic_string();

		if (FAILED(SaveChunk(coord, chunkPath.generic_string())))
		{
			return E_FAIL;
		}

		chunk.saveState = EChunkSaveState::Saved;

		rootJson["chunks"].push_back(nlohmann::ordered_json
		{
			{"coord", MakeCoordJson(coord)},
			{"file", chunk.filePath},
			{"objectCount", chunk.hObjects.size()}
		});
	}

	for (const auto& pair : layers)
	{
		const auto& objects = pair.second;

		for(const auto& objectHandle : objects)
		{
			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(objectHandle);
			
			if (pMeshObj == nullptr)
				continue;

			const auto& layerName = pair.first;

			const auto& pos = pMeshObj->GetTransform().GetPosition();
			const MAPCHUNK_COORD coord = WorldToChunkCoord(pos);
			rootJson["objects"].push_back(MakeObjectJson(pMeshObj, layerName, coord));
		}
	}

	for (const auto& coord : originallyUnloadedChunks)
	{
		UnLoadChunk(coord);
	}

	std::ofstream outFile((mapDir / "map.json").string());
	if (!outFile.is_open())
	{
		return E_FAIL;
	}

	outFile << rootJson.dump(4);
	outFile.close();

	return S_OK;
}

HRESULT CMapManager::LoadMap(const std::string& path, _bool clearBeforeLoad)
{
	if (clearBeforeLoad)
	{
		m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
		CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
		QueueAllChunkModelReleases();
		m_Chunks.clear();
	}

	const std::filesystem::path mapDir(path);
	std::filesystem::path mapFilePath = mapDir / "map.json";
	if (std::filesystem::exists(mapFilePath))
	{
		const _float3 requestedChunkSize = DEFAULT_MAP_CHUNK_SIZE;

		if (FAILED(LoadMapData(path)))
		{
			return E_FAIL;
		}

		if (!IsSameChunkSize(m_vChunkSize, requestedChunkSize))
		{
			std::vector<MAPCHUNK_COORD> oldChunkCoords;
			oldChunkCoords.reserve(m_Chunks.size());
			for (const auto& [coord, chunk] : m_Chunks)
				oldChunkCoords.push_back(coord);

			for (const MAPCHUNK_COORD& coord : oldChunkCoords)
			{
				if (FAILED(LoadChunk(coord)))
					return E_FAIL;
			}

			m_vChunkSize = requestedChunkSize;
			m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
			QueueAllChunkModelReleases();
			m_Chunks.clear();
			RebuildChunks();

			if (FAILED(SaveMap(path)))
				return E_FAIL;

			// 마이그레이션에는 해당 객체가 일시적으로만 필요
			// 일반적인 스트리밍 맵 로드 시 사용되는 것과 동일한, 메타데이터 전용 상태로 복원
			CGameInstance::Get().DelGameObjectLayer(E::MAPMESHOBJECTLAYER);
			m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
			QueueAllChunkModelReleases();
			m_Chunks.clear();
			if (FAILED(LoadMapData(path)))
				return E_FAIL;
		}

		std::vector<MAPCHUNK_COORD> chunkCoords;
		chunkCoords.reserve(m_Chunks.size());
		for (const auto& [coord, chunk] : m_Chunks)
		{
			chunkCoords.push_back(coord);
		}

		// 메타만 Load하고 chunk 로딩은 Update함수에서 스트리밍에게 맡긴다
		//for (const auto& coord : chunkCoords)
		//{
		//	if (FAILED(RequestLoadChunkAsync(coord)/*LoadChunk(coord)*/))
		//	{
		//		return E_FAIL;
		//	}
		//}

		return S_OK;
	}

	mapFilePath = mapDir / "TestMap.json";
	std::ifstream inFile(mapFilePath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson;
	inFile >> rootJson;

	inFile.close();

	int version = rootJson["version"];

	std::unordered_map<MAPCHUNK_COORD, std::vector<MAP_MESH_OBJECT_LOAD_DESC>, tagMapChunkCoordHash> legacyObjectsByChunk;
	for (const auto& objectJson : rootJson["objects"])
	{
		if (auto desc = MakeMapMeshLoadDesc(objectJson))
		{
			const auto& pos = objectJson["position"];
			const MAPCHUNK_COORD coord = WorldToChunkCoord({ pos[0], pos[1], pos[2] });
			legacyObjectsByChunk[coord].push_back(std::move(desc.value()));
		}
	}

	for (auto& [coord, objects] : legacyObjectsByChunk)
	{
		auto& chunk = m_Chunks[coord];
		chunk.coord = coord;
		chunk.bounds = MakeChunkBoundingBox(coord);
		if (FAILED(AcquireChunkModelResources(chunk, objects)))
			return E_FAIL;

		for (const auto& objectDesc : objects)
		{
			CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
			desc.sObjectTag = objectDesc.objectTag;
			desc.protoGroupTag = objectDesc.protoGroup;
			desc.prototypeTag = objectDesc.prototype;
			desc.modelGroupTag = objectDesc.modelGroup;
			desc.modelResTag = objectDesc.model;
			desc.windDesc = objectDesc.windDesc;

			auto hObject = CGameInstance::Get().AddGameObjectToLayer(
				desc.protoGroupTag, desc.prototypeTag, objectDesc.layer, &desc);
			if (!hObject)
				continue;

			auto* object = CGameInstance::Get().GetGameObjectByHandle(hObject.value());
			if (!object)
				continue;

			object->GetTransform().SetPosition(objectDesc.position);
			object->GetTransform().SetQuaternion(objectDesc.rotation);
			object->GetTransform().SetScale(objectDesc.scale);
			chunk.hObjects.push_back(hObject.value());
		}

		chunk.loadState = EChunkLoadState::Loaded;
		chunk.saveState = EChunkSaveState::Saved;
	}
	return S_OK;
}

HRESULT CMapManager::SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath)
{
	const auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	const std::filesystem::path filePath(chunkPath);
	std::error_code ec;
	std::filesystem::create_directories(filePath.parent_path(), ec);
	if (ec)
	{
		return E_FAIL;
	}

	const MAPCHUNK& chunk = iter->second;
	nlohmann::ordered_json chunkJson = {};
	chunkJson["version"] = 1;
	chunkJson["coord"] = MakeCoordJson(coord);
	chunkJson["bounds"] =
	{
		{"center", { chunk.bounds.Center.x, chunk.bounds.Center.y, chunk.bounds.Center.z }},
		{"extents", { chunk.bounds.Extents.x, chunk.bounds.Extents.y, chunk.bounds.Extents.z }}
	};
	chunkJson["objects"] = nlohmann::ordered_json::array();

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();
	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			if (!HasHandle(chunk.hObjects, handle))
			{
				continue;
			}

			CMapMeshObject* pMeshObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);
			if (pMeshObj == nullptr)
			{
				continue;
			}

			chunkJson["objects"].push_back(MakeObjectJson(pMeshObj, layerName, coord));
		}
	}

	std::ofstream outFile(filePath.string());
	if (!outFile.is_open())
	{
		return E_FAIL;
	}

	outFile << chunkJson.dump(4);
	outFile.close();

	return S_OK;
}

HRESULT CMapManager::LoadMapData(const std::string& path)
{
	const std::filesystem::path mapDir(path);
	const std::filesystem::path mapFilePath = mapDir / "map.json";

	std::ifstream inFile(mapFilePath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json rootJson;
	inFile >> rootJson;
	inFile.close();

	m_sMapRootPath = mapDir.generic_string();
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	QueueAllChunkModelReleases();
	m_Chunks.clear();

	if (rootJson.contains("chunkSize"))
	{
		const auto& chunkSize = rootJson["chunkSize"];
		m_vChunkSize = _float3{ chunkSize[0], chunkSize[1], chunkSize[2] };
	}

	if (!rootJson.contains("chunks"))
	{
		return S_OK;
	}

	for (const auto& chunkMetaJson : rootJson["chunks"])
	{
		const MAPCHUNK_COORD coord = ReadCoordJson(chunkMetaJson["coord"]);
		MAPCHUNK chunk{};
		chunk.coord = coord;
		chunk.bounds = MakeChunkBoundingBox(coord);
		chunk.filePath = chunkMetaJson.value("file", (std::filesystem::path("chunks") / ChunkFileName(coord)).generic_string());
		chunk.loadState = EChunkLoadState::Unloaded;
		chunk.saveState = EChunkSaveState::Saved;

		m_Chunks.emplace(coord, std::move(chunk));
	}

	return S_OK;
}

HRESULT CMapManager::LoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	MAPCHUNK& chunk = iter->second;
	if (chunk.loadState == EChunkLoadState::Loaded)
	{
		return S_OK;
	}

	std::filesystem::path chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.filePath;
	if (chunk.filePath.empty())
	{
		chunk.filePath = (std::filesystem::path("chunks") / ChunkFileName(coord)).generic_string();
		chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.filePath;
	}

	std::ifstream inFile(chunkPath.string());
	if (!inFile.is_open())
	{
		return E_FAIL;
	}

	nlohmann::ordered_json chunkJson;
	inFile >> chunkJson;
	inFile.close();

	chunk.hObjects.clear();
	chunk.loadState = EChunkLoadState::Loading;

	std::vector<MAP_MESH_OBJECT_LOAD_DESC> objectDescs;
	objectDescs.reserve(chunkJson["objects"].size());
	for (const auto& objectJson : chunkJson["objects"])
	{
		if (auto desc = MakeMapMeshLoadDesc(objectJson))
			objectDescs.push_back(std::move(desc.value()));
	}

	if (FAILED(AcquireChunkModelResources(chunk, objectDescs)))
	{
		chunk.loadState = EChunkLoadState::Unloaded;
		return E_FAIL;
	}

	for (const auto& objectJson : chunkJson["objects"])
	{
		if (auto hObject = CreateMapMeshObjectFromJson(objectJson))
		{
			chunk.hObjects.push_back(hObject.value());
		}
	}

	chunk.bounds = MakeChunkBoundingBox(coord);
	chunk.loadState = EChunkLoadState::Loaded;
	chunk.saveState = EChunkSaveState::Saved;

	chunk.octreeNode.reset();
	chunk.octreeNode = COctreeNode::Create(chunk.bounds, 0);
	if (chunk.octreeNode)
	{
		chunk.octreeNode->BuildOctree(chunk.hObjects);
	}

	/*----------- 광윤 추가 -----------*/
	if (!chunk.hObjects.empty())
	{
		const BoundingBox& ChangedBounds = chunk.octreeNode ? chunk.octreeNode->GetCullingBoundingBox() : chunk.bounds;
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	/*---------------------------------*/

	return S_OK;
}

HRESULT CMapManager::UnLoadChunk(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
	{
		return E_FAIL;
	}

	MAPCHUNK& chunk = iter->second;
	if (chunk.saveState == EChunkSaveState::Unsaved)
	{
		return E_FAIL;
	}


	/*----------- 광윤 추가 -----------*/
	const _bool bHadObjects = !chunk.hObjects.empty();
	const BoundingBox RemovedBounds = chunk.octreeNode ? chunk.octreeNode->GetCullingBoundingBox() : chunk.bounds;
	/*---------------------------------*/


	chunk.loadState = EChunkLoadState::Unloading;

	for (const auto& handle : chunk.hObjects)
	{
		if (auto* pObj = CGameInstance::Get().GetGameObjectByHandle(handle))
		{
			pObj->SetPendingDestroyCascade(true);
		}
	}

	QueueChunkModelRelease(chunk);
	chunk.hObjects.clear();
	chunk.octreeNode.reset();
	chunk.loadState = EChunkLoadState::Unloaded;

	/*----------- 광윤 추가 -----------*/
	if (bHadObjects)	CGameInstance::Get().Notify_StaticShadowSceneChanged(RemovedBounds);
	/*---------------------------------*/


	return S_OK;
}

_float3 CMapManager::GetChunkCenter(const MAPCHUNK_COORD& coord)
{
	return _float3(
			{
				(coord.x + 0.5f) * m_vChunkSize.x,
				(coord.y + 0.5f) * m_vChunkSize.y,
				(coord.z + 0.5f) * m_vChunkSize.z,
			});
}

BoundingBox CMapManager::MakeChunkBoundingBox(const MAPCHUNK_COORD& coord)
{
	_float3 center = GetChunkCenter(coord);
	_float3 extents = {
		m_vChunkSize.x * 0.5f,
		m_vChunkSize.y * 0.5f,
		m_vChunkSize.z * 0.5f
	};
	return BoundingBox(center, extents);
}

MAPCHUNK_COORD CMapManager::WorldToChunkCoord(const _float3& pos) const
{
	return {
		static_cast<int64_t>(std::floor(pos.x / m_vChunkSize.x)),
		static_cast<int64_t>(std::floor(pos.y / m_vChunkSize.y)),
		static_cast<int64_t>(std::floor(pos.z / m_vChunkSize.z))
	};
}

void CMapManager::RebuildChunks()
{
	m_MapGeneration.fetch_add(1, std::memory_order_acq_rel);
	std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash> prevChunks;
	prevChunks.reserve(m_Chunks.size());
	//prevChunks.insert(m_Chunks.begin(), m_Chunks.end());

	for (auto& [coord, chunk] : m_Chunks)
	{
		MAPCHUNK prev{};
		prev.coord = chunk.coord;
		prev.bounds = chunk.bounds;
		prev.loadState = chunk.loadState;
		prev.saveState = chunk.saveState;
		prev.filePath = chunk.filePath;
		prev.modelResources = std::move(chunk.modelResources);

		prevChunks.emplace(coord, std::move(prev));
	}

	m_Chunks.clear();
	m_Chunks.reserve(prevChunks.size());

	for (const auto& [coord, prevChunk] : prevChunks)
	{
		MAPCHUNK rebuiltChunk{};
		rebuiltChunk.coord = prevChunk.coord;
		rebuiltChunk.bounds = prevChunk.bounds;
		rebuiltChunk.loadState = prevChunk.loadState;
		rebuiltChunk.saveState = prevChunk.saveState;
		rebuiltChunk.filePath = prevChunk.filePath;
		rebuiltChunk.modelResources = prevChunk.modelResources;

		rebuiltChunk.hObjects.clear();
		rebuiltChunk.loadState = prevChunk.loadState; //EChunkLoadState::Unloaded;
		m_Chunks.emplace(coord, std::move(rebuiltChunk));
	}

	const auto& layers = CGameInstance::Get().GetGameObjectLayers();

	for (const auto& [layerName, layer] : layers)
	{
		for (const auto& handle : layer)
		{
			CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(handle);

			if (pObj == nullptr)
				continue;

			const auto& pos = pObj->GetTransform().GetPosition();

			MAPCHUNK_COORD coord = WorldToChunkCoord(pos);

			auto prevIter = prevChunks.find(coord);
			const bool isKnownChunk = prevIter != prevChunks.end();

			auto& chunk = m_Chunks[coord];
			if (!isKnownChunk)
			{
				chunk.coord = coord;
				chunk.bounds = MakeChunkBoundingBox(coord);
				chunk.saveState = EChunkSaveState::Unsaved;
				chunk.filePath.clear();
				chunk.octreeNode.reset();
			}

			chunk.hObjects.push_back(handle);
			chunk.loadState = EChunkLoadState::Loaded;
		}
	}

	for (auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.loadState != EChunkLoadState::Loaded)
			continue;

		chunk.bounds = MakeChunkBoundingBox(coord);
		chunk.octreeNode = COctreeNode::Create(chunk.bounds, 0);
		if (chunk.octreeNode)
		{
			chunk.octreeNode->BuildOctree(chunk.hObjects);
		}
	}

}

HRESULT CMapManager::RegisterMapMeshObject(const CHandle& hObject)
{
	CMapMeshObject* pObj = CGameInstance::Get().GetGameObjectByHandleT<CMapMeshObject>(hObject);
	if (pObj == nullptr)
	{
		return E_FAIL;
	}

	/*----------- 광윤 추가 -----------*/
	pObj->GetTransform().Update();
	/*---------------------------------*/

	const MAPCHUNK_COORD coord = WorldToChunkCoord(pObj->GetTransform().GetPosition());
	auto& chunk = m_Chunks[coord];

	chunk.coord = coord;
	chunk.bounds = MakeChunkBoundingBox(coord);
	chunk.loadState = EChunkLoadState::Loaded;
	chunk.saveState = EChunkSaveState::Unsaved; // 새 오브젝트가 배치되면서 변질된 chunk니까 Unsaved되는게 맞음

	if (!HasHandle(chunk.hObjects, hObject))
	{
		chunk.hObjects.push_back(hObject);
	}

	if (!chunk.octreeNode)
	{
		chunk.octreeNode = COctreeNode::Create(chunk.bounds, 0);
	}

	if (chunk.octreeNode)
	{
		chunk.octreeNode->BuildOctree(chunk.hObjects);
	}

	pObj->SetRenderEnable(true);

	

	/*----------- 광윤 추가 -----------*/
	BoundingBox ChangedBounds{};

	if (pObj->GetShadowBounds(ChangedBounds)) {
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	else {
		CGameInstance::Get().Notify_StaticShadowSceneChanged(chunk.bounds);
	}
	/*---------------------------------*/
	return S_OK;
}

std::vector<CHandle> CMapManager::CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const
{
	std::vector<CHandle> candidates;

	for (const auto& [coord, chunk] : m_Chunks)
	{
		if (chunk.loadState != EChunkLoadState::Loaded || !chunk.octreeNode)
			continue;

		chunk.octreeNode->CollectRayCandidates(rayOrigin, rayDirection, candidates);
	}

	return candidates;
}

#ifdef _DEBUG

HRESULT CMapManager::RenderDebugMapChunk()
{
	if (m_bDebugDrawMapChunk == false)
		return E_FAIL;

	CCameraObject* cam = CGameInstance::Get().GetActiveCamera();

	if (cam == nullptr)
		return E_FAIL;

	const auto view = cam->GetView();
	const auto proj = cam->GetProj();

	//const XMVECTOR octreeDepthColors[] =
	//{
	//	Colors::Cyan,
	//	Colors::DeepSkyBlue,
	//	Colors::Yellow,
	//	Colors::Orange,
	//	Colors::Black,
	//	Colors::White,
	//};
	//const size_t octreeDepthColorCount = sizeof(octreeDepthColors) / sizeof(octreeDepthColors[0]);

	for (const auto& [coord, mapChunk] : m_Chunks)
	{
		if (mapChunk.loadState == EChunkLoadState::Loaded && mapChunk.octreeNode)
		{
			std::vector<OCTREE_DEBUG_BOUNDS> octreeBounds;
			mapChunk.octreeNode->CollectDebugBounds(octreeBounds);

			for (const auto& nodeBounds : octreeBounds)
			{
				//const FXMVECTOR nodeColor = octreeDepthColors[
				//	nodeBounds.depth % octreeDepthColorCount];
				DrawBox(nodeBounds.bounds, Colors::Red, view, proj, XMMatrixIdentity());
			}
		}
		FXMVECTOR color = mapChunk.loadState == EChunkLoadState::Loaded ? Colors::Lime : Colors::Red;
		DrawBox(mapChunk.bounds, color, view, proj, XMMatrixIdentity());
	}

	//// 카메라 주변 3x3x3 스트리밍 대상 청크 표시
	//const auto& camPos = cam->GetTransform().GetPosition();
	//const MAPCHUNK_COORD camCoord = WorldToChunkCoord(camPos);

	//for (int64_t y = -1; y <= 1; ++y)
	//{
	//	for (int64_t z = -1; z <= 1; ++z)
	//	{
	//		for (int64_t x = -1; x <= 1; ++x)
	//		{
	//			MAPCHUNK_COORD coord
	//			{
	//				camCoord.x + x,
	//				camCoord.y + y,
	//				camCoord.z + z
	//			};

	//			BoundingBox bounds = MakeChunkBoundingBox(coord);

	//			// 빨간색 : 현재 카메라 주변 스트리밍 범위
	//			DrawBox(bounds, Colors::Red, view, proj, XMMatrixIdentity());
	//		}
	//	}
	//}

	return S_OK;
}

void CMapManager::DrawBox(const DirectX::BoundingBox& box, DirectX::FXMVECTOR color, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world)
{
	_effect->SetView(view);
	_effect->SetProjection(projection);
	_effect->SetWorld(world);
	CGameInstance::Get().GetGraphicDeviceContext()->IASetInputLayout(_inputLayout.Get());

	// 그리기
	_effect->Apply(CGameInstance::Get().GetGraphicDeviceContext().Get());
	_batch->Begin();

	// DirectXTK의 내장 Draw 함수 호출
	DX::Draw(_batch.get(), box, color);

	_batch->End();
}
#endif

HRESULT CMapManager::RequestLoadChunkAsync(const MAPCHUNK_COORD& coord)
{
	auto iter = m_Chunks.find(coord);
	if (iter == m_Chunks.end())
		return E_FAIL;

	MAPCHUNK& chunk = iter->second;

	if (chunk.loadState == EChunkLoadState::Loaded || chunk.loadState == EChunkLoadState::Loading)
	{
		return S_OK;
	}

	if (chunk.filePath.empty())
	{
		chunk.filePath = (std::filesystem::path("chunks") / ChunkFileName(coord)).generic_string();
	}

	const std::filesystem::path chunkPath = std::filesystem::path(m_sMapRootPath) / chunk.filePath;
	const uint64_t mapGeneration = m_MapGeneration.load(std::memory_order_acquire);

	// 큐에 넣기 전에 메인스레드에서 Loading으로 바꿔야 중복 요청이 안 들어감
	chunk.loadState = EChunkLoadState::Loading;
	m_AsyncChunkLoadsInFlight.fetch_add(1, std::memory_order_acq_rel);

	const _bool queued = CGameInstance::Get().WorkerEnqueue("LoadChunk", [this, coord, chunkPath, mapGeneration]()
		{
			PENDING_CHUNK_LOAD_RESULT result{};
			result.coord = coord;
			result.mapGeneration = mapGeneration;

			try
			{
				std::ifstream inFile(chunkPath.string());
				if (!inFile.is_open())
				{
					result.hr = E_FAIL;
				}
				else
				{
					nlohmann::ordered_json chunkJson;
					inFile >> chunkJson;

					for (const auto& objectJson : chunkJson["objects"])
					{
						if (auto desc = MakeMapMeshLoadDesc(objectJson))
							result.objects.push_back(std::move(desc.value()));
					}

					if (result.mapGeneration != m_MapGeneration.load(std::memory_order_acquire))
						result.hr = E_ABORT;
					else
						result.hr = PreloadChunkModelResources(result);
				}
			}
			catch (const std::exception&)
			{
				result.hr = E_FAIL;
			}

			{
				std::lock_guard<std::mutex> lock(m_LoadResultMutex);
				m_LoadResults.push_back(std::move(result));
			}
			m_AsyncChunkLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		});

	if (!queued)
	{
		m_AsyncChunkLoadsInFlight.fetch_sub(1, std::memory_order_acq_rel);
		chunk.loadState = EChunkLoadState::Unloaded;
		return E_FAIL;
	}

	return S_OK;
}
void CMapManager::ProcessLoadedChunkResults()
{
	ZoneScopedN("ChunkApplyBudget");
	std::vector<PENDING_CHUNK_LOAD_RESULT> results;

	{
		std::lock_guard<std::mutex> lock(m_LoadResultMutex);
		results.swap(m_LoadResults);
	}

	for (auto& result : results)
	{
		auto state = std::make_unique<PENDING_CHUNK_APPLY_STATE>();
		state->result = std::move(result);
		m_ChunkApplyQueue.push_back(std::move(state));
	}

	constexpr auto APPLY_BUDGET = std::chrono::microseconds(2000);
	const auto deadline = std::chrono::steady_clock::now() + APPLY_BUDGET;
	while (!m_ChunkApplyQueue.empty())
	{
		_bool completed = false;
		ContinueApplyLoadedChunkResult(*m_ChunkApplyQueue.front(), deadline, completed);
		if (completed)
			m_ChunkApplyQueue.pop_front();

		if (!completed || std::chrono::steady_clock::now() >= deadline)
			break;
	}
}

HRESULT CMapManager::ContinueApplyLoadedChunkResult(PENDING_CHUNK_APPLY_STATE& state, const std::chrono::steady_clock::time_point& deadline, _bool& completed)
{
	completed = false;
	if (state.result.mapGeneration != m_MapGeneration.load(std::memory_order_acquire))
	{
		if (!state.initialized)
			ReleasePendingModelResources(state.result);
		completed = true;
		return S_OK;
	}

	auto iter = m_Chunks.find(state.result.coord);
	if (iter == m_Chunks.end())
	{
		if (!state.initialized)
			ReleasePendingModelResources(state.result);
		completed = true;
		return E_FAIL;
	}

	MAPCHUNK& chunk = iter->second;
	if (FAILED(state.result.hr))
	{
		chunk.hObjects.clear();
		chunk.octreeNode.reset();
		chunk.loadState = EChunkLoadState::Unloaded;
		completed = true;
		return E_FAIL;
	}

	if (m_bChunkStreaming && !IsChunkInStreamingRange(state.result.coord))
	{
		if (state.initialized)
			UnLoadChunk(state.result.coord);
		else
		{
			ReleasePendingModelResources(state.result);
			chunk.hObjects.clear();
			chunk.octreeNode.reset();
			chunk.loadState = EChunkLoadState::Unloaded;
		}
		completed = true;
		return S_OK;
	}

	if (!state.initialized)
	{
		chunk.hObjects.clear();
		if (FAILED(AcquirePreloadedChunkModelResources(chunk, state.result)))
		{
			chunk.octreeNode.reset();
			chunk.loadState = EChunkLoadState::Unloaded;
			completed = true;
			return E_FAIL;
		}
		state.initialized = true;
	}

	do
	{
		if (state.nextObjectIndex >= state.result.objects.size())
			break;

		const auto& objectDesc = state.result.objects[state.nextObjectIndex++];
		CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectDesc.objectTag;
		desc.protoGroupTag = objectDesc.protoGroup;
		desc.prototypeTag = objectDesc.prototype;
		desc.modelGroupTag = objectDesc.modelGroup;
		desc.modelResTag = objectDesc.model;
		desc.windDesc = objectDesc.windDesc;

		auto hObject = CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			objectDesc.layer,
			&desc);

		if (hObject.has_value())
		{
			if (auto* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject.value()))
			{
				auto& transform = pObject->GetTransform();
				transform.SetPosition(objectDesc.position);
				transform.SetQuaternion(objectDesc.rotation);
				transform.SetScale(objectDesc.scale);
				chunk.hObjects.push_back(hObject.value());
			}
		}
	}
	while (std::chrono::steady_clock::now() < deadline);

	if (state.nextObjectIndex < state.result.objects.size())
		return S_OK;

	chunk.bounds = MakeChunkBoundingBox(state.result.coord);
	chunk.loadState = EChunkLoadState::Loaded;
	chunk.saveState = EChunkSaveState::Saved;
	chunk.octreeNode = COctreeNode::Create(chunk.bounds, 0);
	if (chunk.octreeNode)
		chunk.octreeNode->BuildOctree(chunk.hObjects);

	completed = true;
	return S_OK;
}

HRESULT CMapManager::ApplyLoadedChunkResult(const PENDING_CHUNK_LOAD_RESULT& result)
{
	auto iter = m_Chunks.find(result.coord);
	if (iter == m_Chunks.end())
		return E_FAIL;

	MAPCHUNK& chunk = iter->second;

	if (FAILED(result.hr))
	{
		chunk.hObjects.clear();
		chunk.octreeNode.reset();
		chunk.loadState = EChunkLoadState::Unloaded;
		return E_FAIL;
	}

	if (m_bChunkStreaming)
	{
		// 스트리밍 모드일때, 워커스레드가 뒤늦게 로드해준 Chunk 결과가 지금 카메라 위치를 보고 유효한 결과인지 판단
		if (IsChunkInStreamingRange(result.coord) == false)
		{
			chunk.hObjects.clear();
			chunk.octreeNode.reset();
			chunk.loadState = EChunkLoadState::Unloaded;
			return S_OK;
		}
	}

	chunk.hObjects.clear();
	if (FAILED(AcquireChunkModelResources(chunk, result.objects)))
	{
		chunk.octreeNode.reset();
		chunk.loadState = EChunkLoadState::Unloaded;
		return E_FAIL;
	}

	for (const auto& objectDesc : result.objects)
	{
		CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
		desc.sObjectTag = objectDesc.objectTag;
		desc.protoGroupTag = objectDesc.protoGroup;
		desc.prototypeTag = objectDesc.prototype;
		desc.modelGroupTag = objectDesc.modelGroup;
		desc.modelResTag = objectDesc.model;
		desc.windDesc = objectDesc.windDesc;

		auto hObject = CGameInstance::Get().AddGameObjectToLayer(
			desc.protoGroupTag,
			desc.prototypeTag,
			objectDesc.layer,
			&desc);

		if (!hObject.has_value())
			continue;

		auto* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject.value());
		if (pObject == nullptr)
			continue;

		auto& transform = pObject->GetTransform();
		transform.SetPosition(objectDesc.position);
		transform.SetQuaternion(objectDesc.rotation);
		transform.SetScale(objectDesc.scale);

		chunk.hObjects.push_back(hObject.value());
	}

	chunk.bounds = MakeChunkBoundingBox(result.coord);
	chunk.loadState = EChunkLoadState::Loaded;
	chunk.saveState = EChunkSaveState::Saved;
	chunk.octreeNode = COctreeNode::Create(chunk.bounds, 0);
	if (chunk.octreeNode)
	{
		chunk.octreeNode->BuildOctree(chunk.hObjects);
	}


	/*----------- 광윤 추가 -----------*/
	if (!chunk.hObjects.empty()) {
		const BoundingBox& ChangedBounds = chunk.octreeNode? chunk.octreeNode->GetCullingBoundingBox() : chunk.bounds;
		CGameInstance::Get().Notify_StaticShadowSceneChanged(ChangedBounds);
	}
	/*---------------------------------*/
	
	return S_OK;
}

_bool CMapManager::IsChunkInStreamingRange(const MAPCHUNK_COORD& coord)
{
	const auto pCam = CGameInstance::Get().GetActiveCamera();
	if (pCam == nullptr)
		return false;

	const auto& pos = pCam->GetTransform().GetPosition();
	MAPCHUNK_COORD cameraCoord = WorldToChunkCoord(pos);

	const int64_t dx = std::llabs(cameraCoord.x - coord.x);
	const int64_t dy = std::llabs(cameraCoord.y - coord.y);
	const int64_t dz = std::llabs(cameraCoord.z - coord.z);

	// Discard an async result if it arrived after leaving the load range.
	return dx <= STREAM_LOAD_RADIUS
		&& dy <= STREAM_LOAD_RADIUS
		&& dz <= STREAM_LOAD_RADIUS;
}

UPtr<CMapManager> CMapManager::Create()
{
	auto pInstance = ToUPtr(new CMapManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

void CMapManager::Free()
{
	CEngineBase::Free();
}


