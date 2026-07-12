#include "pch.h"
#include "ComLuaScript.h"
#include "GameInstance.h"

NS_USING(Engine)

void CComLuaScript::UpdateGUI()
{
}

CComLuaScript::CComLuaScript()
{
}

CComLuaScript::~CComLuaScript()
{
}

HRESULT CComLuaScript::Initialize(void* pArg)
{
	auto* pDesc = static_cast<DESC*>(pArg);
	if (!pDesc->pResScript)
	{
		MSG_BOX("Lua Com Need Source");
		return E_FAIL;
	}
	m_pResLuaScript = pDesc->pResScript;
	m_funcScriptLoad = pDesc->funcScriptLoad;
	if (FAILED(CComponent::Initialize(pArg)))
	{
		return E_FAIL;
	}

	CGameInstance::Get().LuaRegisterComponent(m_pResLuaScript->GetPath(), this);

	LoadScript();

	if (m_OnCreate.valid())
		m_OnCreate();
	return S_OK;
}

void CComLuaScript::PriorityUpdate(_float fTimeDelta)
{
	CGameInstance::Get().LuaCallCacheFunction(m_PriorityUpdate, fTimeDelta);
}

void CComLuaScript::FixedUpdate(_float fTimeDelta)
{
	CGameInstance::Get().LuaCallCacheFunction(m_FixedUpdate, fTimeDelta);
}

void CComLuaScript::Update(_float fTimeDelta)
{
	CGameInstance::Get().LuaCallCacheFunction(m_Update, fTimeDelta);
}

void CComLuaScript::LateUpdate(_float fTimeDelta)
{
	CGameInstance::Get().LuaCallCacheFunction(m_LateUpdate, fTimeDelta);
}

void CComLuaScript::LuaScriptRelod()
{
	// 환경을 새로 덮어쓰고 다시 로드
	LoadScript();
}

void CComLuaScript::LoadScript()
{
	m_Environment = CGameInstance::Get().LuaCreateEnvironment();

	m_Environment["__ScriptPath"] = m_pResLuaScript->GetPath();

	CGameInstance::Get().LuaScriptExecute(m_pResLuaScript->GetSource(), m_Environment, m_pResLuaScript->GetPath());

	m_OnCreate = CGameInstance::Get().LuaCacheFunction(m_Environment, "OnCreate");
	m_OnDestroy = CGameInstance::Get().LuaCacheFunction(m_Environment, "OnDestroy");

	m_PriorityUpdate = CGameInstance::Get().LuaCacheFunction(m_Environment, "PriorityUpdate");
	m_FixedUpdate = CGameInstance::Get().LuaCacheFunction(m_Environment, "FixedUpdate");

	m_Update = CGameInstance::Get().LuaCacheFunction(m_Environment, "Update");
	m_LateUpdate = CGameInstance::Get().LuaCacheFunction(m_Environment, "LateUpdate");

	m_Environment["self"] = this;
	m_Environment["gameObject"] = GetGameObject();
	m_Environment["transform"] = &GetGameObject()->GetTransform();

	if (m_funcScriptLoad)
	{
		m_funcScriptLoad(this);
	}
}

//void CComLuaScript::CacheFunctions(_string_view name, sol::protected_function& out)
//{
//	sol::object obj = m_Environment[name];
//	if (obj.valid() && obj.get_type() == sol::type::function)
//		out = obj.as<sol::protected_function>();
//}

UPtr<CComLuaScript> CComLuaScript::Create()
{
	auto pInstance = ToUPtr(new CComLuaScript{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CComLuaScript");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> CComLuaScript::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CComLuaScript{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: CComLuaScript");
		return nullptr;
	}
	return pInstance;
}

void CComLuaScript::Free()
{
	// 프로토타입 등록된애들도 프리가 불림
	if (m_pResLuaScript)
	{
		CGameInstance::Get().LuaUnregisterComponent(m_pResLuaScript->GetPath(), this);
	}
	CComponent::Free();
}
