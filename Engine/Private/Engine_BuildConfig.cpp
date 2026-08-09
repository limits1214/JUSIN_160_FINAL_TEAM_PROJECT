#include "pch.h"
#include "Engine_BuildConfig.h"

extern "C" ENGINE_DLL std::uint64_t Engine_GetBuildSignature() noexcept
{
	return Engine::EXPECTED_ENGINE_BUILD_SIGNATURE;
}
