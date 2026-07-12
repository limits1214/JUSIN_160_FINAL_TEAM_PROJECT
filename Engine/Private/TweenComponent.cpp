#include "pch.h"

#include "TweenComponent.h"
#include "UIObject.h"

NS_USING(Engine)

TweenComponent::TweenComponent()
{
}

TweenComponent::~TweenComponent()
{
}

void TweenComponent::PlayTween(float start, float end, float duration, std::function<void(float)> onUpdate, std::function<void()> onComplete)
{
	FUITween newTween;
	newTween.fStartValue = start;
	newTween.fEndValue = end;
	newTween.fDuration = duration;
	newTween.OnUpdate = onUpdate;
	newTween.OnComplete = onComplete;

	m_ActiveTweens.push_back(newTween);
}

void TweenComponent::Tick(_float fTimeDelta)
{
	for (auto& tween : m_ActiveTweens)
	{
		if (tween.bIsFinished) continue;

		tween.fCurrentTime += fTimeDelta;

		float fRatio = tween.fCurrentTime / tween.fDuration;
		if (fRatio > 1.0f) fRatio = 1.0f;

		float fCurrentValue = std::lerp(tween.fStartValue, tween.fEndValue, fRatio);

		if (tween.OnUpdate)
			tween.OnUpdate(fCurrentValue);

		if (fRatio >= 1.0f)
		{
			tween.bIsFinished = true;
			if (tween.OnComplete)
				tween.OnComplete();
		}
	}

	std::erase_if(m_ActiveTweens, [](const FUITween& t) { return t.bIsFinished; });
}

void TweenComponent::ClearTweens()
{
	m_ActiveTweens.clear();
}

HRESULT TweenComponent::Initialize(void* pArg)
{
	if (FAILED(CComponent::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

UPtr<TweenComponent> TweenComponent::Create()
{
	auto pInstance = ToUPtr(new TweenComponent{});
	if (FAILED(pInstance->InitializePrototype()))
	{
		MSG_BOX("Failed to Created : TweenComponent");
		return nullptr;
	}
	return pInstance;
}

UPtr<CPrototype> TweenComponent::Clone(void* pArg)
{
	auto pInstance = ToUPtr(new TweenComponent{ *this });
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned: TweenComponent");
		return nullptr;
	}
	return pInstance;
}
