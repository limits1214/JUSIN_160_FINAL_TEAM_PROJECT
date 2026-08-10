#pragma once
#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)
enum class EDG_SPAWN_NUMBER { FIRST, SECOND, THIRD };
typedef struct stredganimfsm
{
	int32_t iAnimIndex{};
	_float	fBlend{};
}EDG_ANIM_FSM;

class CEdg_Spawn : public CState
{
public:
	DECLARE_DERIVED_TYPE(CEdg_Spawn, CState)
private:
	CEdg_Spawn();
	~CEdg_Spawn() override;
private:
	HRESULT Initialize();
public:
	void Enter(CStateMachine* pStateMachine)override;
	void Exit(CStateMachine* pStateMachine)override;

	void PriorityUpdate(CStateMachine* pStateMachine, _float fTimeDelta) override;
	void Update(CStateMachine* pStateMachine, _float fTimeDelta) override;
private:
	_bool		MoveSpawn(CEnderDragon* pDragon, _float fTimeDelta);
	void		Play_Anim(CEnderDragon* pDragon, _float fTimeDelta);
private:
	uint32_t				m_iEffectID{};

	_bool					m_bNext{};
	_float					m_fTick{};
	_float3					m_vNextDir{}, m_vLastDir{};
	EDG_SPAWN_NUMBER		m_eSpawn{EDG_SPAWN_NUMBER::FIRST};

	std::list<EDG_ANIM_FSM> m_Anims;
	std::list<_float3>	m_PhasePos;
public:
	static SPtr<CEdg_Spawn> Create();
};

NS_END

