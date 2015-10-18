
//------------- Public Functions -------------//
// Initialize SPI Manager
void spiManager_init(void);

// Process received byte
void spiManager_receivedByte();

// Process data of buffer
void spiManager_processData();

//------------- Static Functions -------------//
// Read and handle message
static void handleMessage(unsigned char messageType);

//------------- Commands (Static Functions) -------------//
// Run keep alive command
static void keepAlive(unsigned char messageSize);
