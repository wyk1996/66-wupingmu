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
#include "4GMain.h"
#include "4GRecv.h"
#include <string.h>
#include "DwinProtocol.h"
#include "dwin_com_pro.h"
#include "main.h"
#include "Frame66.h"
/* Private define-----------------------------------------------------------------------------*/
#define   GPRSSEND_Q_LEN  								20
/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/


uint8_t OFFlineBuf[300];		//离线交易记录写入读取换成
uint8_t OFFFSlineBuf[300];	//离线分时交易记录写入读取换成

OS_Q Send4GMq;
/*****************************************************************************
* Function     : Task4GSend
* Description  : 4G发送任务
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月14日
*****************************************************************************/
void AppTask4GSend(void *p_arg)
{
    OS_ERR err;
    MQ_MSG_T stMsg = {0};
    static uint8_t sendbuf[10];
    uint8_t* pBillGunA = APP_GetBillInfo();

    uint8_t i = 0;
    static uint32_t curtime = 0,lasttime = 0;

    OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &timeerr);
    if(DisSysConfigInfo.standaloneornet != DISP_NET)
    {
        return;
    }

    OSQCreate (&Send4GMq,
               "4G send mq",
               20,
               &err);
    if(err != OS_ERR_NONE)
    {
        printf("OSQCreate %s Fail", "4G send mq");
        return;
    }
    //mq_service_bind(CM_4GSEND_MODULE_ID,"4G send mq");
    while(1)
    {
        if(mq_service_recv_msg(&Send4GMq,&stMsg,sendbuf,sizeof(sendbuf),CM_TIME_1_SEC) == 0 )
        {
            if(APP_GetSIM7600Mode() == MODE_HTTP)   //远程升级其他无关数据帧都不不要发送和处理
            {
                continue;
            }
            switch(stMsg.uiMsgCode)   //消息码
            {
            case APP_START_ACK:		//开始充电应答
                if((APP_GetSIM7600Status() == STATE_OK) && (APP_GetModuleConnectState(0) == STATE_OK)) //连接上服务器
                {
                    _4G_SendStartAck();
                    if(NetConfigInfo.NetNum > 1)
                    {
                        ZF_SendStartCharge();				//发送启动帧
                    }
                }
                break;

            case APP_STOP_ACK:		//停止充电应答
                if((APP_GetSIM7600Status() == STATE_OK) && (APP_GetModuleConnectState(0) == STATE_OK)) //连接上服务器
                {
                    _4G_SendStopAck();
                }
                break;
            case APP_STOP_BILL:		//停止结算
                WriterFmBill(1);
                APP_SetResendBillState(TRUE);
                _4G_SendBill();
                _4G_SetStartType(_4G_APP_START);			//设置为APP
                if(NetConfigInfo.NetNum > 1)
                {
                    ZF_SendBill();
                }
                break;

            case	APP_SJDATA_QUERY:     //读取实时数据
                _66_SendDataGunACmd();
                break;
            case	APP_STE_BILL:     //查询订单
                ReSendBill(pBillGunA,1);
                break;
            case APP_RATE_ACK:		//停止结算
                _4G_SendRateAck(stMsg.uiMsgVar);	 //费率设置应答
                break;
            case APP_QUERY_RATE:	//费率请求
                _4G_SendQueryRate();
                break;
            case APP_RATE_MODE:		//计费模型验证
                _4G_SendRateMode();
                break;
            case APP_STE_TIME:			//校准时间
                _4G_SendSetTimeAck();
                break;
            case APP_SETQR_ACK:						//设置二维码应答
                _4G_SendSetQR();
                break;

            case APP_OTA_ACK:						//远程升级应答
                _4G_SendOTA();
                break;

            case BSP_4G_SENDNET1:
                if(NetConfigInfo.NetNum > 1)
                {
                    Send_AT_CIPRXGET(0);   //主动读取数据
                }
                break;
            case BSP_4G_SENDNET2:
                if(NetConfigInfo.NetNum > 1)
                {
                    Send_AT_CIPRXGET(1);	//主动读取数据
                }
                break;
            default:
                break;

            }
        }
        if(APP_GetSIM7600Mode() == MODE_HTTP)   //远程升级其他无关数据帧都不不要发送和处理
        {
            continue;
        }

        for(i = 0; i < NetConfigInfo.NetNum; i++)
        {
            if((APP_GetSIM7600Status() == STATE_OK) && (APP_GetModuleConnectState(i) == STATE_OK)) //连接上服务器
            {
                //发送数据给服务器
                //ModuleSIM7600_SendData(i, (uint8_t*)"hello word qiangge\r\n", strlen("hello word qiangge\r\n"));
                if(i == 0)
                {
                    _4G_SendFrameDispose();  //周期性发送帧
                }
                else
                {
                    ZF_SendFrameDispose();
                }
            }
        }
        if (NetConfigInfo.NetNum > 1)
        {
            curtime = OSTimeGet(&timeerr);
            //每10s读取一次信号强度
            if((curtime >= lasttime) ? ((curtime - lasttime) >= CM_TIME_10_SEC) : \
                    ((curtime + (0xffffffffU - lasttime)) >= CM_TIME_10_SEC))
            {
                lasttime = curtime;
                Send_AT_CSQ();				//读取信号强度
            }
        }
        //处理发送订单
        if(APP_GetResendBillState() == TRUE)
        {
            if((APP_GetSIM7600Status() == STATE_OK) && (APP_GetModuleConnectState(0) == STATE_OK) && (APP_GetAppRegisterState(0) == STATE_OK)	) //连接上服务器
            {
                ReSendBill(pBillGunA,0);
            }
        }
        else
        {
            ResendBillControl.CurTime = OSTimeGet(&timeerr);		//获取当前时间
            ResendBillControl.LastTime = ResendBillControl.CurTime;
        }
    }
}



/************************(C)COPYRIGHT *****END OF FILE****************************/

