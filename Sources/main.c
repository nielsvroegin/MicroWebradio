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
GPIO_InitTypeDef GPIOB_InitStruct;
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

	/*** GPIOB initialization ***/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); //Enable clock for GPIOB

	/*** USART1 Tx on PB6 | Rx on PB7 ***/
	GPIOB_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIOB_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIOB_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIOB_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIOB_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOB, &GPIOB_InitStruct);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);//Connect PB6 to USART1_Tx
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);//Connect PB7 to USART1_Rx
}

void UART_Initialize(void) {
	/* Enable peripheral clock for USART1 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/* USART1 configured as follow:
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

	USART_Init(USART1, &USART_InitStructure); // USART configuration
	USART_Cmd(USART1, ENABLE); // Enable USART
}

void DMA_Initialize(void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	DMA_DeInit(DMA2_Stream2);

	DMA_InitStructure.DMA_Channel = DMA_Channel_4;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory; // Receive
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)receiveBuffer;
	DMA_InitStructure.DMA_BufferSize = (uint16_t)sizeof(receiveBuffer);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&USART1->DR;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

	DMA_Init(DMA2_Stream2, &DMA_InitStructure);

	/* Enable the USART Rx DMA request */
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);

	/* Enable the DMA RX Stream */
	DMA_Cmd(DMA2_Stream2, ENABLE);
}
