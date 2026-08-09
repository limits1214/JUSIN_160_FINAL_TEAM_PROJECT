#include "pch.h"
#include "Player_Fly_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComCharacterMotor.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

CPlayer_Fly_State::CPlayer_Fly_State() = default;

void CPlayer_Fly_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	CacheAnimationIndices(*pPlayer);
	m_iActiveAnimation = -1;
	m_eFlightPhase = FLIGHT_PHASE::MOUNTING;
	pPlayer->SetMovementLocked(true);
	pPlayer->SetRootMotionTranslationActive(true);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMotor = pPlayer->GetCharacterMotor())
	{
		_float3 vVelocity = pMotor->GetVelocity();
		vVelocity.y = 0.f;
		pMotor->SetVelocity(vVelocity);
		pMotor->SetUseGravity(false);
	}

	const int32_t iMountAnimation = pPlayer->HasRawMoveInput() && m_iMountJogAnimation >= 0 ? m_iMountJogAnimation : m_iMountHoverAnimation;
	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		return;
	}

	if (iMountAnimation >= 0)
	{
		pAnimator->Play_Anim(iMountAnimation, false, 0.15f);
		m_iActiveAnimation = iMountAnimation;
	}
	else if (m_iHoverAnimation >= 0)
	{
		pPlayer->SetMovementLocked(false);
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		pAnimator->Play_Anim(m_iHoverAnimation, true, 0.15f); 
		m_iActiveAnimation = m_iHoverAnimation; 
	}
}

void CPlayer_Fly_State::Exit(CStateMachine* pStateMachine)
{
	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	pPlayer->SetMovementLocked(false);
	pPlayer->SetRootMotionTranslationActive(false);
	pPlayer->SetRootMotionRotationActive(false);
	if (auto* pMotor = pPlayer->GetCharacterMotor())
	{
		pMotor->SetUseGravity(true);
	}
}

void CPlayer_Fly_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{

	auto* pPlayer = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!pPlayer)
	{
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	auto* pMoveIntent = pPlayer->GetMoveIntent();
	if (!pAnimator || !pMoveIntent)
	{
		return;
	}

	if (!pPlayer->IsFlyRequested() && m_eFlightPhase != FLIGHT_PHASE::MOUNTING && m_eFlightPhase != FLIGHT_PHASE::DISMOUNTING)
	{
		m_eFlightPhase = FLIGHT_PHASE::DISMOUNTING;
		pPlayer->SetMovementLocked(true);
		if (m_iDismountAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iDismountAnimation, false, 0.15f);
			m_iActiveAnimation = m_iDismountAnimation;
		}
	}

	if (pPlayer->HasRawMoveInput())
	{
		pMoveIntent->SetFacingIntent(pPlayer->GetRawMoveDirection(), m_fFacingTurnSpeed);
	}
	else
	{
		pMoveIntent->ClearFacingIntent();
	}
	const _bool bWantsToFly = pPlayer->HasRawMoveInput() && pPlayer->GetCurrentMoveSpeed() > m_fHoverSpeedThreshold;

	switch (m_eFlightPhase)
	{
	case FLIGHT_PHASE::MOUNTING:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		pPlayer->SetMovementLocked(false);
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		if (m_iHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iHoverAnimation, true, 0.15f); 
			m_iActiveAnimation = m_iHoverAnimation; 
		}
		return;

	case FLIGHT_PHASE::HOVER:
		if (!bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::INTO_FLY;
		if (m_iIntoFlyAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iIntoFlyAnimation, false, 0.12f); 
			m_iActiveAnimation = m_iIntoFlyAnimation; 
			return; 
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::INTO_FLY:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::FLYING;
		break;

	case FLIGHT_PHASE::FLYING:
		if (bWantsToFly) 
		{
			break;
		}
		m_eFlightPhase = FLIGHT_PHASE::INTO_HOVER;
		if (m_iIntoHoverAnimation >= 0)
		{
			pAnimator->Play_Anim(m_iIntoHoverAnimation, false, 0.12f); 
			m_iActiveAnimation = m_iIntoHoverAnimation; 
			return; 
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::INTO_HOVER:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		m_eFlightPhase = FLIGHT_PHASE::HOVER;
		break;

	case FLIGHT_PHASE::DISMOUNTING:
		if (!pAnimator->GetFinish()) 
		{
			return;
		}
		if (auto* pMotor = pPlayer->GetCharacterMotor())
		{
			pMotor->SetUseGravity(true);
		}
		if (auto* pPlayerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine)) 
		{
			pPlayerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		}
		return;
	}

	const int32_t iDesiredAnimation = m_eFlightPhase == FLIGHT_PHASE::HOVER ? m_iHoverAnimation : ResolveAnimation(*pPlayer);
	if (iDesiredAnimation < 0 || iDesiredAnimation == m_iActiveAnimation) 
	{
		return;
	}
	pAnimator->Play_Anim(iDesiredAnimation, true, 0.2f);
	m_iActiveAnimation = iDesiredAnimation;
}

void CPlayer_Fly_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached) 
	{
		return;
	}

	m_iHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Hover_Idle_anm.bin");
	m_iMountHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_Mount_Hover_anm.bin");
	m_iMountJogAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_Mount_Fly_fJog_anm.bin");
	m_iDismountAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_NoStirrups_Dismount_anm.bin");
	m_iIntoFlyAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Hover_IntoFly_anm.bin");
	m_iIntoHoverAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fly_IntoHover_anm.bin");
	m_iForwardAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Idle_Fwd_anm.bin");
	m_iSlowUpAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Up_anm.bin");
	m_iSlowDownAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Dn_anm.bin");
	m_iSlowLeftAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Lft_anm.bin");
	m_iSlowRightAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Slow_Rht_anm.bin");
	m_iFastUpAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Up_anm.bin");
	m_iFastDownAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Dn_anm.bin");
	m_iFastLeftAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Lft_anm.bin");
	m_iFastRightAnimation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Broom_FlyNoStirrups_Fast_Rht_anm.bin");
	m_bAnimationIndicesCached = true;
}

int32_t CPlayer_Fly_State::ResolveAnimation(const CPlayer& player) const
{
	const _float fSpeed = player.GetCurrentMoveSpeed();
	if (!player.HasRawMoveInput() || fSpeed <= m_fHoverSpeedThreshold) 
	{
		return m_iHoverAnimation;
	}

	const _float3& vInput = player.GetRawMoveDirection();
	const _bool bFast = player.IsSprintRequested() || fSpeed >= m_fFastSpeedThreshold;
	if (vInput.y >= m_fVerticalInputThreshold) 
	{
		return bFast ? m_iFastUpAnimation : m_iSlowUpAnimation;
	}
	if (vInput.y <= -m_fVerticalInputThreshold) 
	{
		return bFast ? m_iFastDownAnimation : m_iSlowDownAnimation;
	}

	const _float fAngle = CalculateSignedAngle(player, vInput);
	if (fAngle >= m_fTurnAngleThreshold) 
	{
		return bFast ? m_iFastRightAnimation : m_iSlowRightAnimation;
	}
	if (fAngle <= -m_fTurnAngleThreshold) 
	{
		return bFast ? m_iFastLeftAnimation : m_iSlowLeftAnimation;
	}
	return m_iForwardAnimation;
}

_float CPlayer_Fly_State::CalculateSignedAngle(const CPlayer& player, const _float3& vMoveDirection) const
{
	_vector vLook = XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vRight = XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);
	_vector vMove = XMVectorSetY(XMLoadFloat3(&vMoveDirection), 0.f);
	if (XMVectorGetX(XMVector3LengthSq(vMove)) <= std::numeric_limits<_float>::epsilon())
	{
		return 0.f;
	}

	vLook = XMVector3Normalize(vLook);
	vRight = XMVector3Normalize(vRight);
	vMove = XMVector3Normalize(vMove);
	const _float fForward = XMVectorGetX(XMVector3Dot(vMove, vLook));
	const _float fRight = XMVectorGetX(XMVector3Dot(vMove, vRight));
	return XMConvertToDegrees(std::atan2(fRight, fForward));
}

int32_t CPlayer_Fly_State::FindAnimationIndex(const CPlayer& player, const _string_view& sAnimationName) const
{
	auto* pModelInstance = player.GetModelInstance();
	if (!pModelInstance || !pModelInstance->GetModel()) 
	{
		return -1;
	}

	const auto& animations = pModelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
		{
			return static_cast<int32_t>(i);
		}
	}
	return -1;
}

SPtr<CPlayer_Fly_State> CPlayer_Fly_State::Create() { 
	return ToSPtr(new CPlayer_Fly_State{}); 
}
