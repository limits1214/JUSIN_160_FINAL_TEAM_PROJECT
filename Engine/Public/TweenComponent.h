#pragma once

#include "Component.h"

NS_BEGIN(Engine)

struct FUITween
{
	float fStartValue;
	float fEndValue;
	float fDuration = 1.f;
	float fCurrentTime = 0.0f;

	// 매 프레임 보간된(Lerp) 값을 전달받아 실제 객체에 적용할 람다 함수
	std::function<void(float)> OnUpdate;

	// 애니메이션이 완전히 끝났을 때 1회 호출될 람다 함수 (옵션)
	std::function<void()> OnComplete;

	bool bIsFinished = false;
};

class ENGINE_DLL TweenComponent : public CComponent
{
public:
	DECLARE_DERIVED_TYPE(TweenComponent, CComponent)
private:
	explicit TweenComponent();
	~TweenComponent() override;


private:
	std::vector<FUITween> m_ActiveTweens;
public:
	// 새로운 트윈을 추가하는 함수
	void PlayTween(float start, float end, float duration, std::function<void(float)> onUpdate, std::function<void()> onComplete = nullptr);
	void Tick(_float fTimeDelta);
	void ClearTweens();

public:
	virtual HRESULT		Initialize(void* pArg) override;

public:
	static UPtr<TweenComponent> Create();
	UPtr<CPrototype> Clone(void* pArg) override;
};

NS_END
