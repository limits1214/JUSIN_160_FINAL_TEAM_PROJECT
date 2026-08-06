#pragma once

#include <filesystem>
#include <fstream>

namespace Engine::PhysXCookedMeshFile
{
	enum class TYPE : uint32_t
	{
		TRIANGLE_MESH = 1,
		CONVEX_MESH = 2,
		HEIGHT_FIELD = 3
	};

	struct HEADER
	{
		uint32_t iMagic{};
		uint32_t iFileVersion{};
		uint32_t iPhysXVersion{};
		uint32_t iMeshType{};
		uint64_t iPayloadSize{};
	};

	inline constexpr uint32_t MAGIC =
		static_cast<uint32_t>('P') |
		(static_cast<uint32_t>('X') << 8) |
		(static_cast<uint32_t>('C') << 16) |
		(static_cast<uint32_t>('M') << 24);
	inline constexpr uint32_t FILE_VERSION = 1;

	inline HRESULT Write(
		const _string& sPath,
		TYPE eType,
		const uint8_t* pData,
		size_t iDataSize)
	{
		if (sPath.empty() || !pData || iDataSize == 0)
			return E_INVALIDARG;

		const std::filesystem::path path{ sPath };
		const std::filesystem::path tempPath{ path.string() + ".tmp" };
		std::error_code ec{};
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
				return E_FAIL;
		}

		const HEADER header{
			MAGIC,
			FILE_VERSION,
			PX_PHYSICS_VERSION,
			static_cast<uint32_t>(eType),
			static_cast<uint64_t>(iDataSize)
		};

		{
			std::ofstream output{ tempPath, std::ios::binary | std::ios::trunc };
			if (!output)
				return E_FAIL;

			output.write(reinterpret_cast<const char*>(&header), sizeof(header));
			output.write(reinterpret_cast<const char*>(pData), static_cast<std::streamsize>(iDataSize));
			output.flush();
			if (!output)
			{
				output.close();
				std::filesystem::remove(tempPath, ec);
				return E_FAIL;
			}
		}

		if (std::filesystem::exists(path, ec))
		{
			std::filesystem::remove(path, ec);
			if (ec)
			{
				std::filesystem::remove(tempPath, ec);
				return E_FAIL;
			}
		}

		std::filesystem::rename(tempPath, path, ec);
		if (ec)
		{
			std::filesystem::remove(tempPath, ec);
			return E_FAIL;
		}

		return S_OK;
	}

	inline HRESULT Read(
		const _string& sPath,
		TYPE eExpectedType,
		std::vector<uint8_t>& outData)
	{
		outData.clear();
		if (sPath.empty())
			return E_INVALIDARG;

		std::ifstream input{ std::filesystem::path{ sPath }, std::ios::binary | std::ios::ate };
		if (!input)
			return E_FAIL;

		const std::streamoff iFileSize = input.tellg();
		if (iFileSize < static_cast<std::streamoff>(sizeof(HEADER)))
			return E_FAIL;

		input.seekg(0, std::ios::beg);
		HEADER header{};
		input.read(reinterpret_cast<char*>(&header), sizeof(header));
		if (!input ||
			header.iMagic != MAGIC ||
			header.iFileVersion != FILE_VERSION ||
			header.iPhysXVersion != PX_PHYSICS_VERSION ||
			header.iMeshType != static_cast<uint32_t>(eExpectedType) ||
			header.iPayloadSize == 0 ||
			header.iPayloadSize > static_cast<uint64_t>(std::numeric_limits<size_t>::max()) ||
			header.iPayloadSize != static_cast<uint64_t>(iFileSize - sizeof(HEADER)))
		{
			return E_FAIL;
		}

		outData.resize(static_cast<size_t>(header.iPayloadSize));
		input.read(reinterpret_cast<char*>(outData.data()),
			static_cast<std::streamsize>(outData.size()));
		if (!input)
		{
			outData.clear();
			return E_FAIL;
		}

		return S_OK;
	}
}
