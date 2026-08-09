#include "pch.h"
#include "BTDecEdgState.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CBTDecEdgState::CBTDecEdgState()
{

}
CBTDecEdgState::CBTDecEdgState(const CBTDecEdgState& rhs) : CBTDecorator(rhs)
{
	m_eState = rhs.m_eState;
}

CBTDecEdgState::~CBTDecEdgState()
{
}
HRESULT CBTDecEdgState::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecEdgState";
	return S_OK;
}
HRESULT CBTDecEdgState::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecEdgState::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck---------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if(!pBB) return m_eDebug = EVALUATE::FAILED;

	auto pState = pBB->Get_Value<EDG_STATE>(EDG_KEY::STATE);
	if (!pState) return m_eDebug = EVALUATE::FAILED;
	//--------------------------------------//

	if (*pState != m_eState)
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = __super::Evaluate(fTimeDelta);
}
void CBTDecEdgState::Update_Gui()
{
	ImGui::Text("EdgState");
	if(ImGui::BeginCombo("##EdgState",MagicEnumToStringView(m_eState).data()))
	{
		for (uint32_t i = 0; i < ETOUI(EDG_STATE::END); ++i)
		{
			_bool bSelect = i == ETOUI(m_eState);

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<EDG_STATE>(i)).data(), bSelect))
			{
				m_eState = static_cast<EDG_STATE>(i);
			}
			if(bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	
}
nlohmann::json CBTDecEdgState::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "EdgState", m_eState);
	return  j;
}
HRESULT CBTDecEdgState::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "EdgState", m_eState);
	return S_OK;
}
E::UPtr<CBTDecEdgState> CBTDecEdgState::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecEdgState{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecEdgState");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecEdgState::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecEdgState{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecEdgState");
		return nullptr;
	}

	return pInstance;
}
