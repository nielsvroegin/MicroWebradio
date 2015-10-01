#include "esp8266.h"
#include "circBuf.h"
#include <string.h>
#include <xc.h>
#include <stdlib.h>

//------------- Global Vars -------------//
CIRCBUF_DEF(receiveBuffer, 32);

static unsigned char amountOfAccessPoints;
static struct AccessPoint accessPoints[10];

//------------- Public Functions -------------//

//Initialize UART to communicate with module
void esp8266_init(void) {
    // UART setup
    #define BAUDRATE 115200
    
    #ifndef _XTAL_FREQ
        #define _XTAL_FREQ 20000000
    #endif
    
    TRISC7 = 1;
    TRISC6 = 1;
    
    // Baudrate
    SPBRG = ((_XTAL_FREQ / 4) / BAUDRATE) - 1;
    SYNC = 0;
    BRGH = 1;
    BRG16 = 1;
    
    SPEN = 1; // Enable serial port pins
    CREN = 1; // Enable reception
    SREN = 0; // No effect
    TXIE = 0; // Disable tx interrupts
    RCIE = 1; // Enable rx interrupts
    TX9 = 0; // 8-bit transmission
    RX9 = 0; // 8-bit reception
    TXEN = 0; // Reset transmitter
    TXEN = 1; // Enanle transmitter
}

void esp8266_fillBuffer(unsigned char c) {
    circBufPush(&receiveBuffer, c);
}

// Check if the module is started
bit esp8266_isOnline(void) {
    performCommand("AT", NULL);      
    return 1;
}

// Restart module
bit esp8266_restart(void) {
    //_esp8266_print("AT+RST\r\n");
    //if (_esp8266_waitResponse() != ESP8266_OK) {
    //    return false;
   // }
    //return (_esp8266_waitResponse() == ESP8266_READY);
    
    return 0;
}

// List Access Points
bit esp8266_listAp(void) {   
    // Reset amount of access points
    amountOfAccessPoints = 0;
    
    // Request new access points
    return performCommand("AT+CWLAP", processAccessPointLine);    
}

//------------- Static Processor Functions -------------//

static void processAccessPointLine(unsigned char *line){
    unsigned char *parsedData;
    
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
static bit performCommand(unsigned const char *cmd, lineProcessor lineProcessor) {
    // Send command
    writeLine(cmd);
    
    // Wait for OK/ERROR and collect sent lines        
    while(1) {
        unsigned char line[50];
        readLine(&line);
        
        if(strcmp(line, "OK") == 0) {
            return 1;
        } else if(strcmp(line, "ERROR") == 0) {
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

static void writeLine(unsigned const char *line) {
    while (*line != 0) {        
        putch(*line++);
    }
    putch('\r');
    putch('\n');
}

// Read line from module
static void readLine(unsigned char line[50]) {
    unsigned char charCnt = 0;    
    
    while(1) {
        unsigned char c = getch();       
        
        if(c == '\n') { // End of line, return line
            return;
        } else if (c == '\r') { // Replace carriage return by \0 char
            line[charCnt] = '\0';
        } else { // Add char to line
            line[charCnt] = c; 
        }
        
        charCnt++;        
    }
    
    return;
}

// Put character to ESP866
static void putch(unsigned char c) {
    while(!TXIF) continue;
    
    TXREG = c;
}

// Read character from ESP866
static unsigned char getch(void) {
    unsigned char c;
    
    while(!circBufPop(&receiveBuffer, &c));
        
    return c;
}

