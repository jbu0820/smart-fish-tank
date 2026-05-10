#ifndef TURBIDITY_H
#define TURBIDITY_H

#include <stdint.h>
#include "common.h"

typedef enum
{
    TURBIDITY_STATE_IDLE = 0,
    TURBIDITY_STATE_RUN,
    TURBIDITY_STATE_DECIDE,
    TURBIDITY_STATE_OUTPUT,
    TURBIDITY_STATE_DONE
} TurbidityFsmState;

typedef struct
{
    WaterState        waterState;
    CleanRequestState cleanRequest;
    TurbidityFsmState fsmState;

    uint16_t          turbidityValue;
    uint16_t          ambientValue;
    uint16_t          ledOnAverageValue;
} TurbidityData;

void Turbidity_Init(void);
void Turbidity_Update(void);
void Turbidity_TimerTick1ms(void);

void Turbidity_RequestMeasure(void);
void Turbidity_RequestAfterFeedingCheck(void);

WaterState        Turbidity_GetWaterState(void);
CleanRequestState Turbidity_GetCleanRequest(void);
TurbidityFsmState Turbidity_GetFsmState(void);

uint16_t          Turbidity_GetValue(void);
uint16_t          Turbidity_GetAmbientValue(void);
uint16_t          Turbidity_GetLedOnAverageValue(void);

void Turbidity_GetData(TurbidityData *data);

#endif /* TURBIDITY_H */