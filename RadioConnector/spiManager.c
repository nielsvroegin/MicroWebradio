#include "spiManager.h"
#include <plib/spi.h>
#include <string.h>
#include "circBuf.h"
#include "esp8266.h"

#ifndef _XTAL_FREQ
    #define _XTAL_FREQ 40000000
#endif

// Message types of Radio connector
#define KEEPALIVE 1
#define LISTACCESSPOINTS 2
#define JOINACCESSPOINT 3
#define QUITACCESSPOINT 4

//------------- Global Vars -------------//
CIRCBUF_DEF(receiveBuffer, 32);
CIRCBUF_DEF(sendBuffer, 64);

unsigned char nextByte;

//------------- Public Functions -------------//

// Initialize SPI Manager
void spiManager_init(void) {
    // Configure pin to trigger interrupt at master
    TRISAbits.TRISA1 = 0; // Master Interrupt trigger as output
    LATAbits.LATA1 = 1; // Set High, as interrupt will be trigger by low
    
    // Close when SPI was already open
    CloseSPI();
    
    // Open SPI as slave
    OpenSPI(SLV_SSON, MODE_01, SMPEND);
    
    // Set buffer to 0
    SSPBUF = 0x00;
    nextByte = 0x00;
    
    // Enable interrupts
    EnableIntSPI;
}

// Process received byte
inline void spiManager_receivedByte() {
    // Read char from buffer
    unsigned char readByte = SSPBUF;
    SSPBUF = nextByte;
    
    // Load next byte from send buffer
    if(!circBufPop(&sendBuffer, &nextByte)) {        
        nextByte = 0x00;        
    }
    
    // Add read byte to buffer    
    circBufPush(&receiveBuffer, readByte);    
    
    SPI_Clear_Intr_Status_Bit;
}

void spiManager_processData() {
    unsigned char c;
    
    // Return when no data in buffer
    if(!circBufPop(&receiveBuffer, &c)) {
        return;
    }
    
    // Handle message when no dummy data has been sent
    if(c != 0) {    
        handleMessage(c);    
    }
}

//------------- Send message functions -------------//

// Send alive message to 
void spiManager_sendKeepAlive() {    
    const unsigned char *aliveMessage = "Slave Alive";
    sendMessage(KEEPALIVE, aliveMessage, strlen(aliveMessage));
}

// Send accesspoints to master
void spiManager_sendAccessPoints(unsigned char accessPointsCount, struct AccessPoint *accessPoints) {
    for (unsigned char i = 0; i < accessPointsCount; i++) {
        struct AccessPoint *accessPoint = accessPoints + i;
        
        sendMessage(LISTACCESSPOINTS, (unsigned char *)accessPoint, sizeof(struct AccessPoint));
    }
}

//------------- Static Functions -------------//
// Send message to master
static void sendMessage(const unsigned char messageType, const unsigned char *message, const unsigned char messageLength) {
    
    // Send message type/length
    circBufPush(&sendBuffer, messageType);
    circBufPush(&sendBuffer, messageLength);
    
    // Send message
    for (unsigned char i = 0; i < messageLength; i++) {        
        circBufPush(&sendBuffer, *(message + i));
    }
    
    // Trigger interrupt at master
    LATAbits.LATA1 = 0;
    LATAbits.LATA1 = 1;
    
    // Wait until send buffer is empty   
    while(sendBuffer.head != sendBuffer.tail);    
}

// Handle received message
static void handleMessage(unsigned char messageType) {
    unsigned char messageSize;
    
    // Determine size of message
    while(!circBufPop(&receiveBuffer, &messageSize));
    
    switch(messageType) {
        case KEEPALIVE:
            handleKeepAlive(messageSize);
            break;
        case LISTACCESSPOINTS:
            handleListAccessPoints();
            break;
        default:
            break;
    }
} 

//------------- Commands (Static Functions) -------------//

// Run keep alive command
static void handleKeepAlive(unsigned char messageSize) {
    // Read keep alive message
    unsigned char keepAliveMessage[16];
    for(int i = 0; i < messageSize; i++) {
        unsigned char c;
        
        while(!circBufPop(&receiveBuffer, &c));
        
        keepAliveMessage[i] = c;
    }
    keepAliveMessage[messageSize] = '\0';
    
    // Send keep alive message back to master
    spiManager_sendKeepAlive();
}

static void handleListAccessPoints() {
    unsigned char accessPointsCount;
    
    // List accesspoints and point first one to pointer
    struct AccessPoint *accessPoints = esp8266_listAp(&accessPointsCount);
    
    // Send access points to master
    spiManager_sendAccessPoints(accessPointsCount, accessPoints);    
}

