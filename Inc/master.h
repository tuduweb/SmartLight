#ifndef __BIN_MASTER_H
#define __BIN_MASTER_H

#include "stm32f4xx_hal.h"

typedef struct _SYSTEM_TYPEDEF{
    int32_t timestamp;//时间戳 从服务器获取 定时更新时间戳
    uint8_t status;//系统状态 比如运行中..
}SystemTypedef;


//初始化
void MasterInit(void);


#endif