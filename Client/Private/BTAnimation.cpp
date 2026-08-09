#include "pch.h"
#include "BTAnimation.h"
#include "ComAnimator.h" 
NS_USING(Client)

CBTAnimation::CBTAnimation()
{

}
CBTAnimation::CBTAnimation(const CBTAnimation& rhs) : CBTAnimRoot(rhs)
{

}

CBTAnimation::~CBTAnimation()
{
}
HRESULT CBTAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTAnimation";
	return S_OK;
}
HRESULT CBTAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);
	
	return S_OK;
}

EVALUATE CBTAnimation::Evaluate(_float fTimeDelta)
{
	auto pBT = Get_ComBT();
	if (!pBT) return EVALUATE::FAILED;
	auto pOwner = static_cast<CMonster*>(pBT->GetGameObject());
	if (!pOwner) return EVALUATE::FAILED;
	auto pTarget = pOwner->Get_Target();
	if (!pTarget) return EVALUATE::FAILED;

	auto pTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	auto pAnimator =(Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));

	if (!pTransform || !pMoveIntent ||!pAnimator || -1 == m_Value.iAnimIndex)
		return m_eDebug = EVALUATE::FAILED;
	_float fAnimRatio = pAnimator->GetPlayAnimRatio();
	if (m_bStart)
		pAnimator->SetPlay(true);
	if (!m_bUseCurAnim)
		pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);

	_bool bFinished = pAnimator->GetFinish();
	Gravity();
	Play_Sound(fTimeDelta);
	EventFlagToRatio(fAnimRatio);
	Rotation(pTransform, pMoveIntent, pTarget, fTimeDelta, fAnimRatio);
	if (m_bEarly && m_fEarlyRatio <= fAnimRatio || bFinished)
	{
		return m_eDebug = EVALUATE::SUCCESS;
	}
	if (m_bLoop || bFinished)
	{
		return m_eDebug = EVALUATE::SUCCESS;
	}

	return m_eDebug = EVALUATE::RUN;
}
void CBTAnimation::Update_Gui()
{

	BoolButton("UseCurAnim : ", m_bUseCurAnim);
	__super::Update_Gui();
	if (ImGui::Button("Abort : ")) 
		m_GuiNode.bAbort = !m_GuiNode.bAbort;
	ImGui::Text("Abort : %s", m_GuiNode.bAbort ? "TRUE" : "FALSE");

	if (ImGui::Button("Animation"))
		m_bPopup = true;
	if (m_bPopup)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_Value.iAnimIndex = iIndex;
		}

		ImGui::PopStyleColor();
	}
}
void CBTAnimation::Abort()
{
	__super::Abort();
}
nlohmann::json CBTAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	SaveJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	return j;
}
HRESULT CBTAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);

	LoadJsonValue(j, "UseCurAnim", m_bUseCurAnim);
	return S_OK;
}
void CBTAnimation::OnEnter()
{
	__super::OnEnter();
}
void CBTAnimation::OnExit(EVALUATE eResult)
{
}
E::UPtr<CBTAnimation> CBTAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTAnimation");
		return nullptr;
	}

	return pInstance;
}
