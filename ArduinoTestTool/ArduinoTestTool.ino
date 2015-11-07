#include <SPI.h>

// Message types of Radio connector
#define KEEPALIVE 1
#define LISTACCESSPOINTS 2
#define JOINACCESSPOINT 3
#define QUITACCESSPOINT 4
#define DATA 100

const int slaveSelectPin = 10;
const int interruptPin = 2;
const int led = 3;
int ledState = LOW;
volatile boolean unprocessedMessage = false;

void setup() {
  // Setup serial
  Serial.begin(115200);
  
  pinMode(led, OUTPUT);
  digitalWrite(led,ledState);  
  
  // INTERRUPT SETUP
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), incomingMessage, FALLING);

  // SPI SETUP
  // Set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // Disable slave
  digitalWrite(slaveSelectPin,HIGH);  
  // Initialize SPI:
  SPI.begin();
  //Set used interrupt
  SPI.usingInterrupt(digitalPinToInterrupt(interruptPin));    
}

void loop() {
  // When data present on serial read commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    // Check for request command
    if(command == "Alive") {
      sendKeepAlive();     
    } else if(command == "ListAp") {
      sendListAp();
    } else {
      Serial.println("Available commands:");
      Serial.println("Alive  --- Request keep alive from slave");
      Serial.println("ListAp --- Request accesspoints from slave");
      Serial.println();
    }    
  }

  // Check for unprcoessed messages
  if (unprocessedMessage) {
    processIncomingMessage();
  }
}

/**
 *  Interrupt method used to get notified about messages of slave
 */
void incomingMessage() {  
  // Show led interrupt has been triggered 
  if(ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }  
  digitalWrite(led, ledState);  

  // Set incoming message true
  unprocessedMessage = true;
}

void processIncomingMessage() {
  // Message will be processed, so clear bool
  unprocessedMessage = false;
  
  // Receive message
  SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE1));  
  digitalWrite(slaveSelectPin,LOW);

  // Ignore 0 and read message type
  byte messageType = SPI.transfer(0x00);  
  while(messageType == 0x00) {
    messageType = SPI.transfer(0x00);    
  }  
  
  // Read message length
  byte messageLength = SPI.transfer(0x00);        

  // Proccess message
  if (messageType == KEEPALIVE) {
    readKeepAlive(messageLength);
  } else if(messageType == LISTACCESSPOINTS) {    
    readListAccesspoints(messageLength);
  } else if(messageType == DATA) {    
    readData(messageLength);
  } else {    
    Serial.println("Unknown message type received");
  }
  
  digitalWrite(slaveSelectPin,HIGH);
  SPI.endTransaction();
}

//------------- Send message Functions -------------//

/**
 *  Send Keep Alive to slave 
 */
void sendKeepAlive(void) {
  char message[] = "Master Alive";

  // Sending keep alive
  Serial.println("---> Sending keep alive");

  // Request keep alive
  SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE1));  
  digitalWrite(slaveSelectPin,LOW);
  
  SPI.transfer(KEEPALIVE);
  SPI.transfer(sizeof(message));
  SPI.transfer(message, sizeof(message));
  
  digitalWrite(slaveSelectPin,HIGH);
  SPI.endTransaction();
}

/**
 *  Send list accesspoints 
 */
void sendListAp() {
  // Keep alive sent to slave
  Serial.println("---> Sending list accesspoints");

  // Request accesspoints
  SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE1));
  digitalWrite(slaveSelectPin,LOW);
  
  SPI.transfer(LISTACCESSPOINTS);
  SPI.transfer(0x00);
  
  digitalWrite(slaveSelectPin,HIGH);
  SPI.endTransaction();
}

//------------- Read message Functions -------------//

/**
 * Read received keep alive message
 */
void readKeepAlive(byte messageLength) {
  for(byte i = 0; i < messageLength; i++) {
    char c = SPI.transfer(0x00);
    Serial.print(c);    
  }
  
  Serial.println();
}

/**
 * Read received list accesspoint message
 */
void readListAccesspoints(byte messageLength) {
  char message[64];
  
  for(byte i = 0; i < messageLength; i++) {
    byte c = SPI.transfer(0x00);    

    if(i > 1) {
      message[i-2] = c;
    }
  }

  Serial.println(message);  
}

/**
 * Read plain data
 */
void readData(byte messageLength) {
  for(byte i = 0; i < messageLength; i++) {
    char c = SPI.transfer(0x00);
    Serial.print(c);    
  }
  
  //Serial.println();
}

