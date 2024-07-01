/*****************************************Copyright(C)******************************************
*******************************************杭州快电*********************************************
*------------------------------------------文件信息---------------------------------------------
* FileName			: GPRSMain.h
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
#ifndef	__BLE_H_
#define	__BLE_H_
#include <stdint.h>
/* Private define-----------------------------------------------------------------------------*/
/* Private typedef----------------------------------------------------------------------------*/
/* Private macro------------------------------------------------------------------------------*/
/* Private variables--------------------------------------------------------------------------*/
/* Private function prototypes----------------------------------------------------------------*/
/* Private functions--------------------------------------------------------------------------*/
typedef enum
{
	BLE_UNLINK = 0,
	BLE_LINK,
}_BLE_LINK_STATE;

typedef struct
{
	_BLE_LINK_STATE BLELinkState;  //蓝牙连接状态
	uint8_t MACAdd[6];
}_BLE_CONTROL;

extern _BLE_CONTROL BLEControl;
#endif
/************************(C)COPYRIGHT  66*****END OF FILE****************************/

