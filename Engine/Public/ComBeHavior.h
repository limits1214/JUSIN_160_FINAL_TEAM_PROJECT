#pragma once
#include "Component.h"
#include "BTRoot.h"

NS_BEGIN(Engine)
class  ENGINE_DLL CComBeHavior : public CComponent
{
public:		
	typedef struct tagbehaviordesc : public CComponent::DESC
	{
		_string OwnerName{}, LoadPath{}, resBeHaviorMajor{}, resBeHaviorMinor{};
	
	}BEHAVIOR_DESC;
public:
	DECLARE_DERIVED_TYPE(CComBeHavior, CComponent)

	//CComBeHavior& operator=(const CComBeHavior&) = delete;
protected:
	explicit CComBeHavior();
	explicit CComBeHavior(const CComBeHavior& Prototype);
	 ~CComBeHavior() override;

private:
	virtual HRESULT			InitializePrototype(void* pArg = nullptr) override;
	HRESULT					Initialize(void* pArg) override;

	void					Set_NodeInfo(CBTRoot* pNode,std::vector<uint32_t>& SubTree);
	void					ResetNode(CBTRoot* pNode);
public:
	HRESULT					Save_Data(const _string& filePath);
	HRESULT					Load_Data(const _string& filePath);
	HRESULT					Load_DataByResource(const _string& restagMajor, const _string& restagMinor);

	void					RegistNode(uint32_t iIndex, CBTRoot* pNode);
public:
	void					Update(_float fTimeDelta);					
	void					UpdateGUI()	override;
public:
	
	class CBTComposite*		Get_Selector();
	class CBTBlackBoard*	Get_Blackboard();

	uint32_t				Get_Flag() { return m_iFlag; }
	uint32_t&				Get_NodeID() { return m_iNodeID; }
	
	CBTRoot*				Find_Node(const uint32_t& iNode);
	void					Add_Node(CBTRoot* pParent, uint32_t iSlot, UPtr<CBTRoot> pNode);

	void					UnRegistNode(uint32_t iindex);
	void					AbortNode();
	void					Set_JsonFileName(const _string& Name) { m_FileName = Name; }

	_bool					Check_Flag(uint32_t iFlag);
	void					Set_Flag(uint32_t iFlag, FLAGTYPE eType);
private:
	UPtr<class CBTComposite>				m_Root{};
	UPtr<class CBTBlackBoard>				m_pBlackBoard{};
	
	std::map<uint32_t, CBTRoot*>				m_NodeMap;

	uint32_t								m_iNodeID{ 0 }, m_iFlag{ 0 };
	_string									m_FileName{}, m_ComponentName{};
public:
	static UPtr<CComBeHavior>Create();
	virtual UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
