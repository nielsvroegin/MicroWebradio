#include <SPI.h>

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
  SPI.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE1));
}

void loop() {
  // When data present on serial read commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if(command == "Hello") {
      char message[] = "Master Alive";
      
      digitalWrite(slaveSelectPin,LOW);

      SPI.transfer(0x01);
      SPI.transfer(sizeof(message));
      SPI.transfer(message, sizeof(message));

      
      Serial.println("Keep alive sent");

      // 
      //for(int i = 0; i < 100 ; i++) {
      //  char c = SPI.transfer(0X00);
      //  Serial.write(c);
      //}

      digitalWrite(slaveSelectPin,HIGH);
     
    }
  }
}

void incomingMessage() {
  const char delay = 10;  
  char dummy;

  if(ledState == LOW) {
    ledState = HIGH;
  } else {
    ledState = LOW;
  }
  
  digitalWrite(led, ledState);  

  digitalWrite(slaveSelectPin,LOW);

  char messageType;
  messageType = SPI.transfer(0x00);
  delayMicroseconds(delay);
  messageType = SPI.transfer(0x00);
  delayMicroseconds(delay);
  
  //messageType = SPI.transfer(0x00);
  //messageType = SPI.transfer(0x00);
  //Serial.print(String(messageType, HEX));
  //Serial.print(":");
  
  //if (messageType == 0) { // Try once again when message type not yet set
  //  messageType = SPI.transfer(0x00);  
  //}
  
  //for (int i = 0; i < 10; i++) {
  //  messageType = SPI.transfer(0x00); 

    
  //  Serial.print(String(messageType, HEX));
  //  Serial.print(":");
    
  //}
  
  messageType = SPI.transfer(0x00);
  delayMicroseconds(delay);
  if(messageType == 1) {
    int messageLength = SPI.transfer(0x00);
    delayMicroseconds(delay);
    char message[messageLength];
  
    for(int i = 0; i < messageLength; i++) {
      message[i] = SPI.transfer(0x00);
      delayMicroseconds(delay);
    }
  
    Serial.println(message);
  }

  digitalWrite(slaveSelectPin,HIGH);
}

