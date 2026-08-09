#include "pch.h"
#include "BTRandSelector.h"
#include "BTSecqunce.h"
CBTRandSelector::CBTRandSelector()
{
}

CBTRandSelector::CBTRandSelector(const CBTRandSelector& rhs) : CBTComposite(rhs)
{

}
CBTRandSelector::~CBTRandSelector()
{
}

HRESULT CBTRandSelector::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTRandSelector";
	m_eGroup = NODEGROUP::RAND_SELECTOR;
	return S_OK;
}

HRESULT CBTRandSelector::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	NODEGROUP eGroup = m_eGroup;
	m_GuiNode.vColor = _float4(0.5294f, 0.9843f, 1.f, 1.f);
	return S_OK;
}

void CBTRandSelector::OnEnter()
{

	Shuffle();
	
}

void CBTRandSelector::OnExit(EVALUATE eResult)
{
	if (m_ePreEvaluate == EVALUATE::RUN)
	{
		if (!m_Shuffle.empty())
			m_Shuffle.pop_back();
	}
}

EVALUATE CBTRandSelector::Evaluate(_float fTimeDelta)
{
	if (m_Shuffle.empty())
		return m_eDebug = EVALUATE::FAILED;;

	uint32_t iRand = m_Shuffle.back();

	if (m_NodeValue.bCur)
		iRand = m_NodeValue.iPreSecquenceIndex;
	
	if (iRand >= m_Actions.size() || nullptr == m_Actions[iRand])
	{
		m_NodeValue.bCur = false;
		m_Shuffle.pop_back();
		return 	m_ePreEvaluate = m_eDebug = EVALUATE::FAILED;
	}
		
	EVALUATE eValuate = m_Actions[iRand]->Execute(fTimeDelta);
	if (eValuate == EVALUATE::SUCCESS)
	{
		m_NodeValue.bCur = false;
		m_Shuffle.pop_back();
		return m_ePreEvaluate = m_eDebug = EVALUATE::SUCCESS;
	}
	else if (eValuate == EVALUATE::RUN)
	{
		m_NodeValue.bCur = true;
		m_NodeValue.iPreSecquenceIndex = iRand;
		return m_ePreEvaluate = m_eDebug = EVALUATE::RUN;
	}
	m_NodeValue.bCur = false;
	m_Shuffle.pop_back();
	return m_ePreEvaluate = m_eDebug = EVALUATE::FAILED;
}

void CBTRandSelector::Abort()
{
	for (size_t i = 0; i < m_Actions.size(); ++i)
	{
		if (nullptr != m_Actions[i])
			m_Actions[i]->AbortExecute();
	}
	m_NodeValue.bCur = false;
}

nlohmann::json CBTRandSelector::Save_Node()
{
	return __super::Save_Node();
}



HRESULT CBTRandSelector::Load_json(const nlohmann::json& j)
{
	return __super::Load_json(j);
}

void CBTRandSelector::Shuffle()
{
	if(m_Actions.empty())
		return;
	if (!m_Shuffle.empty())
		return;

	uint32_t iRand = m_Actions.size();

	for (size_t i = 0; i < iRand; ++i)
		m_Shuffle.push_back(i);

	for (size_t i = m_Shuffle.size(); i > 1; --i)
	{
		size_t iRand = rand() % i;

		std::swap(m_Shuffle[i - 1], m_Shuffle[iRand]);
	}
}

UPtr<CBTRandSelector> CBTRandSelector::Create(void* pArg)
{
	auto pInstance = ToUPtr(new CBTRandSelector());
	if (FAILED(pInstance->InitializePrototype(pArg)))
	{
		MSG_BOX("Failed to Created : CBTRandSelector");
		return nullptr;
	}
	return pInstance;
}

E::UPtr<E::CPrototype> CBTRandSelector::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTRandSelector{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTRandSelector");
		return nullptr;
	}

	return pInstance;
}

