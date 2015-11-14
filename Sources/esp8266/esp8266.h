#include <stdbool.h>
#include "circbuf.h"

#define RECEIVE_BUFFER_SIZE 2048

typedef void lineProcessor(char *);

//------------- Vars -------------//
extern volatile char receiveBuffer[RECEIVE_BUFFER_SIZE];

//------------- Structs -------------//
struct AccessPoint
{
   int ecn;
   char ssid[32];
   int rssi;
};

//------------- Public Functions -------------//

// Init ESP8266 by arranging uart connection
void esp8266_init(void);

// Check if the module is started
bool esp8266_isOnline(void);

// Restart module
bool esp8266_restart(void);

// List Access Points, returns amount of found accesspoints
struct AccessPoint *esp8266_listAp(unsigned char *amountOfAccessPointsRef);

// Join Access Point
bool esp8266_joinAp(const char *ssid, const char *password);

// Quit Access Point
bool esp8266_quitAp(void);

// Enable or disable multiple connection mode
bool esp8266_setMultipleConnectionMode(bool multipleConnectionMode);

// Opens connect to address on port
bool esp8266_openConnection(const char *address, const char *port);

// Closes connection
bool esp8266_closeConnection(void);

// Send data to connection
bool esp8266_sendData(const char *data);

// Read data send by ESP8266 to buffer
void esp8266_readData(circBuf_t *dataBuffer);


