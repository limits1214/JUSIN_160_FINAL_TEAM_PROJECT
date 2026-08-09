#include "pch.h"
#include "Inspector.h"
#include "ComTransform.h"
#include "GameInstance.h"
#include "MapMeshObject.h"
#include "Resources.h"

NS_USING(Client)

namespace
{
	struct ModelResourceGUIItem
	{
		std::string groupName{};
		std::string resourceName{};
		std::string label{};
	};

	std::string SafeDbgStr(const E::StringID& id)
	{
		const char* text = id.GetDbgStr();
		return text != nullptr ? text : "<unnamed>";
	}

	std::vector<ModelResourceGUIItem> CollectModelResourceGUIItems()
	{
		std::vector<ModelResourceGUIItem> items{};

		const auto& resourceGroups = E::CGameInstance::Get().GetResources();
		for (const auto& [groupId, resources] : resourceGroups)
		{
			const std::string groupName = SafeDbgStr(groupId);
			for (const auto& [resourceId, resourceList] : resources)
			{
				const std::string resourceName = SafeDbgStr(resourceId);

				const bool hasModelResource = std::any_of(resourceList.begin(), resourceList.end(),
					[](const E::SPtr<E::CResource>& resource)
					{
						return resource != nullptr && resource->IsA(E::CResStaticModel::StaticType);
					});

				if (!hasModelResource)
				{
					continue;
				}

				ModelResourceGUIItem item{};
				item.groupName = groupName;
				item.resourceName = resourceName;
				item.label = groupName + " / " + resourceName;
				items.push_back(std::move(item));
			}
		}

		std::sort(items.begin(), items.end(), [](const ModelResourceGUIItem& lhs, const ModelResourceGUIItem& rhs)
			{
				return lhs.label < rhs.label;
			});

		return items;
	}

	bool DrawVec3Control(const char* label, E::_float3& value, const E::_float3& resetValue, float speed)
	{
		bool changed = false;

		ImGui::PushID(label);
		ImGui::TextUnformatted(label);
		ImGui::SameLine(82.f);
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 34.f);
		changed = ImGui::DragFloat3("##Value", reinterpret_cast<float*>(&value), speed, 0.f, 0.f, "%.3f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		if (ImGui::Button("R", ImVec2(26.f, 0.f)))
		{
			value = resetValue;
			changed = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset %s", label);
		}
		ImGui::PopID();

		return changed;
	}

	void DrawSelectedTransform(E::CComTransform& transform)
	{
		E::_float3 position = transform.GetPosition();
		E::_float3 rotation = transform.GetRotationEuler();
		E::_float3 scale = transform.GetScale();

		if (DrawVec3Control("Position", position, E::_float3{ 0.f, 0.f, 0.f }, 0.1f))
		{
			transform.SetPosition(position);
		}
		if (DrawVec3Control("Rotation", rotation, E::_float3{ 0.f, 0.f, 0.f }, 0.5f))
		{
			transform.SetRotationEuler(rotation);
		}
		if (DrawVec3Control("Scale", scale, E::_float3{ 1.f, 1.f, 1.f }, 0.1f))
		{
			transform.SetScale(scale);
		}

		transform.Update();
	}

	void DrawMapMeshObjectInspector(E::CMapMeshObject& mapMeshObject)
	{
		ImGui::TextUnformatted("Model");
		ImGui::SameLine(82.f);
		ImGui::Text("%s / %s", mapMeshObject.GetModelResourceGroup().c_str(), mapMeshObject.GetModelResourceTag().c_str());

		const std::string preview = mapMeshObject.GetModelResourceGroup() + " / " + mapMeshObject.GetModelResourceTag();
		if (ImGui::BeginCombo("Model Resource", preview.c_str()))
		{
			const auto modelItems = CollectModelResourceGUIItems();
			for (const auto& item : modelItems)
			{
				const bool bSelected = item.groupName == mapMeshObject.GetModelResourceGroup()
					&& item.resourceName == mapMeshObject.GetModelResourceTag();

				if (ImGui::Selectable(item.label.c_str(), bSelected))
				{
					mapMeshObject.SetModelResource(item.groupName, item.resourceName);
				}

				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			if (modelItems.empty())
			{
				ImGui::TextDisabled("No CResTestModel resources.");
			}

			ImGui::EndCombo();
		}

		ImGui::Separator();
		ImGui::TextUnformatted("Wind");

		E::WIND_DESC windDesc = mapMeshObject.GetWindDesc();
		int32_t windType = static_cast<int32_t>(windDesc.type);
		constexpr const char* windTypeNames[] = { "None", "Grass", "Tree" };

		_bool changed = ImGui::Combo(
			"Wind Type", &windType, windTypeNames, static_cast<int32_t>(std::size(windTypeNames)));
		changed |= ImGui::DragFloat("Strength", &windDesc.strength, 0.01f, 0.f, 10.f, "%.3f");
		changed |= ImGui::DragFloat("Speed", &windDesc.speed, 0.01f, 0.f, 10.f, "%.3f");
		changed |= ImGui::DragFloat("Frequency", &windDesc.frequency, 0.01f, 0.f, 10.f, "%.3f");
		changed |= ImGui::DragFloat("Bend Exponent", &windDesc.bendExponent, 0.05f, 0.1f, 8.f, "%.3f");
		changed |= ImGui::DragFloatRange2(
			"Height Weight Range",
			&windDesc.heightStart,
			&windDesc.heightEnd,
			0.01f,
			0.f,
			1.f,
			"Start %.2f",
			"End %.2f");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Vertices below Start stay fixed; vertices above End receive full wind strength.");
		}

		if (changed)
		{
			windDesc.type = static_cast<E::EWindType>(windType);
			mapMeshObject.SetWindDesc(windDesc);
		}
	}
}

CInspector::CInspector()
{
}

CInspector::~CInspector()
{
}

void CInspector::UpdateGUI(E::_float fTimeDelta)
{
	ImGui::TextDisabled("Inspector");

	auto* pSelectedObject = GetSelectedObject();
	auto* pSelectedHandle = GetSelectedHandle();
	if (pSelectedObject == nullptr || pSelectedHandle == nullptr)
	{
		ImGui::TextColored(ImVec4(1.f, 1.f, 1.f, 0.55f), "Select an object to view details.");
		return;
	}

	ImGui::BeginChild("##Inspector", ImVec2(0.f, 0.f), true);
	ImGui::TextUnformatted("Name");
	ImGui::SameLine(82.f);
	ImGui::TextUnformatted(pSelectedObject->GetObjectTag().data());

	ImGui::TextUnformatted("Handle");
	ImGui::SameLine(82.f);
	ImGui::Text("%zu : %u", pSelectedHandle->GetIndex(), pSelectedHandle->GetGeneration());

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		DrawSelectedTransform(pSelectedObject->GetTransform());
	}

	if (auto* pMapMeshObject = E::CGameInstance::Get().GetGameObjectByHandleT<E::CMapMeshObject>(*pSelectedHandle))
	{
		if (ImGui::CollapsingHeader("MapMeshObject", ImGuiTreeNodeFlags_DefaultOpen))
		{
			DrawMapMeshObjectInspector(*pMapMeshObject);
		}
	}

	ImGui::EndChild();
}

E::UPtr<CInspector> CInspector::Create(E::CHandle* pSelectedObject)
{
	auto pInstance = E::UPtr<CInspector>(new CInspector{});
	if (FAILED(pInstance->Initialize(pSelectedObject)))
	{
		MSG_BOX("Failed to Created : CInspector");
		return nullptr;
	}

	return pInstance;
}
