/*******************************************************************************
 *          Copyright (c) 2020-2050, wanzhuangwulian Co., Ltd.
 *                              All Right Reserved.
 * @file
 * @note
 * @brief
 *
 * @author
 * @date
 * @version
 *
 * @Description ͨ�ö���ͷ�ļ�
 *
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning
 *******************************************************************************/
#ifndef   __COMMON_H__
#define   __COMMON_H__

#include <stdint.h>
#include "os.h"

#define  MIN(a, b)      (((a) < (b)) ? (a) : (b))
typedef enum
{
    TIME_PERIOD_1 = 0,
    TIME_PERIOD_2,
    TIME_PERIOD_3,
    TIME_PERIOD_4,
    TIME_PERIOD_5,
    TIME_PERIOD_6,
    TIME_PERIOD_7,
    TIME_PERIOD_8,
    TIME_PERIOD_9,
    TIME_PERIOD_10,

    TIME_PERIOD_11,
    TIME_PERIOD_12,
    TIME_PERIOD_13,
    TIME_PERIOD_14,
    TIME_PERIOD_15,
    TIME_PERIOD_16,
    TIME_PERIOD_17,
    TIME_PERIOD_18,
    TIME_PERIOD_19,
    TIME_PERIOD_20,

    TIME_PERIOD_21,
    TIME_PERIOD_22,
    TIME_PERIOD_23,
    TIME_PERIOD_24,
    TIME_PERIOD_25,
    TIME_PERIOD_26,
    TIME_PERIOD_27,
    TIME_PERIOD_28,
    TIME_PERIOD_29,
    TIME_PERIOD_30,

    TIME_PERIOD_31,
    TIME_PERIOD_32,
    TIME_PERIOD_33,
    TIME_PERIOD_34,
    TIME_PERIOD_35,
    TIME_PERIOD_36,
    TIME_PERIOD_37,
    TIME_PERIOD_38,
    TIME_PERIOD_39,
    TIME_PERIOD_40,

    TIME_PERIOD_41,
    TIME_PERIOD_42,
    TIME_PERIOD_43,
    TIME_PERIOD_44,
    TIME_PERIOD_45,
    TIME_PERIOD_46,
    TIME_PERIOD_47,
    TIME_PERIOD_48,
    TIME_PERIOD_MAX,
} _TIME_PERIOD;


typedef enum
{
    TIME_QUANTUM_J = 0,	//��
    TIME_QUANTUM_F,		//��
    TIME_QUANTUM_P,		//ƽ
    TIME_QUANTUM_G,		//��
    TIME_QPERIOD_5,
    TIME_QPERIOD_6,
    TIME_QPERIOD_7,
    TIME_QPERIOD_8,
    TIME_QPERIOD_9,
    TIME_QPERIOD_10,
    TIME_QPERIOD_11,
    TIME_QPERIOD_12,
    TIME_QUANTUM_MAX,
} _TIME_QUANTUM;

typedef enum
{
    GUN_A = 0,
    GUN_B,
    GUN_UNDEFIN = 0X55,
} _GUN_NUM;

#define   COMMON_INVALID_FLAG   0
#define   COMMON_VALID_FLAG     1

typedef enum
{
    FLASH_ORDER_READ = 0x01, //0x01:������
    FLASH_ORDER_WRITE,       //0x02:д����
} _FLASH_ORDER;


/* ����ϵͳ tick �� 1ms */
#define CM_TIME_5_MSEC         5
#define CM_TIME_10_MSEC         10
#define CM_TIME_20_MSEC         20
#define CM_TIME_30_MSEC         30
#define CM_TIME_50_MSEC         50
#define CM_TIME_100_MSEC        100ul
#define CM_TIME_250_MSEC        250ul
#define CM_TIME_500_MSEC        500ul
#define CM_TIME_1_SEC           1000ul
#define CM_TIME_2_SEC           2000ul
#define CM_TIME_3_SEC           3000ul
#define CM_TIME_5_SEC           (5  * CM_TIME_1_SEC)
#define CM_TIME_10_SEC          (10 * CM_TIME_1_SEC)
#define CM_TIME_15_SEC          (15 * CM_TIME_1_SEC)
#define CM_TIME_20_SEC          (20 * CM_TIME_1_SEC)
#define CM_TIME_30_SEC          (30 * CM_TIME_1_SEC)
#define CM_TIME_60_SEC          (60 * CM_TIME_1_SEC)
#define CM_TIME_90_SEC          (90 * CM_TIME_1_SEC)
#define CM_TIME_5_MIN           (5 * CM_TIME_60_SEC)
#define CM_TIME_3_MIN           (3 * CM_TIME_60_SEC)
/* �������ģ��ID */
#define  CM_CHTASK_MODULE_ID     0x01

/* ��׮GPRSģ��ID */
#define  CM_WZGPRS_MODULE_ID     0x02

/* TCU����ģ��ID */
#define  CM_TCUTASK_MODULE_ID    0x03

/* CARD����ģ��ID */
#define  CM_CARD_MODULE_ID       0x04

/* DISP����ģ��ID */
#define  CM_DISP_MODULE_ID       0x05

/* ������Ļ��������ģ��ID */
#define  CM_DWINRECV_MODULE_ID       0x06

/* 4G��������ģ��ID */
#define  CM_4GRECV_MODULE_ID       0x07

/*4G��������ģ��ID*/
#define  CM_4GSEND_MODULE_ID       0x08

/*4G������ģ��ID*/
#define  CM_4GMAIN_MODULE_ID       0x09


/*4G������ģ��ID*/
#define  CM_4GUARTRECV_MODULE_ID    0x0A

/*��������ģ��ID*/		
#define CM_BEL_MODULE_ID			0x0B	

/*485*/
#define  CM_485USRTRECV_MODULE_ID    0x0C
#define  CM_UNDEFINE_ID       		0xaa  //�������Դ
/* ��ȡ��������֮��Ľ�Сֵ */
#define  CM_DATA_GET_MIN(a,b)  (((a) <= (b)) ? (a) : (b))

/* ��ȡ��������֮��Ľϴ�ֵ */
#define  CM_DATA_GET_MAX(a,b)  (((a) >= (b)) ? (a) : (b))

typedef struct
{
    /* ��ϢԴͷ�Ĺ���ģ��ID */
    uint32_t     uiSrcMoudleId;
    /* ��ϢĿ��Ĺ���ģ��ID */
    uint32_t     uiDestMoudleId;
    /* ��Ϣ�� */
    uint32_t     uiMsgCode;
    /* ��Ϣ������� */
    uint32_t     uiMsgVar;
    /* ��Ϣ��Ч�غɵĳ��� */
    uint32_t     uiLoadLen;
    /* ָ����Ч��Ϣ�Ļ����� */
    uint8_t      *pucLoad;
} MQ_MSG_T;

enum
{
    WZGPRS_TO_TCU_NETSTATE,         /* ����״̬ */
    WZGPRS_TO_TCU_IMEI,             /* imei */
    WZGPRS_TO_TCU_IMSI,             /* imsi */
    WZGPRS_TO_TCU_CSQ,              /* CSQ */
    WZGPRS_TO_TCU_DATA,             /* ��׮GPRSģ�鷢�����ݸ�TCUģ�� */
    TCU_TO_WZGPRS_DATA,             /* TCU��Ҫ�������ݸ�ģ�� */
    TCU_TO_WZGPRS_FILEDOWN,	        /* �ļ����ظ�ʽ */
};


enum
{
    CH_TO_TCU_REALTIMEDATA,        /* ʵʱ���� */
    CH_TO_TCU_CHCTL_ACK,           /* ������Ӧ�� */
    CH_TO_TCU_RECORD,              /* ����¼ */
    TCU_TO_CH_CHCLT,               /* ������ */
    CARD_TO_TCU_INFO,              /* ����Ϣ */
} ;

enum
{
    CH_TO_DIP_STARTSUCCESS,     		/* �����ɹ� */
    CH_TO_DIP_STARTFAIL,              	/* ����ʧ�� */
    CH_TO_DIP_STOP,              		/* ֹͣ��� */
} ;

enum
{
    WZGPRS_TO_HTTP_START_ERASE ,    /* �������� */
    WZGPRS_TO_HTTP_DOWN_FILE       /* �����ļ� */
} ;

//4g����
#define  	APP_START_ACK		0	//��ʼ���Ӧ��
#define  	APP_ATIV_STOP		1	//����ֹͣ���
#define  	APP_STOP_ACK		3	//ֹͣ���Ӧ��
#define  	APP_STOP_BILL		17	//ֹͣ����
#define 	APP_RATE_ACK		5   //��������Ӧ��
#define 	APP_STE_DEVS		6	//��ѯ�豸״̬
#define 	APP_STE_BILL		7	//��ѯ��¼
#define 	APP_STE_RATE		8	//��ѯ����
#define 	APP_STE_TIME		9	//У׼ʱ��
#define     APP_CARDVIN_CHARGE	10	//ˢ��vin����
#define     APP_CARD_INFO		11	//����Ȩ
#define     APP_VIN_INFO		12	//Vin��Ȩ
#define		APP_CARD_WL			13	//��������
#define		APP_VIN_WL			14	//VIN������
#define		APP_VINCARD_RES		15	//���������
#define		APP_OFFLINE_ACK		16	//����Ӧ��
#define     APP_SJDATA_QUERY	18  //��ѯʵʱ����
#define		APP_QUERY_RATE		19	//��ѯ����
#define		APP_RATE_MODE		20	//�Ʒ�ģ��
#define    APP_UPDADA_BALANCE   21  //�������
#define    APP_SETQR_ACK   		22  //���ö�ά��Ӧ��
#define    APP_OTA_ACK   		23  //���ö�ά��Ӧ��









typedef enum
{
    INPUT_YX1_DOOR = 0,
    INPUT_YX2,
    INPUT_YX3_EM,			//��ͣ
    INPUT_YX_MAX,
} _IO_INPINT_STATE;
uint8_t mq_service_bind(uint32_t uiModuleId,const char *sMqName);
uint8_t mq_service_recv_msg(OS_Q * pq, MQ_MSG_T *pstMsg, uint8_t *pucMsgBuf, uint32_t uiBufSize, uint32_t rtTimeout);
uint8_t mq_service_xxx_send_msg_to_chtask(uint32_t uiSrcMoudleId, uint32_t uiMsgCode,uint32_t uiMsgVar, uint32_t uiLoadLen, uint8_t *pucLoad); 										  ;
uint8_t mq_service_card_send_disp(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
uint8_t uart_service_dwinrecv_send_disp(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
uint8_t uart_service_4GUart_send_recv(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
uint8_t mq_service_ch_send_dip(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
uint8_t mq_service_send_to_4gsend(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
#if(USE_BLE==2)
uint8_t mq_service_485Uart_send_recv(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
#elif(USE_BLE==1)
uint8_t uart_service_send_to_blesend(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
uint8_t mq_service_ch_send_ble(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad);
#endif


#endif  /*����*/