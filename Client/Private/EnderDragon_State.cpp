#include "pch.h"
#include "EnderDragon_State.h"

NS_USING(Client)
CEnderDragon_State::CEnderDragon_State()
{
}

CEnderDragon_State::CEnderDragon_State(const CEnderDragon_State& rhs) : CStateMachine{ rhs }
{
}

CEnderDragon_State::~CEnderDragon_State()
{
}

HRESULT CEnderDragon_State::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

_bool CEnderDragon_State::Add_State(EDG_STATE eState, SPtr<CState> pState)
{
	if (eState == EDG_STATE::NONE || eState == EDG_STATE::END || nullptr == pState || IsRegistered(eState))
		return false;

	__super::AddState(ETOUI(eState), std::move(pState));
	m_RegisteredState.insert(ETOUI(eState));

	return true;
}

_bool CEnderDragon_State::Initialize_State(EDG_STATE eState)
{
	//초기상태 지정
	if (m_eCurState != EDG_STATE::NONE || !IsRegistered(eState))
		return false;

	__super::ChangeState(ETOUI(eState));
	m_eCurState = eState;

	return true;
}

_bool CEnderDragon_State::Request_State(EDG_STATE eState)
{
	if (!IsRegistered(eState) || m_eCurState == eState)
		return false;

	m_eRequestState = eState;

	return true;
}
void CEnderDragon_State::ApplyStateRequest()
{
	//상태가 없으면 아무것도 하지 않음
	if (m_eRequestState == EDG_STATE::NONE)
		return;

	//예약된 상태를 이동하고 기존 예약 칸을 비움
	EDG_STATE eNextState = m_eRequestState;
	m_eRequestState = EDG_STATE::NONE;

	if (!IsRegistered(eNextState) || m_eCurState == eNextState)
		return;

	__super::ChangeState(ETOUI(eNextState));
	m_eCurState = eNextState;
}
void CEnderDragon_State::PriorityUpdate(_float fTimeDelta)
{
	ApplyStateRequest();
	__super::PriorityUpdate(fTimeDelta);
}
_bool CEnderDragon_State::IsRegistered(EDG_STATE eState)
{
	if (eState == EDG_STATE::NONE || eState == EDG_STATE::END)
		return false;

	return m_RegisteredState.contains(ETOUI(eState));
}


UPtr<CEnderDragon_State> CEnderDragon_State::Create()
{
	auto pInstance = ToUPtr(new CEnderDragon_State{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to create CEnderDragon_State");
		return nullptr;
	}

	return pInstance;
}

UPtr<CPrototype> CEnderDragon_State::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new CEnderDragon_State{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to clone CEnderDragon_State");
		return nullptr;
	}

	return pInstance;
}
