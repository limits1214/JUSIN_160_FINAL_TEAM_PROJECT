#include "pch.h"
#include "Player_Attack_State.h"

#include "Player.h"
#include "Player_StateMachine.h"
#include "ComAnimator.h"
#include "ComCharacterMotor.h"
#include "ComCharacterMoveIntent.h"
#include "ComModelInstance.h"
#include "CameraObject.h"
#include "PlayerAnimationRatioGuard.h"
#include "ResModel.h"
#include "ResModelAnim.h"

NS_USING(Client)


void CPlayer_Attack_State::Enter(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;



	CacheAnimationIndices(*player);

	m_iCurrentForwardLightAnimation = static_cast<size_t>(Engine::RandInt(0,(int32_t)(m_ForwardLightAnimations.size()) - 1));
	m_iComboCount = 1;
	m_bAttackQueued = false;
	m_bPlayingHeavy = false;
	m_bMagicBulletFired = false;
	m_fPreviousAnimRatio = 0.f;

	auto* animator = player->GetAnimator();
	if (!animator)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	player->SetCurrentMoveSpeed(0.f);
	player->SetMovementLocked(true);
	player->SetRootMotionTranslationActive(true);
	player->SetRootMotionRotationActive(false);
	if (auto* moveIntent = player->GetMoveIntent())
	{
		moveIntent->ClearMoveIntent();
		moveIntent->ClearFacingIntent();
	}

	if (!PlayDirectionalAttack(*player, false))
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);

	pTarget = CGameInstance::Get().GetGameObjectByHandle(player->GetTargetHandle());
	
	player->SetPlayerCurSKill(PLAYER_SKILL_TYPE::ATTACK);
}

void CPlayer_Attack_State::Exit(CStateMachine* pStateMachine)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	if (!player)
		return;

	player->SetMovementLocked(false);
	player->SetRootMotionTranslationActive(false);
	player->SetRootMotionRotationActive(false);
	m_iComboCount = 0;
	m_bAttackQueued = false;
	m_bPlayingHeavy = false;
	m_bMagicBulletFired = false;
	m_fPreviousAnimRatio = 0.f;
}

void CPlayer_Attack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* player = pStateMachine ? pStateMachine->GetOwner<CPlayer>() : nullptr;
	auto* playerStateMachine = Cast<CPlayer_StateMachine>(pStateMachine);
	if (!player || !playerStateMachine)
		return;

	auto* animator = player->GetAnimator();
	if (!animator)
	{
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	




	const _float fPreviousAnimRatio = m_fPreviousAnimRatio;
	const _float fAnimRatio =
		PlayerAnimationRatioGuard::Sanitize(
			animator->GetPlayAnimRatio());

	const _float fMagicBulletFireRatio =
		m_bPlayingHeavy
		? HEAVY_MAGIC_BULLET_FIRE_RATIO
		: LIGHT_MAGIC_BULLET_FIRE_RATIO;

	if (!m_bMagicBulletFired &&
		fPreviousAnimRatio < fMagicBulletFireRatio &&
		fAnimRatio >= fMagicBulletFireRatio)
	{
		player->Attack_Magic_Bullet();
		m_bMagicBulletFired = true;
	}


	if (fAnimRatio >= MOVE_CANCEL_START_RATIO &&player->HasRawMoveInput()) {
		m_bAttackQueued = false;
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}


	const _bool bInComboInputWindow =
		PlayerAnimationRatioGuard::Intersects(
			fPreviousAnimRatio,
			fAnimRatio,
			COMBO_INPUT_START_RATIO,
			COMBO_INPUT_END_RATIO);

	if (bInComboInputWindow &&CGameInstance::Get().MouseDown(MOUSEKEYSTATE::LB))
	{
		m_bAttackQueued = true;
	}

	m_fPreviousAnimRatio = fAnimRatio;
	 
	if (m_bAttackQueued && fAnimRatio >= COMBO_LINK_RATIO)
	{
		m_bAttackQueued = false;
		++m_iComboCount;

		if (m_iComboCount == 3)
		{
			m_iComboCount = 0;
			if (!PlayDirectionalAttack(*player, true))
				playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
			return;
		}

		m_iCurrentForwardLightAnimation =
			(m_iCurrentForwardLightAnimation + 1) %
			m_ForwardLightAnimations.size();

		if (!PlayDirectionalAttack(*player, false))
			playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
		return;
	}

	if (animator->GetFinish())
		playerStateMachine->RequestState(PLAYER_STATE::LOCOMOTION);
}

CPlayer_Attack_State::ATTACK_DIRECTION
CPlayer_Attack_State::ResolveAttackDirection(const CPlayer& player) const
{
	auto* camera =
		CGameInstance::Get().GetActiveCamera("PlayerCamera");

	if (!camera)
		return ATTACK_DIRECTION::FWD;

	_vector vPlayerLook =
		XMVectorSetY(player.GetTransform().GetState(STATE::LOOK), 0.f);
	_vector vPlayerRight =
		XMVectorSetY(player.GetTransform().GetState(STATE::RIGHT), 0.f);

	_vector vAttackDirection =
		XMVectorSetY(camera->GetTransform().GetState(STATE::LOOK), 0.f);

	auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
		player.GetTargetHandle());

	if (pTarget && !pTarget->GetPendingDestroy())
	{
		const _vector vPlayerPosition =
			player.GetTransform().GetState(STATE::POSITION);

		const _vector vTargetPosition =
			pTarget->GetTransform().GetState(STATE::POSITION);

		vAttackDirection =
			XMVectorSetY(vTargetPosition - vPlayerPosition, 0.f);
	}
	
	else
	{
		// 타겟이 없거나 제거됐으면 카메라 방향
		vAttackDirection = XMVectorSetY(camera->GetTransform().GetState(STATE::LOOK),0.f);
	}

	constexpr _float EPSILON =std::numeric_limits<_float>::epsilon();
	if (XMVectorGetX(XMVector3LengthSq(vPlayerLook)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vPlayerRight)) <= EPSILON ||
		XMVectorGetX(XMVector3LengthSq(vAttackDirection)) <= EPSILON)
	{
		return ATTACK_DIRECTION::FWD;
	}

	vPlayerLook = XMVector3Normalize(vPlayerLook);
	vPlayerRight = XMVector3Normalize(vPlayerRight);
	vAttackDirection = XMVector3Normalize(vAttackDirection);

	const _float fForward = XMVectorGetX(XMVector3Dot(vPlayerLook, vAttackDirection));
	const _float fRight = XMVectorGetX(XMVector3Dot(vPlayerRight, vAttackDirection));
	const _float fSignedAngle = XMConvertToDegrees(std::atan2(fRight, fForward));
	const _float fAbsAngle = std::abs(fSignedAngle);
	const _bool bRight = fSignedAngle >= 0.f;

	if (fAbsAngle < 22.5f)
		return ATTACK_DIRECTION::FWD;
	if (fAbsAngle < 67.5f)
		return bRight
			? ATTACK_DIRECTION::RHT_45
			: ATTACK_DIRECTION::LFT_45;
	if (fAbsAngle < 112.5f)
		return bRight
			? ATTACK_DIRECTION::RHT_90
			: ATTACK_DIRECTION::LFT_90;
	if (fAbsAngle < 157.5f)
		return bRight
			? ATTACK_DIRECTION::RHT_135
			: ATTACK_DIRECTION::LFT_135;

	return bRight
		? ATTACK_DIRECTION::RHT_180
		: ATTACK_DIRECTION::LFT_180;
}

int32_t CPlayer_Attack_State::GetAttackAnimation(ATTACK_DIRECTION eDirection,_bool bHeavy) const
{
	if (eDirection == ATTACK_DIRECTION::FWD)
	{
		if (bHeavy)
			return m_ForwardHvyAnimations.front();

		return m_ForwardLightAnimations[
			m_iCurrentForwardLightAnimation];
	}

	const size_t iDirection = (size_t)(eDirection);
	if (iDirection >= ATTACK_DIRECTION_COUNT)
		return -1;

	return bHeavy
		? m_DirectionalHeavyAnimations[iDirection]
		: m_DirectionalLightAnimations[iDirection];
}

_bool CPlayer_Attack_State::PlayDirectionalAttack(CPlayer& player,_bool bHeavy)
{
	auto* animator = player.GetAnimator();
	if (!animator)
		return false;

	ATTACK_DIRECTION eDirection = ResolveAttackDirection(player);
	int32_t iAnimation = GetAttackAnimation(eDirection, bHeavy);

	if (iAnimation < 0)
	{
		eDirection = ATTACK_DIRECTION::FWD;
		iAnimation = bHeavy ? m_ForwardHvyAnimations.front() : m_ForwardLightAnimations[m_iCurrentForwardLightAnimation];
	}

	if (iAnimation < 0)
		return false;

	m_bPlayingHeavy = bHeavy;
	m_bMagicBulletFired = false;
	m_fPreviousAnimRatio = 0.f;
	// 왼쪽으로 90도 도는 애만 이상함 RootMotion이
	player.SetRootMotionTranslationActive(eDirection != ATTACK_DIRECTION::LFT_90);
	player.SetRootMotionRotationActive(bHeavy || eDirection != ATTACK_DIRECTION::FWD);
	animator->Play_Anim(iAnimation,false,ATTACK_BLEND_DURATION);
	animator->GetCurAnimState().fSpeed = 1.3f;
	return true;
}

void CPlayer_Attack_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	m_ForwardLightAnimations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_01_anm.bin");
	m_ForwardLightAnimations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_02_anm.bin");
	m_ForwardLightAnimations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_03_Uppercut_anm.bin");
	m_ForwardLightAnimations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_04_anm.bin");
	m_ForwardLightAnimations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_05_anm.bin");
	m_ForwardLightAnimations[5] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_06_anm.bin");
	m_ForwardLightAnimations[6] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_07_anm.bin");
	m_ForwardLightAnimations[7] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_08_anm.bin");
	m_ForwardLightAnimations[8] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_09_anm.bin");
	m_ForwardLightAnimations[9] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_StepBwd_01_anm.bin");
	m_ForwardLightAnimations[10] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_StepBwd_02_anm.bin");
	m_ForwardLightAnimations[11] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_StepBwd_03_anm.bin");
	m_ForwardLightAnimations[12] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_StepBwd_04_anm.bin");

	m_ForwardHvyAnimations[0] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_01_Spin_anm.bin");
	m_ForwardHvyAnimations[1] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_02_anm.bin");
	m_ForwardHvyAnimations[2] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_03_anm.bin");
	m_ForwardHvyAnimations[3] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_04_anm.bin");
	m_ForwardHvyAnimations[4] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_05_anm.bin");
	m_ForwardHvyAnimations[5] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_06_anm.bin");
	m_ForwardHvyAnimations[6] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_08_anm.bin");
	m_ForwardHvyAnimations[7] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_09_anm.bin");
	m_ForwardHvyAnimations[8] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_frmLft_anm.bin");
	m_ForwardHvyAnimations[9] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Hvy_frmRht_anm.bin");

	m_DirectionalLightAnimations.fill(-1);
	m_DirectionalHeavyAnimations.fill(-1);

	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::FWD)] =m_ForwardLightAnimations.front();
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_45_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_90_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_135_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_180_Lht_Spin_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_45_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_90_Lht_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_135_Lht_frmLft_anm.bin");
	m_DirectionalLightAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_180_Lht_Spin_anm.bin");

	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::FWD)] = m_ForwardHvyAnimations.front();
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_45_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_90_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_135_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::LFT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Lft_180_Hvy_SpinRht_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_45)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_45_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_90)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_90_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_135)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_135_Hvy_anm.bin");
	m_DirectionalHeavyAnimations[static_cast<size_t>(ATTACK_DIRECTION::RHT_180)] = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Rht_180_Hvy_Spin_anm.bin");

	m_bAnimationIndicesCached = true;
}

int32_t CPlayer_Attack_State::FindAnimationIndex(const CPlayer& player,_string_view sAnimationName) const
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


SPtr<CPlayer_Attack_State> CPlayer_Attack_State::Create()
{
	return ToSPtr(new CPlayer_Attack_State{});
}
