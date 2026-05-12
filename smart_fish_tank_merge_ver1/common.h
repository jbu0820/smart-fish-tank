#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

/*
 * ============================================================
 * TRUE / FALSE 정의
 * ============================================================
 */
#define TRUE    1
#define FALSE   0

/*
 * ============================================================
 * 공통 ON/OFF 상태
 * ============================================================
 *
 * 여러 모듈에서 공통으로 사용할 수 있는 상태입니다.
 *
 * 예:
 * - LED ON/OFF
 * - 펌프 ON/OFF
 * - 레일 동작 ON/OFF
 */
typedef enum
{
    STATE_OFF = 0,
    STATE_ON  = 1
} OnOffState;

/*
 * ============================================================
 * 청소 요청 상태
 * ============================================================
 *
 * 레일 모듈과 연결할 때 사용하는 공통 상태입니다.
 *
 * CLEAN_REQUEST_OFF:
 *   청소 요청 없음
 *
 * CLEAN_REQUEST_ON:
 *   청소 요청 있음
 */
typedef enum
{
    CLEAN_REQUEST_OFF = 0,
    CLEAN_REQUEST_ON  = 1
} CleanRequestState;

/*
 * ============================================================
 * 수질 상태
 * ============================================================
 *
 * 탁도 측정 결과를 표현하는 공통 상태입니다.
 *
 * WATER_STATE_GOOD:
 *   수질 좋음
 *
 * WATER_STATE_NORMAL:
 *   수질 보통
 *
 * WATER_STATE_BAD:
 *   수질 나쁨
 */
typedef enum
{
    WATER_STATE_GOOD = 0,
    WATER_STATE_NORMAL,
    WATER_STATE_BAD
} WaterState;

#endif /* COMMON_H */
