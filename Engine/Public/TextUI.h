#pragma once

#include "GameObject.h"
#include "UIObject.h"

NS_BEGIN(Engine)

typedef struct tagTextInfo
{
	std::wstring Text{ L"" };
}TEXT_INFO;

class ENGINE_DLL CTextUI : public CUIObject
{
public:
	DECLARE_DERIVED_TYPE(CUITex, CUIObject)

public:
	typedef struct tagTextDesc : public E::CUIObject::UIOBJECT_DESC
	{
		std::wstring Text{ L"" };
	}TEXT_DESC;

protected:
	CTextUI();
	~CTextUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);

protected:
	virtual void PlayEffect(uint32_t uiState);

public:
	TEXT_INFO& GetTextInfo() { return m_textInfo; }
	const TEXT_INFO& GetTextInfo() const { return m_textInfo; }

private:
	TEXT_INFO m_textInfo{};
};

NS_END
