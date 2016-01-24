
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "main.h"

/** @addtogroup Template
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SECRET_NUMBER 0x01


/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
extern CanRxMsg RxMessage;
extern CanTxMsg TxMessage;
extern void Delay(uint32_t);
extern void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
extern void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
__IO uint32_t msCount;

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
  {
  }
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
  {
  }
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
  {
  }
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
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	msCount++;
}


/******************************************************************************/
/*            STM32F4xx Peripherals Interrupt Handlers                        */
/******************************************************************************/

/**
  * @brief  This function handles TIM3 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM3_IRQHandler(void)
{
  
	

}

 

void EXTI0_IRQHandler(void){
	/* add user-button handling code here */


	
	EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);  //NEED THIS LINE TO MAKE THE ROUTINE WORK!!!
}



void EXTI1_IRQHandler(void){
// for the external , another button.
//why one time push the button, trigger interrupt twice?	---Contace bouncing problem. need debouncing.
	
	
	 
	EXTI_ClearITPendingBit(EXTI_Line1);
}

#ifdef USE_CAN1
/**
  * @brief  This function handles CAN1 RX0 request.
  * @param  None
  * @retval None
  */
void CAN1_RX0_IRQHandler(void){
	if(CAN_GetITStatus(CAN1, CAN_IT_FMP0) != RESET){
		CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);
		STM_EVAL_LEDOn(LED4);
		Msg_Display(3, 1, (uint8_t*)"DataRx:", RxMessage.Data[1]); // Display the transmitted message data
		CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
		NVIC_SystemReset();
	}
}
#endif  /* USE_CAN1 */

#ifdef USE_CAN2
/**
  * @brief  This function handles CAN2 RX0 request.
  * @param  None
  * @retval None
  */
void CAN2_RX0_IRQHandler(void)
{
 
}
#endif  /* USE_CAN2 */

