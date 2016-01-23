#include "main.h"

#define BUFFERSIZE 1024
#define COLUMN(x) ((x) * (((sFONT *)LCD_GetFont())->Width))
//the following two addresses are useful when using the ADC and DAC in DMA mode

#define ADC_CDR_ADDR 				((uint32_t)(ADC_BASE + 0x08)) //ADC_CDR address as described in section 10.13.17 of reference manual
//ADC CDR is the common regular (not injected) data register for dual and triple modes
//also defined in stm32f4xx_adc.c as 
//#define CDR_ADDRESS 			((uint32_t)0x40012308) 
//which results in the same address

int ticks = 0; //84*counter
double value = 0;
uint16_t ADCConvertedValues[BUFFERSIZE]; //buffer for converted values
int offset = 0;
int dec_val = 0;

void ADC_Configuration(void);
void DAC_Config(void);
void PWM_Config(void);
void TIMER_INIT(void);
void PB_12_Config (void);
void PB_10_Config (void);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);

int main(void){
//-----------------------------------initialize	functions
	int i;
	int current1 = 0;
	int last1 = 0;
	int current2 = 0;
	int last2 = 0;
	int Temp = 20;
	int count_up = 0;
	int count_down = 0;
	
	DAC_Config();
	
	TIMER_INIT();

  LCD_Init();

  LCD_LayerInit();
    
  LTDC_Cmd(ENABLE);

  LCD_SetLayer(LCD_FOREGROUND_LAYER);

	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI);

	ADC_Configuration();
	
	LCD_Clear(LCD_COLOR_WHITE); //clear LCD
	//---------------------------------------------------------Display Temperature Setpoint
	PB_12_Config();	
	PB_10_Config();
  LCD_DisplayString(3, 5, "Setpoint");
	LCD_DisplayInt(3, 2, Temp);
  i = 0;
 //--------------------------------------------------------------------------While Loops
  while(1) // Don't want to exit
  {
    /* Start ADC1 Software Conversion */
    ADC_SoftwareStartConv(ADC1);
		
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
		//------------------------------------------------------- Get State of Buttons
    {
		GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12); 
		GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10);
//---------------------------------------------------------------------------------pin12pressed			
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_12) == 0) //use pullup;
		{
			current1 = 1;
		}
		else {current1 = 0; count_up = 0;}
//---------------------------------------------------------------------------------------pin10pressed
		if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_10) == 0) //use pullup;
		{
			current2 = 1;
		}
		else {current2 = 0; count_down = 0;}
//--------------------------------------------------------------pin12pressed --> counter++	
		if (last1 == 1)
		{
			if (current1 == 1)
			{
				count_up++;
			}
		}
//----------------------------------------------pin10pressed --> counter--
		if (last2 == 1)
		{
			if (current2 ==1)
			{
				count_down++;
			}
		}
//--------------------------------------------set Temp if counter == 1000
		if (count_up == 1000)
			{count_up = 0;
			Temp++;
			LCD_DisplayInt(3, 2, Temp);
			LCD_DisplayString(3, 5, "Setpoint");
			}
		if (count_down == 1000)
			{count_down = 0;
			Temp--;
			LCD_DisplayInt(3, 2, Temp);
			LCD_DisplayString(3, 5, "Setpoint");
			}
		last1 = current1;
		last2 = current2;	
		//-------------------------------------------------------- theremometer ADC

    ADCConvertedValues[i++] = ADC_GetConversionValue(ADC1);
		value = ADCConvertedValues[0];
		value = (value/4096);
		value = value*82; //3V * 33.3 degrees/volt
		dec_val = value;
		LCD_DisplayInt(1, 2, dec_val);
		LCD_DisplayString(1, 7, "degree");
    i %= BUFFERSIZE;
		
//-------------------------------------------------------------control fan speed (non-linear)
		if (dec_val == Temp) {offset = 0; PWM_Config();}
		else if (dec_val == Temp+1) {ticks = 5; PWM_Config();}
		else if (dec_val == Temp+2) {ticks = 10; PWM_Config();}
		else if (dec_val == Temp+3) {ticks = 15; PWM_Config();}
		else if (dec_val == Temp+4) {ticks = 20; PWM_Config();}
		else if (dec_val == Temp+5) {ticks = 25; PWM_Config();}
		else if (dec_val == Temp+6) {ticks = 30; PWM_Config();}
		else if (dec_val == Temp+7) {ticks = 35; PWM_Config();}
		else if (dec_val == Temp+8) {ticks = 40; PWM_Config();}
		else if (dec_val == Temp+9) {ticks = 45; PWM_Config();}
		else if (dec_val >= Temp+10) {ticks = 50; PWM_Config();}
		else if (dec_val < Temp) {offset = 1; PWM_Config();}
//---------------------------------------------------------------			
  }
 }
}	
 
void ADC_Configuration(void) //configure input for temperature sensor 
{
	GPIO_InitTypeDef GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
  ADC_InitTypeDef ADC_InitStructure;
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL ; //change to NOPULL
  GPIO_Init(GPIOC, &GPIO_InitStructure);

  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInit(&ADC_CommonInitStructure);
 
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
  ADC_InitStructure.ADC_ScanConvMode = DISABLE; // 1 Channel
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; // Conversions Triggered
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None; // Manual
  ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
  ADC_InitStructure.ADC_NbrOfConversion = 1;
  ADC_Init(ADC1, &ADC_InitStructure);
 
  ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 1, ADC_SampleTime_144Cycles); // PC1
 
  ADC_Cmd(ADC1, ENABLE);
}
 
void PB_12_Config (void)   //configures push button 12
{
	GPIO_InitTypeDef GPIO_InitStruct;    //constructors 
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //Reset Clock Control (RCC)    
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);	
}
void PB_10_Config (void)  //configures push button 10
{
	GPIO_InitTypeDef GPIO_InitStruct;    //constructors 
	
		RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);	
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE); //Reset Clock Control (RCC)    
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void DAC_Config(void) //cofigure output for PWM pin 12
{
	GPIO_InitTypeDef GPIO_InitStruct;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

    GPIO_PinAFConfig(GPIOD, GPIO_PinSource12, GPIO_AF_TIM4);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void PWM_Config(void)
	{
		TIM_OCInitTypeDef TIM_OCStruct;
		TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM2;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_Low;
	  if (offset == 0){TIM_OCStruct.TIM_Pulse = 4199 + (84*ticks);}  //intially 50% duty cycle
		else {TIM_OCStruct.TIM_Pulse = 0;} // 0% duty cycle (always off)
    TIM_OC1Init(TIM4, &TIM_OCStruct);
    TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);
  }

void TIMER_INIT(void)
{
	TIM_TimeBaseInitTypeDef TIM_BaseStruct;    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);

    TIM_BaseStruct.TIM_Prescaler = 0;
   
    TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_BaseStruct.TIM_Period = 8399; // 10kHz PWM  this value is found since 84MHz / 10KHz = 8400 which is 0 - 8399
    TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_BaseStruct.TIM_RepetitionCounter = 0;

    TIM_TimeBaseInit(TIM4, &TIM_BaseStruct);

    TIM_Cmd(TIM4, ENABLE);
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

void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		sprintf(lcd_buffer,"%d",Number);
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}
