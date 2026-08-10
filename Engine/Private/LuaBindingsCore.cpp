#include "pch.h"
#include "LuaManager.h"

#include "GameObject.h"

NS_USING(Engine)

HRESULT CLuaManager::InitializePrintBinding()
{
	sol::protected_function ToString = m_Lua["tostring"];
	m_Lua.set_function("print",
		[ToString](sol::variadic_args Arguments) mutable
		{
			std::ostringstream Stream{};
			Stream << "[Lua] ";

			_bool bFirst = true;
			for (const auto& Value : Arguments)
			{
				if (!bFirst)
					Stream << '\t';

				bFirst = false;
				sol::protected_function_result Result = ToString(Value);
				Stream << (Result.valid() ? Result.get<std::string>() : "<tostring error>");
			}

			Stream << '\n';
			OutputDebugStringA(Stream.str().c_str());
		});

	return S_OK;
}

HRESULT CLuaManager::InitializeValueTypeBindings()
{
	auto HandleType = m_Lua.new_usertype<CHandle>(
		"ObjectHandle",
		sol::call_constructor,
		sol::constructors<CHandle()>(),
		sol::meta_function::equal_to,
		[](const CHandle& Left, const CHandle& Right)
		{
			return Left == Right;
		},
		sol::meta_function::to_string,
		[](const CHandle& Handle)
		{
			return "ObjectHandle(" + std::to_string(Handle.GetIndex()) + ", " +
				std::to_string(Handle.GetGeneration()) + ")";
		});

	HandleType["Index"] = sol::property(&CHandle::GetIndex);
	HandleType["Generation"] = sol::property(&CHandle::GetGeneration);
	HandleType.set_function("IsValid",
		[this](const CHandle& Handle)
		{
			return ResolveObject(Handle) != nullptr;
		});

	m_Lua.new_usertype<_float2>(
		"Vector2",
		sol::call_constructor,
		sol::constructors<_float2(), _float2(_float, _float)>(),
		"x", &_float2::x,
		"y", &_float2::y,
		sol::meta_function::addition,
		[](const _float2& Left, const _float2& Right)
		{
			return _float2{ Left.x + Right.x, Left.y + Right.y };
		},
		sol::meta_function::subtraction,
		[](const _float2& Left, const _float2& Right)
		{
			return _float2{ Left.x - Right.x, Left.y - Right.y };
		},
		sol::meta_function::multiplication,
		[](const _float2& Value, _float Scale)
		{
			return _float2{ Value.x * Scale, Value.y * Scale };
		});

	auto Vector3Type = m_Lua.new_usertype<_float3>(
		"Vector3",
		sol::call_constructor,
		sol::constructors<_float3(), _float3(_float, _float, _float)>(),
		"x", &_float3::x,
		"y", &_float3::y,
		"z", &_float3::z,
		sol::meta_function::to_string,
		[](const _float3& Value)
		{
			return "Vector3(" + std::to_string(Value.x) + ", " +
				std::to_string(Value.y) + ", " + std::to_string(Value.z) + ")";
		},
		sol::meta_function::addition,
		[](const _float3& Left, const _float3& Right)
		{
			_float3 Result{};
			XMStoreFloat3(&Result, XMLoadFloat3(&Left) + XMLoadFloat3(&Right));
			return Result;
		},
		sol::meta_function::subtraction,
		[](const _float3& Left, const _float3& Right)
		{
			_float3 Result{};
			XMStoreFloat3(&Result, XMLoadFloat3(&Left) - XMLoadFloat3(&Right));
			return Result;
		},
		sol::meta_function::unary_minus,
		[](const _float3& Value)
		{
			return _float3{ -Value.x, -Value.y, -Value.z };
		},
		sol::meta_function::multiplication,
		sol::overload(
			[](const _float3& Value, _float Scale)
			{
				_float3 Result{};
				XMStoreFloat3(&Result, XMVectorScale(XMLoadFloat3(&Value), Scale));
				return Result;
			},
			[](const _float3& Left, const _float3& Right)
			{
				_float3 Result{};
				XMStoreFloat3(&Result, XMLoadFloat3(&Left) * XMLoadFloat3(&Right));
				return Result;
			}));

	Vector3Type.set_function("Length",
		[](const _float3& Value)
		{
			return XMVectorGetX(XMVector3Length(XMLoadFloat3(&Value)));
		});
	Vector3Type.set_function("Normalize",
		[](const _float3& Value)
		{
			const _vector LoadedValue = XMLoadFloat3(&Value);
			if (XMVectorGetX(XMVector3LengthSq(LoadedValue)) <= 1e-12f)
				return _float3{};

			_float3 Result{};
			XMStoreFloat3(&Result, XMVector3Normalize(LoadedValue));
			return Result;
		});
	Vector3Type.set_function("Dot",
		[](const _float3& Left, const _float3& Right)
		{
			return XMVectorGetX(XMVector3Dot(XMLoadFloat3(&Left), XMLoadFloat3(&Right)));
		});
	Vector3Type.set_function("Cross",
		[](const _float3& Left, const _float3& Right)
		{
			_float3 Result{};
			XMStoreFloat3(&Result, XMVector3Cross(XMLoadFloat3(&Left), XMLoadFloat3(&Right)));
			return Result;
		});
	Vector3Type.set_function("Distance",
		[](const _float3& Left, const _float3& Right)
		{
			return XMVectorGetX(XMVector3Length(XMLoadFloat3(&Right) - XMLoadFloat3(&Left)));
		});
	Vector3Type.set_function("Lerp",
		[](const _float3& Left, const _float3& Right, _float Ratio)
		{
			_float3 Result{};
			XMStoreFloat3(&Result, XMVectorLerp(XMLoadFloat3(&Left), XMLoadFloat3(&Right), Ratio));
			return Result;
		});

	m_Lua.new_usertype<_float4>(
		"Vector4",
		sol::call_constructor,
		sol::constructors<_float4(), _float4(_float, _float, _float, _float)>(),
		"x", &_float4::x,
		"y", &_float4::y,
		"z", &_float4::z,
		"w", &_float4::w,
		sol::meta_function::addition,
		[](const _float4& Left, const _float4& Right)
		{
			_float4 Result{};
			XMStoreFloat4(&Result, XMLoadFloat4(&Left) + XMLoadFloat4(&Right));
			return Result;
		},
		sol::meta_function::subtraction,
		[](const _float4& Left, const _float4& Right)
		{
			_float4 Result{};
			XMStoreFloat4(&Result, XMLoadFloat4(&Left) - XMLoadFloat4(&Right));
			return Result;
		},
		sol::meta_function::multiplication,
		[](const _float4& Value, _float Scale)
		{
			_float4 Result{};
			XMStoreFloat4(&Result, XMVectorScale(XMLoadFloat4(&Value), Scale));
			return Result;
		});

	auto MatrixType = m_Lua.new_usertype<_float4x4>(
		"Matrix",
		sol::call_constructor,
		sol::constructors<_float4x4()>(),
		sol::meta_function::multiplication,
		[](const _float4x4& Left, const _float4x4& Right)
		{
			_float4x4 Result{};
			XMStoreFloat4x4(&Result, XMLoadFloat4x4(&Left) * XMLoadFloat4x4(&Right));
			return Result;
		});

	MatrixType.set_function("Get",
		[](const _float4x4& Value, int32_t iRow, int32_t iColumn)
		{
			if (iRow < 1 || iRow > 4 || iColumn < 1 || iColumn > 4)
				throw sol::error{ "Matrix.Get indices must be in the range 1..4." };

			return Value.m[iRow - 1][iColumn - 1];
		});
	MatrixType.set_function("Set",
		[](_float4x4& Value, int32_t iRow, int32_t iColumn, _float fElement)
		{
			if (iRow < 1 || iRow > 4 || iColumn < 1 || iColumn > 4)
				throw sol::error{ "Matrix.Set indices must be in the range 1..4." };

			Value.m[iRow - 1][iColumn - 1] = fElement;
		});
	MatrixType.set_function("Transpose",
		[](const _float4x4& Value)
		{
			_float4x4 Result{};
			XMStoreFloat4x4(&Result, XMMatrixTranspose(XMLoadFloat4x4(&Value)));
			return Result;
		});
	MatrixType.set_function("Inverse",
		[](const _float4x4& Value) -> std::optional<_float4x4>
		{
			const _matrix LoadedValue = XMLoadFloat4x4(&Value);
			const _vector Determinant = XMMatrixDeterminant(LoadedValue);
			if (std::abs(XMVectorGetX(Determinant)) <= 1e-8f)
				return std::nullopt;

			_float4x4 Result{};
			XMStoreFloat4x4(&Result, XMMatrixInverse(nullptr, LoadedValue));
			return Result;
		});
	MatrixType.set_function("Identity",
		[]()
		{
			_float4x4 Result{};
			XMStoreFloat4x4(&Result, XMMatrixIdentity());
			return Result;
		});

	return S_OK;
}

HRESULT CLuaManager::InitializeConstantBindings()
{
	sol::table Key = m_Lua.create_named_table("Key");

#define LUA_KEY(LuaName, DIKName) Key[#LuaName] = DIK_##DIKName
	LUA_KEY(Num0, 0); LUA_KEY(Num1, 1); LUA_KEY(Num2, 2); LUA_KEY(Num3, 3); LUA_KEY(Num4, 4);
	LUA_KEY(Num5, 5); LUA_KEY(Num6, 6); LUA_KEY(Num7, 7); LUA_KEY(Num8, 8); LUA_KEY(Num9, 9);
	LUA_KEY(A, A); LUA_KEY(B, B); LUA_KEY(C, C); LUA_KEY(D, D); LUA_KEY(E, E); LUA_KEY(F, F);
	LUA_KEY(G, G); LUA_KEY(H, H); LUA_KEY(I, I); LUA_KEY(J, J); LUA_KEY(K, K); LUA_KEY(L, L);
	LUA_KEY(M, M); LUA_KEY(N, N); LUA_KEY(O, O); LUA_KEY(P, P); LUA_KEY(Q, Q); LUA_KEY(R, R);
	LUA_KEY(S, S); LUA_KEY(T, T); LUA_KEY(U, U); LUA_KEY(V, V); LUA_KEY(W, W); LUA_KEY(X, X);
	LUA_KEY(Y, Y); LUA_KEY(Z, Z);
	LUA_KEY(Left, LEFT); LUA_KEY(Right, RIGHT); LUA_KEY(Up, UP); LUA_KEY(Down, DOWN);
	LUA_KEY(Escape, ESCAPE); LUA_KEY(Space, SPACE); LUA_KEY(Return, RETURN); LUA_KEY(Tab, TAB);
	LUA_KEY(Backspace, BACK); LUA_KEY(LShift, LSHIFT); LUA_KEY(RShift, RSHIFT);
	LUA_KEY(LCtrl, LCONTROL); LUA_KEY(RCtrl, RCONTROL); LUA_KEY(LAlt, LMENU); LUA_KEY(RAlt, RMENU);
	LUA_KEY(Home, HOME); LUA_KEY(End, END); LUA_KEY(Insert, INSERT); LUA_KEY(Delete, DELETE);
	LUA_KEY(PageUp, PRIOR); LUA_KEY(PageDown, NEXT);
	LUA_KEY(F1, F1); LUA_KEY(F2, F2); LUA_KEY(F3, F3); LUA_KEY(F4, F4); LUA_KEY(F5, F5); LUA_KEY(F6, F6);
	LUA_KEY(F7, F7); LUA_KEY(F8, F8); LUA_KEY(F9, F9); LUA_KEY(F10, F10); LUA_KEY(F11, F11); LUA_KEY(F12, F12);
	LUA_KEY(Numpad0, NUMPAD0); LUA_KEY(Numpad1, NUMPAD1); LUA_KEY(Numpad2, NUMPAD2);
	LUA_KEY(Numpad3, NUMPAD3); LUA_KEY(Numpad4, NUMPAD4); LUA_KEY(Numpad5, NUMPAD5);
	LUA_KEY(Numpad6, NUMPAD6); LUA_KEY(Numpad7, NUMPAD7); LUA_KEY(Numpad8, NUMPAD8); LUA_KEY(Numpad9, NUMPAD9);
	LUA_KEY(Minus, MINUS); LUA_KEY(Equals, EQUALS); LUA_KEY(Grave, GRAVE);
	LUA_KEY(LeftBracket, LBRACKET); LUA_KEY(RightBracket, RBRACKET); LUA_KEY(Backslash, BACKSLASH);
	LUA_KEY(Semicolon, SEMICOLON); LUA_KEY(Apostrophe, APOSTROPHE);
	LUA_KEY(Comma, COMMA); LUA_KEY(Period, PERIOD); LUA_KEY(Slash, SLASH);
	LUA_KEY(NumpadAdd, ADD); LUA_KEY(NumpadSub, SUBTRACT); LUA_KEY(NumpadMul, MULTIPLY);
	LUA_KEY(NumpadDiv, DIVIDE); LUA_KEY(NumpadEnter, NUMPADENTER); LUA_KEY(NumpadDecimal, DECIMAL);
#undef LUA_KEY

	sol::table Mouse = m_Lua.create_named_table("Mouse");
	Mouse["Left"] = ETOUI(MOUSEKEYSTATE::LB);
	Mouse["Right"] = ETOUI(MOUSEKEYSTATE::RB);
	Mouse["Middle"] = ETOUI(MOUSEKEYSTATE::MB);

	sol::table MouseMove = m_Lua.create_named_table("MouseMove");
	MouseMove["X"] = ETOUI(MOUSEMOVESTATE::X);
	MouseMove["Y"] = ETOUI(MOUSEMOVESTATE::Y);
	MouseMove["Z"] = ETOUI(MOUSEMOVESTATE::Z);

	return S_OK;
}
