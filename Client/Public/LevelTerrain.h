#pragma once

#include "Client_Defines.h"
#include "Level.h"

NS_BEGIN(Client)
class CLevelTerrain final : public Engine::CLevel
{

private:
	explicit CLevelTerrain();
	~CLevelTerrain() override;

public:
	HRESULT Initialize() override;
	void Update(E::_float fTimeDelta) override;
	HRESULT Render() override;
	void UpdateGUI() override;

public:
	void Picking();
private:
	void		Resources();
	void		Objects();
	void		BeHaviors();

	HRESULT SpawnPlayerCamera(std::optional<CHandle> hPlayer);
	std::optional<CHandle> SpawnPlayer();
	HRESULT InitializeJointTests(
		CHandle hPlayer,
		const std::array<CHandle, 6>& hOilBarrels);
	HRESULT InitializeCamerasAndLighting(
		const std::optional<CHandle>& hPlayer);
	HRESULT InitializePathPlaybackTests();
	HRESULT InitializeTombBossBulletTest(CHandle hPlayer);
	HRESULT InitializeOilBarrelPool();

	HRESULT SpawnMonster(const std::optional<CHandle>& hPlayer);
public:
	static Engine::UPtr<CLevelTerrain> Create();

private:
	_float3		m_fPos{};
	_bool		m_bSpawn{ false };
	int32_t  m_iResourceSelect{ 0 }, m_iObjSelect{ 0 };
	const	_string m_strLevelName = { "LEVEL_CREATURE" };
	_string	m_SelectResourceTag{}, m_SelectObjecteTag{}, m_SelectFileName{}, m_SelectFilePath{};

	std::map<_string, _string>		m_BeHaviorJsonList;
	std::vector<CHandle>				m_MedDebrisHandles{};

private:
	_bool m_bCreatePlayScreenUI{ false };
	CHandle m_hPlayer{};
	_float m_fTombBossBulletSpawnYawDegrees{};
	_float3 m_vOilBarrelPoolSpawnPosition{ 10.f, 100.f, 10.f };

private:
	HRESULT InitializeMyMagicSquareStep();
private:
	void Free() override;
};

NS_END

