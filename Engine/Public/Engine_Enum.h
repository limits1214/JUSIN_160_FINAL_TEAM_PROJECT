#pragma once

namespace Engine
{
	enum class WINMODE { FULL, WIN };
	enum class NVCLOTH_BACKEND { CPU, DX11 };
	enum class MOUSEKEYSTATE { LB, RB, MB, END };
	enum class MOUSEMOVESTATE { X, Y, Z, END };
	enum class RENDERGROUP { PRIORITY, NONBLEND_INSTANCED, NONBLEND, BLEND, LIGHT, EFFECT, SKYBOX, COLLIDER, UI3D, UI, END };
	enum class RENDERPASS : uint32_t
	{
		DEFAULT = 1 << 0,	// 1
		SHADOW	= 1 << 1,	// 2
		DEPTH	= 1 << 2	// 4
	};
	enum class STATE { RIGHT, UP, LOOK, POSITION, END };
	enum class MODEL {STATIC, SKELETAL, END };
	enum class NODETYPE {START,NODE_END ,END};
	enum class BEHAVIOR {SELECTOR, SECQUNCE, DECORATOR, ACTION, RAND_SELECTOR,END};
	enum class CollType { Box, OrientedBox, Sphere, Frustum };
	//enum class VSYNC{ OFF, ON };

	enum class EVALUATE { SUCCESS, FAILED, RUN, END };
#define X(name) name,
	enum class NODEGROUP { NODE_ACTION_M };
	enum class MOVE { MOVE_M };
#undef X	
	enum class FLAGTYPE { ADD, DEL, RESET, INVERT };
	enum class LIGHT_TYPE { DIRECTIONAL, POINT, SPOTLIGHT };
	enum class TRAIL_TYPE { POINT,PLANE,END };
	enum class MESHORTEXTURE{ MESH, TEX, END};

	enum class ACTORTYPE { STATIC, DYNAMIC, END };
	enum class BLENDTYPE { ALPHABLEND, ALPHAADD, NONE, END};


	enum class UI_TYPE{ CONTAINER, TEXUI, FLIPBOOK, TEXT, BUTTON, SPELLMETER, HPBAR, HPFILL, LEFTHPFILL, MINIMAP, SPELLBTN, SHORTCUT_ICON, DISOLVE, GAMEOVERMASK, VIDEOOBJ, CURSOR, END };
	enum class UI_EFFECT_TYPE { NONE, HOVER, CLICK, END};
	enum class EUITweenTarget { SCALE, EFFECT_ALPHA, POSITION_X, POSITION_Y }; // 제어할 UI 속성 타입

	enum class B_SLOTNUMBER { PER_OBJECT, PER_PASS, BONES, MATERIAL, LIGHT, UI = 7, GPUPART = 9 };

	enum class EMapMeshRenderFeature : uint32_t { Static, Foliage };

	enum class EWindType : uint32_t { None, Grass, Tree };
}
