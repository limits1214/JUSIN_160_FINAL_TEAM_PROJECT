#pragma once

#ifdef _DEBUG
#include "DebugDraw.h"
#include <directxtk/Effects.h>
#endif

#include <functional>
#include <chrono>
#include <deque>
#include "Engine_Base.h"

NS_BEGIN(Engine)
class COctreeNode;
class CCameraObject;

enum class EChunkLoadState
{
	Unloading,  // 언로드 요청 중
	Unloaded,   // 메타만 있고 월드에는 없음
	Loading,    // 비동기 로딩 요청 중
	Loaded,     // 월드에 올라와 있음
};

enum class EChunkSaveState
{
	Unsaved,    // 아직 저장 파일 없음
	Saved,      // 저장 파일 있음
};

struct MAP_MODEL_RESOURCE_KEY
{
	std::string group;
	std::string tag;

	bool operator==(const MAP_MODEL_RESOURCE_KEY& rhs) const
	{
		return group == rhs.group && tag == rhs.tag;
	}
};

struct MAP_MODEL_RESOURCE_KEY_HASH
{
	size_t operator()(const MAP_MODEL_RESOURCE_KEY& key) const
	{
		const size_t groupHash = std::hash<std::string>{}(key.group);
		const size_t tagHash = std::hash<std::string>{}(key.tag);
		return groupHash ^ (tagHash << 1);
	}
};

typedef struct tagMapChunk
{
	MAPCHUNK_COORD coord{};
	std::vector<CHandle> hObjects{};
	std::vector<MAP_MODEL_RESOURCE_KEY> modelResources{};
	BoundingBox bounds{};
	UPtr<COctreeNode> octreeNode;

	EChunkLoadState loadState = EChunkLoadState::Unloaded;
	EChunkSaveState saveState = EChunkSaveState::Unsaved;

	std::string filePath;
}MAPCHUNK;

struct tagMapChunkCoordHash
{
	size_t operator()(const MAPCHUNK_COORD& coord) const
	{
		size_t h1 = std::hash<int64_t>{}(coord.x);
		size_t h2 = std::hash<int64_t>{}(coord.y);
		size_t h3 = std::hash<int64_t>{}(coord.z);

		return h1 ^ (h2 << 1) ^ (h3 << 3);
	}
};

// 전방선언
struct MAP_MESH_OBJECT_LOAD_DESC;
struct PENDING_CHUNK_LOAD_RESULT;
struct PENDING_CHUNK_APPLY_STATE;

constexpr _float3 DEFAULT_MAP_CHUNK_SIZE{ 150.f, 150.f, 150.f };

class ENGINE_DLL CMapManager : public CEngineBase
{
public:
	CMapManager(const CMapManager&) = delete;
	CMapManager& operator=(const CMapManager& rhs) = delete;

private:
	CMapManager();
	~CMapManager() override;

private:
	HRESULT Initialize();

public:
	void PriorityUpdate(_float fTimeDelta);
	void Update(_float fTimeDelta);
	void LateUpdate(_float fTimeDelta);

public:
	void ClearAllChunk(); // 씬 전환할 때 Clear하셈

public:
	HRESULT SaveMap(const std::string& path); // 메타 + 모든 청크 저장
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true); // 메타 + 모든 청크 불러오기
	HRESULT SaveChunk(const MAPCHUNK_COORD& coord, const std::string& chunkPath); // 청크 단위 저장

	HRESULT LoadMapData(const std::string& path);
	HRESULT LoadChunk(const MAPCHUNK_COORD& coord); // 메인스레드 동기 로드, 저장/툴용
	HRESULT UnLoadChunk(const MAPCHUNK_COORD& coord);

	// 모델 태그 → 모델 .bin 파일 경로
	void SetMapModelResourceIndex(const std::filesystem::path& staticModelRoot, const std::string& resourceGroup, std::unordered_map<std::string, std::filesystem::path> modelPaths);

// ---------------------------------MapChunk-----------------------------------
public:
	void RebuildChunks();
	HRESULT RegisterMapMeshObject(const CHandle& hObject);
	std::vector<CHandle> CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const;
	const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& GetChunks() const { return m_Chunks; }
	const _float3& GetChunkSize() const { return m_vChunkSize; }
	void SetChunkStreaming(_bool enable) { m_bChunkStreaming = enable; }
	_bool IsChunkStreaming() const { return m_bChunkStreaming; } 

private:
	_float3 GetChunkCenter(const MAPCHUNK_COORD& coord);
	BoundingBox MakeChunkBoundingBox(const MAPCHUNK_COORD& coord);
	MAPCHUNK_COORD WorldToChunkCoord(const _float3& pos) const;
	std::vector<MAPCHUNK_COORD> GetChunksAroundCamera(
		const CCameraObject* pCamera, int64_t radius) const;
	void UnloadChunksOutsideRange(const std::vector<MAPCHUNK_COORD>& neededChunks);
	void RequestNeededChunkLoads(const std::vector<MAPCHUNK_COORD>& neededChunks);
	void CullLoadedChunksByCameraFrustum(const std::vector<MAPCHUNK_COORD>& neededChunks, const BoundingFrustum& boundingFrustum);


	HRESULT AcquireChunkModelResources(MAPCHUNK& chunk, const std::vector<MAP_MESH_OBJECT_LOAD_DESC>& objects);
	HRESULT PreloadChunkModelResources(PENDING_CHUNK_LOAD_RESULT& result);
	HRESULT AcquirePreloadedChunkModelResources(MAPCHUNK& chunk, const PENDING_CHUNK_LOAD_RESULT& result);
	void ReleasePendingModelResources(const PENDING_CHUNK_LOAD_RESULT& result);

	// 청크가 런타임에 로드될 때 m_MapModelPaths에서 파일 경로를 찾고 해당 모델만 로드
	HRESULT EnsureModelResourceLoaded(const MAP_MODEL_RESOURCE_KEY& key);

	void QueueChunkModelRelease(MAPCHUNK& chunk);
	void QueueAllChunkModelReleases();
	void ProcessDeferredModelReleases();
private:
	static constexpr int64_t STREAM_LOAD_RADIUS = 2;   // 5 x 5 x 5
	static constexpr int64_t STREAM_UNLOAD_RADIUS = 3; // 7 x 7 x 7
	_float3 m_vChunkSize = DEFAULT_MAP_CHUNK_SIZE;
	std::string m_sMapRootPath;

private:
	std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash> m_Chunks;
	_bool m_bChunkStreaming = true;

	std::atomic_uint64_t m_MapGeneration{};
	std::atomic_uint32_t m_AsyncChunkLoadsInFlight{};

	// 모델 태그 → 모델 .bin 파일 경로
	std::filesystem::path m_MapModelStaticRoot;
	std::string m_MapModelResourceGroup;
	std::unordered_map<std::string, std::filesystem::path> m_MapModelPaths;

	std::mutex m_MapModelResourceIndexMutex;
	std::mutex m_ModelResourceLoadMutex;
	std::mutex m_PendingModelRefMutex;

	// 완전히 생성된 청크들이 모델을 사용 중
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, size_t, MAP_MODEL_RESOURCE_KEY_HASH> m_ModelChunkRefCounts;
	// 워커 로드는 끝났지만 청크 생성은 아직 안 끝남
	std::unordered_map<MAP_MODEL_RESOURCE_KEY, size_t, MAP_MODEL_RESOURCE_KEY_HASH> m_PendingModelRefCounts;


	std::vector<std::vector<MAP_MODEL_RESOURCE_KEY>> m_DeferredModelReleases;
	std::vector<MAP_MODEL_RESOURCE_KEY> m_DeferredUnusedModelReleases;


// ---------------------------------MapChunk-----------------------------------

#ifdef _DEBUG
	// Debug Draw
	// Debug Draw를 위한 도구들
	// static으로 관리
	static std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _batch;
	static std::unique_ptr<DirectX::BasicEffect> _effect;
	static ComPtr<ID3D11InputLayout> _inputLayout;
public:
	HRESULT RenderDebugMapChunk();
	void SetDebugDrawMapChunk(_bool draw) { m_bDebugDrawMapChunk = draw; }
private:
	void DrawBox(const DirectX::BoundingBox& box, DirectX::FXMVECTOR color, DirectX::CXMMATRIX view, DirectX::CXMMATRIX projection, DirectX::CXMMATRIX world);

private:
	_bool m_bDebugDrawMapChunk = false;
#endif

// -------------------------------Worker---------------------------------------
public:
	HRESULT RequestLoadChunkAsync(const MAPCHUNK_COORD& coord); // 워커에게 청크 로딩 요청 // 스트리밍용 비동기 요청
	void ProcessLoadedChunkResults();

private:
	HRESULT ApplyLoadedChunkResult(const PENDING_CHUNK_LOAD_RESULT& result); // 실제 오브젝트 생성 // 메인스레드 월드 반영
	_bool IsChunkInStreamingRange(const MAPCHUNK_COORD& coord); // 나중에 도착한 Chunk로딩 결과가 유효한지 확인하기 위한 함수

private:
	HRESULT ContinueApplyLoadedChunkResult(PENDING_CHUNK_APPLY_STATE& state, const std::chrono::steady_clock::time_point& deadline, _bool& completed);
	std::mutex m_LoadResultMutex{};
	std::deque<std::unique_ptr<PENDING_CHUNK_APPLY_STATE>> m_ChunkApplyQueue{};
	std::vector<PENDING_CHUNK_LOAD_RESULT> m_LoadResults{}; //워커가 로딩한 결과모음
// -------------------------------Worker---------------------------------------

public:
	static UPtr<CMapManager> Create();

public:
	void Free() override;

};

// 워커스레드는 파일읽고, 데이터 포장까지만
// 실제 월드 반영은 메인스레드가
struct MAP_MESH_OBJECT_LOAD_DESC
{
	std::string objectTag;
	std::string protoGroup;
	std::string prototype;
	std::string modelGroup;
	std::string model;
	std::string layer;

	_float3 position{};
	_float4 rotation{ 0.f, 0.f, 0.f, 1.f };
	_float3 scale{ 1.f, 1.f, 1.f };
	WIND_DESC windDesc{};
};

struct PENDING_CHUNK_LOAD_RESULT
{
	MAPCHUNK_COORD coord{};
	uint64_t mapGeneration{};
	HRESULT hr = E_FAIL;
	std::vector<MAP_MESH_OBJECT_LOAD_DESC> objects{};
	std::vector<MAP_MODEL_RESOURCE_KEY> modelResources{};
};
NS_END



