
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_it.h"
#include "stm32f429i_discovery.h"
#include "main.h"
#include "stm32f4xx_rtc.h"

/**
  * @brief  Converts a 2 digit decimal to BCD format.
  * @param  Value: Byte to be converted.
  * @retval Converted byte
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
//extern __IO uint8_t UBPressed;
extern int alarm;
extern uint16_t line;
extern int column;
int a = 0;
extern int time;
extern RTC_TimeTypeDef  RTC_TimeStruct;
extern int val_buff_1;
extern int val_buff_2;


int state = 0;
int month = 0; //multiply m1*10 plus odate from state 2
int year = 0;
int date = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

void Display_Time(void);
void Display_Date(void);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);

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

void EXTI0_IRQHandler(void)		//press button to show date
{
	//RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
	Display_Date();
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
	time = RTC_TimeStruct.RTC_Seconds;
	while(STM_EVAL_PBGetState(BUTTON_USER) == Bit_SET);
	LCD_DisplayString(line, 0, "                       ");		
	a = 2;
	EXTI_ClearITPendingBit(USER_BUTTON_EXTI_LINE);
}


void EXTI2_IRQHandler(void){ //reset button
	state++;
	if (state == 7) { 
		state = 0;	
		RTC_TimeStruct.RTC_Hours = hours;
		RTC_TimeStruct.RTC_Minutes = minutes;
		RTC_TimeStruct.RTC_Seconds = seconds;		
		RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct);
		RTC_WaitForSynchro();
		RTC_AlarmCmd(RTC_Alarm_A, ENABLE);

	} 
		LCD_DisplayInt(5, 5, state);
	EXTI_ClearITPendingBit(EXTI_Line2);
}


void RTC_Alarm_IRQHandler(void)
{
  /* Check on the AlarmA flag and on the number of interrupts per Second (60*8) */
  if(RTC_GetITStatus(RTC_IT_ALRA) != RESET) 
  { 
    alarm = 1;
    RTC_ClearITPendingBit(RTC_IT_ALRA);
  }
  /* Clear the EXTIL line 17 */
  EXTI_ClearITPendingBit(EXTI_Line17);
  
}

void EXTI15_10_IRQHandler(void) //resets date and time OR displays last two button presses
{ 

    if (EXTI_GetITStatus(EXTI_Line10) != RESET) { //if not reset...

			//Should be done with enum or switch statement but I was rushing..
			
			if (state ==1) {
				Display_Date();
				month++; 
				if (month == 13) {month = 0;}				//reset month
				RTC_DateStructure.RTC_Month = month;
				RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
				Display_Date();
			}
			
			else if (state ==2) {
				Display_Date();
				date++; 
				if (date == 32) {date = 0;}							//reset day
				RTC_DateStructure.RTC_Date = date;
				RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure); 
				Display_Date();
			}
			
			else if (state ==3) {
				Display_Date();
				year++;														//reset year
				if (year == 99) {year = 0;}
				RTC_DateStructure.RTC_Year = year;
				RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure);
				Display_Date();
			}
			
			else if (state ==4) {
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE);
				RCC_BackupResetCmd(DISABLE);
				RCC_LSICmd(DISABLE);
				LCD_DisplayString(0, 0, "                      "); //reset hours
				hours++; 
				if (hours == 25) {hours = 0;}
				//RTC_TimeStruct.RTC_Hours = hours;
				//RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct); 
				Display_Time();
			}
			
			else if (state ==5) {
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE); //safety
				LCD_DisplayString(0, 0, "                      ");  //reset minutes
				minutes++; 
				if (minutes == 61) {minutes = 0;}
				//RTC_TimeStruct.RTC_Minutes = minutes;
				Display_Time();
			}
			
			else if (state ==6) {
				RTC_AlarmCmd(RTC_Alarm_A, DISABLE); //safety
				LCD_DisplayString(0, 0, "                      ");  //reset seconds
				seconds++; 
				if (seconds == 61) {seconds = 0;}
				//RTC_TimeStruct.RTC_Seconds = seconds;
				//RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct); 
				Display_Time();
			}
			else {}
			
      EXTI_ClearITPendingBit(EXTI_Line10);
			}
		
		if (EXTI_GetITStatus(EXTI_Line12) != RESET) { //last two button presses..
        LCD_DisplayInt(8, 5, val_buff_1);       
				LCD_DisplayInt(7, 5, val_buff_2);
        EXTI_ClearITPendingBit(EXTI_Line12); //reset flag to zero
			}
}

void Display_Time(void)
{
	line = 0;
	if (hours <=9){
	column = 2; LCD_DisplayInt(line, 1, 0);}
	else {column = 1;}
	LCD_DisplayInt(line, column, hours);
	LCD_DisplayString(line, 3, ":");
				
	if (minutes <=9){
	column = 5; LCD_DisplayInt(line, 4, 0);}
	else {column = 4;}
	LCD_DisplayInt(line, column, minutes);
	LCD_DisplayString(line, 6, ":");
				
	if (seconds <=9){
	column = 8; LCD_DisplayInt(line, 7, 0);}
	else {column = 7;}
	LCD_DisplayInt(line, column, seconds);
}
