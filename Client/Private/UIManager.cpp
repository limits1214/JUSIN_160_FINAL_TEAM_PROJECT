#include "pch.h"
#include "GameInstance.h"
#include "UIManager.h"
#include "TextureUI.h"
#include "EffectUI.h"
#include "TextBox.h"
#include "Button.h"
#include <fstream>
#include "LevelLogo.h"
#include "LevelLoading.h"
#include "SpellMeter.h"
#include "HPBar.h"
#include "Level_Defines.h"
#include "MiniMap.h"
#include "UIController.h"
#include "GameOverMask.h"
#include "VideoObject.h"
#include "Monster.h"

NS_USING(Client)

UIManager::~UIManager()
{
	MFShutdown();
}

void UIManager::Update()
{
}

void UIManager::Initialize(ComPtr<ID3D11Device> pDevice, ComPtr<ID3D11DeviceContext> pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	MFStartup(MF_VERSION);
}

void UIManager::InitializeActions()
{
	m_EventMap["ClearAction"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pTween->ClearTweens();
		};
	m_vEventNames.push_back("ClearAction");

	// ==========================================
	// 1. 사이즈 업
	// ==========================================
	m_EventMap["ScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float originScaleRatio = pCaller->GetScaleRatio();
		pTween->PlayTween(pCaller->GetScaleRatio(), 1.1f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}});
	};
	m_vEventNames.push_back("ScaleUp");

	m_EventMap["ScaleUp0.6"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(pCaller->GetScaleRatio(), 0.65f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}});
		};
	m_vEventNames.push_back("ScaleUp0.6");

	m_EventMap["AppearScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		//pCaller->SetInputLcok(true);

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float scaleRatio = pCaller->GetScaleRatio();

		pTween->PlayTween(0.5f, scaleRatio, 0.2f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetAlpha(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("AppearScaleUp");

	m_EventMap["AppearScaleUp0.1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();

			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 0.1f);

			pTween->PlayTween(0.f, 1.f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 0.1f);
		};
	m_vEventNames.push_back("AppearScaleUp0.1");

	m_EventMap["AppearScaleUp1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();

			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle, bSoundPlayed = false](float currentValue) mutable {
					if (auto pObj = GetSafeUI(handle))
					{
						//if (!bSoundPlayed)
						//{
						//	E::CGameInstance::Get()
						//		.GetSoundManager()
						//		->Play2D(
						//			"./Resources/SampleClient/Sound/UI/Paper.wav",
						//			SOUND_PLAY_DESC{
						//				.sBusID = SOUND_BUS::UI,
						//				.fVolume = 0.3f,
						//				.fPitch = 1.f,
						//				.iPriority = 64,
						//				.bLoop = false
						//			});
						//
						//	bSoundPlayed = true;
						//}

						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.f);

			pTween->PlayTween(0.f, 1.f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.f);
		};
	m_vEventNames.push_back("AppearScaleUp1");

	m_EventMap["AppearScaleUp1.1"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			_float scaleRatio = pCaller->GetScaleRatio();


			pTween->PlayTween(0.5f, scaleRatio, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.2f);

			pTween->PlayTween(0.f, 1.f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle))
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}, nullptr, EEaseType::EaseOutQuad, 1.2f);
		};
	m_vEventNames.push_back("AppearScaleUp1.1");

	m_EventMap["TextScaleUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float scaleRatio = pCaller->GetScaleRatio();
		_float2 originSize = pCaller->GetSize();

		pTween->PlayTween(0.5f, 1.f, 0.2f,
			[handle, originSize](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetSize({ originSize.x * currentValue,  originSize.y * currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					pObj->SetAlpha(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("TextScaleUp");

	// ==========================================
	// 2. 사이즈 축소
	// ==========================================
	m_EventMap["ScaleDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.0f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			});
	};
	m_vEventNames.push_back("ScaleDown");

	m_EventMap["ScaleDown0.6"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(pCaller->GetScaleRatio(), 0.6f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}
				});
		};
	m_vEventNames.push_back("ScaleDown0.6");

	m_EventMap["DisappearScaleDown"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 0.5f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, [handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
				}, EEaseType::EaseOutQuad);
	};
	m_vEventNames.push_back("DisappearScaleDown");

	m_EventMap["DisappearScaleDown_D"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 0.5f, 0.2f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad);

		pTween->PlayTween(1.f, 0.f, 0.1f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle))
				{
					if (currentValue <= 1.f)
					{
						pObj->SetAlpha(currentValue);
						pObj->CalcUICoord();
					}
				}
			}, [handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
			}, EEaseType::EaseOutQuad, 0.1f);
	};
	m_vEventNames.push_back("DisappearScaleDown_D");

	// ==========================================
	// 3. 페이드 인
	// ==========================================
	m_EventMap["FadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		_float originAlpha = pCaller->GetAlpha();
		pTween->PlayTween(0.f, originAlpha, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});
	};
	m_vEventNames.push_back("FadeIn");

	m_EventMap["LocalFadeIn"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();
		pTween->PlayTween(pCaller->GetAlphaRatio(), 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalFadeIn");

	m_EventMap["LocalFadeIn0.2"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			CHandle handle = pCaller->GetHandle();
			pTween->PlayTween(pCaller->GetAlphaRatio(), 1.0f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
				});
		};
	m_vEventNames.push_back("LocalFadeIn0.2");

	// ==========================================
	// 4. 페이드 아웃
	// ==========================================
	m_EventMap["FadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlpha(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			}, 
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("FadeOut");

	m_EventMap["LocalFadeOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		//pCaller->SetInputLcok(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetAlphaRatio(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				//if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
			});
	};
	m_vEventNames.push_back("LocalFadeOut");

	m_EventMap["LocalFadeOut0.2"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(pCaller->GetAlphaRatio(), 0.0f, 0.2f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
				},
				[handle]() {
					//if (auto pObj = GetSafeUI(handle)) pObj->SetActive(false);
				});
		};
	m_vEventNames.push_back("LocalFadeOut0.2");

	m_EventMap["FadeOut_D"] = [this](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetInputLcok(true);
		pCaller->SetInputLcok(true);

		CHandle handle = pCaller->GetHandle();
		pTween->PlayTween(pCaller->GetAlpha(), 0.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			},
			[handle, this]() {
				if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
			});
	};
	m_vEventNames.push_back("FadeOut_D");

	m_EventMap["SpellEffect"] = [this](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			CHandle handle = pCaller->GetHandle();

			pTween->PlayTween(1.f, 1.5f, 1.f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetScaleRatio(currentValue);
				}, nullptr, EEaseType::EaseOutQuad);

			pTween->PlayTween(1.f, 0.0f, 1.f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
				},
				[handle, this]() {
					if (auto pObj = GetSafeUI(handle)) DeleteUIRecursive(handle);
				}, EEaseType::EaseOutQuad);
		};
	m_vEventNames.push_back("SpellEffect");

	// ==========================================
	// 5. 페이드 인 & 아웃
	// ==========================================
	m_EventMap["FadInOut"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(0.f, 1.f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle))
				{
					if (auto pNextTween = pObj->GetTweenCom())
					{
						pNextTween->PlayTween(1.f, 0.f, 0.3f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetAlphaRatio(currentValue);
							},
							[handle]() {
								if (auto pObj2 = GetSafeUI(handle)) pObj2->SetActive(false);
							});
					}
				}
			});
	};
	m_vEventNames.push_back("FadInOut");

	// ==========================================
	// 6. 스케일 업 & 다운
	// ==========================================
	m_EventMap["ScaleUpDown"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		CHandle handle = pCaller->GetHandle();

		pTween->PlayTween(pCaller->GetScaleRatio(), 1.2f, 0.08f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetScaleRatio(currentValue);
					pObj->CalcUICoord();
				}
			},
			[handle]() {
				if (auto pObj = GetSafeUI(handle)) 
				{
					if (auto pNextTween = pObj->GetTweenCom()) 
					{
						pNextTween->PlayTween(1.2f, 1.1f, 0.08f,
							[handle](float currentValue) {
								if (auto pObj2 = GetSafeUI(handle)) {
									pObj2->SetScaleRatio(currentValue);
									pObj2->CalcUICoord();
								}
							});
					}
				}
			});
	};
	m_vEventNames.push_back("ScaleUpDown");

	// ==========================================
	// 위치 업
	// ==========================================
	m_EventMap["PosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosUp");

	m_EventMap["LocalPosUp"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetLocalPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetLocalPos().x, originalPos.y - currentValue });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlphaRatio(currentValue);
			});
	};
	m_vEventNames.push_back("LocalPosUp");

	// ==========================================
	// 오른쪽
	// ==========================================
	m_EventMap["PosRight"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 30.f, 0.4f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			});

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("PosRight");

	// ==========================================
	// 바운스
	// ==========================================
	m_EventMap["Bounce"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		pCaller->SetActive(true);
		pCaller->SetInputLcok(true);

		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 150.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x, originalPos.y + currentValue });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBounce);

		pTween->PlayTween(0, 80.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					_float2 pos = pObj->GetPos();
					pObj->SetPos({ originalPos.x + currentValue, pos.y });
					pObj->CalcUICoord();
				}
			}, [handle]() {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetInputLcok(false);
				}
			}, EEaseType::EaseOutQuad);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Bounce");


	// ==========================================
	// 탄성
	// ==========================================
	m_EventMap["Elastic"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 100.f, 1.f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ pObj->GetPos().x, originalPos.y + currentValue - 100.f });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutElastic);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("Elastic");

	// ==========================================
	// 오버슛
	// ==========================================
	m_EventMap["OverShoot"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		_float2 originalPos = pCaller->GetPos();
		pTween->PlayTween(0, 50.f, 0.5f,
			[handle, originalPos](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetPos({ originalPos.x + currentValue, originalPos.y });
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutBack);

		pTween->PlayTween(0.f, 1.0f, 0.3f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
			});

	};
	m_vEventNames.push_back("OverShoot");

	// ==========================================
	// 둥둥
	// ==========================================
	m_EventMap["Floating"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();
		float startY = pCaller->GetUIInfo().fY;
		float endY = startY + 15.0f;

		pTween->PlayTween(startY, endY, 1.5f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().fY = currentValue;
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::Floating, 0.0f, true);
	};
	m_vEventNames.push_back("Floating");

	// ==========================================
	// 순차
	// ==========================================
	int maxIterations = 10;
	for (int i = 1; i <= maxIterations; ++i)
	{
		float delay = i * 0.3f;

		char szName[32];
		snprintf(szName, sizeof(szName), "PosUp%.1f", delay);
		std::string eventName = szName;

		m_EventMap[eventName] = [delay](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetActive(true);
			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float2 originalPos = pCaller->GetPos();

			pTween->PlayTween(0.f, 30.f, 0.4f,
				[handle, originalPos](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetPos({ pObj->GetPos().x, originalPos.y - currentValue });
						pObj->CalcUICoord();
					}
				}, [handle]() {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetInputLcok(false);
					}
				}, EEaseType::Linear, delay, false);

			pTween->PlayTween(0.f, 1.0f, 0.3f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) pObj->SetAlpha(currentValue);
				}, nullptr, EEaseType::Linear, delay, false);
		};
		m_vEventNames.push_back(eventName);
	}

	// ==========================================
	// 펄스
	// ==========================================
	m_EventMap["LockOnEffect"] = [](CUIObject* pCaller)
	{
		if (!pCaller) return;
		auto pTween = pCaller->GetTweenCom();
		if (!pTween) return;

		CHandle handle = pCaller->GetHandle();

		float startSizeX = pCaller->GetUIInfo().SizeX;
		float targetSizeX = startSizeX * 2.0f;

		pTween->PlayTween(startSizeX, targetSizeX, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->GetUIInfo().SizeX = currentValue;
					pObj->GetUIInfo().SizeY = currentValue; 
					pObj->CalcUICoord();
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true); 

		pTween->PlayTween(1.0f, 0.0f, 0.6f,
			[handle](float currentValue) {
				if (auto pObj = GetSafeUI(handle)) {
					pObj->SetAlphaRatio(currentValue);
				}
			}, nullptr, EEaseType::EaseOutQuad, 0.0f, true);
	};
	m_vEventNames.push_back("LockOnEffect");

	/****************텍스트 버튼용*******************/
	m_EventMap["TxtButtonScaleUp"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(originScaleRatio, 1.2f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonScaleUp");

	m_EventMap["TxtButtonScaleDown"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float originScaleRatio = pCaller->GetScaleRatio();
			pTween->PlayTween(originScaleRatio, 1.f, 0.1f,
				[handle](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetScaleRatio(currentValue);
						pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonScaleDown");

	m_EventMap["TxtButtonColorUp"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float3 originColor = pCaller->GetUIInfo().Color;
			pTween->PlayTween(1.f, 2.f, 0.1f,
				[handle, originColor](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->GetUIInfo().Color = { originColor.x * currentValue, originColor.y * currentValue, originColor.z * currentValue };
						//pObj->CalcUICoord();
					}}, [handle]() {
						if (auto pObj = GetSafeUI(handle)) {
							pObj->SetInputLcok(false);
						}
						});
		};
	m_vEventNames.push_back("TxtButtonColorUp");

	m_EventMap["TxtButtonColorDown"] = [](CUIObject* pCaller)
		{
			if (!pCaller) return;
			auto pTween = pCaller->GetTweenCom();
			if (!pTween) return;

			pCaller->SetInputLcok(true);

			CHandle handle = pCaller->GetHandle();
			_float3 originColor = pCaller->GetUIInfo().Color;
			pTween->PlayTween(1.f, 0.5f, 0.1f,
				[handle, originColor](float currentValue) {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->GetUIInfo().Color = { originColor.x * currentValue, originColor.y * currentValue, originColor.z * currentValue };
						//pObj->CalcUICoord();
					}
				}, [handle]() {
					if (auto pObj = GetSafeUI(handle)) {
						pObj->SetInputLcok(false);
					}
				});
		};
	m_vEventNames.push_back("TxtButtonColorDown");
}

void UIManager::InitializeFunc()
{
	m_FuncMap["Create"] = [](std::string name)
	{
		GET_SINGLE(UIManager)->LoadPrefab(name);
	};
	m_vFuncNames.push_back("Create");

	m_FuncMap["SceneChange"] = [this](std::string name)
	{
		Engine::CGameInstance::Get().ChangeLevel(
			CLevelLoading::Create(m_pDevice, m_pContext, LEVEL::LOGO));
	};
	m_vFuncNames.push_back("SceneChange");

	m_FuncMap["SpellTypeDesCreate"] = [](std::string name)
		{
			GET_SINGLE(UIManager)->LoadPrefab(name);
		};
	m_vFuncNames.push_back("SpellTypeDesCreate");

	m_FuncMap["ClearDeathScene"] = [](std::string name)
		{
			if(std::nullopt != GET_SINGLE(UIManager)->GetUIController())
				E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*GET_SINGLE(UIManager)->GetUIController())->ClearDeathScene();
		};
	m_vFuncNames.push_back("ClearDeathScene");

	m_FuncMap["CreateSpellDragIcon"] = [this](std::string name)
		{
			std::string currentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

			CTextureUI::UIOBJECT_DESC Desc{};
			Desc.sObjectTag = "Select_Image";

			Desc.fSizeX = 150.f;
			Desc.fSizeY = 150.f;

			Desc.fX = g_iWinSizeX * 0.5f;
			Desc.fY = g_iWinSizeY * 0.5f;
			Desc.fAlpha = 1.f;
			Desc.UIType = ETOUI(UI_TYPE::SHORTCUT_ICON);
			Desc.ResWeight = 350;
			Desc.ResTag = name;

			std::optional<CHandle> m_oSelectHandle = E::CGameInstance::Get().AddGameObjectToLayer(currentLevel, "Prototype_GameObject_TextureUI", "Layer_UI_Texture", &Desc);
			CTextureUI* selectUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*m_oSelectHandle);
			selectUI->SetMouseTracking(true);
			selectUI->SetUIType(ETOUI(UI_TYPE::SHORTCUT_ICON));


			if (m_UIController != std::nullopt &&
				nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*m_UIController))
			{
				CUIController* pController = E::CGameInstance::Get().GetGameObjectByHandleT<CUIController>(*m_UIController);
				
				if (name == "TEX_UI_T_spellmeter_ArrestoMomentum_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_ARRESTOMOMENTUM));
				}
				else if (name == "TEX_UI_T_spellmeter_Glacius_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_GLACIUS));
				}
				else if (name == "TEX_UI_T_spellmeter_Levioso_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_LEVIOSO));
				}
				else if (name == "TEX_UI_T_spellmeter_TransformationOverlandOverlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_TRANSFORMATION));
				}
				else if (name == "TEX_UI_T_spellmeter_Accio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_ASSIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Depulso_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DEPULSO));
				}
				else if (name == "TEX_UI_T_spellmeter_Descendo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DESENDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Flipendo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_FLIPENDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Confringo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_CONFRINGO));
				}
				else if (name == "TEX_UI_T_spellmeter_Diffindo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DIFFINDO));
				}
				else if (name == "TEX_UI_T_spellmeter_Expelliarmus_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_EXPELLIARMUS));
				}
				else if (name == "TEX_UI_T_spellmeter_Bombarda_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_BOMBARDA));
				}
				else if (name == "TEX_UI_T_spellmeter_Incendio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_INCENDIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Disillusionment_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_DISILLUSIONMENT));
				}
				else if (name == "TEX_UI_T_spellmeter_Lumos_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_LUMOS));
				}
				else if (name == "TEX_UI_T_spellmeter_Reparo_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_REPARO));
				}
				else if (name == "TEX_UI_T_spellmeter_WingardiumLeviosa_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_WINGARDIUM));
				}
				else if (name == "TEX_UI_T_spellmeter_AvadaKedavra_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_AVADAKEDAVRA));
				}
				else if (name == "TEX_UI_T_spellmeter_Crucio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_CRUCIO));
				}
				else if (name == "TEX_UI_T_spellmeter_Imperio_Overlay")
				{
					pController->SetTargetIcon(ETOUI(SPELL_TYPE::B_IMPERIO));
				}
			}
		};
	m_vFuncNames.push_back("CreateSpellDragIcon");
}

void UIManager::UpdateRootUIHandles()
{
	std::vector<Engine::CUIObject*> uiList;

	if (nullptr == CGameInstance::Get().GetGameObjectLayer("Layer_UI"))
		return;

	rootUIHandles.clear();

	const std::vector<CHandle>* uiHandles = CGameInstance::Get().GetGameObjectLayer("Layer_UI");

	for (auto ui : *uiHandles)
	{
		Engine::CUIObject* checkUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(ui);

		if (checkUI != nullptr)
		{
			if (std::nullopt == checkUI->GetParent())
			{
				rootUIHandles.push_back(ui);
			}
		}
	}
}

std::function<void(CUIObject* pCaller)> UIManager::GetAction(const std::string& actionName)
{
	auto iter = m_EventMap.find(actionName);
	if (iter != m_EventMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Action not found: ");
	return [](CUIObject*) {};
}

std::function<void(std::string text)> UIManager::GetFunc(const std::string& funcName)
{
	auto iter = m_FuncMap.find(funcName);
	if (iter != m_FuncMap.end())
		return iter->second;

	MSG_BOX("[UI Error] Func not found: ");
	return [](std::string text) {};
}

void UIManager::CreateFadeIn(float delay, float playtime)
{
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeIn(hBG, delay, playtime);
}

void UIManager::CreateFadeOut(float delay, float playtime)
{
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeOutDelete(hBG, delay, playtime);
}

void UIManager::CreateFadeInSceneChange(float delay, float playtime, LEVEL level)
{
	CHandle hBG = GET_SINGLE(UIManager)->LoadPrefab("BlackBG").front();
	PlayFadeInChange(hBG, level, delay, playtime);
}

void UIManager::CreateDamageFont(uint32_t damage, CHandle targetMonster, _bool isCritical)
{
	auto* pMonster =
		E::CGameInstance::Get()
		.GetGameObjectByHandleT<CMonster>(targetMonster);

	auto* pCamera =
		E::CGameInstance::Get().GetActiveCamera();

	if (!pMonster || !pCamera || damage == 0)
		return;

	const _float3 worldPosition =
		pMonster->GetHurtBoxPosition();

	const _float2 screenSize =
		E::CGameInstance::Get().GetClientScreenSize();

	const _vector world =
		XMLoadFloat3(&worldPosition);

	const _matrix view = pCamera->GetView();
	const _matrix proj = pCamera->GetProj();

	// 카메라 뒤쪽 검사
	const _vector clipPosition =
		XMVector4Transform(
			XMVectorSet(
				worldPosition.x,
				worldPosition.y,
				worldPosition.z,
				1.f),
			view * proj);

	if (XMVectorGetW(clipPosition) <= 0.f)
		return;

	const _vector projected =
		XMVector3Project(
			world,
			0.f,
			0.f,
			screenSize.x,
			screenSize.y,
			0.f,
			1.f,
			proj,
			view,
			XMMatrixIdentity());

	_float3 screenPosition{};
	XMStoreFloat3(&screenPosition, projected);

	if (screenPosition.z < 0.f || screenPosition.z > 1.f)
		return;

	// 랜덤 오프셋
	static std::mt19937 generator{ std::random_device{}() };
	static std::uniform_real_distribution<float> offsetX{ -25.f, 25.f };
	static std::uniform_real_distribution<float> offsetY{ -35.f, -10.f };

	//auto handles = LoadPrefab("DamageFont");
	//if (handles.empty())
	//	return;



	//const CHandle hDamageFont = handles.front();

	m_CurrentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	CTextUI::TEXT_DESC desc{};

	desc.sObjectTag = "DamageFont";
	desc.Name = "DamageFont";
	desc.fSizeX = 1.5f;
	desc.fSizeY = 1.5f;
	desc.fAlpha = 1.f;
	desc.Text = L"";
	desc.ResWeight = 1;

	std::optional<CHandle> hDamageFont = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextBox", "Layer_UI", &desc);
	auto* pDamageFont = E::CGameInstance::Get()
		.GetGameObjectByHandleT<CTextBox>(*hDamageFont);

	if (!pDamageFont)
		return;

	pDamageFont->SetwText(std::to_wstring(damage));
	pDamageFont->SetPos({
		screenPosition.x + offsetX(generator),
		screenPosition.y + offsetY(generator)
		});

	if (isCritical)
	{
		pDamageFont->SetColor({ 0.72f, 0.64f, 0.40f });
		pDamageFont->SetSize({ 1.5f, 1.5f });
		pDamageFont->SetScaleRatio(1.25f);
	}
	else
	{
		pDamageFont->SetColor({ 1.f, 1.f, 1.f });
		pDamageFont->SetSize({1.2f, 1.2f});
		pDamageFont->SetScaleRatio(1.f);
	}

	pDamageFont->SetAlpha(0.f);
	pDamageFont->CalcUICoord();

	PlayFadeIn(*hDamageFont, 0.f, 0.12f);
	PlayPosUP(*hDamageFont, 0.12f, 0.7f);
	PlayFadeOutDelete(*hDamageFont, 0.3f, 0.65f);
}

std::optional<CHandle> UIManager::RootUIPicking()
{
	std::optional<CHandle> targetHandle = std::nullopt;
	for (auto uiHandle : rootUIHandles)
	{
		if (nullptr == E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle))
			continue;

		CUIObject* pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(uiHandle);
		const UI_INFO& pInfo = pUI->GetUIInfo();

		if (pUI->GetWorldSpace())
			continue;

		if (PtInRect(pInfo, pUI->GetScaleRatio()))
		{
			if (std::nullopt == targetHandle)
				targetHandle = uiHandle;
			else
			{
				if (nullptr != E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle))
				{
					CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<CUIObject>(*targetHandle);
					const UI_INFO& targetInfo = targetUI->GetUIInfo();

					if (pInfo.Weight > targetInfo.Weight)
						targetHandle = uiHandle;
				}
			}
		}
	}

	return targetHandle;
}

_bool UIManager::PtInRect(const UI_INFO& selectInfo, _float scaleRatio)
{
	_float2 mousePos = E::CGameInstance::Get().GetMousePos();

	_float2 origin = { selectInfo.fX, selectInfo.fY };
	_float2 size = { selectInfo.SizeX * scaleRatio, selectInfo.SizeY * scaleRatio };

	if (selectInfo.UIType == ETOUI(UI_TYPE::TEXT))
	{
		size = { selectInfo.SizeX * 50.f, selectInfo.SizeY * 50.f  };
		origin = { selectInfo.fX + size.x * 0.5f, selectInfo.fY + size.y * 0.5f};
	}


	_float2 minPos =
	{
		origin.x - size.x * 0.5f,
		origin.y - size.y * 0.5f
	};

	_float2 maxPos =
	{
		origin.x + size.x * 0.5f,
		origin.y + size.y * 0.5f
	};

	if (mousePos.x >= minPos.x &&
		mousePos.x <= maxPos.x &&
		mousePos.y >= minPos.y &&
		mousePos.y <= maxPos.y)
	{
		return true;
	}

	return false;
}

std::vector<CHandle> UIManager::LoadPrefab(std::string name, std::string g_BasePath)
{
	m_vLoadPrefabRoot.clear();
	uint32_t num = E::CGameInstance::Get().GetCurrentLevelID();
	if (num > 100)
		m_CurrentLevel = "LEVEL_LOADING";
	else
		m_CurrentLevel = _string("LEVEL_") + MagicEnumToStringView(static_cast<LEVEL>(E::CGameInstance::Get().GetCurrentLevelID())).data();

	char path[256] = "";
	strcpy_s(path, sizeof(path), g_BasePath.c_str());
	strcat_s(path, sizeof(path), name.c_str());
	strcat_s(path, sizeof(path), ".json");

	std::ifstream file(path);

	if (!file.is_open())
	{
		/* ---- 광윤 수정 ---- */
		std::string MSGBoxText = "Cannot Open json" + g_BasePath + name + ".json";
		MessageBoxA(NULL, MSGBoxText.c_str(), "System Message", MB_OK);
		/* ------------------- */
		return m_vLoadPrefabRoot;
	}

	nlohmann::ordered_json root;
	file >> root;
	file.close();

	for (const auto& obj : root["UI"])
	{
		LoadUIRecursive(obj, nullptr);
	}

	return m_vLoadPrefabRoot;
}

E::CUIObject* UIManager::LoadUIRecursive(const nlohmann::ordered_json& obj, E::CUIObject* parent)
{
	int uiType = obj["UiType"];

	E::CUIObject* pUI = nullptr;

	E::CUIObject::UIOBJECT_DESC Desc{};
	std::optional<CHandle> uiHandle = std::nullopt;

	Desc.sObjectTag = obj["Name"];

	int EffectType = obj["UI_EFFECT_TYPE"];

	switch (uiType)
	{
	case ETOUI(UI_TYPE::TEXUI):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(uiHandle);
			}
		}
		pUI->SetUIType(ETOUI(UI_TYPE::TEXUI));
		break;
	case ETOUI(UI_TYPE::SHORTCUT_ICON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::SHORTCUT_ICON));
		break;
	case ETOUI(UI_TYPE::DISOLVE):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextureUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextureUI>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::DISOLVE));
		break;
	case ETOUI(UI_TYPE::FLIPBOOK):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_EffectUI", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CEffectUI>(*uiHandle);
		{
			FLIP_INFO& flipInfo = static_cast<CEffectUI*>(pUI)->GetFlipInfo();
			flipInfo.cellsize = obj["CellSize"];
			flipInfo.TotalFrame = obj["TotalFrame"];
			flipInfo.Padding = obj["Padding"];
			flipInfo.Duration = obj["Duration"];
		}

		if (EffectType == ETOUI(UI_EFFECT_TYPE::HOVER))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectHovered(uiHandle);
			}
		}
		else if (EffectType == ETOUI(UI_EFFECT_TYPE::CLICK))
		{
			pUI->SetActive(false);
			if (parent &&
				*parent->GetUIType() == ETOUI(UI_TYPE::BUTTON))
			{
				static_cast<CButton*>(parent)->SetEffectClicked(uiHandle);
			}
		}
		break;
	case ETOUI(UI_TYPE::TEXT):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_TextBox", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CTextBox>(*uiHandle);
		{
			TEXT_INFO& textInfo = static_cast<CTextBox*>(pUI)->GetTextInfo();
			textInfo.Text = StringToWUTF8(obj["Text"]);
		}
		break;
	case ETOUI(UI_TYPE::BUTTON):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::SPELLMETER):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_SpellMeter", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CSpellMeter>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPBAR):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::HPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::HPFILL));
		break;
	case ETOUI(UI_TYPE::LEFTHPFILL):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_HPBar", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CHPBar>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::LEFTHPFILL));
		break;
	case ETOUI(UI_TYPE::MINIMAP):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_MiniMap", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CMiniMap>(*uiHandle);
		pUI->SetUIType(ETOUI(UI_TYPE::MINIMAP));
		break;
	case ETOUI(UI_TYPE::SPELLBTN):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_Button", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CButton>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::GAMEOVERMASK):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_GameOverMask", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CGameOverMask>(*uiHandle);
		break;
	case ETOUI(UI_TYPE::VIDEOOBJ):
		uiHandle = E::CGameInstance::Get().AddGameObjectToLayer(m_CurrentLevel, "Prototype_GameObject_VideoObject", "Layer_UI", &Desc);
		pUI = E::CGameInstance::Get().GetGameObjectByHandleT<CVideoObject>(*uiHandle);
		break;
	default:
		break;
	}

	if (pUI == nullptr)
		return nullptr;

	if (parent == nullptr)
	{
		if (obj.contains("ScaleRatio"))
			pUI->SetScaleRatio(obj["ScaleRatio"]);

		m_vLoadPrefabRoot.push_back(pUI->GetHandle());
	}
		
	if (obj.contains("LocalScaleRatio"))
		pUI->SetLocalScaleRatio(obj["LocalScaleRatio"]);

	UI_INFO& uiInfo = static_cast<CUIObject*>(pUI)->GetUIInfo();

	uiInfo.EffectType = obj["UI_EFFECT_TYPE"];
	uiInfo.Name = obj["Name"];

	uiInfo.SizeX = obj["SizeX"];
	uiInfo.SizeY = obj["SizeY"];

	uiInfo.Alpha = obj["Alpha"];
	uiInfo.AlphaRatio = obj["AlphaRatio"];

	uiInfo.Weight = obj["Weight"];
	uiInfo.WeightOffset = obj["WeightOffset"];

	uiInfo.LocalX = obj["LocalX"];
	uiInfo.LocalY = obj["LocalY"];

	uiInfo.WidthRatioX = obj["WidthRatioX"];
	uiInfo.WidthRatioY = obj["WidthRatioY"];

	uiInfo.Restag = obj["ResTag"];

	uiInfo.Rot = obj["Rot"];
	uiInfo.LocalRot = obj["LocalRot"];

	auto color = obj["Color"];
	uiInfo.Color = { color[0], color[1], color[2] };

	UI_EVENT& eventInfo = pUI->GetUIEvent();

	eventInfo.ClickFunc = obj.value("ClickFunc", "");
	eventInfo.ClickAction = obj.value("ClickAction", "");
	eventInfo.EnterAction = obj.value("EnterAction", "");
	eventInfo.ExitAction = obj.value("ExitAction", "");
	eventInfo.AppearAction = obj.value("AppearAction", "");
	eventInfo.DisappearAction = obj.value("DisappearAction", "");

	auto bindAction = [](const std::string& actionStr, std::function<void(CUIObject*)>& targetFunc) {
		if (!actionStr.empty() && actionStr != "None") {
			targetFunc = GET_SINGLE(UIManager)->GetAction(actionStr);
		}
	};

	bindAction(eventInfo.ClickAction, pUI->OnClicked);
	bindAction(eventInfo.EnterAction, pUI->OnHoverEnter);
	bindAction(eventInfo.ExitAction, pUI->OnHoverExit);
	bindAction(eventInfo.AppearAction, pUI->Appear);
	bindAction(eventInfo.DisappearAction, pUI->Disappear);

	if(!eventInfo.ClickFunc.empty() && eventInfo.ClickFunc != "None")
		pUI->OnClickedAction = GET_SINGLE(UIManager)->GetFunc(eventInfo.ClickFunc);


	if (parent == nullptr)
	{
		m_rootHandle = uiHandle;

		uiInfo.fX = obj["X"];
		uiInfo.fY = obj["Y"];
	}
	else
	{
		pUI->SetParent(parent->GetHandle());
		parent->AddChildren(pUI->GetHandle());

		uiInfo.LocalX = obj["LocalX"];
		uiInfo.LocalY = obj["LocalY"];
	}

	// 부모 기준으로 다시 계산
	if (obj.contains("IsWorldSpace"))
	{
		bool isWorldSpace = obj["IsWorldSpace"];
		pUI->SetWorldSpace(isWorldSpace);

		if (isWorldSpace && obj.contains("WorldPos"))
		{
			auto posArr = obj["WorldPos"];
			_float3 loadedPos = { posArr[0], posArr[1], posArr[2] };

			// Transform에 3D 월드 좌표 적용 (XMLoadFloat3 사용)
			pUI->GetTransform().SetPosition(XMLoadFloat3(&loadedPos));
		}

		if(!isWorldSpace)
			pUI->CalcUICoord();
	}
	else
		pUI->CalcUICoord();
	

	for (const auto& child : obj["Children"])
	{
		LoadUIRecursive(child, pUI);
	}

	return pUI;
}

void UIManager::DeleteUIRecursive(std::optional<CHandle> targetHandle)
{
	Engine::CUIObject* targetUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetHandle);

	std::vector<CHandle>  childHandles = targetUI->GetChildren();

	for (auto childHandle : childHandles)
	{
		DeleteUIRecursive(childHandle);
	}

	if (targetUI->GetParent())
	{
		Engine::CUIObject* parentUI = E::CGameInstance::Get().GetGameObjectByHandleT<Engine::CUIObject>(*targetUI->GetParent());

		if (nullptr != parentUI)
			parentUI->DeleteChild(targetUI->GetHandle());
	}

	targetUI->SetPendingDestroyCascade();

	return;
}

void UIManager::PlayFadeOutDelete(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(1.f, 0.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle]() {
			if (auto pObj = GetSafeUI(pHandle)) GET_SINGLE(UIManager)->DeleteUIRecursive(pHandle);
			}, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayScaleDown(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	_float scale = pBtn->GetSize().x;

	pTween->PlayTween(scale, scale * 0.5f, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->GetUIInfo().SizeX = currentValue;
				pObj->CalcUICoord();
			}
		},nullptr, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayPosUP(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	_float2 originPos = pBtn->GetPos();

	pTween->PlayTween(originPos.y, originPos.y - 20.f, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->GetUIInfo().fY = currentValue;
				pObj->CalcUICoord();
			}
		}, nullptr, EEaseType::Linear, delay);
}

void UIManager::PlayFadeIn(CHandle pHandle, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();
	_float scaleRatio = pBtn->GetScaleRatio();

	pTween->PlayTween(0.8f, scaleRatio, playtime,
		[pHandle](float currentValue) {
			if (auto pObj = GetSafeUI(pHandle))
			{
				pObj->SetScaleRatio(currentValue);
				pObj->CalcUICoord();
			}
		}, nullptr, EEaseType::EaseOutQuad, delay);

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, nullptr, EEaseType::EaseOutQuad, delay);
}

void UIManager::PlayFadeInChange(CHandle pHandle, LEVEL level, float delay, float playtime)
{
	CUIObject* pBtn = SafeGetOBJ(pHandle);
	auto pTween = pBtn->GetTweenCom();

	pBtn->SetInputLcok(true);

	_float Alpah = pBtn->GetAlpha();

	pTween->PlayTween(0.f, 1.f, playtime,
		[pBtn](float currentValue) {
			pBtn->SetAlpha(currentValue);
		}, [pHandle, level]() {
			Engine::CGameInstance::Get().ChangeLevel(
				CLevelLoading::Create(E::CGameInstance::Get().GetGraphicDevice(), E::CGameInstance::Get().GetGraphicDeviceContext(), level));
			}, EEaseType::EaseOutQuad, delay);
}

