/*
The stepper motor used is 26M048B1B (Bi-polar from Thomson Airpax Mechatronics with 48 step drive)
Note: Bi-polar motors use negative and positive voltage and therefore have more torque

Rotation time = 40 seconds
Full step frequency = 1.2 Hz
Half step frequency = 2.4 Hz

The stepper motor has four connecting wires: A,A',B,B'. The simplest way to achieve stepping is to
hardcode the sequential combination into a list and iterate through the list. This would allow for
both full-step and half-step modes as well as directional control.

The 'list' would look like: [AB, B, A'B, A', A'B', B', B'A, A]. Iterating through with an each element
(changing index by 1) would correspond to half-stepping while iterating through every other element (changing
the index by 2) would constitute a full-stepping motion. To control the motor's direction, one would simply
loop through the list 'forwards' or 'backwards' by incrementing or decrementing the index. 

In our case, the correspondence between list elements and GPIO pins is as follows:
A:  Pin_2
A': Pin_4
B:  Pin_3
B': Pin_5

*Note: with four outputs and four inputs I do not want to confuse interrupts.
			 For this reason, I have decided to use TIM3 interrupt flag to control 
			 the outputs, User Button interrupts to control Mode, an external interrupt
			 button to control Direction, and two polled buttons to control speed.
*/
#include "main.h"
#define COLUMN(x) ((x) * (((sFONT *)LCD_GetFont())->Width)) 

#define Forward 0				//variables
#define Reverse 1
#define HalfStep 0
#define FullStep 1

typedef int Mode;				//Modes
typedef int Direction;

int mode = HalfStep;
int direction = Forward;
extern int flag; 
int step_index = 1;  //goes from 1 - 8 inclusive 
int modulo;

void Step_Control(Direction dir, Mode step_mode); 
void Faster (void);
void Slower(void);
void Move(int step);
void TIM3_Config(void); 
void TIM3_OCConfig(void);
void external_PB (void);


TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
TIM_OCInitTypeDef  TIM_OCInitStructure;

__IO uint16_t CCR1_Val0 = 3750; //9 KHz / 3750 = 2.4 Hz flag
__IO uint16_t CCR1_Val1 = 7500; //9 KHz / 7500 = 1.2 Hz flag
uint16_t PrescalerValue = 0;

char lcd_buffer[14];    // LCD display buffer

void GPIOE_Config(void);
void GPIOD_Config(void);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);





int main(void){

	GPIOE_Config();
	GPIOD_Config();
	external_PB();
	STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_EXTI); //enables interrupt from push button
	
	STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4);
	
	//external_PB();
	LCD_Init();
  LCD_LayerInit();
  LTDC_Cmd(ENABLE);
  LCD_SetLayer(LCD_FOREGROUND_LAYER);
	LCD_Clear(LCD_COLOR_WHITE); 
	
	PrescalerValue = ((SystemCoreClock / 2) / 9000) - 1;
  TIM3_Config();
	TIM3_OCConfig();
  TIM_ITConfig(TIM3, TIM_IT_CC1, ENABLE);
  TIM_Cmd(TIM3, ENABLE);
	
	while(1) 
	{
		Step_Control(direction, mode); //if (GPIO_ReadInputDataBit(GPIOD,	GPIO_Pin_10) == 1) {Faster();}; if (GPIO_ReadInputDataBit(GPIOD,	GPIO_Pin_12) == 1) {Slower();};	
	}
}

void Step_Control(Direction dir, Mode step_mode)
{	
	Direction spin = dir; //spin is direction of motor spin
	Mode step = step_mode; //step is mode of opperation
	
	if (spin == Forward)
	{	
		LCD_DisplayString(7, 5, "Forward");
		LCD_DisplayInt(9, 5, step_index);
		if (step == HalfStep) 
		{
			LCD_DisplayString(2, 5, "Half-Step"); 
			TIM_OCInitStructure.TIM_Pulse=CCR1_Val0; 
			TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
			TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
			TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;	
			TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
			TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
			Move(step_index);
		  step_index = step_index + 1;
			if (step_index > 8) {step_index = 1;}
		}        
		else if (step == FullStep)
		{
			LCD_DisplayString(2, 5, "Full-Step"); 
			TIM_OCInitStructure.TIM_Pulse=CCR1_Val1;
			TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
			TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
			TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;	
			TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
			TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
			Move(step_index);
			modulo = step_index%2;
			if(modulo == 0) {step_index = step_index + 2;}
			else if(modulo == 1) {step_index = step_index +1;}
			if (step_index > 8) {step_index = 2;}
		}		 
	}
	else if (spin == Reverse)
	{	
		LCD_DisplayString(7, 5, "Reverse");
		LCD_DisplayInt(9, 5, step_index);
		if (step == HalfStep) 
		{
			LCD_DisplayString(2, 5, "Half-Step");
			TIM_OCInitStructure.TIM_Pulse=CCR1_Val0;
			TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
			TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
			TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;	
			TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
			TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
			Move(step_index);
		  step_index = step_index - 1;
			if (step_index < 1) {step_index = 8;}
		}        
		else if (step == FullStep)
		{
			LCD_DisplayString(2, 5, "Full-Step"); 
			TIM_OCInitStructure.TIM_Pulse=CCR1_Val1;
			TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
			TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
			TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;	
			TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
			TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
			Move(step_index);
			modulo = step_index%2;
			if(modulo == 1) {step_index = step_index - 1;}
			else if(modulo == 0) {step_index = step_index -2;}
			if (step_index < 1) {step_index = 8;}
		}		 
	}
}

void Move(int step)
{
	int index = step;
	while(1) {if(flag == 1) {break;}} //blocks until TIM3 flag is set and keeps the thing running slowly
	if (index == 1)  //AB
	{
		GPIOE->BSRRL = GPIO_Pin_2;  //set high A
		GPIOE->BSRRL = GPIO_Pin_3; //set high B
		GPIOE->BSRRH = GPIO_Pin_4; //set low  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}
	if (index == 2)  //B
	{
		GPIOE->BSRRH = GPIO_Pin_2;  //set low A
		GPIOE->BSRRL = GPIO_Pin_3; //set high B
		GPIOE->BSRRH = GPIO_Pin_4; //set low  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}
	if (index == 3)  //BA'
	{
		GPIOE->BSRRH = GPIO_Pin_2;  //set low A
		GPIOE->BSRRL = GPIO_Pin_3; //set high B
		GPIOE->BSRRL = GPIO_Pin_4; //set high  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}
	if (index == 4)  //A'
	{
		GPIOE->BSRRH = GPIO_Pin_2;  //set low A
		GPIOE->BSRRH = GPIO_Pin_3; //set low B
		GPIOE->BSRRL = GPIO_Pin_4; //set high  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}
	if (index == 5)  //A'B'
	{
		GPIOE->BSRRH = GPIO_Pin_2;  //set low A
		GPIOE->BSRRH = GPIO_Pin_3; //set low B
		GPIOE->BSRRL = GPIO_Pin_4; //set high  A'
		GPIOE->BSRRL = GPIO_Pin_5; //set high  B'
	}
	if (index == 6)  //B'
	{
		GPIOE->BSRRH = GPIO_Pin_2;  //set low A
		GPIOE->BSRRH = GPIO_Pin_3; //set low B
		GPIOE->BSRRH = GPIO_Pin_4; //set low  A'
		GPIOE->BSRRL = GPIO_Pin_5; //set high  B'
	}
	if (index == 7)  //B'A
	{
		GPIOE->BSRRL = GPIO_Pin_2;  //set high A
		GPIOE->BSRRH = GPIO_Pin_3; //set low B
		GPIOE->BSRRL = GPIO_Pin_4; //set high  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}
	if (index == 8)  //A
	{
		GPIOE->BSRRL = GPIO_Pin_2;  //set high A
		GPIOE->BSRRH = GPIO_Pin_3; //set low B
		GPIOE->BSRRH = GPIO_Pin_4; //set low  A'
		GPIOE->BSRRH = GPIO_Pin_5; //set low  B'
	}	
	flag = 0;
}

void Faster (void)
{
	CCR1_Val0 = CCR1_Val0 - 50;
	CCR1_Val1 = CCR1_Val1 - 50;
}

void Slower(void)
{
	CCR1_Val0 = CCR1_Val0 + 50;
	CCR1_Val1 = CCR1_Val1 + 50;
}

void GPIOE_Config(void) //Sets outputs
{
	GPIO_InitTypeDef GPIO_InitStruct;    
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);	
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_4|GPIO_Pin_5;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
  GPIO_Init(GPIOE, &GPIO_InitStruct);
}

void GPIOD_Config(void) //Sets outputs
{
	GPIO_InitTypeDef GPIO_InitStruct;    
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);	
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStruct.GPIO_Pin = GPIO_Pin_12;
  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOD, &GPIO_InitStruct);
}

void TIM3_Config(void)
	{
		NVIC_InitTypeDef NVIC_InitStructure;
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
		NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0X01; //low priority
		NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01;
		NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
		NVIC_Init(&NVIC_InitStructure);	
		TIM_TimeBaseStructure.TIM_Period=65535; 
		TIM_TimeBaseStructure.TIM_Prescaler=PrescalerValue;   
		TIM_TimeBaseStructure.TIM_ClockDivision=0;
		TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;	
		TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
  }
	
void TIM3_OCConfig(void) {
	TIM_OCInitTypeDef TIM_OCInitStructure; 
	TIM_OCInitStructure.TIM_OCMode=TIM_OCMode_Timing;
	TIM_OCInitStructure.TIM_OutputState=TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse=CCR1_Val1;										 
	TIM_OCInitStructure.TIM_OCPolarity=TIM_OCPolarity_High;	
	TIM_OC1Init(TIM3, &TIM_OCInitStructure);	
	TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Disable);
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

void external_PB(void) 
{
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
        if (direction == Forward) {direction = Reverse;}
				else if (direction == Reverse) {direction = Forward;}
        EXTI_ClearITPendingBit(EXTI_Line12); //reset flag to zero
    }
}
