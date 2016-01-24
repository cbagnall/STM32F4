
/* Includes ------------------------------------------------------------------*/
#include "main.h" 

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define COLUMN(x) ((x) * (((sFONT *)LCD_GetFont())->Width))    //see font.h, for defining LINE(X)
#define KEY_PRESSED     0x00
#define KEY_NOT_PRESSED 0x01

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
CAN_InitTypeDef        CAN_InitStructure;
CAN_FilterInitTypeDef  CAN_FilterInitStructure;
CanTxMsg TxMessage;
uint8_t TransmitMailbox=0; 

char lcd_buffer[14];    // LCD display buffer
CanRxMsg RxMessage;
extern __IO uint32_t msCount;


/* Private function prototypes -----------------------------------------------*/

void CAN_Config(void);

void NVIC_Config(void);
void Delay(uint32_t);

void LEDs_Init(void);


void LCD_Config(void);
void LCD_DisplayString(uint16_t LineNumber, uint16_t ColumnNumber, uint8_t *ptr);
void LCD_DisplayInt(uint16_t LineNumber, uint16_t ColumnNumber, int Number);
void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint);


/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int main(void)
{		
	
	if (SysTick_Config(SystemCoreClock/1000)) 
		{
				while(1);
		}

    NVIC_SetPriority(SysTick_IRQn,0); // set SysTick priority to the highest
		
		NVIC_Config(); //required to enable CAN1_RX0_IRQn interrupt and set its priority
		/* Initialize LEDs*/
		LEDs_Init();
		
	/* Configure user Push button key */
		STM_EVAL_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);   //or BUTTON_MODE_EXTI
		
		/* CAN configuration */
		CAN_Config();
		
		/*Configuration of LCD*/
		//LCD_Config();

		//after set up NVIC for CAN_RX_IRQn 
	  NVIC_SetPriority(CAN1_RX0_IRQn,2);  //Set CAN1 RX0_IRQn priority lower than SysTick's
		
		CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);
    STM_EVAL_LEDOn(LED3); // LED on after reset button pushed
		while (1)
		{   
			while(STM_EVAL_PBGetState(BUTTON_USER) != KEY_PRESSED){  // If user button pressed
				//Msg_Display(1, 1, (uint8_t*)"DataTx:", TxMessage.Data[0]); // Display the transmitted message data
				STM_EVAL_LEDOff(LED3);                           // LED off indicates message sent
				
				//Delay(1000);
				
				//Msg_Display(1, 1, (uint8_t*)"                            ", TxMessage.Data[0]); // Clear screen
				//LCD_DeInit();
				
				TransmitMailbox=CAN_Transmit(CAN1, &TxMessage);        // Transmit message
			
				Delay(1000);
				NVIC_SystemReset();
				
				if (CAN_TransmitStatus(CAN1, TransmitMailbox)==CANTXOK) { //that is: CAN_TxStatus_Ok, defined value as 0x01
					//NVIC_SystemReset();
									
				} else { //if Tx status is CANTXFAILED (0x00), or CDANTXPENDING(0x02), or CAN_NO_MB(CAN_TxStatus_NoMailBox, 0x04)) 	
					
							 
							CAN_CancelTransmit(CAN1, TransmitMailbox); //Cancel transmit, release transmit mailbox
							//NVIC_SystemReset();		//software reset the board in case LCD has been used.																							
				}	
				Delay(1500);
			}
	}
	
}



void NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn; //handler
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x00;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
}

void CAN_Config(void)
{
/*This function is is used to
	configure  GPIO ports including enabling of clock
	Initializing the CAN
	Initializing the CAN filters
	Enabling the FIFO pending interrupt */
	
	// NOTE: No GPIO ports or CAN bus is used for running in the Silent and Loopback mode, 
	// but required for normal operation
	
  // The Baudrate of CAN bus used in lab is 500k Bps 
	

  GPIO_InitTypeDef   GPIO_InitStructure;
	CAN_InitTypeDef CAN_InitStructure;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	
	/* Enable GPIOD clock */
	RCC_AHB1PeriphClockCmd(CAN_GPIO_CLK, ENABLE);
	
	/* Connect PD1 to CAN1_Tx pin */
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_CAN1);
	
	/* Connect PD0 to CAN1_Rx pin */
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource0, GPIO_AF_CAN1); 
	
	/* Configure CAN1_Rx(PD0) and CAN1_Tx(PD1) pins */
	GPIO_InitStructure.GPIO_Pin = CAN_RX_PIN | CAN_TX_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);


  /* CAN configuration ********************************************************/  
  /* Enable CAN clock */
  RCC_APB1PeriphClockCmd(CAN_CLK, ENABLE);
  
  /* CAN register init */
  CAN_DeInit(CAN1);

  /* CAN cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM = DISABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = ENABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
	CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;  //CAN_Mode_SilentLoopBack
 
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq; //synchronized jump width = 1 time quanta 
    
  // CAN Baudrate = 500 Bps
	// The bus the CAN is attached is of 45 Mhz. 
	//with prescaler 3 (this is the "real" prescaler, during init process, 2 will be written in register) while 1tq=2 clock cycle. (CAN clocked at 45  MHz for F429i board) 
	// so the baudrate should be 45/3/2/15   (15 is: 1+ 9forBS1 + 5forBS2)  =0.5 M Bps (500 K Bps) 
	
  CAN_InitStructure.CAN_BS1 = CAN_BS1_9tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS1_5tq;
  CAN_InitStructure.CAN_Prescaler = 3;
  CAN_Init(CAN1, &CAN_InitStructure);

  /* CAN filter init */
#ifdef  USE_CAN1
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
#else /* USE_CAN2 */
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
#endif  /* USE_CAN1 */
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
	
	CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;
	CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;

  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  
	CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  
  /* Transmit Structure preparation */
	TxMessage.StdId = GROUP_ID;
  TxMessage.ExtId = 0x00;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 1;
	TxMessage.Data[0] =(GROUP_ID & 0x0FF); //group id
	
	CAN_ITConfig(CAN1,CAN_IT_FMP0, ENABLE); //enables message pending interrupt on CAN1 FIFO 0  
 
}




void Msg_Display(int row, int col, uint8_t* text, uint8_t msg)
{
 	LCD_Config();
	
	LCD_DisplayString(row, col, (uint8_t *)text);
 
	LCD_DisplayInt(row , col+8, (int)msg);
	
	Delay(1000);
	
  //LCD_DeInit();
	//NVIC_SystemReset();   //once the LCD is used, the board must be reset to free the pins used by LCD display.
	                     //the CAN bus will be ruined if any STM32f429i-Disco board on the CAN bus is useing LCD display.
	
}


void LCD_Config(void) //after these steps, LCD can be used to display strings or integers...
{
	LCD_Init();
   
  LCD_LayerInit();

	// Enable the LTDC 
  LTDC_Cmd(ENABLE);
 
	LCD_SetLayer(LCD_FOREGROUND_LAYER);
	
	//with the default font, LCD can display  12 lines of chars, they are LINE(0), LINE(1)...LINE(11) 
	//with the default font, LCD can display  15 columns, they are COLUMN(0)....COLUMN(14)
		
	LCD_Clear(LCD_COLOR_WHITE);
}	

void Delay(uint32_t ms)
{
  msCount=0;
	while (msCount<ms) { }	
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

void LCD_DisplayFloat(uint16_t LineNumber, uint16_t ColumnNumber, float Number, int DigitAfterDecimalPoint)
{  
  //here the LineNumber and the ColumnNumber are NOT  pixel numbers!!!
		char lcd_buffer[15];
		
		sprintf(lcd_buffer,"%.*f",DigitAfterDecimalPoint, Number);  //6 digits after decimal point, this is also the default setting for Keil uVision 4.74 environment.
	
		LCD_DisplayString(LineNumber, ColumnNumber, (uint8_t *) lcd_buffer);
}


void LEDs_Init(void)
{
 /* Initialize Leds mounted on STM32F429-Discovery board */
  STM_EVAL_LEDInit(LED3);
	STM_EVAL_LEDInit(LED4); 
}

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
