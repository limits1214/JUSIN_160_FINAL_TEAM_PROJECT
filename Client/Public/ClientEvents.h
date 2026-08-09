#pragma once
#include "Client_Defines.h"
#include "UI_Enums.h"

NS_BEGIN(Client)


struct FRequestPlayerCameraShake
{
	// 0~1
	_float fIntensity{ 1.f };

	_float fDuration{ 0.3f };

	// 초당 흔들리는 횟수
	_float fFrequency{ 18.f };
};

struct FPlayerDied
{
	CHandle hPlayer{};
	_float fLevelBgmFadeDuration{ 1.f };
};

struct FAcientMagicStart {};

struct FQuestUIGroupChanged
{
	QUEST_UI_GROUP Group{ QUEST_UI_GROUP::NONE };
	_bool Active{ false };
};

NS_END
