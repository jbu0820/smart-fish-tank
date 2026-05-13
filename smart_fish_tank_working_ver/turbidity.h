#ifndef TURBIDITY_H
#define TURBIDITY_H

#include <stdint.h>
#include "common.h"

/*
 * ============================================================
 * 탁도 측정 FSM 상태
 * ============================================================
 *
 * IDLE:
 *   측정 시작 조건을 기다리는 상태
 *
 * RUN:
 *   실제 조도센서 측정을 수행하는 상태
 *
 * DECIDE:
 *   측정값을 기준으로 수질 상태를 판단하는 상태
 *
 * OUTPUT:
 *   판단 결과를 내부 상태값에 반영한 상태
 *
 * DONE:
 *   플래그를 정리하고 IDLE로 복귀하는 상태
 */
typedef enum
{
    TURBIDITY_STATE_IDLE = 0,
    TURBIDITY_STATE_RUN,
    TURBIDITY_STATE_DECIDE,
    TURBIDITY_STATE_OUTPUT,
    TURBIDITY_STATE_DONE
} TurbidityFsmState;

/*
 * ============================================================
 * 탁도 모듈 데이터 구조체
 * ============================================================
 *
 * 다른 모듈에서 탁도 관련 값을 한 번에 가져갈 때 사용합니다.
 *
 * waterState:
 *   현재 수질 상태
 *
 * cleanRequest:
 *   레일 청소 요청 상태
 *
 * fsmState:
 *   현재 탁도 FSM 상태
 *
 * turbidityValue:
 *   주변광 보정 후 빛 통과량
 *   DIFF = ledOnValue - ledOffValue
 *
 * ledOffValue:
 *   측정 LED OFF 상태에서 측정한 주변광 기준값
 *
 * ledOnValue:
 *   측정 LED ON 상태에서 측정한 조도값
 */
typedef struct
{
    WaterState         waterState;
    CleanRequestState cleanRequest;
    TurbidityFsmState fsmState;

    uint16_t turbidityValue;
    uint16_t ledOffValue;
    uint16_t ledOnValue;
} TurbidityData;

/*
 * ============================================================
 * 초기화 함수
 * ============================================================
 *
 * 프로그램 시작 시 main.c에서 한 번 호출합니다.
 */
void Turbidity_Init(void);

/*
 * ============================================================
 * FSM 업데이트 함수
 * ============================================================
 *
 * main.c의 while(1) 안에서 계속 호출합니다.
 */
void Turbidity_Update(void);

/*
 * ============================================================
 * 수동 측정 요청
 * ============================================================
 *
 * 버튼 입력이나 외부 조건으로 즉시 측정을 요청할 때 사용합니다.
 */
void Turbidity_RequestMeasure(void);

/*
 * ============================================================
 * 먹이 배급 후 측정 요청
 * ============================================================
 *
 * 먹이 배급 후 일정 시간이 지난 뒤,
 * 먹이 담당 모듈에서 호출할 수 있습니다.
 */
void Turbidity_RequestAfterFeedingCheck(void);

/*
 * ============================================================
 * Getter 함수
 * ============================================================
 */
WaterState Turbidity_GetWaterState(void);
CleanRequestState Turbidity_GetCleanRequest(void);
TurbidityFsmState Turbidity_GetFsmState(void);

uint16_t Turbidity_GetValue(void);
uint16_t Turbidity_GetLedOffValue(void);
uint16_t Turbidity_GetLedOnValue(void);

/*
 * ============================================================
 * 구조체 포인터 방식 데이터 전달 함수
 * ============================================================
 *
 * 여러 값을 한 번에 가져오고 싶을 때 사용합니다.
 *
 * 사용 예:
 *
 * TurbidityData data;
 * Turbidity_GetData(&data);
 */
void Turbidity_GetData(TurbidityData *data);
void turbidity(TurbidityData *turbidityData);

#endif /* TURBIDITY_H */
