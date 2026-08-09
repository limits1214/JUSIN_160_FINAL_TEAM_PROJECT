#pragma once

#include "BTAnimation.h"// 애니매이션 노드
#include "BTTurnSlow.h"//천천히 회전
#include "BTTurnDirect.h"// 바로 회전
#include "BTMove.h"//그냥 이동
#include "BTOnlyTrue.h"//SUCCESSE만 반환
#include "BTOnlyFalse.h"//FAILED만 반환
#include "BTTeleport.h" //대상 위치로 텔포
#include "BTTurnAnimation.h" // 회전 애니매이션이 너무 많아서 하나로 묶는용도
#include "BTAttackAnimation.h" //애니매이션에 이동량 들어가야 될 경우 // 플래그 있음
#include "BTRandMoveAnim.h" //랜덤 움직임 필요한경우
#include "BTHitAnimMonster.h"
#include "BTCreatureFlag.h"// 개별 플래그 세팅용
#include "BTMonAttType.h" // 몬스터 전용
#include "BTMonResetTable.h"
#include "BTCinematic.h"

#include "BTDecIsGround.h"
#include "BTDecHp.h"
#include "BTDecLier.h" //하위 노드 true시 다시 재진입 안함
#include "BTDecTimer.h"//타이머 // 플래그 있음
#include "BTDecSearch.h"//적거리 기반 탐색
#include "BTDecInvert.h"// true false 인버터
#include "BTDecFlag.h" //맞은판정 플래그 있음
#include "BTDecHitCnt.h"
#include "BTDecIsPending.h"


//드래곤용
#include "BTDecEdgState.h"
#include "BTEdgStateFinished.h"
#include "BTDecEdgPhase.h"
#include "BTDecEdgPatroll.h"
