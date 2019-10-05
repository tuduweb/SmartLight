#ifndef __BIN_ENERGYPROBE_H
#define __BIN_ENERGYPROBE_H

#include "stm32f4xx_hal.h"

#define RECDATA_SIZE 100

extern UART_HandleTypeDef huart3;

typedef struct{
	uint8_t recData[RECDATA_SIZE];
	int16_t pos;
}QueueTypeDef;

extern QueueTypeDef probeRec;


#define QUEUE_MAX_SIZE 80   /*!< ָ����ջ�������С��������Ҫ�������������ô�һЩ*/

typedef unsigned char qdata;
typedef unsigned short qsize;


extern void queue_push(qdata _data);



extern void EnergyProbe_RxCallBack(void);
extern void Export_Function(void);

#endif
