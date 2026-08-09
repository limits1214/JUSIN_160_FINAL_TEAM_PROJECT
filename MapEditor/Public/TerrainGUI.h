#pragma once

#include "GUIWindow.h"
#include "TerrainEditCommand.h"
#include "MapMeshCommandCommon.h"

NS_BEGIN(Client)

class CTerrainPickingPass;
class CTerrainBrushController;
class CEditorCommandManager;

class CTerrainGUI final : public CGUIWindow
{
public:
	DECLARE_DERIVED_TYPE(CTerrainGUI, CGUIWindow)

private:
	CTerrainGUI() = default;
	~CTerrainGUI() override = default;

public:
	void UpdateGUI(E::_float fTimeDelta) override;
	bool IsSculptEnabled() const { return m_bSculptEnabled || m_bTexturePaintEnabled || m_bScatterEnabled; }
	static E::UPtr<CTerrainGUI> Create(E::CHandle* selectedObject, CEditorCommandManager* commandManager);

private:
	_float NoiseFade(_float value);
	uint32_t NoiseHash(int32_t x, int32_t z, uint32_t seed);
	_float GradientDot(int32_t gridX, int32_t gridZ, _float x, _float z, uint32_t seed);
	_float Perlin2D(_float x, _float z, uint32_t seed);
	_float FractalPerlin2D(_float worldX, _float worldZ, uint32_t seed, _float noiseScale,
		int octaves, _float persistence, _float lacunarity);
	HRESULT GenerateTerrainNoise(E::CTerrain& terrain, uint32_t seed, _float noiseScale,
		_float amplitude, _float baseHeight, int octaves, _float persistence,
		_float lacunarity, _bool additive);

private:
	E::UPtr<CTerrainPickingPass> m_pPickingPass{};
	E::UPtr<CTerrainBrushController> m_pBrushController{};
	std::optional<E::_float3> m_PickedPosition{};
	bool m_bPickingDebug = false;
	bool m_bSculptEnabled = false;
	bool m_bTexturePaintEnabled = false;
	char m_TerrainDataPath[512] = "./Resources/json/MapSaved/LevelName/Terrain/terrain.json";
	std::string m_TerrainIOStatus{};
	CEditorCommandManager* m_pCommandManager = nullptr;
	std::unique_ptr<CTerrainEditCommand> m_pActiveEditCommand{};
	bool m_bScatterEnabled = false;
	int m_iScatterCount = 5;
	float m_fScatterSpacing = 3.f;
	float m_fScatterScaleMin = 0.8f;
	float m_fScatterScaleMax = 1.2f;
	bool m_bScatterRandomYaw = true;
	E::WIND_DESC m_ScatterWindDesc{};
	std::string m_ScatterModelGroup{};
	std::string m_ScatterModelTag{};
	std::optional<E::_float3> m_PreviousScatterHit{};
	std::vector<MAPMESH_OBJECT_SNAPSHOT> m_ScatterSnapshots{};
	std::vector<E::CHandle> m_ScatterHandles{};

private:
	int m_iNoiseSeed = 1337;
	_float m_fNoiseScale = 60.f;
	_float m_fNoiseAmplitude = 15.f;
	_float m_fNoiseBaseHeight = 0.f;
	int m_iNoiseOctaves = 5;
	_float m_fNoisePersistence = 0.5f;
	_float m_fNoiseLacunarity = 2.f;
	_bool m_bNoiseAdditive = false;
	std::string m_NoiseStatus{};
};

NS_END
