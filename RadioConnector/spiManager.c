#include "spiManager.h"
#include <plib/spi.h>
#include <string.h>
#include "circBuf.h"

// Message types of Radio connector
#define KEEPALIVE 1
#define LISTACCESSPOINTS 2
#define JOINACCESSPOINT 3
#define QUITACCESSPOINT 4

//------------- Global Vars -------------//
CIRCBUF_DEF(receiveBuffer, 32);
CIRCBUF_DEF(sendBuffer, 32);

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
void spiManager_sendKeepAlive() {    
    const unsigned char *aliveMessage = "Master Alive";
    sendMessage(KEEPALIVE, aliveMessage, strlen(aliveMessage));
}

//------------- Static Functions -------------//
// Send message to master
static void sendMessage(const unsigned char messageType, const unsigned char *message, const unsigned char messageLength) {
    circBufPush(&sendBuffer, messageType);
    circBufPush(&sendBuffer, messageLength);
    
    for (unsigned char i = 0; i < messageLength; i++) {
        circBufPush(&sendBuffer, *(message + i));
    }
    
    // Trigger interrupt at master
    LATAbits.LATA1 = 0;
    LATAbits.LATA1 = 1;
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

