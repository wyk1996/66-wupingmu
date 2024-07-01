/*****************************************Copyright(C)******************************************
***************************************************************************************
*------------------------------------------文件信息---------------------------------------------
* FileName			: GPRSMain.c
* Author			: 
* Date First Issued	: 
* Version			: 
* Description		: 
*----------------------------------------历史版本信息-------------------------------------------
* History			:
* //2010		    : V
* Description		: 
*-----------------------------------------------------------------------------------------------
***********************************************************************************************/
/* Includes-----------------------------------------------------------------------------------*/
#include "BLELink.h"
#include <string.h>
#include "common.h"
#include "main.h"
#include "uart3.h"
#include "dwin_com_pro.h"
#include "chtask.h"
	#if(USE_BLE==1)	
/* Private define-----------------------------------------------------------------------------*/
#define REASON_BASE  10
/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/
OS_Q RecvBELMq;
//蓝牙控制结构体
/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/
typedef enum
{
	SEND_CHECK_ACK = 0,  		//校验应答
	SEND_DEV_INFO_ACK,			//获取设备信息应答
	SEND_CHARGE_MODE_ACK,		//充电策略录入应答
	SEND_SET_TIME_ACK,			//设置时间
	SEND_CHARGE_ACK,			//启动充电应答
	SEND_CHARGE_INFO_ACK,		//充电中持续发送应答
	SEND_STOP_CHARGE_ACK,		//手动停止充电应答
	SEND_RESERVA_CHARGE_ACK,	//预约充电策应答
	SEND_UNRESERVA_CHARGE_ACK,	//取消预约充电应答
	SEND_BILL_ACK,				//充电记录查询应答
	SEND_FAIL_ACK,				//充电桩故障原因应答
	BLE_SEND_MAX,
}_BLE_SEND_CMD;

typedef enum
{
	SEND_CHECK_RECV = 0,  		//校验
	SEND_DEV_INFO_RECV,			//获取设备信息
	SEND_CHARGE_MODE_RECV,		//充电策略录入
	SEND_SET_TIME_RECV,			//设置时间
	SEND_CHARGE_RECV,			//启动充电
	SEND_CHARGE_INFO_RECV,		//充电中持续发送
	SEND_STOP_CHARGE_RECV,		//手动停止充电
	SEND_RESERVA_CHARGE_RECV,	//预约充电策
	SEND_UNRESERVA_CHARGE_RECV,	//取消预约充电
	SEND_BILL_RECV,				//充电记录查询
	SEND_FAIL_RECV,				//充电桩故障原因
	BLE_RECV_MAX,
}_BLE_RECV_CMD;


typedef struct
{
	uint8_t CmdCode;
	uint8_t FunctionCode;
}_BLE_CMD;

_BLE_CMD SendBLECmdTable[BLE_SEND_MAX] = 
{

	{0x81,0x01},  		//校验设备应答
	{0x81,0x02},		//设备信息应答
	{0x81,0x03},		//充电策略录入应答
	{0x81,0x04},		//设置时间应答
	{0x82,0x01},		//启动充电应答
	{0x82,0x02},		//充电中持续发送应答
	{0x82,0x03},		//手动停止充电应答
	{0x82,0x04},		//预约充电应答
	{0x82,0x05},		//取消预约充电应答
	{0x82,0x06},		//充电记录查询应答
	{0x82,0x07},		//充电桩故障原因应答
};



//校验
__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功，2表示失败
}_BLE_CHECK_ACK;

typedef struct
{
	uint8_t Key[16];			//密钥
}_BLE_CHECK_RECV;




//获取设备信息
__packed typedef struct
{
	uint8_t DevState;			// 设备状态，	1、已连接-插枪2、已连接-未插枪3、已连接-充电中4、已连接-故障5、未连接
	uint8_t	SoftwareVersion[7];	//软件版本ASICC
	uint8_t	HardwareVersion[7];	//硬件版本ASICC
}_BLE_DEV_INFO_ACK;



//充电模式录入
__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功，2表示失败
}_BLE_CHARGE_MODE_ACK;

__packed typedef struct
{
	uint8_t Mode;			//1、蓝牙手动启停充电2、蓝牙连接后自动启停充电
}_BLE_CHARGE_MODE_RECV;

//设置时间
__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功，2表示失败
	uint32_t curtime;			//当前时间戳
}_BLE_SET_TIME_ACK;

__packed typedef struct
{
	uint32_t curtime;			//当前时间戳
}_BLE_SET_TIME_RECV;


//启动充电
__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功，失败原因详见协议说明
	uint8_t	OrderNum[6];		//订单号
}_BLE_CHARGE_ACK;


//停止充电
__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功  2表示失败
}_BLE_STOP_ACK;


//充电信息
__packed typedef struct
{
	uint16_t 	ChargeVol;			//电压 0.01
	uint16_t		ChargeCur;			//电流0.01
	uint32_t	Electric;			//0.001
	uint16_t	ChargeTime;			//充电时长 s
	uint8_t		ChargeState;		//充电状态 1正常充电  2待机  3故障  4充电结束
}_BLE_CHARGE_INFO_ACK;



//预约充电
__packed typedef struct
{
	uint16_t starttime;
	uint16_t stoptime;
}_BLE_RESERVA_TIME;
__packed typedef struct
{
	uint8_t num;				//时间段个数
	_BLE_RESERVA_TIME time[3];	//最多3个
}_BLE_RESERVA_RECV;


__packed typedef struct
{
	uint8_t ifSuccess;			//1表示成功，失败原因详见协议说明
	uint8_t	OrderNum[6];		//订单号
}_BLE_RESERVA_ACK;



//取消预约充电
__packed typedef struct
{
	uint8_t	OrderNum[6];		//订单号
}_BLE_UNRESERVA_RECV;

__packed typedef struct
{
	uint8_t ifSuccess;			//1取消成功 2订单不匹配   3用户不匹配  4其他原因
}_BLE_UNRESERVA_ACK;

//订单查询
__packed typedef struct
{
	uint8_t		OrderNum[6];		//订单号
	uint32_t	StartTime;			//开始时间  时间戳
	uint32_t	StopTime;			//结束时间  时间戳
	uint16_t	Electric;			//充电电量
}_BLE_BILL_ACK;

//查询故障原因
__packed typedef struct
{
	uint8_t DevFail;    //见下方故障原因附录
}_BLE_FAIL_ACK;

__packed typedef struct
{
	uint8_t CmdCode;
	uint8_t FunctionCode;
	uint8_t  (*recvfunction)(uint8_t *,uint8_t);
	uint8_t  (*sendfunction)(void);
}_BLE_RECV_TABLE;



//接收
_BLE_CHECK_ACK BLECheckAck;				//校验应答
_BLE_DEV_INFO_ACK BLEDevInfoAck;		//获取设备信息应答
_BLE_CHARGE_MODE_ACK BLEChargeModeAck;	//充电模式应答
_BLE_SET_TIME_ACK	BLESetTimeAck;		//设置时间应答
_BLE_CHARGE_ACK	BLEChargeAck;			//启动充电	
_BLE_STOP_ACK	BLEStopAck;				//停止充电
_BLE_CHARGE_INFO_ACK BLEInfoAck;		//充电信息
_BLE_RESERVA_ACK	BLEReservaAck;		//预约充电应答
_BLE_UNRESERVA_ACK	BLEUnReservaAck;	//取消预约充电应答
_BLE_BILL_ACK	BLEBillAck;				//订单查询应答
_BLE_FAIL_ACK	BLEFailAck;				//故障查询应答
//发送
_BLE_CHECK_RECV BLECheckRecv;			//校验接收
_BLE_CHARGE_MODE_RECV BLEChargeModeRecv;//充电模式
_BLE_SET_TIME_RECV	BLESetTimeRecv;		//设置时间
_BLE_RESERVA_RECV	BLEReservaRecv;		//余约充电
_BLE_UNRESERVA_RECV	BLEUnReservaRecv;	//取消预约充电
_BLE_CONTROL BLEControl;
extern CH_TASK_T stChTcb;
extern SYSTEM_RTCTIME gs_SysTime;

_BLE_CMD Recv_BLE_CMDCmdTable[BLE_RECV_MAX] = 
{
	{0x67,0x01},  		//校验设备
	{0x67,0x02},		//设备信息
	{0x67,0x03},		//充电策略录入
	{0x67,0x04},		//设置时间
	{0x68,0x01},		//启动充电
	{0x68,0x02},		//充电中持续发送
	{0x68,0x03},		//手动停止充电
	{0x68,0x04},		//预约充电
	{0x68,0x05},		//取消预约充电
	{0x68,0x06},		//充电记录查询
	{0x68,0x07},		//充电桩故障原因
};
//接收处理表

uint8_t   _BLE_CheckRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_ChargeModeRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_SetTimeRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_ReservaRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_UnReservaRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_StartRecv(uint8_t * pdata,uint8_t len);
uint8_t   _BLE_StopRecv(uint8_t * pdata,uint8_t len);

uint8_t   _BLE_SendRegisterAck(void);
uint8_t   _BLE_SendDevInfoAck(void);
uint8_t   _BLE_ChargeModeAck(void);
uint8_t   _BLE_SetTimeAck(void);
uint8_t   _BLE_ChargeAck(void);
uint8_t   _BLE_ChargeInfoAck(void);
uint8_t   _BLE_StopAck(void);
uint8_t   _BLE_ReservaAck(void);
uint8_t   _BLE_UnReservaAck(void);
uint8_t   _BLE_BillAck(void);
uint8_t   _BLE_FailAck(void);

//同步立马回复
_BLE_RECV_TABLE BleRecvTable[BLE_RECV_MAX] = 
{
	{0x67,0x01,				_BLE_CheckRecv		,	_BLE_SendRegisterAck	},  		//校验
	
	{0x67,0x02,				NULL				,	_BLE_SendDevInfoAck		}, 			//获取设备信息

	{0x67,0x03,				_BLE_ChargeModeRecv,	_BLE_ChargeModeAck		}, 		//充电策略录入
	
	{0x67,0x04,				_BLE_SetTimeRecv,		_BLE_SetTimeAck			}, 		//设置时间
	
	{0x68,0x01,				_BLE_StartRecv		,	NULL					}, 			//启动充电
	
	{0x68,0x02,				NULL			,		_BLE_ChargeInfoAck}, 		//充电中持续发送
		
	{0x68,0x03,				_BLE_StopRecv			,		NULL}, 		//手动停止充电
	
	{0x68,0x04,				_BLE_ReservaRecv,		_BLE_ReservaAck}, 	//预约充电策
	
	{0x68,0x05,				_BLE_UnReservaRecv,		_BLE_UnReservaAck}, 	//取消预约充电
	
	{0x68,0x06,				NULL					,_BLE_BillAck}, 				//充电记录查询
	
	{0x68,0x07,				NULL					,_BLE_FailAck},				//充电桩故障原因
};
/**
 * @brief 
 * @param[in]   
 * @param[out]   
 * @return
 * @note
 */
static uint8_t ble_crc_check_sum(uint8_t *pucData, int iDataLen)
{
    uint8_t crc = 0; 
	int i = 0; 
	
    for(i = 0; i < iDataLen; i++)
    {
        crc  += pucData[i]; 
    }
	
    return crc;
}

/*****************************************************************************
* Function     : BLE_SendData
* Description  : 
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年2月9日        
*****************************************************************************/
uint8_t BLE_SendData(void *pdata,uint8_t len,uint8_t CmdCode,uint8_t FunctionCode)
{
	uint8_t buf[20];
	if((pdata == NULL) || (len > 16) )
	{
		return FALSE;
	}
	buf[0] = CmdCode;
	buf[1] = FunctionCode;
	buf[2] = len;
	memcpy(&buf[3],pdata,len);
	buf[3+len] = ble_crc_check_sum(&buf[3],len);
	UART3SENDBUF(buf,4+len);
		return TRUE;
}

/**********************************发送数据**********************************/
/*****************************************************************************
* Function     : _BLE_SendRegisterAck
* Description  : 校验
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_SendRegisterAck(void)
{	
		
	return BLE_SendData(&BLECheckAck,sizeof(BLECheckAck),SendBLECmdTable[SEND_CHECK_ACK ].CmdCode,SendBLECmdTable[SEND_CHECK_ACK ].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_SendDevInfoAck
* Description  : 获取设备信息应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_SendDevInfoAck(void)
{	
	memset(&BLEDevInfoAck,0,sizeof(BLEDevInfoAck));
//	1、已连接-插枪
//2、已连接-未插枪
//3、已连接-充电中
//4、已连接-故障
//5、未连接
	
	if(stChTcb.ucState ==  INSERT)
	{
		BLEDevInfoAck.DevState = 1;
	}
	else if(stChTcb.ucState ==  CHARGING)
	{
		BLEDevInfoAck.DevState = 3;
	}
	else if(stChTcb.ucState ==  CHARGER_FAULT)
	{
		BLEDevInfoAck.DevState = 4;
	}
	else
	{
		BLEDevInfoAck.DevState = 2;		//未插枪
	}
	memcpy(BLEDevInfoAck.SoftwareVersion,"V1.1.0",strlen("V1.1.0"));
	memcpy(BLEDevInfoAck.HardwareVersion,"V1.1.0",strlen("V1.1.0"));
	return BLE_SendData(&BLEDevInfoAck,sizeof(BLEDevInfoAck),SendBLECmdTable[SEND_DEV_INFO_ACK ].CmdCode,SendBLECmdTable[SEND_DEV_INFO_ACK ].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_ChargeModeAck
* Description  充电模式应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ChargeModeAck(void)
{	
	return BLE_SendData(&BLEChargeModeAck,sizeof(BLEChargeModeAck),SendBLECmdTable[SEND_CHARGE_MODE_ACK].CmdCode,SendBLECmdTable[SEND_CHARGE_MODE_ACK ].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_SetTimeAck
* Description  设置时间应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_SetTimeAck(void)
{	
	return BLE_SendData(&BLESetTimeAck,sizeof(BLESetTimeAck),SendBLECmdTable[SEND_SET_TIME_ACK].CmdCode,SendBLECmdTable[SEND_SET_TIME_ACK].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_ChargeAck
* Description  启动充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ChargeAck(void)
{	
	memset(&BLEChargeAck,0,sizeof(BLEChargeAck));
	if(stChTcb.ucState ==  CHARGING)
	{
		BLEChargeAck.ifSuccess = 1;  //充电中说明启动成功
	}
	else
	{
		 BLEChargeAck.ifSuccess = stChTcb.stCh.reason + REASON_BASE;
	}
	
	//订单号为6个字节，为当前时间的年月日，时分秒
	BLEChargeAck.OrderNum[0] = gs_SysTime.ucYear;
	BLEChargeAck.OrderNum[1] = gs_SysTime.ucMonth;
	BLEChargeAck.OrderNum[2] = gs_SysTime.ucDay;
	BLEChargeAck.OrderNum[3] = gs_SysTime.ucHour;
	BLEChargeAck.OrderNum[4] = gs_SysTime.ucMin;
	BLEChargeAck.OrderNum[5] = gs_SysTime.ucSec;
	
	return BLE_SendData(&BLEChargeAck,sizeof(BLEChargeAck),SendBLECmdTable[SEND_CHARGE_ACK].CmdCode,SendBLECmdTable[SEND_CHARGE_ACK].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_ChargeInfoAck
* Description  充电信息
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ChargeInfoAck(void)
{	
	uint8_t i;
	uint32_t kwh = 0;
	for(i = 0;i < (sizeof(stChTcb.stCh.Curkwh)/sizeof(uint32_t));i++)
	{
		kwh +=stChTcb.stCh.Curkwh[i];
	}
	memset(&BLEInfoAck,0,sizeof(BLEInfoAck));
	if(stChTcb.ucState ==  CHARGING)
	{
		BLEInfoAck.ChargeState = 1;  //1正常充电
		BLEInfoAck.ChargeVol = stChTcb.stHLW8112.usVolt*10;
		BLEInfoAck.ChargeCur = stChTcb.stHLW8112.usCurrent;
		BLEInfoAck.Electric = kwh;
		BLEInfoAck.ChargeTime =cp56time(NULL) - stChTcb.stCh.timestart;
	}
	else
	{
		if(stChTcb.ucState ==  CHARGER_FAULT)
		{
			BLEInfoAck.ChargeState = 3;  //3故障
		}
		else
		{
			BLEInfoAck.ChargeState = 2;  //待机
		}
	}
	return BLE_SendData(&BLEInfoAck,sizeof(BLEInfoAck),SendBLECmdTable[SEND_CHARGE_INFO_ACK].CmdCode,SendBLECmdTable[SEND_CHARGE_INFO_ACK].FunctionCode);
}


/*****************************************************************************
* Function     : _BLE_StopAck
* Description   停止充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_StopAck(void)
{	
	BLEStopAck.ifSuccess = 1;
	return BLE_SendData(&BLEStopAck,sizeof(BLEStopAck),SendBLECmdTable[SEND_STOP_CHARGE_ACK].CmdCode,SendBLECmdTable[SEND_STOP_CHARGE_ACK].FunctionCode);
}


/*****************************************************************************
* Function     : _BLE_ReservaAck
* Description  预约充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ReservaAck(void)
{	
	
		//订单号为6个字节，为当前时间的年月日，时分秒
	BLEReservaAck.OrderNum[0] = gs_SysTime.ucYear;
	BLEReservaAck.OrderNum[1] = gs_SysTime.ucMonth;
	BLEReservaAck.OrderNum[2] = gs_SysTime.ucDay;
	BLEReservaAck.OrderNum[3] = gs_SysTime.ucHour;
	BLEReservaAck.OrderNum[4] = gs_SysTime.ucMin;
	BLEReservaAck.OrderNum[5] = gs_SysTime.ucSec;
	return BLE_SendData(&BLEReservaAck,sizeof(BLEReservaAck),SendBLECmdTable[SEND_RESERVA_CHARGE_ACK].CmdCode,SendBLECmdTable[SEND_RESERVA_CHARGE_ACK].FunctionCode);
}


/*****************************************************************************
* Function     : _BLE_UnReservaAck
* Description  取消预约充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_UnReservaAck(void)
{	
	stChTcb.BLEInfo.Reserva = DISABLE;
	if(stChTcb.ucState == CHARGING)
	{
		send_ch_ctl_msg(2,1,4,0);  //结束充电
	}
	return BLE_SendData(&BLEUnReservaAck,sizeof(BLEUnReservaAck),SendBLECmdTable[SEND_UNRESERVA_CHARGE_ACK].CmdCode,SendBLECmdTable[SEND_UNRESERVA_CHARGE_ACK].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_BillAck
* Description  订单查询应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_BillAck(void)
{	
	return BLE_SendData(&BLEBillAck,sizeof(BLEBillAck),SendBLECmdTable[SEND_BILL_ACK].CmdCode,SendBLECmdTable[SEND_BILL_ACK].FunctionCode);
}

/*****************************************************************************
* Function     : _BLE_FailAck
* Description  故障查询应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_FailAck(void)
{	
	return BLE_SendData(&BLEFailAck,sizeof(BLEFailAck),SendBLECmdTable[SEND_FAIL_ACK].CmdCode,SendBLECmdTable[SEND_FAIL_ACK].FunctionCode);
}

/*****************************************************************************
* Function     : BLE_Send_BLEName
* Description  :查询模组名字
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   BLE_Send_BLEName(void)
{	
	uint8_t buf[5];
	
	buf[0] = 0xFA;
	buf[1] = 0x55;
	buf[2] = 0x05;
	buf[3] = 0x00;
	buf[4] = 0x05;
	UART3SENDBUF(buf,5);
		return TRUE;
}

/*****************************************************************************
* Function     : BLE_Send_SetName
* Description  :查询MAC地址
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   BLE_Send_SetName(uint8_t *padd,uint8_t len)
{	
	uint8_t buf[11];
	
	if(padd == NULL || len != 6)
	{
		return FALSE;
	}
	buf[0] = 0xFA;
	buf[1] = 0x55;
	buf[2] = 0x06;
	buf[3] = 0x06;
	memcpy(&buf[4],padd,6);
	buf[10] = ble_crc_check_sum(&buf[2],8);
	UART3SENDBUF(buf,11);
		return TRUE;
}


/*****************************************************************************
* Function     : BLE_Send_LINKStatle
* Description  :查询连接状态
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   BLE_Send_LINKStatle(void)
{	
	uint8_t buf[5];
	
	buf[0] = 0xFA;
	buf[1] = 0x55;
	buf[2] = 0x02;
	buf[3] = 0x00;
	buf[4] = 0x02;
	UART3SENDBUF(buf,5);
	return TRUE;
}
/*********************************接收函数***********************************/
/*****************************************************************************
* Function     : _BLE_CheckRecv
* Description   校验接收
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_CheckRecv(uint8_t * pdata,uint8_t len)
{	
	if(pdata == NULL) 
	{
		return FALSE;
	}
	memcpy(&BLECheckRecv,pdata,len);
	BLECheckAck.ifSuccess = 1;			//校验应答成功
	return TRUE;
}


/*****************************************************************************
* Function     : _BLE_ChargeModeRecv
* Description   充电模式接收
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ChargeModeRecv(uint8_t * pdata,uint8_t len)
{	
	if(pdata == NULL) 
	{
		return FALSE;
	}
	memcpy(&BLEChargeModeRecv,pdata,len);
	//目前不做处理，蓝牙手动启停充电
	return TRUE;
}


/*****************************************************************************
* Function     : _BLE_SetTimeRecv
* Description   时间设置
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_SetTimeRecv(uint8_t * pdata,uint8_t len)
{	

	static CP56TIME2A_T cp56time;
	if(pdata == NULL)
	{
		return FALSE;
	}
	
	memcpy(&BLESetTimeRecv,pdata,len);
	localtime_to_cp56time(BLESetTimeRecv.curtime,&cp56time);
	
	set_date(cp56time.ucYear + 2000,cp56time.ucMon&0x0f,cp56time.ucDay&0x1f);
    set_time(cp56time.ucHour,cp56time.ucMin,cp56time.usMsec/1000);
	return TRUE;
}


/*****************************************************************************
* Function     : _BLE_StartRec
* Description   启动充电
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_StartRecv(uint8_t * pdata,uint8_t len)
{	
	stChTcb.BLEInfo.Reserva  = DISABLE;  //取消预约
	send_ch_ctl_msg(1,1,4,0);  //启动充电
	return TRUE;
}



/*****************************************************************************
* Function     : _BLE_StopRecv
* Description   停止充电
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_StopRecv(uint8_t * pdata,uint8_t len)
{	
	stChTcb.BLEInfo.Reserva = DISABLE;
	send_ch_ctl_msg(2,1,4,0);  //启动充电
	return TRUE;
}

/*****************************************************************************
* Function     : _BLE_ReservaRecv
* Description   预约充电
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_ReservaRecv(uint8_t * pdata,uint8_t len)
{	
	uint8_t i;
	if(pdata == NULL) 
	{
		return FALSE;
	}
	memcpy(&BLEReservaRecv,pdata,len);
	for(i = 0;i < BLEReservaRecv.num;i++)
	{
		if(BLEReservaRecv.time[i].starttime == BLEReservaRecv.time[i].stoptime)
		{
			BLEReservaAck.ifSuccess = 0;  //预约成功
			return TRUE;
		}
		else
		{
			stChTcb.BLEInfo.time[i].starttime =  BLEReservaRecv.time[i].starttime;
			stChTcb.BLEInfo.time[i].stoptime = BLEReservaRecv.time[i].stoptime;
		}
	}
	
	BLEReservaAck.ifSuccess = 1;
	stChTcb.BLEInfo.num = BLEReservaRecv.num;
	stChTcb.BLEInfo.Reserva =ENABLE;
	return TRUE;
}

/*****************************************************************************
* Function     : _BLE_UnReservaRecv
* Description   取消预约充电
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   _BLE_UnReservaRecv(uint8_t * pdata,uint8_t len)
{	
	if(pdata == NULL)
	{
		return FALSE;
	}
	memcpy(&BLEUnReservaRecv,pdata,len);
	//对比订单号是否正确
	if(stChTcb.BLEInfo.Reserva !=ENABLE)
	{
		BLEUnReservaAck.ifSuccess = 4;		//其他原因
		return TRUE;
	}
	if(strncmp((char*)BLEUnReservaRecv.OrderNum,(char *)BLEReservaAck.OrderNum,sizeof(BLEReservaAck.OrderNum)) != 0)
	{
		BLEUnReservaAck.ifSuccess = 2;	//订单号不一致
		return TRUE;
	}
	BLEUnReservaAck.ifSuccess = 1;  //取消预约成功
	stChTcb.BLEInfo.Reserva  = DISABLE;
	return TRUE;
}

/*****************************************************************************
* Function     : _BLE_UnReservaRecv
* Description   接收处理
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   BLE_RecvDesposeCmd(uint8_t * pdata,uint8_t len)
{
	uint8_t i;
	uint8_t CmdCode;
	uint8_t FunctionCode;
	
	if(((pdata == NULL)  || (len < 4)) )
	{
		return FALSE;
	}
//	if((len != (datalen + 4)) || (len != (datalen + 3)) )
//	{
//		return FALSE;
//	}
	CmdCode = pdata[0];
	FunctionCode = pdata[1];
	for(i = 0;i < BLE_RECV_MAX;i++)
	{
		if((BleRecvTable[i].CmdCode == CmdCode) && (BleRecvTable[i].FunctionCode == FunctionCode))
		{
			if(BleRecvTable[i].recvfunction != NULL)
			{
				BleRecvTable[i].recvfunction(&pdata[3],len);  //执行接收函数
			}
			if(BleRecvTable[i].sendfunction != NULL)
			{
				BleRecvTable[i].sendfunction();				//同步接收返回
			}
			return TRUE;
		}
	}
	return FALSE;
}

/*****************************************************************************
* Function     : BLE_UnlinkRecvDesposeCmd
* Description   蓝牙连接接收处理
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2023年02月9日
*****************************************************************************/
uint8_t   BLE_UnlinkRecvDesposeCmd(uint8_t * pdata,uint8_t len)
{
	uint8_t datalen;
	uint8_t cmd;
	
	if((pdata == NULL) || !len)
	{
		return FALSE;
	}
	if(len < 5)
	{
		return FALSE;
	}
	datalen = pdata[3];  //提取数据长度
	if((datalen + 5) != len)
	{
		return FALSE;
	}
	
	if((pdata[0] != 0xfa) || (pdata[1] != 0x55) )
	{
		return FALSE;
	}
	cmd = pdata[2];
	
	
	if(cmd == 0x05)  //查询名称
	{
		memcpy(BLEControl.MACAdd,&pdata[4],MIN(datalen,sizeof(BLEControl.MACAdd)));  //拷贝名字
	}
	if(cmd == 0x02)  //查询连接状态
	{
		if(pdata[4])
		{
			BLEControl.BLELinkState = BLE_LINK;
		}
		else
		{
			BLEControl.BLELinkState = BLE_UNLINK;
		}
	}
	return TRUE;
	
}
//专门做蓝牙发送接收处理
uint8_t bltbuf[MSG_RECV_LEN];
/*****************************************************************************
* Function     : TaskGPRSRecv
* Description  : 
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月16日        
*****************************************************************************/
void AppTaskBELLink(void *pdata)
{
	OS_ERR ERR;
	MQ_MSG_T stMsg = {0};
	static uint32_t curtime = 0,lasttime = 0,linkcurtime = 0,linklasttime = 0,curtimeout = 0,lasttimeout = 0;

	
	UART3BTinit();
	_BLE_RET_ON;		
	OSQCreate (&RecvBELMq,
                 "RecvBELMq send mq",
                 20,
                 &ERR);
	if(ERR != OS_ERR_NONE)
	{
		printf("OSQCreate %s Fail", "BEL send mq");
		return;
	}
	memset(&BLEControl,0,sizeof(BLEControl));
	while(1)
    {
		
		if(mq_service_recv_msg(&RecvBELMq,&stMsg,bltbuf,sizeof(bltbuf),CM_TIME_1_SEC) == 0 )
		{
			 if(stMsg.uiSrcMoudleId == CM_UNDEFINE_ID)
			 {
				if(BLEControl.BLELinkState == BLE_LINK)   //
				{
					if(BLE_RecvDesposeCmd(stMsg.pucLoad,stMsg.uiLoadLen)) //蓝牙接收处理
					{
						lasttimeout = curtimeout;
					}
				}
				else
				{
					BLE_UnlinkRecvDesposeCmd(stMsg.pucLoad,stMsg.uiLoadLen);  //蓝牙未连接接收处理
				}
			 }
			 if(stMsg.uiSrcMoudleId == CM_CHTASK_MODULE_ID)
			 {
			  switch(stMsg.uiMsgCode)
				{
					case CH_TO_DIP_STARTSUCCESS: 	//启动成功
					case CH_TO_DIP_STARTFAIL:    	//启动失败
						_BLE_ChargeAck();
					case CH_TO_DIP_STOP:			//停止时
						_BLE_StopAck();
						break;

					default:
						break;
				}
			}
		}
		
		curtimeout = OSTimeGet(&timeerr);
		linkcurtime = OSTimeGet(&timeerr);
		curtime = OSTimeGet(&timeerr);
		if(BLEControl.BLELinkState == BLE_UNLINK)
		{
			//需要周期性发送读取连接状态和蓝牙名字
			if((curtime >= lasttime) ? ((curtime - lasttime) >= CM_TIME_2_SEC) : \
			((curtime + (0xffffffffU - lasttime)) >= CM_TIME_2_SEC))
            {
                lasttime = curtime;
				if(!strncmp((char*)BLEControl.MACAdd,(char*)&DisSysConfigInfo.DevNum[2],6))  //一样
				{
					__NOP();
				}
				else   //不一样
				{
//					_BLE_RET_OFF;
//					OSTimeDly(100, OS_OPT_TIME_PERIODIC, &ERR);   //设备号比一样发送之前复位
//					_BLE_RET_ON;
//					OSTimeDly(100, OS_OPT_TIME_PERIODIC, &ERR);   //设备号比一样发送之前复位
					BLE_Send_BLEName();			//MAC地址
					OSTimeDly(100, OS_OPT_TIME_PERIODIC, &ERR);   //设备号比一样发送之前复位
					BLE_Send_SetName(&DisSysConfigInfo.DevNum[2],6);   //从第三个开去取，一共取6个
				}
            }
			if((linkcurtime >= linklasttime) ? ((linkcurtime - linklasttime) >= CM_TIME_3_SEC) : \
			((linkcurtime + (0xffffffffU - linklasttime)) >= CM_TIME_3_SEC))
            {
                linklasttime = linkcurtime;
				BLE_Send_LINKStatle();			//连接状态
            }
		}
		else
		{
			//判断是否未连接
			if((curtimeout >= lasttimeout) ? ((curtimeout - lasttimeout) >= CM_TIME_5_MIN) : \
			((curtimeout + (0xffffffffU - lasttimeout)) >= CM_TIME_5_MIN))
            {
				BLEControl.BLELinkState = BLE_UNLINK;
				memset(BLEControl.MACAdd,0,sizeof(BLEControl.MACAdd));
                lasttimeout = curtimeout;
				BLE_Send_LINKStatle();			//连接状态
            }
			
		}
		
	}	
	
}
#endif
/************************(C)COPYRIGHT *****END OF FILE****************************/

