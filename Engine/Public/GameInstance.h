#pragma once
#include "Engine_Defines.h"
#include "ResourceManager.h"
#include "WorkerManager.h"
#include "GameObjectManager.h"
#include "CameraManager.h"
#include "ShaderManager.h"
#include "DbgLineRender.h"
#include "MapManager.h"
#include "LightManager.h"
#include "NavMeshManager.h"
#include "SerializeManager.h"
#include "LuaManager.h"

NS_BEGIN(physx)
class PxScene;
class PxPhysics;
NS_END

struct FMOD_SOUND;
NS_BEGIN(Engine)
class CTimeProvider;
class CImguiManager;
class CGraphicDevice;
class CDInputManager;
class CLevelManager;
class CLevel;
class CSoundManager;
class CFontManager;
class CPrototypeManager;
class CPrototype;
class CColliderManager;
class CCollider;
class CRenderer;
class CAnimEdit_Manager;
class CNodeEditor;
class CParticleManager;
class CAction_Manager;
class CPhysXManager;
class CDbgLineRender;
class CSerializeManager;
class ILuaScriptRelodable;

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
	{
		return m_pResourceManager->AddResourceT<T>(sGroupTag, sResTag, sPath, pArg);
	}
	template<typename T>
	SPtr<T> AddResourceT(const StringID& sGroupTag, const StringID& sResTag, SPtr<T> pAsset)
	{
		return m_pResourceManager->AddResourceT<T>(sGroupTag, sResTag, pAsset);
	}
	template<typename T>
	SPtr<T> GetResourceFirst(const StringID& sGroupTag, const StringID& sResTag) const
	{
		return m_pResourceManager->GetResourceFirst<T>(sGroupTag, sResTag);
	}
	const std::vector<SPtr<CResource>>* GetResource(const StringID& sGroupTag, const StringID& sResTag) const;
	const std::unordered_map<StringID, std::vector<SPtr<CResource>>>* GetResource(const StringID& sGroupTag) const;
	const std::unordered_map<StringID, std::unordered_map<StringID, std::vector<SPtr<CResource>>>>& GetResources() const;
	HRESULT LoadResource(const StringID& sGroupTag);
	HRESULT LoadResource(const StringID& sGroupTag, const StringID& sResTag);
	HRESULT UnLoadResource(const StringID& sGroupTag);
	HRESULT UnLoadResource(const StringID& sGroupTag, const StringID& sResTag);
	void DelResource(const StringID& sGroupTag);
	void DelResource(const StringID& sGroupTag, const StringID& sResTag);

	const std::vector<CResource*>* GetResourcesByPath(const _string& sPath) const;
	void RemoveResourcePathLookup(const _string& sPath, CResource* pRes);
#pragma endregion

#pragma region LEVEL_MANAGER
public:
	HRESULT ChangeLevel(UPtr<CLevel> pNewLevel);
	HRESULT ChangeLevel(const _string& ID);
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
	HRESULT CreateSound(const _string& sPath, FMOD_SOUND** ppSound);

	HRESULT SoundAddChannel(const StringID& channelTag, const std::pair<StringID, StringID>& soundResources);
	HRESULT SoundPlay(const StringID& channelTag, _float fVolume = 1.f, _float fPitch = 1.f);
	void SoundStop(const StringID& channelTag);
	void SoundPause(const StringID& channelTag, _bool bPause);
	_bool SoundGetVolume(const StringID& channelTag, _float& fVolume);
	_bool SoundSetVolume(const StringID& channelTag, _float fVolume);
	_bool SoundIsPlaying(const StringID& channelTag) const;
	void SoundSetPitch(const StringID& channelTag, float fPitchRatio);
#pragma endregion

#pragma region FONT_MANAGER
	void FontDraw(const StringID& fontName, const _tchar* pText, const _float2& vPosition, float fScale = 1.f, _fvector vColor = XMVectorSet(1.f, 1.f, 1.f, 1.f), _float fRotation = 0.f, const _float2& vOrigin = { 0.f, 0.f });
	void FontAddLateDraw(RENDERGROUP eRenderGroup, const StringID& fontName, const _wstring& pText, const _float2& vPosition, float fScale = 1.f, _fvector vColor = XMVectorSet(1.f, 1.f, 1.f, 1.f), _float fRotation = 0.f, const _float2& vOrigin = { 0.f, 0.f });
	_float2 FontMeasureString(const StringID& fontName, const wchar_t* txt, float scale = 1.f) const;
	void FontLateDraw(RENDERGROUP eRenderGroup);
#pragma


#pragma region WORKER_MANAGER
public:
	void WorkerEnqueue(_string_view svTaskName, _Func func);
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
#pragma endregion

#pragma region GAMEOBJECT_MANAGER
public:
	void GameObjectAllReset();
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

	//const CCameraObject* GetCameraObject(const StringID& GroupID) const;
	//HRESULT SetCameraObject(const StringID& GroupID, const CHandle& handle);

	//CCameraObject* GetActiveGameCamera() const;
	//HRESULT SetActiveGameCamera(const StringID& CameraID);
	//CCameraObject* GetActiveUICamera() const;
	//HRESULT SetActiveUICamera(const StringID& CameraID);

	//CCameraObject* GetActiveGameCamera(const StringID& CameraID) const;
	//CCameraObject* GetActiveUICamera(const StringID& CameraID) const;

	//CCameraObject* GetGameCamera(const StringID& CameraID) const;
	//CCameraObject* GetUICamera(const StringID& CameraID) const;

	//HRESULT RegistGameCamera(const StringID& CameraID, const CHandle& handle);
	//HRESULT RegistUICamera(const StringID& CameraID, const CHandle& handle);
#pragma endregion

#pragma region RENDERER
public:
	HRESULT AddRenderObject(RENDERGROUP eRenderGroup, IRenderable* pRenderObject);
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
	VOID	Bind_EnviromentLight();
	VOID	Bind_DynamicLight();

	VOID	Add_DirectionalLight(XMFLOAT3 _Direction, XMFLOAT3 _Color, _float _Intensity);
	VOID	Add_PointLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range);
	VOID	Add_SpotLight(XMFLOAT3 _Position, XMFLOAT3 _Color, _float _Intensity, _float _Range, _float _InnerAtt, _float _OuterAtt);

	VOID	Clear_DynamicLightList();
#pragma endregion

#pragma region ANIMATIONEDTIOR_MANAGER
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
	HRESULT					Add_Action_Prototype(NODEGROUP eType, const _string& strActionName, UPtr<class CBTRoot> pAction);
	UPtr<class CBTRoot>		Show_ActioNode_List(NODEGROUP eType, uint32_t& iNode, ImVec2 vNodePos, CHandle Handle);
	void					Show_Action_NodeWidget(CBTRoot* pNode);
	UPtr<class CBTRoot>	    Clone_Action(NODEGROUP eType, const _string& strActionName, void* pArg);

#pragma endregion

#pragma region PARTICLE_MANAGER
public:
	HRESULT Spawn(const StringID& sGroupTag, const StringID& sTypeTag,
		uint32_t count, const PARTICLE_SPAWN_DATA* pSpawnData,
		_bool bLoop, _float fSpawnInterval);

	HRESULT Add_Particle(const StringID& sGroupTag, const StringID& sTypeTag, UPtr<class CParticle> particle);

	HRESULT SpawnRibbon(uint32_t quantity, const _float4& start, const _float4& end,
		_float fDisplacementAmplitude, _float iDisplacementIterations, _float fDisplacementDamping,
		_float fFlickerInterval, _float4 vColor, _float4 emissive, _float fDuration = 1.f);
#pragma endregion

#pragma region MAP_MANAGER
public:
	HRESULT SaveMap(const std::string& path);
	HRESULT LoadMap(const std::string& path, _bool clearBeforeLoad = true);
	HRESULT LoadMapData(const std::string& path);
	HRESULT LoadMapChunk(const MAPCHUNK_COORD& coord);
	HRESULT UnLoadMapChunk(const MAPCHUNK_COORD& coord);
	void RebuildMapChunks();
	HRESULT RegisterMapMeshObjectToMapChunk(const CHandle& hObject);
	const std::unordered_map<MAPCHUNK_COORD, MAPCHUNK, tagMapChunkCoordHash>& GetMapChunks() const;
	const _float3& GetMapChunkSize() const;
	void SetMapChunkStreaming(_bool enable);
	_bool IsMapChunkStreaming() const;
#ifdef _DEBUG
	void SetDebugDrawMapChunk(_bool draw);
#endif
#pragma endregion

public:
	CPhysXManager* GetPhysiXManager() const { return m_pPhysXManager.get(); };
	physx::PxScene* PxGetScene() const;
	physx::PxPhysics* PxGetPhysics() const;

	_bool PxRayCast(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, PHYSIX_RAYCAST_RESULT& outResult) const;
	_bool PxRayCastMultiple(const _float3& vOrigin, const _float3& vNormalizedDir, _float fMaxDistance, std::vector<PHYSIX_RAYCAST_RESULT>& outVecResult, uint32_t iMaxHit = 10) const;
#pragma endregion


#pragma region DBG_LINE_RENDER
public:
	CDbgLineRender* GetDbgLineRender() const { return m_pDbgLineRender.get(); };
#pragma endregion

#pragma region NAVMESH_MANAGER
public:
	CNavMeshManager* GetNavMeshManager() const { return m_pNavMeshManager.get(); }
#pragma endregion



#pragma region SERIALIZE_MANAGER
public:
	template<typename T>
	HRESULT BinDeSerialize(const std::string& path, T& outValue, const std::string& rootName = "BIN")
	{ return m_pSerializeManager->BinDeSerialize(path, outValue, rootName); }
	template<typename T>
	HRESULT BinSerialize(const std::string& path, const T& value, const std::string& rootName = "BIN")
	{ return m_pSerializeManager->BinSerialize(path, value, rootName); }
	template<typename T>
	HRESULT JsonDeSerialize(const std::string& path, T& outValue, const std::string& rootName = "JSON")
	{ return m_pSerializeManager->JsonDeSerialize(path, outValue, rootName); }
	template<typename T>
	HRESULT JsonSerialize(const std::string& path, const T& value, const std::string& rootName = "JSON")
	{ return m_pSerializeManager->JsonSerialize(path, value, rootName); }
#pragma endregion

#pragma region LUA_MANAGER
	HRESULT LuaScriptExecute(const std::string& script, const sol::environment& env, const std::string& chunkName = "InlineScript");
	HRESULT LuaScriptExecute(const std::string& script, const std::string& chunkName = "InlineScript");
	sol::environment LuaCreateEnvironment();

	sol::protected_function LuaCacheFunction(const std::string& funcName);
	sol::protected_function LuaCacheFunction(const sol::environment& env, const std::string& funcName);

	template<typename... Args>
	bool LuaCallCacheFunction(const sol::protected_function& func, Args&&... args)
	{ return m_pLuaManager->CallCacheFunction(func, std::forward<Args>(args)...); }

	template<typename T>
	void LuaSetValue(std::string_view name, T&& value)
	{ m_pLuaManager->SetValue(name, value); }

	template<typename T>
	bool LuaGetValue(std::string_view name, T& outValue)
	{ return m_pLuaManager->GetValue(name, outValue); }

	HRESULT LuaCompile(const std::string& script);

	bool LuaIsEnvValid(const sol::environment& env) const;
	bool LuaHasValue(const sol::environment& env, std::string_view name) const;
	void LuaRemoveValue(sol::environment& env, std::string_view name);
	void LuaEnvDump(const sol::environment& env) const;
	void LuaEnvClear(sol::environment& env);
	void LuaRegisterComponent(const std::string& path, ILuaScriptRelodable* pComp);
	void LuaUnregisterComponent(const std::string& path, ILuaScriptRelodable* pComp);
	void LuaRegisterExtension(std::function<void(sol::state&)> extensionFunc);
	template<typename T>
	void LuaRegisterType() { m_pLuaManager->RegisterType<T>(); }
#pragma endregion

public:
	_float2 GetClientScreenSize() const { return m_vClientScreenSize; }
	HWND GetHwnd() const { return m_hWnd; }
	_bool GetMouseFix() const { return m_bMouseFix; }

	_float2 GetMousePos() {
		POINT pt; GetCursorPos(&pt); ScreenToClient(m_hWnd, &pt);
		return { (float)pt.x, (float)pt.y };
	}
private:
	_float2 m_vClientScreenSize{ 1280.f, 720.f };
	HWND m_hWnd{};
	_bool m_bMouseFix{};

private:
	void MouseFix() const;

private:
	HRESULT InitializeResources();
	HRESULT InitializePrototype();

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
	UPtr<CCameraManager> m_pCameraManager{};
	UPtr<CColliderManager> m_pColliderManager{};
	UPtr<CRenderer> m_pRenderer{};
	UPtr<CShaderManager> m_pShaderManager{};
	UPtr<CLightManager> m_pLightManager{};
	//UPtr<CVoxelManager> m_pVoxelManager{};
	//UPtr<CVoxelManager2> m_pVoxelManager2{};
	//UPtr<CVoxelManager3> m_pVoxelManager3{};
	UPtr<CParticleManager> m_pParticleManager{};
	UPtr<CFontManager> m_pFontManager{};
	UPtr<CAnimEdit_Manager> m_pAnimEdit_Manager{};
	UPtr<CPhysXManager> m_pPhysXManager{};
	UPtr<CDbgLineRender> m_pDbgLineRender{};
	UPtr<CNodeEditor>		m_pNodeEditor{};
	UPtr<CAction_Manager>	m_pActionManager{};
	//UPtr<CWorldManager> m_pWorldManager{};
	UPtr<CMapManager> m_pMapManager{};
	UPtr<CNavMeshManager> m_pNavMeshManager{};
	UPtr<CSerializeManager> m_pSerializeManager{};
	UPtr<CLuaManager> m_pLuaManager{};
};

NS_END
