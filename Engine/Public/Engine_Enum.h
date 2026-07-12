#pragma once

namespace Engine
{
	enum class WINMODE { FULL, WIN };
	enum class MOUSEKEYSTATE { LB, RB, MB, END };
	enum class MOUSEMOVESTATE { X, Y, Z, END };
	enum class RENDERGROUP { PRIORITY, NONBLEND, BLEND, LIGHT, SKYBOX, COLLIDER, PARTICLE, UI, END };
	enum class RENDERPASS : uint32_t
	{
		DEFAULT = 1 << 0,	// 1
		SHADOW	= 1 << 1,	// 2
		DEPTH	= 1 << 2	// 4
	};
	enum class STATE { RIGHT, UP, LOOK, POSITION, END };
	enum class MODEL {STATIC, SKELETAL, END };
	enum class NODETYPE {START,NODE_END ,END};
	enum class BEHAVIOR {SELECTOR, SECQUNCE, DECORATOR, ACTION,END};
	enum class CollType { Box, OrientedBox, Sphere, Frustum };
	//enum class VSYNC{ OFF, ON };

	enum class EVALUATE { SUCCESS, FAILED, RUN, END };
#define X(name) name,
	enum class NODEGROUP { NODE_ACTION_M };
	enum class MOVE { MOVE_M };
#undef X	
	enum class LIGHT_TYPE { DIRECTIONAL, POINT, SPOTLIGHT };
	enum class PARTICLE_TYPE { FIRE_CPU,FIRE_GPU,RIBBON,TRAIL,END };
	enum class TRAIL_TYPE { POINT,PLANE,END };
	enum class MESHORTEXTURE{ MESH, TEX, END};

	//나중에 이 성 민 씨 가 옮길거임 접근 금지
	static const uint32_t MAX_SPAWN_PER_CALL = 2000;

	enum class UI_TYPE{ CONTAINER, TEXUI, FLIPBOOK, TEXT, BUTTON, END };
	enum class UI_EFFECT_TYPE { NONE, HOVER, CLICK, END};
	enum class EUITweenTarget { SCALE, EFFECT_ALPHA, POSITION_X, POSITION_Y }; // 제어할 UI 속성 타입

}
