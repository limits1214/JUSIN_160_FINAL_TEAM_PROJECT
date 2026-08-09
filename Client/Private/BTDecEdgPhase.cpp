#include "pch.h"
#include "BTDecEdgPhase.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
NS_USING(Client)

CBTDecEdgPhase::CBTDecEdgPhase()
{

}
CBTDecEdgPhase::CBTDecEdgPhase(const CBTDecEdgPhase& rhs) : CBTDecorator(rhs)
{
	m_eState = rhs.m_eState;
}

CBTDecEdgPhase::~CBTDecEdgPhase()
{
}
HRESULT CBTDecEdgPhase::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecEdgPhase";
	return S_OK;
}
HRESULT CBTDecEdgPhase::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecEdgPhase::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck---------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if (!pBB) return m_eDebug = EVALUATE::FAILED;

	auto pState = pBB->Get_Value<DRAGON_PHASE>(EDG_KEY::EDGPHASE);
	if (!pState) return m_eDebug = EVALUATE::FAILED;
	//--------------------------------------//

	if (*pState != m_eState)
		return m_eDebug = EVALUATE::FAILED;

	return m_eDebug = __super::Evaluate(fTimeDelta);
}
void CBTDecEdgPhase::Update_Gui()
{
	ImGui::Text("EdgState");
	if (ImGui::BeginCombo("##EdgState", MagicEnumToStringView(m_eState).data()))
	{
		for (uint32_t i = 0; i < ETOUI(DRAGON_PHASE::END); ++i)
		{
			_bool bSelect = i == ETOUI(m_eState);

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<DRAGON_PHASE>(i)).data(), bSelect))
			{
				m_eState = static_cast<DRAGON_PHASE>(i);
			}
			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}


}
nlohmann::json CBTDecEdgPhase::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "EdgState", m_eState);
	return  j;
}
HRESULT CBTDecEdgPhase::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "EdgState", m_eState);
	return S_OK;
}
E::UPtr<CBTDecEdgPhase> CBTDecEdgPhase::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecEdgPhase{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecEdgPhase");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecEdgPhase::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecEdgPhase{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecEdgPhase");
		return nullptr;
	}

	return pInstance;
}
