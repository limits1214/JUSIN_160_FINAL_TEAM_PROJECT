// ParticlePattern.h
#pragma once
#include "Engine_Defines.h"
NS_BEGIN(Engine)

namespace ParticlePattern
{
	//10. 패턴 만들기

	std::vector<PARTICLE_SPAWN_DATA> MakeStairs(const SStairsParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeCircle(const SCircleParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeCircleAndSpread(const SCircleSpreadParam& param);
	std::vector<PARTICLE_SPAWN_DATA> MakeSpiral(const SSpiralParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeStraightGround(const SStraightGroundParam& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeSmoke(const SMOKE& param);
	std::vector<PARTICLE_SPAWN_DATA> MakeLightning(const SLightning& p);
	std::vector<PARTICLE_SPAWN_DATA> MakeCone(const SConeParam& param);

}
NS_END
