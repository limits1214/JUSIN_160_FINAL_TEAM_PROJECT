#include "pch.h"
#include "TerrainGUI.h"

#include "DbgLineRender.h"
#include "GameInstance.h"
#include "Terrain.h"
#include "TerrainBrushController.h"
#include "TerrainPickingPass.h"
#include "Resources.h"
#include "EditorCommandManager.h"
#include "TerrainEditCommand.h"
#include "ScatterObjectsCommand.h"
#include "MapMeshCommandCommon.h"

NS_USING(Client)

namespace
{
	constexpr const char* PAYLOAD_MODEL_RESOURCE = "MAPEDITOR_MODEL_RESOURCE";
	struct ModelResourceDragPayload { char groupName[128]{}; char resourceName[128]{}; };
}

E::_float CTerrainGUI::NoiseFade(E::_float value)
{
	return value * value * value * (value * (value * 6.f - 15.f) + 10.f);
}

uint32_t CTerrainGUI::NoiseHash(int32_t x, int32_t z, uint32_t seed)
{
	uint32_t hash = seed;
	hash ^= static_cast<uint32_t>(x) * 0x9E3779B9u;
	hash ^= static_cast<uint32_t>(z) * 0x85EBCA6Bu;
	hash ^= hash >> 16u;
	hash *= 0x7FEB352Du;
	hash ^= hash >> 15u;
	hash *= 0x846CA68Bu;
	hash ^= hash >> 16u;
	return hash;
}

E::_float CTerrainGUI::GradientDot(
	int32_t gridX, int32_t gridZ, E::_float x, E::_float z, uint32_t seed)
{
	const E::_float offsetX = x - static_cast<E::_float>(gridX);
	const E::_float offsetZ = z - static_cast<E::_float>(gridZ);
	constexpr E::_float diagonal = 0.70710678f;
	E::_float gradientX = 0.f;
	E::_float gradientZ = 0.f;

	switch (NoiseHash(gridX, gridZ, seed) & 7u)
	{
	case 0: gradientX = 1.f; break;
	case 1: gradientX = -1.f; break;
	case 2: gradientZ = 1.f; break;
	case 3: gradientZ = -1.f; break;
	case 4: gradientX = diagonal; gradientZ = diagonal; break;
	case 5: gradientX = -diagonal; gradientZ = diagonal; break;
	case 6: gradientX = diagonal; gradientZ = -diagonal; break;
	default: gradientX = -diagonal; gradientZ = -diagonal; break;
	}

	return gradientX * offsetX + gradientZ * offsetZ;
}

E::_float CTerrainGUI::Perlin2D(E::_float x, E::_float z, uint32_t seed)
{
	const int32_t x0 = static_cast<int32_t>(std::floor(x));
	const int32_t z0 = static_cast<int32_t>(std::floor(z));
	const int32_t x1 = x0 + 1;
	const int32_t z1 = z0 + 1;
	const E::_float u = NoiseFade(x - static_cast<E::_float>(x0));
	const E::_float v = NoiseFade(z - static_cast<E::_float>(z0));
	const E::_float top = std::lerp(GradientDot(x0, z0, x, z, seed), GradientDot(x1, z0, x, z, seed), u);
	const E::_float bottom = std::lerp(GradientDot(x0, z1, x, z, seed), GradientDot(x1, z1, x, z, seed), u);

	return std::lerp(top, bottom, v);
}

E::_float CTerrainGUI::FractalPerlin2D(E::_float worldX, E::_float worldZ,
	uint32_t seed, E::_float noiseScale, int octaves, E::_float persistence, E::_float lacunarity)
{
	E::_float frequency = 1.f / std::max(noiseScale, 0.001f);
	E::_float amplitude = 1.f;
	E::_float result = 0.f;
	E::_float totalAmplitude = 0.f;
	for (int octave = 0; octave < octaves; ++octave)
	{
		result += Perlin2D(worldX * frequency, worldZ * frequency, seed + static_cast<uint32_t>(octave) * 1013u) * amplitude;
		totalAmplitude += amplitude;
		amplitude *= persistence;
		frequency *= lacunarity;
	}
	return totalAmplitude > 0.f ? result / totalAmplitude : 0.f;
}

HRESULT CTerrainGUI::GenerateTerrainNoise(E::CTerrain& terrain, uint32_t seed,
	E::_float noiseScale, E::_float amplitude, E::_float baseHeight, int octaves,
	E::_float persistence, E::_float lacunarity, E::_bool additive)
{
	const uint32_t countX = terrain.GetVertexCountX();
	const uint32_t countZ = terrain.GetVertexCountZ();
	const E::_float spacing = terrain.GetVertexSpacing();
	if (countX == 0 || countZ == 0)
		return E_FAIL;

	for (uint32_t z = 0; z < countZ; ++z)
	{
		for (uint32_t x = 0; x < countX; ++x)
		{
			const E::_float noise = FractalPerlin2D(static_cast<E::_float>(x) * spacing, static_cast<E::_float>(z) * spacing, seed, noiseScale, octaves, persistence, lacunarity);
			const E::_float noiseHeight = noise * amplitude;
			const E::_float nextHeight = additive ? terrain.GetVertexHeight(x, z) + noiseHeight : baseHeight + noiseHeight;

			if (FAILED(terrain.SetVertexHeight(x, z, nextHeight)))
				return E_FAIL;
		}
	}

	return terrain.CommitAllHeights();
}

void CTerrainGUI::UpdateGUI(E::_float fTimeDelta)
{
	auto finishEditCommand = [&]()
	{
		if (!m_pActiveEditCommand) 
			return;

		if (m_pActiveEditCommand->Finalize() && m_pCommandManager)
			m_pCommandManager->RecordExecuted(std::move(m_pActiveEditCommand));
		else
			m_pActiveEditCommand.reset();
	};
	auto finishScatterCommand = [&]()
	{
		if (!m_ScatterSnapshots.empty() && m_pCommandManager)
			m_pCommandManager->RecordExecuted(std::make_unique<CScatterObjectsCommand>(std::move(m_ScatterSnapshots), std::move(m_ScatterHandles)));

		m_ScatterSnapshots.clear();
		m_ScatterHandles.clear();
		m_PreviousScatterHit.reset();
	};
	auto* selectedHandle = GetSelectedHandle();
	if (!selectedHandle)
	{
		finishEditCommand();
		finishScatterCommand();
		return;
	}
	auto* terrain = E::CGameInstance::Get().GetGameObjectByHandleT<E::CTerrain>(*selectedHandle);
	if (!terrain)
	{
		finishEditCommand();
		finishScatterCommand();
		return;
	}

	ImGui::Separator();
	ImGui::TextDisabled("Terrain");
	ImGui::Text("Chunks: %u / %u visible", terrain->GetVisibleChunkCount(), static_cast<uint32_t>(terrain->GetChunks().size()));
	ImGui::InputText("Terrain Data", m_TerrainDataPath, IM_ARRAYSIZE(m_TerrainDataPath));
	if (ImGui::Button("Save Terrain"))
		m_TerrainIOStatus = SUCCEEDED(terrain->SaveTerrain(m_TerrainDataPath)) ? "Saved" : "Save failed";
	ImGui::SameLine();

	if (ImGui::Button("Load Terrain"))
		m_TerrainIOStatus = SUCCEEDED(terrain->LoadTerrain(m_TerrainDataPath)) ? "Loaded" : "Load failed";
	if (!m_TerrainIOStatus.empty())
		ImGui::SameLine(), ImGui::TextUnformatted(m_TerrainIOStatus.c_str());
	if (ImGui::Button("Add Chunk +X"))
		terrain->AddChunkPositiveX();
	ImGui::SameLine();

	if (ImGui::Button("Add Chunk +Z"))
		terrain->AddChunkPositiveZ();
	if (ImGui::Button("Add Chunk -X"))
		terrain->AddChunkNegativeX();
	ImGui::SameLine();

	if (ImGui::Button("Add Chunk -Z"))
		terrain->AddChunkNegativeZ();
	if (ImGui::Checkbox("Sculpt", &m_bSculptEnabled) && m_bSculptEnabled)
		m_bTexturePaintEnabled = m_bScatterEnabled = false;
	ImGui::SameLine();

	if (ImGui::Checkbox("Paint Texture", &m_bTexturePaintEnabled) && m_bTexturePaintEnabled)
		m_bSculptEnabled = m_bScatterEnabled = false;
	ImGui::SameLine();

	if (ImGui::Checkbox("Scatter", &m_bScatterEnabled) && m_bScatterEnabled)
		m_bSculptEnabled = m_bTexturePaintEnabled = false;
	ImGui::Checkbox("GPU Picking Debug", &m_bPickingDebug);

	auto& brush = m_pBrushController->GetSettings();
	int brushMode = static_cast<int>(brush.mode);
	constexpr const char* brushModes[] = { "Raise", "Lower", "Flatten", "Smooth", "Noise" };

	if (ImGui::Combo("Sculpt Mode", &brushMode, brushModes, IM_ARRAYSIZE(brushModes)))
		brush.mode = static_cast<ETerrainBrushMode>(brushMode);

	ImGui::SliderFloat("Brush Radius", &brush.radius, 0.5f, 50.f, "%.1f");
	ImGui::SliderFloat("Brush Strength", &brush.strength, 0.1f, 30.f, "%.1f");
	ImGui::SliderFloat("Brush Falloff", &brush.falloff, 0.1f, 8.f, "%.1f");

	if (ImGui::CollapsingHeader("Terrain Noise Generator", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::InputInt("Noise Seed", &m_iNoiseSeed);
		ImGui::DragFloat("Noise Scale", &m_fNoiseScale, 1.f, 1.f, 1000.f, "%.1f");
		ImGui::DragFloat("Noise Amplitude", &m_fNoiseAmplitude, 0.5f, 0.f, 500.f, "%.1f");
		ImGui::DragFloat("Base Height", &m_fNoiseBaseHeight, 0.5f, -500.f, 500.f, "%.1f");
		ImGui::SliderInt("Noise Octaves", &m_iNoiseOctaves, 1, 8);
		ImGui::SliderFloat("Persistence", &m_fNoisePersistence, 0.1f, 0.9f, "%.2f");
		ImGui::SliderFloat("Lacunarity", &m_fNoiseLacunarity, 1.1f, 4.f, "%.2f");
		ImGui::Checkbox("Add To Existing Height", &m_bNoiseAdditive);

		if (ImGui::Button("Generate Terrain Noise"))
		{
			const HRESULT result = GenerateTerrainNoise(
				*terrain,
				static_cast<uint32_t>(m_iNoiseSeed),
				m_fNoiseScale,
				m_fNoiseAmplitude,
				m_fNoiseBaseHeight,
				m_iNoiseOctaves,
				m_fNoisePersistence,
				m_fNoiseLacunarity,
				m_bNoiseAdditive);
			m_NoiseStatus = SUCCEEDED(result) ? "Generated" : "Generation failed";
		}
		if (!m_NoiseStatus.empty())
		{
			ImGui::SameLine();
			ImGui::TextUnformatted(m_NoiseStatus.c_str());
		}
	}
	if (m_bScatterEnabled)
	{
		ImGui::Text("Model: %s", m_ScatterModelTag.empty() ? "Drop model here" : m_ScatterModelTag.c_str());
		ImGui::Button("Drop Static Model", { -1.f, 42.f });
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(PAYLOAD_MODEL_RESOURCE))
			{
				const auto* model = static_cast<const ModelResourceDragPayload*>(payload->Data);
				if (model) { m_ScatterModelGroup = model->groupName; m_ScatterModelTag = model->resourceName; }
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SliderInt("Objects Per Stamp", &m_iScatterCount, 1, 100);
		ImGui::SliderFloat("Stamp Spacing", &m_fScatterSpacing, 0.5f, 30.f, "%.1f");
		ImGui::DragFloatRange2("Random Scale", &m_fScatterScaleMin, &m_fScatterScaleMax, 0.01f, 0.05f, 5.f, "Min %.2f", "Max %.2f");
		ImGui::Checkbox("Random Yaw", &m_bScatterRandomYaw);

		ImGui::Separator();
		ImGui::TextUnformatted("Scatter Wind");
		ImGui::PushID("ScatterWind");
		int32_t windType = static_cast<int32_t>(m_ScatterWindDesc.type);
		constexpr const char* windTypeNames[] = { "None", "Grass", "Tree" };
		if (ImGui::Combo("Wind Type", &windType, windTypeNames, static_cast<int32_t>(std::size(windTypeNames))))
			m_ScatterWindDesc.type = static_cast<E::EWindType>(windType);
		ImGui::DragFloat("Strength", &m_ScatterWindDesc.strength, 0.01f, 0.f, 10.f, "%.3f");
		ImGui::DragFloat("Speed", &m_ScatterWindDesc.speed, 0.01f, 0.f, 10.f, "%.3f");
		ImGui::DragFloat("Frequency", &m_ScatterWindDesc.frequency, 0.01f, 0.f, 10.f, "%.3f");
		ImGui::DragFloat("Bend Exponent", &m_ScatterWindDesc.bendExponent, 0.05f, 0.1f, 8.f, "%.3f");
		ImGui::DragFloatRange2(
			"Height Weight Range",
			&m_ScatterWindDesc.heightStart,
			&m_ScatterWindDesc.heightEnd,
			0.01f,
			0.f,
			1.f,
			"Start %.2f",
			"End %.2f");
		ImGui::PopID();
	}

	if (m_bTexturePaintEnabled)
	{
		int selectedLayer = static_cast<int>(brush.tileLayer);
		ImGui::TextUnformatted("Paint Layer");
		for (int layer = 0; layer < 4; ++layer)
		{
			if (layer > 0) 
				ImGui::SameLine();

			const std::string label = "Layer " + std::to_string(layer);
			ImGui::RadioButton(label.c_str(), &selectedLayer, layer);
		}
		brush.tileLayer = static_cast<uint32_t>(selectedLayer);

		for (uint32_t layer = 0; layer < 4; ++layer)
		{
			ImGui::PushID(static_cast<int>(layer));
			auto current = terrain->GetTileTexture(layer);
			const std::string preview = current ? std::filesystem::path(current->GetPath()).filename().string() : "None";
			ImGui::Separator();
			ImGui::Text("Tile %u%s", layer, brush.tileLayer == layer ? " (Paint)" : "");
			if (current && current->GetState() == E::CResource::STATE::LOADED && current->GetSRV())
			{
				const auto& textureDesc = current->GetTexture2DDesc();
				constexpr float thumbnailExtent = 64.f;
				const float aspect = textureDesc.Height > 0
					? static_cast<float>(textureDesc.Width) / static_cast<float>(textureDesc.Height)
					: 1.f;
				const ImVec2 thumbnailSize = aspect >= 1.f
					? ImVec2{ thumbnailExtent, thumbnailExtent / aspect }
					: ImVec2{ thumbnailExtent * aspect, thumbnailExtent };
				ImGui::Image(
					reinterpret_cast<ImTextureID>(current->GetSRV().Get()), thumbnailSize);
			}
			else
			{
				ImGui::Dummy({ 64.f, 64.f });
			}
			ImGui::SameLine();
			ImGui::BeginGroup();
			ImGui::TextWrapped("%s", current ? current->GetPath().c_str() : "No texture selected");
			ImGui::TextDisabled("%s", preview.c_str());
			ImGui::EndGroup();
			ImGui::PopID();
		}

		ImGui::Separator();
		ImGui::Text("Available Tiles -> Layer %u", brush.tileLayer);
		const auto groups = E::CGameInstance::Get().GetResources();
		const auto groupIt = groups.find(E::StringID{ "MAPEDITOR_TERRAIN_TILE" });
		if (groupIt == groups.end())
		{
			ImGui::TextDisabled("MAPEDITOR_TERRAIN_TILE is empty");
		}
		else
		{
			constexpr float tileSize = 72.f;
			constexpr float tilePanelHeight = 260.f;
			ImGui::BeginChild("TerrainTilePalette", { 0.f, tilePanelHeight }, true,
				ImGuiWindowFlags_AlwaysVerticalScrollbar);
			const float cellWidth = tileSize + ImGui::GetStyle().ItemSpacing.x;
			const int columnCount = std::max(1,
				static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));
			int tileIndex = 0;
			for (const auto& [resourceId, list] : groupIt->second)
			{
				for (const auto& resource : list)
				{
					if (!resource || !resource->IsA(E::CResTexture2D::StaticType)) continue;
					auto texture = std::static_pointer_cast<E::CResTexture2D>(resource);
					if (texture->GetState() != E::CResource::STATE::LOADED || !texture->GetSRV()) continue;
					ImGui::PushID(texture.get());
					if (ImGui::ImageButton(reinterpret_cast<ImTextureID>(texture->GetSRV().Get()),
						{ tileSize, tileSize }))
						terrain->SetTileTexture(brush.tileLayer, texture);
					if (ImGui::IsItemHovered())
						ImGui::SetTooltip("%s", texture->GetPath().c_str());
					ImGui::PopID();
					++tileIndex;
					if (tileIndex % columnCount != 0)
						ImGui::SameLine();
				}
			}
			ImGui::EndChild();
		}
	}

	if (!m_bPickingDebug && !m_bSculptEnabled && !m_bTexturePaintEnabled && !m_bScatterEnabled)
	{
		m_PickedPosition.reset();
		m_pBrushController->EndStroke();
		finishEditCommand();
		finishScatterCommand();
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	const float brushTimeDelta = fTimeDelta > 0.f ? fTimeDelta : io.DeltaTime;
	const E::_float2 mouse = E::CGameInstance::Get().GetMousePos();
	const E::_float2 clientSize = E::CGameInstance::Get().GetClientScreenSize();
	const bool mouseInClient = mouse.x >= 0.f && mouse.y >= 0.f &&
		mouse.x < clientSize.x && mouse.y < clientSize.y;
	if (!io.WantCaptureMouse && mouseInClient && !ImGuizmo::IsOver() && m_pPickingPass)
	{
		m_PickedPosition = m_pPickingPass->Pick(
			*terrain, static_cast<uint32_t>(mouse.x), static_cast<uint32_t>(mouse.y));
	}
	else if (!mouseInClient)
	{
		m_PickedPosition.reset();
	}

	if (!m_PickedPosition)
	{
		ImGui::TextUnformatted("Hit: none");
		m_pBrushController->EndStroke();
		finishEditCommand();
		finishScatterCommand();
		return;
	}

	const auto& hit = *m_PickedPosition;
	ImGui::Text("Hit: %.2f, %.2f, %.2f", hit.x, hit.y, hit.z);
	if (m_bSculptEnabled)
	{
		m_pBrushController->DrawPreview(*terrain, hit);
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
		{
			if (!m_pActiveEditCommand) m_pActiveEditCommand = std::make_unique<CTerrainEditCommand>(terrain);
			m_pActiveEditCommand->CaptureHeightBefore(hit, brush.radius);
			m_pBrushController->UpdateStroke(*terrain, hit, brushTimeDelta);
		}
		else
		{
			m_pBrushController->EndStroke();
			finishEditCommand();
		}
	}
	else if (m_bTexturePaintEnabled)
	{
		m_pBrushController->DrawPreview(*terrain, hit);
		if (!io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
		{
			if (!m_pActiveEditCommand) m_pActiveEditCommand = std::make_unique<CTerrainEditCommand>(terrain);
			m_pActiveEditCommand->CaptureMaskBefore(hit, brush.radius);
			m_pBrushController->UpdateTextureStroke(*terrain, hit, brushTimeDelta);
		}
		else
		{
			m_pBrushController->EndStroke();
			finishEditCommand();
		}
	}
	else if (m_bScatterEnabled)
	{
		m_pBrushController->DrawPreview(*terrain, hit);
		const bool painting = !io.WantCaptureMouse && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver() && !ImGuizmo::IsUsing() && !m_ScatterModelTag.empty();
		if (painting)
		{
			const float dx = m_PreviousScatterHit ? hit.x - m_PreviousScatterHit->x : 0.f;
			const float dz = m_PreviousScatterHit ? hit.z - m_PreviousScatterHit->z : 0.f;

			if (!m_PreviousScatterHit || std::sqrt(dx * dx + dz * dz) >= m_fScatterSpacing)
			{
				static std::mt19937 rng{ std::random_device{}() };
				std::uniform_real_distribution<float> unit(0.f, 1.f);
				const E::_matrix inverseWorld = XMMatrixInverse(nullptr,terrain->GetTransform().GetLoadedCombinedWorldMatrix());
				E::_float3 localCenter{};
				XMStoreFloat3(&localCenter, XMVector3TransformCoord(XMLoadFloat3(&hit), inverseWorld));
				const auto& terrainScale = terrain->GetTransform().GetScale();

				for (int index = 0; index < m_iScatterCount; ++index)
				{
					const float angle = unit(rng) * XM_2PI;
					const float distance = std::sqrt(unit(rng)) * brush.radius;
					const float localX = localCenter.x + std::cos(angle) * distance / std::max(std::abs(terrainScale.x), 0.0001f);
					const float localZ = localCenter.z + std::sin(angle) * distance / std::max(std::abs(terrainScale.z), 0.0001f);
					float localHeight = 0.f;

					if (!terrain->TryGetLocalHeight(localX, localZ, localHeight)) 
						continue;

					E::_float3 worldPosition{};
					XMStoreFloat3(&worldPosition, XMVector3TransformCoord(
						XMVectorSet(localX, localHeight, localZ, 1.f),
						terrain->GetTransform().GetLoadedCombinedWorldMatrix()));

					const float objectScale = std::lerp(m_fScatterScaleMin, m_fScatterScaleMax, unit(rng));
					E::_float4 rotation{ 0.f, 0.f, 0.f, 1.f };

					if (m_bScatterRandomYaw)
						XMStoreFloat4(&rotation, XMQuaternionRotationAxis(terrain->GetTransform().GetState(E::STATE::UP), unit(rng) * XM_2PI));

					MAPMESH_OBJECT_SNAPSHOT snapshot{};
					snapshot.objectTag = "TerrainScatter_" + std::to_string(m_ScatterSnapshots.size());
					snapshot.modelGroupTag = m_ScatterModelGroup;
					snapshot.modelResTag = m_ScatterModelTag;
					snapshot.layerTag = E::MAPMESHOBJECTLAYER;
					snapshot.position = worldPosition;
					snapshot.rotation = rotation;
					snapshot.scale = { objectScale, objectScale, objectScale };
					snapshot.windDesc = m_ScatterWindDesc;
					if (auto handle = SpawnMapMeshObject(snapshot))
					{
						m_ScatterSnapshots.push_back(snapshot);
						m_ScatterHandles.push_back(*handle);
					}
				}
				m_PreviousScatterHit = hit;
			}
		}
		else
		{
			finishScatterCommand();
		}
	}
	if (auto* debugDraw = E::CGameInstance::Get().GetDbgLineRender())
	{
		constexpr float markerSize = 0.5f;
		debugDraw->SetColor({ 1.f, 0.2f, 0.1f, 1.f });
		debugDraw->AddLine({ hit.x - markerSize, hit.y, hit.z }, { hit.x + markerSize, hit.y, hit.z });
		debugDraw->AddLine({ hit.x, hit.y - markerSize, hit.z }, { hit.x, hit.y + markerSize, hit.z });
		debugDraw->AddLine({ hit.x, hit.y, hit.z - markerSize }, { hit.x, hit.y, hit.z + markerSize });
	}
}

E::UPtr<CTerrainGUI> CTerrainGUI::Create(E::CHandle* selectedObject, CEditorCommandManager* commandManager)
{
	auto instance = E::ToUPtr(new CTerrainGUI{});
	if (FAILED(instance->Initialize(selectedObject)))
		return nullptr;

	instance->m_pPickingPass = CTerrainPickingPass::Create();
	if (!instance->m_pPickingPass)
		return nullptr;

	instance->m_pBrushController = CTerrainBrushController::Create();
	if (!instance->m_pBrushController)
		return nullptr;

	instance->m_pCommandManager = commandManager;
	return instance;
}
