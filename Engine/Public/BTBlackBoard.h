#pragma once
#include "Engine_Defines.h"
#include "Engine_Base.h"
NS_BEGIN(Engine)
class ENGINE_DLL CBTBlackBoard : public CEngineBase
{
public:
	DECLARE_DERIVED_TYPE(CBTBlackBoard, CEngineBase)
private:
	CBTBlackBoard() ;
	~CBTBlackBoard() override ;

public:
	HRESULT			Initialize();
public:
template<typename T>
void Set_Value(const StringID& Key, const T& Value)
{
	m_Values.insert_or_assign(Key, Value);
}

template<typename T>
T* Get_Value(const StringID& Key)
{
	auto iter = m_Values.find(Key);
	
	if (iter == m_Values.end())
		return nullptr;

	return std::any_cast<T>(&iter->second);
}
public:
	_bool		Has_Value(const StringID& Key) const { return m_Values.contains(Key); }
	void		Remove_Value(const StringID& Key) { m_Values.erase(Key); }
	void		Clear_BlackBoard() { m_Values.clear(); }

private:
	std::unordered_map<StringID, std::any> m_Values;
public:
	static			UPtr<CBTBlackBoard>	Create();
};
NS_END

