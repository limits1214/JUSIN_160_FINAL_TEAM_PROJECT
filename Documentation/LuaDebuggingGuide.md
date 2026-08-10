# Visual Studio 2022 Lua 디버깅 가이드

## 적용된 프로젝트 설정

- Lua 런타임을 `5.4.8`로 고정했다. 현재 사용하는 네이티브 Visual Studio Lua 디버거는 Lua 5.4까지 지원한다.
- Debug 빌드 시 `Client/Bin`에 `lua_dkm_debug.json`과 `lua.pdb`를 복사한다.
- Lua 청크 이름은 절대 경로로 바꾸지 않고 `./LuaFiles/...` 상대 경로를 유지한다. 디버거는 이 경로와 `lua_dkm_debug.json`의 `ScriptPaths`를 이용해 원본 Lua 파일을 찾는다.
- sol2가 설치하는 기본 traceback error handler를 그대로 사용한다. `safe_script`와 `protected_function` 오류 로그에는 Lua 호출 스택이 포함된다.
- `Documentation/EngineLua.xml`에는 현재 엔진이 바인딩하는 주요 Lua API를 기록했다.

## 필요한 Visual Studio 확장

1. Visual Studio를 모두 종료한다.
2. `ThirdParty/aboutLuaExt/LuaDkmDebugger17.vsix`를 실행해 패치된 Lua 디버거를 설치한다.
3. `ThirdParty/aboutLuaExt/NPLForVisualStudio.vsix`를 실행해 Lua 편집 확장을 설치한다.
4. Visual Studio를 다시 열고 `확장 > 확장 관리 > 설치됨`에서 두 확장을 확인한다.

Visual Studio 확장 검색으로 나오는 원본 `C++ Debugger Extensions for Lua` 0.9.9는 VS2022를 설치 대상으로 인식하지 못한다. 반드시 프로젝트에 포함된 패치 버전을 사용한다.

역할은 다음과 같다.

- `C++ Debugger Extensions for Lua`: C++ 프로세스 안의 Lua 5.4 상태에 붙어 Lua 브레이크포인트, 호출 스택, Locals, Watch, Step Over/Out을 제공한다.
- `NPLForVisualStudio`: `.lua` 파일의 구문 강조와 코드 자동완성을 제공하고 `Documentation/*.xml`을 엔진 API 문서로 읽는다.

## 브레이크포인트 테스트

1. 구성은 `Debug | x64`, 시작 프로젝트는 `Client`로 둔다.
2. `Engine/LuaFiles/Test.lua`의 `Update(dt)` 내부에 브레이크포인트를 건다.
3. Client를 디버그 실행하고 해당 Lua 컴포넌트가 들어 있는 레벨로 진입한다.
4. 브레이크되면 `호출 스택`, `지역`, `조사식` 창에서 Lua 값을 확인한다.

브레이크포인트가 빈 원으로 남으면 다음 순서로 확인한다.

1. `Client/Bin/lua_dkm_debug.json`이 존재하는지 확인한다.
2. `Client/Bin/lua.dll`이 Lua 5.4.8 빌드 결과인지 확인한다.
3. Visual Studio의 `확장 > Lua Debugger`에서 Debug Log를 켜고 Lua Script List에 `./LuaFiles/...`가 표시되는지 확인한다.
4. EmmyLua 등 다른 Lua 디버거가 Lua hook을 점유하고 있다면 끈다. 현재 `CLuaManager::Initialize_DebuggerBinding()`의 옛 Emmy 초기화 코드는 활성화하지 않는다.

Lua에서 C++ 함수 안으로 `Step Into`하는 기능은 이 확장의 제한 사항이다. Lua 코드에는 Lua 브레이크포인트를, 바인딩된 C++ 함수에는 일반 C++ 브레이크포인트를 각각 걸어 사용한다.

## 자동완성 사용

- 솔루션을 다시 연면 `Documentation/EngineLua.xml`이 로드된다.
- Lua에서 `Object.`, `Input.`, `Camera.`, `Collision.`, `DbgLine.` 등을 입력하면 바인딩 API가 후보로 표시된다.
- `gameObject.`, `transform.`도 각각 `GameObject`, `Transform` API 후보를 표시한다.
- C++ 바인딩을 추가하거나 이름을 바꾸면 `EngineLua.xml`도 함께 갱신하고 솔루션을 다시 연다.

## 현재 구조의 주의점

- `self`, `gameObject`, `transform`은 스크립트 실행이 끝난 뒤 Lua 환경에 주입된다. 따라서 파일 최상위 실행문에서는 사용하지 말고 `OnLoad`, `PriorityUpdate`, `FixedUpdate`, `Update`, `LateUpdate` 같은 콜백 안에서 사용한다.
- Hot Reload는 원본 파일을 런타임 폴더에 복사한 뒤 다시 실행한다. 같은 상대 청크 경로를 유지하므로 원본 파일의 브레이크포인트가 계속 연결되어야 한다.
- Lua API를 대규모로 전부 바인딩하기보다 수치 조정, 상태 전환, 연출 타이밍처럼 반복 수정이 잦은 기능부터 제한적으로 노출하는 것이 안전하다.
