#include "pch.h"
#include "EffectUI.h"
#include "GameInstance.h"
#include "CameraObject.h"
#include "Resources.h"
#include "Client_Resources.h"
#include "ComConstantBuffer.h"
#include "Resources.h"
#include "Client_Defines.h"
#include "UIManager.h"
#include "MouseComponent.h"
#include "TweenComponent.h"

NS_USING(Client)

CEffectUI::CEffectUI()
{
}

CEffectUI::~CEffectUI()
{
}

HRESULT CEffectUI::InitializePrototype(void* pArg)
{
	return S_OK;
}

HRESULT CEffectUI::Initialize(void* pArg)
{
	auto		pDesc = static_cast<CFlipbookUI::FLIPBOOK_DESC*>(pArg);

	if (FAILED(CFlipbookUI::Initialize(pDesc)))
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

		if (FAILED(AddComponentFromProto(ES_EngineProtoMajorType::UI, "Prototype_Component_Tween", "Com_Tween", &CDesc, &m_pComTween)))
		{
			return E_FAIL;
		};
	}

	m_UIINFO.UIType = ETOUI(UI_TYPE::FLIPBOOK);

	return S_OK;
}

void CEffectUI::PriorityUpdate(E::_float fTimeDelta)
{
}

void CEffectUI::Update(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	CFlipbookUI::Update(fTimeDelta);
}

void CEffectUI::LateUpdate(E::_float fTimeDelta)
{
	if (!m_isActive)
		return;

	E::CGameInstance::Get().AddRenderObject(E::RENDERGROUP::UI, this);
	GetTransform().Update();
}

HRESULT CEffectUI::Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx)
{
	std::string currentLevel = "LEVEL_UIEDITOR";

	//VS_QuadTex
	const auto& vs = E::CGameInstance::Get().GetResourceFirst<E::CResVertexShader>(TAG_RES_GRP_PERMANENT_SHADER, "VS_QuadTexFlipBook");
	const auto& ps = E::CGameInstance::Get().GetResourceFirst<E::CResPixelShader>(TAG_RES_GRP_PERMANENT_SHADER, "PS_QuadTexFlipBook");
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
		perUI.texCoord = m_texcoord;
		perUI.uvSize = m_uvSize;
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

		const auto& sampler = E::CGameInstance::GetConst().GetResourceFirst<E::CResSamplerState>(TAG_RES_GRP_PERMANENT_STATE, TAG_RES_STATE_SS_LINEAR_WRAP);
		pContext->PSSetSamplers(0, 1, sampler->GetSamplerState().GetAddressOf());
	}

	pContext->DrawIndexed(viBuffer->GetNumIndices(), 0, 0);



	return S_OK;
}

void CEffectUI::PlayEffect(uint32_t uiState)
{
	switch (uiState)
	{
	case ETOUI(UI_STATE::HOVERED):
		std::optional<CHandle> effect = GET_SINGLE(UIManager)->LoadPrefab("Magic");
		break;
	}
}

E::UPtr<CEffectUI> CEffectUI::Create()
{
	auto pInstance = E::ToUPtr(new CEffectUI{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : CFlipBook");
		return nullptr;
	}
	return  pInstance;
}

E::UPtr<E::CPrototype> CEffectUI::Clone(void* pArg)
{
	auto	pInstance = E::ToUPtr(new CEffectUI{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CFlipBook");
		return nullptr;
	}

	return pInstance;
}
