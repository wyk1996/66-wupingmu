/*====================================================================================================================================
//file:UART2.c
//name:mowenxing
//data:2021/03/23
//readme:
//===================================================================================================================================*/
#include "uart3.h"
#include "at32f4xx_rcc.h"
	
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   UART3BTinit
//fun work：   初始化蓝牙
//in： 无
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
void  UART3BTinit(void)
{
   GPIO_InitType GPIO_InitStructure;
  USART_InitType USART_InitStructure;
	NVIC_InitType NVIC_InitStructure;

	/* config USART2 clock */
	RCC_APB1PeriphClockCmd(RCC_APB1PERIPH_USART3 , ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2PERIPH_GPIOB, ENABLE);

	
	/* USART2 GPIO config */
	/* Configure USART2 Tx (PA.2) as alternate function push-pull */
	GPIO_InitStructure.GPIO_Pins = GPIO_Pins_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_MaxSpeed = GPIO_MaxSpeed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);    
	/* Configure USART2 Rx (PA.3) as input floating */
	GPIO_InitStructure.GPIO_Pins = GPIO_Pins_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
#if((USE_BLE==1)|(USE_BLE==0))
    /* 9600-8-1-N   无奇偶校验   blue和print*/
	USART_InitStructure.USART_BaudRate = 9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
#elif(USE_BLE==2)
    /*9600-9-1   偶校验    单独外部电表用*/
    USART_InitStructure.USART_BaudRate = 9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_9b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_Even;
    /* 485控制引脚初始化 */
    GPIO_InitStructure.GPIO_Pins = GPIO_Pins_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_ResetBits(GPIOB,PB2);  //low
#endif
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);
	
	USART_INTConfig(USART3, USART_INT_RDNE, ENABLE);
	
	//中断优先级设置
  NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1 ;//抢占优先级
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//从优先级
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
  NVIC_Init(&NVIC_InitStructure);
	USART_Cmd(USART3, ENABLE);
}


_UART_RECV_CONTORL Uart3RecvControl = {0};
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   USART2_IRQHandler
//fun work：     UART2中断
//in： 无
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
 void USART3_RecvDispose(uint8_t ch)
 {
	 
	 Uart3RecvControl.recv_buf[Uart3RecvControl.recv_index] = ch;
	 Uart3RecvControl.count = 0;
	 Uart3RecvControl.recv_index++;
	 if(Uart3RecvControl.recv_index >= MSG_RECV_LEN)
	 {
		 Uart3RecvControl.recv_index = 0;
	 }
	 
 }
#if((USE_BLE==1)|(USE_BLE==2))					//蓝牙=1 和 485=2 
//ver： 无
//===================================================================================================================================*/
 void USART3_IRQHandler(void)
{
	uint8_t ch;
	 CPU_SR  cpu_sr = 0;
	
    CPU_CRITICAL_ENTER();                                       /* Tell the OS that we are starting an ISR            */

    OSIntEnter();

    CPU_CRITICAL_EXIT();
	
	if(USART_GetITStatus(USART3, USART_INT_RDNE) != RESET)
	{ 	
		USART_ClearITPendingBit(USART3, USART_INT_RDNE);
			ch = USART_ReceiveData(USART3);
			USART3_RecvDispose(ch);
	} 
	OSIntExit();
}
#endif
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23 
//fun name：   UART2SendByte
//fun work：     UART2发送一个BYTE
//in： IN：发送的单字节
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
void UART3SendByte(unsigned char IN)
{      
	while((USART3->STS&0X40)==0);//循环发送,直到发送完毕   
	USART3->DT = IN;  
}
/*====================================================================================================================================
//name：mowenxing data：   2021/03/23  
//fun name：   UART2SENDBUF
//fun work：     UART2发送一个缓冲数据
//in： buf：数据包缓冲  len：数据包长度
//out:   无     
//ret：   无
//ver： 无
//===================================================================================================================================*/
#if((USE_BLE==1)|(USE_BLE==0))
void UART3SENDBUF(uint8 *buf,uint16  len) 
{
    uint16  i;
//	   __set_PRIMASK(1);
    for(i=0; i<len; i++)
    {
        UART3SendByte(buf[i]);
    }
//		 __set_PRIMASK(0);
}

#elif(USE_BLE==2)
void UART3SENDBUF(uint8_t *buf,uint16_t  len)
{
    uint16_t  i;
    GPIO_SetBits(GPIOB,PB2);  //high
    delay_u(2);
    for(i=0; i<len; i++)
    {
        UART3SendByte(buf[i]);
    }
    while((USART3->STS&0X40)==0);  //最后一个发送完成
    GPIO_ResetBits(GPIOB,PB2);  //low
}
#endif
//END
