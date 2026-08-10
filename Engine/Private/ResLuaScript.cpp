#include "pch.h"
#include "ResLuaScript.h"
#include "GameInstance.h"
#include "LuaManager.h"

NS_USING(Engine)

HRESULT CResLuaScript::Load(const std::any& arg)
{
	if (m_eState == STATE::LOADED)
	{
		return S_OK;
	}

	{
		m_eState = STATE::LOADING;

		std::ifstream file(m_sPath, std::ios::binary);
		if (!file.is_open())
		{
			m_Source.clear();
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}

		m_Source.assign(
			std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());

		auto pLuaManager = CGameInstance::Get().GetLuaManager();
		if (!pLuaManager || FAILED(pLuaManager->Compile(m_Source)))
		{
			m_Source.clear();
			m_eState = STATE::LOADFAIL;
			return E_FAIL;
		}
	}

	m_eState = STATE::LOADED;
	return S_OK;
}

HRESULT CResLuaScript::Unload(const std::any& arg)
{
	m_Source.clear();
	m_eState = STATE::UNLOAD;
	return S_OK;
}

CResLuaScript::CResLuaScript(const _string& sPath)
	: CResLua{ sPath }
{
}

CResLuaScript::~CResLuaScript()
{
}

SPtr<CResLuaScript> CResLuaScript::Create(const _string& sPath)
{
	return ToSPtr(new CResLuaScript{ sPath });
}

SPtr<CResLuaScript> CResLuaScript::CreateAndLoad(const _string& sPath)
{
	auto pRes = CResLuaScript::Create(sPath);
	if (FAILED(pRes->Load()))
	{
		return nullptr;
	}
	return pRes;
}


HRESULT CResLuaScript::Reload()
{
	std::string PreviousSource = m_Source;
	const STATE ePreviousState = m_eState.load();

	m_eState = STATE::UNLOAD;
	if (SUCCEEDED(Load()))
		return S_OK;

	// 핫리로드 실패 시 현재 실행 중인 인스턴스가 마지막 정상 소스를 계속 사용한다.
	m_Source = std::move(PreviousSource);
	m_eState = ePreviousState;
	return E_FAIL;
}
