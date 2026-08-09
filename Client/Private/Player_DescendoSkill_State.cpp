#include "pch.h"
#include "Player_DescendoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
#include "PlayerAnimationRatioGuard.h"
#include "Monster.h"
#include "Player_Weapon.h"
#include "Trail_CPU.h"
NS_USING(Client)

void CPlayer_DescendoSkill_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (auto* pSound = pPlayer->GetSound())
	{
		pSound->PlaySlot2D(
			E::StringID{ "PLAYER_VOICE_DESCENDO" },
			"./Resources/SampleClient/Sound/Player/Spell/Descendo/Descendo_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 2.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}

	CacheAnimationIndices(*pPlayer);
	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::DESCENDO);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;

	{
		auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());
		if (!pWeapon)
			return;
		const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
		m_iEffectID = CGameInstance::Get().PlayEffect("DescendoStick", spawnWorld);
	}
	{
		auto a = CGameInstance::Get().GetParticle("Lightning_Trail", "Lightning_Trail");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(243 / 255.f, 37 / 255.f, 14 / 255.f, 1.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(243 / 255.f, 37 / 255.f, 14 / 255.f, 4.f));
	}

}

void CPlayer_DescendoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DescendoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");
	m_DescendoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_BM_RF_Cast_Casual_Fwd_Descendo_anm.bin");
	m_AttackFail_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_LF_Atk_Heavy_Fail_anm.bin");

	m_bAnimationIndicesCached =
		m_DescendoCast_Animation >= 0 &&
		m_DescendoEnd_Animation >= 0 &&
		m_AttackFail_Animation >= 0;
}

void CPlayer_DescendoSkill_State::Update(CStateMachine* pStateMachine, _float)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio = PlayerAnimationRatioGuard::Sanitize(pAnimator->GetPlayAnimRatio());
	pPlayer->SetCurrentMoveSpeed(0.f);

	switch (m_ePhase)
	{
	case PHASE::CAST:
	
		if (m_fAnimRatio >= CAST_START_RATIO)
		{
			m_ePhase = PHASE::ATTACK;
			if (!PlayRandomTargetAttack(*pPlayer))
				RequestLocomotion(pStateMachine);
		}
		break;

	case PHASE::ATTACK:
	{
		{
			auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());
			if (!pWeapon)
				return;
			const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
			CGameInstance::Get().SetEffectWorldMatrix(m_iEffectID, spawnWorld);
		}
		if (m_fAnimRatio >= CAST_END_RATIO)
		{
		/*	if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::DESCENDO))
			{
				m_ePhase = PHASE::ATTACK_FAILED;
				pAnimator->Play_Anim(m_AttackFail_Animation, false, 0.2f);
				break;
			}*/
			{
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());
				if (!pWeapon)
					return;
				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				CGameInstance::Get().PlayEffect("DescendoWips", spawnWorld);
			}
			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
				pMonster->Check_Table(PLAYER_SKILL_TYPE::DESCENDO);
			m_ePhase = PHASE::PUSH;
			pAnimator->Play_Anim(m_DescendoCast_Animation, false, 0.25f);
			pAnimator->GetCurAnimState().fSpeed = 1.f;
		}
		break;
	}

	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

	case PHASE::PUSH:
		{
			auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

			if (!pWeapon)
				return;

			const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
			_float3 vstart, vend;
			vstart = _float3(spawnWorld._41, spawnWorld._42 + 0.1f, spawnWorld._43);
			vend = _float3(spawnWorld._41, spawnWorld._42 - 0.1f, spawnWorld._43);
			CGameInstance::Get().AddTrailPoint("Lightning_Trail", "Lightning_Trail", vstart, vend);
		}
	
		if (m_fAnimRatio >= ATTACK_END_RATIO && m_fAnimRatio != 1.f)
		{
			m_ePhase = PHASE::RECOVERY;
			RequestLocomotion(pStateMachine);
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
			RequestLocomotion(pStateMachine);
		break;
	}
}

void CPlayer_DescendoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
}

SPtr<CPlayer_DescendoSkill_State> CPlayer_DescendoSkill_State::Create()
{
	return ToSPtr(new CPlayer_DescendoSkill_State{});
}
