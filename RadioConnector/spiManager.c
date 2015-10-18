#include "spiManager.h"
#include <plib/spi.h>
#include "circBuf.h"

// Message types of Radio connector
#define KEEPALIVE 1
#define LISTACCESSPOINTS 2
#define JOINACCESSPOINT 3
#define QUITACCESSPOINT 4

//------------- Global Vars -------------//
CIRCBUF_DEF(receiveBuffer, 32);

//------------- Public Functions -------------//

// Initialize SPI Manager
void spiManager_init(void) {
    // Close when SPI was already open
    CloseSPI();
    
    // Open SPI as slave
    OpenSPI(SLV_SSON, MODE_01, SMPEND);
    
    // Enable interrupts
    EnableIntSPI;
}

// Process received byte
void spiManager_receivedByte() {
    unsigned char c = SSPBUF;
    
    circBufPush(&receiveBuffer, c);
    
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

//------------- Static Functions -------------//

// Run sent command
static void handleMessage(unsigned char messageType) {
    unsigned char messageSize;
    
    // Determine size of message
    while(!circBufPop(&receiveBuffer, &messageSize));
    
    switch(messageType) {
        case KEEPALIVE:
            keepAlive(messageSize);
            break;
        default:
            break;
    }
} 

//------------- Commands (Static Functions) -------------//

// Run keep alive command
static void keepAlive(unsigned char messageSize) {
    unsigned char keepAliveMessage[16];
    for(int i = 0; i < messageSize; i++) {
        unsigned char c;
        
        while(!circBufPop(&receiveBuffer, &c));
        
        keepAliveMessage[i] = c;
    }
    keepAliveMessage[messageSize] = '\0';
    
    // Send alive string to master
    //unsigned char message[6] = "ALIVE";
    //putsSPI(message);    
}