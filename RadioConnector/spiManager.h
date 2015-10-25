
//------------- Public Functions -------------//
// Initialize SPI Manager
void spiManager_init(void);

// Process received byte
inline void spiManager_receivedByte();

// Process data of buffer
void spiManager_processData();

//------------- Send message functions -------------//
void spiManager_sendKeepAlive();

// Send accesspoints to master
void spiManager_sendAccessPoints(unsigned char accessPointsCount, struct AccessPoint *accessPoints);

//------------- Static Functions -------------//
// Read and handle message
static void handleMessage(unsigned char messageType);

// Send message to master
static void sendMessage(const unsigned char messageType, const unsigned char *message, const unsigned char messageLength);

//------------- Handle message funtions (Static Functions) -------------//
// Handle keep alive message
static void handleKeepAlive(unsigned char messageSize);

// Hadnle list access points
static void handleListAccessPoints();
