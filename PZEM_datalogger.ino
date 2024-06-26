// PZEM-017 DC Energy Meter with LCD By Solarduino 

// Note Summary
// Note :  Safety is very important when dealing with electricity. We take no responsibilities while you do it at your own risk.
// Note :  This DC Energy Monitoring Code needs PZEM-017 DC Energy Meter to measure values and Arduio Mega / UNO for communication and display. 
// Note :  This Code monitors DC Voltage, current, Power, and Energy.
// Note :  The values shown in LCD Display is refreshed every second.
// Note :  The values are calculated internally by energy meter and function of Arduino is only to read the value and for further calculation. 
// Note :  The first step is need to select shunt value and change the value accordingly. look for line "static uint16_t NewshuntAddr = 0x0000; "
// Note :  You need to download and install (modified) Modbus Master library at our website (https://solarduino.com/pzem-014-or-016-ac-energy-meter-with-arduino/ )
// Note :  The Core of the code was from EvertDekker.com 2018 which based on the example from http://solar4living.com/pzem-arduino-modbus.htm
// Note :  Solarduino only amend necessary code and integrate with LCD Display Shield.

/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/////////////*/


        /* 1- PZEM-017 DC Energy Meter */
        
        #include <ModbusMaster.h>                   // Load the (modified) library for modbus communication command codes. Kindly install at our website.
        #define MAX485_DE      2                    // Define DE Pin to Arduino pin. Connect DE Pin of Max485 converter module to Pin 2 (default) Arduino board
        #define MAX485_RE      3                    // Define RE Pin to Arduino pin. Connect RE Pin of Max485 converter module to Pin 3 (default) Arduino board
                                                    // These DE anr RE pins can be any other Digital Pins to be activated during transmission and reception process.
        static uint8_t pzemSlaveAddr = 0x01;        // Declare the address of device (meter) in term of 8 bits. You can change to 0x02 etc if you have more than 1 meter.
        static uint16_t NewshuntAddr = 0x0003;      // Declare your external shunt value. Default is 100A, replace to "0x0001" if using 50A shunt, 0x0002 is for 200A, 0x0003 is for 300A
                                                    // By default manufacturer may already set, however, to set manually kindly delete the "//" for line "// setShunt(0x01);" in void setup
        ModbusMaster node;                          /* activate modbus master codes*/  
        float PZEMVoltage =0;                       /* Declare value for DC voltage */
        float PZEMCurrent =0;                       /* Declare value for DC current*/
        float PZEMPower =0;                         /* Declare value for DC Power */
        float PZEMEnergy=0;                         /* Declare value for DC Energy */
        unsigned long previousMillis = 0;            // Store the last time you updated the energy reading
        float accumulatedEnergy = 0;                 // Total energy in watt-hours                // Total energy in watt-hours
        unsigned long startMillisPZEM;              /* start counting time for LCD Display */
        unsigned long currentMillisPZEM;            /* current counting time for LCD Display */
        const unsigned long periodPZEM = 1000;      // refresh every X seconds (in seconds) in LED Display. Default 1000 = 1 second 
        int page = 1;                               /* display different pages on LCD Display*/
        

        /* 2 - LCD Display  */
        #include <Wire.h>
        #include <LiquidCrystal_I2C.h>                   /* Load the liquid Crystal Library (by default already built-it with arduino solftware)*/
        LiquidCrystal_I2C LCD(0x27, 16, 2);             /* Creating the LiquidCrystal object named LCD. The pin may be varies based on LCD module that you use*/
        unsigned long startMillisLCD;               /* start counting time for LCD Display */                            
        unsigned long currentMillisLCD;             /* current counting time for LCD Display */
        const unsigned long periodLCD = 1000;       /* refresh every X seconds (in seconds) in LCD Display. Default 1000 = 1 second */
        int ResetEnergy = 0;                        /* reset energy function */
        unsigned long startMillisEnergy;            /* start counting time for LCD Display */
        unsigned long currentMillisEnergy;          /* current counting time for LCD Display */
        const unsigned long periodEnergy = 1000;    // refresh every X seconds (in seconds) in LCD Display. Default 1000 = 1 second 

        /* uSDcard Module   */

        #include <SPI.h>
        #include <SD.h>

        File myFile;



void setup() 
{
        /*0 General*/
        
        Serial.begin(9600);                         /* to display readings in Serial Monitor at 9600 baud rates */

        /* 1- PZEM-017 DC Energy Meter */


        Serial.print("Initializing SD Card...");

        if(!SD.begin(4)) {
          Serial.println("initizilzing failed");
          return;
        }

        Serial.println("SD card Initilized");
        
        setShunt(0x03);                          // Delete the "//" to set shunt rating (0x01) is the meter address by default
        // resetEnergy(0x01);                       // By delete the double slash symbol, the Energy value in the meter is reset. Can also be reset on the LCD Display      
        startMillisPZEM = millis();                 /* Start counting time for run code */
        Serial3.begin(9600,SERIAL_8N2);             /* To assign communication port to communicate with meter. with 2 stop bits (refer to manual)*/
                                                    // By default communicate via Serial3 port: pin 14 (Tx) and pin 15 (Rx)
        node.begin(pzemSlaveAddr, Serial3);         /* Define and start the Modbus RTU communication. Communication to specific slave address and which Serial port */
        pinMode(MAX485_RE, OUTPUT);                 /* Define RE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
        pinMode(MAX485_DE, OUTPUT);                 /* Define DE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
        digitalWrite(MAX485_RE, 0);                 /* Arduino create output signal for pin RE as LOW (no output)*/
        digitalWrite(MAX485_DE, 0);                 /* Arduino create output signal for pin DE as LOW (no output)*/
                                                    // both pins no output means the converter is in communication signal receiving mode
        node.preTransmission(preTransmission);      // Callbacks allow us to configure the RS485 transceiver correctly
        node.postTransmission(postTransmission);
        changeAddress(0XF8, 0x01);                  // By delete the double slash symbol, the meter address will be set as 0x01. 
                                                    // By default I allow this code to run every program startup. Will not have effect if you only have 1 meter
                           
        delay(1000);                                /* after everything done, wait for 1 second */

        /* 2 - LCD Display  */

        LCD.init();
        LCD.backlight();                         /* Set LCD to start with upper left corner of display*/  
        startMillisLCD = millis();                  /* Start counting time for display refresh time*/


}


void loop() {
    /* 1- PZEM-017 DC Energy Meter */
    currentMillisPZEM = millis();  // count time for program run every second (by default)
    if (currentMillisPZEM - startMillisPZEM >= periodPZEM) {  // for every x seconds, run the codes below
        uint8_t result;  // Declare variable "result" as 8 bits
        result = node.readInputRegisters(0x0000, 6);  // read the 6 registers of the PZEM-017
        if (result == node.ku8MBSuccess) {  // If there is a response
            uint32_t tempdouble = 0x00000000;
            PZEMVoltage = node.getResponseBuffer(0x0000) / 100.0;  // get voltage
            PZEMCurrent = node.getResponseBuffer(0x0001) / 100.0;  // get current
            tempdouble = (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002);  // get power
            PZEMPower = tempdouble / 10.0;
            
            tempdouble = (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004);  // get energy
            float currentPower = tempdouble / 10.0;  // Power in watts

            // Calculate energy
            float intervalHours = periodPZEM / 3600.0;  // Convert period from milliseconds to hours
            float energyThisPeriod = currentPower * intervalHours;  // Energy in watt-hours
            accumulatedEnergy += energyThisPeriod;  // Accumulate the total energy
  
            // Display on Serial Monitor
            Serial.print(PZEMVoltage, 1);
            Serial.print("V   ");
            Serial.print(PZEMCurrent, 3);
            Serial.print("A   ");
            Serial.print(PZEMPower, 1);
            Serial.print("W  ");
            //Serial.print(accumulatedEnergy, 2);
            //Serial.println(" Wh");
            Serial.println();

            // Log to SD Card
            myFile = SD.open("data.txt", FILE_WRITE);
            if (myFile) {
                Serial.println("Logging to SD Card...");
                // Calculate time
                unsigned long totalSeconds = millis() / 1000;
                unsigned int seconds = totalSeconds % 60;
                unsigned int minutes = (totalSeconds / 60) % 60;
                unsigned int hours = (totalSeconds / 3600);
                
                char timeString[9]; // Create a string to hold the formatted time
                sprintf(timeString, "%02u:%02u:%02u", hours, minutes, seconds); // Format the string to HH:MM:SS
                
                myFile.print("Time: ");
                myFile.print(timeString);  // Log formatted time
                myFile.print(", Voltage: ");
                myFile.print(PZEMVoltage);
                myFile.print(" V, Current: ");
                myFile.print(PZEMCurrent);
                myFile.print(" A, Power: ");
                myFile.print(PZEMPower);
                myFile.print(" W ");
                //myFile.print(accumulatedEnergy);
                //myFile.println(" Wh");
                myFile.close();  // close the file
            } else {
                Serial.println("Error opening data.txt");
            }

            // Reset timer for next period
            startMillisPZEM = currentMillisPZEM;
        } else {
            Serial.println("Failed to read modbus");
        }
    }

    /* 2 - LCD Display */
    currentMillisLCD = millis();
    if (currentMillisLCD - startMillisLCD >= periodLCD) {
        LCD.clear();
        LCD.setCursor(0, 0);
        LCD.print(PZEMVoltage, 1);
        LCD.print("V ");
        LCD.setCursor(9, 0);
        //LCD.print(PZEMEnergy, 0);
        //LCD.print("Wh");
        LCD.setCursor(0, 1);
        LCD.print(PZEMCurrent, 2);
        LCD.print("A ");
        LCD.setCursor(9, 1);
        LCD.print(PZEMPower, 1);
        LCD.print("W");
        startMillisLCD = currentMillisLCD;
    }
}


void preTransmission()                                                                                    /* transmission program when triggered*/
{
  
        /* 1- PZEM-017 DC Energy Meter */
        
        digitalWrite(MAX485_RE, 1);                                                                       /* put RE Pin to high*/
        digitalWrite(MAX485_DE, 1);                                                                       /* put DE Pin to high*/
        delay(1);                                                                                         // When both RE and DE Pin are high, converter is allow to transmit communication
}

void postTransmission()                                                                                   /* Reception program when triggered*/
{
        
        /* 1- PZEM-017 DC Energy Meter */
        
        delay(3);                                                                                         // When both RE and DE Pin are low, converter is allow to receive communication
        digitalWrite(MAX485_RE, 0);                                                                       /* put RE Pin to low*/
        digitalWrite(MAX485_DE, 0);                                                                       /* put DE Pin to low*/
}


void setShunt(uint8_t slaveAddr)                                                                          //Change the slave address of a node
{

        /* 1- PZEM-017 DC Energy Meter */
        
        static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
        static uint16_t registerAddress = 0x0003;                                                         /* change shunt register address command code */
        
        uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
        u16CRC = crc16_update(u16CRC, slaveAddr);                                                         // Calculate the crc16 over the 6bytes to be send
        u16CRC = crc16_update(u16CRC, SlaveParameter);
        u16CRC = crc16_update(u16CRC, highByte(registerAddress));
        u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
        u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
        u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));
      
        Serial.println("Change shunt address");
        preTransmission();                                                                                 /* trigger transmission mode*/
      
        Serial3.write(slaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        Serial3.write(SlaveParameter);
        Serial3.write(highByte(registerAddress));
        Serial3.write(lowByte(registerAddress));
        Serial3.write(highByte(NewshuntAddr));
        Serial3.write(lowByte(NewshuntAddr));
        Serial3.write(lowByte(u16CRC));
        Serial3.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (Serial3.available())                                                                        /* while receiving signal from Serial3 from meter and converter */
          {   
            Serial.print(char(Serial3.read()), HEX);                                                       /* Prints the response and display on Serial Monitor (Serial)*/
            Serial.print(" ");
          }
}

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr)                                            //Change the slave address of a node
{

        /* 1- PZEM-017 DC Energy Meter */
        
        static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
        static uint16_t registerAddress = 0x0002;                                                         /* Modbus RTU device address command code */
        uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
        u16CRC = crc16_update(u16CRC, OldslaveAddr);                                                      // Calculate the crc16 over the 6bytes to be send
        u16CRC = crc16_update(u16CRC, SlaveParameter);
        u16CRC = crc16_update(u16CRC, highByte(registerAddress));
        u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
        u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
        u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
      
        Serial.println("Change Slave Address");
        preTransmission();                                                                                 /* trigger transmission mode*/
      
        Serial3.write(OldslaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        Serial3.write(SlaveParameter);
        Serial3.write(highByte(registerAddress));
        Serial3.write(lowByte(registerAddress));
        Serial3.write(highByte(NewslaveAddr));
        Serial3.write(lowByte(NewslaveAddr));
        Serial3.write(lowByte(u16CRC));
        Serial3.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (Serial3.available())                                                                        /* while receiving signal from Serial3 from meter and converter */
          {   
            Serial.print(char(Serial3.read()), HEX);                                                       /* Prints the response and display on Serial Monitor (Serial)*/
            Serial.print(" ");
          }
}