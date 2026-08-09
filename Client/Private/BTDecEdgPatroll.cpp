#include "pch.h"
#include "BTDecEdgPatroll.h"
#include "ComBeHavior.h"
#include "BTBlackBoard.h"
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTDecEdgPatroll::CBTDecEdgPatroll()
{

}
CBTDecEdgPatroll::CBTDecEdgPatroll(const CBTDecEdgPatroll& rhs) : CBTDecorator(rhs)
{
}

CBTDecEdgPatroll::~CBTDecEdgPatroll()
{
}
HRESULT CBTDecEdgPatroll::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);

	m_eGroup = NODEGROUP::DECORATOR;
	m_MasterName = "BTDecEdgPatroll";
	return S_OK;
}
HRESULT CBTDecEdgPatroll::Initalize(void* pArg)
{

	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTDecEdgPatroll::Evaluate(_float fTimeDelta)
{
	//--------------NullCheck---------------//
	auto pBT = Get_ComBT();
	if (!pBT) return m_eDebug = EVALUATE::FAILED;

	auto pBB = pBT->Get_Blackboard();
	if (!pBB) return m_eDebug = EVALUATE::FAILED;

	MOVE* eState = pBB->Get_Value<MOVE>(EDG_KEY::EPATROL);
	if (nullptr == eState) return  m_eDebug = EVALUATE::FAILED;
	//--------------------------------------//

	if (m_eState == *eState)
	{
		EVALUATE result = __super::Evaluate(fTimeDelta);
		if (EVALUATE::SUCCESS == Patroll(*eState, pBB))
		{
			if (result == EVALUATE::SUCCESS)
			{
				if (m_eState == MOVE::LEFT)
					pBB->Set_Value<MOVE>(EDG_KEY::EPATROL, MOVE::RIGHT);
				else if (m_eState == MOVE::RIGHT)
					pBB->Set_Value<MOVE>(EDG_KEY::EPATROL, MOVE::LEFT);
			}
		}
		
		return m_eDebug = result;
	
	}
	
	return m_eDebug = EVALUATE::FAILED;
}
void CBTDecEdgPatroll::Update_Gui()
{
	ImGui::Text("Move");
	if (ImGui::BeginCombo("##Move", MagicEnumToStringView(m_eState).data()))
	{
		for (uint32_t i = 0; i < ETOUI(MOVE::END); ++i)
		{

			_bool bSelect = i == ETOUI(m_eState);

			if (ImGui::Selectable(MagicEnumToStringView(static_cast<MOVE>(i)).data(), &bSelect))
			{
				m_eState = static_cast<MOVE>(i);
			}

			if (bSelect)
				ImGui::SetItemDefaultFocus();
		}

		ImGui::EndCombo();
	}
}
nlohmann::json CBTDecEdgPatroll::Save_Node()
{
	nlohmann::json j = __super::Save_Node();
	SaveJsonEnum(j, "Patroll", m_eState);
	return  j;
}
HRESULT CBTDecEdgPatroll::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	LoadJsonEnum(j, "Patroll", m_eState);
	return S_OK;
}
EVALUATE CBTDecEdgPatroll::Patroll(MOVE eState, CBTBlackBoard* pBB)
{
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if (nullptr == pMoveIntent) return EVALUATE::FAILED;

	auto pDragon = static_cast<CEnderDragon*>(pMoveIntent->GetGameObject());
	if (nullptr == pDragon) return EVALUATE::FAILED;
	EVALUATE result{};
	_float3 vSrcPos = pDragon->GetTransform().GetPosition();
	_float3 vDir{};
	
	if (eState == MOVE::LEFT)
		result = Moving(vDir, vSrcPos,  EDG_KEY::LPATROL, pBB);
	else if (eState == MOVE::RIGHT)
		result = Moving(vDir, vSrcPos,  EDG_KEY::RPATROL, pBB);
	
	if (result == EVALUATE::SUCCESS)
		return result;
	_float3 vMoveDirection{};
	XMStoreFloat3(&vMoveDirection, XMVector3Normalize(XMVectorSetY(XMLoadFloat3(&vDir),0.f)));
	pMoveIntent->SetMoveIntent(vMoveDirection, 15.f);

	return result;
}
EVALUATE CBTDecEdgPatroll::Moving(_float3& vOutDir, _float3 vSrcPos, const StringID ArrowKey ,CBTBlackBoard* pBB)
{
	_float3* vDestPos = pBB->Get_Value<_float3>(ArrowKey);
	XMStoreFloat3(&vOutDir, XMLoadFloat3(vDestPos) - XMLoadFloat3(&vSrcPos));
	_float fDistance = XMVectorGetX(XMVector3Length(XMLoadFloat3(&vOutDir)));
	if (fDistance <= 0.5f)
		return EVALUATE::SUCCESS;
	
	return  EVALUATE::FAILED;
}
E::UPtr<CBTDecEdgPatroll> CBTDecEdgPatroll::Create()
{
	auto pInstance = E::ToUPtr(new CBTDecEdgPatroll{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTDecEdgPatroll");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTDecEdgPatroll::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTDecEdgPatroll{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTDecEdgPatroll");
		return nullptr;
	}

	return pInstance;
}
