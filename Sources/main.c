#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_usart.h"
#include "stm32f4xx_dma.h"
#include "esp8266.h"

// Prototypes
void GPIOInitialize(void);
void UART_Initialize(void);
void DMA_Initialize(void);

// Global vars
GPIO_InitTypeDef GPIOD_InitStruct;
GPIO_InitTypeDef GPIOC_InitStruct;
USART_InitTypeDef USART_InitStructure;
DMA_InitTypeDef  DMA_InitStructure;

int main(void)
{
	GPIOInitialize();
	UART_Initialize();
	DMA_Initialize();

	// Delay to be sure Esp8266 is online
	static int i = 0;
	for (i = 0; i < 10000000; ++i);

	// Check if ESP8266 is started
	while(!esp8266_isOnline());

	// Join AP
	esp8266_joinAp("UPC1248023", "CHNAJRDQ");

	// Set multiple connection mode
	esp8266_setMultipleConnectionMode(false);

	// Open connection
	esp8266_openConnection("startsmart.nl", "80");

	// Perform GET request
	esp8266_sendData("GET / HTTP/1.1\r\nHost: startsmart.nl\r\n\r\n");

	// Enable led
	GPIO_ToggleBits(GPIOC, GPIO_Pin_1);

	while (1) {
		char data[64];
		esp8266_readData(data);
	}
}

void GPIOInitialize(void) {
	/*** GPIOC initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIOC_InitStruct.GPIO_Pin = GPIO_Pin_1;
	GPIOC_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIOC_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIOC_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIOC_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIOC_InitStruct);

	/*** GPIOD initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //Enable clock for GPIOD

	/*** USART2 Tx on PD5 | Rx on PD6 ***/
	GPIOD_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6;
	GPIOD_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIOD_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIOD_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIOD_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOD, &GPIOD_InitStruct);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource5, GPIO_AF_USART2);//Connect PD5 to USART1_Tx
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource6, GPIO_AF_USART2);//Connect PD6 to USART1_Rx
}

void UART_Initialize(void) {
	/* Enable peripheral clock for USART2 */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

	/* USART2 configured as follow:
	* BaudRate 115200 baud
	* Word Length 8 Bits
	* 1 Stop Bit
	* No parity
	* Hardware flow control disabled
	* Receive and transmit enabled
	*/
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure); // USART configuration
	USART_Cmd(USART2, ENABLE); // Enable USART
}

void DMA_Initialize(void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	DMA_DeInit(DMA1_Stream5);

	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory; // Receive
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)receiveBuffer;
	DMA_InitStructure.DMA_BufferSize = (uint16_t)sizeof(receiveBuffer);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_Init(DMA1_Stream5, &DMA_InitStructure);

	/* Enable the USART Rx DMA request */
	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);

	/* Enable the DMA RX Stream */
	DMA_Cmd(DMA1_Stream5, ENABLE);
}
