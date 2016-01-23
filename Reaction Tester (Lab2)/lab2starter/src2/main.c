/////////////////////*******************__________________Reaction Tester_______________******************//////////////////////////////

/* Includes ------------------------------------------------------------------*/
#include "main.h" 
int randnum = 0; 
extern int counter; //this is used for the pause function when the LED's turn off
extern long timer; //made long in case it overflows. This measures user's time
int pause = 1;	//intializes pause as first state
int best_time;
int time = 0;//unnecessary variable but I use it to ensure that timer doesn't increment when it shouldn't
int initial = 1; //this sets intial state of best_time
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)LCD_GetFont())->Width))    //see font.h, for defining LINE(X)




/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;
__IO uint16_t CCR1_Val3 = 50000; //500 KHz / 50,000 = 10 Hz flag
__IO uint16_t CCR1_Val2 = 500; //500KHz / 500 = 1 KHz flag
uint16_t PrescalerValue = 0;
__IO uint8_t UBPressed = 0;

uint16_t line;

char lcd_buffer[14];    // LCD display buffer


/* Virtual address defined by the user: 0xFFFF value is prohibited
 * This global variable stores the EEPROM addresses for NB_OF_VAR(=3) variables
 * where the user can write to. To increase the number of variables written to EEPROM
 * modify this variable and NB_OF_VAR in eeprom.h
 */
uint16_t VirtAddVarTab[NB_OF_VAR] = {0x5555, 0x6666, 0x7777};


/* Private function prototypes -----------------------------------------------*/
void PB_Config(void);
void LED_Config(void);
void TIM3_Config(void); 
void TIM3_OCConfig(void);
void TIM2_Config(void); //Yes, I am using two timers. Is it necessary? No. Is it convenient? Yes.
void TIM2_OCConfig(void);

void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);
void RNG_Config(void);

/* Private functions ---------------------------------------------------------*/
void Pause_Random(int random_time); //pauses LED's for random amount of time
void Ext_PushButton_Interrupt(void); //enables external push button
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{
  /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file
     */
	
//initiate user button
  //PB_Config();
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);

	//initiate LEDs and turn them on
  LED_Config();	
	
 

  /* -----------------------------------------------------------------------
    TIM3 Configuration: Output Compare Timing Mode:
    
    In this example TIM3 input clock (TIM3CLK) is set to 2 * APB1 clock (PCLK1), 
    since APB1 prescaler is different from 1.   
      TIM3CLK = 2 * PCLK1  
      PCLK1 = HCLK / 4 
      => TIM3CLK = HCLK / 2 = SystemCoreClock /2
          
    To get TIM3 counter clock at 50 MHz, the prescaler is computed as follows:
       Prescaler = (TIM3CLK / TIM3 counter clock) - 1
       Prescaler = ((SystemCoreClock /2) /0.5 MHz) - 1
                                              
    CC1 update rate = TIM3 counter clock / CCR1_Val = 10.0 Hz
    ==> Toggling frequency = 5 Hz

    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.    
		 ----------------------------------------------------------------------- */ 	
	
	//=======================Configure and init Timer======================
  /* Compute the prescaler value */
  PrescalerValue = (uint16_t) ((SystemCoreClock / 2) / 500000) - 1; //configures clock speed at 500 KHz. Both Tim2 and Tim3 use the same prescsaler and therefore run at the same speed.

 /* TIM Configuration */
  TIM3_Config();
	TIM2_Config();

	// configure the output compare
	TIM3_OCConfig();
	TIM2_OCConfig();

  /* TIM Interrupts enable */
  TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
	TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE);
	
  /* TIM3 enable counter */
  TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM2, ENABLE); 
	
//======================================configure and init LCD  ======================	
	 /* LCD initiatization */
  LCD_Init();
  
  /* LCD Layer initiatization */
  LCD_LayerInit();
    
  /* Enable the LTDC */
  LTDC_Cmd(ENABLE);
  
  /* Set LCD foreground layer */
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
	
//================EEPROM init====================================

/* Unlock the Flash Program Erase controller */
		FLASH_Unlock();
		/* EEPROM Init */
		EE_Init();

//============ Set up for random number generation==============
	RNG_Config();
	Ext_PushButton_Interrupt(); //configures external push button

	//with the default font, LCD can display  12 lines of chars, they are LINE(0), LINE(1)...LINE(11) 
	//with the default font, LCD can display  15 columns, they are COLUMN(0)....COLUMN(14)


		LCD_Clear(LCD_COLOR_WHITE); //change the background colour of LCD 
			
		//Display a string in one line, on the first line (line=0)
		LCD_DisplayString(0, 2, (uint8_t *) "Best: ");  //the line will not wrap
		
  while (1){ 
		
			if (UBPressed==1) { //press user button
					if (pause==1){	//pause mode
						randnum = ((RNG_GetRandomNumber()%2000)+1000); //generates a random number between 1000 and 3000
						Pause_Random(randnum); //see below function to see how the pause is implemented
					}
					else { //measure time mode
						TIM_ITConfig(TIM2, TIM_IT_CC1, DISABLE); //turns off timer 2
						TIM_Cmd(TIM2, DISABLE);
						time = timer; //gets user's time
						if (initial == 1) { //sets initial best_time to first time 
						best_time = time; 
						initial = 0;
						LCD_DisplayInt((uint16_t) 0, (uint16_t) 7, best_time);
						} 
						LCD_DisplayString(2, 1, (uint8_t *) "Time: "); //print time
						LCD_DisplayString(2, 7, (uint8_t *) "                 "); //clears line
						LCD_DisplayInt((uint16_t) 2, (uint16_t) 7, time); //displays user's time
						LCD_DisplayString(2, 11, (uint8_t *) "ms"); //print ms
						if (time > 10 && time < best_time) { //set new best time
							best_time = time;
							LCD_DisplayString(0, 7, (uint8_t *) "          "); //clears line
							LCD_DisplayInt((uint16_t) 0, (uint16_t) 7, best_time);
						}
						TIM_ITConfig(TIM2, TIM_IT_CC1, ENABLE); //turns on timer 2
						TIM_Cmd(TIM2, ENABLE);
						pause = 1; //this makes it so that you can use the user button to repeat the cycle in case you don't have an external push button
					}
				UBPressed=0;				
			}

	}
}

/**
  * @brief  Configure the TIM IRQ Handler.
  * @param  No
  * @retval None
  */
//----------------------------------------------------------------------This fuction keeps the LED's off for a random amount of time. Once that time is reached it turns them on and starts a timer-----
void Pause_Random(int random_time)   
{
	TIM_ITConfig(TIM3, TIM_IT_CC1, DISABLE); //turns off timer 3
	TIM_Cmd(TIM3, DISABLE);

	counter = 0; //resets counter
	while (counter < random_time){   //counter gets incremented in timer 2
		STM_EVAL_LEDOff(LED4); //keeps LED's off for random time
		STM_EVAL_LEDOff(LED3);
	}
	pause = 0; //@next button press the Pause_Random function will not execute
	timer = 0;		//starts timer variable
	TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE); //turns on timer 3 interrupt
	TIM_Cmd(TIM3, ENABLE);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


void PB_Config(void)
{
/* Initialize User_Button on STM32F4-Discovery
   * Normally one would need to initialize the EXTI interrupt
   * to handle the 'User' button, however the function already
   * does this.
   */
  STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);
}

void LED_Config(void)
{
 /* Initialize Leds mounted on STM32F429-Discovery board */
  STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4); 

  /* Turn on LED3, LED4 */
  STM_EVAL_LEDOn(LED3);
	STM_EVAL_LEDOn(LED4);
}


void TIM3_Config(void) 
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//since TIMER 3 is on APB1 bus, need to enale APB1 bus clock first
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	
	//====================================================
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0X00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	//================================================
	
	TIM_TimeBaseStructure.TIM_Period=65535; // need to be larger than CCR1_VAL, has no effect on the Output compare event.
	TIM_TimeBaseStructure.TIM_Prescaler=PrescalerValue;    //why all the example make this one equal 0, and then use 
					//function TIM_PrescalerConfig() to re-assign the prescaller value?
	TIM_TimeBaseStructure.TIM_ClockDivision=0;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;	
	 
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	//TIM_PrescalerConfig(TIM3, TIM3Prescaler, TIM_PSCReloadMode_Immediate);
}


void TIM3_OCConfig(void) {
	TIM_OCInitTypeDef TIM_OCInitStructure;
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=CCR1_Val3;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);
	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable); //if disabled, 
	//the TIMx_CCRx register can be updated at any time by software to control the output
	//waveform---from the reference manual
}	
	
//-----------------------------------------------------------------------------------------------------Configure Seperate Timer (TIM2) with different overflow rate than TIM3-----------------------------------
void TIM2_Config(void) //code copied from TIM3_Config provided above
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	//since TIMER 2 is on APB1 bus, need to enale APB1 bus clock first
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);

	//Enable TIM2 global interrupt ====does this part need to be done before TIM_BaseStructure set up?
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0X00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);	
	
	TIM_TimeBaseStructure.TIM_Period=65535; 
	TIM_TimeBaseStructure.TIM_Prescaler=PrescalerValue;     
	TIM_TimeBaseStructure.TIM_ClockDivision=0;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;	 
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
}

void TIM2_OCConfig(void) { //code copied from TIM3_OCConfig provided above
	TIM_OCInitTypeDef TIM_OCInitStructure;
	
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=CCR1_Val2;
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;
	TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Disable);
}
//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
/*void TIM_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

  NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x01;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

}*/


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

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}

void RNG_Config(void){
	//Enable RNG controller clock
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	
	//activate the RNG peripheral 
	RNG_Cmd(ENABLE);
	
	// to get a random number, need to continue steps: 3. Wait until the 32 bit Random number Generator 
	//contains a valid random data (using polling/interrupt mode). For more details, 
	//refer to Section 20.2.4: "Interrupt and flag management" module description.
	//4. Get the 32 bit Random number using RNG_GetRandomNumber() function
	//5. To get another 32 bit Random number, go to step 3.
}	

//-------------------------------------------------------------------------------------------------------------------------This is what happens whan an external push button on pin 12 is pressed--------
//majority of this code was learned from UserButton code in given file 

void Ext_PushButton_Interrupt(void) {
			
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

    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x01;

    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE; //turn on handler

    NVIC_Init(&NVIC_InitStruct); //creates intrrupt
}
//---------------------------------------------------------------------------------------------------------------------------------------------


void EXTI15_10_IRQHandler(void) 	//handler for pin 12
{ 

    if (EXTI_GetITStatus(EXTI_Line12) != RESET) { //if not reset...
        UBPressed = 1; 
        
        EXTI_ClearITPendingBit(EXTI_Line12); //reset flag to zero
    }
}




//-----------------------------------------------------------------------------------------------------------------------------------------------------------------


#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  while (1)
  {}
}
#endif
