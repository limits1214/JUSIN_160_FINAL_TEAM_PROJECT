#pragma once
#include "Prototype.h"
#include "Engine_Defines.h"
#include "GameInstance.h"
#include "ComBeHavior.h"
//뿌리
NS_BEGIN(Engine)
class ENGINE_DLL  CBTRoot : public CPrototype
{
public:
#define X(name,idx) name = idx,
	enum class BTFLAG{BTFLAG_M};
#undef X
public:
	DECLARE_DERIVED_TYPE(CBTRoot, CPrototype)
public:
	typedef struct tagbtroot
	{
		CHandle		Handle;
		NODEGROUP    eGroup;
		GUINODE		 m_GuiNode;
		GUINODE_LINK m_GuiLink;
		_string		 NodeName;
		_string      m_MasterName;
	}BTROOT_DESC;

protected:
	explicit CBTRoot();
	CBTRoot(const CBTRoot& rhs);
	~CBTRoot() override;

	HRESULT						InitializePrototype(void* pArg = nullptr) override;
	virtual HRESULT				Initalize(void* pArg);

	virtual						void OnEnter() {};
	virtual						void OnExit(EVALUATE eResult) {};
public:
	virtual HRESULT				Save_SubTree(const _string& SavePath);
	GUINODE&					Get_GuiNodeInfo() { return m_GuiNode; }
	GUINODE_LINK&				Get_GuiNodeLink() { return m_GuiLink; }
	void						Set_Handle(CHandle Handle) { m_Handle = Handle; }
	CHandle&					Get_Handle() { return m_Handle; }
	virtual void				ResetDebug() { m_eDebug = EVALUATE::END; }
	EVALUATE					GetDebugType() const {return m_eDebug;}

	const _string& GetMasterName() { return m_MasterName; }
public:
	virtual nlohmann::json		Save_Node();
	virtual HRESULT				Load_json(const nlohmann::json& j);
public:
	virtual EVALUATE			Evaluate(_float fTimeDelta) PURE;
	virtual void				Abort() PURE;
	void						AbortExecute();
	EVALUATE					Execute(_float fTimeDelta);
	void						Set_OwnerName(const _string& strOwnerName) { m_OwnerName = strOwnerName; }
	class CComBeHavior*			Get_ComBT();
	_bool						Check_Flag(uint32_t iFlag);
	uint32_t					Get_Flag();
	void						Set_Flag(uint32_t iFlag, FLAGTYPE eType);
protected:
	GUINODE								m_GuiNode{};
	GUINODE_LINK						m_GuiLink{};
	CHandle								m_Handle{};
	_string								m_MasterName{}, m_OwnerName{};
	NODEGROUP							m_eGroup{};
	_bool								m_bEntered{ false };
	EVALUATE							m_eDebug{};
public:
	template<typename T1> 
	T1* Get_Component(const CHandle & Handle, const _string& name)
	{
		if (auto pObj = CGameInstance::Get().GetGameObjectByHandle(Handle))
		{
			if (auto pComBt = pObj->GetComponent<T1>(name))
			{
				m_Handle = Handle;
				return pComBt;
			}
		}
		return nullptr;
	}
};

NS_END
