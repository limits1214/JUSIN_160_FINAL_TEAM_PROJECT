#include "pch.h"
#include "Player_Roll_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "PlayerAnimationRatioGuard.h"
#include "ComAnimator.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)

void CPlayer_Roll_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	auto* animator = player->GetAnimator();
	auto* moveIntent = player->GetMoveIntent();
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!playerStateMachine)
		return;


	_vector vRollDirection = player->HasRawMoveInput()? XMLoadFloat3(&player->GetRawMoveDirection()) : XMVectorSetY(player->GetTransform().GetState(STATE::LOOK), 0.f);

	if (XMVectorGetX(XMVector3LengthSq(vRollDirection)) <=std::numeric_limits<_float>::epsilon())
	{
		vRollDirection = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	}

	XMStoreFloat3(&m_vRollDirection,XMVector3Normalize(vRollDirection));

	player->SetMovementLocked(true);
	player->SetRootMotionRotationActive(false);
	player->SetRootMotionTranslationActive(false);

	if (moveIntent)
	{
		moveIntent->ClearMoveIntent();
		moveIntent->SetFacingIntentImmediate(m_vRollDirection);
	}

	m_iRollAnimation = FindAnimationIndex(*player,"AN_ProfessorSharp_MasterRig_Hu_Cmbt_DdgeRll_Fwd_anm.bin");

	if (!animator || m_iRollAnimation < 0)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}
	player->SetInvincible(true);
	animator->Play_Anim(m_iRollAnimation, false, 0.1f);
	m_fPreviousAnimRatio = 0.f;
}

void CPlayer_Roll_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetRootMotionTranslationActive(false);
	player->SetRootMotionRotationActive(false);
	player->SetMovementLocked(false);
	player->SetInvincible(false);
	if (auto* moveIntent = player->GetMoveIntent())
		moveIntent->ClearMoveIntent();

	m_fPreviousAnimRatio = 0.f;
}

void CPlayer_Roll_State::Update(CStateMachine* pStateMachine,_float fTimeDelta)
{
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!playerStateMachine)
		return;

	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	auto* animator = player->GetAnimator();
	if (!animator)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	const _float fAnimationRatio = PlayerAnimationRatioGuard::Sanitize(animator->GetPlayAnimRatio());

	if (player->HasRawMoveInput())
	{
		const _vector vCurrentDirection = XMVector3Normalize(
			XMVectorSetY(
				XMLoadFloat3(&m_vRollDirection),
				0.f));
		const _vector vTargetDirection = XMVector3Normalize(
			XMVectorSetY(
				XMLoadFloat3(&player->GetRawMoveDirection()),
				0.f));
		const _float fDot = std::clamp(
			XMVectorGetX(XMVector3Dot(
				vCurrentDirection,
				vTargetDirection)),
			-1.f,
			1.f);
		const _float fCrossY = XMVectorGetY(XMVector3Cross(
			vCurrentDirection,
			vTargetDirection));
		_float fSignedAngle = std::atan2(fCrossY, fDot);

		// 정반대 입력은 외적이 0이므로 회전 방향을 선택해
		// 일반 벡터 Lerp에서 발생하는 순간 반전을 피한다.
		if (std::abs(fCrossY) <= std::numeric_limits<_float>::epsilon() &&
			fDot < 0.f)
		{
			fSignedAngle = XM_PI;
		}

		const _float fDirectionBlend = std::clamp(
			1.f - std::exp(-m_fRollDirectionResponse * fTimeDelta),
			0.f,
			1.f);
		const _vector vSteeredDirection = XMVector3Normalize(
			XMVector3TransformNormal(
				vCurrentDirection,
				XMMatrixRotationY(fSignedAngle * fDirectionBlend)));
		XMStoreFloat3(&m_vRollDirection, vSteeredDirection);

		if (auto* moveIntent = player->GetMoveIntent())
		{
			moveIntent->SetFacingIntent(
				m_vRollDirection,
				360.f);
		}
	}

	const _float fMoveRatioEnd = std::min(fAnimationRatio, m_fRollMoveEndRatio);
	const _float fMoveTime = PlayerAnimationRatioGuard::CalculateActiveDeltaTime(m_fPreviousAnimRatio, fAnimationRatio, 0.f,m_fRollMoveEndRatio, fTimeDelta);

	if (fMoveTime > 0.f)
	{
		const _float fSampleRatio =
			(m_fPreviousAnimRatio + fMoveRatioEnd) * 0.5f;
		_float fSpeedScale{ 1.f };
		if (fSampleRatio > m_fRollStopStartRatio)
		{
			const _float fStopRatio = std::clamp(
				(fSampleRatio - m_fRollStopStartRatio) /
				(m_fRollMoveEndRatio - m_fRollStopStartRatio),
				0.f,
				1.f);
			const _float fSmoothStop =fStopRatio * fStopRatio * (3.f - 2.f * fStopRatio);
			fSpeedScale = 1.f - fSmoothStop;
		}

		player->ApplyDirectionalMovement(
			m_vRollDirection,
			m_fRollSpeed * fSpeedScale,
			fMoveTime);
	}
	m_fPreviousAnimRatio = fAnimationRatio;

	// 지정 비율 이후 이동 입력이 있으면 Roll을 취소하고 Locomotion으로 복귀한다.
	if (fAnimationRatio >= m_fLocomotionCancelRatio &&player->HasRawMoveInput())
	{
		player->PrepareLocomotionResume();
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	if (animator->GetFinish())
	{
		player->PrepareLocomotionResume();
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
	}
}

int32_t CPlayer_Roll_State::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
{
	auto* modelInstance = player.GetModelInstance();
	if (!modelInstance || !modelInstance->GetModel())
		return -1;

	const auto& animations = modelInstance->GetModel()->GetAnimations();
	for (size_t i = 0; i < animations.size(); ++i)
	{
		if (animations[i] && animations[i]->GetAnimName() == sAnimationName)
			return static_cast<int32_t>(i);
	}

	return -1;
}

SPtr<CPlayer_Roll_State> CPlayer_Roll_State::Create()
{
	return ToSPtr(new CPlayer_Roll_State{});
}
