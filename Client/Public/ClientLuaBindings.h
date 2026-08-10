#pragma once

#include "Client_Defines.h"

NS_BEGIN(Engine)
class CComCharacterMoveIntent;
class CGameObject;
class CLuaManager;
NS_END

NS_BEGIN(Client)

class CClientLuaBindings final
{
private:
	CClientLuaBindings() = delete;
	~CClientLuaBindings() = delete;

public:
	static HRESULT Register();

private:
	static HRESULT RegisterObjectSpawnFactories(E::CLuaManager& LuaManager);
	static HRESULT RegisterComponentAttachFactories(E::CLuaManager& LuaManager);
	static void BindClientAPIs(sol::state& Lua);
	static void BindSoundAPI(sol::state& Lua);

	static E::CGameObject* ResolveObject(const CHandle& hObject);
	static E::CComCharacterMoveIntent* ResolveMoveIntent(
		const CHandle& hObject,
		std::string_view sComponentTag);
};

NS_END
