#pragma once

namespace Client
{
	static const unsigned int	g_iWinSizeX{ 1280 };
	static const unsigned int	g_iWinSizeY{ 720 };

}

extern HWND g_hWnd;
extern HINSTANCE g_hInstance;

#define DECLARE_SINGLE(classname)				\
private:										\
	classname() { }								\
public:											\
	static classname* GetInstance()				\
	{											\
		static classname s_instance;			\
												\
		return &s_instance;						\
	}

#define GET_SINGLE(classname) classname::GetInstance()
