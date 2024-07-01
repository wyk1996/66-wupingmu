/*******************************************************************************
 * @file
 * @note
 * @brief
 *
 * @author
 * @date     2021-10-23
 * @version  V1.0.0
 *
 * @Description  ��Ϣ���з���
 *
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning
 *******************************************************************************/

#include <stdint.h>
#include <string.h>
#include "common.h"
#include "os.h"
#include "main.h"
/*
===============================================================================================================================
                                            ****** ���Ͷ���/�궨�� ******
===============================================================================================================================
*/

#define        MQ_SERVICE_MSG_MAX_SIZE     sizeof(MQ_MSG_T)
#define        MQ_SERVICE_MSG_MAX_NUM      8

/**
 * @brief ����ģ��id�Ƿ��Ѿ�����
 * @param[in] uiModuleId:����ģ��ID
 * @param[out]
 * @return RT_NULL:����ģ��IDû�а� �ǿ�:����ģ��ID��Ӧ�Ľڵ�ָ��
 * @note
 */
OS_Q * mq_service_moduleid_exist(uint32_t uiModuleId)
{
	if(CM_DISP_MODULE_ID == uiModuleId)
	{
		return &DwinMq;
	}
	else if(CM_CHTASK_MODULE_ID == uiModuleId)
	{
		return &ChTaskMq;
	}
	else if(CM_4GSEND_MODULE_ID == uiModuleId)
	{
		return &Send4GMq;
	}
	else if(CM_4GRECV_MODULE_ID == uiModuleId)
	{
		return &Recv4GMq;
	}
	#if(USE_BLE==1)					
	else if(CM_BEL_MODULE_ID == uiModuleId)
	{
		return &RecvBELMq;
	}
	#elif(USE_BLE==2)
	else if(CM_485USRTRECV_MODULE_ID == uiModuleId)
    {
        return &Recv485USRTMq;
    }
	#endif
    return NULL;
}


#define MSQLEN     5
uint8_t pucDipBuf[MSQLEN][MSG_RECV_LEN] = {0};  //���ͳ�4G�������Ϣ
MQ_MSG_T         stDipMsg[MSQLEN]    = {0};

uint8_t pucBLEBuf[MSQLEN][MSG_RECV_LEN] = {0};  //���ͳ�4G�������Ϣ
MQ_MSG_T         stBLEMsg[MSQLEN]    = {0};

uint8_t puc4GBuf[MSQLEN][URART_4GRECV_LEN] = {0};  //����4G����Ϣ
MQ_MSG_T         st4GMsg[MSQLEN]    = {0};


#define MST_T_LEN     10
#define BUF_T_LEN 		50
uint8_t pucBuf[MST_T_LEN][BUF_T_LEN] = {0};  //����4G����Ϣ
MQ_MSG_T         stMsg[MST_T_LEN]    = {0};
/**
 * @brief ������Ϣ��ָ���Ĺ���ģ��
 * @param[in]  uiSrcMoudleId  ����ϢԴͷ�Ĺ���ģ��
 *             uiDestMoudleId ����Ҫ���͵��Ĺ���ģ��
 *             uiMsgCode      ����Ϣ��
 *             uiMsgVar       ����Ϣ����
 *             uiLoadLen      ����Ϣ��Ч�غɳ���
 *             pucLoad        ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0: ���ͳɹ�  ��0: ����ʧ��
 * @note �ڲ�����
 */
uint8_t mq_service_send_msg(uint32_t uiSrcMoudleId, uint32_t  uiDestMoudleId, uint32_t  uiMsgCode,
                            uint32_t uiMsgVar, uint32_t  uiLoadLen, uint8_t  *pucLoad)
{
	
    OS_Q  *pstNode = NULL;
	OS_ERR   err;
	static uint8_t count = 0;

	OSSchedLock(&ERRSTATE);
    pstNode = mq_service_moduleid_exist(uiDestMoudleId);

    if(pstNode == NULL)
    {
		OSSchedUnlock(&ERRSTATE);
        return 1;
    }
	if(uiLoadLen > BUF_T_LEN)
	{
		OSSchedUnlock(&ERRSTATE);
		return 1;
	}
	 if((pucLoad != NULL) && (uiLoadLen > 0))
	{
		memcpy(&pucBuf[count][0],pucLoad,uiLoadLen);
	}

	stMsg[count].uiSrcMoudleId    = uiSrcMoudleId;
	stMsg[count].uiDestMoudleId   = uiDestMoudleId;
	stMsg[count].uiMsgCode        = uiMsgCode;
	stMsg[count].uiMsgVar         = uiMsgVar;
	stMsg[count].uiLoadLen        = uiLoadLen;
	stMsg[count].pucLoad          = &pucBuf[count][0];

	OSQPost (pstNode,&stMsg[count],sizeof(MQ_MSG_T),OS_OPT_POST_FIFO,&err);	  
	count++;
	if(count >= MST_T_LEN)   //һ����3�����淢��
	{
		count = 0;
	}
	
	if(err == OS_ERR_NONE)
	{
		OSSchedUnlock(&ERRSTATE);
		return 0;
	}
	OSSchedUnlock(&ERRSTATE);
	return 1;
}
/**
 * @brief   ָ���Ĺ���ģ�������Ϣ
 * @param[in]  uiSrcMoudleId ����Ҫ������ϢԴͷ�Ĺ���ģ��
 *             uiBufSize     ������buf�ĳߴ�
 *             rtTimeout     ���ȴ���ʱ��ʱ��
 * @param[out] pstMsg �� ָ��洢���յ�����Ϣ������
*              pucMsgBuf��ָ��洢���յ���Ϣ����Ч�غ�
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t mq_service_recv_msg(OS_Q * pq,MQ_MSG_T *pstMsg,uint8_t *pucMsgBuf,uint32_t uiBufSize,uint32_t rtTimeout)
{
	OS_ERR err;
	uint16_t len = sizeof(MQ_MSG_T);
	MQ_MSG_T* pbuf = NULL;
	uint16_t uiRxValidLen = 0;


	pbuf = OSQPend (pq,rtTimeout,OS_OPT_PEND_BLOCKING,&len,NULL,&err);
	if(err != OS_ERR_NONE)
	{
		return 1;
	}
	if(pbuf == NULL)
	{
		return 1;
	}	
	memcpy(pstMsg,pbuf,sizeof(MQ_MSG_T));
	uiRxValidLen  = CM_DATA_GET_MIN(uiBufSize,pstMsg->uiLoadLen);
	memcpy(pucMsgBuf,pstMsg->pucLoad,uiRxValidLen);
	pstMsg->pucLoad    = pucMsgBuf;
	pstMsg->uiLoadLen  = uiRxValidLen;
	return 0;
}


/**
 * @brief   ������Ϣ���������ģ��
 * @param[in]  uiSrcMoudleId ����ϢԴͷ�Ĺ���ģ��
 *             uiMsgCode     ����Ϣ��
 *             uiMsgVar      ����Ϣ����
 *             uiLoadLen     ����Ϣ��Ч�غɳ���
 *             pucLoad       ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
* @return 0:���ͳɹ�  ��0:����ʧ��
 * @note
 */
uint8_t mq_service_xxx_send_msg_to_chtask(uint32_t uiSrcMoudleId, uint32_t  uiMsgCode,
        uint32_t uiMsgVar, uint32_t uiLoadLen, uint8_t  *pucLoad)
{
    return mq_service_send_msg(uiSrcMoudleId,CM_CHTASK_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad);
}



/**
 * @brief   ˢ����������Ϣ����ʾ����
 * @param[in]  uiMsgCode ����Ϣ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t mq_service_card_send_disp(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
    return mq_service_send_msg(CM_CARD_MODULE_ID,CM_DISP_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad) ;
}



/**
* @brief   ���ģ�鷢�͸���ʾ����
 * @param[in]  uiMsgCode ��ֹͣԭ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t mq_service_ch_send_dip(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
    return mq_service_send_msg(CM_CHTASK_MODULE_ID,CM_DISP_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad) ;
}


/**
* @brief   ���ģ�鷢�͸���ʾ����
 * @param[in]  uiMsgCode ��ֹͣԭ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
#if(USE_BLE==1)
uint8_t mq_service_ch_send_ble(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
    return mq_service_send_msg(CM_CHTASK_MODULE_ID,CM_BEL_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad) ;
}
#endif

/**
* @brief   ���͸�4G��������
 * @param[in]  uiMsgCode ����Ϣ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t mq_service_send_to_4gsend(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
	    return mq_service_send_msg(CM_UNDEFINE_ID,CM_4GSEND_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad);
}
uint8_t mq_service_485Uart_send_recv(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
    return mq_service_send_msg(CM_UNDEFINE_ID,CM_485USRTRECV_MODULE_ID,uiMsgCode,uiMsgVar,uiLoadLen,pucLoad) ;
}



#if(USE_BLE==1)	
/**
* @brief   ���͸�4G��������
 * @param[in]  uiMsgCode ����Ϣ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t uart_service_send_to_blesend(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
    OS_ERR   err;
	static uint8_t countble = 0;
	if(uiLoadLen > MSG_RECV_LEN)
	{
		return FALSE;
	}
	if((pucLoad == NULL) || (uiLoadLen == 0))
	{
		return FALSE;
	}
	memcpy(&pucBLEBuf[countble][0],pucLoad,uiLoadLen);
	stBLEMsg[countble].uiSrcMoudleId    = CM_UNDEFINE_ID;
	stBLEMsg[countble].uiDestMoudleId   = CM_UNDEFINE_ID;
	stBLEMsg[countble].uiMsgCode        = uiMsgCode;
	stBLEMsg[countble].uiMsgVar         = uiMsgVar;
	stBLEMsg[countble].uiLoadLen        = uiLoadLen;
	stBLEMsg[countble].pucLoad          = &pucBLEBuf[countble][0];
	OSQPost (&RecvBELMq,&stBLEMsg[countble],sizeof(MQ_MSG_T),OS_OPT_POST_FIFO,&err);	  
	countble++;
	if(countble >= MSQLEN)   //һ����3�����淢��
	{
		countble = 0;
	}
	return TRUE;
}
#endif

/**
* @brief   ���Ľ��������͵���ʾ����
 * @param[in]  uiMsgCode ����Ϣ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t uart_service_dwinrecv_send_disp(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
     OS_ERR   err;
	static uint8_t countdip = 0;
	if(uiLoadLen > MSG_RECV_LEN)
	{
		return FALSE;
	}
	if((pucLoad == NULL) || (uiLoadLen == 0))
	{
		return FALSE;
	}
	memcpy(&pucDipBuf[countdip][0],pucLoad,uiLoadLen);
	stDipMsg[countdip].uiSrcMoudleId    = CM_DWINRECV_MODULE_ID;
	stDipMsg[countdip].uiDestMoudleId   = CM_DWINRECV_MODULE_ID;
	stDipMsg[countdip].uiMsgCode        = uiMsgCode;
	stDipMsg[countdip].uiMsgVar         = uiMsgVar;
	stDipMsg[countdip].uiLoadLen        = uiLoadLen;
	stDipMsg[countdip].pucLoad          = &pucDipBuf[countdip][0];
	OSQPost (&DwinMq,&stDipMsg[countdip],sizeof(MQ_MSG_T),OS_OPT_POST_FIFO,&err);	  
	countdip++;
	if(countdip >= MSQLEN)   //һ����3�����淢��
	{
		countdip = 0;
	}
	return TRUE;
}


/**
* @brief   4G���շ��͸���������
 * @param[in]  uiMsgCode ����Ϣ��
 *             uiMsgVar  ����Ϣ����
 *             uiLoadLen ����Ϣ��Ч�غɳ���
 *             pucLoad   ��ָ����Ҫ���͵���Ч�غɵ�ַ
 * @param[out]
 * @return 0 �����ͳɹ�  ��0������ʧ��
 * @note
 */
uint8_t uart_service_4GUart_send_recv(uint32_t  uiMsgCode ,uint32_t uiMsgVar ,uint32_t uiLoadLen ,uint8_t  *pucLoad)
{
   
	OS_ERR   err;
	static uint8_t count = 0;
	static uint8_t count4G = 0;
	if(uiLoadLen > URART_4GRECV_LEN)
	{
		return FALSE;
	}
	if((pucLoad == NULL) || (uiLoadLen == 0))
	{
		return FALSE;
	}
	memcpy(&puc4GBuf[count4G][0],pucLoad,uiLoadLen);
	st4GMsg[count4G].uiSrcMoudleId    = CM_4GUARTRECV_MODULE_ID;
	st4GMsg[count4G].uiDestMoudleId   = CM_4GUARTRECV_MODULE_ID;
	st4GMsg[count4G].uiMsgCode        = uiMsgCode;
	st4GMsg[count4G].uiMsgVar         = uiMsgVar;
	st4GMsg[count4G].uiLoadLen        = uiLoadLen;
	st4GMsg[count4G].pucLoad          = &puc4GBuf[count4G][0];
	OSQPost (&Recv4GMq,&st4GMsg[count4G],sizeof(MQ_MSG_T),OS_OPT_POST_FIFO,&err);	  
	count4G++;
	if(count4G >= MSQLEN)   //һ����3�����淢��
	{
		count4G = 0;
	}
	return TRUE;
}
