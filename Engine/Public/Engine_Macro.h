#pragma once

#ifdef	ENGINE_EXPORTS
#define ENGINE_DLL		_declspec(dllexport)
#else
#define ENGINE_DLL		_declspec(dllimport)
#endif

//#define CHECK_HR(hr) \
//    if (FAILED(hr)) { \
//        wchar_t szBuffer[512]; \
//        swprintf_s(szBuffer, L"File: %s\nLine: %d\nError: 0x%08X", \
//                   TEXT(__FILE__), __LINE__, hr); \
//        MessageBox(NULL, szBuffer, L"Render Error", MB_OK); \
//        return hr; \
//    }

#ifdef _DEBUG
#define LOG_MEMORY(...) LogMemoryUsageImpl(__FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#else
#define LOG_MEMORY(...) ((void)0)
#endif

#ifdef _DEBUG
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() ((void)0)
#endif

#ifdef _DEBUG
#define DEBUG_LOG(_message) \
	do { OutputDebugStringA((_message)); } while (0)
#define DEBUG_LOG_STR(_message) \
	do { OutputDebugStringA((_message).c_str()); } while (0)
#else
#define DEBUG_LOG(_message) ((void)0)
#define DEBUG_LOG_STR(_message) ((void)0)
#endif

#define CHECK_HR(hr, fmt, ...) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            wchar_t szFinalMsg[1024]; \
            wchar_t szUserMsg[512]; \
            /* 유저가 입력한 커스텀 메시지 구성 */ \
            swprintf_s(szUserMsg, fmt, ##__VA_ARGS__); \
            /* 위치 정보와 합치기 */ \
            swprintf_s(szFinalMsg, L"Message: %s\n\nFile: %s\nLine: %d\nError: 0x%08X", \
                       szUserMsg, TEXT(__FILE__), __LINE__, _hr); \
            MessageBox(NULL, szFinalMsg, L"System Error", MB_OK | MB_ICONERROR); \
            return _hr; \
        } \
    } while (0)

#define CHECK_HR_NULL(hr, fmt, ...) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            wchar_t szFinalMsg[1024]; \
            wchar_t szUserMsg[512]; \
            /* 유저가 입력한 커스텀 메시지 구성 */ \
            swprintf_s(szUserMsg, fmt, ##__VA_ARGS__); \
            /* 위치 정보와 합치기 */ \
            swprintf_s(szFinalMsg, L"Message: %s\n\nFile: %s\nLine: %d\nError: 0x%08X", \
                       szUserMsg, TEXT(__FILE__), __LINE__, _hr); \
            MessageBox(NULL, szFinalMsg, L"System Error", MB_OK | MB_ICONERROR); \
            return nullptr; \
        } \
    } while (0)

#ifndef			MSG_BOX
#define			MSG_BOX(_message)			MessageBox(NULL, TEXT(_message), L"System Message", MB_OK)
#define			MSG_BOX_STR(_message)			MessageBox(NULL, _message, L"System Message", MB_OK)
#endif

#define			NS_BEGIN(NAMESPACE)		namespace NAMESPACE {
#define			NS_END						}

#define			NS_USING(NAMESPACE)	using namespace NAMESPACE;

//#define NO_COPY(CLASSNAME)											\
//			private:												\
//			CLASSNAME(const CLASSNAME&) = delete;					\
//			CLASSNAME& operator = (const CLASSNAME&) = delete;		
//
//#define DECLARE_SINGLETON(CLASSNAME)								\
//			NO_COPY(CLASSNAME)										\
//		public:														\
//			static CLASSNAME& Get(void) {							\
//			static CLASSNAME Instance;								\
//			return Instance;										\
//		}

#define NODE_ACTION_M    \
X(ACTION)\
X(ANIMATION)            \
X(DECORATOR)            \
X(EFFECT)               \
X(SELECTOR)\
X(SEQUENCE)\
X(ROOT)\
X(RAND_SELECTOR)\
X(END)

#define MOVE_M    \
X(LEFT)			     \
X(RIGHT)            \
X(STRAIGHT)            \
X(BACKWARD)               \
X(UP)					\
X(BU)					\
X(DOWN)					\
X(END)

#define BTFLAG_M		\
X(NOCKDOWN,0x0000001)\
X(HIT,0x0000002)\
X(ATTACK,0x0000004)\
X(ABORT,0x0000008)\
X(SUPERARMOR,0x0000010)\
X(THROW,0x0000020)\
X(DEAD,0x0000040)\
X(EMISSIVE,0x0000080)\
X(DEBRIS, 0x0000100)\
X(DISSOLVE, 0x0000200)\
X(DROP,0x0000400)\
X(ENDHIT, 0x0000800)\
X(EFFECT, 0x0001000)\
X(LOOP, 0x0002000)\
X(GROGY,   0x0004000)\

/////// -- Light / LightManager -- ///////
#define MAX_NORMAL_LIGHT_RENDER_COUNT	16
#define MAX_SHADOW_LIGHT_RENDER_COUNT	8

#define	MAX_POINT_SHADOW_ACTIVE_COUNT	2
#define	MAX_SPOT_SHADOW_ACTIVE_COUNT	3

#define POINT_SHADOW_MAPCOUNT			6

#define MAX_EFFECT_LIGHT_RENDER_COUNT	15
#define MAX_CASCADE_COUNT				4

#define POINT_SHADOW_MAPSIZE			512
#define SPOT_SHADOW_MAPSIZE				512
#define CSM_SHADOW_MAPSIZE				2048
/////////////////////////////////////////
