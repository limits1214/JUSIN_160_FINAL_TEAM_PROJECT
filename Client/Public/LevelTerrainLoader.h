#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Client)

class CLevelTerrainLoader
{
public:
	static std::future<bool> Load();
	static std::future<bool> UnLoad();

private:
	static _bool UILoad();
	static HRESULT MonsterLoad_InWorker();
};

NS_END
