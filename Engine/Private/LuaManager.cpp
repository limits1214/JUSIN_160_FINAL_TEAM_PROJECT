#include "pch.h"
#include "LuaManager.h"

#include "GameInstance.h"
#include "LuaScriptInstance.h"
#include "LuaWatcher.h"
#include "ResLuaScript.h"

NS_USING(Engine)

CLuaManager::CLuaManager() = default;
CLuaManager::~CLuaManager() = default;

void CLuaManager::UpdateGUI()
{
	if (!ImGui::Begin("Lua Manager"))
	{
		ImGui::End();
		return;
	}

	std::erase_if(m_ScriptInstances,
		[](const WPtr<CLuaScriptInstance>& pWeakInstance)
		{
			return pWeakInstance.expired();
		});

	ImGui::Text("Live Script Instances: %zu", m_ScriptInstances.size());
	for (const auto& pWeakInstance : m_ScriptInstances)
	{
		if (const auto pInstance = pWeakInstance.lock())
		{
			ImGui::BulletText("%s [%s]",
				pInstance->GetDebugName().c_str(),
				pInstance->IsLoaded() ? "Loaded" : "Not Loaded");
		}
	}

	ImGui::End();
}

void CLuaManager::Update(_float fTimeDelta)
{
	UpdateHotReload();
}

SPtr<CLuaScriptInstance> CLuaManager::CreateScriptInstance(
	const SPtr<CResLuaScript>& pScript,
	const _string& sDebugName)
{
	CLuaScriptInstance::DESC Desc{};
	Desc.pScript = pScript;
	Desc.sDebugName = sDebugName;

	auto pInstance = CLuaScriptInstance::Create(*this, Desc);
	if (!pInstance)
		return nullptr;

	std::erase_if(m_ScriptInstances,
		[](const WPtr<CLuaScriptInstance>& pWeakInstance)
		{
			return pWeakInstance.expired();
		});

	m_ScriptInstances.emplace_back(pInstance);
	return pInstance;
}

SPtr<CLuaScriptInstance> CLuaManager::CreateScriptInstanceByPath(
	const _string& sScriptPath,
	const _string& sDebugName)
{
	if (sScriptPath.empty())
		return nullptr;

	const _string sNormalizedPath =
		std::filesystem::path{ sScriptPath }.lexically_normal().generic_string();
	auto pScript = CGameInstance::Get().GetOrCreateResourceByPath<CResLuaScript>(
		sNormalizedPath,
		[&]()
		{
			return CResLuaScript::CreateAndLoad(sNormalizedPath);
		});
	if (!pScript)
		return nullptr;

	const CResource::STATE eState = pScript->GetState();
	if (eState == CResource::STATE::LOADING)
		return nullptr;
	if (eState != CResource::STATE::LOADED && FAILED(pScript->Load()))
		return nullptr;

	return CreateScriptInstance(pScript, sDebugName);
}

_bool CLuaManager::RegisterObjectSpawnFactory(
	std::string_view sSpawnType,
	LUA_OBJECT_SPAWN_FACTORY Factory)
{
	if (sSpawnType.empty() || !Factory)
		return false;

	m_ObjectSpawnFactories.insert_or_assign(
		std::string{ sSpawnType },
		std::move(Factory));
	return true;
}

_bool CLuaManager::UnregisterObjectSpawnFactory(std::string_view sSpawnType)
{
	return m_ObjectSpawnFactories.erase(std::string{ sSpawnType }) > 0;
}

_bool CLuaManager::RegisterComponentAttachFactory(
	std::string_view sAttachType,
	LUA_COMPONENT_ATTACH_FACTORY Factory)
{
	if (sAttachType.empty() || !Factory)
		return false;

	m_ComponentAttachFactories.insert_or_assign(
		std::string{ sAttachType },
		std::move(Factory));
	return true;
}

_bool CLuaManager::UnregisterComponentAttachFactory(std::string_view sAttachType)
{
	return m_ComponentAttachFactories.erase(std::string{ sAttachType }) > 0;
}

HRESULT CLuaManager::Initialize()
{
#ifdef _DEBUG
	m_pLuaWatcher = CLuaWatcher::Create();
#endif

	m_Lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::math,
		sol::lib::table,
		sol::lib::utf8);

	if (FAILED(InitializePrintBinding()))
		return E_FAIL;
	if (FAILED(InitializeValueTypeBindings()))
		return E_FAIL;
	if (FAILED(InitializeConstantBindings()))
		return E_FAIL;
	if (FAILED(InitializeEngineBindings()))
		return E_FAIL;

	return S_OK;
}

sol::table CLuaManager::CreateTable()
{
	return m_Lua.create_table();
}

HRESULT CLuaManager::ExecuteModule(
	const std::string& sSource,
	const std::string& sChunkName,
	const sol::table& Context,
	sol::environment& OutEnvironment,
	sol::table& OutExports)
{
	try
	{
		std::string FormattedChunkName = sChunkName;
		std::replace(FormattedChunkName.begin(), FormattedChunkName.end(), '\\', '/');
		if (!FormattedChunkName.empty() && FormattedChunkName.front() != '@')
			FormattedChunkName.insert(FormattedChunkName.begin(), '@');

		sol::environment NewEnvironment{ m_Lua, sol::create, m_Lua.globals() };
		NewEnvironment["Context"] = Context;
		NewEnvironment["__ScriptPath"] = sChunkName;

		sol::protected_function_result Result = m_Lua.safe_script(
			sSource,
			NewEnvironment,
			sol::script_pass_on_error,
			FormattedChunkName);

		if (!Result.valid())
		{
			sol::error Error = Result;
			OutputDebugStringA(("[Lua Module Execute Error] " +
				std::string{ Error.what() } + "\n").c_str());
			return E_FAIL;
		}

		sol::object ReturnedObject = Result.get<sol::object>();
		if (!ReturnedObject.valid() || ReturnedObject.get_type() != sol::type::table)
		{
			OutputDebugStringA(("[Lua Module Error] Script must return a table: " +
				sChunkName + "\n").c_str());
			return E_FAIL;
		}

		OutEnvironment = std::move(NewEnvironment);
		OutExports = ReturnedObject.as<sol::table>();
	}
	catch (const std::exception& Exception)
	{
		OutputDebugStringA(("[C++ Exception in Lua Module] " +
			std::string{ Exception.what() } + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLuaManager::Execute(
	const std::string& script,
	const std::string& chunkName)
{
	try
	{
		std::string FormattedChunkName = chunkName;
		std::replace(FormattedChunkName.begin(), FormattedChunkName.end(), '\\', '/');
		if (!FormattedChunkName.empty() && FormattedChunkName.front() != '@')
			FormattedChunkName.insert(FormattedChunkName.begin(), '@');

		sol::protected_function_result Result = m_Lua.safe_script(
			script,
			sol::script_pass_on_error,
			FormattedChunkName);

		if (!Result.valid())
		{
			sol::error Error = Result;
			OutputDebugStringA(("[Lua Execute Error] " +
				std::string{ Error.what() } + "\n").c_str());
			return E_FAIL;
		}
	}
	catch (const std::exception& Exception)
	{
		OutputDebugStringA(("[C++ Exception in Lua Execute] " +
			std::string{ Exception.what() } + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLuaManager::Compile(const std::string& script)
{
	sol::load_result Result = m_Lua.load(script);
	if (Result.valid())
		return S_OK;

	sol::error Error = Result;
	const std::string Message = "[Lua Compile Error] Status(" +
		std::to_string(static_cast<int>(Result.status())) + "): " +
		Error.what() + "\n";
	OutputDebugStringA(Message.c_str());
	return E_FAIL;
}

void CLuaManager::UpdateHotReload()
{
	if (!m_pLuaWatcher)
		return;

	namespace fs = std::filesystem;
	const auto ChangedFiles = m_pLuaWatcher->GetChangedFilesAndClear();
	for (const auto& FullFileName : ChangedFiles)
	{
		std::string SourceString = FullFileName;
		std::replace(SourceString.begin(), SourceString.end(), '\\', '/');

		constexpr std::string_view Keyword = "LuaFiles/";
		const size_t Position = SourceString.find(Keyword);
		if (Position == std::string::npos)
			continue;

		const std::string RelativePath = SourceString.substr(Position);
		const fs::path SourcePath = SourceString;
		const fs::path DestinationPath = fs::path("./") / RelativePath;

		try
		{
			fs::create_directories(DestinationPath.parent_path());
			fs::copy_file(
				SourcePath,
				DestinationPath,
				fs::copy_options::overwrite_existing);
		}
		catch (const fs::filesystem_error&)
		{
			OutputDebugStringA(("[Lua] File copy failed: " + SourceString + "\n").c_str());
			continue;
		}

		std::string TargetResourcePath = DestinationPath.generic_string();
		OnFileChanged(TargetResourcePath);
	}
}

void CLuaManager::OnFileChanged(const std::string& path)
{
	const auto Resources = CGameInstance::Get().GetResourcesByPath(path);
	_bool bResourceReloaded{};

	for (const auto& pResource : Resources)
	{
		auto pLuaScript = Cast<CResLuaScript>(pResource);
		if (pLuaScript && SUCCEEDED(pLuaScript->Reload()))
			bResourceReloaded = true;
	}

	if (!bResourceReloaded)
		return;

	auto NormalizePath = [](std::string_view sPath)
	{
		std::string NormalizedPath =
			std::filesystem::path{ sPath }.lexically_normal().generic_string();

		std::ranges::transform(
			NormalizedPath,
			NormalizedPath.begin(),
			[](unsigned char Character)
			{
				return static_cast<char>(std::tolower(Character));
			});

		return NormalizedPath;
	};

	const std::string ChangedPath = NormalizePath(path);
	std::erase_if(m_ScriptInstances,
		[&](const WPtr<CLuaScriptInstance>& pWeakInstance)
		{
			auto pInstance = pWeakInstance.lock();
			if (!pInstance)
				return true;

			if (NormalizePath(pInstance->GetScriptPath()) == ChangedPath &&
				FAILED(pInstance->Reload()))
			{
				OutputDebugStringA(("[Lua] Script instance reload failed: " +
					pInstance->GetScriptPath() + "\n").c_str());
			}

			return false;
		});
}

UPtr<CLuaManager> CLuaManager::Create()
{
	auto pInstance = UPtr<CLuaManager>(new CLuaManager{});
	if (FAILED(pInstance->Initialize()))
		return nullptr;

	return pInstance;
}

void CLuaManager::Free()
{
	for (const auto& pWeakInstance : m_ScriptInstances)
	{
		if (auto pInstance = pWeakInstance.lock())
			pInstance->Invalidate();
	}

	m_ScriptInstances.clear();
	m_ObjectSpawnFactories.clear();
	m_ComponentAttachFactories.clear();
	CEngineBase::Free();
}
