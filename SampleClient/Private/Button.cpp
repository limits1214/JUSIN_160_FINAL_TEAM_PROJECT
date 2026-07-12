#include "pch.h"
#include "Button.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "UIManager.h"
#include "Client_Defines.h"
#include "EffectUI.h"
#include "ButtonComponent.h"
#include "TweenComponent.h"

NS_USING(Client)

CButton::CButton()
{

}

CButton::~CButton()
{
}

HRESULT CButton::InitializePrototype(void* pArg)
{


	return S_OK;
}

HRESULT CButton::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CUIObject::UIOBJECT_DESC*>(pArg);

	if (FAILED(CUIObject::Initialize(pDesc)))
		return E_FAIL;

	{
		/* Buffer */
		CComConstantBuffer::DESC Desc{};
		Desc.cBufferId = { TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerUI" };
		if (FAILED(AddComponentFromProto("PERMANENT", "Prototype_Component_ConstantBuffer", "ComCBufferPerUI", &Desc, &m_pComCBufferPerUI)))
		{
			return E_FAIL;
		};

		/* Component */
		CComponent::DESC CDesc{};
		Desc.pGameObject = this;

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_ButtonUI", "Com_Button", &CDesc, &m_pComCButton)))
		{
			return E_FAIL;
		};

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::BUTTON);

	return S_OK;
}

void CButton::PriorityUpdate(E::_float fTimeDelta)
{
}

void CButton::Update(E::_float fTimeDelta)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	if (!m_isActive)
		return;

	CUIObject::Update(fTimeDelta);

	m_pComCButton->CheckPixelPerfectCollision(mousePos, true);

	if (m_bMouseTracking)
	{
		m_UIINFO.fX = mousePos.x;
		m_UIINFO.fY = mousePos.y;
		CalcUICoord();
	}

	for (auto& pComponent : m_UIComponents)
	{
		pComponent->Update(fTimeDelta, mousePos);
	}

	if (m_pComTween != nullptr)
	{
		m_pComTween->Tick(fTimeDelta);
	}
}

void CButton::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CButton::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = "LEVEL_UIEDITOR";

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexUI");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexUI");
	const auto& viBuffer = E::CGameInstance::Get().GetResourceFirst<E::CResQuadTexBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "VIBuffer_QuadTex");

	pContext->IASetInputLayout(vs->GetInputLayout().Get());
	pContext->VSSetShader(vs->GetVertexShader().Get(), nullptr, 0);
	pContext->PSSetShader(ps->GetPixelShader().Get(), nullptr, 0);

	ID3D11Buffer* vertexBuffers[] = {
			viBuffer->GetVertexBuffer().Get()
	};
	uint32_t strides[] = {
		viBuffer->GetVertexStride()
	};
	uint32_t offsets[] = {
		0
	};
	pContext->IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);
	pContext->IASetIndexBuffer(viBuffer->GetIndexBuffer().Get(), viBuffer->GetIndexFormat(), 0);
	pContext->IASetPrimitiveTopology(viBuffer->GetPrimitiveType());

	{
		E::CB_PER_UI perUI{};
		perUI.texCoord = { 0.f, 0.f };
		perUI.uvSize = { 0.f, 0.f };
		perUI.color = { m_UIINFO.Color.x, m_UIINFO.Color.y, m_UIINFO.Color.z, m_UIINFO.Alpha };

		if (FAILED(m_pComCBufferPerUI->MapDiscard(pContext, &perUI, sizeof(perUI))))
		{
			return E_FAIL;
		}
		pContext->VSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
		pContext->PSSetConstantBuffers(7, 1, m_pComCBufferPerUI->GetAdressOfBuffer());
	}

	{
		//auto pUICam = E::CGameInstance::Get().GetActiveUICamera();
		{
			auto pCbPerObject = E::CGameInstance::Get().GetResourceFirst<E::CResCBuffer>(TAG_RES_GRP_PERMANENT_BUFFER, "CB_PerObject");
			D3D11_MAPPED_SUBRESOURCE mappedSubResource;
			if (SUCCEEDED(pContext->Map(pCbPerObject->GetCBuffer().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubResource)))
			{

				E::CB_PER_OBJECT cbPerObject{};
				cbPerObject.matWorld = *GetTransform().GetWorldMatrix();
				XMStoreFloat4x4(&cbPerObject.matWVP, GetTransform().GetLoadedWorldMatrix() * ctx.matProj);

				memcpy(mappedSubResource.pData, &cbPerObject, sizeof(cbPerObject));
				pContext->Unmap(pCbPerObject->GetCBuffer().Get(), 0);
			}
			pContext->VSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
			pContext->PSSetConstantBuffers(0, 1, pCbPerObject->GetCBuffer().GetAddressOf());
		}
	}

	{
		const auto& srv = E::CGameInstance::GetConst().GetResourceFirst<E::CResTexture2D>(currentLevel, m_UIINFO.Restag);
		pContext->PSSetShaderResources(0, 1, srv->GetSRV().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

void CButton::PlayEffect(uint32_t uiState)
{
	if (m_pComTween == nullptr)
		return;

	switch (uiState)
	{
	case ETOUI(UI_STATE::ENTER):
		
		CGameInstance::Get().LuaScriptExecute(R"( print("Hello Lua") )");
		m_pComTween->ClearTweens();
		{
			this->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("ScaleUp");
			//_float startScale = this->GetScaleRatio();
			//m_pComTween->PlayTween(startScale, 1.1f, 0.1f,
			//	[this](float currentValue) {
			//		this->SetScaleRatio(currentValue);
			//		this->CalcUICoord();
			//	});
			OnHoverEnter(this);
		}
		if (m_Effect_Hovered != nullptr)
		{
			m_Effect_Hovered->SetActive(true);
			m_Effect_Hovered->OnHoverEnter = GET_SINGLE(UIManager)->GetAction("FadeIn");

			//float startAlpha = m_Effect_Hovered->GetAlpha();
			//m_pComTween->PlayTween(startAlpha, 1.0f, 0.3f,
			//	[this](float currentValue) {
			//		m_Effect_Hovered->SetAlpha(currentValue);
			//	});
			m_Effect_Hovered->OnHoverEnter(m_Effect_Hovered);
		}
		break;

	case ETOUI(UI_STATE::CLICK):
		//if (m_Effect_Clicked != nullptr)
		//{
		//	m_Effect_Clicked->SetActive(true);
		//
		//	float startAlpha = m_Effect_Clicked->GetAlpha();
		//	m_pComTween->PlayTween(startAlpha, 1.0f, 0.3f,
		//		[this](float currentValue) {
		//			m_Effect_Clicked->SetAlpha(currentValue);
		//		});
		//}
		break;
	case ETOUI(UI_STATE::EXIT):
		m_pComTween->ClearTweens();
		{
			_float startScale = this->GetScaleRatio();

			m_pComTween->PlayTween(startScale, 1.f, 0.1f,
				// 매 프레임 알파값을 적용하는 Update 람다
				[this](float currentValue) {
					this->SetScaleRatio(currentValue);
					this->CalcUICoord();
				});
		}

		if (m_Effect_Hovered != nullptr)
		{
			float startAlpha = m_Effect_Hovered->GetAlphaRatio();

			m_pComTween->PlayTween(startAlpha, 0.0f, 0.3f,
				[this](float currentValue) {
					m_Effect_Hovered->SetAlphaRatio(currentValue);
				},
					[this]() {
					m_Effect_Hovered->SetActive(false);
				});
		}

		break;
	}
}

E::UPtr<CButton> CButton::Create()
{
	auto pInstance = E::ToUPtr(new CButton{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CButton");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CButton::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CButton{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CButton");
		return nullptr;
	}

	return pInstance;
}
