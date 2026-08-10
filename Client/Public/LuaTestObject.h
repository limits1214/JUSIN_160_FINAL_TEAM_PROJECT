#pragma once

#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CLuaScriptInstance;
NS_END

NS_BEGIN(Client)

class CLuaTestObject final : public CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CLuaTestObject, CGameObject)

private:
	CLuaTestObject();
	CLuaTestObject(const CLuaTestObject& Prototype);
	~CLuaTestObject() override = default;

public:
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(_float fTimeDelta) override;
	void FixedUpdate(_float fTimeDelta) override;
	void Update(_float fTimeDelta) override;
	void LateUpdate(_float fTimeDelta) override;

protected:
	void OnRegisteredToManager() override;

private:
	SPtr<CLuaScriptInstance> m_pLuaScript{};
	_bool m_bLuaCreated{};

public:
	static UPtr<CLuaTestObject> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

protected:
	void Free() override;
};

NS_END
