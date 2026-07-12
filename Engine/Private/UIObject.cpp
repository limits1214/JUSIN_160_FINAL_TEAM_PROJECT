#include "pch.h"
#include "UIObject.h"
#include "GameInstance.h"

NS_USING(Engine)

CUIObject::CUIObject()
{
}

CUIObject::~CUIObject()
{
}

void CUIObject::UpdateGUI()
{
	CGameObject::UpdateGUI();
}

HRESULT CUIObject::Initialize(void* pArg)
{
	auto		pDesc = static_cast<UIOBJECT_DESC*>(pArg);

	m_UIINFO.fX = pDesc->fX;
	m_UIINFO.fY = pDesc->fY;
	m_UIINFO.SizeX = pDesc->fSizeX;
	m_UIINFO.SizeY = pDesc->fSizeY;
	m_UIINFO.Alpha = pDesc->fAlpha;
	m_UIINFO.Weight = pDesc->ResWeight;
	m_UIINFO.Restag = pDesc->ResTag;
	m_UIINFO.Name = pDesc->Name;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	CalcUICoord();

	return S_OK;
}

void CUIObject::Update(_float fTimeDelta)
{
	if (std::nullopt != m_pParent)
	{
		CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
		UI_INFO& parentInfo = parentUI->GetUIInfo();

		m_UIINFO.fX		= parentInfo.fX + m_UIINFO.LocalX;
		m_UIINFO.fY		= parentInfo.fY + m_UIINFO.LocalY;
		m_UIINFO.SizeX	= parentInfo.SizeX * m_UIINFO.WidthRatioX;
		m_UIINFO.SizeY	= parentInfo.SizeY * m_UIINFO.WidthRatioY;
		m_UIINFO.Alpha	= parentInfo.Alpha * m_UIINFO.AlphaRatio;
		m_UIINFO.Weight = parentInfo.Weight + m_UIINFO.WeightOffset;
		m_UIINFO.Rot	= parentInfo.Rot + m_UIINFO.LocalRot;

		m_ScaleRatio = parentUI->GetScaleRatio();

		CalcUICoord();
	}

	for (auto& pComponent : m_UIComponents)
	{
		pComponent->Update(fTimeDelta);
	}

		
}

void CUIObject::LateUpdate(_float fTimeDelta)
{
	if (!m_isActive)
		return;
}

void CUIObject::PlayEffect(uint32_t uiState)
{
}

void CUIObject::DeleteChild(CHandle childHandle)
{
	m_vChildren.erase(
		std::remove(m_vChildren.begin(), m_vChildren.end(), childHandle),
		m_vChildren.end());
}

void CUIObject::CalcUICoord()
{
	auto clientSize = CGameInstance::Get().GetClientScreenSize();
	auto clientWidth = clientSize.x;
	auto clientHeight = clientSize.y;
	GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * m_ScaleRatio, m_UIINFO.SizeY * m_ScaleRatio, 1.f });
	auto x = m_UIINFO.fX - clientWidth * 0.5f;
	auto y = -m_UIINFO.fY + clientHeight * 0.5f;
	
	GetTransform().SetPosition(XMVectorSet(x, y, GetTransform().GetPosition().z, 1.f));
}
