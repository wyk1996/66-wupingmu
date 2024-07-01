/*******************************************************************************
 * @file ch_port.c
 * @note
 * @brief ���ӿں���
 *
 * @author
 * @date     2021-05-02
 * @version  V1.0.0
 *
 * @Description
 *
 * @note History:
 * @note     <author>   <time>    <version >   <desc>
 * @note
 * @warning
 *******************************************************************************/
#include <string.h>
#include "chtask.h"
#include "ch_port.h"
#include "common.h"
extern SYSTEM_RTCTIME gs_SysTime;
#define  IS_LEAP_YEAR(usYear)      ((((usYear) % 4) == 0) && (((usYear) % 100) != 0) || (((usYear) % 400) == 0))
#define  YEAR_TOTAL_DATA(usYear)   ((IS_LEAP_YEAR(usYear)) ? 366 : 365)

/* 1970��1��1�������� */
static const uint8_t  cucaWeekTab[7] = {4,5,6,7,1,2,3};
/* 1��12���¶�Ӧ������ */
static const uint8_t  cucaDay[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
/* ������������ */
uint16_t g_usChStartCpltCode = 0;



/**
 * @brief ����ʱ��ת��Ϊcp56
 * @param[in] tTime: ����ʱ�� pstCp56: �����cp56ʱ��
 * @param[out]
 * @return
 * @note
 */
void localtime_to_cp56time(time_t tTime, CP56TIME2A_T *pstCp56)
{
    uint32_t    uiDay = 0;
    uint32_t    uiWeekIndex = 0;
    uint32_t    uiTmp = 0;
    uint8_t     ucMonDay[12] = {0};
    uint8_t     ucSec = 0;
    uint8_t     ucMon = 0;
    uint16_t    usYear = 0;

	tTime += 8*60*60;
    uiDay       = tTime/86400;                   //һ��86400 �룬�����ж�����
    uiTmp       = tTime - uiDay * 86400;           //ʣ����ǵ����ʱ��
    uiWeekIndex = uiDay % 7;

    pstCp56->ucHour = uiTmp / 3600;
    uiTmp -= (uint32_t)pstCp56->ucHour * 3600;

    pstCp56->ucMin = uiTmp / 60;
    ucSec = uiTmp % 60;
    pstCp56->usMsec = (uint16_t)ucSec * 1000;

    pstCp56->ucWeek = cucaWeekTab[uiWeekIndex];

    for(usYear = 1970; uiDay >= YEAR_TOTAL_DATA(usYear); usYear++)
    {
        uiDay -= YEAR_TOTAL_DATA(usYear);
    }

    pstCp56->ucYear = usYear >= 2000 ? usYear - 2000 : usYear - 1970;

    memcpy(ucMonDay,cucaDay,12);

    if(IS_LEAP_YEAR(usYear))
    {
        ucMonDay[1] = 29;
    }

    for(ucMon = 1; uiDay >= ucMonDay[ucMon - 1]; ucMon++)
    {
        uiDay -= ucMonDay[ucMon - 1];
    }

    pstCp56->ucMon = ucMon;
    pstCp56->ucDay = uiDay + 1;
}

/**
 * @brief
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
static uint32_t time_cal_day(uint8_t ucMon)
{
    uint8_t  i;
    uint32_t uiSum = 0;

    if (ucMon > 12 || ucMon <= 1)
    {
        return 0;
    }

    for (i = 0; i < (ucMon - 1); i++)
    {
        uiSum += cucaDay[i];
    }
    return uiSum;
}

/**
 * @brief cp56ת��Ϊ����ʱ��
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint32_t cp56time(void * pq)
{
	
    uint8_t  ucTmp0 = 0;
    uint8_t  ucTmp1 = 0;
    uint8_t  ucYear = 0;
    uint32_t uiDt = 0;

	pq =pq;
	
    /* pstCp56->ucYear �� 2000 �꿪ʼ */
    ucYear = 30 + gs_SysTime.ucYear - 100;

    ucTmp0 = ucYear / 4;
    ucTmp1 = ucYear % 4;

    uiDt  = ucTmp0 * 1461 + ucTmp1 * 365;
    uiDt += ucTmp1 > 2 ? 1 : 0;


    uiDt += time_cal_day(gs_SysTime.ucMonth);
    /* ���� ��һ�� */
    uiDt += ((ucTmp1 == 2) && (gs_SysTime.ucMonth > 2)) ? 1 : 0;

    uiDt += (gs_SysTime.ucDay - 1);

    uiDt *= 24;
    uiDt += gs_SysTime.ucHour;

    uiDt *= 60;
    uiDt += gs_SysTime.ucMin;

    uiDt    *= 60;
    ucTmp0   = gs_SysTime.ucSec;
    uiDt    += ucTmp0;
   uiDt -= (8*3600);  //����ʱ���1970��8��00��ʼ

    return uiDt;
}
