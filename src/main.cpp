#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <LoRa.h>

// * WiFi Configuration Magic
// see: https://github.com/tzapu/WiFiManager
#include <WiFiManager.h>

#include "main.h"

void setup() {
  Serial.begin(115200);
  while(!Serial);

  pinMode(LED_BUILTIN, OUTPUT);             // LED setup
  digitalWrite(LED_BUILTIN, 0);             // LED on
  WiFiManager wiFiManager;
  wiFiManager.autoConnect("Well Meter AP"); // connect WiFi or config AP
  digitalWrite(LED_BUILTIN, 1);             // LED off

  WiFi.mode(WIFI_STA);                      // Ensure AP is off
  client.setInsecure();

  LoRa.setPins(D3,D1,D2);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa init failed. Check your connections.");
    while (true);                           // if failed, do nothing
  }

  noInterrupts();

    // interupt to increment counter every second
    timer0_isr_init();
    timer0_attachInterrupt(ISR_timer0);
    timer0_write(ESP.getCycleCount() + 20000000L); // start in 250ms (80M/20M=1/4)

  interrupts();
}

void loop() {
  uint8_t crc;

  // try to parse packet
  int packetSize = LoRa.parsePacket();

  if (packetSize>6) {

    // Check sender MAC
    for(int c=0;c<6;c++) {
      if(LoRa.read() != (uint8_t)sender_mac[c]) return;
    }

    // Print sender MAC
    Serial.print("From ");
    for(int c=0;c<6;c++) {
      Serial.print((uint8_t)sender_mac[c], HEX);
      if(c<5) Serial.print(':');
      else    Serial.print(" of type '");
    }

    // Read payload type
    // TODO: type switch-case
    Serial.print((char)LoRa.read());

    // Read payload + CRC
    data =  LoRa.read() << 8; // msb
    data |= LoRa.read();      // lsb
    crc  =  LoRa.read();      // crc

    // Check CRC
    if( crc == lookupCRC8(data) ) {
      digitalWrite(LED_BUILTIN, 0); // LED on

      readings[readIndex] = data;
      readIndex = readIndex + 1;

      if (readIndex >= numReadings) {
        // ...wrap around to the beginning:
        readIndex = 0;
        firstLoop = false;
      }

      Serial.print("' value '");
      Serial.print(data);
      Serial.print("' with RSSI ");
      Serial.println(LoRa.packetRssi());
    } else {
      Serial.print("' CRC ERROR: '");
      Serial.print(data);
      Serial.println("'");
    }
  }
  
  if((send==true) & !firstLoop) {
    send = false;

    // Sort array (BubbleSort)
    for(int i=0; i<(numReadings-1); i++) {
        for(int o=0; o<(numReadings-(i+1)); o++) {
                if(readings[o] > readings[o+1]) {
                    int t = readings[o];
                    readings[o] = readings[o+1];
                    readings[o+1] = t;
                }
        }
    }

    // get median value
    uint16_t median;
    if((numReadings & 1) == 0) {
      median = (readings[(numReadings/2)-1]+readings[numReadings/2])/2;
    } else {
      median = readings[numReadings/2];
    }

    if (WiFi.status() == WL_CONNECTED) {
      if (!client.connect("api.thingspeak.com", 443)) {
        Serial.println("ERROR: connecting to api.thingspeak.com failed");
        return;
      } else  {
        client.print(String("GET /update?api_key=") + thingspeakApiKey +
                    "&field1=" + String(median) +
                    " HTTP/1.1\r\n" +
                    "Host: api.thingspeak.com\r\n" +
                    "User-Agent: WellMeterESP8266\r\n" +
                    "Connection: close\r\n\r\n");

        Serial.print("INFO: update request sent with value: '");
        Serial.print(median);
        Serial.println("'");
        while (client.connected()) {
          String line = client.readStringUntil('\n');
        }
      }
    } else {
      Serial.println("WARN: WiFi not connected!");
    }
  }
  digitalWrite(LED_BUILTIN, 1); // LED off
}

/*
 * lookup a CRC8 over data
 */

uint8_t lookupCRC8(uint16_t data) {
  uint8_t CRC = 0xFF; //inital value
  CRC ^= (uint8_t)(data >> 8); //start with MSB
  CRC = CRC8LookupTable[CRC >> 4][CRC & 0xF]; //look up table [MSnibble][LSnibble]
  CRC ^= (uint8_t)data; //use LSB
  CRC = CRC8LookupTable[CRC >> 4][CRC & 0xF]; //look up table [MSnibble][LSnibble]
  return CRC;
}

/*
 * ISR let it blink every 4 seconds
 */

void ICACHE_RAM_ATTR inline ISR_timer0() {
  seconds = seconds + 1;
  if(seconds > 119) {
    seconds = 0;
    send = true;
  }

  timer0_write(ESP.getCycleCount() + 80000000L); // 80M/80M = 1s
}