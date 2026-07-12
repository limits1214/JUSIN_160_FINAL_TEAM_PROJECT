#pragma once

#include "UITex.h"
#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComConstantBuffer;
class CButtonComponent;
class TweenComponent;
NS_END

NS_BEGIN(Client)
class CEffectUI;

class CButton final : public E::CUITex
{
public:
	DECLARE_DERIVED_TYPE(CButton, E::CUITex)

private:
	CButton();
	~CButton() override;

public:
	HRESULT InitializePrototype(void* pArg = nullptr) override;
	HRESULT Initialize(void* pArg) override;
	void PriorityUpdate(E::_float fTimeDelta) override;
	void Update(E::_float fTimeDelta) override;
	void LateUpdate(E::_float fTimeDelta) override;
	HRESULT Render(ID3D11DeviceContext* pContext, const E::RENDER_CTX& ctx) override;

public:
	void SetMouseTracking(_bool isTracking) { m_bMouseTracking = isTracking; }
	void SetEffectHovered(std::optional<CHandle> effectUIHandle) { m_Effect_Hovered = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*effectUIHandle); }
	void SetEffectClicked(std::optional<CHandle> effectUIHandle) { m_Effect_Clicked = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*effectUIHandle); }
private:
	_bool m_bMouseTracking{ false };

private:
	bool m_bOutline{};

protected:
	virtual void PlayEffect(uint32_t uiState) override;

	CEffectUI* m_Effect_Hovered = nullptr;
	CEffectUI* m_Effect_Clicked = nullptr;

	float m_fHoverScale = 1.1f;
	float m_fScaleDuration = 0.1f;
	float m_fAlphaDuration = 0.3f;
private:
	std::vector<std::optional<CHandle>> m_vEffects;

private:
	std::string m_EffectTag;

public:
	static E::UPtr<CButton> Create();
	E::UPtr<E::CPrototype> Clone(void* pArg) override;
};

NS_END
