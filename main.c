#include "STM32f10x.h"
#include "STM32f10x_gpio.h"
#include "STM32f10x_rcc.h"
#include "STM32f10x_bkp.h"
#include "STM32f10x_pwr.h"
#include "STM32f10x_rtc.h"
#include "gpio.h"
#include "STM32f10x_usart.h"
#include "STM32f10x_iwdg.h"
#include "STM32f10x_dbgmcu.h"
#include "FreeRTOS.h"
#include "task.h"
#include "dmd.h"
#include <string.h>
#include <stdio.h>
#include "dma.h"
#include "font.h"
#include "core_cm3.h"

PinHandle pin;
DmdHandle dmd;

//void SysTick_Handler(void)
//{
//	extern void DisplayScanner();
//	DisplayScanner();
////	//DMA_config(GetBaseAddress());
//}

#define BUFFER_SIZE 1024
uint8_t serial_buffer[BUFFER_SIZE];
char data_buffer[BUFFER_SIZE];
volatile uint8_t flag = 0;
uint32_t serial_index = 0;
float temp = 25.00;



void NVIC_SystemReset(void)
{
 __DSB();                                                     /* Ensure all outstanding memory accesses included
                                                                 buffered write are completed before reset */
 SCB->AIRCR  = ((0x5FA << SCB_AIRCR_VECTKEY_Pos)      |
                (SCB->AIRCR & SCB_AIRCR_PRIGROUP_Msk) |
                SCB_AIRCR_SYSRESETREQ_Msk);                   /* Keep priority group unchanged */
 __DSB();                                                     /* Ensure completion of memory access */
 while(1);                                                    /* wait until reset */
}
//void Default_Handler()
//{
//
//
//}

uint32_t DateInMonth(int month);
uint8_t get_crc(const unsigned char *buff, int buff_sz);

uint8_t HexToByte(uint8_t hex, uint8_t * buf)
{
	switch (hex) {
		case '0':
			*buf = 0;
			break;
		case '1':
			*buf = 1;
			break;
		case '2':
			*buf = 2;
			break;
		case '3':
			*buf = 3;
			break;

		case '4':
			*buf = 4;
			break;
		case '5':
			*buf = 5;
			break;
		case '6':
			*buf = 6;
			break;
		case '7':
			*buf = 7;
			break;
		case '8':
			*buf = 8;
			break;
		case '9':
			*buf = 9;
			break;
		case 'A':
		case 'a':
			*buf = 10;
			break;
		case 'B':
		case 'b':
			*buf = 11;
			break;
		case 'C':
		case 'c':
			*buf = 12;
			break;
		case 'D':
		case 'd':
			*buf = 13;
			break;
		case 'E':
		case 'e':
			*buf = 14;
			break;
		case 'F':
		case 'f':
			*buf = 15;
			break;
		default:
			return 0;
			break;
	}
	return 1;
}

typedef struct
{
	uint32_t sec;
	uint32_t minute;
	uint32_t hour;
	uint32_t date;
	uint32_t month;
	uint32_t year;
}Time;

volatile Time time;



void USART1_IRQHandler(void)
{
	if (USART_GetITStatus(USART1, USART_IT_RXNE) == 1) {
		uint8_t data = USART_ReceiveData(USART1);
		if (data == '$') {
			if (serial_index != 0) {

				uint8_t temp = 0;
				if (HexToByte(serial_buffer[serial_index-2], &temp)) {
					uint8_t crc =  temp<< 4;
					if (HexToByte(serial_buffer[serial_index-1], &temp)) {
						crc |= temp;

						uint8_t ccrc = get_crc(serial_buffer, serial_index - 2);

						if (crc == ccrc) {
							strcpy((char *)data_buffer, (const char *)serial_buffer);
							flag = 1;
						}
					}
				}



			}
			memset(serial_buffer, 0, BUFFER_SIZE);
			serial_index = 0;
		}
		else {
			serial_buffer[serial_index++] = data;
			if (serial_index >= BUFFER_SIZE) {
				memset(serial_buffer, 0, BUFFER_SIZE);
				serial_index = 0;
			}
		}
	}
}







uint8_t SplitString(char * str, char token, char ** string_list)
{
	char * head = str;
	char * iterator = str;
	uint8_t count = 0;
	uint8_t field_count = 1;
	for ( ; *iterator != 0; )
	{
		if (*iterator == token)
		{
			*iterator = 0;
			string_list[count++] = head;
			head = (iterator + 1);
			field_count++;
		}
		iterator++;
	}
	string_list[count++] = head;
	return field_count++;
}

uint8_t get_crc(const unsigned char *buff, int buff_sz)
{
	uint8_t chsum = 0;
	int it;

	for(it = 0; it < buff_sz; ++it)
	chsum ^= (uint8_t)buff[it];

	return chsum;
}

int IsLeapYear(uint32_t year)
{
	if (year % 400 == 0)
		return 1;
	else if (year % 100 == 0)
		return 0;
	else if (year % 4 == 0)
		return 1;
	else	return 0;
}


void Watchdog_Config(uint16_t timeout_ms) {
	#ifdef LSI_TIM_MEASURE
		/* Enable the LSI OSC */
		RCC_LSICmd(ENABLE);
		/* Wait till LSI is ready */
		while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
		{}
		/* TIM Configuration -------------------------------------------------------*/
		TIM5_ConfigForLSI();
		/* Wait until the TIM5 get 2 LSI edges */
		while(CaptureNumber != 2)
		{}
		/* Disable TIM5 CC4 Interrupt Request */
		TIM_ITConfig(TIM5, TIM_IT_CC4, DISABLE);
	#endif
	/* IWDG timeout equal to timeout ms (the timeout may varies due to LSI frequency dispersion) */
	/* Enable write access to IWDG_PR and IWDG_RLR registers */
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

	/* IWDG counter clock: LSI/32 */
	IWDG_SetPrescaler(IWDG_Prescaler_32);

	/* Set counter reload value to obtain 250ms IWDG TimeOut.
	Counter Reload Value e.g. = 250ms/IWDG counter clock period
                          = 250ms / (LSI/32)
                          = 0.25s / (LsiFreq/32)
                          = LsiFreq/(32 * 4)
                          = LsiFreq/128
	So set timeout_ms*LsiFreq/(128*250)=1.25*timeout_ms=timeout_ms*5/4
	As LsiFreq=40000 Hz
	*/
	IWDG_SetReload(timeout_ms*5/4);

	/* Reload IWDG counter */
	IWDG_ReloadCounter();

	/* Enable IWDG (the LSI oscillator will be enabled by hardware) */
	IWDG_Enable();
}

uint8_t Watchdog_Reset_Detect(void) {
	/* Check if the system has resumed from IWDG reset */
	if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) != RESET) {
		/* IWDGRST flag set */
 		/* Clear reset flags */
		RCC_ClearFlag();
		return 1;
  	}
	else {
		/* IWDGRST flag is not set */
		/* Turn off LED1 */
		return 0;
	}
}





uint32_t DateInMonth(int month)
{
	uint32_t date_in_month = 31;
	switch (month) {
		case 1:
		case 3:
		case 5:
		case 7:
		case 8:
		case 10:
		case 12:
			date_in_month = 31;
			break;
		case 2:
			date_in_month = 28;
			if (IsLeapYear(time.year) == 1) {
				date_in_month = 29;
			}
		case 4:
		case 6:
		case 9:
		case 11:
			date_in_month = 31;
			break;
		default:
			break;
	}
	return date_in_month;
}



void DemoTask(void * pvParameters)
{
	ClearScreen(dmd);
	drawString(dmd,0,32,"Md Mahmudul Hasan Sumon.", 20,GRAPHICS_NORMAL);
	FlushScreen(dmd);
	while(1)
	{
//		IWDG_ReloadCounter();
		vTaskDelay(10);
//		ClearScreen(dmd);
//		drawString(dmd,0,48,"MST Fahmida Haque Munny.", 20,GRAPHICS_NORMAL);
//		FlushScreen(dmd);
//		vTaskDelay(1000);
	}
}

volatile uint32_t r0;
volatile uint32_t r1;
volatile uint32_t r2;
volatile uint32_t r3;
volatile uint32_t r12;
volatile uint32_t lr; /* Link register. */
volatile uint32_t pc; /* Program counter. */
volatile uint32_t psr;/* Program status register. */

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
/* These are volatile to try and prevent the compiler/linker optimising them
away as the variables never actually get used.  If the debugger won't show the
values of the variables, make them global my moving their declaration outside
of this function. */


    r0 = pulFaultStackAddress[ 0 ];
    r1 = pulFaultStackAddress[ 1 ];
    r2 = pulFaultStackAddress[ 2 ];
    r3 = pulFaultStackAddress[ 3 ];

    r12 = pulFaultStackAddress[ 4 ];
    lr = pulFaultStackAddress[ 5 ];
    pc = pulFaultStackAddress[ 6 ];
    psr = pulFaultStackAddress[ 7 ];

    /* When the following line is hit, the variables contain the register values. */
    for( ;; );
}



void HardFault_Handler()
{
	__asm volatile
	    (
	        " tst lr, #4                                                \n"
	        " ite eq                                                    \n"
	        " mrseq r0, msp                                             \n"
	        " mrsne r0, psp                                             \n"
	        " ldr r1, [r0, #24]                                         \n"
	        " ldr r2, handler2_address_const                            \n"
	        " bx r2                                                     \n"
	        " handler2_address_const: .word prvGetRegistersFromStack    \n"
	    );
}

char company_name[] = "Welcome";
char company_title[] = "LIMITED";

const char * MonthNameList[] = {NULL, "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};

int main(void)
{
	DBGMCU_Config(DBGMCU_CR_DBG_TIM1_STOP|DBGMCU_CR_DBG_TIM3_STOP|DBGMCU_CR_DBG_IWDG_STOP, ENABLE);
	SystemCoreClockUpdate();
	Watchdog_Reset_Detect();
	Watchdog_Config(1000);
	dmd = InitDmdModule(3,2);
	ClearScreen(dmd);
	SelectFont(dmd, FindFont(12));
	int length = StringWidth(company_name);
	int x_cordinate = (DISPLAY_WIDE*32-length)/2;




//	drawString(dmd, 32, 0, company_name, 3, GRAPHICS_NORMAL);
	drawString(dmd, x_cordinate, 16, company_name, 7, GRAPHICS_NORMAL);
	length = StringWidth(company_title);
	x_cordinate = (DISPLAY_WIDE*32-length)/2;
	//drawString(dmd, x_cordinate, 32, company_title, 7, GRAPHICS_NORMAL);
	FlushScreen(dmd);
	InitDma();



	GPIO_InitTypeDef gpio_config;
	//gpio_config.GPIO_Mode = GPIO_Mode_AF_PP;
	//gpio_config.GPIO_Pin = GPIO_Pin_9;
	//gpio_config.GPIO_Speed = GPIO_Speed_50MHz;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	//GPIO_Init(GPIOA, &gpio_config);

	gpio_config.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	gpio_config.GPIO_Pin = GPIO_Pin_10;
	gpio_config.GPIO_Speed = GPIO_Speed_50MHz;


	GPIO_Init(GPIOA, &gpio_config);

	//GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);
	USART_InitTypeDef usart_config;
	usart_config.USART_BaudRate = 9600;
	usart_config.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	usart_config.USART_Mode = USART_Mode_Rx;
	usart_config.USART_Parity = USART_Parity_No;
	usart_config.USART_StopBits = USART_StopBits_1;
	usart_config.USART_WordLength = USART_WordLength_8b;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
	USART_Init(USART1, &usart_config);



	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn, 30);



	USART_Cmd(USART1, ENABLE);

	uint8_t stream[512];



	while(1)
	{
		IWDG_ReloadCounter();
		if (flag == 1) {
			flag = 0;

			memset(stream, 0, 512);

			int length = strlen(data_buffer);

			if (length % 2 != 0) {
				continue;
			}
			int i = 0, j = 0;
			uint8_t data = 0;
			while(length--)
			{
				if (HexToByte(data_buffer[j++], &data)) {
					stream[i] = data << 4;
				}
				if (HexToByte(data_buffer[j++], &data)) {
					stream[i] |= data;
				}
				i++;
				length--;

			}



			switch(stream[0])
			{
			case 1:
				{
					int line_count = stream[1];
					int line_start_position = 2;
					ClearScreen(dmd);
					int y_index = 0;
					int write_status = 1;
					for(int lne = 0; lne < line_count; lne++)
					{
						int font_size = stream[line_start_position++];
						int line_length = stream[line_start_position++];
						SelectFont(dmd, FindFont(font_size));
						char buf[30];
						memset(buf, 0, 30);
						if (line_length > 29) {
							strncpy(buf, (char *)&stream[line_start_position], 29);
							line_length = 29;
						} else {
							strncpy(buf, (char *)&stream[line_start_position], line_length);
						}
						int x_index = 0;

						if (strlen(buf) < line_length) {
							write_status = 0;
							break;
						}

						uint8_t len = StringWidth(buf);
						if(len >= DISPLAY_WIDE*32 )
						{
							x_index = 0;
						}
						else {
							x_index = (DISPLAY_WIDE*32-len)/2;
						}


						drawString(dmd, x_index, y_index, buf, line_length, GRAPHICS_NORMAL);
						line_start_position += line_length;
						y_index += (font_size)+ (FontHeight() * 15)/100;
					}
					if (write_status) {


						FlushScreen(dmd);
					}
					flag = 0;
				}
				break;
			default:
				break;
			}

			memset(data_buffer, 0, BUFFER_SIZE);
		}

	}

}
