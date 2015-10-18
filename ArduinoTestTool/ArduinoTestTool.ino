#include <SPI.h>

const int slaveSelectPin = 10;

void setup() {
  // Setup serial
  Serial.begin(115200);

  // SPI SETUP
  // Set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  // Disable slave
  digitalWrite(slaveSelectPin,HIGH);
  
  // Initialize SPI:
  SPI.begin();
  // Set SPI settings  
  SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE1));
}

void loop() {
  // When data present on serial read commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');

    if(command == "Hello") {
      char message[] = "Master Alive";
      
      digitalWrite(slaveSelectPin,LOW);

      delay(100);
      
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
