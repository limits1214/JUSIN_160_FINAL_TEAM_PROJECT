#include "FlipbookUI.h"

CFlipbookUI::CFlipbookUI()
{
}

CFlipbookUI::~CFlipbookUI()
{
}

HRESULT CFlipbookUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CFlipbookUI::FLIPBOOK_DESC*>(pArg);
	m_FLIPINFO.cellsize = pDesc->cellsize;
	m_FLIPINFO.TotalFrame = pDesc->TotalFrame;
	m_FLIPINFO.Duration = pDesc->Duration;
	m_FLIPINFO.Padding = pDesc->Padding;

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CFlipbookUI::Update(_float fTimeDelta)
{
	CUIObject::Update(fTimeDelta);

	uint32_t	cellSize = m_FLIPINFO.cellsize;
	uint32_t	totalFrame = m_FLIPINFO.TotalFrame;
	_float		duration = m_FLIPINFO.Duration;
	_float		padding = m_FLIPINFO.Padding / cellSize;

	padding = 2.f / cellSize;
	m_Columns = static_cast<int>(std::round(std::sqrt(totalFrame)));
	m_Rows = static_cast<int>(std::round(std::sqrt(totalFrame)));

	if (m_Loop == false && m_CurrentFrame == totalFrame)
		return;

	m_fSumTime += fTimeDelta;

	uint32_t frameCount = (totalFrame - m_StartFrame + 1);
	float delta = duration / frameCount;

	if (m_fSumTime >= delta)
	{
		m_fSumTime = 0.f;
		m_CurrentFrame = (m_CurrentFrame + 1) % frameCount;
	}

	m_curColum = m_CurrentFrame % m_Columns;
	m_curRow = m_CurrentFrame / m_Rows;

	m_texcoord = { m_curColum / (float)m_Columns + padding, m_curRow / (float)m_Rows + padding };
	m_uvSize = { 1 / (float)m_Columns - padding * 2 , 1 / (float)m_Rows - padding * 2 };

	// 잠깐 정지
	//if (m_CurrentFrame % m_iPuaseFrame == 0 && m_fPauseSumTime < m_fPauseTime && m_CurrentFrame != 0 && m_isPause)
	//{
	//	m_fPauseSumTime += fTimeDelta;
	//}
	//else
	//{
	//	m_fPauseSumTime = 0.f;
	// 
	// 		m_fSumTime += fTimeDelta;
	//
	//	uint32_t frameCount = (m_TotalFrame - m_StartFrame + 1);
	//	float delta = m_fDuration / frameCount;
	//	
	//	if (m_fSumTime >= delta)
	//	{
	//		m_fSumTime = 0.f;
	//		m_CurrentFrame = (m_CurrentFrame + 1) % frameCount;
	//	}
	//}
}

void CFlipbookUI::LateUpdate(_float fTimeDelta)
{
}

void CFlipbookUI::PlayEffect(uint32_t uiState)
{
}
