#ifndef __BIN_ENERGYPROBE_H
#define __BIN_ENERGYPROBE_H

#include "stm32f4xx_hal.h"


extern UART_HandleTypeDef huart3;



#define QUEUE_MAX_SIZE 80

typedef unsigned char qdata;
typedef unsigned short qsize;


extern void Queue_Push(qdata _data);



extern void EnergyProbe_CMD(uint8_t length,uint8_t *data);


//extern uint8_t EnergyProbe_Write(CommandTypeDef* cmd);


extern void Export_Function1(void);
extern void Export_Function(void);


#endif
