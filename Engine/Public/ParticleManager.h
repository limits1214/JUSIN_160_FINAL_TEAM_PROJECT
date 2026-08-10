#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)
class CParticle;
class CParticleShaderCache;

struct PARTICLE_LOOP_REQUEST
{
    StringID                          sGroupTag;
    StringID                          sTypeTag;
    std::vector<PARTICLE_SPAWN_DATA>  vecSpawnData;
    _float                            fSpawnInterval = 0.1f;
    _float                            fElapsed = 0.f;
	uint32_t						  iUserId;
};

typedef struct tagParticlePreset
{
	std::string presetName;
	StringID sGroupTag;     
	StringID sTypeTag;
	uint32_t groupTypeIndex = 0;      // 0:PARTICLE_CPU, 1:PARTICLE_GPU, 2:BEAM_CPU, 3:RIBBON_CPU
	uint32_t whatKindFilterIndex = 0;
	_float maxLife = 1.f;
	_float3 fStartSize = {1.f,1.f ,1.f };
	_float3 fEndSize = { 1.f ,1.f ,1.f };
	_float4   rotation = { 0.f, 0.f, 0.f, 0.f };
	_float3 velocity = { 0,0,0 };
	_float3 originalVelocity = { 0,0,0 };
	//EndColor는 보간 로직 만든 뒤에 추가
	_float4 StartColor = { 1.f, 1.f, 1.f, 1.f };
	_float4 originalEmissive = { 1.f, 1.f, 1.f, 0.f };
	_float4 Emissive = { 1.f, 1.f, 1.f, 0.f };
	_float4 endEmissive = { 1.f, 1.f, 1.f, 0.f };
	uint32_t iBehaviorType = 0;
	_float fStopSizeTime = 0;
	_bool bKeepRotate{};
	_float3 rotationAxis{};
	_float rotationSpeed{};
} PARTICLE_PRESET;


struct TextureSlotState
{
	std::string label;              // "Diffuse", "Normal", "Distortion", "Noise"
	char szTextureID1[128] = "SAMPLE_CLINET_TEXTURE";
	char szTextureID2[128] = "";
	int selectedIndex = -1;
	std::string selectedPath;
};


struct PARTICLE_EFFECT_PRESET
{
    std::string sEffectName;              // 저장 시 식별용 이름 (예: "Explosion_Fire")
    std::vector<SPAWN_COMMAND> commands;  // 이 이펙트를 구성하는 여러 스폰 명령
};

class ENGINE_DLL CParticleManager final : public CEngineBase, public IRenderable
{
private:
    explicit CParticleManager();
    virtual ~CParticleManager();
public:
    CParticleManager(const CParticleManager&) = delete;
    CParticleManager& operator=(const CParticleManager& rhs) = delete;
public:
	HRESULT Initialize();
    void Update(_float fTimeDelta);
    HRESULT Render(ID3D11DeviceContext* pContext, const RENDER_CTX& ctx) override;
    void UpdateGUI();
    bool HasRenderPass(RENDERPASS ePass) const override { return ePass == RENDERPASS::DEFAULT; };

public:
    // 사전 등록 - [대분류][소분류]로 저장
    HRESULT Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<CParticle> particle);



    // 정확히 지정해서 스폰
    HRESULT Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
        uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData, 
        _bool bLoop = false, _float fSpawnInterval = 0.1f);
	uint32_t Spawn(const std::string& strJsonPath, const _float4x4& worldMat, const _fvector endPos);

	

	//실제 스폰 함수
	std::vector<SPAWN_COMMAND> Parse_Command(const std::string& strJsonPath);
	uint32_t Spawn(const std::vector<SPAWN_COMMAND>& templateCommands, const _float4x4& worldMat, _fvector endPos);





public:
	std::optional<BEAM_HANDLE> SpawnBeam(const StringID& groupTag, const StringID& typeTag, const BEAM_PARAMS& params);
	HRESULT Save_Binary_Json(std::string outpath,
		const std::string& FullPath, const std::string& whatKind,
		const std::string& particleType, const std::string& particleName,
		int iMaxParticles,
		const std::string& VSGroup, const std::string& VSID, const std::string& VSEntryPoint,
		const std::string& PSGroup, const std::string& PSID, const std::string& PSEntryPoint,
		const std::string& sGroupTag, const std::string& sResTag,
		const std::string& textureID1, const std::string& textureID2,
		const std::string& viBufferID1, const std::string& viBufferID2,
		int RowCount, int ColCount,
		const std::string& normalTexID1 = "", const std::string& normalTexID2 = "",
		const std::string& distortionTexID1 = "", const std::string& distortionTexID2 = "",
		const std::string& noiseTexID1 = "", const std::string& noiseTexID2 = "",
		const std::string& normalTexPath = "",
		const std::string& distortionTexPath = "",
		const std::string& noiseTexPath = "",
		const std::string& hdrTexID1 = "",
		const std::string& hdrTexID2 = "",
		const std::string& hdrTexPath = "",
		const std::string& hdrNormalTexID1 = "",  
		const std::string& hdrNormalTexID2 = "",  
		const std::string& hdrNormalTexPath = "",
		const std::string& AnyTexID1 = "",  
		const std::string& AnyTexID2 = "",
		const std::string& AnyTexPath = "",
		int iSelectedBlend = 0,
		_bool bShrinkWidth = true,
		_float fMaxduration = 0,
		int iTrailBehaviorMode = 1);

	HRESULT Save_Beam_Json(std::string outpath, const std::string& FullPath, const std::string& whatKind, const std::string& particleType,
		const std::string& particleName, int iMaxParticles, const std::string& VSGroup, const std::string& VSID, const std::string& VSEntryPoint,
		const std::string& PSGroup, const std::string& PSID, const std::string& PSEntryPoint, 
		const std::string& textureID1, const std::string& textureID2,
		const std::string& normalTexID1, const std::string& normalTexID2,
		const std::string& distortionTexID1 , const std::string& distortionTexID2 ,
		const std::string& noiseTexID1, const std::string& noiseTexID2,
		const std::string& normalTexPath,
		const std::string& distortionTexPath,
		const std::string& noiseTexPath,
		const std::string& AnyTexID1,
		const std::string& AnyTexID2 ,
		const std::string& AnyTexPath ,
		int RowCount, int ColCount, int iSelectedBlend,
		 uint32_t maxBeams, uint32_t maxDisplacementIterations);
	HRESULT LoadParticleJson(const std::string& strJsonPath);
	ID3D11ShaderResourceView* GetOrLoadTextureThumbnail(const std::string& fullPath);
	HRESULT SaveCommandQueue(const std::string& strJsonPath);
	HRESULT LoadCommandQueue(const std::string& strJsonPath);
	HRESULT LoadParticlePresets(const std::string& strJsonPath);


	HRESULT SaveEffectPreset(const std::string& strJsonPath, const PARTICLE_PRESET& preset);
	HRESULT PlayParticle(const std::string& presetName, const _float3& position, uint32_t count = 1);
	HRESULT DeleteEffectPreset(const std::string& strJsonPath, const std::string& presetName);
	std::vector<PARTICLE_SPAWN_DATA> BuildSpawnData(const PatternParamVariant& v);
	void ApplyStartEndToPattern(PatternParamVariant& pv, _fvector startPos, _fvector endPos);
	void ApplyWorldMatToPattern(PatternParamVariant& pv, FXMMATRIX matWorld);
	std::vector<std::string> ScanBinFolder(const std::string& strBinFolder);
	// 조회 헬퍼
	CParticle* GetParticle(const StringID& sGroupTag, const StringID& sTypeTag) const;
	bool HasGroup(const StringID& sGroupTag) const;
	HRESULT ClearLoopRequests();
	HRESULT DeleteLoopRequests(uint32_t userId);


public:
	void ClearByOwner(uint32_t ownerId);
	void TranslateOwner(uint32_t ownerId, const _float3& delta);
	void TransformOwner(uint32_t ownerId , const _float4x4& deltaMatrixData);
	void SetColorByOwner(uint32_t ownerId, const _float4& color);
	std::vector<std::string> Load_FilePath_ByExtension(const std::filesystem::path& _FolderPath, std::string_view _Extension);
	HRESULT Load_ParticleJsonPackage(const std::vector<std::string>& _FilePathPackage);
	HRESULT AddTrailPoint(const StringID& groupTag, const StringID& typeTag, const _float3& start, const _float3& end);
	HRESULT SetBeamPositions(const BEAM_HANDLE& handle, const _float4& start, const _float4& end);
	HRESULT StopBeam(const BEAM_HANDLE& handle);
	void SetBeamPositionsByOwner(uint32_t ownerId, const _float3& start, const _float3& end);
public:

private:
	void ComboList(_string comboName, _string resourceName, _string& previewName);
    uint32_t ExecuteCommandQueue(std::vector<SPAWN_COMMAND>& queue);
public:
    static UPtr<CParticleManager> Create();
private:
    // [대분류][소분류] -> 파티클 인스턴스
    std::unordered_map<StringID, std::unordered_map<StringID, UPtr<CParticle>>> m_Particles;
	std::unordered_map<StringID, PARTICLE_PRESET> m_ParticlePresets;
    std::vector<PARTICLE_LOOP_REQUEST> m_LoopRequests;
	std::vector<SPAWN_COMMAND> m_vecCommandQueue;
	// 큐 전체를 실행
	std::string m_sLastResultMsg;
	bool m_bLastResultSuccess = false;
	bool bNeedTypeIndexSync = false;
	StringID pendingSyncGroup, pendingSyncType;
	std::unordered_map<std::string, ComPtr<ID3D11ShaderResourceView>> m_TextureThumbnailCache;
	std::unordered_map<std::string, std::vector<SPAWN_COMMAND>> m_ParsedCommandCache;
	SPtr<CParticleShaderCache> m_pShaderCache;
	uint32_t m_iNextOwnerId = 1;
};
NS_END
