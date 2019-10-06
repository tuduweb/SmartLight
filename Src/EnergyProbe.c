#include "EnergyProbe.h"
//#include "stm32f4xx_hal_uart.h"
#include "main.h"


#define BUILD_U16(addr) ( ((uint16_t)*((uint8_t *)addr) << 8) + *((uint8_t *)addr  + 1) )
#define BUILD_S16(addr) ( ((int16_t)*((uint8_t *)addr) << 8) + *((uint8_t *)addr  + 1) )


typedef struct _QUEUE
{
	qsize _head; //队列头 当前收到消息的位置
	qsize _tail;  //队列尾 当前处理到的位置
	qsize _tempTail;
	int16_t _nextHead;
	qdata _data[QUEUE_MAX_SIZE];	//数据保存位置
}QUEUE;





#define CMD_HEAD 0xFF

typedef enum RegisterPermission{
	Permission_Empty,
	Permission_R,
	Permission_W,
	Permission_RW
}RegisterPermission_e;

typedef enum EnergyProbe_Command{
	EnergyProbe_Configure = 0x00,
	EnergyProbe_BatchReadHex = 0x03,
	EnergyProbe_BatchReadStr,
	EnergyProbe_Temperature = 0x08,
	EnergyProbe_Voltage,
	EnergyProbe_Current,
	EnergyProbe_AverageCurrent

}EnergyProbe_Command_e;

typedef enum EnergyProbe_DataType{
	DataType_HEX,
	DataType_Str,
	DataType_S8,
	DataType_U8,
	DataType_S16,
	DataType_U16,
	DataType_U32
}DataType_e;

typedef struct _Comm{
	uint8_t addr;
	RegisterPermission_e type;
	DataType_e dataType;//返回数据类型
	uint8_t length;//长度 这里可以不要 知道数据类型和读写能算出来
	//这里也可以用指针的形式…直接指向储存区域?or储存函数
	uint8_t data[4];
	
}CommandTypeDef;

static QUEUE que = {0,0,0,-1,0};//ָ队列
static qsize cmd_pos = 0;//当前    位置

extern UART_HandleTypeDef huart1;

uint8_t dataTypeSize[] = {0,0,1,1,2,2,4};

CommandTypeDef comm[] = {
	//似乎我并不需要在乎长度是多少.只要把数据弄出来就好了!?

	{ 0x08, Permission_R , DataType_S8 , 10 },//温度
	{ 0x09, Permission_R , DataType_U16 , 10 },//电压
	{ 0x0A, Permission_R , DataType_S16 , 10 },//电流
	{ 0x0B, Permission_R , DataType_S16 , 10 },//平均电流

	{ 0x00, Permission_RW , DataType_HEX , 10 },//配置参数 这里传入的是配置的HEX 传出的也是.


	{ 0x0D, Permission_R , DataType_U8 , 10 },//相对有效电量
	{ 0x0E, Permission_R , DataType_U8 , 10 },//相对待机电量

	{ 0x0F, Permission_RW , DataType_U32 , 10 },//剩余容量
	{ 0x10, Permission_R , DataType_U16 , 10 },//充满容量
	{ 0x11, Permission_R , DataType_U16 , 10 },//预测剩余运行时间 要搞清楚用的什么算法预测的

	{ 0x16, Permission_R , DataType_U16 , 10 },//电池状态

	{ 0x18, Permission_RW , DataType_U16 , 10 },//设计容量
	{ 0x19, Permission_RW , DataType_U16 , 10 },//充电电压
	{ 0x1A, Permission_RW , DataType_U16 , 10 },//充电电流

};

void Queue_Push(qdata _data)
{
	qsize pos = (que._head + 1 )% QUEUE_MAX_SIZE;
	
	//追尾 队列未满才会PUSH
	if(pos != que._tail)
	{
		que._data[que._head] = _data;
		que._head = pos;
	}
}

//推出数据
static void Queue_Pop(qdata* _data)
{
	if(que._tail!=que._head)
	{
		*_data = que._data[que._tail];
		que._tail = (que._tail+1)%QUEUE_MAX_SIZE;
	}
}



static void Queue_Get(qdata* _data)
{
	//get 和 pop 的区别就是 get是用temp获取的
	if(que._tempTail != que._head)
	{
		*_data = que._data[que._tempTail];
		que._tempTail = ( que._tempTail + 1 ) % QUEUE_MAX_SIZE;
	}
}




static qsize Queue_Size(void)
{
	return ((que._head+QUEUE_MAX_SIZE-que._tempTail)%QUEUE_MAX_SIZE);
}



typedef enum _EnergyProbe_ParseStep{
	EPP_Step_None,
	EPP_Step_Head,
	EPP_Step_Length,
	EPP_Step_Type,
	EPP_Step_Data,
	EPP_Step_Check,
	EPP_Step_ERR,
	EPP_Step_SUCCESS
}EnergyProbe_ParseStep_e;

EnergyProbe_ParseStep_e EPPstep = EPP_Step_Head;



typedef struct _EnergyProbe_ParseData{
	uint8_t addr;
	uint8_t length;
	uint8_t data[4];
}EnergyProbe_ParseDataTypeDef;

EnergyProbe_ParseDataTypeDef EPPdata;

#include <string.h>
#include "stdio.h"

char st[30] = {0};

union _NUMS
{
	uint16_t u16;
	int16_t s16;
	uint32_t u32;
	int32_t s32;
	uint8_t u8;
	int8_t s8;
	uint8_t data[4];
}stNum;


uint8_t EnergyProbe_Parse(void)
{
	//寻找
	qdata _data;
	static uint8_t checksum = CMD_HEAD;
	static int16_t nextHeadPos = -1;
	//static uint8_t dataLength = 0;

	static uint8_t dataRecCnt = 0;
	static uint8_t dataRecMax = 0;

	while(Queue_Size() > 0)
	{
		Queue_Get(&_data);

		if(EPPstep == EPP_Step_Head)
		{
			if(_data == CMD_HEAD)
			{
				//清空参数
				checksum = CMD_HEAD;
				nextHeadPos = -1;
				//dataLength = 0;
				dataRecCnt = 0;
				dataRecMax = 0;
				memset(&EPPdata,0,sizeof(EPPdata));
				EPPstep = EPP_Step_Length;

			}

		}else{

			//校验和相加
			if(EPPstep != EPP_Step_Check)
				checksum += _data;

			//记录回退位置.因为 这时候已经有0xFF 所以下次直接回退到这个位置.不需要再找Head
			if(_data == CMD_HEAD && nextHeadPos < 0)
				nextHeadPos = que._tail;

			if(EPPstep == EPP_Step_Length)
			{
				EPPdata.length = _data;
				EPPstep = EPP_Step_Type;

				if(EPPdata.length > 0)
					dataRecMax = EPPdata.length - 1;
				else
					EPPstep = EPP_Step_Check;

				if(dataRecMax > 4)
					EPPstep = EPP_Step_ERR;


			}else if(EPPstep == EPP_Step_Type)
			{
				EPPdata.addr = _data;
				EPPstep = EPP_Step_Data;

				//根据类型 确定剩余接收
			}else if(EPPstep == EPP_Step_Data)
			{
				//!-----------这里的步骤需要提前---------------
				//储存数据

				//储存完毕 进入下一步骤
				EPPdata.data[dataRecCnt++] = _data;

				if(dataRecCnt >= dataRecMax)
				{
					EPPstep = EPP_Step_Check;
				}

			}else if(EPPstep == EPP_Step_Check)
			{
				//检查环节
				if(checksum != _data)//减掉最后不该加上的
				{
					EPPstep = EPP_Step_ERR;
				}else{
					EPPstep = EPP_Step_SUCCESS;
				}
			}

		}

		
		if(EPPstep == EPP_Step_ERR)
		{
			//err 记录问题
			if(nextHeadPos == -1)
			{
				EPPstep = EPP_Step_Head;
				que._tail = que._tempTail;
			}else{
				//位置退回
				EPPstep = EPP_Step_Length;
				que._tail = nextHeadPos;
				checksum = CMD_HEAD;
			}
			return 'F';//false
		}else if(EPPstep == EPP_Step_SUCCESS)
		{
			//Build the result Package
			que._tail = que._tempTail;
			EPPstep = EPP_Step_Head;

			return 'T';//true
		}



	}

	return 'I';//ing

}


void EnergyProbe_Read(CommandTypeDef* cmd)
{
	uint8_t cmdData[4] = {0};

	cmdData[0] = 0xFF;
	cmdData[1] = 0x01;//Length 固定
	cmdData[2] = cmd->addr;//Command Addr
	cmdData[3] = cmdData[0] + cmdData[1] + cmdData[2];//Checksum


	EnergyProbe_CMD(4,cmdData);
	//wait the respose for receive data
	
	uint32_t receiveTickStart = HAL_GetTick();
	uint32_t wait = 500;//100ms


	while(HAL_GetTick() - receiveTickStart < wait)
	{
		//等待解析
		HAL_Delay(50);
		if (EnergyProbe_Parse() == 'T')
		{
			if(cmd->dataType ==  DataType_U16)
			{
				stNum.u16 = BUILD_U16(EPPdata.data);
				sprintf(st, "%02X %d\n", cmd->addr, stNum.u16);

			}else if(cmd->dataType == DataType_S16)
			{
				stNum.s16 = BUILD_S16(EPPdata.data);
				sprintf(st, "%02X %d\n", cmd->addr, stNum.s16);
			}else if(cmd->dataType == DataType_S8)
			{
				stNum.s8 = (int8_t)EPPdata.data[0];
				sprintf(st, "%02X %d\n", cmd->addr, stNum.s8);
			}
			

			//num = (uint16_t)(*(EPPdata.data+1) << 8);//((uint16_t)*((uint8_t *)EPPdata.data + 1) );

			HAL_UART_Transmit(&huart1, st, strlen(st), 0xffff);
			break;
		}

	}


	
	//Push the result

}

uint8_t EnergyProbe_Write(CommandTypeDef* cmd)
{
	uint8_t cmdData[10] = {0};
	uint8_t cnt = 0;
	uint8_t checksum = 0;
	
	uint32_t receiveTickStart = HAL_GetTick();
	uint32_t wait = 1000;//100ms
	int16_t i = 0;
	uint8_t flag = 'N';

	cmdData[cnt++] = 0xEF;
	cmdData[cnt++] = dataTypeSize[cmd->dataType] + 1;//Length
	cmdData[cnt++] = cmd->addr;//Command Addr

	for(int i = 0; i < dataTypeSize[cmd->dataType] ; ++i )
	{
		cmdData[cnt++] = cmd->data[i];
	}

	for(int i = 0 ; i < cnt ; ++i)
	{
		checksum += cmdData[i];
	}

	cmdData[cnt] = checksum;//Checksum



	EnergyProbe_CMD(cnt + 1,cmdData);//在这里记录发送完成的时间

	
	
	receiveTickStart = HAL_GetTick();

	while(HAL_GetTick() - receiveTickStart < wait)
	{
		HAL_Delay(50);//等待解析
		if( EnergyProbe_Parse() == 'T')
		{
			if(EPPdata.addr == cmd->addr)
			{
				for(i = 0; i < dataTypeSize[cmd->dataType] ; ++i )
				{
					if(EPPdata.data[i] != cmd->data[i])
					{
						flag = 'F';
						break;
					}
				}

				if(i == dataTypeSize[cmd->dataType])
				{
					flag = 'T';
					break;
				}
			}
		}
	}
	if(flag == 'T')
	{
		sprintf(st, "addr %d ok\n", EPPdata.addr);
		HAL_UART_Transmit(&huart1, st, strlen(st), 0xffff);
		return 'T';
	}


	return 'F';




	//wait the respose for check result;
	//	ok	->	
	//	err	->	

}


void EnergyProbe_CMD(uint8_t length,uint8_t *data)
{
	//Real Send
	HAL_UART_Transmit(&huart3,data,length,0xffff);

}



void Export_Function(void)
{
	EnergyProbe_Read(&comm[0]);

	EnergyProbe_Read(&comm[1]);

	EnergyProbe_Read(&comm[2]);

}


void Export_Function1(void)
{
	comm[12].data[0] = 0x2E;
	comm[12].data[1] = 0xE0;
	EnergyProbe_Write(&comm[12]);
}