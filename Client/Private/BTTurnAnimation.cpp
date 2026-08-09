#include "pch.h"
#include "BTTurnAnimation.h"
#include "ComAnimator.h" 
#include "ComCharacterMoveIntent.h"
NS_USING(Client)

CBTTurnAnimation::CBTTurnAnimation()
{

}
CBTTurnAnimation::CBTTurnAnimation(const CBTTurnAnimation& rhs) : CBTAnimRoot(rhs)
{

}

CBTTurnAnimation::~CBTTurnAnimation()
{
}
HRESULT CBTTurnAnimation::InitializePrototype(void* pArg)
{
	__super::InitializePrototype(pArg);
	m_MasterName = "BTTurnAnimation";
	return S_OK;
}
HRESULT CBTTurnAnimation::Initalize(void* pArg)
{
	__super::Initalize(pArg);

	return S_OK;
}

EVALUATE CBTTurnAnimation::Evaluate(_float fTimeDelta)
{
	auto pAnimator = (Get_Component<CComAnimator>(m_Handle, "ComCModelAnimator"));
	auto pSrcTransform = (Get_Component<CComTransform>(m_Handle, "Com_Transform"));
	auto pMoveIntent = Get_Component<CComCharacterMoveIntent>(m_Handle, "ComCharacterMoveIntent");
	if (auto pBT = Get_ComBT())
	{
		if (auto pOwner = static_cast<CMonster*>(pBT->GetGameObject()))
		{
			if (auto pTarget = pOwner->Get_Target())
			{
				if (pAnimator == nullptr || pSrcTransform == nullptr || pMoveIntent == nullptr || pTarget == nullptr)
					return m_eDebug = EVALUATE::FAILED;
				auto& pDestTransform = pTarget->GetTransform();
				EVALUATE Resut = EVALUATE::END;
				//ㅋㅋ;
				if (!m_bTurn)
				{
					_vector vDestPos = XMLoadFloat3(&pDestTransform.GetPosition());
					_vector vSrcPos = XMLoadFloat3(&pSrcTransform->GetPosition());

					_vector vTargetLook = XMVectorSetY(XMVector3Normalize(vDestPos - vSrcPos), 0);
					_vector vSrcLook = XMVectorSetY(XMVector3Normalize(pSrcTransform->GetState(STATE::LOOK)), 0);

					_float fDot = XMVectorGetX(XMVector3Dot(vSrcLook, vTargetLook));
					_float fCrossY = XMVectorGetY(XMVector3Cross(vSrcLook, vTargetLook));
					if (false == SelectAngle(XMConvertToDegrees(atan2f(fCrossY, fDot))))
						return m_eDebug = EVALUATE::SUCCESS;

					XMStoreFloat3(&m_vCurrentLook, vSrcLook);
					XMStoreFloat3(&m_vTargetLook, vTargetLook);
					pAnimator->SetPlay(true);
					pAnimator->Play_Anim(m_Value.iAnimIndex, m_bLoop, m_fBlend);

					m_bTurn = true;
				}
				m_fTick += fTimeDelta;

				Play_Sound(fTimeDelta);
				const _float fTurnSpeed = std::max(std::abs(m_fAngle) / 1.6f, 1.f);
				pMoveIntent->SetFacingIntent(m_vTargetLook, fTurnSpeed);


				_bool bFinished = pAnimator->GetFinish();

				if (bFinished)
					return m_eDebug = EVALUATE::SUCCESS;
				
			}
		}
	}
	return m_eDebug = EVALUATE::RUN;
}
void CBTTurnAnimation::Update_Gui()
{
	__super::Update_Gui();
	ImGui::Text("Angle : %2.f", m_GuiNode.fValue);

	if (!m_bPopup)
	{
		for (size_t i = 0; i < ETOUI(TURN::END); ++i)
		{
			_string Name = _string("Animation : ") + MagicEnumToStringView(static_cast<TURN>(i)).data();
			if (ImGui::Button(Name.c_str()))
			{
				m_iTurnIdx = i;
				m_bPopup = true;
				break;
			}
		}
	}

	if (m_bPopup)
	{
		ImGui::Text("Select Animation : "); ImGui::SameLine(150.f);
		ImGui::Text(MagicEnumToStringView(static_cast<TURN>(m_iTurnIdx)).data());

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.f, 1.f, 1.f, 1.f));
		if (CGameInstance::Get().MouseDown(MOUSEKEYSTATE::RB))
			m_bPopup = false;
		int32_t iIndex = CGameInstance::Get().GetAnimIndex(m_Handle);

		if (-1 != iIndex)
		{
			m_bPopup = false;
			m_iTurnAnimIndex[m_iTurnIdx] = iIndex;
			m_iTurnIdx = -1;
		}

		ImGui::PopStyleColor();
	}
}
nlohmann::json CBTTurnAnimation::Save_Node()
{
	nlohmann::json j = __super::Save_Node();

	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		SaveJsonValue(j, Name,m_iTurnAnimIndex[i]);
	}

	return j;
}
HRESULT CBTTurnAnimation::Load_json(const nlohmann::json& j)
{
	__super::Load_json(j);
	for (uint32_t i = 0; i < ETOUI(TURN::END); ++i)
	{
		_string Name = "AnimIndex" + std::to_string(i);
		LoadJsonValue(j, Name, m_iTurnAnimIndex[i]);
	}

	return S_OK;
}
void CBTTurnAnimation::Abort()
{
	__super::Abort();
	m_fTick = 0.f;
	m_Value.iAnimIndex = -1;
	m_bTurn = false;
}
_bool CBTTurnAnimation::SelectAngle( _float fAngle)
{
	if (fAngle > 157.f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_180)];
		m_fAngle = 180.f;
	}
	else if (fAngle > 112.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_135)];
		m_fAngle = 135.f;
	}
	else if (fAngle > 67.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_90)];
		m_fAngle = 90.f;
	}
	else if (fAngle > 22.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::RIGHT_45)];
		m_fAngle = 45.f;

	}
	else if (fAngle < -157.f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_180)];
		m_fAngle = -180.f;
	}
	else if (fAngle < -112.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_135)];
		m_fAngle = -135.f;
	}
	else if (fAngle < -67.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_90)];
		m_fAngle = -90.f;
	}
	else if (fAngle < -22.5f)
	{
		m_Value.iAnimIndex = m_iTurnAnimIndex[ETOUI(TURN::LEFT_45)];
		m_fAngle = -45.f;
	}
	else
		return false;
	if (m_Value.iAnimIndex < 0)
		return false;
	return true;
}
void CBTTurnAnimation::Turn(_float fTimeDelta)
{
}
void CBTTurnAnimation::OnEnter()
{
	__super::OnEnter();

	m_fTick = 0.f;
	m_Value.iAnimIndex = -1;
	m_bTurn = false;
}
void CBTTurnAnimation::OnExit(EVALUATE eResult)
{
	__super::OnExit(eResult);
}
E::UPtr<CBTTurnAnimation> CBTTurnAnimation::Create()
{
	auto pInstance = E::ToUPtr(new CBTTurnAnimation{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CBTTurnAnimation");
		return nullptr;
	}
	return  pInstance;
}
E::UPtr<E::CPrototype> CBTTurnAnimation::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CBTTurnAnimation{ *this });
	if (FAILED(pInstance->Initalize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBTTurnAnimation");
		return nullptr;
	}

	return pInstance;
}
