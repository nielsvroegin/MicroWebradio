#include <SPI.h>

// Message types of Radio connector
#define KEEPALIVE 1
#define LISTACCESSPOINTS 2
#define JOINACCESSPOINT 3
#define QUITACCESSPOINT 4

const int slaveSelectPin = 10;
const int interruptPin = 2;
const int led = 3;
int ledState = LOW;

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
  // Set SPI settings  
  SPI.beginTransaction(SPISettings(200000, MSBFIRST, SPI_MODE1));
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

  // Receive message
  digitalWrite(slaveSelectPin,LOW);

  // Ignore 0 and read message type
  byte messageType;
  while(messageType == 0x00) {
    messageType = transferByte(0x00);        
  }
  
  // Read message length
  int messageLength = SPI.transfer(0x00);      
  delayMicroseconds(1);

  // Proccess message
  if (messageType == KEEPALIVE) {
    readKeepAlive(messageLength);
  } else if(messageType == LISTACCESSPOINTS) {    
    readListAccesspoints(messageLength);
  } else {    
    Serial.println("Unknown message type received");
  }
  
  digitalWrite(slaveSelectPin,HIGH);
}

byte transferByte(const byte c) {
  byte receivedByte = SPI.transfer(c);
  //delayMicroseconds(1);  

  return receivedByte;
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
  digitalWrite(slaveSelectPin,LOW);
  
  SPI.transfer(KEEPALIVE);
  SPI.transfer(sizeof(message));
  SPI.transfer(message, sizeof(message));
  
  digitalWrite(slaveSelectPin,HIGH);
}

/**
 *  Send list accesspoints 
 */
void sendListAp() {
  // Keep alive sent to slave
  Serial.println("---> Sending list accesspoints");

  // Request accesspoints
  digitalWrite(slaveSelectPin,LOW);
  
  SPI.transfer(LISTACCESSPOINTS);
  SPI.transfer(0x00);
  
  digitalWrite(slaveSelectPin,HIGH);
}

//------------- Read message Functions -------------//

/**
 * Read received keep alive message
 */
void readKeepAlive(int messageLength) {
  for(int i = 0; i < messageLength; i++) {
    char c = transferByte(0x00);
    Serial.print(c);    
  }
  
  Serial.println();
}

/**
 * Read received list accesspoint message
 */
void readListAccesspoints(int messageLength) {
  char message[messageLength];
  
  for(int i = 0; i < messageLength; i++) {
    byte c = transferByte(0x00);    

    if(i > 1) {
      message[i-2] = c;
    }
  }

  Serial.println(message);  
}

