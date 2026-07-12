#pragma once
#include "Component.h"
#include "ResLuaScript.h"
#include "ILuaScriptRelodable.h"
NS_BEGIN(Engine)

class ENGINE_DLL CComLuaScript : public CComponent, public ILuaScriptRelodable
{
public:
	struct DESC : public CComponent::DESC
	{
		SPtr<CResLuaScript> pResScript{};
		std::function<void(CComLuaScript*)> funcScriptLoad{};
	};

public:
	DECLARE_DERIVED_TYPE(CComLuaScript, CComponent)

public:
	void UpdateGUI() override;

protected:
	explicit CComLuaScript();
	~CComLuaScript() override;

private:
	HRESULT Initialize(void* pArg) override;

public:
	virtual void PriorityUpdate(_float fTimeDelta);
	virtual void FixedUpdate(_float fTimeDelta);
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);
public:
	void LuaScriptRelod() override; // 외부(Manager)에서 호출할 리로드 함수

protected:
	void LoadScript(); // 환경 설정 + 실행 + 캐싱을 담당하는 공통 함수
public:
	//void CacheFunctions(_string_view name, sol::protected_function& out);
protected:
	sol::protected_function m_OnCreate;
	sol::protected_function m_OnDestroy;

	sol::protected_function m_PriorityUpdate;
	sol::protected_function m_FixedUpdate;
	sol::protected_function m_Update;
	sol::protected_function m_LateUpdate;

protected:
	std::function<void(CComLuaScript*)> m_funcScriptLoad{};

public:
	static UPtr<CComLuaScript> Create();
	UPtr<CPrototype> Clone(void* pArg) override;

public:
	sol::environment& GetEnv() { return m_Environment; }

protected:
	SPtr<CResLuaScript> m_pResLuaScript{};
	sol::environment m_Environment;

protected:
	void Free() override;
};

NS_END
