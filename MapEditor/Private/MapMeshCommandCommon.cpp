#include "pch.h"
#include "MapMeshCommandCommon.h"

#include "GameInstance.h"
#include "MapMeshObject.h"

NS_USING(Client)

std::optional<MAPMESH_OBJECT_SNAPSHOT> Client::MakeMapMeshObjectSnapshot(const E::CHandle& handle)
{
	auto* object = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(handle);
	if (object == nullptr)
		return std::nullopt;

	std::string layerTag;
	for (const auto& [layerName, handles] : E::CGameInstance::Get().GetGameObjectLayers())
	{
		if (std::find(handles.begin(), handles.end(), handle) != handles.end())
		{
			layerTag = layerName;
			break;
		}
	}
	if (layerTag.empty())
		return std::nullopt;

	const auto& transform = object->GetTransform();
	MAPMESH_OBJECT_SNAPSHOT snapshot{};
	snapshot.objectTag = object->GetObjectTag();
	snapshot.modelGroupTag = object->GetModelResourceGroup();
	snapshot.modelResTag = object->GetModelResourceTag();
	snapshot.layerTag = std::move(layerTag);
	snapshot.position = transform.GetPosition();
	snapshot.rotation = transform.GetQuaternion();
	snapshot.scale = transform.GetScale();
	snapshot.windDesc = object->GetWindDesc();
	return snapshot;
}

std::optional<E::CHandle> Client::SpawnMapMeshObject(const MAPMESH_OBJECT_SNAPSHOT& snapshot)
{
	E::CMapMeshObject::MAP_MESH_OBJECT_DESC desc{};
	desc.sObjectTag = snapshot.objectTag;
	desc.modelGroupTag = snapshot.modelGroupTag;
	desc.modelResTag = snapshot.modelResTag;
	desc.protoGroupTag = snapshot.protoGroupTag;
	desc.prototypeTag = snapshot.prototypeTag;
	desc.windDesc = snapshot.windDesc;

	auto handle = E::CGameInstance::Get().AddGameObjectToLayer(
		desc.protoGroupTag, desc.prototypeTag, snapshot.layerTag, &desc);
	if (!handle)
		return std::nullopt;

	auto* object = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(*handle);
	if (object == nullptr)
		return std::nullopt;

	auto& transform = object->GetTransform();
	transform.SetPosition(snapshot.position);
	transform.SetQuaternion(snapshot.rotation);
	transform.SetScale(snapshot.scale);
	transform.Update();

	if (FAILED(E::CGameInstance::Get().RegisterMapMeshObjectToMapChunk(*handle)))
	{
		object->SetPendingDestroyCascade(true);
		return std::nullopt;
	}

	return handle;
}

_bool Client::DestroyMapMeshObject(const E::CHandle& handle)
{
	auto* object = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(handle);
	if (object == nullptr)
		return false;

	object->SetPendingDestroyCascade(true);
	return true;
}
