#pragma once
#include "Engine_Defines.h"
#include "ILuaScriptRelodable.h"

NS_BEGIN(Engine)
class CLuaWatcher;
class CResLuaScript;
class CComLuaScript;
class CLuaManager final : public CEngineBase
{
private:
	CLuaManager();
	~CLuaManager();

public:
	void UpdateGUI();

public:
	void Update(_float fTimeDelta);

private:
	HRESULT Initialize();
	HRESULT Initialize_PrintBinding();
	HRESULT Initialize_RegistType();
	HRESULT Initialize_DebuggerBinding();
	HRESULT Initialize_MathBinding();
	HRESULT Initialize_GameInstanceBindnig();
	HRESULT Initialize_ClassBindnig();
	HRESULT Initialize_DefineBinding();

public:
	sol::protected_function CacheFunction(const std::string& funcName);
	sol::protected_function CacheFunction(const sol::environment& env, const std::string& funcName);
	template<typename... Args>
	bool CallCacheFunction(const sol::protected_function& func, Args&&... args)
	{
		// 1. 함수가 비어있거나 유효하지 않으면 패스
		if (!func.valid())
			return false;

		// 2. 가변 인자를 풀어서(std::forward) 루아 함수 실행
		auto result = func(std::forward<Args>(args)...);

		// 3. 에러 발생 시 로그 출력
		if (!result.valid())
		{
			sol::error err = result;
			std::string errorMsg = "[Lua Script Error] " + std::string(err.what()) + "\n";
			OutputDebugStringA(errorMsg.c_str());
			return false; // 실행 실패
		}

		return true; // 실행 성공
	}

public:
	bool HasFunction(sol::environment& env, std::string_view function) const;

	//HRESULT Execute(const std::string& script, const sol::environment& env, const std::string& path);

	// Env(독립된 환경) 내부에서 실행하는 버전
	HRESULT Execute(const std::string& script, const sol::environment& env, const std::string& chunkName = "InlineScript");

	// State(전역) 레벨에서 직접 실행하는 버전
	HRESULT Execute(const std::string& script, const std::string& chunkName = "InlineScript");

	sol::environment CreateEnvironment();

	template<typename... Args>
	HRESULT Call(sol::environment& env, std::string_view function, Args&&... args);

	template<typename T>
	void SetValue( sol::environment& env, std::string_view name, T&& value)
	{
		env[std::string(name)] = std::forward<T>(value);
	}

	template<typename T>
	bool GetValue( sol::environment& env, std::string_view name, T& value)
	{
		sol::object obj = env[std::string(name)];

		if (!obj.valid())
			return false;

		if (!obj.is<T>())
			return false;

		value = obj.as<T>();
		return true;
	}

	template<typename T>
	void SetValue(std::string_view name, T&& value)
	{
		// 전역 테이블(globals)에 직접 값을 기록합니다.
		m_Lua.globals()[name] = std::forward<T>(value);
	}

	template<typename T>
	bool GetValue(std::string_view name, T& outValue)
	{
		// 전역 테이블에서 값을 찾아옵니다.
		sol::object obj = m_Lua.globals()[name];
		if (obj.valid() && obj.is<T>())
		{
			outValue = obj.as<T>();
			return true;
		}
		return false;
	}

	HRESULT Compile(const std::string& script);

	template<typename Ret, typename... Args>
	HRESULT Call(sol::environment& env, std::string_view function, Ret& ret, Args&&... args)
	{
		sol::object obj = env[std::string(function)];

		if (!obj.valid())
			return S_FALSE;

		sol::protected_function func =
			obj.as<sol::protected_function>();

		auto result =
			func(std::forward<Args>(args)...);

		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(err.what());
			return E_FAIL;
		}

		ret = result.get<Ret>();

		return S_OK;
	}

	bool IsEnvValid(const sol::environment& env) const { return env.valid(); }
	bool HasValue(const sol::environment& env, std::string_view name) const
	{
		sol::object obj = env[std::string(name)];
		return obj.valid() && obj.get_type() != sol::type::lua_nil;
	}
	void RemoveValue(sol::environment& env, std::string_view name) { env[std::string(name)] = sol::lua_nil; }
	void EnvDump(const sol::environment& env) const;
	void EnvClear(sol::environment& env);
	void RegisterExtension(std::function<void(sol::state&)> extensionFunc) {
		extensionFunc(m_Lua);
	}

public:
	template<typename T>
	void RegisterType() {
		m_TypeRegistry[ StringID{ T::StaticType }] = [this](CEngineBase* pBase) -> sol::object {
			T* pCasted = Cast<T>(pBase);

			// pCasted가 nullptr이면 sol::nil을 반환하여 루아에서 안전하게 처리됨
			if (!pCasted) return sol::nil;

			return sol::make_object(m_Lua, pCasted);
			};
	}
private:
	std::unordered_map<StringID, std::function<sol::object(CEngineBase*)>> m_TypeRegistry;

private:
	// 엔진에서 사용하는 유일한 VM
	sol::state m_Lua{};

	UPtr<CLuaWatcher> m_pLuaWatcher{};

public:
	void UpdateHotReload();
	void OnFileChanged(const std::string& path);

private:
	// 파일 경로를 Key로, 해당 리소스를 참조하는 컴포넌트들을 Value로 관리
	std::unordered_map<std::string, std::vector<ILuaScriptRelodable*>> m_scriptRegistry{};

//public:
//	void RegistHotReloadScriptResource(WPtr<CResLuaScript> pResLuaScript);
//private:
//	std::list<WPtr<CResLuaScript>> m_listResLuascript{};

public:
	// 컴포넌트가 생성될 때 등록
	void RegisterComponent(const std::string& path, ILuaScriptRelodable* pComp);
	// 삭제될 때 제거
	void UnregisterComponent(const std::string& path, ILuaScriptRelodable* pComp);
public:
	static UPtr<CLuaManager> Create();
};

NS_END

template<typename... Args>
inline HRESULT CLuaManager::Call(
	sol::environment& env,
	std::string_view function,
	Args&&... args)
{
	sol::object obj = env[std::string(function)];

	if (!obj.valid() || obj.get_type() != sol::type::function)
		return S_FALSE;   // 함수가 없으면 정상적으로 무시

	sol::protected_function func = obj.as<sol::protected_function>();

	sol::protected_function_result result =
		func(std::forward<Args>(args)...);

	if (!result.valid())
	{
		sol::error err = result;
		OutputDebugStringA((std::string(err.what()) + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}
