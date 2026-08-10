#pragma once
#include <initializer_list>
#include "Engine_Defines.h"
#include "ResourceManager.h"
#include "WorkerManager.h"
#include "GameObjectManager.h"
#include "GameObjectPoolManager.h"
#include "CameraManager.h"
#include "ShaderManager.h"
#include "DbgLineRender.h"
#include "MapManager.h"
#include "LightManager.h"
#include "NavMeshManager.h"
#include "SerializeManager.h"
#include "PrototypeManager.h"
#include "LuaManager.h"
#include "SoundManager.h"
#include "EventManager.h"
#include "PhysXManager.h"
#include "NvClothManager.h"

NS_BEGIN(physx)
class PxScene;
class PxPhysics;
class PxControllerManager;
NS_END

struct FMOD_SOUND;
NS_BEGIN(Engine)
class CTimeProvider;
class CImguiManager;
class CGraphicDevice;
class CDInputManager;
class CLevelManager;
class CLevel;
class CFontManager;
class CPrototype;
class CColliderManager;
class CCollider;
class CRenderer;
class CHizOcclusionCuller;
class CHizBuffer;
class CAnimEdit_Manager;
class CNodeEditor;
class CParticleManager;
struct SPAWN_COMMAND;
class CAction_Manager;
class CDbgLineRender;
class CSerializeManager;
class CModel_Instance_Manager;
class CMapMeshInstancingRenderer;
class CResStaticModel;
class CEffectManager;
class CComPxFixedJoint;
class CComPxDistanceJoint;
class CComPxRevoluteJoint;
class CComPxD6Joint;
class CPathPlaybackEditor;

class ENGINE_DLL CGameInstance final : public Singleton<CGameInstance>
{
	friend Singleton<CGameInstance>;
private:
	CGameInstance();
	~CGameInstance();

public:
	HRESULT InitializeEngine(const ENGINE_DESC& EngineDesc, ComPtr<ID3D11Device>& ppDevice, ComPtr<ID3D11DeviceContext>& ppContext);
	void FixedUpdateEngine(_float fFixedTimeDelta);
	void UpdateEngine(_float fTimeDelta);
	HRESULT Draw();
	void UpdateGUI();
	//void ClearResource(uint32_t iClearLevelIndex);
	void FrameStart(_float fTimeDelta);
	void FrameEnd(_float fTimeDelta);

public:
	void Release_Engine();

public:
	void SetMouseFix(_bool mousefix);// 유아이용

#pragma region TIME_PROVIDER
public:
	_float UpdateTimeProvider();
#pragma endregion

#pragma region IMGUI_MANAGER
public:
	void ImguiNewFrame();
	void ImguiEndFrameAndRender();
	_bool ImguiWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	_bool ImguiGetActive() const;
	void ImguiSetActive(_bool bActive);
	void ImguiEnableDocking(_bool bEnableDocking, _bool bEnableViewports);
#pragma endregion


#pragma region RESOURCE_MANAGER
public:
	SPtr<CResource> AddResource(const StringID& sGroupTag, const StringID& sResTag, _string_id eAssetType, const _string& sPath, void* pArg = nullptr);
	SPtr<CResource> AddResource(const StringID& sGroupTag, const StringID& sResTag, SPtr<CResource> pAsset);
	template<typename T>
	SPtr<T> AddResourceT(const StringID& sGroupTag, const StringID& sResTag, const _string& sPath, void* pArg = nullptr)
	{ return m_pResourceManager->AddResourceT<T>(sGroupTag, sResTag, sPath, pArg); }
	template<typename T>
	SPtr<T> AddResourceT(const StringID& sGroupTag, const StringID& sResTag, SPtr<T> pAsset)
	{ return m_pResourceManager->AddResourceT<T>(sGroupTag, sResTag, pAsset); }
	template<typename T, typename CreateFunc>
	SPtr<T> GetOrCreateResourceByPath(const _string& sPath, CreateFunc&& createFunc)
	{ return m_pResourceManager->GetOrCreateResourceByPath<T>(sPath, std::forward<CreateFunc>(createFunc)); }
	template<typename T>
	SPtr<T> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const
	{ return m_pResourceManager->GetResourceFirst<T>(sGroupTag, sResTag); }
	std::vector<SPtr<CResource>> GetResource(const StringID& sGroupTag, const StringID& sResTag) const;
	std::unordered_map<StringID, std::vector<SPtr<CResource>>> GetResource(const StringID& sGroupTag) const;
	std::unordered_map<StringID, std::unordered_map<StringID, std::vector<SPtr<CResource>>>> GetResources() const;

	void DelResource(const StringID& sGroupTag);
	void DelResource(const StringID& sGroupTag, const StringID& sResTag);

	std::vector<SPtr<CResource>> GetResourcesByPath(const _string& sPath) const;
	void RemoveResourcePathLookup(const _string& sPath, SPtr<CResource> pRes);
#pragma endregion

#pragma region LEVEL_MANAGER
public:
	HRESULT ChangeLevel(UPtr<CLevel> pNewLevel);
	HRESULT ChangeLevel(const _string& ID);
	uint32_t GetCurrentLevelID() const;
	void RegisterLevelChangeFunc(const _string& ID, _Func func);
#pragma endregion

#pragma region GRAPHIC_DEVICE
public:
	ComPtr<ID3D11Device> GetGraphicDevice() const;
	ComPtr<ID3D11DeviceContext> GetGraphicDeviceContext()const;
	ComPtr<ID3D11RenderTargetView> GetBackBufferRTV() const;
	ComPtr<ID3D11DepthStencilView> GetBackBufferDSV() const;
	ComPtr<ID3D11Texture2D> GetBackBufferTexture() const;
	HRESULT ClearBackBufferView(const _float4* pClearColor);
	HRESULT ClearDepthStencilView();
	HRESULT Present();
#pragma endregion

#pragma region DINPUT_MANAGER
public:
	_bool KeyPressing(_ubyte byKeyID) const;
	_bool KeyUp(_ubyte byKeyID) const;
	_bool KeyDown(_ubyte byKeyID) const;
	int32_t	MouseMove(MOUSEMOVESTATE eMouseState) const;
	_bool MousePressing(MOUSEKEYSTATE eMouseState) const;
	_bool MouseUp(MOUSEKEYSTATE eMouseState) const;
	_bool MouseDown(MOUSEKEYSTATE eMouseState) const;
#pragma endregion

#pragma region SOUND_MANAGER
public:
	CSoundManager* GetSoundManager() const { return m_pSoundManager.get(); }
#pragma endregion

#pragma region FONT_MANAGER
	void FontDraw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale = 1.f, _fvector vColor = XMVectorSet(1.f, 1.f, 1.f, 1.f), _float fRotation = 0.f, const _float2& vOrigin = { 0.f, 0.f });
	void FontAddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale = 1.f, _fvector vColor = XMVectorSet(1.f, 1.f, 1.f, 1.f), _float fRotation = 0.f, const _float2& vOrigin = { 0.f, 0.f });
	_float2 FontMeasureString(const StringID& fontName, const wchar_t* txt, float scale = 1.f) const;
	void FontLateDraw(RENDERGROUP eRenderGroup);
	void FontAddLateDraw3D(const std::string& fontTag, const std::wstring& text, _fmatrix matWVP, _fvector color, _float2 pivot);
	void Render3DFont();
#pragma


#pragma region WORKER_MANAGER
public:
	_bool WorkerEnqueue(_string_view svTaskName, _Func func);
	template<typename Func, typename... Args>
	auto WorkerEnqueueWithFuture(_string_view svTaskName, Func&& f, Args&&... args)
		-> std::future<std::invoke_result_t<Func, Args...>>
	{
		return m_pWorkerManager->WorkerEnqueueWithFuture(
			svTaskName,
			std::forward<Func>(f),
			std::forward<Args>(args)...
		);
	}
#pragma endregion

#pragma region PROTOTYPE_MANAGER
public:
	HRESULT AddPrototype(const StringID& svGroupTag, const StringID& svPrototypetag, UPtr<CPrototype> pPrototype);
	UPtr<CPrototype> ClonePrototype(const StringID& svGroupTag, const StringID& svPrototypetag, void* pArg = nullptr);
	void DelPrototype(const StringID& sGroupTag);
	void DelPrototype(const StringID& sGroupTag, const StringID& sPrototypeTag);
	std::vector<StringID> GetPrototypeTags(const StringID& svGroupTag) const;
#pragma endregion

#pragma region GAMEOBJECT_MANAGER
public:
	void GameObjectAllReset();
	size_t GameObjectResetLayers(std::span<const std::string_view> layerNames);
	size_t GameObjectAllResetExceptLayers(std::span<const std::string_view> excludedLayerNames);

	template<typename TLayer>
	size_t GameObjectResetLayers(std::initializer_list<TLayer> layerNames);

	template<typename TLayer>
	size_t GameObjectAllResetExceptLayers(std::initializer_list<TLayer> excludedLayerNames);

	template<typename TLayer>
	std::optional<CHandle> AddGameObjectToLayer(const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, TLayer&& sLayerName, void* pArg = nullptr)
	{
		return m_pGameObjectManager->AddGameObjectToLayer(iPrototypeLevelIndex, svPrototypeTag, MagicEnumToStringView(std::forward<TLayer>(sLayerName)), pArg);
	}
	template<typename TLayer>
	const std::vector<CHandle>* GetGameObjectLayer(TLayer&& sLayerName) const
	{
		return m_pGameObjectManager->GetLayer(MagicEnumToStringView(std::forward<TLayer>(sLayerName)));
	}
	template<typename TLayer>
	const std::vector<CHandle>* GetGameObjectLayer(TLayer&& sLayerName, const StringID& iPrototypeLevelIndex, const StringID& svPrototypeTag, void* pArg)
	{
		return m_pGameObjectManager->GetLayer(MagicEnumToStringView(std::forward<TLayer>(sLayerName)), iPrototypeLevelIndex, svPrototypeTag, pArg);
	}
	const std::vector<std::pair<std::string, std::vector<CHandle>>>& GetGameObjectLayers() const;
	template<typename TLayer>
	void DelGameObjectLayer(TLayer&& sLayerName)
	{
		return m_pGameObjectManager->DelLayer(MagicEnumToStringView(std::forward<TLayer>(sLayerName)));
	}

	//std::optional<CHandle> GetFreeHandle() const;

	inline CGameObject* GetGameObjectByHandle(const CHandle& handle);
	template<typename T>
	T* GetGameObjectByHandleT(const CHandle& handle)
	{
		return m_pGameObjectManager->GetGameObjectByHandleT<T>(handle);
	}
	template<typename T>
	const T* GetGameObjectByHandleT(const CHandle& handle) const
	{
		return static_cast<const CGameObjectManager*>(m_pGameObjectManager.get())->GetGameObjectByHandleT<T>(handle);
	}

	template<typename T, typename TLayer>
	T* GetFirstGameObjectByLayer(TLayer&& sLayerName) const
	{
		return m_pGameObjectManager->GetFirstGameObjectByLayer<T>(MagicEnumToStringView(std::forward<TLayer>(sLayerName)));
	}
	CGameObjectPoolManager* GetGameObjectPoolManager() const
	{
		return m_pGameObjectPoolManager.get();
	}
	//template<typename T, typename E> requires std::is_enum_v<E>
	//T* GetFirstGameObjectByLayer(E layer) const
	//{
	//	return GetFirstGameObjectByLayer<T>(magic_enum::enum_name(layer));
	//}
#pragma endregion

#pragma region COLLIDER_MANAGER
public:
	void AddColliderGroup(const StringID& groupTag, const CCollider*);
	const std::vector<const CCollider*>* GetColliderGroup(const StringID& groupTag) const;
	_bool IntersectColl(const CCollider* pColl1, const CCollider* pColl2);
	const std::unordered_map<StringID, std::vector<const CCollider*>>* GetColliders() const;
#pragma endregion

#pragma region CAMERA_MANAGER
public:
	CCameraObject* GetActiveCamera() const;
	CCameraObject* GetActiveCamera(const StringID& CameraID) const;
	HRESULT SetActiveCamera(const StringID& CameraID);

	CCameraObject* GetCamera(const StringID& CameraID) const;
	HRESULT RegistCamera(const StringID& CameraID, const CHandle& handle);

	HRESULT RegistCinematicAsset(const SPtr<CCinematicAsset>& pAsset);
	HRESULT LoadCinematic(const std::string& CinematicName);
	HRESULT PlayCinematic(const StringID& CinematicID, const FCinematicPlayOptions& Options = {});
	HRESULT PlayCinematic(const StringID& CinematicID, const CHandle& TargetHandle, const FCinematicPlayOptions& Options = {});
	void StopCinematic();
	_bool IsCinematicPlaying() const;
	_float GetCinematicPlayTime() const;
	void SetCinematicCollisionQueryMask(uint32_t iQueryMask);
#pragma endregion

#pragma region RENDERER
public:
	HRESULT AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);
	_bool IsOcclusionCulled(const IRenderable* pRenderObject);
	const CHizBuffer* GetPrevHizBuffer() const;
	HRESULT	Reset_DefaultShader(RENDERGROUP _Group);

	SPtr<CResDynamicTexture2D>	Generate_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _Format, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_DepthStencil_RenderTarget(const StringID& _sResTag, DXGI_FORMAT _TexFormat, DXGI_FORMAT _DSVFormat, DXGI_FORMAT _SRVFormat, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResDynamicTexture2D>	Generate_UnorderedAccessView(const StringID& _sResTag, DXGI_FORMAT _TexFormat, uint32_t _BindFlags, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	SPtr<CResViewPort>			Generate_ViewPort(const StringID& _sResTag, uint32_t _TexWidth = 0, uint32_t _TexHeight = 0);
	 
	HRESULT	Generate_Texture2DArray(std::vector<ComPtr<ID3D11DepthStencilView>>* _ShadowDSVList, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT	Generate_ShadowCubeMap(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _TextureArray, ID3D11ShaderResourceView** _SRV, uint32_t _Resolution, uint32_t _MaxLightCount);
	HRESULT	Generate_ShadowTexture(ID3D11DepthStencilView** _ShadowDSV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _SRV, uint32_t _ResolutionX, uint32_t _ResolutionY);
	HRESULT Generate_ShadowMapOutput(ID3D11UnorderedAccessView** _ShadowUAV, ID3D11Texture2D** _Texture, ID3D11ShaderResourceView** _ShadowSRV, uint32_t _LTYPE, uint32_t _ResolutionX, uint32_t _ResolutionY);

	VOID	Render_ChromaticRing(XMVECTOR _WorldPosition, _float _Duration, _float _Scale);
	VOID	Set_ChromaticRingOpacity(_float _Opacity);

	VOID	Apply_OutlineEffect(std::optional<CHandle> targetHandle);
#pragma endregion


#pragma region SHADER_MANAGER
public:
	template<typename T>
	HRESULT Bind_ConstantBuffer(T _Argument, SPtr<CResCBuffer> _Buffer) const
	{
		return m_pShaderManager->Bind_ConstantBuffer<T>(_Argument, _Buffer);
	}
#pragma endregion

#pragma region LIGHT_MANAGER
public:
	HRESULT	Initialize_EffectLight(uint32_t _PoolSize);

	std::optional<CHandle> Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	std::optional<CHandle> Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _InnerRange, _float _OuterRange);
	std::optional<CHandle> Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);
	_bool	Remove_Light(const CHandle& hLight);
	size_t	Remove_PlacementLightGroup(std::string_view sGroup);
	void	SetActivePlacementLightGroup(std::string_view sGroup);

	VOID	Clear_DynamicLightList();

	HRESULT	AddShadowRenderGroup(ACTORTYPE _ATYPE, IRenderable* pRenderObject);

	HRESULT	Render_ObjectShadow();
	HRESULT	Render_ObjectNonShadow();
	const SPtr<CResDynamicTexture2D>& Get_CombinedResource() { return m_pLightManager->Get_CombinedResource(); }

	std::optional<CHandle> Allocate_EffectLight(XMVECTOR _WorldPos, _float _Intensity, _float3 _Color, _float _InnerRange, _float _OuterRange, _float _LifeTime, _float3 _Velocity);
	HRESULT	Capture_ShadowMap();

	VOID	Notify_StaticShadowSceneChanged(const BoundingBox& ChangedBounds);

	VOID	Bind_VolumetricLocalLightResources();
	VOID	UnBind_VolumetricLocalLightResources();

	_bool		Evaluate_DirectionalLightCount();

	XMMATRIX	Get_CascadeShadowViewProj(uint32_t _Index);
	XMFLOAT4	Get_CascadeShadowSplits();
	CSM_DATA&	Get_MainDirectionalLightData();

#pragma endregion

#pragma region ANIMATIONEDTIOR_MANAGER
	_string GetAnimName(uint32_t iIndex, CHandle Handle);
	int32_t GetAnimIndex(CHandle Handle);
#pragma endregion
#pragma region PARTICLE_MANAGER
public:
	HRESULT SetupTestModel();
#pragma endregion

#pragma region NODE_EDITOR
	HRESULT	   OpenBeHavior(CHandle Handle);
#pragma endregion

#pragma region Action_Manager
	UPtr<class CBTRoot>		Show_ActioNode_List(NODEGROUP eType, uint32_t& iNode, ImVec2 vNodePos, CHandle Handle);
	void					Show_Action_NodeWidget(CBTRoot* pNode);

#pragma endregion

#pragma region PARTICLE_MANAGER
public:
	HRESULT LoadParticleJson(const std::string& strJsonPath);

	HRESULT Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
		uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
		_bool bLoop, _float fSpawnInterval);
	uint32_t Spawn(const std::string& strJsonPath, const _float4x4& worldMat, const _fvector endPos = XMVectorZero());
	HRESULT Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<class CParticle> particle);

	HRESULT LoadParticlePresets(const std::string& strJsonPath);
	std::vector<SPAWN_COMMAND> Parse_Command(const std::string& strJsonPath);
	uint32_t Spawn(const std::vector<SPAWN_COMMAND>& templateCommands, const _float4x4& worldMat, _fvector endPos = XMVectorSet(0,0,0,1));
	CParticle* GetParticle(const StringID& sGroupTag, const StringID& sTypeTag);
	std::vector<std::string> Load_FilePath_ByExtension(const std::filesystem::path& _FolderPath, std::string_view _Extension);
	HRESULT Load_ParticleJsonPackage(const std::vector<std::string>& _FilePathPackage);
	void TranslateOwner(uint32_t ownerId, const _float3& delta);
	HRESULT AddTrailPoint(const StringID& groupTag, const StringID& typeTag, const _float3& start, const _float3& end);
	std::optional<BEAM_HANDLE> SpawnBeam(const StringID& groupTag, const StringID& typeTag, const BEAM_PARAMS& params);
	HRESULT SetBeamPositions(const BEAM_HANDLE& handle, const _float4& start, const _float4& end);
	HRESULT StopBeam(const BEAM_HANDLE& handle);
#pragma endregion

#pragma region EFFECT_MANAGER
public:
	EFFECT_INSTANCE_ID PlayEffect(const std::string& sEffectName,const _float4x4& matWorld,
		_fvector vEndPosition = XMVectorZero(), EFFECT_FINISHED_CALLBACK onFinsihed = {});

	void StopEffect(EFFECT_INSTANCE_ID iEffectId);

	void SetEffectPosition(EFFECT_INSTANCE_ID iEffectId,const _float3& vPosition);

	void SetEffectWorldMatrix(EFFECT_INSTANCE_ID iEffectId,const _float4x4& colliderWorldMatrix);
	void SetBeamPositionsByOwner(EFFECT_INSTANCE_ID effectId, const _float3& start, const _float3& end);

	void ClearAllRunningEffect();
#pragma endregion

#pragma region MAP_MANAGER
public:
	HRESULT SaveMap(const std::string& path);
	HRESULT LoadMapResources(const std::string& path);
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true);
	HRESULT LoadMapData(const std::string& path);
	HRESULT LoadMapChunk(const MAPCHUNK_COORD& coord);
	HRESULT UnLoadMapChunk(const MAPCHUNK_COORD& coord);
	void RebuildMapChunks();
	HRESULT RegisterMapMeshObjectToMapChunk(const CHandle& hObject);
	std::vector<CHandle> CollectMapMeshPickCandidates(FXMVECTOR rayOrigin, FXMVECTOR rayDirection) const;
	const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& GetMapChunks() const;
	const _float3& GetMapChunkSize() const;
	void SetMapChunkStreaming(_bool enable);
	_bool IsMapChunkStreaming() const;
#ifdef _DEBUG
	void SetDebugDrawMapChunk(_bool draw);
#endif
	void ClearAllChunk();
#pragma endregion

#pragma region PHYSX_MANAGER
public:
	CPhysXManager* GetPhysXManager() const { return m_pPhysXManager.get(); };
	physx::PxScene* PxGetScene() const;
	physx::PxPhysics* PxGetPhysics() const;
	physx::PxControllerManager* PxGetControllerManager() const;

	template<typename TJoint>
	TJoint* AddPxJoint(
		CGameObject& JointOwner,
		const StringID& ComponentTag,
		typename TJoint::DESC Desc);

#pragma endregion

#pragma region NVCLOTH_MANAGER
public:
	CNvClothManager* GetNvClothManager() const
	{
		return m_pNvClothManager.get();
	}
#pragma endregion


#pragma region DBG_LINE_RENDER
public:
	CDbgLineRender* GetDbgLineRender() const { return m_pDbgLineRender.get(); };
#pragma endregion

#pragma region NAVMESH_MANAGER
public:
	CNavMeshManager* GetNavMeshManager() const { return m_pNavMeshManager.get(); }
#pragma endregion

#pragma region INSTNACE_MANAGER
public:
	void Add_Instance(class CComModelInstance* pModelInstance, class CComAnimator* pAnimator, const _float4x4& WorldMatrix, uint32_t iFlags = 0);
	void Add_Instance(class CComStaticModelInstance* pModelInstance, const _float4x4& WorldMatrix, uint32_t iFlags = 0);


	void Add_Instance(class CComModelInstance* pModelInstance, const GPU_ANIM_INSTANCE_DATA& InstanceData);
	void Add_Part_Instance(class CComStaticModelInstance* pModelInstance, const GPU_PART_INSTANCE_DATA& InstanceData);
	const std::vector<MODEL_INSTANCE_BATCH*>& Get_ActiveBatches() const;

	/*----------- 광윤 추가 -----------*/
	HRESULT Render_ShadowInstanced(const ComPtr<ID3D11DeviceContext>& pContext, std::optional<CHandle> _LightHandle, _bool _bStaticBatch, int32_t _PointFaceIndex);
	_bool	Has_ActiveDynamicShadowBatch();
	/*---------------------------------*/
#pragma endregion

#pragma region MAPMESH_INSTANCE_RENDER
	HRESULT PushMapObjectInstance(const SPtr<CResStaticModel>& pModel, const MAPMESH_INSTANCE_DATA& instanceData, MAPMESH_OCCLUSION_DATA& occlusionData, EMapMeshRenderFeature renderFeature = EMapMeshRenderFeature::Static);
	// 인스턴싱 On/Off , 드로우 콜 GUI
	_bool IsInstancingEnabled();
	void SetInstancingEnabled(_bool bEnabled);
	const struct INSTANCING_STATS& GetInstancingStats();
	_bool IsDebugBoundsEnabled();
	void SetDebugBoundsEnabled(_bool bEnabled);
	void ClearMapMeshTextureCache();
	void EraseMapMeshTextureCache(const SPtr<CResStaticModel>& model);
#pragma endregion

#pragma region EVENT_MANAGER
	template<typename TEvent, typename TCallback>
	EVENT_LISTENER_ID EventSubscribe(CHandle owner, TCallback&& callback)
	{
		return m_pEventManager->Subscribe<TEvent>(owner, std::forward<TCallback>(callback));
	}

	template<typename TEvent>
	void EventUnsubscribe(EVENT_LISTENER_ID listenerId)
	{
		m_pEventManager->Unsubscribe<TEvent>(listenerId);
	}

	template<typename TEvent>
	void EventUnsubscribeAll(CHandle owner)
	{
		m_pEventManager->UnsubscribeAll<TEvent>(owner);
	}

	void EventUnsubscribeAll(CHandle owner)
	{
		m_pEventManager->UnsubscribeAll(owner);
	}

	template<typename TEvent>
	void EventPublish(TEvent&& event)
	{
		m_pEventManager->Publish(std::forward<TEvent>(event));
	}

	template<typename TEvent>
	void EventPublish()
	{
		m_pEventManager->Publish<TEvent>();
	}
#pragma endregion



#pragma region SERIALIZE_MANAGER
public:
	template<typename T>
	HRESULT BinDeSerialize(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "BIN",
		bool bShowError = true)
	{
		return m_pSerializeManager->BinDeSerialize(path, outValue, rootName, bShowError);
	}
	template<typename T>
	HRESULT BinSerialize(
		const std::string& path,
		const T& value,
		const std::string& rootName = "BIN",
		bool bShowError = true)
	{
		return m_pSerializeManager->BinSerialize(path, value, rootName, bShowError);
	}
	template<typename T>
	HRESULT JsonDeSerialize(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "JSON",
		bool bShowError = true)
	{
		return m_pSerializeManager->JsonDeSerialize(path, outValue, rootName, bShowError);
	}
	template<typename T>
	HRESULT JsonSerialize(
		const std::string& path,
		const T& value,
		const std::string& rootName = "JSON",
		bool bShowError = true)
	{
		return m_pSerializeManager->JsonSerialize(path, value, rootName, bShowError);
	}

	template<typename T>
	SERIALIZE_RESULT BinDeSerializeDetailed(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "BIN")
	{
		return m_pSerializeManager->BinDeSerializeDetailed(path, outValue, rootName);
	}

	template<typename T>
	SERIALIZE_RESULT BinSerializeDetailed(
		const std::string& path,
		const T& value,
		const std::string& rootName = "BIN")
	{
		return m_pSerializeManager->BinSerializeDetailed(path, value, rootName);
	}

	template<typename T>
	SERIALIZE_RESULT JsonDeSerializeDetailed(
		const std::string& path,
		T& outValue,
		const std::string& rootName = "JSON")
	{
		return m_pSerializeManager->JsonDeSerializeDetailed(path, outValue, rootName);
	}

	template<typename T>
	SERIALIZE_RESULT JsonSerializeDetailed(
		const std::string& path,
		const T& value,
		const std::string& rootName = "JSON")
	{
		return m_pSerializeManager->JsonSerializeDetailed(path, value, rootName);
	}
#pragma endregion

#pragma region LUA_MANAGER
	CLuaManager* GetLuaManager() const { return m_pLuaManager.get(); }
#pragma endregion

public:
	_float2 GetClientScreenSize() const { return m_vClientScreenSize; }
	_float2 GetDisplayScreenSize() const { return m_vDisplayScreenSize; }
	HWND GetHwnd() const { return m_hWnd; }
	_bool GetMouseFix() const { return m_bMouseFix; }

	_float2 GetMousePos() {
		POINT pt; GetCursorPos(&pt); ScreenToClient(m_hWnd, &pt);
		return { (float)pt.x, (float)pt.y };
	}
	uint64_t GetFrameCnt() const { return m_iFrameCnt; }
private:
	_float2 m_vClientScreenSize{ 1280.f, 720.f };
	_float2 m_vDisplayScreenSize{ 1920.f, 1280.f };
	HWND m_hWnd{};
	_bool m_bMouseFix{};
	uint64_t m_iFrameCnt{};

private:
	void MouseFix() const;

private:
	UPtr<CGraphicDevice> m_pGraphicDevice{};
	UPtr<CImguiManager> m_pImguiManager{};
	UPtr<CResourceManager> m_pResourceManager{};
	UPtr<CSoundManager> m_pSoundManager{};
	UPtr<CDInputManager> m_pDInputManager{};
	UPtr<CLevelManager> m_pLevelManager{};
	UPtr<CWorkerManager> m_pWorkerManager{};
	UPtr<CTimeProvider> m_pTimeProvider{};
	UPtr<CPrototypeManager> m_pPrototypeManager{};
	UPtr<CGameObjectManager> m_pGameObjectManager{};
	UPtr<CGameObjectPoolManager> m_pGameObjectPoolManager{};
	UPtr<CCameraManager> m_pCameraManager{};
	UPtr<CColliderManager> m_pColliderManager{};
	UPtr<CRenderer> m_pRenderer{};
	UPtr<CHizOcclusionCuller> m_pHizOcclusionCuller{};
	UPtr<CShaderManager> m_pShaderManager{};
	UPtr<CLightManager> m_pLightManager{};
	//UPtr<CVoxelManager> m_pVoxelManager{};
	//UPtr<CVoxelManager2> m_pVoxelManager2{};
	//UPtr<CVoxelManager3> m_pVoxelManager3{};
	UPtr<CParticleManager> m_pParticleManager{};
	UPtr<CFontManager> m_pFontManager{};
	UPtr<CAnimEdit_Manager> m_pAnimEdit_Manager{};
	UPtr<CPhysXManager> m_pPhysXManager{};
	UPtr<CNvClothManager> m_pNvClothManager{};
	UPtr<CDbgLineRender> m_pDbgLineRender{};
	UPtr<CNodeEditor>		m_pNodeEditor{};
	UPtr<CAction_Manager>	m_pActionManager{};
	//UPtr<CWorldManager> m_pWorldManager{};
	UPtr<CMapManager> m_pMapManager{};
	UPtr<CNavMeshManager> m_pNavMeshManager{};
	UPtr<CSerializeManager> m_pSerializeManager{};
	UPtr<CLuaManager> m_pLuaManager{};
	UPtr<CModel_Instance_Manager> m_pModel_Instance_Manager{};
	UPtr<CMapMeshInstancingRenderer> m_pMapMeshInstancingRenderer{};
	UPtr<CEventManager> m_pEventManager{};
	UPtr<CEffectManager> m_pEffectManager{};
	UPtr<CPathPlaybackEditor> m_pPathPlaybackEditor{};
};

template<typename TLayer>
size_t CGameInstance::GameObjectResetLayers(
	std::initializer_list<TLayer> layerNames)
{
	std::vector<std::string_view> layerNameViews{};
	layerNameViews.reserve(layerNames.size());

	for (const auto& layerName : layerNames)
	{
		layerNameViews.push_back(MagicEnumToStringView(layerName));
	}

	return GameObjectResetLayers(
		std::span<const std::string_view>{ layerNameViews });
}

template<typename TLayer>
size_t CGameInstance::GameObjectAllResetExceptLayers(
	std::initializer_list<TLayer> excludedLayerNames)
{
	std::vector<std::string_view> layerNameViews{};
	layerNameViews.reserve(excludedLayerNames.size());

	for (const auto& layerName : excludedLayerNames)
	{
		layerNameViews.push_back(MagicEnumToStringView(layerName));
	}

	return GameObjectAllResetExceptLayers(
		std::span<const std::string_view>{ layerNameViews });
}

template<typename TJoint>
TJoint* CGameInstance::AddPxJoint(
	CGameObject& JointOwner,
	const StringID& ComponentTag,
	typename TJoint::DESC Desc)
{
	static_assert(
		std::is_same_v<TJoint, CComPxFixedJoint> ||
		std::is_same_v<TJoint, CComPxDistanceJoint> ||
		std::is_same_v<TJoint, CComPxRevoluteJoint> ||
		std::is_same_v<TJoint, CComPxD6Joint>,
		"AddPxJoint supports PhysX Joint components only.");

	ES_EngineProtoPhysXComponent ePrototype{};
	if constexpr (std::is_same_v<TJoint, CComPxFixedJoint>)
	{
		ePrototype =
			ES_EngineProtoPhysXComponent::
				Prototype_Component_ComPxFixedJoint;
	}
	else if constexpr (
		std::is_same_v<TJoint, CComPxDistanceJoint>)
	{
		ePrototype =
			ES_EngineProtoPhysXComponent::
				Prototype_Component_ComPxDistanceJoint;
	}
	else if constexpr (
		std::is_same_v<TJoint, CComPxRevoluteJoint>)
	{
		ePrototype =
			ES_EngineProtoPhysXComponent::
				Prototype_Component_ComPxRevoluteJoint;
	}
	else if constexpr (
		std::is_same_v<TJoint, CComPxD6Joint>)
	{
		ePrototype =
			ES_EngineProtoPhysXComponent::
				Prototype_Component_ComPxD6Joint;
	}

	TJoint* pJoint{};
	if (FAILED(JointOwner.AddComponentFromProto(
		ES_EngineProtoMajorType::PHYSX,
		ePrototype,
		ComponentTag,
		&Desc,
		&pJoint)) ||
		!pJoint)
	{
		return nullptr;
	}

	return pJoint;
}

NS_END
