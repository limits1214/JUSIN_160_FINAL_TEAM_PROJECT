#include "pch.h"
#include "Player_AcientAttack_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "PlayerAnimationRatioGuard.h"
#include "Player_Weapon.h"
#include "Monster.h"
#include "ClientEvents.h"
#include "Trail_CPU.h"

NS_USING(Client)

void CPlayer_AcientAttack_State::Enter(CStateMachine* pStateMachine)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	if (!HasValidTarget(*pPlayer))
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	CacheAnimationIndices(*pPlayer);
	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	if (m_AcientCast_Animations[iSkillIndex] < 0 ||
		m_AcientEnd_Animations[iSkillIndex] < 0)
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

	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::ACIENT_LIGHTNING);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;

	// 고대마법 발동 이벤트 발행
	{
		auto a = CGameInstance::Get().GetParticle("Lightning_Trail", "Lightning_Trail");
		static_cast<CTrail_CPU*>(a)->SetColor(_float4(67 / 255.f, 97 / 255.f, 174 / 255.f, 1.f));
		static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(51 / 255.f, 77 / 255.f, 126 / 255.f, 4.f));
	}
	CGameInstance::Get().EventPublish<FAcientMagicStart>();
}

void CPlayer_AcientAttack_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	const auto iSkillIndex = ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING);
	m_AcientCast_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_Start_anm.bin");
	//m_AcientEnd_Animations[iSkillIndex] = FindAnimationIndex(
	//	player,
	//	"AN_ProfessorSharp_MasterRig_DW_Cmbt_Atk_AOE_Lightning_Cast_End_anm.bin");
	//m_AcientCast_Animations[iSkillIndex] = FindAnimationIndex(
	//	player,
	//	"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");
	m_AcientEnd_Animations[iSkillIndex] = FindAnimationIndex(
		player,
		"AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Slam_Dwn_anm.bin");

	m_bAnimationIndicesCached =
		m_AcientCast_Animations[iSkillIndex] >= 0 &&
		m_AcientEnd_Animations[iSkillIndex] >= 0;
}

void CPlayer_AcientAttack_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
{
	auto* pPlayer = GetPlayer(pStateMachine);
	if (!pPlayer)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	pPlayer->SetCurrentMoveSpeed(0.f);

	auto* pAnimator = pPlayer->GetAnimator();
	if (!pAnimator)
	{
		RequestLocomotion(pStateMachine);
		return;
	}

	m_fAnimRatio =PlayerAnimationRatioGuard::Sanitize(	pAnimator->GetPlayAnimRatio());

	switch (m_ePhase)
	{
	case PHASE::CAST:
		m_ePhase = PHASE::ATTACK;
		pAnimator->Play_Anim(
			m_AcientCast_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)],
			false,
			0.24f);

		CGameInstance::Get().PlayEffect("LightningSound", *pPlayer->GetTransform().GetWorldMatrix());
		// 스킬 컷신 재생
		{
			FCinematicPlayOptions options{};
			options.eStartMode = ECinematicStartMode::Blend;
			options.fStartBlendDuration = 1.f;
			options.eReturnMode = ECinematicReturnMode::Blend;
			options.fReturnBlendDuration = 1.f;
			CGameInstance::Get().PlayCinematic("Lightning", pPlayer->GetHandle(), options);
		}
		break;

	case PHASE::ATTACK:
		m_fAcientElapsed += std::max(0.f, fTimeDelta);



		if (!m_bOnceLighting) {
			{
				m_fSpawnDelay += fTimeDelta;
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;

				const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
				_float3 vstart, vend;
				vstart = _float3(spawnWorld._41, spawnWorld._42 + 0.2f, spawnWorld._43);
				vend = _float3(spawnWorld._41, spawnWorld._42 - 0.2f, spawnWorld._43);
				CGameInstance::Get().AddTrailPoint("Lightning_Trail", "Lightning_Trail", vstart, vend);


				if (m_fSpawnDelay > 0.03f) {
					CGameInstance::Get().PlayEffect("Lightning_Trail_Particle", pWeapon->GetSpawnWorldMatrix());
					m_fSpawnDelay = 0.f;
				}
			}
		}
		if (m_fAcientElapsed >= ACIENT_LIGHTENING_ATTACK_DURATION)
		{
			if (!m_bOnceLighting) {
				auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

				if (!pWeapon)
					return;
				CGameInstance::Get().PlayEffect("Lightning_Wand", pWeapon->GetSpawnWorldMatrix());

				m_bOnceLighting = true;
			}
			
			pAnimator->GetCurAnimState().fSpeed = 0.2f;
			if (m_fAcientElapsed >= ACIENT_LIGHTENING_ATTACK_STOP_DURATION) {
				m_ePhase = PHASE::RECOVERY;

				auto* Target = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle());

				if (!Target)
					return;
				CGameInstance::Get().PlayEffect("Player_Lightning", *Target->GetTransform().GetWorldMatrix());
				pAnimator->Play_Anim(m_AcientEnd_Animations[ETOUI(ACIENT_SKILL::ACIENT_LIGHTENING)], false, 0.25f);
				pAnimator->GetCurAnimState().fSpeed = 1.f;

			}
		
		}
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO) {
			//내리고 있는ㅇㅋ
	
			RequestLocomotion(pStateMachine);
		}
		
		if (m_fAnimRatio >= ACIENT_LIGHTENING_LAST_ATTACK && !m_bOnceLastLighting) {
			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
				pMonster->Check_Table(PLAYER_SKILL_TYPE::ACIENT_LIGHTNING);
			m_bOnceLastLighting = true;
		
			//CGameInstance::Get().EventPublish(FRequestPlayerCameraShake
			//	{
			//	   1.f, // 강도 0 ~ 1
			//	   1.f, // 지속시간
			//	   15.f, // 초당 진동횟수
			//	});
		}
		
		break;
	}
}

void CPlayer_AcientAttack_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);
	m_bOnceLighting = false;
	m_bOnceLastLighting = false;
	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fAcientElapsed = 0.f;
}

SPtr<CPlayer_AcientAttack_State> CPlayer_AcientAttack_State::Create()
{
	return ToSPtr(new CPlayer_AcientAttack_State{});
}
