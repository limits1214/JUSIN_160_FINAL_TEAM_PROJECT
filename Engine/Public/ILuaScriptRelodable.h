#pragma once
#include "Engine_Defines.h"

NS_BEGIN(Engine)

class ENGINE_DLL ILuaScriptRelodable
{
public:
	virtual ~ILuaScriptRelodable() = default;
	virtual void LuaScriptRelod() = 0;
};

NS_END
