#pragma once
#include "BTComposite.h"

NS_BEGIN(Engine)
class  CBTRandSelector final : public CBTComposite
{
public:
	DECLARE_DERIVED_TYPE(CBTRandSelector, CBTComposite)
public:
	typedef struct tagbtselector : CBTRoot::BTROOT_DESC
	{

	}BTSELECTOR_DESC;
private:
	explicit CBTRandSelector();
	CBTRandSelector(const CBTRandSelector& rhs);
	~CBTRandSelector() override;
	HRESULT	InitializePrototype(void* pArg = nullptr);
	HRESULT Initalize(void* pArg) override;

	void OnEnter() override;
	void OnExit(EVALUATE eResult) override;
public:
	virtual EVALUATE		Evaluate(_float fTimeDelta) override;
	void					Abort() override;
	nlohmann::json			Save_Node() override;
	HRESULT					Load_json(const nlohmann::json& j) override;
public:
	void					Shuffle();
private:
	EVALUATE				m_ePreEvaluate{};
	std::vector<int32_t>	m_Shuffle;
	uint32_t				m_iPreindex{};
public:
	static  UPtr<CBTRandSelector> Create(void* pArg);
	UPtr<CPrototype>Clone(void* pArg)override;
};

NS_END
