#include "pch.h"
#include "LuaScriptInstance.h"

#include "LuaManager.h"

NS_USING(Engine)

CLuaScriptInstance::CLuaScriptInstance(CLuaManager& LuaManager)
	: m_pLuaManager{ &LuaManager }
{
}

CLuaScriptInstance::~CLuaScriptInstance() = default;

HRESULT CLuaScriptInstance::Initialize(const DESC& Desc)
{
	if (!Desc.pScript)
		return E_INVALIDARG;

	m_pScript = Desc.pScript;
	m_sDebugName = Desc.sDebugName.empty() ? m_pScript->GetPath() : Desc.sDebugName;
	m_Context = m_pLuaManager->CreateTable();

	return S_OK;
}

HRESULT CLuaScriptInstance::Load()
{
	if (!m_pLuaManager || !m_pScript || !m_Context.valid())
		return E_FAIL;

	sol::environment NewEnvironment{};
	sol::table NewExports{};
	if (FAILED(m_pLuaManager->ExecuteModule(
		m_pScript->GetSource(),
		m_pScript->GetPath(),
		m_Context,
		NewEnvironment,
		NewExports)))
	{
		return E_FAIL;
	}

	m_Environment = std::move(NewEnvironment);
	m_Exports = std::move(NewExports);
	m_bLoaded = true;

	return S_OK;
}

HRESULT CLuaScriptInstance::Reload()
{
	if (FAILED(Load()))
		return E_FAIL;

	const LUA_CALL_RESULT eReloadResult = Call("OnReload");
	if (eReloadResult == LUA_CALL_RESULT::SCRIPT_ERROR ||
		eReloadResult == LUA_CALL_RESULT::INVALID_INSTANCE)
	{
		return E_FAIL;
	}

	return S_OK;
}

void CLuaScriptInstance::RemoveContext(std::string_view sName)
{
	if (m_Context.valid())
		m_Context[std::string{ sName }] = sol::lua_nil;
}

_bool CLuaScriptInstance::HasFunction(std::string_view sFunctionName) const
{
	if (!m_bLoaded || !m_Exports.valid())
		return false;

	const sol::object FunctionObject = m_Exports[std::string{ sFunctionName }];
	return FunctionObject.valid() && FunctionObject.get_type() == sol::type::function;
}

const _string& CLuaScriptInstance::GetScriptPath() const
{
	static const _string sEmptyPath{};
	return m_pScript ? m_pScript->GetPath() : sEmptyPath;
}

SPtr<CLuaScriptInstance> CLuaScriptInstance::Create(
	CLuaManager& LuaManager,
	const DESC& Desc)
{
	auto pInstance = ToSPtr(new CLuaScriptInstance{ LuaManager });
	if (FAILED(pInstance->Initialize(Desc)))
		return nullptr;

	return pInstance;
}

void CLuaScriptInstance::Invalidate()
{
	m_bLoaded = false;
	m_Exports = {};
	m_Context = {};
	m_Environment = {};
	m_pScript.reset();
	m_pLuaManager = nullptr;
}

void CLuaScriptInstance::Free()
{
	Invalidate();

	CEngineBase::Free();
}
