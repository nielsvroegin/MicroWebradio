typedef void lineProcessor(unsigned char *);

//------------- Structs -------------//
struct AccessPoint
{
   unsigned char ecn;
   unsigned char ssid[32];
   char rssi;
};

//------------- Public Functions -------------//

// Init ESP8266 by arranging uart connection
void esp8266_init(void);

// Fill receive buffer with byte
void esp8266_fillBuffer(unsigned char c);

// Check if the module is started
bit esp8266_isOnline(void);

// Restart module
bit esp8266_restart(void);          

// List Access Points
bit esp8266_listAp(void);

// Join Access Point
bit esp8266_joinAp(const unsigned char *ssid, const unsigned char *password);

// Quit Access Point
bit esp8266_quitAp(void);

//------------- Static Processor Functions -------------//

static void processAccessPointLine(unsigned char *line);

//------------- Static Functions -------------//

// Perform ESP8266 command
static bit performCommand(unsigned const char *cmd, lineProcessor lineProcessor);

// Write line to module
static void writeLine(unsigned const char *line);

// Read line from module
static void readLine(unsigned char line[50]);

// Put character to ESP866
static void putch(unsigned char);

// Read character from ESP866
static unsigned char getch(void);
