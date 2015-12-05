#include "externalbuffer.h"
#include "stm32f4xx_conf.h"

//------------- Static Functions -------------//
static void sendByte(char data);
static char receiveByte(void);

// Init methods
static void GPIOInitialize(void);
static void SPIInitialize(void);

//------------- Public Functions -------------//
// Initialize external buffer
void extbuffer_init(void) {
	GPIOInitialize();
	SPIInitialize();
}

static void GPIOInitialize(void) {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB | RCC_AHB1Periph_GPIOD, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;

	/*-------- Configuring SCK, MISO, MOSI --------*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*-------- Configuring ChipSelect-Pin PD7 --------*/
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;

	GPIO_Init(GPIOD, &GPIO_InitStruct);

	GPIO_SetBits(GPIOD, GPIO_Pin_7);

	/*-------- Configure alternate function --------*/
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_SPI1);
}

static void SPIInitialize(void) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

	SPI_InitTypeDef SPI_InitTypeDefStruct;

	SPI_InitTypeDefStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitTypeDefStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitTypeDefStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitTypeDefStruct.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitTypeDefStruct.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitTypeDefStruct.SPI_NSS = SPI_NSS_Soft;
	SPI_InitTypeDefStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
	SPI_InitTypeDefStruct.SPI_FirstBit = SPI_FirstBit_MSB;

	SPI_Init(SPI1, &SPI_InitTypeDefStruct);

	SPI_Cmd(SPI1, ENABLE);
}

void extbuffer_test(void) {
	GPIO_WriteBit(GPIOD, GPIO_Pin_7, Bit_RESET);

	sendByte(0x02); // WRITE
	sendByte(0x00); // ADDRESS
	sendByte(0x00); // ADDRESS
	sendByte(0x00); // ADDRESS
	sendByte('H');
	sendByte('E');
	sendByte('L');
	sendByte('L');
	sendByte('O');

	GPIO_WriteBit(GPIOD, GPIO_Pin_7, Bit_SET);

	DelayMs(100);

	GPIO_WriteBit(GPIOD, GPIO_Pin_7, Bit_RESET);

	sendByte(0x03); // READ
	sendByte(0x00); // ADDRESS
	sendByte(0x00); // ADDRESS
	sendByte(0x00); // ADDRESS

	char bla1 = receiveByte();
	char bla2 = receiveByte();
	char bla3 = receiveByte();
	char bla4 = receiveByte();
	char bla5 = receiveByte();

	GPIO_WriteBit(GPIOD, GPIO_Pin_7, Bit_SET);
}

static void sendByte(char data) {
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
	SPI_I2S_SendData(SPI1, data);
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));
	SPI_I2S_ReceiveData(SPI1); //Clear RXNE bit
}

static char receiveByte(void) {
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE));
	SPI_I2S_SendData(SPI1, 0x00);
	while(!SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE));
	return SPI_I2S_ReceiveData(SPI1);
}
