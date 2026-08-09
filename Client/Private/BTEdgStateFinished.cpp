#include "pch.h"
#include "BTEdgStateFinished.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
#include "BlackBoardKey.h"

NS_USING(Client)

CBTEdgStateFinished::CBTEdgStateFinished()
{

}
CBTEdgStateFinished::CBTEdgStateFinished(const CBTEdgStateFinished& rhs) : CBTActionNode(rhs)
{

}

CBTEdgStateFinished::~CBTEdgStateFinished()
{
}
HRESULT CBTEdgStateFinished::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::ACTION;
	m_MasterName = "BTEdgStateFinished";
	return S_OK;
}
HRESULT CBTEdgStateFinished::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	return S_OK;
}


EVALUATE CBTEdgStateFinished::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck----------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if (!pBB) return m_eDebug = EVALUATE::FAILED;
	//---------------------------------------//
	
	pBB->Set_Value<_bool>(EDG_KEY::BSTATE_FINISHED, true);

	return m_eDebug = EVALUATE::SUCCESS;
}
void CBTEdgStateFinished::Update_Gui()
{
}
nlohmann::json CBTEdgStateFinished::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	return j;
}
HRESULT CBTEdgStateFinished::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	return S_OK;
}
void CBTEdgStateFinished::OnEnter()
{
}
void CBTEdgStateFinished::OnExit(EVALUATE eResult)
{
}
E::UPtr<CBTEdgStateFinished> CBTEdgStateFinished::Create()
{
	auto pInstance = E::ToUPtr(new CBTEdgStateFinished{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTEdgStateFinished");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTEdgStateFinished::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTEdgStateFinished{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTEdgStateFinished");
		return nullptr;
	}

	return pInstance;
}
