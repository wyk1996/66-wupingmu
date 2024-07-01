/*******************************************************************************
 * @file chproc.c
 * @note
 * @brief 充电处理
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
#include "common.h"
#include "dwin_com_pro.h"
#include "4GMain.h"
#include "device.h"
#include "main.h"
#include "dlt645_port.h"
#include "BLELink.h"

#define CH_ENABLE_PIN1              19                  // GET_PIN(C,8) //98//PB4 
#define CH_ENABLE_PIN2              20                  // GET_PIN(C,9) //99//PB3 

extern RATE_T	           stChRate;                 /* 充电费率  */
extern RATE_T	           stChRateA;                 /* 充电费率  */
extern RATE_T	           stChRateB;                 /* 充电费率  */

extern CH_TASK_T stChTcb;


extern SYSTEM_RTCTIME gs_SysTime;


static uint8_t PWMState = 0; //pwm状态   0表示无占空比状态，1表示有占空比状态
uint16_t PWM = 313; 				//32A  313                       16A  493
void cp_pwm_full(void)
{
	PWMState = 0;
	TIM2_PWM_Conrtol(0);
}
void cp_pwm_ch_puls(uint16_t pwm)
{
	PWMState =1;
	TIM2_PWM_Conrtol(pwm);			//32A 313                       16A  493
}

void cp_set_pwm_val(uint16_t pwm)
{
	
	if(pwm > 665)
	{
		return;
	}
	PWM = pwm;
	if(PWMState)
	{
		cp_pwm_ch_puls(PWM);
	}
}


/**
 * @brief
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
void  ch_ctl_init(void)
{
	KEY_OUT1_OFF;
	OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);	
    KEY_OUT2_OFF;
}

/**
 * @brief 充电控制使能
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
void ch_ctl_enable(void)
{
   KEY_OUT1_ON;
   OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);	
    KEY_OUT2_ON;
}

/**
 * @brief 充电控制禁止
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
void ch_ctl_disable(void)
{
    // rt_enter_critical();

	KEY_OUT1_OFF;
	OSTimeDly(100, OS_OPT_TIME_PERIODIC, &timeerr);	
    KEY_OUT2_OFF;
    //rt_exit_critical();
}

/**
 * @brief 跳转到新状态
 * @param[in] pstChTcb 指向充电任务的数据
 *            ucNewStat 要跳到的新状态
 * @param[out]
 * @return
 * @note
 */
void ch_jump_new_stat(CH_TASK_T *pstChTcb,uint8_t ucNewStat)
{
    pstChTcb->ucState = ucNewStat;
    pstChTcb->uiJumpTick = OSTimeGet(&timeerr);
}

/**
 * @brief 是否故障
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_fault(FAULT_T *pstFault)
{
    if(0 == pstFault->uiFaultFlg)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief 是否插枪
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_insert(GUN_T *pstGunTcb)
{
    if(CP_6V == pstGunTcb->ucCpStat || CP_9V == pstGunTcb->ucCpStat)      /* 插枪 */
    {
        pstGunTcb->ucCount = 0;
        return 1;
    }

    if(++pstGunTcb->ucCount > 5)                                          /* 连续5次检测到未插枪 */
    {
        pstGunTcb->ucCount = 0;
        return 0;
    }

    return 1;
}

/**
 * @brief cp 是否off
 * @param[in] pstGunTcb 指向枪
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_cp_off(CH_TASK_T *pstChTcb)
{
    static uint32_t curtick = 0,lasttick = 0;
    curtick = OSTimeGet(&timeerr);
    if(CP_6V == pstChTcb->stGunStat.ucCpStat || CP_9V == pstChTcb->stGunStat.ucCpStat )     /* cp 未断开  插枪状态 */
    {
        lasttick = curtick;
        pstChTcb->stGunStat.ucCount_6v = 0;
       // ch_ctl_enable();
        return 0;
    }

    if((++pstChTcb->stGunStat.ucCount_6v > 5) && ((curtick - lasttick) >  CM_TIME_2_SEC))      /* cp 连续5次off   且 连续2s*/
    {
        lasttick = curtick;
        pstChTcb->stGunStat.ucCount_6v = 0;
        return 1;
    }

    return 0;
}


/**
 * @brief 是否过流
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_over_current(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }

    if(DisSysConfigInfo.energymeter == 0)
    {
        if(dlt645_info.out_cur < 35.0)
        {
            pstChTcb->stOverCurr.ucFlg = CH_DEF_FLG_DIS;       /* 没超过最大电流 */
            return 0;
        }
    }
    else
    {
        if(pstChTcb->stHLW8112.usCurrent < CH_CURR_MAX)
        {
            pstChTcb->stOverCurr.ucFlg = CH_DEF_FLG_DIS;       /* 没超过最大电流 */
            return 0;
        }

    }





    if(CH_DEF_FLG_DIS == pstChTcb->stOverCurr.ucFlg)       /*  第一次发现过流 */
    {
        pstChTcb->stOverCurr.ucFlg   = CH_DEF_FLG_EN;      /* 置标志 */
        pstChTcb->stOverCurr.uiTick  = OSTimeGet(&timeerr);      /* 获取当前时间 */
        return 0;
    }

    if(OSTimeGet(&timeerr) - pstChTcb->stOverCurr.uiTick < CH_TIME_10_SEC)
    {
        return 0;
    }
    return 1;
}

/**
 * @brief 是否过压
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_over_volt(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }



    if(DisSysConfigInfo.energymeter == 0)
    {
        if(dlt645_info.out_cur < 253.0)
        {
            pstChTcb->stOverVolt.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }
    else
    {
        if(pstChTcb->stHLW8112.usVolt < CH_VOLT_MAX)
        {
            pstChTcb->stOverVolt.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }






    if(CH_DEF_FLG_DIS == pstChTcb->stOverVolt.ucFlg)           /* 第一次发现过流 */
    {

        pstChTcb->stOverVolt.ucFlg   = CH_DEF_FLG_EN;          /* 置标志 */
        pstChTcb->stOverVolt.uiTick  = OSTimeGet(&timeerr);        /* 获取当前时间 */
        return 0;
    }

    if(OSTimeGet(&timeerr) - pstChTcb->stOverVolt.uiTick < CH_TIME_10_SEC)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief 是否欠压
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_under_volt(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }

    if(DisSysConfigInfo.energymeter == 0)
    {
        if(dlt645_info.out_vol >= 187.0)
        {   /* 没有欠压 */
            pstChTcb->stUnderVolt.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }
    else
    {
        if(pstChTcb->stHLW8112.usVolt >= CH_VOLT_MIN)
        {   /* 没有欠压 */
            pstChTcb->stUnderVolt.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }




    if(CH_DEF_FLG_DIS == pstChTcb->stUnderVolt.ucFlg)
    {
        pstChTcb->stUnderVolt.ucFlg   = CH_DEF_FLG_EN;     /* 置标志 */
        pstChTcb->stUnderVolt.uiTick  = OSTimeGet(&timeerr);     /* 获取当前时间 */
        return 0;
    }

    if(OSTimeGet(&timeerr) - pstChTcb->stUnderVolt.uiTick < CH_TIME_10_SEC)
    {
        return 0;
    }

    return 1;
}



//=====充电过程中，无电量或者电量异常停止充电=======
uint8_t electricity_err(CH_TASK_T *pstChTcb)
{
    static uint32_t  ChargeEnergybuf=0,lasttime=0,elect_err_flag=0;
    if(CHARGING != pstChTcb->ucState)
    {
		ChargeEnergybuf = 0;
		elect_err_flag = 0;
		lasttime = OSTimeGet(&timeerr);
        return 0;
    }
	
    if((dlt645_info.out_cur > 1.0) || (pstChTcb->stHLW8112.usCurrent > 100))  //内外电表大于1A时
    {
        if((OSTimeGet(&timeerr) >= lasttime ? (OSTimeGet(&timeerr) - lasttime):((0xFFFFFFFF - lasttime) + OSTimeGet(&timeerr))) >= (CH_TIME_15_SEC))
        {
            if(pstChTcb->stCh.uiAllEnergy != ChargeEnergybuf )
            {
                ChargeEnergybuf = pstChTcb->stCh.uiAllEnergy;
                lasttime = OSTimeGet(&timeerr);
                elect_err_flag = 0;
				return 0;
            }
            else
            {
                lasttime = OSTimeGet(&timeerr);
                ++elect_err_flag;
            }
        }
    }

    if(elect_err_flag > 12)    //3分钟电量不动就是异常停止
    {
        elect_err_flag = 0;
        return 1;
    }
    return 0;
} 

/**
 * @brief 是否漏电
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_l_cur(CH_TASK_T *pstChTcb)
{

	if(pstChTcb->stHLW8112.usLCurrent < 30)  //大于30maj
	{ 
		pstChTcb->stLCurrent.ucFlg = CH_DEF_FLG_DIS;
		return 0;
	}




    if(CH_DEF_FLG_DIS == pstChTcb->stLCurrent.ucFlg)
    {
        pstChTcb->stLCurrent.ucFlg   = CH_DEF_FLG_EN;     /* 置标志 */
        pstChTcb->stLCurrent.uiTick  = OSTimeGet(&timeerr);     /* 获取当前时间 */
        return 0;
    }

    if(OSTimeGet(&timeerr) - pstChTcb->stLCurrent.uiTick < CH_TIME_1_SEC)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief 是否持续9V状态
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_under_cp9v(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }
		
		if(CP_6V == pstChTcb->stGunStat.ucCpStat)
		{
		  pstChTcb->stCP9Vstate.ucFlg = CH_DEF_FLG_DIS;
			return 0;
		}


    if(CH_DEF_FLG_DIS == pstChTcb->stCP9Vstate.ucFlg)
    {
        pstChTcb->stCP9Vstate.ucFlg   = CH_DEF_FLG_EN;     /* 置标志 */
        pstChTcb->stCP9Vstate.uiTick  = OSTimeGet(&timeerr);     /* 获取当前时间 */
        return 0;
    }

    if((OSTimeGet(&timeerr) - pstChTcb->stCP9Vstate.uiTick) < (CH_TIME_60_SEC * 30))  //持续30分钟，返回
    {
        return 0;
    }
    return 1;
}

/**
 * @brief 充电待机处理
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note	
 */
uint8_t ch_standy_proc(CH_TASK_T *pstChTcb)
{
    if(ch_is_fault(&pstChTcb->stIOFault) || ch_is_over_current(pstChTcb) || ch_is_over_volt(pstChTcb) || ch_is_under_volt(pstChTcb) || electricity_err(pstChTcb))
    {
        ch_jump_new_stat(pstChTcb,CHARGER_FAULT);
        return 1;
    }

    if(CP_6V == pstChTcb->stGunStat.ucCpStat || CP_9V == pstChTcb->stGunStat.ucCpStat )
    {
        pstChTcb->ucRunState = PT_STATE_INSERT_GUN_NOCHARGING;
        ch_jump_new_stat(pstChTcb,INSERT);
        return 1;
    }

    return 0;
}

/**
 * @brief 是否开始充电
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_start(CH_TASK_T *pstChTcb)
{
//按照24小时制来判断启动条件 【1】 模式5  【2】当前小时 = 预约小时  特别注意：获取时间不能这样写，这样插枪后不停的来获取，会影响记录存入的时间
////    pstChTcb->stCh.uiChStartTime = gs_SysTime;
//    if((pstChTcb->stChCtl.ucChMode == 5)&(pstChTcb->stCh.uiChStartTime.ucHour == pstChTcb->stChCtl.uiStopParam))
//    {
//        DispControl.CurSysState = DIP_STATE_NORMAL;  //预约时间到后，页面自动跳转到当前的状态（省略了一个预充电的枚举）
//        return 1;
//    }

    //用下面的方式来书写
    if((CH_START_CTL == pstChTcb->stChCtl.ucCtl)&(pstChTcb->stChCtl.ucChMode == 5))  //只有刷卡后下发启动命令和启动预约才可以启动
    {
        pstChTcb->stCh.uiChStartTime = gs_SysTime; //获取当前的时间
        if(pstChTcb->stCh.uiChStartTime.ucHour == pstChTcb->stChCtl.uiStopParam)
        {
            DispControl.CurSysState = DIP_STATE_NORMAL;  //预约时间到后，页面自动跳转到当前的状态（省略了一个预充电的枚举）
            return 1;
        }
		
        return 0;
    }

    if((CH_START_CTL == pstChTcb->stChCtl.ucCtl)&(pstChTcb->stChCtl.ucChMode != 5))  /* 其他的模式下发后，直接开始*/
    {
        return 1;
    }

    return 0;
}

/**
 * @brief 开始充电函数
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
void ch_start_func(CH_TASK_T *pstChTcb)
{
    uint8_t    ucState = 0;
    uint32_t   uiE = 0;

    memset(&pstChTcb->stCh,0,sizeof(CH_T));
    pstChTcb->stLowCurr.uiTick  = OSTimeGet(&timeerr);;
    if(DisSysConfigInfo.energymeter == 0)
    {
        uiE  = dlt645_info.cur_hwh * 1000;  //小数点3位
    }
    else
    {
        uiE  = pstChTcb->stHLW8112.uiE;
    }


    pstChTcb->stCh.uiDChQ        = uiE;

	 pstChTcb->stCh.uiChStartTime = gs_SysTime;
	pstChTcb->stCh.timestart = cp56time(NULL);
	
    cp_pwm_ch_puls(PWM);

    if(CP_6V == pstChTcb->stGunStat.ucCpStat)
    {
        ucState = PRECHARGE;
        ch_ctl_enable();
    }
    else
    {
        ucState = WAIT_CAR_READY;
    }

    ch_jump_new_stat(pstChTcb,ucState);
}

 #if(USE_BLE==1)
/**
 * @brief ch_is_ble_start
 * @param[in] 蓝牙预约充电
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_ble_start(void)
{
	uint32_t ucMin;
	uint8_t i;

	if(stChTcb.BLEInfo.Reserva  ==  DISABLE )  //非预约
	{
		return 0;
	}	
	
	ucMin = gs_SysTime.ucMin + gs_SysTime.ucHour * 60;  //当天转换为分钟
	for(i = 0;i < stChTcb.BLEInfo.num;i++)
	{
		if(stChTcb.BLEInfo.time[i].starttime < stChTcb.BLEInfo.time[i].stoptime)  //开始时间小，说明跨时间段了
		{
			if((ucMin >= stChTcb.BLEInfo.time[i].starttime) && (ucMin <= stChTcb.BLEInfo.time[i].stoptime) )
			{
				stChTcb.stChCtl.ucCtl = CH_START_CTL;
				stChTcb.BLEInfo.curtimequau = i;
				return 1;
			}
		}
		else  //开始时间大，说明跨时间段了 如23  到 2点，拆分为 23 - 24  0 - 2
		{
			if( ((ucMin >= stChTcb.BLEInfo.time[i].starttime) && (ucMin <= 24*60) ) || \
				(ucMin <= stChTcb.BLEInfo.time[i].stoptime)  ) 
			{
				stChTcb.stChCtl.ucCtl = CH_START_CTL;
				stChTcb.BLEInfo.curtimequau = i;
				return 1;
			}
		}
	}
	return 0;
}


uint8_t ch_is_ble_stop(void)
{
	uint32_t ucMin;
	uint8_t i;

	if(stChTcb.BLEInfo.Reserva  ==  DISABLE )  //非预约
	{
		return 0;
	}	
	if(stChTcb.BLEInfo.curtimequau >= 3)
	{
		return FALSE;
	}
	
	ucMin = gs_SysTime.ucMin + gs_SysTime.ucHour * 60;  //当天转换为分钟
	for(i = 0;i < stChTcb.BLEInfo.num;i++)
	{
		
		if(stChTcb.BLEInfo.time[i].starttime < stChTcb.BLEInfo.time[i].stoptime)  //开始时间小，说明跨时间段了
		{
			if(ucMin > stChTcb.BLEInfo.time[i].stoptime)
			{
				return 1;
			}
		}
		else  //开始时间大，说明跨时间段了 如23  到 2点，拆分为 23 - 24  0 - 2   3
		{
			if((ucMin > stChTcb.BLEInfo.time[i].stoptime)  &&  (ucMin < stChTcb.BLEInfo.time[i].starttime) )
			{
				return 1;
			}
		}
	}
	return 0;
}
#endif

/**
 * @brief 插枪处理
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_insert_proc(CH_TASK_T *pstChTcb)
{
    if(ch_is_fault(&pstChTcb->stIOFault) || ch_is_over_current(pstChTcb) || ch_is_over_volt(pstChTcb) || ch_is_under_volt(pstChTcb) || electricity_err(pstChTcb))
    {
        ch_jump_new_stat(pstChTcb,CHARGER_FAULT);
        return 1;
    }

    if(0 == ch_is_insert(&pstChTcb->stGunStat))         //连续5次检测都未为插枪，才认为为为插枪
    {
        pstChTcb->ucRunState = PT_STATE_IDLE;              /* 插枪未充电 */
        ch_jump_new_stat(pstChTcb,STANDBY);
        mode5();  //等于预约模式时，拔枪后自动写入记录
        //stChTcb.ucState = CH_TO_DIP_STOP;
        DispControl.CurSysState = DIP_STATE_NORMAL;  //只要检测到拔枪  就会清空一下状态
        memset(&pstChTcb->stChCtl,0,sizeof(pstChTcb->stChCtl));  //拔枪后，启停结构体清零
        return 1;
    }

    if(ch_is_start(pstChTcb))                               /* 开始充电 */
    {
        ch_start_func(pstChTcb);
        return 1;
    }
	//蓝牙预约充电
	 #if(USE_BLE==1)
	if(ch_is_ble_start())
	{
		 ch_start_func(pstChTcb);
        return 1;
	}
	#endif
    return 0;
}



/**
 * @brief 是否持续低电流
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_low_current(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }

    if(DisSysConfigInfo.energymeter == 0)
    {
        if(dlt645_info.out_cur >= 1.0)
        {
            pstChTcb->stLowCurr.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }
    else
    {
        if(pstChTcb->stHLW8112.usCurrent >= CH_CURR_THRESHOLD) 	     /* 电流没有小于门槛电流 */
        {
            pstChTcb->stLowCurr.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }

    if(CH_DEF_FLG_DIS == pstChTcb->stLowCurr.ucFlg)
    {
        pstChTcb->stLowCurr.ucFlg   = CH_DEF_FLG_EN;
        pstChTcb->stLowCurr.uiTick  = OSTimeGet(&timeerr);
        return 0;
    }

  	if((OSTimeGet(&timeerr) - pstChTcb->stLowCurr.uiTick) <= (CH_TIME_60_SEC * 15))   /*6v无电流持续15min*/
    {
        return 0;
    }
    return 1;
}


/**
 * @brief 9V是否持续低电流
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */

uint8_t ch_is_low_current_9V(CH_TASK_T *pstChTcb)
{
    if(CHARGING != pstChTcb->ucState)
    {
        return 0;
    }

    if(DisSysConfigInfo.energymeter == 0)
    {
        if(dlt645_info.out_cur >= 1.0)
        {
			pstChTcb->stCP9Vstate.ucFlg = CH_DEF_FLG_DIS;
            pstChTcb->stLowCurr.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }
    else
    {
        if(pstChTcb->stHLW8112.usCurrent >= CH_CURR_THRESHOLD) 	     /* 电流没有小于门槛电流 */
        {
			pstChTcb->stCP9Vstate.ucFlg = CH_DEF_FLG_DIS;
            pstChTcb->stLowCurr.ucFlg = CH_DEF_FLG_DIS;
            return 0;
        }
    }

    if(CH_DEF_FLG_DIS == pstChTcb->stLowCurr.ucFlg)
    {
        pstChTcb->stLowCurr.ucFlg   = CH_DEF_FLG_EN;
        pstChTcb->stLowCurr.uiTick  = OSTimeGet(&timeerr);
        return 0;
    }

    if(ch_is_under_cp9v(pstChTcb) == 0 )   //无电流持续2min+9v持续2min
    {
        return 0;
    }

    return 1;
}
/**
 * @brief
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
//uint8_t ch_is_end_condition(CH_TASK_T *pstChTcb)
//{
//    /*    uint32_t uiParam = 0;

//        switch(pstChTcb->stChCtl.ucChMode)
//    	{
//            case 0:
//    			uiParam = 0;
//    	        break;
//    		case 1:
//    			uiParam = pstChTcb->stCh.stChInfo.uiTotalMoney/1000;
//    			break;
//    		case 2:
//    			uiParam = pstChTcb->stCh.stChInfo.uiChTime;
//    			break;
//    		case 3:
//    			uiParam = pstChTcb->stCh.stChInfo.uiChTotalQ;
//    		    break;
//    		default:
//    			break;
//    	}

//        uiParam = uiParam;
//    	if(uiParam >= pstChTcb->stChCtl.uiStopParam)
//    	{
//            return 1;
//    	}
//    */
//    return 0;
//}

extern _m1_card_info m1_card_info;		 //M1卡相关信息

uint8_t money_not_enough(CH_TASK_T *pstChTcb)
{
	  //==判断金额大于    单机/单机预约模式下    卡类型是收费卡
    if(((pstChTcb->stCh.uiAllEnergy / 100) >= m1_card_info.balance) && (m1_card_info.CardType==C0card || m1_card_info.CardType==C1card) && (DisSysConfigInfo.standaloneornet == DISP_CARD || DisSysConfigInfo.standaloneornet == DISP_CARD_mode))
    {
        return 1;
    }
    return 0;
}



//充电中循环处理是否达到充电条件
//0：自动充满  1:按电量充电  2:按时间充电(单位1分钟)  3:按金额充电(单位0.01元)  4:自动充满
uint8_t ch_is_end_condition(CH_TASK_T *pstChTcb)
{
    uint32_t Ch_condition = 0, ch_systime = 0;
    switch(pstChTcb->stChCtl.ucChMode)
    {
    case 1:
        Ch_condition = pstChTcb->stCh.uiChargeEnergy/1000;  //1：按电量充电
        break;

    case 2:   //2:按时间充电(单位1分钟)
        Ch_condition = pstChTcb->stCh.timestart+(pstChTcb->stChCtl.uiStopParam*60);
        break;

    case 3:   //3：金额充值
        Ch_condition = pstChTcb->stCh.uiAllEnergy/10000;  //3:按金额充电
        break;

    default:
        break;  //其他的方式充电
    }


    ch_systime=cp56time(NULL);  //获取当前的时间
	
    if((pstChTcb->stChCtl.ucChMode==2)&&(ch_systime >= Ch_condition))  //2按时间来充，当前时间必须 >= 定时的时间停止
    {
        return 1;
    }

    if((pstChTcb->stChCtl.ucChMode==1)&&(Ch_condition >= pstChTcb->stChCtl.uiStopParam)) //1按电量充  当前的电量 >= 要充的电量停止
    {
        return 1;
    }

    if((pstChTcb->stChCtl.ucChMode==3)&&(Ch_condition >= pstChTcb->stChCtl.uiStopParam)) //3按金额充
    {
        return 1;
    }


    return 0;
}

/**
 * @brief 充电是否停止
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_is_stp(CH_TASK_T *pstChTcb)
{
    if(ch_is_fault(&pstChTcb->stIOFault) && pstChTcb->stIOFault.ucFrist > 0)
    {
        if(pstChTcb->stIOFault.ucFrist == INPUT_YX2)   //急停时返回1
        {
            return EM_STOP; //急停
        }
        else
        {
            return STOP_OTHEN;
        }
    }


    /* 大致为1s中时间 */
    if(((PRECHARGE == pstChTcb->ucState )  ||  (CHARGING == pstChTcb->ucState ) || (WAIT_CAR_READY == pstChTcb->ucState) )/* 预充 */
            && (ch_is_cp_off(pstChTcb)))
    {
        if(CP_9V == pstChTcb->stGunStat.ucCpStat)                           /* 9v 断开继电器,但不停止充电,见标准 */
        {
            //ch_ctl_disable(); //修改流程
            printf("cp==9v,ch disable,cp volt:%d\r\n",pstChTcb->stGunStat.uiCpVolt) ;
        }
        else
        {
            return ((CP_12V == pstChTcb->stGunStat.ucCpStat) ? UNPLUG : CP_LINK_ABN);
        }
    }


    if(CH_STOP_CTL == pstChTcb->stChCtl.ucCtl)
    {
        if((_4G_GetStartType() ==  _4G_APP_START )&&(DisSysConfigInfo.standaloneornet == DISP_NET))
        {
            return E_END_BY_APP;   //刷卡主动停止
        }
        return E_END_BY_CARD;   //刷卡主动停止
    }
	
	
	if(electricity_err(pstChTcb))
	{
	   return R_POWER_DOWN;     /* 电表异常-检测到掉电 */
	}
	

//    if(ch_is_low_current(pstChTcb))  //持续60s无电流，暂时注释
//    {
//        return NO_CURRENT;
//    }
	
	if(ch_is_low_current_9V(pstChTcb) == 1 )   //无电流持续30min+9v持续10min    无电流9V停止
    { 
        return NO_CURRENT_9V;
    }

    if(money_not_enough(pstChTcb))  /*单机/单机预约 收费卡   判断单机余额不足*/
    {
        return E_END_APP_BALANC;
    }
    if(ch_is_end_condition(pstChTcb))
    {
        return END_CONDITION;
    }

    if(ch_is_over_current(pstChTcb))
    {
        return  OVER_CURRENT;     /* 过流 */
    }

    if(ch_is_over_volt(pstChTcb))
    {               
        return OVER_VOLT;       /* 过压 */
    }

    if(ch_is_under_volt(pstChTcb))
    {
        return UNDER_VOLT;      /* 欠压 */
    }
//	if(ch_is_l_cur(pstChTcb))
//	{
//		return FAIL_L_CUR;
//	}
	#if(USE_BLE==1)
	if(ch_is_ble_stop())   //蓝牙预约是否结束充电
	{
		return BLE_RESER_STOP;
	}
	#endif
    return 0;
}

time_t    tTime = 0;
/**
 * @brief
 * @param[in]
 * @param[out]
 * @return
 * @note
 */
uint32_t get_q_money(uint32_t uiQ,uint32_t *puiaE,uint32_t *pfwuiaE,uint32_t *pfsuiaE,uint32_t *pfsfwuiaE)
{
    uint32_t  uiSec = 0;
    uint32_t  uiFeeIndex = 0;
    uint32_t  uiFee = 0,uifwFee = 0;
    uint32_t  uiTmp = 0,uifwTmp;
    RATE_T * pstChRate;
	
    tTime  = cp56time(NULL);
	tTime += BEIJI_TIMESTAMP_OFFSET;            /* 获取北京时间的时间戳 */
    uiSec  = tTime % CH_DAT_SEC;                       /* 获取当天对应的时间 */

    uiFeeIndex = uiSec / CH_HALF_HOUR_SEC;


	pstChRate = &stChRate;
	
	if(uiFeeIndex >= 48)
	{
		uiFeeIndex =  0;
	}
	stChTcb.stCh.Curkwh[uiFeeIndex] += uiQ;
	uifwFee = 0;			//服务费固定位0
	uiFee = pstChRate->SegPrices[uiFeeIndex];

    uiTmp = uiQ * uiFee;                                 /* 电量3位小数 费率5位费率 */
    uiTmp = uiTmp / 100;                               /* 总8位小数,费率需要保存6位小数 */



    uifwTmp = uiQ * uifwFee;                                 /* 电量3位小数 费率5位费率 */
    uifwTmp = uifwTmp / 100;                               /* 总8位小数,费率需要保存6位小数 */



	pfsuiaE[uiFeeIndex] += uiTmp;
    pfsfwuiaE[uiFeeIndex] += uifwTmp;
    return 1;
}

/**
 * @brief 充电信息更新
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
void ch_info_update(CH_TASK_T *pstChTcb)
{
    uint32_t  uiE     = 0, uiDE = 0;
	  uint8_t  count  = 0;
	uint32_t AllEnergy = 0;
	

    if(DisSysConfigInfo.energymeter == 0)
    {
        uiE  = dlt645_info.cur_hwh * 1000;      /* 小数点三位 */
        uiDE = uiE - pstChTcb->stCh.uiDChQ;
    }
    else
    {
     	if(pstChTcb->stCh.EnergySelect)  //自行计算
		{
			uiE  = pstChTcb->stCh.viDChQ;
			uiDE = uiE >= pstChTcb->stCh.uiDChQ ?(uiE - pstChTcb->stCh.uiDChQ)
				   : 1;
		}
		else
		{
			uiE  = pstChTcb->stHLW8112.uiE;
			uiDE = uiE >= pstChTcb->stCh.uiDChQ ?(uiE - pstChTcb->stCh.uiDChQ)
				   : 1;
		}
    }

	if(uiDE > 500)     //电能一次超过0.5度，做一个保护
	{
		 pstChTcb->stCh.uiDChQ   = uiE; 	  //记录上一次读取的电量
		return;
	}
	
    if(0 < uiDE)
    {
        pstChTcb->stCh.uiChargeEnergy += uiDE;
		if(reststae)  //复位处理
		{
			reststae = 0;
			memcpy((void *)pstChTcb->stCh.uifsChargeEnergy,(void *)uifsChargeEnergy,sizeof(uifsChargeEnergy));   //断电接着上一个订单充电，拷贝充电信息
		}
        get_q_money(uiDE,(uint32_t *)pstChTcb->stCh.uiaChargeEnergy,(uint32_t *)pstChTcb->stCh.uifwChargeEnergy,(uint32_t *)pstChTcb->stCh.uifsChargeEnergy,(uint32_t *)pstChTcb->stCh.uifsfwChargeEnergy);
        pstChTcb->stCh.uiDChQ          = uiE; 	  //记录上一次读取的电量
		AllEnergy = 0;
		for(count = 0;count < 48;count++)
		{
			AllEnergy += pstChTcb->stCh.uifsChargeEnergy[count];
		}
		
		pstChTcb->stCh.uiAllEnergy  = AllEnergy/100;
		
    }
}

/**
 * @brief 充电完成函数
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
void ch_cplt_func(CH_TASK_T *pstChTcb)
{
    uint8_t ucState  = 0;

    if(ch_is_fault(&pstChTcb->stIOFault) || ch_is_over_current(pstChTcb) || ch_is_over_volt(pstChTcb) || ch_is_under_volt(pstChTcb))
    {
        ucState = CHARGER_FAULT;
    }
    else
    {
        if(ch_is_insert(&pstChTcb->stGunStat))
        {
            ucState = INSERT;
            pstChTcb->ucRunState = PT_STATE_CHARGING_STOP_NOT_GUN;
        }
        else
        {
            ucState = STANDBY;
            pstChTcb->ucRunState = PT_STATE_IDLE;
        }
    }
    ch_jump_new_stat(pstChTcb,ucState);
    memset(&pstChTcb->stChCtl,0,sizeof(CH_CTL_T));

}

/**
 * @brief 充电停止函数
 * @param[in] pstChTcb 指向充电任务的数据
 *            ucReason 停止原因
 * @param[out]
 * @return
 * @note
 */
void ch_stop_func(CH_TASK_T *pstChTcb,uint8_t ucReason)
{
	OS_ERR      err;
	
    cp_pwm_full();
	OSTimeDly(2000, OS_OPT_TIME_PERIODIC, &err);
    ch_ctl_disable();

    if(ch_is_insert(&pstChTcb->stGunStat))
    {
        pstChTcb->ucRunState = PT_STATE_CHARGING_STOP_NOT_GUN;
        pstChTcb->stChCtl.ucCtl = CH_STOP_CTL;
    }
    else
    {
        pstChTcb->ucRunState = PT_STATE_IDLE;
        pstChTcb->stChCtl.ucCtl = CH_STOP_CTL;
    }

    ch_jump_new_stat(pstChTcb, WAIT_STOP);
}

/**
 * @brief 等待车就绪
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_wait_car_ready(CH_TASK_T *pstChTcb)
{
    uint8_t ucReason = 0;

    do
    {
        ucReason = ch_is_stp(pstChTcb);

        /* 充电停止 */
        if(0 < ucReason)
        {
            break;
        }
        /* 一分钟车还未就绪 */
        if(OSTimeGet(&timeerr) - pstChTcb->uiJumpTick > CH_TIME_60_SEC)
        {
            ucReason = READY_TIMEOUT;
            break;
        }

        /* 车就绪 */
        if(CP_6V == pstChTcb->stGunStat.ucCpStat)
        {
            pstChTcb->stGunStat.ucCount_6v = 0;
            ch_ctl_enable();
            ch_jump_new_stat(pstChTcb,PRECHARGE);
        }
        return 0;
    } while(0);

    mq_service_ch_send_dip(CH_TO_DIP_STARTFAIL ,ucReason ,0,NULL);
	#if(USE_BLE==1)
	mq_service_ch_send_ble(CH_TO_DIP_STARTFAIL ,ucReason ,0,NULL);
	#endif
    ch_stop_func(pstChTcb,ucReason);

    return ucReason;
}

/**
 * @brief 预充电
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
void ch_precharge(CH_TASK_T *pstChTcb)
{
    pstChTcb->ucRunState = PT_STATE_CHARGING;
    ch_info_update(pstChTcb);

    ch_jump_new_stat(pstChTcb,CHARGING);
    mq_service_ch_send_dip(CH_TO_DIP_STARTSUCCESS ,0 ,0,NULL);  //启动成功
	#if(USE_BLE==1)
	mq_service_ch_send_ble(CH_TO_DIP_STARTSUCCESS ,0 ,0,NULL); 
	#endif
}

/**
 * @brief 充电
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_charging(CH_TASK_T *pstChTcb)
{
    uint8_t ucReason = 0;

    ch_info_update(pstChTcb);

    ucReason = ch_is_stp(pstChTcb);

    if(0 < ucReason)
    {
        printf("stop charge , reason = %d!!\r\n",ucReason);
        ch_stop_func(pstChTcb,ucReason);
        mq_service_ch_send_dip(CH_TO_DIP_STOP ,ucReason,0,NULL);   //发送停止
		#if(USE_BLE==1)
		if((E_END_BY_APP == ucReason) || (E_END_BY_CARD == ucReason) )
		{
			  mq_service_ch_send_ble(CH_TO_DIP_STOP ,ucReason,0,NULL);
		}
		if( ucReason == NO_CURRENT)
		{
			pstChTcb->BLEInfo.Reserva = DISABLE;  //取消预约
		}
		#endif
        return ucReason;
    }
    return 0;
}

/**
 * @brief 充电等待停止
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_wait_stop(CH_TASK_T *pstChTcb)
{
//    do
//    {
//        if(CH_IS_POWERDOWN(pstChTcb->stFault.uiFaultFlg))
//        {
//            /* 掉电 */
//            break;
//        }
//
//        if(rt_tick_get() - pstChTcb->uiJumpTick > CH_TIME_30_SEC)
//        {
//            /* 进入停止状态 */
//            break;
//        }
//
//        if(ch_is_low_current(pstChTcb))
//        {
//            /* 确定无电流 */
//            break;
//        }
//
//        return 0;
//
//    } while(0);

    ch_cplt_func(pstChTcb);
    return 1;
}
/**
 * @brief 故障处理
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
uint8_t ch_fault_proc(CH_TASK_T *pstChTcb)
{
    uint8_t ucState = 0;

    do
    {
        if((0 == pstChTcb->stIOFault.uiFaultFlg) && (ch_is_over_current(pstChTcb) == 0) && (ch_is_over_volt(pstChTcb) == 0) && (ch_is_under_volt(pstChTcb) == 0))
        {
            pstChTcb->stIOFault.ucFrist = 0;
            if(ch_is_insert(&pstChTcb->stGunStat))
            {
                ucState = INSERT;
                pstChTcb->ucRunState = PT_STATE_INSERT_GUN_NOCHARGING;
            }
            else
            {
                ucState = STANDBY;
                pstChTcb->ucRunState = PT_STATE_IDLE;
            }
            ucState = STANDBY;
            break;
        }
        return 0;
				
    } while(0);

    ch_jump_new_stat(pstChTcb,ucState);

    return 1;
}

/**
 * @brief 充电状态机循环处理
 * @param[in] pstChTcb 指向充电任务的数据
 * @param[out]
 * @return
 * @note
 */
void ch_loop_proc(CH_TASK_T *pstChTcb)
{
    switch(pstChTcb->ucState)
    {
    case STANDBY:
        ch_standy_proc(pstChTcb);
		#if(USE_BLE==1)   
		pstChTcb->BLEInfo.Reserva = DISABLE;  //拔枪取消预约
		#endif
        break;
    case INSERT:
        ch_insert_proc(pstChTcb);
        break;
    case WAIT_CAR_READY:
        ch_wait_car_ready(pstChTcb);
        break;
    case PRECHARGE:
        ch_precharge(pstChTcb);
        break;
    case CHARGING:
        ch_charging(pstChTcb);
        break;
    case WAIT_STOP:
        ch_wait_stop(pstChTcb);  //充电等待停止
        break;
    case CHARGER_FAULT:
        ch_fault_proc(pstChTcb);
        break;
    case POWER_DOWN:
        break;
    default:
        break;
    }
}

