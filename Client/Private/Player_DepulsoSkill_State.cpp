#include "pch.h"
#include "Player_DepulsoSkill_State.h"

#include "Player.h"
#include "ComAnimator.h"
#include "ComSound.h"
#include "PlayerAnimationRatioGuard.h"
#include "Monster.h"
#include "Player_Weapon.h"
#include "Trail_CPU.h"
NS_USING(Client)

void CPlayer_DepulsoSkill_State::Enter(CStateMachine* pStateMachine)
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
			E::StringID{ "PLAYER_VOICE_DEPULSO" },
			"./Resources/SampleClient/Sound/Player/Spell/Depulso/Depulso_Man.wav",
			SOUND_PLAY_DESC{
				.sBusID = SOUND_BUS::VOICE,
				.fVolume = 2.f,
				.fPitch = 1.f,
				.iPriority = 64,
				.bLoop = false
			});
	}

	CacheAnimationIndices(*pPlayer);
	// Depulso 이동은 애니메이션 Root Motion이 아니라 아래의 조절 가능한
	// 전방 이동 구간을 사용한다.
	SetSkillControl(*pPlayer, true, true, false);
	pPlayer->SetCurrentMoveSpeed(0.f);
	pPlayer->SetPlayerCurSKill(PLAYER_SKILL_TYPE::DEPULSO);
	if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
	{
		pMonster->Check_Table(PLAYER_SKILL_TYPE::DEPULSO);
	
	}

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fPreviousAnimRatio = 0.f;

	{
		{
			auto a = CGameInstance::Get().GetParticle("Lightning_Trail", "Lightning_Trail");
			static_cast<CTrail_CPU*>(a)->SetColor(_float4(67 / 255.f, 97 / 255.f, 174 / 255.f, 1.f));
			static_cast<CTrail_CPU*>(a)->SetEmissive(_float4(51 / 255.f, 77 / 255.f, 126 / 255.f, 4.f));
		}
	}
}

void CPlayer_DepulsoSkill_State::CacheAnimationIndices(const CPlayer& player)
{
	if (m_bAnimationIndicesCached)
		return;

	// 고쳐야 할거 
	m_DepulsoCast_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Cast_Fwd_Lht_01_anm.bin");
	m_DepulsoEnd_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_Atk_Charge_Depulso_anm.bin");
	m_AttackFail_Animation = FindAnimationIndex(player, "AN_ProfessorSharp_MasterRig_Hu_Cmbt_LF_Atk_Heavy_Fail_anm.bin");

	m_bAnimationIndicesCached =
		m_DepulsoCast_Animation >= 0 &&
		m_DepulsoEnd_Animation >= 0 &&
		m_AttackFail_Animation >= 0;
}

void CPlayer_DepulsoSkill_State::Update(CStateMachine* pStateMachine, _float fTimeDelta)
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
			m_fPreviousAnimRatio = 0.f;
			if (!PlayRandomTargetAttack(*pPlayer))
				RequestLocomotion(pStateMachine);
			m_fAnimRatio = 0.f;
	
		}
		break;

	case PHASE::ATTACK:
	{
		const _float fMoveTime =
			PlayerAnimationRatioGuard::CalculateActiveDeltaTime(
				m_fPreviousAnimRatio,
				m_fAnimRatio,
				ATTACK_MOVE_START_RATIO,
				ATTACK_MOVE_END_RATIO,
				fTimeDelta);
		auto* pWeapon = CGameInstance::Get().GetGameObjectByHandleT<CPlayer_Weapon>(pPlayer->GetWeaponHandle());

		if (!pWeapon)
			return;

		const _float4x4 spawnWorld = pWeapon->GetSpawnWorldMatrix();
		_float3 vstart, vend;
		vstart = _float3(spawnWorld._41, spawnWorld._42 + 0.2f, spawnWorld._43);
		vend = _float3(spawnWorld._41, spawnWorld._42 - 0.2f, spawnWorld._43);
		CGameInstance::Get().AddTrailPoint("Lightning_Trail", "Lightning_Trail", vstart, vend);

		if (fMoveTime > 0.f)
		{
			if (auto* pTarget = CGameInstance::Get().GetGameObjectByHandle(
				pPlayer->GetTargetHandle()))
			{
				_float3 vTargetDirection{};
				XMStoreFloat3(
					&vTargetDirection,
					pTarget->GetTransform().GetState(STATE::POSITION) -
					pPlayer->GetTransform().GetState(STATE::POSITION));
				pPlayer->ApplyDirectionalMovement(
					vTargetDirection,
					ATTACK_MOVE_SPEED,
					fMoveTime);
			}
		}

		if (m_fAnimRatio >= CAST_END_RATIO) {
	/*		if (!TryApplySkillToTarget(*pPlayer, PLAYER_SKILL_TYPE::DEPULSO))
			{
				m_ePhase = PHASE::ATTACK_FAILED;
				pAnimator->Play_Anim(m_AttackFail_Animation, false, 0.2f);
				break;
			}*/

			// 밀기 시작

			if (auto pMonster = CGameInstance::Get().GetGameObjectByHandleT<CMonster>(pPlayer->GetTargetHandle()))
			{
				_vector monsterPos = XMVectorSet(pMonster->GetHurtBoxPosition().x, pMonster->GetHurtBoxPosition().y, pMonster->GetHurtBoxPosition().z, 1);
				_float3 camPos = CGameInstance::Get().GetActiveCamera()->GetTransform().GetPosition();
				_vector vCamPos = XMVectorSet(camPos.x, camPos.y, camPos.z, 1);

				_vector dirToCam = vCamPos - monsterPos;
				dirToCam = XMVectorSetY(dirToCam, 0.f);
				dirToCam = XMVector3Normalize(dirToCam);

				float spawnOffset = 5.5f;
				_vector spawnPos = monsterPos + dirToCam * spawnOffset;

				float yaw = atan2f(XMVectorGetX(dirToCam), XMVectorGetZ(dirToCam));
				_matrix effectMatrix = XMMatrixRotationY(yaw);
				effectMatrix.r[3] = XMVectorSetW(spawnPos, 1.f);

				_float4x4 storedMatrix;
				XMStoreFloat4x4(&storedMatrix, effectMatrix);

				CGameInstance::Get().PlayEffect("Depulso", storedMatrix);
			}
			
			m_ePhase = PHASE::PUSH;
			pAnimator->Play_Anim(m_DepulsoCast_Animation, false, 0.2f);
		}

		break;
	}

	case PHASE::ATTACK_FAILED:
		if (pAnimator->GetFinish())
			RequestLocomotion(pStateMachine);
		break;

	case PHASE::PUSH:
		if (m_fAnimRatio >= ATTACK_END_RATIO && m_fAnimRatio != 1.f) {
			m_ePhase = PHASE::RECOVERY;
			RequestLocomotion(pStateMachine);
		}
	
		break;

	case PHASE::RECOVERY:
		if (m_fAnimRatio >= RECOVERY_EXIT_RATIO)
			RequestLocomotion(pStateMachine);
		break;
	}

	m_fPreviousAnimRatio = m_fAnimRatio;
}

void CPlayer_DepulsoSkill_State::Exit(CStateMachine* pStateMachine)
{
	if (auto* pPlayer = GetPlayer(pStateMachine))
		ResetSkillControl(*pPlayer);

	m_ePhase = PHASE::CAST;
	m_fAnimRatio = 0.f;
	m_fPreviousAnimRatio = 0.f;
}

SPtr<CPlayer_DepulsoSkill_State> CPlayer_DepulsoSkill_State::Create()
{
	return ToSPtr(new CPlayer_DepulsoSkill_State{});
}
