#include "EnergyProbe.h"
//#include "stm32f4xx_hal_uart.h"
#include "main.h"

typedef struct _QUEUE
{
	qsize _head; //队列头 当前收到消息的位置
	qsize _tail;  //队列尾 当前处理到的位置
	qsize _tempTail;
	qsize _nextHead;
	qdata _data[QUEUE_MAX_SIZE];	//数据保存位置
}QUEUE;

static QUEUE que = {0,0,0,-1,0};//ָ队列
static qsize cmd_pos = 0;//当前    位置




#define CMD_HEAD 0xFF
extern UART_HandleTypeDef huart1;

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

uint8_t dataTypeSize[] = {0,0,1,1,2,2,4};

typedef struct _Comm{
	uint8_t addr;
	RegisterPermission_e type;
	DataType_e dataType;//返回数据类型
	uint8_t length;//长度 这里可以不要 知道数据类型和读写能算出来
	//这里也可以用指针的形式…直接指向储存区域?or储存函数
	uint8_t data[4];
	
}CommandTypeDef;

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

	{ 0x18, Permission_RW , DataType_U8 , 10 },//设计容量
	{ 0x19, Permission_RW , DataType_U8 , 10 },//充电电压
	{ 0x1A, Permission_RW , DataType_U8 , 10 },//充电电流

};

void Queue_Push(qdata data)
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
static void Queue_Pop(qdata* data)
{
	if(que._tail!=que._head)//非空状态
	{
		*_data = que._data[que._tail];
		que._tail = (que._tail+1)%QUEUE_MAX_SIZE;
	}
}



static void Queue_Get(qdata* data)
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
	return ((que._head+QUEUE_MAX_SIZE-que._tail)%QUEUE_MAX_SIZE);
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

EnergyProbe_ParseStep_e EPPstep;



typedef struct _EnergyProbe_ParseData{
	uint8_t addr;
	uint8_t length;
	uint8_t data[4];
}EnergyProbe_ParseData_e;

EnergyProbe_ParseData_e EPPdata;

uint8_t EnergyProbe_Parse(void)
{
	//寻找
	qdata _data;
	static uint8 checksum = 0;
	int16_t nextHeadPos = -1;
	uint8_t dataLength = 0;

	uint8_t dataRecCnt = 0;
	uint8_t dataRecMax = 0;

	while(Queue_Size() > 0)
	{
		Queue_Get(&_data);

		if(EPPstep == EPP_Step_Head)
		{
			if(_data == CMD_HEAD)
				EPPstep = EPP_Step_Length;

		}else{

			//校验和相加
			checksum += _data;

			//记录回退位置.因为 这时候已经有0xFF 所以下次直接回退到这个位置.不需要再找Head
			if(_data == CMD_HEAD && nextHeadPos < 0)
				nextHeadPos = pos.tail;

			if(EPPstep == EPP_Step_Length)
			{
				EPPdata.length = _data;

			}else if(EPPstep == EPP_Step_Type)
			{
				EPPdata.addr = _data;
				//根据类型 确定剩余接收
			}else if(EPPstep == EPP_Step_Data)
			{
				//!-----------这里的步骤需要提前---------------
				//储存数据
				EPPdata.data[dataRecCnt++] = _data;

				//储存完毕 进入下一步骤
				if(dataRecCnt == dataRecMax)
					EPPstep = EPP_Step_Check;

			}else if(EPPstep == EPP_Step_Check)
			{
				//检查环节
				if(checksum != _data)
				{
					EPPstep = EPP_Step_ERR;
				}else{
					EPPstep = EPP_Step_OK;
					//Push the result
				}
			}

		}

		
		if(EPPstep == EPP_Step_ERR)
		{
			//err 记录问题
			if(nextHeadPos == -1)
			{
				EPPstep = EPP_Step_Head;
			}else{
				//位置退回
				pos.tail = nextHeadPos;
				EPPstep = EPP_Step_Length;
			}
			return 'F';
		}else if(EPPstep == EPP_Step_SUCCESS)
		{
			return 'T';
		}



	}
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
	uint32_t wait = 200;//100ms

	while(HAL_GetTick() - tickStart < wait)
	{
		//等待解析
		EnergyProbe_Parse();
	}
}

void EnergyProbe_Write(CommandTypeDef* cmd)
{
	uint8_t cmdData[10] = {0};
	uint8_t cnt = 0;
	uint8_t checksum = 0;

	cmdData[cnt++] = 0xEF;
	cmdData[cnt++] = dataTypeSize[cmd.dataType] + 1;//Length
	cmdData[cnt++] = cmd.addr;//Command Addr

	for(int i = 0; i < dataTypeSize[cmd.dataType] ; ++i )
	{
		cmdData[cnt++] = data[i];
	}

	for(int i = 0 ; i < cnt ; ++i)
	{
		checksum += comData[i];
	}

	cmdData[cnt] = checksum;//Checksum


	EnergyProbe_CMD(cnt + 1,cmdData);
	//在这里记录发送完成的时间

	uint32_t receiveTickStart = HAL_GetTick();
	uint32_t wait = 100;//100ms

	while(HAL_GetTick() - tickStart < wait)
	{
		//等待解析
		EnergyProbe_Parse();
	}


	//wait the respose for check result;
	//	ok	->	
	//	err	->	

}


void EnergyProbe_CMD(uint8_t length,uint8_t *data)
{
	//Real Send
	HAL_UART_Transmit(&huart1,data,length,0xffff);
}