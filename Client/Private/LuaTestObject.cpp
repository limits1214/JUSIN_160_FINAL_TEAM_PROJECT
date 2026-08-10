#include "pch.h"
#include "LuaTestObject.h"

#include "GameInstance.h"
#include "LuaManager.h"
#include "LuaScriptInstance.h"

NS_USING(Client)

CLuaTestObject::CLuaTestObject() = default;

CLuaTestObject::CLuaTestObject(const CLuaTestObject& Prototype)
	: CGameObject{ Prototype }
{
}

HRESULT CLuaTestObject::Initialize(void* pArg)
{
	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	auto* pLuaManager = CGameInstance::Get().GetLuaManager();
	if (!pLuaManager)
		return E_FAIL;

	m_pLuaScript = pLuaManager->CreateScriptInstanceByPath(
		"./LuaFiles/Test.lua",
		"LuaTestObject");
	if (!m_pLuaScript)
		return E_FAIL;

	m_pLuaScript->SetContext("ownerHandle", GetHandle());
	if (FAILED(m_pLuaScript->Load()))
		return E_FAIL;

	return S_OK;
}

void CLuaTestObject::OnRegisteredToManager()
{
	if (!m_pLuaScript)
	{
		SetPendingDestroyCascade();
		return;
	}

	// [LSY] Manager의 슬롯과 레이어 등록이 끝난 시점이므로 Lua에서 ownerHandle을 안전하게 조회할 수 있다.
	const LUA_CALL_RESULT eCreateResult = m_pLuaScript->Call("OnCreate");
	if (eCreateResult == LUA_CALL_RESULT::SCRIPT_ERROR ||
		eCreateResult == LUA_CALL_RESULT::INVALID_INSTANCE)
	{
		SetPendingDestroyCascade();
		return;
	}

	m_bLuaCreated = true;
}

void CLuaTestObject::PriorityUpdate(_float fTimeDelta)
{
	if (!m_pLuaScript)
		return;

	m_pLuaScript->Call("PriorityUpdate", fTimeDelta);
}

void CLuaTestObject::FixedUpdate(_float fTimeDelta)
{
	if (m_pLuaScript)
		m_pLuaScript->Call("FixedUpdate", fTimeDelta);
}

void CLuaTestObject::Update(_float fTimeDelta)
{
	if (m_pLuaScript)
		m_pLuaScript->Call("Update", fTimeDelta);
}

void CLuaTestObject::LateUpdate(_float fTimeDelta)
{
	if (m_pLuaScript)
		m_pLuaScript->Call("LateUpdate", fTimeDelta);
	GetTransform().Update();
}

UPtr<CLuaTestObject> CLuaTestObject::Create()
{
	auto pInstance = ToUPtr(new CLuaTestObject{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Create: CLuaTestObject");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CLuaTestObject::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CLuaTestObject{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Clone: CLuaTestObject");
		return nullptr;
	}

	return pInstance;
}

void CLuaTestObject::Free()
{
	if (m_pLuaScript && m_bLuaCreated)
		m_pLuaScript->Call("OnDestroy");
	m_pLuaScript.reset();

	CGameObject::Free();
}
