#include "main.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#define COLUMN(x) ((x) * (((sFONT *)LCD_GetFont())->Width)) //have to define this to use LCD String function

//memory location to write to in the device
__IO uint16_t memLocation = 0x000A; //pick any location within range

__IO uint8_t UBPressed = 0; //include this for User Button

uint8_t Tx1_Buffer = 0;
uint8_t Rx1_Buffer = 0;
uint16_t NumDataRead = 1;

RTC_InitTypeDef RTC_InitStructure;
RTC_TimeTypeDef  RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStructure;

//-------------------------------------------------------------------------------------------added these
void RTC_Config(void);
void display_int (uint16_t line, int num);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void RTC_AlarmConfig(void);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void Ext_PushButton_Interrupt10(void);
void Ext_PushButton_Interrupt12(void);
void Ext_PushButton_Interrupt2(void);
void Display_Date(void);
void store_EEPROM(int t);

extern int pressed;
uint16_t line;
int column =0;
int alarm = 0;
extern int a;
int time = 0;
int val_buff_1;
int val_buff_2;

extern int setdate;
//---------------------------------------------------------------------------------------------------------

int main(void){ //----------------------------------------------------------------------main function
	
	sEE_Init();
	//configure push-button interrupts
	PB_Config();
	
	 /* LCD initiatization */
  LCD_Init();
  
  /* LCD Layer initiatization */
  LCD_LayerInit();
    
  /* Enable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD foreground layer */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
	
	
	LCD_DisplayStringLine(LINE(line),  (uint8_t *) "Ready");
	
	sEE_WriteBuffer(&Tx1_Buffer, memLocation,1); 

  /* Wait for EEPROM standby state */
  sEE_WaitEepromStandbyState();  
 
  
	LCD_DisplayStringLine(LINE(line),  (uint8_t *) "Reading...");
  /* Read from I2C EEPROM from memLocation */
  sEE_ReadBuffer(&Rx1_Buffer, memLocation, (uint16_t *)(&NumDataRead)); 
	line++;
	
	
	LCD_DisplayStringLine(LINE(line),  (uint8_t *) "Comparing...");  
	line++;
	
	
	if(Tx1_Buffer== Rx1_Buffer){
		LCD_DisplayStringLine(LINE(line),  (uint8_t *) "Success!");  
	}else{
		LCD_DisplayStringLine(LINE(line),  (uint8_t *) "Mismatch!"); 
	}
	
	
	RTC_Config();
	
	
	RTC_AlarmConfig(); //alarm config
	
	LCD_Clear(LCD_COLOR_WHITE); //clears LCD initially

	
//------------------------------------------------------------------------------------------
  Ext_PushButton_Interrupt12();
	Ext_PushButton_Interrupt10();
	Ext_PushButton_Interrupt2();
	
//------------------------------------------------------------------------------------------------------------------------------main loop
	while(1){
		if (a == 1){store_EEPROM(time); a=0;} //flag
		
		if (alarm==1) //----------------------------------------------------------------------------Alarm that updates clock
		{
			line = 0;
			RTC_WaitForSynchro();
			RTC_GetTime(RTC_Format_BIN, &RTC_TimeStruct);
			
			if (RTC_TimeStruct.RTC_Hours <=9){
			column = 2; LCD_DisplayInt(line, 1, 0);}
			else {column = 1;}
			LCD_DisplayInt(line, column, RTC_TimeStruct.RTC_Hours);
			LCD_DisplayString(line, 3, ":");
			
			if (RTC_TimeStruct.RTC_Minutes <=9){
			column = 5; LCD_DisplayInt(line, 4, 0);}
			else {column = 4;}
			LCD_DisplayInt(line, column, RTC_TimeStruct.RTC_Minutes);
			LCD_DisplayString(line, 6, ":");
			
			if (RTC_TimeStruct.RTC_Seconds <=9){
			column = 8; LCD_DisplayInt(line, 7, 0);}
			else {column = 7;}
			LCD_DisplayInt(line, column, RTC_TimeStruct.RTC_Seconds);
			alarm = 0;
		}

	}
}

//---------------------------------------------------Userbutton
void PB_Config(void)
{
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);
}
//-------------------------------------------------------------clock config
void RTC_Config(void)
{
  // Enable the PWR clock 
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  // Allow access to RTC 
  PWR_BackupAccessCmd(ENABLE);

  /* Reset RTC Domain */
  RCC_BackupResetCmd(ENABLE);
  RCC_BackupResetCmd(DISABLE);

  // Enable the LSI OSC 
  RCC_LSICmd(ENABLE);
	

  // Wait till LSI is ready  
  while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
  {
  }

	  // Enable the RTC Clock 
	RCC_RTCCLKCmd(ENABLE);
	
  // Select the RTC Clock Source 
  RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
	
	// Wait for RTC APB registers synchronisation 
  RTC_WaitForSynchro(); 

  // RTC time base = LSI / ((AsynchPrediv+1) * (SynchPrediv+1)) = 1 Hz
 RTC_InitStructure.RTC_AsynchPrediv = 127;
 RTC_InitStructure.RTC_SynchPrediv = 255;
 RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
 RTC_Init(&RTC_InitStructure);
	
 //Set the Time
 RTC_TimeStruct.RTC_Hours = 0x00;
 RTC_TimeStruct.RTC_Minutes = 0x00;
 RTC_TimeStruct.RTC_Seconds = 0x00;
 RTC_SetTime(RTC_Format_BCD, &RTC_TimeStruct);

 // Set the Date
 RTC_DateStructure.RTC_Month = RTC_Month_November;
 RTC_DateStructure.RTC_WeekDay = RTC_Weekday_Monday;
 RTC_DateStructure.RTC_Date = 02;
 RTC_DateStructure.RTC_Year = 15;
 RTC_SetDate(RTC_Format_BCD, &RTC_DateStructure); 

}

//--------------------------------------------------------------------------------------------------------------------------------------------------LCD display functions

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		while (*ptr!=NULL)
    {
				LCD_DisplayChar(LINE(LineNumber), COLUMN(ColumnNumber), *ptr);
				ColumnNumber++;
			 //to avoid wrapping on the same line and replacing chars 
				if (ColumnNumber*(((sFONT *)LCD_GetFont())->Width)>=LCD_PIXEL_WIDTH ){
					ColumnNumber=0;
					LineNumber++;
				}
					
				ptr++;
		}
}
//----------------------------------------------------------
void Display_Date(void)
{
	line = 2;
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);//store time to eprom
	if (RTC_DateStructure.RTC_Month <=9){
	column = 2; LCD_DisplayInt(line, 1, 0);}
	else {column = 1;}
	LCD_DisplayInt(line, column, RTC_DateStructure.RTC_Month);
	LCD_DisplayString(line, 3, "/");
				
	if (RTC_DateStructure.RTC_Date <=9){
	column = 5; LCD_DisplayInt(line, 4, 0);}
	else {column = 4;}
	LCD_DisplayInt(line, column, RTC_DateStructure.RTC_Date);
	LCD_DisplayString(line, 6, "/");
				
	if (RTC_DateStructure.RTC_Year <=9){
	column = 8; LCD_DisplayInt(line, 7, 0);}
	else {column = 7;}
	LCD_DisplayInt(line, column, RTC_DateStructure.RTC_Year);
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------Configure Alarm

void RTC_AlarmConfig(void)
{
  EXTI_InitTypeDef EXTI_InitStructure;
  RTC_AlarmTypeDef RTC_AlarmStructure;
  NVIC_InitTypeDef NVIC_InitStructure;
  

  EXTI_ClearITPendingBit(EXTI_Line17);
  EXTI_InitStructure.EXTI_Line = EXTI_Line17;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);
  

  NVIC_InitStructure.NVIC_IRQChannel = RTC_Alarm_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x01; //setting this at 1 ensures it won't interrupt when
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02; //the User button is in the while loop
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
 

  RTC_AlarmStructure.RTC_AlarmMask = RTC_AlarmMask_All;
  RTC_SetAlarm(RTC_Format_BCD, RTC_Alarm_A, &RTC_AlarmStructure);
  
  // Set AlarmA subseconds and enable SubSec Alarm : generate 8 interripts per Second 
  RTC_AlarmSubSecondConfig(RTC_Alarm_A, 0xFF, RTC_AlarmSubSecondMask_SS14_5);

  RTC_ITConfig(RTC_IT_ALRA, ENABLE);
	RTC_AlarmCmd(RTC_Alarm_A, ENABLE);
}
//------------------------------------------------------------------------------------------------------------------------------------------------------------Store time to internal EEProm
void store_EEPROM(int t)
{
	int times = t;
	Tx1_Buffer = times;
	sEE_WriteBuffer(&Tx1_Buffer,memLocation,1);
	sEE_WaitEepromStandbyState();
	sEE_ReadBuffer(&Rx1_Buffer, (memLocation), (uint16_t *)1);
	val_buff_1 = Rx1_Buffer;
	sEE_ReadBuffer(&Rx1_Buffer, (memLocation), (uint16_t *)1);
	val_buff_2 = Rx1_Buffer;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------External Pushbutton 1 with interrupts

void Ext_PushButton_Interrupt10(void) { //change to PD2
			
		//GPIO - general purpose input output
		//EXTI - external interrupt
		//NVIC - nested vector interrupt control 

    GPIO_InitTypeDef GPIO_InitStruct;    //constructors
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct; 
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //Reset Clock Control (RCC)
    
    
    //setup pin 12 as input with internal pullup resistor and I/O speed as 50 MHz
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
  
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource10); //configures interrupt source as pin_12
    

    EXTI_InitStruct.EXTI_Line = EXTI_Line10; //used for pin 12 since some of the other pins are unavailable
 
    EXTI_InitStruct.EXTI_LineCmd = ENABLE; 

    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
   
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising; //sets interrupt to rising edge (so button pressed)

    EXTI_Init(&EXTI_InitStruct); //creates handler

    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn; //for pins 10 - 15 you use EXTI15_10 
   
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00; //set max priority 

    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x02;

    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //turn on handler

    NVIC_Init(&NVIC_InitStruct); //creates intrrupt
}
//---------------------------------------------------------------------------------------------------------------------------------------------External Pushbutton 2 with interrupts
void Ext_PushButton_Interrupt12(void) {
			
		//GPIO - general purpose input output
		//EXTI - external interrupt
		//NVIC - nested vector interrupt control 

    GPIO_InitTypeDef GPIO_InitStruct;    //constructors
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct; 
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //Reset Clock Control (RCC)
    
    
    //setup pin 12 as input with internal pullup resistor and I/O speed as 50 MHz
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
  
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOB, EXTI_PinSource12); //configures interrupt source as pin_12
    

    EXTI_InitStruct.EXTI_Line = EXTI_Line12; //used for pin 12 since some of the other pins are unavailable
 
    EXTI_InitStruct.EXTI_LineCmd = ENABLE; 

    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
   
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising; //sets interrupt to rising edge (so button pressed)

    EXTI_Init(&EXTI_InitStruct); //creates handler

    NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn; //for pins 10 - 15 you use EXTI15_10 
   
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00; //set max priority 

    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x02;

    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //turn on handler

    NVIC_Init(&NVIC_InitStruct); //creates intrrupt
}

void Ext_PushButton_Interrupt2(void)
{
		//GPIO - general purpose input output
		//EXTI - external interrupt
		//NVIC - nested vector interrupt control 

    GPIO_InitTypeDef GPIO_InitStruct;    //constructors
    EXTI_InitTypeDef EXTI_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct; 
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //Reset Clock Control (RCC)
    
    
    //setup pin 12 as input with internal pullup resistor and I/O speed as 50 MHz
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIO_InitStruct);
    
  
    SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource2); //configures interrupt source as pin_2
    

    EXTI_InitStruct.EXTI_Line = EXTI_Line2; //used for pin 12 since some of the other pins are unavailable
 
    EXTI_InitStruct.EXTI_LineCmd = ENABLE; 

    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
   
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising; //sets interrupt to rising edge (so button pressed)

    EXTI_Init(&EXTI_InitStruct); //creates handler

    NVIC_InitStruct.NVIC_IRQChannel = EXTI2_IRQn; //for pins 10 - 15 you use EXTI15_10 
   
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0x00; //set max priority 

    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x00;

    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //turn on handler

    NVIC_Init(&NVIC_InitStruct); //creates intrrupt
}
