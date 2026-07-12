#pragma once

#include "GameObject.h"
#include "UIObject.h"

NS_BEGIN(Engine)

typedef struct tagFlipbookInfo
{
	uint32_t	cellsize = 4096;
	_float		Padding = 2 / 4096;
	uint32_t	TotalFrame = 64;
	_float		Duration = 1.5f;
}FLIP_INFO;

class ENGINE_DLL CFlipbookUI : public CUIObject
{
public:
	DECLARE_DERIVED_TYPE(FlipbookUI, CUIObject)
public:
	typedef struct tagFlipbookDesc : public E::CUIObject::UIOBJECT_DESC
	{
		uint32_t	cellsize;
		uint32_t	Padding;
		uint32_t	TotalFrame;
		_float 		Duration;
	}FLIPBOOK_DESC;

protected:
	CFlipbookUI();
	~CFlipbookUI() override;

public:
	HRESULT Initialize(void* pArg) override;
	virtual void Update(_float fTimeDelta);
	virtual void LateUpdate(_float fTimeDelta);

protected:
	virtual void PlayEffect(uint32_t uiState);

protected:
	FLIP_INFO	m_FLIPINFO{};

public:
	const FLIP_INFO& GetFlipInfo() const { return m_FLIPINFO; }
	FLIP_INFO& GetFlipInfo() { return m_FLIPINFO; }

protected:
	uint32_t		m_CurrentFrame = 0;
	uint32_t		m_StartFrame = 0;
	
	uint32_t		m_Columns = 8;
	uint32_t		m_Rows = 8;

	_float			m_curColum = 0;
	_float			m_curRow = 0;

	_float2			m_texcoord;
	_float2			m_uvSize;

protected:
	bool		m_Loop = true;
	_float		m_fSumTime = 0.f;

private:
	char g_BasePath[256] = "./Resources/SampleClient/UIData/LevelUI/";
};

NS_END
