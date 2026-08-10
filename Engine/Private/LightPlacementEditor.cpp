#include "pch.h"
#include "LightPlacementEditor.h"

#include "CameraObject.h"
#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Light.h"
#include "LightManager.h"

NS_USING(Engine)

namespace Engine::LightPlacementEditorDetail
{
	inline constexpr int GIZMO_ID = 0x4C495445;
	inline constexpr _float MIN_DIRECTION_LENGTH_SQ = 1e-8f;
	inline constexpr _float MIN_RANGE = 0.02f;
	inline constexpr std::array<const char*, 6> FILE_PRESETS{
		"Custom",
		"Level_CharlesRookwood",
		"Level_BossCharlesRookwood",
		"Level_Terrain",
		"Level_LastBossRanrok",
		"Level_HogwartWorld"
	};

	const char* GetLightTypeName(LIGHT_TYPE eType)
	{
		switch (eType)
		{
		case LIGHT_TYPE::DIRECTIONAL:
			return "Directional";
		case LIGHT_TYPE::POINT:
			return "Point";
		case LIGHT_TYPE::SPOTLIGHT:
			return "Spot";
		default:
			return "Unknown";
		}
	}

	_float3 NormalizeDirection(
		const _float3& direction,
		const _float3& fallback = { 0.f, -1.f, 0.f })
	{
		const _vector loadedDirection = XMLoadFloat3(&direction);
		if (XMVectorGetX(XMVector3LengthSq(loadedDirection)) <=
			MIN_DIRECTION_LENGTH_SQ)
		{
			return fallback;
		}

		_float3 normalized{};
		XMStoreFloat3(
			&normalized,
			XMVector3Normalize(loadedDirection));
		return normalized;
	}

	_matrix MakeDirectionWorld(
		const _float3& position,
		const _float3& direction)
	{
		const _float3 normalizedDirection =
			NormalizeDirection(direction);
		const _vector loadedDirection =
			XMLoadFloat3(&normalizedDirection);
		_vector up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		if (std::abs(XMVectorGetX(
				XMVector3Dot(up, loadedDirection))) > 0.99f)
		{
			up = XMVectorSet(1.f, 0.f, 0.f, 0.f);
		}

		return XMMatrixInverse(
			nullptr,
			XMMatrixLookToLH(
				XMLoadFloat3(&position),
				loadedDirection,
				up));
	}

	_matrix MakeSpotConeWorld(
		const _float3& position,
		const _float3& direction,
		_float length)
	{
		const _float3 normalizedDirection =
			NormalizeDirection(direction);
		_float3 coneBasePosition{};
		XMStoreFloat3(
			&coneBasePosition,
			XMLoadFloat3(&position) +
				XMLoadFloat3(&normalizedDirection) * length);

		const _float3 coneAxis{
			-normalizedDirection.x,
			-normalizedDirection.y,
			-normalizedDirection.z
		};

		// AddCone의 로컬 +Y 끝점이 광원 위치의 꼭짓점이 되도록
		// 원뿔 밑면을 광원 진행 방향의 끝에 놓는다.
		return XMMatrixRotationX(XM_PIDIV2) *
			MakeDirectionWorld(coneBasePosition, coneAxis);
	}
}

CLightPlacementEditor::CLightPlacementEditor(
	CLightManager* pLightManager)
	: m_pLightManager{ pLightManager }
{
}

void CLightPlacementEditor::UpdateGUI()
{
	DrawWindow();
	DrawDebugLights();

	if (m_bEditMode)
		RenderGizmo();

	if (m_SelectedLight &&
		ImGui::IsMouseClicked(0) &&
		!ImGui::IsAnyItemHovered() &&
		!ImGuizmo::IsOver() &&
		!ImGuizmo::IsUsing())
	{
		m_SelectedLight.reset();
	}

	DrawDebugLights();
}

void CLightPlacementEditor::SetActivePlacementGroup(
	std::string_view sGroup)
{
	const std::string group =
		MakeLightPlacementGroupName(
			std::string{ sGroup });
	strncpy_s(
		m_LightFileName,
		std::size(m_LightFileName),
		group.c_str(),
		_TRUNCATE);

	m_iLightFilePreset = 0;
	for (size_t i = 1;
		i < LightPlacementEditorDetail::FILE_PRESETS.size();
		++i)
	{
		if (group ==
			LightPlacementEditorDetail::FILE_PRESETS[i])
		{
			m_iLightFilePreset =
				static_cast<int32_t>(i);
			break;
		}
	}
	m_SelectedLight.reset();
}

void CLightPlacementEditor::DrawWindow()
{
	ImGui::SetNextWindowSize(
		ImVec2(520.f, 650.f),
		ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Light Placement Editor"))
	{
		ImGui::End();
		return;
	}

	if (m_bOpenResultPopup)
	{
		ImGui::OpenPopup("Light IO Result");
		m_bOpenResultPopup = false;
	}

	ImGui::Checkbox("Edit Mode", &m_bEditMode);
	ImGui::SameLine();
	ImGui::Checkbox("Visible", &m_bVisible);
	ImGui::SameLine();
	ImGui::Checkbox("Depth", &m_bDepthTest);

	ImGui::Checkbox("Show All", &m_bShowAllLights);
	ImGui::SameLine();
	ImGui::Checkbox("Influence Range", &m_bShowInfluenceRange);
	ImGui::SameLine();
	ImGui::Checkbox("Direction", &m_bShowDirection);

	ImGui::Checkbox(
		"Effect Lights",
		&m_bShowEffectLights);
	ImGui::SameLine();
	ImGui::Checkbox(
		"Effect Depth",
		&m_bEffectLightDepthTest);
	if (m_pLightManager)
	{
		m_pLightManager->
			SetEffectLightDebugOptions(
				m_bShowEffectLights,
				m_bEffectLightDepthTest);

		m_pLightManager->SetEffectLightDebugOptions(
			m_bShowEffectLights,
			m_bEffectLightDepthTest);
	}

	ImGui::DragFloat(
		"Spawn Distance",
		&m_fSpawnDistance,
		0.1f,
		0.1f,
		100.f,
		"%.1f");

	if (ImGui::Button("Create Directional", ImVec2(150.f, 0.f)))
		m_SelectedLight =
			CreateLightAtCamera(LIGHT_TYPE::DIRECTIONAL);
	ImGui::SameLine();
	if (ImGui::Button("Create Point", ImVec2(130.f, 0.f)))
		m_SelectedLight =
			CreateLightAtCamera(LIGHT_TYPE::POINT);
	ImGui::SameLine();
	if (ImGui::Button("Create Spot", ImVec2(130.f, 0.f)))
		m_SelectedLight =
			CreateLightAtCamera(LIGHT_TYPE::SPOTLIGHT);

	ImGui::Separator();
	const auto& filePresets =
		LightPlacementEditorDetail::FILE_PRESETS;
	if (ImGui::Combo(
		"Level Preset",
		&m_iLightFilePreset,
		filePresets.data(),
		static_cast<int32_t>(std::size(filePresets))) &&
		m_iLightFilePreset > 0)
	{
		strcpy_s(
			m_LightFileName,
			std::size(m_LightFileName),
			filePresets[m_iLightFilePreset]);
		m_SelectedLight.reset();
	}

	ImGui::Checkbox(
		"Manual File Name",
		&m_bManualFileNameInput);
	if (!m_bManualFileNameInput)
	{
		ImGui::PushItemFlag(
			ImGuiItemFlags_Disabled,
			true);
		ImGui::PushStyleVar(
			ImGuiStyleVar_Alpha,
			ImGui::GetStyle().Alpha * 0.5f);
	}
	if (ImGui::InputText(
		"Light File",
		m_LightFileName,
		std::size(m_LightFileName)))
	{
		m_iLightFilePreset = 0;
		m_SelectedLight.reset();
	}
	if (!m_bManualFileNameInput)
	{
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();
	}
	if (ImGui::Button("Save", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Light Save");
	ImGui::SameLine();
	if (ImGui::Button("Load", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Light Load");
	ImGui::SameLine();
	if (ImGui::Button("Clear", ImVec2(90.f, 0.f)))
		ImGui::OpenPopup("Confirm Light Clear");

	if (ImGui::BeginPopupModal(
		"Confirm Light Save",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		const std::string path =
			MakeLightPlacementFilePath(m_LightFileName);
		ImGui::Text("Save light placement to:");
		ImGui::TextWrapped("%s", path.c_str());
		if (ImGui::Button("Save", ImVec2(100.f, 0.f)))
		{
			const _bool success = SUCCEEDED(Save());
			m_Status = success
				? "Saved: " + path
				: "Save failed: " + path;
			QueueResultPopup(m_Status, success);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Confirm Light Load",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		const std::string path =
			MakeLightPlacementFilePath(m_LightFileName);
		ImGui::Text("Replace current lights with:");
		ImGui::TextWrapped("%s", path.c_str());
		if (ImGui::Button("Load", ImVec2(100.f, 0.f)))
		{
			const _bool success = SUCCEEDED(Load());
			m_Status = success
				? "Loaded: " + path
				: "Load failed: " + path;
			QueueResultPopup(m_Status, success);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Confirm Light Clear",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		const std::string group =
			GetActivePlacementGroup();
		ImGui::Text(
			"Remove lights in placement group: %s?",
			group.c_str());
		if (ImGui::Button("Clear", ImVec2(100.f, 0.f)))
		{
			Clear();
			m_Status = "Current light placement cleared.";
			QueueResultPopup(m_Status, true);
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal(
		"Light IO Result",
		nullptr,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(
			m_bResultPopupSuccess
				? ImVec4(0.3f, 1.f, 0.3f, 1.f)
				: ImVec4(1.f, 0.3f, 0.3f, 1.f),
			"%s",
			m_ResultPopupMessage.c_str());
		if (ImGui::Button("OK", ImVec2(100.f, 0.f)))
			ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
	}

	DrawLightList();
	DrawSelectedLightInspector();

	if (!m_Status.empty())
	{
		ImGui::Separator();
		ImGui::TextWrapped("%s", m_Status.c_str());
	}

	ImGui::End();
}

void CLightPlacementEditor::DrawLightList()
{
	if (!m_pLightManager)
		return;

	ImGui::Separator();
	ImGui::TextUnformatted("Lights");
	if (!ImGui::BeginListBox(
		"##LightPlacementList",
		ImVec2(-FLT_MIN, 160.f)))
	{
		return;
	}

	const std::string activeGroup =
		GetActivePlacementGroup();
	for (const auto& optionalHandle :
		m_pLightManager->GetLightHandles())
	{
		if (!optionalHandle)
			continue;

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(*optionalHandle);
		if (!light)
			continue;
		if (light->Get_LightPlacementGroup() !=
			activeGroup)
		{
			continue;
		}

		const CHandle handle = *optionalHandle;
		ImGui::PushID(
			static_cast<int>(handle.GetIndex()));
		std::string displayName{ light->GetObjectTag() };
		if (!light->Get_LightAlias().empty())
		{
			displayName += " (";
			displayName += light->Get_LightAlias();
			displayName += ")";
		}
		const std::string label =
			displayName +
			" [" +
			LightPlacementEditorDetail::GetLightTypeName(
				light->Get_LightType()) +
			"]";
		const _bool selected =
			m_SelectedLight &&
			*m_SelectedLight == handle;
		if (ImGui::Selectable(label.c_str(), selected))
			m_SelectedLight = handle;
		if (selected)
			ImGui::SetItemDefaultFocus();
		ImGui::PopID();
	}

	ImGui::EndListBox();
}

void CLightPlacementEditor::DrawSelectedLightInspector()
{
	CLight* light = GetSelectedLight();
	if (!light)
	{
		m_SelectedLight.reset();
		return;
	}

	ImGui::Separator();
	std::string displayName{ light->GetObjectTag() };
	if (!light->Get_LightAlias().empty())
	{
		displayName += " (";
		displayName += light->Get_LightAlias();
		displayName += ")";
	}
	ImGui::Text("Selected: %s", displayName.c_str());

	char alias[128]{};
	strncpy_s(
		alias,
		std::size(alias),
		light->Get_LightAlias().c_str(),
		_TRUNCATE);
	if (ImGui::InputText(
		"Alias",
		alias,
		std::size(alias)))
	{
		light->Set_LightAlias(alias);
	}

	LIGHT_TYPE lightType = light->Get_LightType();
	int32_t lightTypeIndex = static_cast<int32_t>(lightType);
	const char* lightTypeNames[] =
	{
		"Directional",
		"Point",
		"Spot"
	};
	if (ImGui::Combo(
		"Light Type",
		&lightTypeIndex,
		lightTypeNames,
		static_cast<int32_t>(std::size(lightTypeNames))))
	{
		lightType = static_cast<LIGHT_TYPE>(lightTypeIndex);
		if (SUCCEEDED(light->Set_LightType(lightType)) &&
			lightType == LIGHT_TYPE::POINT &&
			light->Get_PointLightOuterAttenuation() <
				LightPlacementEditorDetail::MIN_RANGE)
		{
			light->Set_PointLightInnerAttenuation(10.f);
			light->Set_PointLightOuterAttenuation(20.f);
		}
	}

	_bool active = light->Get_LightActivateState();
	if (ImGui::Checkbox("Active", &active))
		light->Set_LightActivateState(active);

	_bool castShadow = light->Get_LightShadowCast();
	if (ImGui::Checkbox("Cast Shadow", &castShadow))
		light->Set_LightShadowCast(castShadow);

	_float3 position = light->Get_LightPosition();
	if (ImGui::DragFloat3(
		"Position",
		&position.x,
		0.05f,
		-10000.f,
		10000.f,
		"%.3f"))
	{
		light->Set_LightPosition(position);
	}

	_float3 direction = light->Get_LightDirection();
	if (lightType != LIGHT_TYPE::POINT &&
		ImGui::DragFloat3(
			"Direction",
			&direction.x,
			0.01f,
			-1.f,
			1.f,
			"%.3f"))
	{
		light->Set_LightDirection(
			LightPlacementEditorDetail::NormalizeDirection(
				direction));
	}

	_float3 color = light->Get_LightColor();
	if (ImGui::ColorEdit3("Color", &color.x))
		light->Set_LightColor(color);

	_float intensity = light->Get_LightIntensity();
	if (ImGui::DragFloat(
		"Intensity",
		&intensity,
		0.1f,
		0.f,
		1000.f,
		"%.2f"))
	{
		light->Set_LightIntensity(intensity);
	}

	if (lightType == LIGHT_TYPE::POINT)
	{
		_float innerRange =
			std::max(
				light->Get_PointLightInnerAttenuation(),
				0.f);
		_float outerRange =
			std::max(
				light->Get_PointLightOuterAttenuation(),
				LightPlacementEditorDetail::MIN_RANGE);

		if (ImGui::DragFloat(
			"Inner Range",
			&innerRange,
			0.1f,
			0.f,
			5000.f,
			"%.2f"))
		{
			innerRange = std::clamp(
				innerRange,
				0.f,
				outerRange);
			light->Set_PointLightInnerAttenuation(
				innerRange);
		}

		if (ImGui::DragFloat(
			"Outer Range",
			&outerRange,
			0.1f,
			LightPlacementEditorDetail::MIN_RANGE,
			5000.f,
			"%.2f"))
		{
			outerRange = std::max(
				outerRange,
				LightPlacementEditorDetail::MIN_RANGE);
			if (innerRange > outerRange)
			{
				innerRange = outerRange;
				light->Set_PointLightInnerAttenuation(
					innerRange);
			}
			light->Set_PointLightOuterAttenuation(
				outerRange);
		}
	}
	else if (lightType == LIGHT_TYPE::SPOTLIGHT)
	{
		_float range = light->Get_LightRange();
		if (ImGui::DragFloat(
			"Range",
			&range,
			0.1f,
			LightPlacementEditorDetail::MIN_RANGE,
			5000.f,
			"%.2f"))
		{
			light->Set_LightRange(std::max(
				range,
				LightPlacementEditorDetail::MIN_RANGE));
		}
	}

	if (lightType == LIGHT_TYPE::SPOTLIGHT)
	{
		_float inner = light->Get_LightInnerAttenuation();
		_float outer = light->Get_LightOuterAttenuation();
		if (ImGui::DragFloat(
			"Inner Angle",
			&inner,
			0.1f,
			0.f,
			75.f,
			"%.1f"))
		{
			light->Set_LightInnerAttenuation(
				std::clamp(inner, 0.f, outer));
		}
		if (ImGui::DragFloat(
			"Outer Angle",
			&outer,
			0.1f,
			0.1f,
			75.f,
			"%.1f"))
		{
			light->Set_LightOuterAttenuation(
				std::clamp(outer, inner, 75.f));
		}
	}

	ImGui::Separator();
	if (ImGui::Button("T", ImVec2(34.f, 0.f)))
		m_eGizmoOperation = ImGuizmo::TRANSLATE;
	ImGui::SameLine();
	if (ImGui::Button("R", ImVec2(34.f, 0.f)))
		m_eGizmoOperation = ImGuizmo::ROTATE;
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"Local",
		m_eGizmoMode == ImGuizmo::LOCAL))
	{
		m_eGizmoMode = ImGuizmo::LOCAL;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton(
		"World",
		m_eGizmoMode == ImGuizmo::WORLD))
	{
		m_eGizmoMode = ImGuizmo::WORLD;
	}

	ImGui::Checkbox("Snap", &m_bSnapEnabled);
	if (m_bSnapEnabled)
	{
		if (m_eGizmoOperation == ImGuizmo::TRANSLATE)
		{
			ImGui::DragFloat(
				"Translation Snap",
				&m_fTranslationSnap,
				0.1f,
				0.01f,
				100.f,
				"%.2f");
		}
		else
		{
			ImGui::DragFloat(
				"Rotation Snap",
				&m_fRotationSnap,
				1.f,
				1.f,
				180.f,
				"%.0f");
		}
	}

	if (ImGui::Button(
		"Delete Selected",
		ImVec2(-FLT_MIN, 0.f)))
	{
		DeleteSelected();
	}

	ImGui::Text(
		"Dirty - Static: %s, Dynamic: %s",
		light->Is_StaticDirty() ? "ON" : "OFF",
		light->Is_DynamicDirty() ? "ON" : "OFF");
}

void CLightPlacementEditor::DrawDebugLights()
{
	if (!m_bVisible || !m_pLightManager)
		return;

	CDbgLineRender* debug =
		CGameInstance::Get().GetDbgLineRender();
	if (!debug)
		return;

	const _float4 previousColor = debug->GetColor();
	const DBG_LINE_DEPTH_MODE previousDepth =
		debug->GetDepthMode();
	debug->SetDepthTest(m_bDepthTest);

	const std::string activeGroup =
		GetActivePlacementGroup();
	for (const auto& optionalHandle :
		m_pLightManager->GetLightHandles())
	{
		if (!optionalHandle)
			continue;

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(*optionalHandle);
		if (!light)
			continue;
		if (light->Get_LightPlacementGroup() !=
			activeGroup)
		{
			continue;
		}

		const _bool selected =
			m_SelectedLight &&
			*m_SelectedLight == *optionalHandle;
		if (!m_bShowAllLights && !selected)
			continue;

		DrawDebugLight(*debug, *light, selected);
	}

	debug->SetColor(previousColor);
	debug->SetDepthMode(previousDepth);
}

void CLightPlacementEditor::DrawDebugLight(
	CDbgLineRender& debug,
	CLight& light,
	_bool bSelected) const
{
	const LIGHT_TYPE type = light.Get_LightType();
	const _float3 position = light.Get_LightPosition();
	const _float3 direction =
		LightPlacementEditorDetail::NormalizeDirection(
			light.Get_LightDirection());

	_float4 color{};
	if (bSelected)
		color = { 1.f, 0.9f, 0.1f, 1.f };
	else if (type == LIGHT_TYPE::POINT)
		color = { 1.f, 0.45f, 0.1f, 1.f };
	else if (type == LIGHT_TYPE::SPOTLIGHT)
		color = { 0.1f, 0.9f, 0.8f, 1.f };
	else
		color = { 0.2f, 0.65f, 1.f, 1.f };

	debug.SetColor(color);
	debug.AddCross(position, bSelected ? 0.35f : 0.2f);

	if (type == LIGHT_TYPE::POINT)
	{
		if (m_bShowInfluenceRange)
		{
			const _float innerRange = std::max(
				light.Get_PointLightInnerAttenuation(),
				0.f);
			const _float outerRange = std::max(
				light.Get_PointLightOuterAttenuation(),
				LightPlacementEditorDetail::MIN_RANGE);

			if (innerRange > 0.f)
			{
				debug.SetColor(
					bSelected
					? _float4{ 1.f, 1.f, 1.f, 1.f }
					: _float4{ 1.f, 0.9f, 0.1f, 1.f });
				debug.AddSphere(
					std::min(innerRange, outerRange),
					XMMatrixTranslation(
						position.x,
						position.y,
						position.z));
			}

			debug.SetColor(
				bSelected
				? _float4{ 1.f, 0.9f, 0.1f, 1.f }
				: _float4{ 1.f, 0.4f, 0.05f, 1.f });
			debug.AddSphere(
				outerRange,
				XMMatrixTranslation(
					position.x,
					position.y,
					position.z));
		}
		return;
	}

	const _float directionLength =
		type == LIGHT_TYPE::DIRECTIONAL
		? 3.f
		: std::min(
			std::max(light.Get_LightRange(), 1.f),
			3.f);
	if (m_bShowDirection)
		debug.AddArrow(
			position,
			direction,
			directionLength);

	if (type == LIGHT_TYPE::SPOTLIGHT &&
		m_bShowInfluenceRange)
	{
		const _float range = std::max(
			light.Get_LightRange(),
			LightPlacementEditorDetail::MIN_RANGE);
		const _float outerAngle = std::clamp(
			light.Get_LightOuterAttenuation(),
			0.1f,
			75.f);
		const _float innerAngle = std::clamp(
			light.Get_LightInnerAttenuation(),
			0.f,
			outerAngle);
		const _float outerHalfAngle =
			XMConvertToRadians(outerAngle);
		const _float innerHalfAngle =
			XMConvertToRadians(innerAngle);
		const _matrix coneWorld =
			LightPlacementEditorDetail::MakeSpotConeWorld(
				position,
				direction,
				range);

		if (innerHalfAngle > 0.f)
		{
			debug.SetColor(
				bSelected
				? _float4{ 1.f, 1.f, 1.f, 1.f }
				: _float4{ 0.2f, 1.f, 0.45f, 1.f });
			debug.AddCone(
				std::tan(innerHalfAngle) * range,
				range,
				coneWorld);
		}

		debug.SetColor(
			bSelected
			? _float4{ 1.f, 0.9f, 0.1f, 1.f }
			: _float4{ 0.1f, 0.9f, 0.8f, 1.f });
		debug.AddCone(
			std::tan(outerHalfAngle) * range,
			range,
			coneWorld);
	}
}

void CLightPlacementEditor::RenderGizmo()
{
	CLight* light = GetSelectedLight();
	CCameraObject* camera =
		CGameInstance::Get().GetActiveCamera();
	if (!light || !camera)
		return;

	if (light->Get_LightType() == LIGHT_TYPE::POINT &&
		m_eGizmoOperation == ImGuizmo::ROTATE)
	{
		return;
	}

	_float4x4 view{};
	_float4x4 projection{};
	_float4x4 world{};
	XMStoreFloat4x4(&view, camera->GetView());
	XMStoreFloat4x4(&projection, camera->GetProj());
	XMStoreFloat4x4(
		&world,
		LightPlacementEditorDetail::MakeDirectionWorld(
			light->Get_LightPosition(),
			light->Get_LightDirection()));

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	if (!viewport)
		return;

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(
		ImGui::GetForegroundDrawList(viewport));
	ImGuizmo::SetRect(
		viewport->Pos.x,
		viewport->Pos.y,
		viewport->Size.x,
		viewport->Size.y);
	ImGuizmo::SetID(
		LightPlacementEditorDetail::GIZMO_ID);

	_float3 snap{};
	const _float* snapValues = nullptr;
	if (m_bSnapEnabled)
	{
		const _float value =
			m_eGizmoOperation == ImGuizmo::TRANSLATE
			? std::max(m_fTranslationSnap, 0.01f)
			: std::max(m_fRotationSnap, 1.f);
		snap = { value, value, value };
		snapValues = &snap.x;
	}

	if (!ImGuizmo::Manipulate(
		&view._11,
		&projection._11,
		m_eGizmoOperation,
		m_eGizmoMode,
		&world._11,
		nullptr,
		snapValues))
	{
		return;
	}

	_vector scale{};
	_vector rotation{};
	_vector translation{};
	if (!XMMatrixDecompose(
		&scale,
		&rotation,
		&translation,
		XMLoadFloat4x4(&world)))
	{
		return;
	}

	_float3 position{};
	XMStoreFloat3(&position, translation);
	light->Set_LightPosition(position);

	if (m_eGizmoOperation == ImGuizmo::ROTATE)
	{
		_float3 direction{};
		XMStoreFloat3(
			&direction,
			XMVector3Normalize(
				XMLoadFloat4x4(&world).r[2]));
		light->Set_LightDirection(direction);
	}
}

std::optional<CHandle>
CLightPlacementEditor::CreateLightAtCamera(LIGHT_TYPE eType)
{
	if (!m_pLightManager)
		return std::nullopt;

	CCameraObject* camera =
		CGameInstance::Get().GetActiveCamera();
	if (!camera)
	{
		m_Status = "Create failed: no active camera.";
		return std::nullopt;
	}

	const _vector cameraPosition =
		camera->GetTransform().GetLoadedPostion();
	const _vector cameraForward = XMVector3Normalize(
		camera->GetTransform().GetState(STATE::LOOK));
	const _vector spawnPosition =
		cameraPosition +
		cameraForward * std::max(m_fSpawnDistance, 0.1f);

	_float3 position{};
	_float3 direction{};
	XMStoreFloat3(&position, spawnPosition);
	XMStoreFloat3(&direction, cameraForward);

	std::optional<CHandle> handle{};
	switch (eType)
	{
	case LIGHT_TYPE::DIRECTIONAL:
		handle = m_pLightManager->Add_DirectionalLight(
			direction,
			{ 1.f, 1.f, 1.f },
			10.f);
		break;
	case LIGHT_TYPE::POINT:
		handle = m_pLightManager->Add_PointLight(
			position,
			{ 1.f, 1.f, 1.f },
			10.f,
			10.f,
			20.f);
		break;
	case LIGHT_TYPE::SPOTLIGHT:
		handle = m_pLightManager->Add_SpotLight(
			position,
			{ 1.f, 1.f, 1.f },
			10.f,
			10.f,
			20.f,
			30.f);
		break;
	default:
		return std::nullopt;
	}

	if (!handle)
	{
		m_Status = "Create light failed.";
		return std::nullopt;
	}

	if (CLight* light = CGameInstance::Get().
		GetGameObjectByHandleT<CLight>(*handle))
	{
		light->Set_LightPlacementGroup(
			GetActivePlacementGroup());
		light->Set_LightPosition(position);
		if (eType != LIGHT_TYPE::POINT)
			light->Set_LightDirection(direction);
	}

	m_Status = std::string{ "Created " } +
		LightPlacementEditorDetail::GetLightTypeName(eType) +
		" light.";
	return handle;
}

std::optional<CHandle> CLightPlacementEditor::CreateLight(
	const LIGHT_PLACEMENT_ENTRY& data)
{
	if (!m_pLightManager)
		return std::nullopt;

	std::optional<CHandle> handle{};
	switch (data.eType)
	{
	case LIGHT_TYPE::DIRECTIONAL:
		handle = m_pLightManager->Add_DirectionalLight(
			data.vDirection,
			data.vColor,
			data.fIntensity);
		break;
	case LIGHT_TYPE::POINT:
	{
		const _float outerRange = std::max(
			data.fOuterAttenuation,
			LightPlacementEditorDetail::MIN_RANGE);
		const _float innerRange = std::clamp(
			data.fInnerAttenuation,
			0.f,
			outerRange);
		handle = m_pLightManager->Add_PointLight(
			data.vPosition,
			data.vColor,
			data.fIntensity,
			innerRange,
			outerRange);
		break;
	}
	case LIGHT_TYPE::SPOTLIGHT:
		handle = m_pLightManager->Add_SpotLight(
			data.vPosition,
			data.vColor,
			data.fIntensity,
			std::max(
				data.fRange,
				LightPlacementEditorDetail::MIN_RANGE),
			data.fInnerAttenuation,
			data.fOuterAttenuation);
		break;
	default:
		return std::nullopt;
	}

	if (!handle)
		return std::nullopt;

	if (CLight* light = CGameInstance::Get().
		GetGameObjectByHandleT<CLight>(*handle))
	{
		light->Set_LightPlacementGroup(
			GetActivePlacementGroup());
		light->SetObjectTag(data.sName);
		light->Set_LightAlias(data.sAlias);
		light->Set_LightPosition(data.vPosition);
		light->Set_LightDirection(
			LightPlacementEditorDetail::NormalizeDirection(
				data.vDirection));
		light->Set_LightActivateState(data.bActive);
		light->Set_LightShadowCast(data.bCastShadow);
	}

	return handle;
}

LIGHT_PLACEMENT_FILE
CLightPlacementEditor::BuildFileData() const
{
	LIGHT_PLACEMENT_FILE file{};
	if (!m_pLightManager)
		return file;

	const std::string activeGroup =
		GetActivePlacementGroup();
	for (const auto& optionalHandle :
		m_pLightManager->GetLightHandles())
	{
		if (!optionalHandle)
			continue;

		CLight* light = CGameInstance::Get().
			GetGameObjectByHandleT<CLight>(*optionalHandle);
		if (!light)
			continue;
		if (light->Get_LightPlacementGroup() !=
			activeGroup)
		{
			continue;
		}

		LIGHT_PLACEMENT_ENTRY entry{};
		entry.sName = light->GetObjectTag();
		entry.sAlias = light->Get_LightAlias();
		entry.eType = light->Get_LightType();
		entry.vPosition = light->Get_LightPosition();
		entry.vDirection = light->Get_LightDirection();
		entry.vColor = light->Get_LightColor();
		entry.fIntensity = light->Get_LightIntensity();
		entry.bCastShadow = light->Get_LightShadowCast();
		if (entry.eType == LIGHT_TYPE::POINT)
		{
			entry.fInnerAttenuation =
				light->Get_PointLightInnerAttenuation();
			entry.fOuterAttenuation =
				light->Get_PointLightOuterAttenuation();
			entry.fRange =
				entry.fOuterAttenuation;
		}
		else
		{
			entry.fRange = light->Get_LightRange();
			entry.fInnerAttenuation =
				light->Get_LightInnerAttenuation();
			entry.fOuterAttenuation =
				light->Get_LightOuterAttenuation();
		}
		entry.bActive =
			light->Get_LightActivateState();
		file.lights.push_back(std::move(entry));
	}

	return file;
}

std::string
CLightPlacementEditor::GetActivePlacementGroup() const
{
	return MakeLightPlacementGroupName(
		m_LightFileName);
}

HRESULT CLightPlacementEditor::Save() const
{
	const std::string path =
		MakeLightPlacementFilePath(m_LightFileName);
	const LIGHT_PLACEMENT_FILE file = BuildFileData();
	return CGameInstance::Get().JsonSerialize(
		path,
		file,
		"LightPlacements");
}

HRESULT CLightPlacementEditor::Load()
{
	const std::string path =
		MakeLightPlacementFilePath(m_LightFileName);
	LIGHT_PLACEMENT_FILE file{};
	if (FAILED(CGameInstance::Get().JsonDeSerialize(
		path,
		file,
		"LightPlacements")))
	{
		return E_FAIL;
	}

	if (file.iVersion != 1)
		return E_FAIL;

	Clear();
	for (const LIGHT_PLACEMENT_ENTRY& entry : file.lights)
	{
		if (!CreateLight(entry))
		{
			Clear();
			return E_FAIL;
		}
	}

	return S_OK;
}

void CLightPlacementEditor::Clear()
{
	if (!m_pLightManager)
		return;

	m_pLightManager->Remove_PlacementLightGroup(
		GetActivePlacementGroup());
	m_SelectedLight.reset();
}

CLight* CLightPlacementEditor::GetSelectedLight() const
{
	if (!m_SelectedLight)
		return nullptr;

	CLight* light = CGameInstance::Get().
		GetGameObjectByHandleT<CLight>(*m_SelectedLight);
	if (!light ||
		light->Get_LightPlacementGroup() !=
			GetActivePlacementGroup())
	{
		return nullptr;
	}
	return light;
}

void CLightPlacementEditor::DeleteSelected()
{
	if (!m_SelectedLight || !m_pLightManager)
		return;

	if (m_pLightManager->Remove_Light(*m_SelectedLight))
		m_Status = "Selected light removed.";
	else
		m_Status = "Remove light failed.";
	m_SelectedLight.reset();
}

void CLightPlacementEditor::QueueResultPopup(
	std::string message,
	_bool bSuccess)
{
	m_ResultPopupMessage = std::move(message);
	m_bResultPopupSuccess = bSuccess;
	m_bOpenResultPopup = true;
}

UPtr<CLightPlacementEditor>
CLightPlacementEditor::Create(CLightManager* pLightManager)
{
	if (!pLightManager)
		return nullptr;

	return ToUPtr(
		new CLightPlacementEditor{ pLightManager });
}
