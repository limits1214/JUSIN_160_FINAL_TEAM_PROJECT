#pragma once
#include "Engine_Defines.h"
#include "Handle.h"

NS_BEGIN(Client)

struct MAPMESH_OBJECT_SNAPSHOT
{
	std::string objectTag{};
	std::string modelGroupTag{};
	std::string modelResTag{};
	std::string protoGroupTag = "PERMANENT";
	std::string prototypeTag = "Prototype_GameObject_MapMeshObject";
	std::string layerTag{};
	E::_float3 position{};
	E::_float4 rotation{ 0.f, 0.f, 0.f, 1.f };
	E::_float3 scale{ 1.f, 1.f, 1.f };
	E::WIND_DESC windDesc{};
};

std::optional<MAPMESH_OBJECT_SNAPSHOT> MakeMapMeshObjectSnapshot(const E::CHandle& handle);
std::optional<E::CHandle> SpawnMapMeshObject(const MAPMESH_OBJECT_SNAPSHOT& snapshot);
_bool DestroyMapMeshObject(const E::CHandle& handle);

NS_END
