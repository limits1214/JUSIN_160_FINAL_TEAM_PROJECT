#include "pch.h"
#include "ClientLuaBindings.h"

#include "ComCharacterMoveIntent.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "LuaManager.h"
#include "SoundManager.h"

NS_USING(Client)

HRESULT CClientLuaBindings::Register()
{
	auto* pLuaManager = CGameInstance::Get().GetLuaManager();
	if (!pLuaManager)
		return E_FAIL;

	if (FAILED(RegisterObjectSpawnFactories(*pLuaManager)))
		return E_FAIL;

	if (FAILED(RegisterComponentAttachFactories(*pLuaManager)))
		return E_FAIL;

	pLuaManager->RegisterBindingExtension([](sol::state& Lua)
		{
			BindClientAPIs(Lua);
		});

	return S_OK;
}

HRESULT CClientLuaBindings::RegisterObjectSpawnFactories(CLuaManager& LuaManager)
{
	const _bool bRegistered = LuaManager.RegisterObjectSpawnFactory(
		"LuaTestObject",
		[](std::string_view sLayer, const sol::table& Parameters)
			-> std::optional<CHandle>
		{
			CGameObject::GAMEOBJECT_DESC Desc{};
			Desc.sObjectTag = "LuaTestObject";

			const sol::optional<std::string> sObjectTag = Parameters["tag"];
			if (sObjectTag && !sObjectTag->empty())
				Desc.sObjectTag = *sObjectTag;

			return CGameInstance::Get().AddGameObjectToLayer(
				LEVEL::TERRAIN,
				PROTO_GAMEOBJECT::Prototype_GameObject_LuaTest,
				sLayer,
				&Desc);
		});

	return bRegistered ? S_OK : E_FAIL;
}

HRESULT CClientLuaBindings::RegisterComponentAttachFactories(CLuaManager& LuaManager)
{
	const _bool bRegistered = LuaManager.RegisterComponentAttachFactory(
		"CharacterMoveIntent",
		[](CGameObject& Owner, std::string_view sComponentTag, const sol::table&)
		{
			CComCharacterMoveIntent::DESC Desc{};
			return SUCCEEDED(Owner.AddComponentFromProto<CComCharacterMoveIntent>(
				ES_EngineProtoMajorType::PERMANENT,
				ES_EngineProtoComponent::Prototype_Component_ComCharacterMoveIntent,
				StringID{ std::string{ sComponentTag } },
				&Desc));
		});

	return bRegistered ? S_OK : E_FAIL;
}

void CClientLuaBindings::BindClientAPIs(sol::state& Lua)
{
	sol::table MoveIntent = Lua.create_named_table("MoveIntent");

	MoveIntent.set_function("SetMove",
		[](const CHandle& hObject, const std::string& sComponentTag,
			const _float3& vDirection, _float fSpeed)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->SetMoveIntent(vDirection, fSpeed);
			return true;
		});
	MoveIntent.set_function("ClearMove",
		[](const CHandle& hObject, const std::string& sComponentTag)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->ClearMoveIntent();
			return true;
		});
	MoveIntent.set_function("SetFacing",
		[](const CHandle& hObject, const std::string& sComponentTag,
			const _float3& vDirection, _float fTurnSpeed)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->SetFacingIntent(vDirection, fTurnSpeed);
			return true;
		});
	MoveIntent.set_function("SetFacingImmediate",
		[](const CHandle& hObject, const std::string& sComponentTag,
			const _float3& vDirection)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->SetFacingIntentImmediate(vDirection);
			return true;
		});
	MoveIntent.set_function("ClearFacing",
		[](const CHandle& hObject, const std::string& sComponentTag)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->ClearFacingIntent();
			return true;
		});
	MoveIntent.set_function("AddExternalDisplacement",
		[](const CHandle& hObject, const std::string& sComponentTag,
			const _float3& vDisplacement)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->AddExternalDisplacement(vDisplacement);
			return true;
		});
	MoveIntent.set_function("RequestJump",
		[](const CHandle& hObject, const std::string& sComponentTag)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->RequestJump();
			return true;
		});
	MoveIntent.set_function("RequestWarp",
		[](const CHandle& hObject, const std::string& sComponentTag,
			const _float3& vPosition)
		{
			auto* pMoveIntent = ResolveMoveIntent(hObject, sComponentTag);
			if (!pMoveIntent)
				return false;

			pMoveIntent->RequestWarp(vPosition);
			return true;
		});

	BindSoundAPI(Lua);
}

void CClientLuaBindings::BindSoundAPI(sol::state& Lua)
{
	sol::table SoundBus = Lua.create_named_table("SoundBus");
	SoundBus["MASTER"] = -1;
	SoundBus["BGM"] = ETOI(SOUND_BUS::BGM);
	SoundBus["SFX"] = ETOI(SOUND_BUS::SFX);
	SoundBus["VOICE"] = ETOI(SOUND_BUS::VOICE);
	SoundBus["UI"] = ETOI(SOUND_BUS::UI);
	SoundBus["AMBIENCE"] = ETOI(SOUND_BUS::AMBIENCE);

	sol::table SoundRolloff = Lua.create_named_table("SoundRolloff");
	SoundRolloff["INVERSE"] = ETOI(SOUND_3D_ROLLOFF::INVERSE);
	SoundRolloff["LINEAR"] = ETOI(SOUND_3D_ROLLOFF::LINEAR);

	auto ResolveBus = [](int32_t iBus) -> std::optional<SOUND_BUS_ID>
	{
		if (iBus == -1)
			return SOUND_MASTER_BUS_ID;

		const auto eBus = magic_enum::enum_cast<SOUND_BUS>(iBus);
		if (!eBus || *eBus == SOUND_BUS::END)
			return std::nullopt;

		return SOUND_BUS_ID{ std::string{ magic_enum::enum_name(*eBus) } };
	};

	auto ReadPlayDesc = [ResolveBus](const sol::table& Options)
		-> std::optional<SOUND_PLAY_DESC>
	{
		SOUND_PLAY_DESC Desc{};

		const sol::optional<int32_t> iBus = Options["bus"];
		if (iBus)
		{
			const auto sBusID = ResolveBus(*iBus);
			if (!sBusID)
				return std::nullopt;
			Desc.sBusID = *sBusID;
		}

		const sol::optional<_float> fVolume = Options["volume"];
		const sol::optional<_float> fPitch = Options["pitch"];
		const sol::optional<_float> fFadeInDuration = Options["fadeIn"];
		const sol::optional<int32_t> iPriority = Options["priority"];
		const sol::optional<_bool> bLoop = Options["loop"];
		const sol::optional<_bool> bStartPaused = Options["startPaused"];

		if (fVolume)
			Desc.fVolume = *fVolume;
		if (fPitch)
			Desc.fPitch = *fPitch;
		if (fFadeInDuration)
			Desc.fFadeInDuration = *fFadeInDuration;
		if (iPriority)
			Desc.iPriority = *iPriority;
		if (bLoop)
			Desc.bLoop = *bLoop;
		if (bStartPaused)
			Desc.bStartPaused = *bStartPaused;

		return Desc;
	};

	auto Read3DDesc = [](const _float3& vPosition, const sol::table& Options)
		-> std::optional<SOUND_3D_DESC>
	{
		SOUND_3D_DESC Desc{};
		Desc.vPosition = vPosition;

		const sol::optional<_float3> vVelocity = Options["velocity"];
		const sol::optional<_float> fMinDistance = Options["minDistance"];
		const sol::optional<_float> fMaxDistance = Options["maxDistance"];
		const sol::optional<int32_t> iRolloff = Options["rolloff"];

		if (vVelocity)
			Desc.vVelocity = *vVelocity;
		if (fMinDistance)
			Desc.fMinDistance = *fMinDistance;
		if (fMaxDistance)
			Desc.fMaxDistance = *fMaxDistance;
		if (iRolloff)
		{
			const auto eRolloff = magic_enum::enum_cast<SOUND_3D_ROLLOFF>(*iRolloff);
			if (!eRolloff)
				return std::nullopt;
			Desc.eRolloff = *eRolloff;
		}

		if (Desc.fMinDistance < 0.f || Desc.fMaxDistance <= Desc.fMinDistance)
			return std::nullopt;

		return Desc;
	};

	auto ReadLoadType = [](const sol::table& Options)
	{
		const sol::optional<_bool> bStream = Options["stream"];
		return bStream && *bStream
			? SOUND_LOAD_TYPE::STREAM
			: SOUND_LOAD_TYPE::SAMPLE;
	};

	sol::table Sound = Lua.create_named_table("Sound");
	Sound["InvalidID"] = INVALID_SOUND_ID;

	Sound.set_function("Preload",
		sol::overload(
			[](const std::string& sPath)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				return pSoundManager && !sPath.empty() && pSoundManager->Preload(sPath);
			},
			[ReadLoadType](const std::string& sPath, const sol::table& Options)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				return pSoundManager && !sPath.empty() &&
					pSoundManager->Preload(sPath, ReadLoadType(Options));
			}));

	Sound.set_function("Play2D",
		sol::overload(
			[](const std::string& sPath)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				return pSoundManager && !sPath.empty()
					? pSoundManager->Play2D(sPath)
					: INVALID_SOUND_ID;
			},
			[ReadPlayDesc, ReadLoadType](
				const std::string& sPath,
				const sol::table& Options)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				const auto Desc = ReadPlayDesc(Options);
				return pSoundManager && !sPath.empty() && Desc
					? pSoundManager->Play2D(sPath, *Desc, ReadLoadType(Options))
					: INVALID_SOUND_ID;
			}));

	Sound.set_function("Play3D",
		sol::overload(
			[](const std::string& sPath, const _float3& vPosition)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				SOUND_3D_DESC Desc{};
				Desc.vPosition = vPosition;
				return pSoundManager && !sPath.empty()
					? pSoundManager->Play3D(sPath, Desc)
					: INVALID_SOUND_ID;
			},
			[ReadPlayDesc, Read3DDesc, ReadLoadType](
				const std::string& sPath,
				const _float3& vPosition,
				const sol::table& Options)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				const auto PlayDesc = ReadPlayDesc(Options);
				const auto Desc3D = Read3DDesc(vPosition, Options);
				return pSoundManager && !sPath.empty() && PlayDesc && Desc3D
					? pSoundManager->Play3D(
						sPath, *Desc3D, *PlayDesc, ReadLoadType(Options))
					: INVALID_SOUND_ID;
			}));

	Sound.set_function("Stop", [](SOUND_ID iSoundID)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->Stop(iSoundID);
		});
	Sound.set_function("SetPaused", [](SOUND_ID iSoundID, _bool bPaused)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->SetPaused(iSoundID, bPaused);
		});
	Sound.set_function("SetVolume", [](SOUND_ID iSoundID, _float fVolume)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->SetVolume(iSoundID, fVolume);
		});
	Sound.set_function("SetPitch", [](SOUND_ID iSoundID, _float fPitch)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->SetPitch(iSoundID, fPitch);
		});
	Sound.set_function("FadeTo",
		[](SOUND_ID iSoundID, _float fTargetVolume, _float fDuration)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager &&
				pSoundManager->FadeTo(iSoundID, fTargetVolume, fDuration);
		});
	Sound.set_function("FadeOutAndStop", [](SOUND_ID iSoundID, _float fDuration)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->FadeOutAndStop(iSoundID, fDuration);
		});
	Sound.set_function("Set3DAttributes",
		sol::overload(
			[](SOUND_ID iSoundID, const _float3& vPosition)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				return pSoundManager &&
					pSoundManager->Set3DAttributes(iSoundID, vPosition);
			},
			[](SOUND_ID iSoundID, const _float3& vPosition, const _float3& vVelocity)
			{
				auto* pSoundManager = CGameInstance::Get().GetSoundManager();
				return pSoundManager &&
					pSoundManager->Set3DAttributes(iSoundID, vPosition, vVelocity);
			}));
	Sound.set_function("Set3DMinMaxDistance",
		[](SOUND_ID iSoundID, _float fMinDistance, _float fMaxDistance)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->Set3DMinMaxDistance(
				iSoundID, fMinDistance, fMaxDistance);
		});
	Sound.set_function("IsValid", [](SOUND_ID iSoundID)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->IsValidSound(iSoundID);
		});
	Sound.set_function("IsPlaying", [](SOUND_ID iSoundID)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->IsPlaying(iSoundID);
		});
	Sound.set_function("IsPaused", [](SOUND_ID iSoundID)
		{
			auto* pSoundManager = CGameInstance::Get().GetSoundManager();
			return pSoundManager && pSoundManager->IsPaused(iSoundID);
		});
}

CGameObject* CClientLuaBindings::ResolveObject(const CHandle& hObject)
{
	CGameObject* pObject = CGameInstance::Get().GetGameObjectByHandle(hObject);
	if (!pObject || pObject->GetPendingDestroy())
		return nullptr;

	return pObject;
}

CComCharacterMoveIntent* CClientLuaBindings::ResolveMoveIntent(
	const CHandle& hObject,
	std::string_view sComponentTag)
{
	CGameObject* pObject = ResolveObject(hObject);
	if (!pObject || sComponentTag.empty())
		return nullptr;

	return pObject->GetComponent<CComCharacterMoveIntent>(
		StringID{ std::string{ sComponentTag } });
}
