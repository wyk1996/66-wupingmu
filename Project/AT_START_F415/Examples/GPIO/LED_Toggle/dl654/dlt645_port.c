/*************************************************
 Copyright (c) 2019
 All rights reserved.
 File name:     dlt645_port.c
 Description:   DLT645 移植&使用例程文件
 History:
*************************************************/
#include "dlt645_port.h"
#include "rc522.h"
//#include "485_1.h"
#include "uart3.h"
#include "common.h"
#include "dwin_com_pro.h"
#include "main.h"

/* 电表信息 */
#warning "YXY"
_dlt645_info dlt645_info;
OS_Q Recv485USRTMq;
extern _DIS_SYS_CONFIG_INFO DisSysConfigInfo;	//显示界面系统配置信息

#if(USE_BLE==2)	
uint8_t USART3_485send(void *buf,uint8_t len)
{
    UART3SENDBUF(buf,len);
    return 1;
}

void AppTaskdlt645(void *p_arg)
{
    MQ_MSG_T stMsg = {0};
    OS_ERR err;
    OSQCreate (&Recv485USRTMq,
               "dwin task mq",
               21,
               &err);
    if(err != OS_ERR_NONE)
    {
        printf("OSQCreate %s Fail", "dwin task mq");
        return;
    }
    static uint8_t RECVUSART3buf[50]; //接收数据
    UART3BTinit(); //串口3初始化
    OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);

    static uint32_t nowSysTime = 0,lastSysTime = 0;
    static uint8_t sengbuf[16]= {0x68,0xAA,0xAA,0xAA,0xAA,0xAA,0xAA,0x68,0x11,0x04};
    static uint8_t i = 0;

    while(1)
    {
        if(DisSysConfigInfo.energymeter == 0)  //外部电表
        {
            nowSysTime = OSTimeGet(&timeerr);
            if((nowSysTime >= lastSysTime) ? ((nowSysTime - lastSysTime) >= (CM_TIME_1_SEC)):\
                    ((nowSysTime + (0xffffffff - lastSysTime)) >= (CM_TIME_1_SEC)) )
            {
                if(i == 0)
                {
                    uint8_t buf1[6]= {0x33,0x33,0x93,0x33,0x0D,0x16};    /**读取高精度电量**/
                    memcpy(&sengbuf[10],&buf1[0],sizeof(buf1));
                    USART3_485send(sengbuf,sizeof(sengbuf));
                }
                if(i == 1)
                {
                    uint8_t buf2[6]= {0x33,0x34,0x34,0x35,0xB1,0x16};    /**A相电压**/
                    memcpy(&sengbuf[10],&buf2[0],sizeof(buf2));
                    USART3_485send(sengbuf,sizeof(sengbuf));
                }
                if(i == 2)
                {
                    uint8_t buf3[6]= {0x33,0x34,0x35,0x35,0xB2,0x16};    /**A相读电流**/
                    memcpy(&sengbuf[10],&buf3[0],sizeof(buf3));
                    USART3_485send(sengbuf,sizeof(sengbuf));
                }

                i++;
                if(i >=3)
                {
                    i = 0;
                }
                lastSysTime=nowSysTime;
            }

            //组合有功总电能:普通的645协议数据位DIC_0,   瑞银和英利达 专用正向总电能 DIC_00600000
            //瑞银电表无4位FEH前导码     英利达单相有4位FEH前导码

            if(mq_service_recv_msg(&Recv485USRTMq,&stMsg,RECVUSART3buf,sizeof(RECVUSART3buf),CM_TIME_500_MSEC) == 0 )
            {
                static uint8_t DATAbuf[30];
                static uint32_t DICbuf;
                static uint32_t dltinfo[5];    //得到的字节
                static uint64_t dlt645info;

                memcpy(DATAbuf,RECVUSART3buf+4,sizeof(RECVUSART3buf)-4);  //英利达(9600-8-1-even偶) 有4位前导码
               // memcpy(DATAbuf,RECVUSART3buf,sizeof(RECVUSART3buf));      //瑞银（38400-8-1-even偶）   无4位前导码

                if(stMsg.uiDestMoudleId == CM_485USRTRECV_MODULE_ID)
                {
                    DICbuf = ((0X000000FF & (DATAbuf[10]-0x33))| ((0X000000FF & (DATAbuf[11]-0x33))<< 8)| \
                              ((0X000000FF & (DATAbuf[12]-0x33)) << 16)|((0X000000FF & (DATAbuf[13]-0x33)) << 24));
                    switch(DICbuf)
                    {
                    case DIC_02010100:   //A相电压
                        dltinfo[4] = (((DATAbuf[14] - 0x33)>>4) *10) +((DATAbuf[14] - 0x33) & 0x0F) ;
                        dltinfo[3] = ((((DATAbuf[15] - 0x33)>>4) *10) +((DATAbuf[15] - 0x33) & 0x0F)) *100 ;
                        dlt645info = (dltinfo[3]+dltinfo[4]);
                        dlt645_info.out_vol = (float)dlt645info / 10;  //2位小数
                        break;

                    case DIC_02020100:   //A相电流
                        dltinfo[4] = (((DATAbuf[14] - 0x33)>>4) *10) +((DATAbuf[14] - 0x33) & 0x0F) ;
                        dltinfo[3] = ((((DATAbuf[15] - 0x33)>>4) *10) +((DATAbuf[15] - 0x33) & 0x0F)) *100 ;
                        dltinfo[2] = ((((DATAbuf[16] - 0x33)>>4) *10) +((DATAbuf[16] - 0x33) & 0x0F)) *10000;
                        dlt645info = (dltinfo[2]+dltinfo[3]+dltinfo[4]);
                        dlt645_info.out_cur = (float)dlt645info / 1000;  //A相电流
                        break;

                    case 0x00600000:   //高精度电量
                        dltinfo[4] = (((DATAbuf[14] - 0x33)>>4) *10) +((DATAbuf[14] - 0x33) & 0x0F) ;
                        dltinfo[3] = ((((DATAbuf[15] - 0x33)>>4) *10) +((DATAbuf[15] - 0x33) & 0x0F)) *100 ;
                        dltinfo[2] = ((((DATAbuf[16] - 0x33)>>4) *10) +((DATAbuf[16] - 0x33) & 0x0F)) *10000;
                        dltinfo[1] = ((((DATAbuf[17] - 0x33)>>4) *10) +((DATAbuf[17] - 0x33) & 0x0F)) *1000000;
                        dltinfo[0] = ((((DATAbuf[18] - 0x33)>>4) *10) +((DATAbuf[18] - 0x33) & 0x0F)) *100000000;
                        dlt645info = (dltinfo[0]+dltinfo[1]+dltinfo[2]+dltinfo[3]+dltinfo[4]);
                        dlt645_info.cur_hwh = (float)dlt645info / 10000;  //4位小数电量
                        break;

                    default:
                        break;
                    }
                }
            }
        }
        else
        {
            OSTimeDly(100, OS_OPT_TIME_PERIODIC, &err);  //内部电表时，不发数据
        }
    }
}

#endif