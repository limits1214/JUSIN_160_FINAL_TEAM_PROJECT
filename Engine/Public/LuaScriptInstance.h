#pragma once

#include "Engine_Defines.h"
#include "ResLuaScript.h"

NS_BEGIN(Engine)

class CLuaManager;

enum class LUA_CALL_RESULT
{
	SUCCESS,
	FUNCTION_NOT_FOUND,
	SCRIPT_ERROR,
	INVALID_INSTANCE,
};

class ENGINE_DLL CLuaScriptInstance final : public CEngineBase
{
public:
	struct DESC
	{
		SPtr<CResLuaScript> pScript{};
		_string sDebugName{};
	};

public:
	DECLARE_DERIVED_TYPE(CLuaScriptInstance, CEngineBase)

private:
	explicit CLuaScriptInstance(CLuaManager& LuaManager);
	~CLuaScriptInstance() override;

private:
	HRESULT Initialize(const DESC& Desc);

public:
	HRESULT Load();
	HRESULT Reload();

	template<typename T>
	void SetContext(std::string_view sName, T&& Value)
	{
		if (!m_Context.valid())
			return;

		m_Context[std::string{ sName }] = std::forward<T>(Value);
	}

	void RemoveContext(std::string_view sName);
	_bool HasFunction(std::string_view sFunctionName) const;
	_bool IsLoaded() const { return m_bLoaded; }
	const _string& GetDebugName() const { return m_sDebugName; }
	const _string& GetScriptPath() const;

	template<typename... Args>
	LUA_CALL_RESULT Call(std::string_view sFunctionName, Args&&... args)
	{
		if (!m_bLoaded || !m_Exports.valid() || !m_Context.valid())
			return LUA_CALL_RESULT::INVALID_INSTANCE;

		sol::object FunctionObject = m_Exports[std::string{ sFunctionName }];
		if (!FunctionObject.valid() || FunctionObject.get_type() != sol::type::function)
			return LUA_CALL_RESULT::FUNCTION_NOT_FOUND;

		sol::protected_function Function = FunctionObject.as<sol::protected_function>();
		sol::protected_function_result Result =
			Function(m_Context, std::forward<Args>(args)...);

		if (!Result.valid())
		{
			sol::error Error = Result;
			OutputDebugStringA(("[Lua Script Error][" + m_sDebugName + "] " +
				std::string{ Error.what() } + "\n").c_str());
			return LUA_CALL_RESULT::SCRIPT_ERROR;
		}

		return LUA_CALL_RESULT::SUCCESS;
	}

	template<typename TResult, typename... Args>
	LUA_CALL_RESULT CallWithResult(
		std::string_view sFunctionName,
		TResult& OutResult,
		Args&&... args)
	{
		if (!m_bLoaded || !m_Exports.valid() || !m_Context.valid())
			return LUA_CALL_RESULT::INVALID_INSTANCE;

		sol::object FunctionObject = m_Exports[std::string{ sFunctionName }];
		if (!FunctionObject.valid() || FunctionObject.get_type() != sol::type::function)
			return LUA_CALL_RESULT::FUNCTION_NOT_FOUND;

		sol::protected_function Function = FunctionObject.as<sol::protected_function>();
		sol::protected_function_result Result =
			Function(m_Context, std::forward<Args>(args)...);

		if (!Result.valid())
		{
			sol::error Error = Result;
			OutputDebugStringA(("[Lua Script Error][" + m_sDebugName + "] " +
				std::string{ Error.what() } + "\n").c_str());
			return LUA_CALL_RESULT::SCRIPT_ERROR;
		}

		try
		{
			OutResult = Result.get<TResult>();
		}
		catch (const std::exception& Exception)
		{
			OutputDebugStringA(("[Lua Return Type Error][" + m_sDebugName + "] " +
				std::string{ Exception.what() } + "\n").c_str());
			return LUA_CALL_RESULT::SCRIPT_ERROR;
		}

		return LUA_CALL_RESULT::SUCCESS;
	}

private:
	friend class CLuaManager;
	void Invalidate();

	CLuaManager* m_pLuaManager{};
	SPtr<CResLuaScript> m_pScript{};
	_string m_sDebugName{};
	sol::environment m_Environment{};
	sol::table m_Context{};
	sol::table m_Exports{};
	_bool m_bLoaded{};

public:
	static SPtr<CLuaScriptInstance> Create(
		CLuaManager& LuaManager,
		const DESC& Desc);

protected:
	void Free() override;
};

NS_END
