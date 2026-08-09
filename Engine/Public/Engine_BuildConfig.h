#pragma once

#include "Engine_Macro.h"

#include <cstdint>
#include <Windows.h>

namespace Engine
{
	enum class ENGINE_BUILD_CONFIGURATION : std::uint32_t
	{
		DEBUG = 1,
		RELEASE = 2
	};

	// Engine 공개 ABI에 영향을 주는 변경이 생기면 이 값을 증가시킨다.
	inline constexpr std::uint32_t ENGINE_ABI_VERSION = 1;

#ifdef _DEBUG
	inline constexpr ENGINE_BUILD_CONFIGURATION CURRENT_BUILD_CONFIGURATION =
		ENGINE_BUILD_CONFIGURATION::DEBUG;
#else
	inline constexpr ENGINE_BUILD_CONFIGURATION CURRENT_BUILD_CONFIGURATION =
		ENGINE_BUILD_CONFIGURATION::RELEASE;
#endif

	constexpr std::uint64_t MakeEngineBuildSignature(
		std::uint32_t iABIVersion,
		ENGINE_BUILD_CONFIGURATION eConfiguration) noexcept
	{
		return (static_cast<std::uint64_t>(iABIVersion) << 32) |
			static_cast<std::uint32_t>(eConfiguration);
	}

	inline constexpr std::uint64_t EXPECTED_ENGINE_BUILD_SIGNATURE =
		MakeEngineBuildSignature(
			ENGINE_ABI_VERSION,
			CURRENT_BUILD_CONFIGURATION);
}

// 빌드 구성이 달라도 안전하게 호출할 수 있도록 STL을 사용하지 않는 C ABI로 노출한다.
extern "C" ENGINE_DLL std::uint64_t Engine_GetBuildSignature() noexcept;

namespace Engine
{
	inline const wchar_t* GetEngineBuildConfigurationName(
		ENGINE_BUILD_CONFIGURATION eConfiguration) noexcept
	{
		switch (eConfiguration)
		{
		case ENGINE_BUILD_CONFIGURATION::DEBUG:
			return L"Debug";

		case ENGINE_BUILD_CONFIGURATION::RELEASE:
			return L"Release";

		default:
			return L"Unknown";
		}
	}

	inline void ValidateEngineBuildCompatibility()
	{
		const std::uint64_t iLoadedSignature = Engine_GetBuildSignature();
		if (iLoadedSignature == EXPECTED_ENGINE_BUILD_SIGNATURE)
			return;

		const auto eLoadedConfiguration = static_cast<ENGINE_BUILD_CONFIGURATION>(
			static_cast<std::uint32_t>(iLoadedSignature));
		const std::uint32_t iLoadedABIVersion =
			static_cast<std::uint32_t>(iLoadedSignature >> 32);

		wchar_t szMessage[512]{};
		swprintf_s(
			szMessage,
			L"Client와 Engine.dll의 빌드 구성이 일치하지 않습니다.\n\n"
			L"Client: %s (ABI %u)\n"
			L"Engine.dll: %s (ABI %u)\n\n"
			L"동일한 구성으로 Engine과 실행 프로젝트를 다시 빌드하세요.",
			GetEngineBuildConfigurationName(CURRENT_BUILD_CONFIGURATION),
			ENGINE_ABI_VERSION,
			GetEngineBuildConfigurationName(eLoadedConfiguration),
			iLoadedABIVersion);

		MessageBoxW(
			nullptr,
			szMessage,
			L"Engine Build Mismatch",
			MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);

		RaiseFailFastException(nullptr, nullptr, 0);
	}
}
