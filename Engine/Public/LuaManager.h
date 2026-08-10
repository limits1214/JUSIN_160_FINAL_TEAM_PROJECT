#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class CGameObject;
class CLuaWatcher;
class CResLuaScript;
class CLuaScriptInstance;

class ENGINE_DLL CLuaManager final : public CEngineBase
{
public:
	using LUA_OBJECT_SPAWN_FACTORY =
		std::function<std::optional<CHandle>(std::string_view, const sol::table&)>;
	using LUA_COMPONENT_ATTACH_FACTORY =
		std::function<_bool(CGameObject&, std::string_view, const sol::table&)>;

private:
	CLuaManager();
	~CLuaManager();

public:
	void UpdateGUI();
	void Update(_float fTimeDelta);

	SPtr<CLuaScriptInstance> CreateScriptInstance(
		const SPtr<CResLuaScript>& pScript,
		const _string& sDebugName = {});
	// 경로 기반 리소스를 캐시하여 인스턴스만 생성한다. Context 설정 후 Load()는 호출자가 수행한다.
	SPtr<CLuaScriptInstance> CreateScriptInstanceByPath(
		const _string& sScriptPath,
		const _string& sDebugName = {});

	// Lua에는 SpawnType만 노출하고 실제 Prototype/DESC 구성은 등록한 C++ Factory가 담당한다.
	_bool RegisterObjectSpawnFactory(
		std::string_view sSpawnType,
		LUA_OBJECT_SPAWN_FACTORY Factory);
	_bool UnregisterObjectSpawnFactory(std::string_view sSpawnType);
	_bool RegisterComponentAttachFactory(
		std::string_view sAttachType,
		LUA_COMPONENT_ATTACH_FACTORY Factory);
	_bool UnregisterComponentAttachFactory(std::string_view sAttachType);

	// 객체에 종속되지 않는 일회성 Lua 코드를 전역 VM에서 실행한다.
	HRESULT Execute(
		const std::string& script,
		const std::string& chunkName = "InlineScript");

	HRESULT Compile(const std::string& script);

	// Client 전용 타입/API는 엔진 바인딩 이후 이 함수로 추가한다.
	void RegisterBindingExtension(std::function<void(sol::state&)> Extension)
	{
		if (Extension)
			Extension(m_Lua);
	}

	void UpdateHotReload();
	void OnFileChanged(const std::string& path);

private:
	HRESULT Initialize();
	HRESULT InitializePrintBinding();
	HRESULT InitializeValueTypeBindings();
	HRESULT InitializeConstantBindings();
	HRESULT InitializeEngineBindings();

	HRESULT BindInputAPI();
	HRESULT BindObjectAPI();
	HRESULT BindComponentAPI();
	HRESULT BindTransformAPI();
	HRESULT BindCameraAPI();
	HRESULT BindUtilityAPI();

	CGameObject* ResolveObject(const CHandle& hObject) const;

	friend class CLuaScriptInstance;

	sol::table CreateTable();
	HRESULT ExecuteModule(
		const std::string& sSource,
		const std::string& sChunkName,
		const sol::table& Context,
		sol::environment& OutEnvironment,
		sol::table& OutExports);

private:
	sol::state m_Lua{};
	std::vector<WPtr<CLuaScriptInstance>> m_ScriptInstances{};
	std::unordered_map<std::string, LUA_OBJECT_SPAWN_FACTORY> m_ObjectSpawnFactories{};
	std::unordered_map<std::string, LUA_COMPONENT_ATTACH_FACTORY> m_ComponentAttachFactories{};
	UPtr<CLuaWatcher> m_pLuaWatcher{};

public:
	static UPtr<CLuaManager> Create();

protected:
	void Free() override;
};

NS_END
