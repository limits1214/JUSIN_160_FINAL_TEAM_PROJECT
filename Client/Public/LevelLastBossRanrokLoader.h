#pragma once
#include "Engine_Defines.h"
#include "Level_Defines.h"

NS_BEGIN(Client)
class CLevelLastBossRanrokLoader
{
public:
	static constexpr const char* MAP_PATH = "./Resources/json/MapSaved/RanrokBoss";

	static std::future<bool> Load();
	static std::future<bool> UnLoad();

private:
	static HRESULT LoadPlayer_InWorker();
	static HRESULT LoadPlayerCape_InWorker();
	static HRESULT MonsterLoad_InWorker();
	static _bool UILoad_InWorker();


private:
	constexpr static LEVEL CURR_LEVEL = LEVEL::LAST_BOSS_RANROK;
};
NS_END
