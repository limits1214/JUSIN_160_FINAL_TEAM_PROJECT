#include "pch.h"
#include "BTDecHitCnt.h"
#include "ComBeHavior.h"
#include "Monster.h"
NS_USING(Client)

CBTDecHitCnt::CBTDecHitCnt()
{

}
CBTDecHitCnt::CBTDecHitCnt(const CBTDecHitCnt& rhs) : CBTDecorator(rhs)
{

}

CBTDecHitCnt::~CBTDecHitCnt()
{
}
HRESULT CBTDecHitCnt::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecHitCnt";
	return S_OK;
}
HRESULT CBTDecHitCnt::Initalize(void* pArg)
{

	__super::Initalize(pArg);
	return S_OK;
}
EVALUATE CBTDecHitCnt::Evaluate(_float fTimeDelta)
{
	if (auto pBT = Get_ComBT())
	{
		if (auto pMonster = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			uint32_t i = pMonster->GetNormalCnt();
			if (pMonster->GetNormalCnt() > m_iMaxHitCnt)
				return m_eDebug = EVALUATE::FAILED;

			return m_eDebug = __super::Evaluate(fTimeDelta);
		}

	}

	return m_eDebug = EVALUATE::FAILED;
}


void CBTDecHitCnt::Update_Gui()
{
	ImGui::Text("MaxHitCnt :");
	ImGui::DragInt("##Hitcnt", &m_iMaxHitCnt, 0.1f, 0, 100);
}
void CBTDecHitCnt::Abort()
{
	__super::Abort();
}
nlohmann::json CBTDecHitCnt::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonValue(j, "HitCnt", m_iMaxHitCnt);


	return j;
}
HRESULT CBTDecHitCnt::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonValue(j, "HitCnt", m_iMaxHitCnt);
	return S_OK;
}
E::UPtr<CBTDecHitCnt> CBTDecHitCnt::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecHitCnt{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecHitCnt");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecHitCnt::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecHitCnt{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecHitCnt");
		return nullptr;
	}

	return pInstance;
}
