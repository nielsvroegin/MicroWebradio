#include "esp8266.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_usart.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MAXACCESSPOINTS 15

//------------- Static Processor Functions -------------//

static void processAccessPointLine(char *line);

//------------- Static Functions -------------//

// Perform ESP8266 command
static bool performCommand(const char *cmd, lineProcessor lineProcessor);

// Write line to module
static void writeLine(const char *line);

// Read line from module
static void readLine(char *line);

// Put character to ESP866
static void putch(char);

// Read character from ESP866
static char getch(void);

// Read ipd from data stream
static void readIpd(char c, circBuf_t *dataBuffer);

// Init methods
static void GPIOInitialize(void);
static void UART_Initialize(void);
static void DMA_Initialize(void);


//------------- Global Vars -------------//
uint16_t receiveBufferTail;
volatile char receiveBuffer[RECEIVE_BUFFER_SIZE];
static char command[64];

static unsigned char amountOfAccessPoints;
static struct AccessPoint accessPoints[MAXACCESSPOINTS];

//------------- Public Functions -------------//

//Initialize UART to communicate with module
void esp8266_init(void) {
	GPIOInitialize();
	UART_Initialize();
	DMA_Initialize();
}

static void GPIOInitialize(void) {
	GPIO_InitTypeDef GPIOD_InitStruct;

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

static void UART_Initialize(void) {
	USART_InitTypeDef USART_InitStructure;

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
	USART_InitStructure.USART_BaudRate = 250000;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART2, &USART_InitStructure); // USART configuration
	USART_Cmd(USART2, ENABLE); // Enable USART
}

static void DMA_Initialize(void) {
	DMA_InitTypeDef  DMA_InitStructure;

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

// Check if the module is started
bool esp8266_isOnline(void) {
    return performCommand("AT", NULL);
}

// Restart module
bool esp8266_restart(void) {
    return performCommand("AT+RST", NULL);
}

// List Access Points, returns amount of found access points
struct AccessPoint *esp8266_listAp(unsigned char *amountOfAccessPointsRef) {
    // Reset amount of access points
    amountOfAccessPoints = 0;

    // Request new access points
    performCommand("AT+CWLAP", processAccessPointLine);

    // Set amount of accesspoints
    *amountOfAccessPointsRef = amountOfAccessPoints;

    return accessPoints;
}

// Join Access Point
bool esp8266_joinAp(const char *ssid, const char *password) {
    strcpy(command, "AT+CWJAP=\"");
    strcat(command, ssid);
    strcat(command, "\",\"");
    strcat(command, password);
    strcat(command, "\"");

    return performCommand(command, NULL);
}

// Restart module
bool esp8266_quitAp(void) {
    return performCommand("AT+CWQAP", NULL);
}

// Enable or disable multiple connection mode
bool esp8266_setMultipleConnectionMode(bool multipleConnectionMode) {
    strcpy(command, "AT+CIPMUX=");

    if(multipleConnectionMode) {
        strcat(command, "1");
    } else {
        strcat(command, "0");
    }

    return performCommand(command, NULL);
}

// Opens connect to address on port
bool esp8266_openConnection(const char *address, const char *port) {
    strcpy(command, "AT+CIPSTART=\"TCP\",\"");
    strcat(command, address);
    strcat(command, "\",");
    strcat(command, port);

    return performCommand(command, NULL);
}

// Closes connection
bool esp8266_closeConnection(void) {
	return performCommand("AT+CIPCLOSE", NULL);
}

// Send data to connection
bool esp8266_sendData(const char *data) {
    // Determine amount of data bytes
    int amountOfBytes = strlen(data);

    // Convert int to string
    char amountOfBytesStr[4];
    itoa(amountOfBytes, amountOfBytesStr, 10);

    // Create command
    strcpy(command, "AT+CIPSEND=");
    strcat(command, amountOfBytesStr);

    // Check if send request was successful
    if(!performCommand(command, NULL)) {
        return 0;
    }

    // Send data
    writeLine(data);

    return true;
}

// Read data send by ESP8266 to buffer
void esp8266_readData(circBuf_t *dataBuffer) {

	// Wait until data available in buffer
	while(receiveBufferTail != (RECEIVE_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA1_Stream5))) {

		// Load char from buffer
		char c = receiveBuffer[receiveBufferTail];

		// Try to read IPD
		readIpd(c, dataBuffer);

		// Increment tail
		receiveBufferTail++;

		// Reset when at end of buffer
		if(receiveBufferTail == RECEIVE_BUFFER_SIZE) {
			receiveBufferTail = 0;
		}
	}
}

//------------- Static Processor Functions -------------//

static void processAccessPointLine(char *line){
    char *parsedData;

    // Check if space for another accesspoint
    if(amountOfAccessPoints == MAXACCESSPOINTS) {
        return;
    }

    // Check if line contains CWLAP(AccessPoint data)
    parsedData = strstr(line, "+CWLAP:(");
    if(parsedData == NULL)
        return;

    // Move pointer to first data item
    parsedData += 8;

    // Create new access point
    accessPoints[amountOfAccessPoints].ecn = atoi(strtok(parsedData,","));
    strcpy(accessPoints[amountOfAccessPoints].ssid, strtok(NULL,",\""));
    accessPoints[amountOfAccessPoints].rssi = atoi(strtok(NULL,","));

    // Increase amount of accesspoints
    amountOfAccessPoints++;
}

//------------- Static Functions -------------//

// Perform ESP8266 command
//
// Return reponse lines
static bool performCommand(const char *cmd, lineProcessor lineProcessor) {
    // Send command
    writeLine(cmd);

    // Wait for OK/ERROR and collect sent lines
    while(1) {
        char line[50];
        readLine(&line[0]);

        if(strcmp(line, "OK") == 0) {
            return 1;
        } else if(strcmp(line, "SEND OK") == 0) {
            return 1;
        } else if(strcmp(line, "ERROR") == 0) {
            return 0;
        } else if(strcmp(line, "FAIL") == 0) {
            return 0;
        } else {
            // Collect other lines
            if(lineProcessor != NULL) {
                lineProcessor(line);
            }
        }
    }

    return 0;
}

static void writeLine(const char *line) {
    while (*line != 0) {
        putch(*line++);
    }
    putch('\r');
    putch('\n');
}

// Read line from module
static void readLine(char *line) {
	unsigned char charCnt = 0;

    for(charCnt = 0; charCnt < 50; charCnt++) {
        unsigned char c = getch();

        if(c == '\n') { // End of line, return line
            line[charCnt] = '\0';
            return;
        } else if (c == '\r') { // Replace carriage return by \0 char
            line[charCnt] = '\0';
        } else { // Add char to line
            line[charCnt] = c;
        }
    }

    line[49] = '\0';
    return;
}

// Put character to ESP866
static void putch(char c) {
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);

	PrintChar(c);

	USART_SendData(USART2, c);
}

// Read character from ESP866
static char getch(void) {
	// Wait until data available in buffer
	uint16_t freeUnits = DMA_GetCurrDataCounter(DMA1_Stream5);
    while(receiveBufferTail == (RECEIVE_BUFFER_SIZE - freeUnits)) {
    	freeUnits = DMA_GetCurrDataCounter(DMA1_Stream5);
    }

    // Reset when at end of buffer
    if(receiveBufferTail == RECEIVE_BUFFER_SIZE) {
    	receiveBufferTail = 0;
    }

    // Load char from buffer
    char c = receiveBuffer[receiveBufferTail];

    PrintChar(c);

    // Increment tail
    receiveBufferTail++;

    return c;
}

// Read ipd from data stream
static void readIpd(char c, circBuf_t *dataBuffer) {
	const char *ipdHeader = "+IPD,";
	static bool readingLength;
	static bool readingPackage;
	static unsigned int packageLength;
	static char packageLengthStr[6];
	static unsigned int counter = 0;

	if (readingPackage) {
		// Read package

		// Push char on buffer
		if(!circBufPush(dataBuffer, c)) {
			printf("No space in MP3 buffer for new byte");
		}

		counter++;

		// Check for end of package stream
		if(counter == packageLength) {
			readingLength = false;
			readingPackage = false;
			packageLength = 0;
			counter = 0;
		}

	} else if (readingLength) {

		// Read length of package
		if(c == ':') {
			readingLength = false;
			readingPackage = true;
			packageLengthStr[counter] = '\0';
			packageLength = atoi(packageLengthStr);
			counter = 0;
		} else {
			packageLengthStr[counter] = c;
			counter++;
		}
	} else {

		// Check for ipd message
		if(ipdHeader[counter] == c) {
			counter++;

			if(strlen(ipdHeader) == counter) {
				readingLength = true;
				counter = 0;
			}
		} else {
			counter = 0;
		}
	}
}
