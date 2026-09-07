<div align="center">

# Hogwarts Legacy Clone

<img src="docs/media/hogwarts-legacy-key-art.jpg" width="620" alt="Hogwarts Legacy main key art">

### C++20 · DirectX 11 Custom Engine · 7인 팀 프로젝트

**자체 제작 엔진으로 구현한 3D 액션 RPG 《호그와트 레거시》 모작**

</div>

## 시연영상

[시연영상 보기](https://youtu.be/nxZvOzm6py0)

## 프로젝트 정보

| 항목 | 내용 |
| --- | --- |
| 장르 | 3인칭 액션 RPG |
| 개발 기간 | 2026.06.30 ~ 2026.08.30 |
| 개발 인원 | 7명 |
| 플랫폼 | Windows x64 |
| 개발 환경 | Visual Studio 2022 |
| 제작 범위 | Engine · Client · Map Editor · Asset Pipeline |
| 기술 스택 | • C++20 · HLSL<br>• DirectX 11 · HBAO+<br>• PhysX 5.6.1 · NvCloth<br>• Recast/Detour<br>• FMOD<br>• Dear ImGui · ImGuizmo · Assimp · Tracy · vcpkg |

## 주요 콘텐츠

- Hogwarts·Hogsmeade 월드 탐험과 NPC 상호작용
- Accio, Depulso, Descendo, Confringo, Bombarda 기반 주문 전투
- Pensieve Guardian·Ranrok Dragon 보스전
- 빗자루 비행과 코인 타임어택
- 물리 기반 Summoner's Court 미니게임
- 상점·퀘스트·대화·시네마틱·사운드 시스템

## 담당 파트

<table>
  <thead>
    <tr>
      <th>팀원</th>
      <th>담당 파트</th>
      <th>구현 내용</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td>
        <a href="https://github.com/EnoughKK"><strong>EnoughKK</strong></a>
      </td>
      <td>UI · UI Tools</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • UIManager·UIController<br>
        • UI Prefab·Flipbook·Tween<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • HUD·스킬 슬롯·미니맵·몬스터 HP<br>
        • 로딩·사망·페이드·상점·대화·퀘스트·미니게임 UI<br>
        <br>
        <strong>툴</strong><br>
        • UI Editor
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/ForestMS1"><strong>ForestMS1</strong></a>
      </td>
      <td>World · Map Tools</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • Chunk Streaming·Instancing<br>
        • GPU Frustum·Hi-Z Culling·Indirect Draw<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • Hogwarts·Hogsmeade 월드 제작 기반<br>
        • Cinematic·Camera 시스템과 비행 콘텐츠 기반<br>
        <br>
        <strong>툴</strong><br>
        • Map Editor
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/gnffk"><strong>gnffk</strong></a>
      </td>
      <td>Player · Animation</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • Animator·GPU Skinning<br>
        • Root Motion·Montage·Socket·Blending<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • 플레이어 FSM·이동·전투·주문·타기팅<br>
        • 빗자루 비행·Foot IK·NPC 상호작용<br>
        <br>
        <strong>툴</strong><br>
        • Animation Editor
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/lcjune816"><strong>lcjune816</strong></a>
      </td>
      <td>AI · Enemy/Boss</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • Behavior Tree·Blackboard<br>
        • 공통 몬스터 전투 AI<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • 거미·트롤·NPC·동물 행동<br>
        • Pensieve Guardian·Ranrok Dragon 전투 패턴<br>
        <br>
        <strong>툴</strong><br>
        • Behavior Tree Node Editor
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/limits1214"><strong>limits1214</strong></a> · 임성윤
      </td>
      <td>Framework · Physics</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • 객체 생명주기·Generation Handle·Object Pool<br>
        • Fixed Update·Deferred Destruction·PhysX 컴포넌트 계층<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • Summoner's Court 경기 시스템<br>
        • Physics Door·Ragdoll·물리 Prop<br>
        <br>
        <strong>툴</strong><br>
        • Physics Authoring Tools
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/Lung-Ji"><strong>Lung-Ji</strong></a>
      </td>
      <td>Shader · Lighting</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • Deferred PBR·Renderer Pipeline<br>
        • Light·Shadow·CSM·HBAO·Post Process<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • Volumetric Fog·Cloud·God Ray<br>
        • Bloom·TAA·Radial Blur·레벨 조명<br>
        <br>
        <strong>툴</strong><br>
        • Light Placement Editor
      </td>
    </tr>
    <tr>
      <td>
        <a href="https://github.com/sungmin2ee"><strong>sungmin2ee</strong></a>
      </td>
      <td>Effect · Particle</td>
      <td>
        <strong>핵심 시스템</strong><br>
        • Effect·Particle Manager<br>
        • CPU/GPU Particle·Beam·Trail<br>
        <br>
        <strong>콘텐츠 구현</strong><br>
        • Particle Shader·Pattern·렌더 최적화<br>
        • 플레이어 주문·보스·월드 이펙트<br>
        <br>
        <strong>툴</strong><br>
        • Particle Effect Editor
      </td>
    </tr>
  </tbody>
</table>
