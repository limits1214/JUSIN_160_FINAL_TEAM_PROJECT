#include "pch.h"
#include "LuaManager.h"
#include "GameInstance.h"
#include "LuaWatcher.h"
#include "ComLuaScript.h"
#include "GameObject.h"
#include "ComTransform.h"
#include "ResLuaScript.h"
#include "UIObject.h"
#include "CameraObject.h"
#include "FlyCamera.h"
#include "UICamera.h"
#include "CollFrustum.h"
#include "CollSphere.h"
#include "CollBox.h"
#include "CollOrientedBox.h"
#include "DbgLineRender.h"
NS_USING(Engine)

CLuaManager::CLuaManager()
{
}

CLuaManager::~CLuaManager()
{
}

void CLuaManager::UpdateGUI()
{
	ImGui::Begin("CLuaManager");
	
	if (ImGui::Button("script test"))
	{
		CGameInstance::Get().LuaScriptExecute(R"( print("Hello Lua") )");
	}
	if (ImGui::Button("GetValue"))
	{
		std::string a;
		CGameInstance::Get().LuaGetValue("test", a);

		int x = 0;
	}

	if (ImGui::Button("SetValue"))
	{
		CGameInstance::Get().LuaSetValue("test", "18.f");
	}
	ImGui::End();
}

void CLuaManager::Update(_float fTimeDelta)
{
	UpdateHotReload();
}

HRESULT CLuaManager::Initialize()
{
#ifdef _DEBUG
	m_pLuaWatcher = CLuaWatcher::Create();
#endif // _DEBUG

	

	// Lua 기본 라이브러리
	m_Lua.open_libraries(
		sol::lib::base,
		sol::lib::package,
		sol::lib::coroutine,
		sol::lib::string,
		sol::lib::os,
		sol::lib::math,
		sol::lib::table,
		sol::lib::utf8
	);
	
	Initialize_PrintBinding();

	// 디버거 시도하다 중단함
	if constexpr (false)
	{
		Initialize_DebuggerBinding();
	}

	m_Lua.script(R"(
		print("Hello Lua")
	)");

	// 엔진레벨 루아 타입 등록 
	// 이등록은 캐스팅할때 필요함
	Initialize_RegistType();
	
	Initialize_MathBinding();
	Initialize_DefineBinding();
	Initialize_ClassBindnig();
	Initialize_GameInstanceBindnig();



	
	
	return S_OK;
}

HRESULT CLuaManager::Initialize_PrintBinding()
{
	sol::protected_function tostring = m_Lua["tostring"];
	m_Lua.set_function("print",
		[tostring](sol::variadic_args args) mutable
		{
			std::ostringstream oss;
			oss << "[LuaManager] ";

			bool first = true;



			for (auto value : args)
			{
				if (!first)
					oss << '\t';

				first = false;

				auto result = tostring(value);

				if (result.valid())
					oss << result.get<std::string>();
				else
					oss << "<error>";
			}

			oss << '\n';

			OutputDebugStringA(oss.str().c_str());
		});
	return S_OK;
}

HRESULT CLuaManager::Initialize_RegistType()
{
	RegisterType<CGameObject>();
	RegisterType<CUIObject>();
	RegisterType<CCameraObject>();
	RegisterType<CUICamera>();
	RegisterType<CFlyCamera>();

	RegisterType<CComponent>();
	RegisterType<CComLuaScript>();
	RegisterType<CComCollider>();
	RegisterType<CComTransform>();

	RegisterType<CCollider>();
	RegisterType<CCollBox>();
	RegisterType<CCollSphere>();
	RegisterType<CCollFrustum>();
	RegisterType<CCollOrientedBox>();
	return S_OK;
}

HRESULT CLuaManager::Initialize_DebuggerBinding()
{
	if (false)
	{
		m_Lua.script("package.cpath = package.cpath .. ';./?.dll'");

		// 2. 디버거 로드 및 포트 대기 (위에서 추천한 방식)
		try
		{
			//m_Lua.require<sol::table>("emmy_core");

			// 포트 점유 에러를 방지하기 위해 pcall로 감싸서 실행
			m_Lua.script(R"(
            local dbg = require('emmy_core')
            local status = pcall(function() dbg.tcpListen('localhost', 9966) end)
            if status then
                -- 디버거 연결 대기 (필요할 때만 주석 해제)
                -- dbg.waitIDE() 
            end
        )");

			OutputDebugStringA("[Lua] EmmyLua Debugger Initialized.\n");
		}
		catch (const sol::error& e)
		{
			OutputDebugStringA(("[Lua] Debugger Load Failed: " + std::string(e.what()) + "\n").c_str());
		}
	}
	return S_OK;
}

HRESULT CLuaManager::Initialize_MathBinding()
{
#pragma region Vec2
	m_Lua.new_usertype<_float2>(
		"Vector2",

		sol::constructors<_float2(), _float2(float, float)>(),

		"x", &DirectX::XMFLOAT2::x,
		"y", &DirectX::XMFLOAT2::y
	);
#pragma endregion

#pragma region Vec3
	m_Lua.new_usertype<_float3>(
		"Vector3",
		sol::constructors<_float3(), _float3(float, float, float)>(),
		"x", &_float3::x,
		"y", &_float3::y,
		"z", &_float3::z,

		sol::meta_function::to_string, [](const _float3& v) {
			return "[ " + std::to_string(v.x) + ", "
				+ std::to_string(v.y) + ", "
				+ std::to_string(v.z) + " ]";
		},

		// SIMD 연산자
		sol::meta_function::addition, [](const _float3& a, const _float3& b) {
			_float3 res; XMStoreFloat3(&res, XMVectorAdd(XMLoadFloat3(&a), XMLoadFloat3(&b))); return res;
		},
		sol::meta_function::subtraction, [](const _float3& a, const _float3& b) {
			_float3 res; XMStoreFloat3(&res, XMVectorSubtract(XMLoadFloat3(&a), XMLoadFloat3(&b))); return res;
		},
		sol::meta_function::multiplication, sol::overload(
			[](const _float3& v, float s) { // 벡터 * 스칼라
				_float3 res; XMStoreFloat3(&res, XMVectorScale(XMLoadFloat3(&v), s)); return res;
			},
			[](const _float3& a, const _float3& b) { // 벡터 * 벡터 (내적 아님, 요소별 곱)
				_float3 res; XMStoreFloat3(&res, XMVectorMultiply(XMLoadFloat3(&a), XMLoadFloat3(&b))); return res;
			}
		),
		"Length", [](const _float3& v) { return XMVectorGetX(XMVector3Length(XMLoadFloat3(&v))); },
		"Normalize", [](const _float3& v) { _float3 res; XMStoreFloat3(&res, XMVector3Normalize(XMLoadFloat3(&v))); return res; }
	);
#pragma endregion
	
#pragma region Vec4
	m_Lua.new_usertype<_float4>(
		"Vector4",
		sol::constructors<_float4(), _float4(float, float, float, float)>(),
		"x", &_float4::x,
		"y", &_float4::y,
		"z", &_float4::z,
		"w", &_float4::w,

		sol::meta_function::addition, [](const _float4& a, const _float4& b) {
			_float4 res; XMStoreFloat4(&res, XMVectorAdd(XMLoadFloat4(&a), XMLoadFloat4(&b))); return res;
		},
		sol::meta_function::subtraction, [](const _float4& a, const _float4& b) {
			_float4 res; XMStoreFloat4(&res, XMVectorSubtract(XMLoadFloat4(&a), XMLoadFloat4(&b))); return res;
		},
		sol::meta_function::multiplication, [](const _float4& v, float s) {
			_float4 res; XMStoreFloat4(&res, XMVectorScale(XMLoadFloat4(&v), s)); return res;
		}
	);
#pragma endregion

#pragma region Matrix
	m_Lua.new_usertype<_float4x4>(
		"Matrix",
		sol::constructors<_float4x4()>(),

		// 행렬 곱셈 (Matrix * Matrix)
		sol::meta_function::multiplication, [](const _float4x4& a, const _float4x4& b) {
			_float4x4 res;
			XMStoreFloat4x4(&res, XMMatrixMultiply(XMLoadFloat4x4(&a), XMLoadFloat4x4(&b)));
			return res;
		},

		"Get", [](const _float4x4& mat, int row, int col) { return mat.m[row][col]; },
		"Set", [](_float4x4& mat, int row, int col, float value) { mat.m[row][col] = value; },
		"Transpose", [](const _float4x4& mat) {
			_float4x4 res;
			XMStoreFloat4x4(&res, XMMatrixTranspose(XMLoadFloat4x4(&mat)));
			return res;
		},
		"Inverse", [](const _float4x4& mat) {
			_float4x4 res;
			XMStoreFloat4x4(&res, XMMatrixInverse(nullptr, XMLoadFloat4x4(&mat)));
			return res;
		}
	);
#pragma endregion

	return S_OK;
}

HRESULT CLuaManager::Initialize_GameInstanceBindnig()
{
#pragma region GameObjectManager
	{
		// "Object"라는 네임스페이스(테이블) 생성
		sol::table objApi = m_Lua.create_named_table("Object");

		// 1. 핸들로 오브젝트 가져오기 (가장 기본)
		objApi.set_function("GetByHandle",
			[this](const CHandle& handle) -> sol::object {
				CGameObject* pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
				if (pObj == nullptr) return sol::nil;

				// 맵에서 찾아서 캐스팅 함수가 있는지 확인
				auto it = m_TypeRegistry.find(StringID{ pObj->GetType() });
				if (it != m_TypeRegistry.end()) {
					// 등록된 타입이라면 전용 타입으로 캐스팅하여 반환
					return it->second(pObj);
				}

				// 등록되지 않았다면 기본 타입으로 반환
				return sol::make_object(m_Lua, pObj);
			}
		);

objApi.set_function("GetLayer",
	[this](const std::string& layerName) -> sol::table
	{
		// 1. 루아로 반환할 빈 테이블(배열) 생성
		sol::table resultTable = m_Lua.create_table();

		// 2. 레이어 내부 핸들 리스트 가져오기
		// (참고: 내부 CGameObjectManager의 GetLayer가 string_view를 받는다고 가정하고 직접 호출합니다.
		// 만약 CGameInstance를 거쳐야 한다면 string 기반의 GetGameObjectLayerByString 같은 함수를 하나 뚫어주세요!)
		const std::vector<CHandle>* pHandles = CGameInstance::Get().GetGameObjectLayer(layerName);

		// 레이어가 없거나 비어있으면 빈 테이블 반환
		if (pHandles == nullptr || pHandles->empty())
			return resultTable;

		// 3. 핸들 순회 및 스마트 캐스팅
		int index = 1; // 루아 배열은 인덱스가 1부터 시작!
		for (const CHandle& handle : *pHandles)
		{
			CGameObject* pObj = CGameInstance::Get().GetGameObjectByHandle(handle);
			if (pObj)
			{
				// 만들어둔 타입 레지스트리를 활용해 진짜 타입으로 캐스팅!
				auto it = m_TypeRegistry.find(StringID{ pObj->GetType() });
				if (it != m_TypeRegistry.end()) {
					resultTable[index++] = it->second(pObj);
				}
				else {
					resultTable[index++] = sol::make_object(m_Lua, pObj);
				}
			}
		}

		// 4. 완성된 테이블(객체 배열) 반환
		return resultTable;
	}
);

// 2. 레이어 이름으로 첫 번째 오브젝트 가져오기 (예: 플레이어 찾기)
objApi.set_function("GetFirst",
	[this](const std::string& layerName) -> sol::object
	{
		// 1. CGameInstance에서 오브젝트 획득
		CGameObject* pObj = CGameInstance::Get().GetFirstGameObjectByLayer<CGameObject>(layerName);

		if (pObj == nullptr) return sol::nil;

		// 2. 이미 구축해둔 m_TypeRegistry에서 실제 타입을 찾음
		auto it = m_TypeRegistry.find(StringID{ pObj->GetType() });

		if (it != m_TypeRegistry.end()) {
			// 3. 등록된 타입(예: CPlayer)이 있다면 캐스팅하여 반환
			return it->second(pObj);
		}

		// 4. 등록되지 않았다면 기본 GameObject 타입으로 반환
		return sol::make_object(m_Lua, pObj);
	}
);

// 3. 게임 오브젝트 생성 (AddGameObjectToLayer)
objApi.set_function("Add",
	[this](const std::string& levelIndex, const std::string& prototypeTag, const std::string& layerName, sol::optional<sol::object> argObj) -> sol::object
	{
		void* pArg = nullptr;

		// 1. 루아에서 넘겨준 인자가 있고, 그것이 C++ 바인딩된 객체(userdata)라면
		if (argObj.has_value() && argObj.value().is<sol::userdata>())
		{
			// userdata에서 포인터를 추출하여 void*로 형변환
			pArg = const_cast<void*>(argObj.value().as<sol::userdata>().pointer());
		}

		// 2. 추출한 pArg를 넣어서 스폰 API 호출
		auto result = CGameInstance::Get().AddGameObjectToLayer(
			StringID(levelIndex),
			StringID(prototypeTag),
			layerName,
			pArg
		);

		// 3. optional<CHandle> 처리
		if (result.has_value()) {
			return sol::make_object(m_Lua, result.value());
		}
		return sol::nil;
	}
);

// 4. 오브젝트 전체 초기화 (씬 전환 등에서 사용)
//objApi.set_function("AllReset",
//	[]()
//	{
//		CGameInstance::Get()->GameObjectAllReset(); //[cite: 1]
//	}
//);
	}
#pragma endregion

#pragma region Rand
	{
		sol::table mathTable = m_Lua.create_named_table("Rand");

		mathTable.set_function("Randf", [](float min, float max) {
			return Engine::Randf(min, max);
			});

		mathTable.set_function("RandInt", [](int min, int max) {
			return Engine::RandInt(min, max);
			});
	}
#pragma endregion

#pragma region CollManager
	{
		{
			sol::table collTable = m_Lua.create_named_table("Collision");

			// 1. AddColliderGroup
			// 루아: Collision.AddColliderGroup("Player", pCollider)
			collTable.set_function("AddColliderGroup",
				[](const std::string& groupTag, const CCollider* pCollider) {
					CGameInstance::Get().AddColliderGroup(StringID(groupTag.c_str()), pCollider);
				}
			);

			// 2. GetColliderGroup (가장 자주 쓸 함수!)
			// 루아: local colls = Collision.GetColliderGroup("Monster")
			collTable.set_function("GetColliderGroup",
				[this](const std::string& groupTag) -> sol::object {
					auto pGroup = CGameInstance::Get().GetColliderGroup(StringID(groupTag.c_str()));

					if (pGroup == nullptr)
						return sol::nil;

					// sol::as_table을 사용하여 std::vector를 루아 배열로 자동 변환
					return sol::make_object(m_Lua, sol::as_table(*pGroup));
				}
			);

			// 3. IntersectColl
			// 루아: if Collision.IntersectColl(col1, col2) then ...
			collTable.set_function("IntersectColl",
				[](const CCollider* pColl1, const CCollider* pColl2) -> bool {
					return CGameInstance::Get().IntersectColl(pColl1, pColl2);
				}
			);

			// 4. GetColliders (전체 맵 반환)
			// 루아: local allGroups = Collision.GetColliders()
			collTable.set_function("GetColliders",
				[this]() -> sol::object {
					auto pMap = CGameInstance::Get().GetColliders();
					if (pMap == nullptr) return sol::nil;

					// map을 table로 변환해서 반환 (전체 맵이라 다소 무거울 수 있음)
					return sol::make_object(m_Lua, sol::as_table(*pMap));
				}
			);
		}
	}

#pragma endregion

#pragma region Input
	{
		sol::table inputTable = m_Lua.create_named_table("Input");

		// 2. 테이블 안에 함수들을 등록합니다.
		// 세 번째 인자로 싱글톤 인스턴스(&CGameInstance::Get())를 넘겨주면, 
		// 루아에서 호출할 때 C++이 알아서 이 인스턴스를 self로 사용합니다.
		inputTable.set_function("KeyPressing", &CGameInstance::KeyPressing, &CGameInstance::Get());
		inputTable.set_function("KeyDown", &CGameInstance::KeyDown, &CGameInstance::Get());
		inputTable.set_function("KeyUp", &CGameInstance::KeyUp, &CGameInstance::Get());

		inputTable.set_function("MouseMove", &CGameInstance::MouseMove, &CGameInstance::Get());
		inputTable.set_function("MousePressing", &CGameInstance::MousePressing, &CGameInstance::Get());
		inputTable.set_function("MouseDown", &CGameInstance::MouseDown, &CGameInstance::Get());
		inputTable.set_function("MouseUp", &CGameInstance::MouseUp, &CGameInstance::Get());
	}
#pragma endregion
	
#pragma region CameraManager
	{
		sol::table cameraTable = m_Lua.create_named_table("Camera");

		// 2. [스마트 캐스팅 헬퍼 함수] CCameraObject를 받아서 알맞은 자식 타입으로 변환 후 반환
		auto smartCastCamera = [this](CCameraObject* pCam) -> sol::object
			{
				if (pCam == nullptr) return sol::nil;

				// 1단계: 정확한 타입 ID로 찾기 (가장 빠름)
				auto it = m_TypeRegistry.find(StringID{ pCam->GetType() });
				if (it != m_TypeRegistry.end()) {
					return it->second(pCam);
				}

				// 3단계: 못 찾았다면 기본 CCameraObject로 반환
				return sol::make_object(m_Lua, pCam);
			};


		// 3. 함수 바인딩 (헬퍼 함수 활용)
		cameraTable.set_function("GetActiveCamera", sol::overload(
			[this, smartCastCamera]() -> sol::object {
				CCameraObject* pCam = CGameInstance::Get().GetActiveCamera();
				return smartCastCamera(pCam);
			},
			[this, smartCastCamera](const std::string& cameraID) -> sol::object {
				CCameraObject* pCam = CGameInstance::Get().GetActiveCamera(StringID(cameraID.c_str()));
				return smartCastCamera(pCam);
			}
		));

		cameraTable.set_function("GetCamera",
			[this, smartCastCamera](const std::string& cameraID) -> sol::object {
				CCameraObject* pCam = CGameInstance::Get().GetCamera(StringID(cameraID.c_str()));
				return smartCastCamera(pCam);
			}
		);

		// ※ SetActiveCamera는 HRESULT(성공/실패)를 반환하므로 캐스팅 없이 그대로 둡니다.
		cameraTable.set_function("SetActiveCamera",
			[](const std::string& cameraID) {
				return CGameInstance::Get().SetActiveCamera(StringID(cameraID.c_str()));
			}
		);
	}
#pragma endregion
	
#pragma region LevelManager
	{
		sol::table levelTable = m_Lua.create_named_table("Level");

		// 2. ChangeLevel 함수 바인딩
		levelTable.set_function("ChangeLevel", [](const std::string& levelID) -> HRESULT {

			return CGameInstance::Get().ChangeLevel(levelID);

			});
	}
#pragma endregion

#pragma region DbgLineRender
	{
		// 엔진의 DbgLineRender 인스턴스를 미리 캡처
		auto pDbg = CGameInstance::Get().GetDbgLineRender();
		if (!pDbg) return E_FAIL;

		sol::table dbg = m_Lua.create_named_table("DbgLine");

		// 1. 기본 설정
		dbg.set_function("SetColor", [pDbg](const _float4& col) { pDbg->SetColor(col); });
		dbg.set_function("GetColor", [pDbg]() { return pDbg->GetColor(); });

		// 2. 라인 계열
		dbg.set_function("AddLine", sol::overload(
			[pDbg](const _float3& p0, const _float3& p1) { pDbg->AddLine(p0, p1); },
			[pDbg](const _float3& p0, const _float3& p1, const _float4& col) { pDbg->AddLine(p0, p1, col); }
		));

		// 3. 도형 계열 (world 매트릭스를 Matrix 타입으로 직접 수신)
		dbg.set_function("AddBox", [pDbg](const _float3& ext, const _float4x4& world) { pDbg->AddBox(ext, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddSphere", [pDbg](float r, const _float4x4& world) { pDbg->AddSphere(r, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddCapsule", [pDbg](float r, float h, const _float4x4& world) { pDbg->AddCapsule(r, h, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddCylinder", [pDbg](float r, float h, const _float4x4& world) { pDbg->AddCylinder(r, h, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddCone", [pDbg](float r, float h, const _float4x4& world) { pDbg->AddCone(r, h, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddFrustum", [pDbg](float fov, float asp, float n, float f, const _float4x4& world) { pDbg->AddFrustum(fov, asp, n, f, XMLoadFloat4x4(&world)); });

		// 4. 라인/방향 계열
		dbg.set_function("AddRay", [pDbg](const _float3& o, const _float3& d, float l) { pDbg->AddRay(o, d, l); });
		dbg.set_function("AddArrow", [pDbg](const _float3& o, const _float3& d, float l, float hl, float ha) { pDbg->AddArrow(o, d, l, hl, ha); });

		// 5. 부가 기능
		dbg.set_function("AddGrid", [pDbg](uint32_t cnt, float size, const _float4x4& world) { pDbg->AddGrid(cnt, size, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddQuad", [pDbg](float w, float h, const _float4x4& world) { pDbg->AddQuad(w, h, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddTriangle", [pDbg](const _float3& p0, const _float3& p1, const _float3& p2) { pDbg->AddTriangle(p0, p1, p2); });
		dbg.set_function("AddAxis", [pDbg](float len, const _float4x4& world) { pDbg->AddAxis(len, XMLoadFloat4x4(&world)); });
		dbg.set_function("AddCircle", [pDbg](float r, const _float4x4& world, uint32_t s) { pDbg->AddCircle(r, XMLoadFloat4x4(&world), s); });
		dbg.set_function("AddCross", [pDbg](const _float3& pos, float s) { pDbg->AddCross(pos, s); });

	}
#pragma endregion
	
	return S_OK;
}

HRESULT CLuaManager::Initialize_ClassBindnig()
{
#pragma region EngineBase
	{
		m_Lua.new_usertype<CEngineBase>("EngineBase",
			sol::no_constructor,
			"GetTypeString", &CEngineBase::GetTypeString,
			"IsA", [](CEngineBase& self, const std::string& typeName) {
				return self.IsA(STRID(typeName.c_str()));
			}
		);
	}
#pragma endregion

#pragma region Collider
	{
		m_Lua.new_usertype<CCollider>("Collider",
			// 1. 추상 클래스이므로 직접 new 생성 불가 명시
			sol::no_constructor,
			sol::base_classes, sol::bases<CEngineBase>(),

			// 2. 상속 구조 (CEngineBase가 바인딩되어 있다면 명시)
			sol::base_classes, sol::bases<CEngineBase>(),

			// 3. 충돌 타입 확인 (Enum인 CollType이 루아에 있다면 그것도 바인딩해주면 좋습니다)
			"GetCollType", &CCollider::GetCollType,

			// 4. 충돌 체크 함수 (멤버 함수)
			// 루아에서: if coll1:Intersect(coll2) then ...
			"Intersect", &CCollider::Intersect,

			// 5. 색상 관련 속성 (디버깅용으로 쓰기 좋습니다)
			"SetOriginalColor", &CCollider::SetOriginalColor,
			"GetOriginalColor", &CCollider::GetOriginalColor,
			"SetIntersectColor", &CCollider::SetIntersectColor,
			"GetIntersectColor", &CCollider::GetIntersectColor
		);



		m_Lua.new_usertype<CCollBox>("CollBox",
			// 1. 팩토리 설정: Create 함수가 UPtr을 반환하므로 람다로 래핑하여 루아에 전달
			//sol::factories([](float cx, float cy, float cz, float ex, float ey, float ez) {
			//	// CCollBox::Create는 UPtr을 반환하지만, 
			//	// sol2가 소유권을 가져가도록 release()로 raw pointer를 넘겨줍니다.
			//	return CCollBox::Create({ cx, cy, cz }, { ex, ey, ez }).release();
			//	}),

			// 2. 부모 상속 관계 명시
			sol::no_constructor,
			sol::base_classes, sol::bases<CCollider, CEngineBase>(),

			// 3. CCollBox 고유 기능
			"GetBoundingBox", &CCollBox::GetBoundingBox,
			"GetLocalBoundingBox", &CCollBox::GetLocalBoundingBox,
			"SetLocalBoundingBox", &CCollBox::SetLocalBoundingBox
		);

		m_Lua.new_usertype<CCollFrustum>("CollFrustum",
			sol::no_constructor,
			sol::base_classes, sol::bases<CCollider, CEngineBase>(),

			// 1. Getter/Setter
			// 주의: BoundingFrustum이 루아에 등록되어 있지 않다면 루아에서 이 객체를 직접 다룰 수 없습니다.
			// 만약 사용하지 않는다면 굳이 바인딩하지 않아도 됩니다.
			"GetBoundingFrustum", &CCollFrustum::GetBoundingFrustum,
			"GetLocalBoundingFrustum", &CCollFrustum::GetLocalBoundingFrustum

			// 2. SetLocalFrustum(_fmatrix mat)
			// [주의] _fmatrix는 DirectX 타입입니다. 
			// 루아에서 행렬을 넘기려면 별도의 변환 함수가 필요할 수 있습니다.
			//"SetLocalFrustum", &CCollFrustum::SetLocalFrustum
		);


		m_Lua.new_usertype<CCollOrientedBox>("CollOrientedBox",
			// 1. 루아에서 직접 생성 방지 (엔진 관리 하에 있으므로)
			sol::no_constructor,

			// 2. 상속 구조 명시
			sol::base_classes, sol::bases<CCollider, CEngineBase>(),

			// 3. Getter 및 Setter 바인딩
			"GetBoundingOrientedBox", &CCollOrientedBox::GetBoundingOrientedBox,
			"GetLocalBoundingOrientedBox", &CCollOrientedBox::GetLocalBoundingOrientedBox,
			"SetLocalBoundingOrientedBox", &CCollOrientedBox::SetLocalBoundingOrientedBox
		);

		m_Lua.new_usertype<CCollSphere>("CollSphere",
			sol::no_constructor,
			sol::base_classes, sol::bases<CCollider, CEngineBase>(),

			// 1. 간단한 Getter
			"GetBoundingSphere", &CCollSphere::GetBoundingSphere,
			"GetLocalBoundingSphere", &CCollSphere::GetLocalBoundingSphere

			// 2. SetLocalBoundingSphere (Wrapper: table -> _float3)
			//"SetLocalBoundingSphere", [](CCollSphere* pSphere, sol::table center, float radius) {
			//	_float3 vCenter{ center[1], center[2], center[3] };
			//	pSphere->SetLocalBoundingSphere(vCenter, radius);
			//}
		);
	}
#pragma endregion

#pragma region Components

#pragma region Component
	{
		m_Lua.new_usertype<CComponent>("Component",
			sol::no_constructor, // 직접 생성이 불가능하므로 명시
			sol::base_classes, sol::bases<CPrototype, CEngineBase>(), // CPrototype을 상속

			// GameObject를 가져오는 함수 (루아에서는 GetOwner로 부르는 게 더 직관적일 수 있음)
			"GetGameObject", &CComponent::GetGameObject
		);
	}
#pragma endregion

#pragma region ComLuaScript
	{
		m_Lua.new_usertype<CComLuaScript>("ComLuaScript",
			sol::no_constructor,
			sol::meta_function::to_string,
			[](CComLuaScript&)
			{
				return std::string("ComLuaScript");
			}
		);
	}
#pragma endregion

#pragma region ComCollider
	{
		m_Lua.new_usertype<CComCollider>("ComCollider",
			sol::no_constructor,
			sol::base_classes, sol::bases<CComponent, CEngineBase>(),

			// 1. 실제 충돌 연산용 Collider 객체 반환
			"Get", &CComCollider::Get,

			// 2. 변환 행렬 적용
			"Transform", &CComCollider::Transform


		);
	}
#pragma endregion

#pragma region ComTransform
	{
		m_Lua.new_usertype<CComTransform>("Transform",
			sol::no_constructor,
			sol::base_classes, sol::bases<CComponent, CEngineBase>(),
			sol::meta_function::to_string,
			[](CComTransform&)
			{
				return std::string("Transform");
			},

			// Position
			"GetPosition", &CComTransform::GetPosition,
			"SetPosition", sol::overload(
				static_cast<void(CComTransform::*)(const _float3&)>(&CComTransform::SetPosition)
			),
			"AddPosition",
			[](CComTransform& t, const _float3& v)
			{
				//OutputDebugStringA("Lua -> AddPosition\n");
				t.AddPosition(v);
			},

			// Rotation
			"GetRotationEuler", &CComTransform::GetRotationEuler,
			"SetRotationEuler", &CComTransform::SetRotationEuler,
			"AddRotationEuler", &CComTransform::AddRotationEuler,

			// Scale
			"GetScale", &CComTransform::GetScale,
			"SetScale", sol::overload(
				static_cast<void(CComTransform::*)(const _float3&)>(&CComTransform::SetScale)
			),

			// Movement
			"GoStraight", &CComTransform::GoStraight,
			"GoBackward", &CComTransform::GoBackward,
			"GoLeft", &CComTransform::GoLeft,
			"GoRight", &CComTransform::GoRight,
			"GoUp", &CComTransform::GoUp,
			"GoDown", &CComTransform::GoDown,

			"GetWorldMatrix", [](const CComTransform& t) {
				return *t.GetWorldMatrix();
			},
			"GetCombinedWorldMatrix", [](const CComTransform& t) {
				return *t.GetCombinedWorldMatrix();
			},
			"GetLoadedWorldMatrix", [](const CComTransform& t) {
				_float4x4 mat;
				XMStoreFloat4x4(&mat, t.GetLoadedWorldMatrix());
				return mat;
			},
			"GetLoadedCombinedWorldMatrix", [](const CComTransform& t) {
				_float4x4 mat;
				XMStoreFloat4x4(&mat, t.GetLoadedCombinedWorldMatrix());
				return mat;
			},

			// Etc
			"LookAt", [](CComTransform& t, const _float3& pos)
			{
				t.LookAt(XMLoadFloat3(&pos));
			},
			"Chase", [](CComTransform& t, const _float3& pos, float dist, float limit)
			{
				t.Chase(XMLoadFloat3(&pos), dist, limit);
			}
		);
	}
#pragma endregion	

#pragma endregion

#pragma region GameObjects

#pragma region GameObject
	{
		m_Lua.new_usertype<CGameObject>("GameObject",
			sol::no_constructor,
			sol::base_classes, sol::bases<CEngineBase>(), // CEngineBase 등록 후 상속 명시

			// 컴포넌트 접근 (문자열 태그 기반)
			"GetComponent", [this](CGameObject& self, const std::string& tag) -> sol::object
			{
				// 1. 일단 부모 타입(CComponent)으로 가져옴
				CComponent* pCom = self.GetComponent<CComponent>(StringID(tag.c_str()));

				if (pCom == nullptr) return sol::nil;

				// 2. 기존에 만들어둔 m_TypeRegistry에서 타입 정보를 찾음
				// pCom->GetType()은 CEngineBase의 가상함수이므로 정확한 자식 타입 ID를 반환함
				auto it = m_TypeRegistry.find(StringID{ pCom->GetType() });

				if (it != m_TypeRegistry.end()) {
					// 3. 등록된 타입(예: CComTransform)이 있다면 캐스팅하여 반환
					return it->second(pCom);
				}

				// 4. 등록되지 않았다면 기본 컴포넌트 타입으로 반환
				return sol::make_object(m_Lua, pCom);
			},

			// Transform은 너무 자주 쓰이므로 따로 빼는 것이 좋음
			"GetTransform", [](CGameObject& self) -> CComTransform& {
				return self.GetTransform();
			},

			// 오브젝트 관리
			"GetHandle", &CGameObject::GetHandle,
			"DestroyCascade", &CGameObject::SetPendingDestroyCascade,
			"GetTag", &CGameObject::GetObjectTag,
			"SetTag", &CGameObject::SetObjectTag
		);
	}
#pragma endregion

#pragma region CameraObject
	{
		m_Lua.new_usertype<CCameraObject>("CameraObject",
			// 1. 부모 클래스(CGameObject) 상속 명시!
			// 이렇게 해야 카메라 객체에서 GetComponent 등을 쓸 수 있습니다.
			sol::no_constructor,
			sol::base_classes, sol::bases<CGameObject, CEngineBase>(),

			// 2. 행렬 반환 함수 (루아 쪽에 _matrix 타입이 바인딩되어 있어야 합니다)
			"GetView", &CCameraObject::GetView,
			"GetProj", &CCameraObject::GetProj,

			// 3. GetRay: std::pair를 루아의 2개 반환값으로 자동 처리!
			"GetRay", &CCameraObject::GetRay,

			// 4. 행렬 업데이트 함수
			"UpdateViewMatrix", &CCameraObject::UpdateViewMatrix,
			"UpdateProjMatrix", &CCameraObject::UpdateProjMatrix
		);
	}
#pragma endregion

#pragma region FlyCamera
	{
		m_Lua.new_usertype<CFlyCamera>("FlyCamera",
			// 생성자가 protected이므로 차단 (엔진 내부에서 Create로 생성하므로)
			sol::no_constructor,

			// [핵심] 부모 클래스가 CCameraObject와 CGameObject임을 명시!
			// 이렇게 하면 GetRay()나 GetComponent()를 루아에서 그대로 쓸 수 있습니다.
			sol::base_classes, sol::bases<CCameraObject, CGameObject, CEngineBase>(),

			// CFlyCamera만의 고유 함수 바인딩 (필요한 경우에만 엽니다)
			// 주의: CCollFrustum이 루아에 바인딩되어 있지 않다면 주석 처리하는 것이 좋습니다.
			"GetFrustumCollider", &CFlyCamera::GetFrustumCollider
		);
	}
#pragma endregion

#pragma region UICamera
	{
		m_Lua.new_usertype<CUICamera>("UICamera",
			// 생성자 차단 (엔진에서 Create로 생성)
			sol::no_constructor,

			// [핵심] CCameraObject와 CGameObject를 상속받았음을 명시
			sol::base_classes, sol::bases<CCameraObject, CGameObject, CEngineBase>()
		);
	}
#pragma endregion

#pragma endregion

	return S_OK;
}

HRESULT CLuaManager::Initialize_DefineBinding()
{

#pragma region DIK_KEY
	{
#define LUA_KEY(luaName, dikName) key[#luaName] = DIK_##dikName

		sol::table key = m_Lua.create_named_table("Key");
		LUA_KEY(Num0, 0);
		LUA_KEY(Num1, 1);
		LUA_KEY(Num2, 2);
		LUA_KEY(Num3, 3);
		LUA_KEY(Num4, 4);
		LUA_KEY(Num5, 5);
		LUA_KEY(Num6, 6);
		LUA_KEY(Num7, 7);
		LUA_KEY(Num8, 8);
		LUA_KEY(Num9, 9);

		// 알파벳
		LUA_KEY(A, A);
		LUA_KEY(B, B);
		LUA_KEY(C, C);
		LUA_KEY(D, D);
		LUA_KEY(E, E);
		LUA_KEY(F, F);
		LUA_KEY(G, G);
		LUA_KEY(H, H);
		LUA_KEY(I, I);
		LUA_KEY(J, J);
		LUA_KEY(K, K);
		LUA_KEY(L, L);
		LUA_KEY(M, M);
		LUA_KEY(N, N);
		LUA_KEY(O, O);
		LUA_KEY(P, P);
		LUA_KEY(Q, Q);
		LUA_KEY(R, R);
		LUA_KEY(S, S);
		LUA_KEY(T, T);
		LUA_KEY(U, U);
		LUA_KEY(V, V);
		LUA_KEY(W, W);
		LUA_KEY(X, X);
		LUA_KEY(Y, Y);
		LUA_KEY(Z, Z);

		// 방향키
		LUA_KEY(Left, LEFT);
		LUA_KEY(Right, RIGHT);
		LUA_KEY(Up, UP);
		LUA_KEY(Down, DOWN);

		// 기능키
		LUA_KEY(Escape, ESCAPE);
		LUA_KEY(Space, SPACE);
		LUA_KEY(Return, RETURN);
		LUA_KEY(Tab, TAB);
		LUA_KEY(Backspace, BACK);

		LUA_KEY(LShift, LSHIFT);
		LUA_KEY(RShift, RSHIFT);
		LUA_KEY(LCtrl, LCONTROL);
		LUA_KEY(RCtrl, RCONTROL);
		LUA_KEY(LAlt, LMENU);
		LUA_KEY(RAlt, RMENU);

		LUA_KEY(Home, HOME);
		LUA_KEY(End, END);
		LUA_KEY(Insert, INSERT);
		LUA_KEY(Delete, DELETE);

		LUA_KEY(PageUp, PRIOR);
		LUA_KEY(PageDown, NEXT);

		// F키
		LUA_KEY(F1, F1);
		LUA_KEY(F2, F2);
		LUA_KEY(F3, F3);
		LUA_KEY(F4, F4);
		LUA_KEY(F5, F5);
		LUA_KEY(F6, F6);
		LUA_KEY(F7, F7);
		LUA_KEY(F8, F8);
		LUA_KEY(F9, F9);
		LUA_KEY(F10, F10);
		LUA_KEY(F11, F11);
		LUA_KEY(F12, F12);

		// 넘패드
		LUA_KEY(Numpad0, NUMPAD0);
		LUA_KEY(Numpad1, NUMPAD1);
		LUA_KEY(Numpad2, NUMPAD2);
		LUA_KEY(Numpad3, NUMPAD3);
		LUA_KEY(Numpad4, NUMPAD4);
		LUA_KEY(Numpad5, NUMPAD5);
		LUA_KEY(Numpad6, NUMPAD6);
		LUA_KEY(Numpad7, NUMPAD7);
		LUA_KEY(Numpad8, NUMPAD8);
		LUA_KEY(Numpad9, NUMPAD9);

		// 특수 문자
		LUA_KEY(Minus, MINUS);           // -
		LUA_KEY(Equals, EQUALS);         // =
		LUA_KEY(Grave, GRAVE);           // ` ~
		LUA_KEY(LeftBracket, LBRACKET);  // [
		LUA_KEY(RightBracket, RBRACKET); // ]
		LUA_KEY(Backslash, BACKSLASH);   // \ |
		LUA_KEY(Semicolon, SEMICOLON);   // ; :
		LUA_KEY(Apostrophe, APOSTROPHE); // ' "
		LUA_KEY(Comma, COMMA);           // , <
		LUA_KEY(Period, PERIOD);         // . >
		LUA_KEY(Slash, SLASH);           // / ?

		LUA_KEY(NumpadAdd, ADD);
		LUA_KEY(NumpadSub, SUBTRACT);
		LUA_KEY(NumpadMul, MULTIPLY);
		LUA_KEY(NumpadDiv, DIVIDE);
		LUA_KEY(NumpadEnter, NUMPADENTER);
		LUA_KEY(NumpadDecimal, DECIMAL);

#undef LUA_KEY
	}
#pragma endregion

#pragma region MOUSESTATE
	// Mouse
	{
		{
			sol::table mouse = m_Lua.create_named_table("Mouse");

			mouse["Left"] = MOUSEKEYSTATE::LB;
			mouse["Right"] = MOUSEKEYSTATE::RB;
			mouse["Middle"] = MOUSEKEYSTATE::MB;
		}

		{
			sol::table mouseMove = m_Lua.create_named_table("MouseMove");

			mouseMove["X"] = MOUSEMOVESTATE::X;
			mouseMove["Y"] = MOUSEMOVESTATE::Y;
			mouseMove["Z"] = MOUSEMOVESTATE::Z;
		}
	}
#pragma endregion
	
	return S_OK;
}

sol::protected_function CLuaManager::CacheFunction(const std::string& funcName)
{
	sol::object obj = m_Lua.globals()[funcName];

	if (obj.valid() && obj.get_type() == sol::type::function)
		return obj.as<sol::protected_function>();

	return sol::protected_function(); // 유효하지 않은 빈 객체 반환
}

sol::protected_function CLuaManager::CacheFunction(const sol::environment& env, const std::string& funcName)
{
	// 1. Env에서 먼저 찾기
	sol::object obj = env[funcName];

	// 2. Env에 없거나 함수가 아니면, 전역(Globals) 공간에서 찾기 (선택적 폴백)
	if (!obj.valid() || obj.get_type() != sol::type::function)
	{
		sol::state_view lua(env.lua_state());
		obj = lua.globals()[funcName];
	}

	// 3. 최종 반환
	if (obj.valid() && obj.get_type() == sol::type::function)
		return obj.as<sol::protected_function>();

	return sol::protected_function(); // 유효하지 않은 빈 객체 반환
}

bool CLuaManager::HasFunction(sol::environment& env, std::string_view function) const
{
	sol::object obj = env[std::string(function)];

	return obj.valid() &&
		obj.get_type() == sol::type::function;
}

HRESULT CLuaManager::Execute(const std::string& script, const sol::environment& env, const std::string& chunkName)
{
	try
	{
		std::string formattedChunkName = chunkName;

		// 파일 경로 형태(.lua)로 들어왔다면 절대 경로 처리
		if (chunkName.find(".lua") != std::string::npos)
		{
			std::filesystem::path absolutePath = std::filesystem::absolute(chunkName);
			formattedChunkName = "@" + absolutePath.generic_string();
		}
		else if (!chunkName.empty() && chunkName[0] != '@') // 단순 문자열이면 @ 추가
		{
			formattedChunkName = "@" + chunkName;
		}

		auto result = m_Lua.safe_script(
			script,
			env,
			sol::script_pass_on_error,
			formattedChunkName
		);

		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(("[Lua Env Execute Error] " + std::string(err.what()) + "\n").c_str());
			return E_FAIL;
		}
	}
	catch (const std::exception& e)
	{
		OutputDebugStringA(("[C++ Exception in Lua Env] " + std::string(e.what()) + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}

HRESULT CLuaManager::Execute(const std::string& script, const std::string& chunkName)
{
	try
	{
		std::string formattedChunkName = chunkName;

		if (chunkName.find(".lua") != std::string::npos)
		{
			std::filesystem::path absolutePath = std::filesystem::absolute(chunkName);
			formattedChunkName = "@" + absolutePath.generic_string();
		}
		else if (!chunkName.empty() && chunkName[0] != '@')
		{
			formattedChunkName = "@" + chunkName;
		}

		// env 매개변수 없이 m_Lua(글로벌 상태)에 바로 실행
		auto result = m_Lua.safe_script(
			script,
			sol::script_pass_on_error,
			formattedChunkName
		);

		if (!result.valid())
		{
			sol::error err = result;
			OutputDebugStringA(("[Lua State Execute Error] " + std::string(err.what()) + "\n").c_str());
			return E_FAIL;
		}
	}
	catch (const std::exception& e)
	{
		OutputDebugStringA(("[C++ Exception in Lua State] " + std::string(e.what()) + "\n").c_str());
		return E_FAIL;
	}

	return S_OK;
}
// object 하나당 하나만들기 
sol::environment CLuaManager::CreateEnvironment()
{
	return sol::environment(m_Lua, sol::create, m_Lua.globals());
}


HRESULT CLuaManager::Compile(const std::string& script)
{
	// script 변수에 파일 경로가 들어온다면 m_Lua.load_file(script) 를 쓰셔야 합니다!
	sol::load_result result = m_Lua.load(script);

	if (!result.valid())
	{
		sol::error err = result;

		// 수정된 부분: sol::load_status 를 사용하거나 auto를 사용하세요.
		sol::load_status status = result.status();

		// 상태 코드와 에러 내용을 함께 출력
		std::string msg = "[LuaError] Status(" + std::to_string((int)status) + "): " + err.what() + "\n";
		MSG_BOX_STR(StringToWString(msg).c_str());
		//OutputDebugStringA(msg.c_str());

		return E_FAIL;
	}

	return S_OK;
}

void CLuaManager::EnvDump(const sol::environment& env) const
{
	OutputDebugStringA("========== Lua Environment ==========\n");

	sol::table table = env;

	for (auto& kv : table)
	{
		sol::object key = kv.first;
		sol::object value = kv.second;

		if (!key.is<std::string>())
			continue;

		std::string line = key.as<std::string>();
		line += " : ";
		line += sol::type_name(value.lua_state(), value.get_type());
		line += "\n";

		OutputDebugStringA(line.c_str());
	}

	OutputDebugStringA("=====================================\n");
}

void CLuaManager::EnvClear(sol::environment& env)
{
	sol::table table = env;

	for (auto& kv : table)
	{
		if (kv.first.is<std::string>())
			env[kv.first.as<std::string>()] = sol::lua_nil;
	}
}

void CLuaManager::UpdateHotReload()
{
	if (!m_pLuaWatcher) return;

	namespace fs = std::filesystem;
	auto changedFiles = m_pLuaWatcher->GetChangedFilesAndClear();
	if (changedFiles.empty()) return;

	for (const auto& fullFileName : changedFiles)
	{
		// fullFileName 예시: "../SampleClient/LuaFiles/Player/Player.lua"

		// 1. 전체 경로 문자열에서 슬래시(/) 방향 통일
		std::string srcStr = fullFileName;
		std::replace(srcStr.begin(), srcStr.end(), '\\', '/');

		// 2. "LuaFiles/" 폴더 구조 통째로 추출
		std::string keyword = "LuaFiles/";
		size_t pos = srcStr.find(keyword);

		if (pos == std::string::npos) continue; // 안전장치: 경로에 LuaFiles가 없으면 스킵

		// relativePath는 "LuaFiles/Player/Player.lua" 형태가 됩니다.
		std::string relativePath = srcStr.substr(pos);

		// 3. 최종 Bin 경로 조합 
		fs::path srcPath = srcStr;
		fs::path destPath = fs::path("./") / relativePath; // 결과: "./LuaFiles/Player/Player.lua"

		try
		{
			// [매우 중요] Bin 폴더 안에 "Player" 같은 하위 폴더가 아직 없을 수 있으므로 무조건 생성해줍니다.
			// 이거 안 하면 폴더가 없어서 복사 에러(catch)가 납니다!
			fs::create_directories(destPath.parent_path());

			// 4. 원본 폴더에서 Bin 폴더로 복사
			fs::copy_file(srcPath, destPath, fs::copy_options::overwrite_existing);
		}
		catch (const fs::filesystem_error& e)
		{
			OutputDebugStringA(("[Lua] File Copy Failed: " + srcStr + "\n").c_str());
			continue;
		}

		// 5. 엔진에 등록된 형식으로 문자열 변환 후 리로드
		std::string targetResourcePath = destPath.string();
		std::replace(targetResourcePath.begin(), targetResourcePath.end(), '\\', '/');

		OnFileChanged(targetResourcePath);


		OutputDebugStringA(("Lua HotReloaded: " + fullFileName + "\n").c_str());
	}
}

void CLuaManager::OnFileChanged(const std::string& path)
{
	// 1. 매니저를 통해 해당 경로의 리소스 리스트를 바로 획득
	auto* pResList = CGameInstance::Get().GetResourcesByPath(path);

	if (!pResList) return; // 해당 경로에 리소스가 없음

	// 2. 리스트 순회하며 리로드
	for (auto pRes : *pResList)
	{
		// 리소스가 LuaScript인지 확인하고 캐스팅
		if (auto pLuaRes = Cast<CResLuaScript>(pRes))
		{
			if (SUCCEEDED(pLuaRes->Reload()))
			{
				const auto& iterCom = m_scriptRegistry.find(path);
				if (iterCom != m_scriptRegistry.end())
				{
					for (auto& com : iterCom->second)
					{
						com->LuaScriptRelod();
					}
				}
			}
		}
	}
}


void CLuaManager::RegisterComponent(const std::string& path, ILuaScriptRelodable* pComp)
{
	m_scriptRegistry[path].push_back(pComp);
}
void CLuaManager::UnregisterComponent(const std::string& path, ILuaScriptRelodable* pComp)
{
	auto& list = m_scriptRegistry[path];
	list.erase(std::remove(list.begin(), list.end(), pComp), list.end());
}

UPtr<CLuaManager> CLuaManager::Create()
{
	auto pInstance = UPtr<CLuaManager>(new CLuaManager{});
	if (FAILED(pInstance->Initialize()))
	{
		return nullptr;
	}
	return pInstance;
}

