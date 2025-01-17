/*****************************************Copyright(C)******************************************
****************************************************************************************
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
#include <string.h>
#include "ModuleA7680C.h"
#include "dwin_com_pro.h"
#include "Frame66.h"
#include "4GRecv.h"
#include "main.h"
#include "chtask.h"
/* Private define-----------------------------------------------------------------------------*/

/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/

/*static OS_EVENT *GPRSMainTaskEvent;				            // 使用的事件
OS_EVENT *SendMutex;                 //互斥锁，同一时刻只能有一个任务进行临界点访问*/
_START_NET_STATE StartNetState = NET_STATE_ONLINE;		//启动的时候状态
//====交流测试:39.170.80.51:18992
//====交流正式：139.196.60.58:18051
_NET_CONFIG_INFO  NetConfigInfo = {{139,196,60,58},18051, {"pile-v2.66ifuel.com"}  ,	 	2};   //正式域名 pile-v2.66ifuel.com    mqtt-test.66ifuel.com
//_NET_CONFIG_INFO  NetConfigInfo = {{39,170,80,51},18992, {"pile-test.66ifuel.com"}  ,	 	2};   //测试

extern CH_TASK_T stChTcb;
_RESEND_BILL_CONTROL ResendBillControl = {0};
OS_MUTEX  sendmutex;
/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/
_HTTP_INFO HttpInfo = {0};
/*****************************************************************************
* Function     : APP_SetUpadaState
* Description  :设置升级是否成功   0表示失败   1表示成功
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
void APP_SetUpadaState(uint8_t state)
{
    __nop();
}

/*****************************************************************************
* Function     : APP_SetResendBillState
* Description  : 设置是否重发状态
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
void APP_SetResendBillState(uint8_t state)
{
    ResendBillControl.ResendBillState = state;
    ResendBillControl.SendCount = 0;
}

uint8_t   APP_GetStartNetState(void)
{
    return (uint8_t)StartNetState;
}
//桩上传结算指令
/*****************************************************************************
* Function     : ReSendBill
* Description  : 重发订单
* Input        : void *pdata  ifquery: 1 查询  0：重复发送
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t  ReSendBill(uint8_t* pdata,uint8_t ifquery)
{

    if(pdata == NULL)
    {
        return FALSE;
    }
    ResendBillControl.CurTime = OSTimeGet(&timeerr);		//获取当前时间
    if(ResendBillControl.ResendBillState == FALSE)
    {
        return FALSE;			//不需要重发订单
    }
    if((ResendBillControl.CurTime - ResendBillControl.LastTime) >= CM_TIME_10_SEC)
    {
        if(++ResendBillControl.SendCount > 3)
        {
            ResendBillControl.ResendBillState = FALSE;		//发送三次没回复就不发了
            ResendBillControl.SendCount = 0;
            return FALSE;
        }
        ResendBillControl.LastTime = ResendBillControl.CurTime;
        return _66_SendBillData(pdata,200);
    }


    return TRUE;
}

/*****************************************************************************
* Function     : APP_GetResendBillState
* Description  : 获取是否重发状态
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t APP_GetResendBillState(void)
{
    return ResendBillControl.ResendBillState;
}

/*****************************************************************************
* Function     : ReSendOffLineBill
* Description  :
* Input        : 发送离线交易记录订单
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t  ReSendOffLineBill(void)
{
    static uint8_t count = 0;			//联网状态下，连续三次发送无反应则丢失

    static uint8_t num = 0;
    //离线交易记录超时不管A枪和B枪都一样，目前都只用A枪
    ResendBillControl.OFFLineCurTime = OSTimeGet(&timeerr);		//获取当前时间

    //获取是否有离线交易记录
    ResendBillControl.OffLineNum = APP_GetNetOFFLineRecodeNum();		//获取离线交易记录
    if(ResendBillControl.OffLineNum > 0)
    {
        if((ResendBillControl.OFFLineCurTime - ResendBillControl.OFFLineLastTime) >= CM_TIME_30_SEC)
        {
            if(num == ResendBillControl.OffLineNum)
            {
                //第一次不会进来
                if(++count >= 3)
                {
                    //联网状态下连续三次未返回，则不需要发送
                    count = 0;
                    ResendBillControl.OffLineNum--;
                    APP_SetNetOFFLineRecodeNum(ResendBillControl.OffLineNum);
                }
            }
            else
            {
                count = 0;
                num = ResendBillControl.OffLineNum;
            }
            ResendBillControl.OFFLineLastTime = ResendBillControl.OFFLineCurTime;
        }
    }
    return TRUE;
}

/*****************************************************************************
* Function     : APP_SetStartNetState
* Description  : 设置启动方式网络状态
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   APP_SetStartNetState(_START_NET_STATE  type)
{
    if(type >=  NET_STATE_MAX)
    {
        return FALSE;
    }

    StartNetState = type;
    return TRUE;
}

/*****************************************************************************
* Function     : APP_GetGPRSMainEvent
* Description  :获取网络状态
* Input        : 那一路
* Output       : TRUE:表示有网络	FALSE:表示无网络
* Return       :
* Note(s)      :
* Contributor  : 2018-6-14
*****************************************************************************/
uint8_t  APP_GetNetState(uint8_t num)
{
    if(STATE_OK == APP_GetAppRegisterState(num))
    {
        return TRUE;
    }
    return FALSE;
}

/*****************************************************************************
* Function     : 4G_RecvFrameDispose
* Description  :4G接收
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t _4G_RecvFrameDispose(uint8_t * pdata,uint16_t len)
{
    return _66_RecvFrameDispose(pdata,len);
}


/*****************************************************************************
* Function     : APP_GetBatchNum
* Description  : 获取交易流水号
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
uint8_t *  APP_GetBatchNum(void)
{
    return _66_GetBatchNum();
}

/*****************************************************************************
* Function     : APP_GetNetMoney
* Description  :获取账户余额
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
******************************************************************************/
uint32_t APP_GetNetMoney(uint8_t gun)
{
    return 0;
}
/*****************************************************************************
* Function     : HY_SendFrameDispose
* Description  :4G发送
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t  _4G_SendFrameDispose()
{

    JM_SendFrameDispose();

    return TRUE;
}

/*****************************************************************************
* Function     : Pre4GBill
* Description  : 保存订单
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   Pre4GBill(uint8_t *pdata)
{

    _66_PreBill(pdata);

    return TRUE;
}




/*****************************************************************************
* Function     : _4G_SendRateAck
* Description  : 费率应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   _4G_SendRateAck(uint8_t cmd)
{
    return TRUE;
}

/*****************************************************************************
* Function     : HY_SendQueryRateAck
* Description  : 查询费率应答
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t    _4G_SendQueryRate(void)
{
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendRateMode
* Description  : 发送计费模型
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t    _4G_SendRateMode(void)
{
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendSetTimeAck
* Description  : 校时应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2021年1月12日
*****************************************************************************/
uint8_t   _4G_SendSetTimeAck(void)
{
    _66_SetTimeAck();
    return TRUE;
}

/*****************************************************************************
* Function     : HY_SendBill
* Description  : 发送订单
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendBill(void)
{


    _66_SendBill();

    return TRUE;
}

/*****************************************************************************
* Function     : _4G_GetStartType
* Description  : 获取快充启动方式
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
_4G_START_TYPE   _4G_GetStartType(void)
{

    return (_4G_START_TYPE)APP_Get66StartType();
}

/*****************************************************************************
* Function     : _4G_SetStartType
* Description  : 设置安培快充启动方式
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   _4G_SetStartType(_4G_START_TYPE  type)
{
    if(type >=  _4G_APP_MAX)
    {
        return FALSE;
    }

    APP_Set66StartType(type);
    return TRUE;
}

/*****************************************************************************
* Function     : _4G_SendSetQR
* Description  : 设置二维码发送
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t _4G_SendSetQR(void)
{

    _66_SetQRAck();

    return TRUE;
}



/***************************************************************
**Function   :_4G_SendSetQR
**Description:
**Input      :None
**Output     :
**Return     :
**note(s)    :
**Author     :CSH
**Create_Time:2023-7-19
***************************************************************/
uint8_t _4G_SendOTA(void)
{
    _66_SetOTAAck();
    return TRUE;
}




/*****************************************************************************
* Function     : _4G_SendStOPtAck
* Description  : 停止应答
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2021年3月19日
*****************************************************************************/
uint8_t   _4G_SendStopAck(void)
{

    _66_SendStopAck();

    return TRUE;
}



/*****************************************************************************
* Function     : HFQG_SendStartAck
* Description  : 开始充电应答
* Input        : void *pdata
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月27日
*****************************************************************************/
uint8_t   _4G_SendStartAck(void)
{

    _66_SendStartAck();

    return TRUE;
}

/*****************************************************************************
* Function     : UART_4GWrite
* Description  :串口写入，因多个任务用到了串口写入，因此需要加互斥锁
* Input        :
* Output       :
* Return       :
* Note(s)      :
* Contributor  : 2020-11-26     叶喜雨
*****************************************************************************/
uint8_t UART_4GWrite(uint8_t* const FrameBuf, const uint16_t FrameLen)
{
    OS_ERR ERR;

    OSMutexPend(&sendmutex,0,OS_OPT_PEND_BLOCKING,NULL,&ERR); //获取锁
    UART1SENDBUF(FrameBuf,FrameLen);
    if(FrameLen)
    {
        OSTimeDly((FrameLen/10 + 10)*1, OS_OPT_TIME_PERIODIC, &ERR);	//等待数据发送完成  115200波特率， 1ms大概能发10个字节（大于10个字节）
        //OSTimeDly(SYS_DELAY_5ms);
        OSTimeDly(CM_TIME_50_MSEC, OS_OPT_TIME_PERIODIC, &ERR);
    }
    OSMutexPost(&sendmutex,OS_OPT_POST_NONE,&ERR); //释放锁
    return FrameLen;
}

/*****************************************************************************
* Function     : Connect_4G
* Description  : 4G网络连接
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年7月11日
*****************************************************************************/
static uint8_t Connect_4G(void)
{
#define RESET_4G_COUNT	7    //目前连续3次未连接上服务器，则重新启动
    static uint8_t i = 8; //第一次上来先复位
    uint8_t count;

    if(APP_GetModuleConnectState(0) !=STATE_OK) //判断主平台
    {
        if(++i > RESET_4G_COUNT)
        {
            i = 0;
            SIM7600Reset();
        }
    }
    if(APP_GetSIM7600Status() != STATE_OK)  //模块存在
    {
        Module_SIM7600Test();    //测试模块是否存在
    }
    if(APP_GetSIM7600Status() != STATE_OK)  //模块不存在
    {
        return FALSE;
    }
    //到此说明模块已经存在了模块存在了
    //连接服务器,可能又多个服务器
    for(count = 0; count < NetConfigInfo.NetNum; count++)
    {
        if(APP_GetModuleConnectState(count) != STATE_OK) //位连接服务器
        {
            if(count == 0)
            {
                ModuleSIM7600_ConnectServer(count,(uint8_t*)NetConfigInfo.pIp,NetConfigInfo.port);
            }
            else
            {
                ModuleSIM7600_ConnectServer(count,(uint8_t*)GPRS_IP2,GPRS_PORT2);		//连接服务器
            }
        }
        if(APP_GetModuleConnectState(count) != STATE_OK)  //模块未连接
        {
            SIM7600CloseNet(count);			//关闭网络操作
        }
    }

    return TRUE;
}


#define MSG_NUM    5
/*****************************************************************************
* Function     : AppTask4GMain
* Description  : 4G主任务
* Input        : void
* Output       : None
* Return       :
* Note(s)      :
* Contributor  : 2018年6月14日
*****************************************************************************/
void AppTask4GMain(void *p_arg)
{
    OS_ERR ERR;
    static uint8_t download_count = 0;
    static uint32_t nowSysTime = 0,lastSysTime = 0;
    OSTimeDly(5000, OS_OPT_TIME_PERIODIC, &timeerr);
    if(DisSysConfigInfo.standaloneornet != DISP_NET)
    {
        return;
    }

    OSMutexCreate(&sendmutex,"sendmutex",&ERR);
    if(ERR != OS_ERR_NONE)
    {
        return;
    }

    //打开电源
    _4G_POWER_ON;
    OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);
    _4G_PWKEY_OFF;
    _4G_RET_OFF;
    OSTimeDly(250, OS_OPT_TIME_PERIODIC, &timeerr);
    //SIM7600Reset();

    nowSysTime = OSTimeGet(&timeerr);
    lastSysTime = nowSysTime;
    while(1)
    {
        if(APP_GetSIM7600Mode() == MODE_DATA)
        {
            Connect_4G();          //4G连接，包括模块是否存在，和连接服务
            //10分钟没连上服务器，而且没在充电中，就重启设备
            nowSysTime = OSTimeGet(&timeerr);
            if((APP_GetAppRegisterState(LINK_NUM) != STATE_OK)  && (stChTcb.ucState != CHARGING) )	//显示已经注册成功了
            {
                if((nowSysTime >= lastSysTime) ? ((nowSysTime - lastSysTime) >= (CM_TIME_5_MIN*2)) : \
                        ((nowSysTime + (0xffffffff - lastSysTime)) >=  (CM_TIME_5_MIN*2)) )
                {
                    SystemReset();			//软件复位
                }
            }
            else
            {
                lastSysTime = nowSysTime;
            }
        }
        else
        {
            if(APP_GetModuleConnectState(0) == STATE_OK)
            {
                //memcpy(HttpInfo.ServerAdd,"https://dfs-test.66ifuel.com/download/d9d8aa313e1e4efbbb1d73799a903bd0/JM.bin",strlen("https://dfs-test.66ifuel.com/download/d9d8aa313e1e4efbbb1d73799a903bd0/JM.bin"));
                //memcpy(HttpInfo.ServerAdd,"http://hy.shuokeren.com/uploads/66999.bin",strlen("http://hy.shuokeren.com/uploads/66999.bin"));
                if(Module_HTTPDownload(&HttpInfo))
                {
                    //升级成功
                    download_count = 0;
                    Send_AT_CIPMODE();
                    OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
                    //发送升级成功
                    OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &timeerr);
                    JumpToProgramCode();  //跳转到boot程序
                }
                else
                {
                    if(++download_count > 5)   //连续5次升级不成功，则返回升级失败
                    {
                        download_count = 0;
                        Send_AT_CIPMODE();
                        OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);;
                        SystemReset();			//软件复位
                    }
                }
            }
        }
        OSTimeDly(1000, OS_OPT_TIME_PERIODIC, &timeerr);
    }
}



/************************(C)COPYRIGHT *****END OF FILE****************************/

