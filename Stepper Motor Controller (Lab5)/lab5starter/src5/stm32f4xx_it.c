

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f429i_discovery.h"
#include "main.h"
__IO uint32_t RTCAlarmCount = 0;
extern __IO uint8_t AlarmUp;

extern int mode;
extern int direction;
int flag = 0;
extern void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);

#define Forward 0				//variables
#define Reverse 1
#define HalfStep 0
#define FullStep 1


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {}
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{}

/**
  * @brief  This function handles PendSV_Handler exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void){
	//uncomment me if you want to handle systicks
	//TimingDelay_Decrement();
}

/******************************************************************************/
/*            STM32F4xx Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)			//TIM3 ISR toggles LED's
{
  if (TIM_GetITStatus(TIM3, TIM_IT_CC1) != RESET)
  {
		flag = 1;
		STM_EVAL_LEDToggle(LED3);
		STM_EVAL_LEDToggle(LED4);
		TIM_ClearITPendingBit(TIM3, TIM_IT_CC1);	
		TIM_SetCounter(TIM3, 0x0000); 
  }
}


/**
 * External interrupt channel 0 Interrupt Handler. This handles
 * the user button.
 */
void EXTI0_IRQHandler(void){
	if (mode == HalfStep) {mode = FullStep;}   //exclusive orr
	else if (mode == FullStep) {mode = HalfStep;}
	EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
}

