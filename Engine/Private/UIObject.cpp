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
	m_UIINFO.Alpha = pDesc->fAlpha;
	m_UIINFO.Restag = pDesc->ResTag;
	m_UIINFO.Name = pDesc->Name;

	if (FAILED(CGameObject::Initialize(pArg)))
		return E_FAIL;

	m_CurrentState = UI_STATE::APPEAR;

	return S_OK;
}

void CUIObject::Update(_float fTimeDelta)
{
	// 부모가 있다면 local변수
	if (std::nullopt != m_pParent)
	{
		if (m_bWorldSpace)
		{
			_float scaleFactor = 0.01f; 
			GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * scaleFactor, m_UIINFO.SizeY * scaleFactor, 1.f });

			if (std::nullopt != m_pParent)
			{
				CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);

				GetTransform().SetParentWorldMatrix(*parentUI->GetTransform().GetCombinedWorldMatrix());

				_float local3DX = m_UIINFO.LocalX * scaleFactor;
				_float local3DY = -m_UIINFO.LocalY * scaleFactor;
				
				_float local3DZ = -0.001f * m_UIINFO.WeightOffset;

				GetTransform().SetPosition(XMVectorSet(local3DX, local3DY, local3DZ, 1.f));
			}
		}
		else
		{
			CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*m_pParent);
			UI_INFO& parentInfo = parentUI->GetUIInfo();

			m_ScaleRatio = parentUI->GetScaleRatio();

			_float2 parentPivot = parentUI->GetPivot();

			_float distanceX = m_UIINFO.LocalX - parentPivot.x;
			_float distanceY = m_UIINFO.LocalY - parentPivot.y;

			_float parentRad = XMConvertToRadians(parentInfo.Rot);
			_float cosR = cosf(parentRad);

			_float sinR = sinf(-parentRad);

			_float rotatedX = (distanceX * cosR) - (distanceY * sinR);
			_float rotatedY = (distanceX * sinR) + (distanceY * cosR);

			m_UIINFO.fX = parentInfo.fX + ((parentPivot.x + rotatedX) * m_ScaleRatio);
			m_UIINFO.fY = parentInfo.fY + ((parentPivot.y + rotatedY) * m_ScaleRatio);

			m_UIINFO.Rot = parentInfo.Rot + m_UIINFO.LocalRot;
			m_UIINFO.Alpha = parentInfo.Alpha * m_UIINFO.AlphaRatio;
			m_UIINFO.Weight = parentInfo.Weight + m_UIINFO.WeightOffset;

			CalcUICoord();
		}

	}
	else
	{
		m_vPivot = { 0.f, 0.f };

		if (!m_bWorldSpace)
		{
			CalcUICoord();
		}
		else
		{
			GetTransform().SetPosition(XMVectorSet(0.f, 0.f, 0.f, 1.f));
		}
	}
}

void CUIObject::LateUpdate(_float fTimeDelta)
{
	if (!m_isActive)
		return;

	if (m_bWorldSpace)
		E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::BLEND, this);
	else
		E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);

	GetTransform().Update();
}

void CUIObject::PlayEffect(uint32_t uiState)
{
}

void CUIObject::ClearEffectTweens()
{
	m_pComTween->ClearTweens();
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

	if (m_pComTransform == nullptr) return;

	GetTransform().SetScale(E::_float3{ m_UIINFO.SizeX * m_ScaleRatio * m_localScaleRatio, m_UIINFO.SizeY * m_ScaleRatio * m_localScaleRatio, 1.f });

	_float x = m_UIINFO.fX - clientWidth * 0.5f;
	_float y = -m_UIINFO.fY + clientHeight * 0.5f;

	_float pivotX = m_vPivot.x * m_ScaleRatio * m_localScaleRatio;
	_float pivotY = -m_vPivot.y * m_ScaleRatio * m_localScaleRatio;

	_float rad = XMConvertToRadians(m_UIINFO.Rot);
	_float cosR = cosf(rad);
	_float sinR = sinf(rad);

	_float rotatedPivotX = pivotX * cosR - pivotY * sinR;
	_float rotatedPivotY = pivotX * sinR + pivotY * cosR;

	x = x + pivotX - rotatedPivotX;
	y = y + pivotY - rotatedPivotY;

	GetTransform().SetPosition(XMVectorSet(x, y, GetTransform().GetPosition().z, 1.f));
	GetTransform().SetRotation({ 0.f, 0.f, 1.f }, m_UIINFO.Rot);
}

void CUIObject::SetChildPivot()
{
	for (auto cHandle : m_vChildren)
	{
		if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(cHandle))
		{
			Engine::CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(cHandle);
			pUI->SetPivot(m_vPivot);
		}
			
	}
}
