#pragma once

#include "psapi.h"

namespace Engine
{
	inline void LogMemoryUsage(const char* tag = "")
	{
		PROCESS_MEMORY_COUNTERS_EX pmc{};
		if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
		{
			char buf[512];
			sprintf_s(buf,
				"[Memory%s%s] WorkingSet: %.2f MB | PeakWorkingSet: %.2f MB | PrivateUsage: %.2f MB | PageFile: %.2f MB\n",
				tag[0] ? " - " : "", tag,
				pmc.WorkingSetSize / (1024.0 * 1024.0),
				pmc.PeakWorkingSetSize / (1024.0 * 1024.0),
				pmc.PrivateUsage / (1024.0 * 1024.0),
				pmc.PagefileUsage / (1024.0 * 1024.0)
			);
			OutputDebugStringA(buf);
		}
		else
		{
			OutputDebugStringA("[Memory] GetProcessMemoryInfo failed\n");
		}
	}

	template<class T>
	constexpr std::string_view MagicEnumToStringView(T&& t)
	{
		if constexpr (std::is_enum_v<std::remove_cvref_t<T>>)
			return magic_enum::enum_name(t);
		else
			return std::string_view(t);
	}

	inline _float4 ColorIntToFloat4(int c) {
		_float4 color;
		color.x = ((c >> 0) & 0xFF) / 255.0f; // R
		color.y = ((c >> 8) & 0xFF) / 255.0f; // G
		color.z = ((c >> 16) & 0xFF) / 255.0f; // B
		color.w = ((c >> 24) & 0xFF) / 255.0f; // A
		return color;
	}

	inline float Randf(float min, float max)
	{
		return min +
			(max - min) *
			(rand() / (float)RAND_MAX);
	}

	inline int RandInt(int min, int max)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());

		std::uniform_int_distribution<int> dist(min, max);
		return dist(gen);
	}
	template<typename T>
	constexpr int32_t ETOI(T e)
	{
		return static_cast<int32_t>(e);
	}

	template<typename T>
	constexpr uint32_t ETOUI(T e)
	{
		return static_cast<uint32_t>(e);
	}

	template<typename T>
	void	Safe_Delete(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	void	Safe_Delete_Array(T& Pointer)
	{
		if (nullptr != Pointer)
		{
			delete[] Pointer;
			Pointer = nullptr;
		}
	}

	template<typename T>
	unsigned int Safe_AddRef(T& pInstance)
	{
		unsigned int	iRefCnt = 0;

		if (nullptr != pInstance)
			iRefCnt = pInstance->AddRef();
		return iRefCnt;
	}

	template<typename T>
	unsigned int Safe_Release(T& pInstance)
	{
		unsigned int	iRefCnt = 0;

		if (nullptr != pInstance)
		{
			iRefCnt = pInstance->Release();

			if (0 == iRefCnt)
				pInstance = nullptr;
		}

		return iRefCnt;
	}

	inline std::string WStringToString(const std::wstring& wstr) {
		if (wstr.empty()) return "";
		int sizeNeeded = WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
		std::string strTo(sizeNeeded, 0);
		WideCharToMultiByte(CP_ACP, 0, &wstr[0], (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
		return strTo;
	}

	inline std::wstring StringToWString(const std::string& str)
	{
		if (str.empty()) return L"";
		int sizeNeeded = MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), nullptr, 0);
		std::wstring wstrTo(sizeNeeded, 0);
		MultiByteToWideChar(CP_ACP, 0, str.data(), (int)str.size(), &wstrTo[0], sizeNeeded);
		return wstrTo;
	}

	
}

