#pragma once

#include "Client_Defines.h"
#include "UI_Enums.h"
#include "GameObject.h"

#include <array>

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
class CUIObject;
NS_END

NS_BEGIN(Client)


class CUIController final : public E::CGameObject
{
public:
	DECLARE_DERIVED_TYPE(CUIController, E::CGameObject)

private:
	CUIController();
	~CUIController() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;
	


	// ******************** 외부 호출용 함수 ***************************//
public:
	void CreatePlayScreen(); // hp등 플레이 스크린 화면 생성
	void CreateSpellType();  // b눌렀을때 스펠 슬롯 설정 생성
	void DeleteSpellType(); // b눌렀을때 스펠 슬롯 설정 해제
	void CreateDeathScene(); // 죽는 화면

	// ******** HP
	void SetHPMax(_float maxHP);  // 최대 hp
	void AddHP(_float amoutFill); // 플마 hp
	
	// ********* Finisher
	void AddFinisher(_float amountFill); // 필살기 게이지
	void SetFinisher(_float amountFill);

	// ********* SpellSlot
	void SetSpellType(uint32_t SlotNumber, uint32_t SpellType); 
	uint32_t GetSpellType(uint32_t SlotNumber); // 1234 슬롯 마법 타입 UI_ENUM에 스펠타입 있음
	void UseSpell(uint32_t SlotNumber);

	// ********* Potion
	void SetPotionCount(_float cnt); // 포션 개수 세팅
	void AddPotionCount(_float cnt); // 포션 개수 플마
	void UsePotion(); // 포션 1개 사용

	// ********** PickIcon -> 스펠 피킹용
	void SetTargetIcon(uint32_t type) { m_TargetSpellType = type; }
	uint32_t GetTargetIcon() { return m_TargetSpellType; }

	// ********** MonsterHP
	void TargetMonsterHP(CHandle monsterHandle); // 나중에는 이 함수 쓸듯 handle로 hp랑 maxhp받아와서
	void AddMonsterHP(_float fill);

	// ********** Death Button
	void ClearDeathScene();

	// ********** Quest UI
	_bool SetQuestUIGroupActive(
		QUEST_UI_GROUP group, _bool active);

private: // ************ 계속 바뀌는 유아이 ******************* //
	/*************플레이 화면 유아이******************/
	CHandle m_SpellSlot[4] = {};
	CHandle m_PlayerHP{};
	CHandle m_Finisher[3] = {};
	CHandle m_FinisherIcon{};
	CHandle m_PotionCount{};
	std::vector<CHandle> m_PlaySceneStatic{};

	std::optional<CHandle> m_MonsterHP{ std::nullopt };
	std::optional<CHandle> m_TargetHandle{ std::nullopt };
	std::optional<CHandle> m_ReserveTargetHandle{std::nullopt};

	/*****************스펠 슬롯 유아이********************/
	CHandle m_SpellShortCutKeySlot[4] = {};
	std::vector<CHandle> m_SpellBTNs = {};
	uint32_t m_TargetSpellType = ETOUI(SPELL_TYPE::B_NONE);
	std::vector<CHandle> m_SpellSlotStatic{};

	/********************죽는 화면**********************/
	CHandle m_Desolve{};
	CHandle m_DeathDivider{};
	std::vector<CHandle> m_DeathTxt{};
	CHandle m_BeathButton[3] = {};
	CHandle m_GameOverMask{};
	_bool m_isCreateDeathScene{ false };

	/***********커서*************/
	std::optional<CHandle> m_Cursor{};
	_bool CursorCreate{ false };
private:
	_bool ActivePlayScreen{ false };
	_bool ActiveShortCutSlot{ false };

	// Finisher
	_float m_FinisherAmount{ 0.f };
	float targetAmount{0.f};

	// Potion
	int m_iPotionCNT{23};

	// MonsterHP
	_bool m_bMonsterHP{ false };

	// Quest UI
	static constexpr size_t QUEST_UI_GROUP_COUNT =
		static_cast<size_t>(QUEST_UI_GROUP::END);
	std::optional<CHandle> m_hMiniMap{ std::nullopt };
	std::array<_bool, QUEST_UI_GROUP_COUNT> m_QuestUIGroupStates{};
	std::array<_bool, QUEST_UI_GROUP_COUNT> m_QuestUIGroupDirty{};
	uint64_t m_iQuestUIListenerID{};

	//*********내부함수*************//
private:
	CUIObject* SafeGetOBJ(CHandle pHandle);

	/******몬스터 hp********/
	public:
	void CreateMonsterHP();
	void DeleteMonsterHP();
	void SetMonsterHPBool(_bool isHP) { m_bMonsterHP = isHP; }
	void SetMonsterHPNull() { m_MonsterHP = std::nullopt; }
	void UpdateMonsterHP();

	/**********Quest UI************/
private:
	void BindMiniMap();
	void SubscribeQuestUIEvents();
	void ApplyPendingQuestUIGroups();

	/**********모션************/
	private:
	void PlayScaleAlphaDownDelete(CHandle pHandle);
	void PlayFadeOutDelete(CHandle pHandle, float delaytime = 0.f, float playtime = 0.3f);
	void PlayFadeOutOnly(CHandle pHandle);
	void PlayFadeInOnly(CHandle pHandle);
	void PlayMonsterHPDelete(CHandle pHandle);
	void PlayMonsterHPDeleteCreate(CHandle pHandle);
	void PlayDividerUPWidth(CHandle pHandle);
	void PlayAlphaUP(CHandle pHandle, float delaytime = 2.f, float playTime = 1.f);
public:
	static E::UPtr<CUIController> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;

private:
	void Free() override;
};

NS_END
