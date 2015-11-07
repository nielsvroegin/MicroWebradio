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


//------------- Global Vars -------------//
uint16_t receiveBufferTail;
volatile char receiveBuffer[RECEIVE_BUFFER_SIZE];
static char command[64];

static unsigned char amountOfAccessPoints;
static struct AccessPoint accessPoints[MAXACCESSPOINTS];

//------------- Public Functions -------------//

//Initialize UART to communicate with module
void esp8266_init(void) {

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
    return performCommand(data, NULL);
}

// Read data send by ESP8266 to maximum of 64 byte
unsigned char esp8266_readData(char *data) {
	unsigned char index = 0;

	// Wait until data available in buffer
	while(receiveBufferTail != (RECEIVE_BUFFER_SIZE - DMA_GetCurrDataCounter(DMA2_Stream2))) {
		// Reset when at end of buffer
		if(receiveBufferTail == RECEIVE_BUFFER_SIZE) {
			receiveBufferTail = 0;
		}

		// Load char from buffer
		char c = receiveBuffer[receiveBufferTail];

		// Set char in data
		*(data + index) = c;

		// Temp print char
		PrintChar(c);

		// Increment tail
		receiveBufferTail++;

		//Stop reading data when 64 bytes have been read
		index++;
		if(index == 64) {
		  break;
		}
	}

    return index;
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
	while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

	PrintChar(c);

	USART_SendData(USART1, c);
}

// Read character from ESP866
static char getch(void) {
	// Wait until data available in buffer
	uint16_t freeUnits = DMA_GetCurrDataCounter(DMA2_Stream2);
    while(receiveBufferTail == (RECEIVE_BUFFER_SIZE - freeUnits)) {
    	freeUnits = DMA_GetCurrDataCounter(DMA2_Stream2);
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

