#include <M5Stack.h>
#include <SPI.h>
#include <LoRa.h>
#include "WiFi.h"
#include "AsyncUDP.h"

#define LORA_DEFAULT_SS_PIN    5
#define LORA_DEFAULT_RESET_PIN 36
#define LORA_DEFAULT_DIO0_PIN  26

char gateway_name[] = {'G', 'W', '0', '1'};
uint16_t first_port = 48654;
uint16_t last_port = 48718;
uint16_t * clients;
AsyncUDP * udps;

unsigned long last = 0;
unsigned long interval = 5000;

void wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("", "");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    M5.Lcd.println("Wi-Fi Failed");
    while (1);
  }
  M5.Lcd.print("Local IP: ");
  M5.Lcd.println(WiFi.localIP());
}

void lora() {
  LoRa.setPins(LORA_DEFAULT_SS_PIN, LORA_DEFAULT_RESET_PIN, LORA_DEFAULT_DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    M5.Lcd.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.enableCrc();
  M5.Lcd.println("LoRa init succeeded.");
}

void preferences() {
  uint16_t s = last_port - first_port;    
  clients = (uint16_t *) malloc(sizeof(uint16_t) * s);
  for (int i = 0; i < s; i++) {
    clients[i] = 0; // prefs
  }
  udps = new AsyncUDP[s];
  for (int i = 0; i < s; i++) {
    if (clients[i]) {
      udps[i].listen(clients[i]);
    }
  }
}

bool forign() {
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    if (gateway_name[i] != (char)LoRa.read()) {
      return true;
    }
  }
  return false;
}

void a1() {
  if (millis() - last >= interval) {
    while (LoRa.beginPacket() == 0) {
      delay(100);
    }
    LoRa.beginPacket();
    LoRa.write(0xA1);
    for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
      LoRa.write((uint8_t)gateway_name[i]);
    }
    LoRa.write((first_port >> 8) & 0xFF);
    LoRa.write(first_port & 0xFF);
    LoRa.write((last_port >> 8) & 0xFF);
    LoRa.write(last_port & 0xFF);
    LoRa.endPacket(true);
    last = millis();
    M5.Lcd.print("*");
  }
}

void a2() {
  if (forign()) {
    return;
  }
  uint16_t port = (LoRa.read() << 8) | LoRa.read();
  M5.Lcd.print(port);
  for (int i = 0; i < (last_port - first_port); i++) {
    if (port == clients[i]) {
      while (LoRa.beginPacket() == 0) {
        delay(100);
      }
      LoRa.beginPacket();
      LoRa.write(0xA2);
      for (int j = 0; j < sizeof(gateway_name) / sizeof(byte); j++) {
        LoRa.write(gateway_name[j]);
      }
      LoRa.write((port >> 8) & 0xFF);
      LoRa.write(port & 0xFF);
      LoRa.write(0x00);
      LoRa.endPacket(true);
      M5.Lcd.print("port busy");
      return; 
    }
  }
  for (int i = 0; i < (last_port - first_port); i++) {
    if (!clients[i]) {
      clients[i] = port;
      udps[i].listen(clients[i]);
      while (LoRa.beginPacket() == 0) {
        delay(100);
      }
      LoRa.beginPacket();
      LoRa.write(0xA2);
      for (int j = 0; j < sizeof(gateway_name) / sizeof(char); j++) {
        LoRa.write(gateway_name[j]);
      }
      LoRa.write((port >> 8) & 0xFF);
      LoRa.write(port & 0xFF);
      LoRa.write(0xFF);
      LoRa.endPacket(true);
      M5.Lcd.print("port allowed");
      return;
    }
  }
}

void udp_out() {
  if (forign()) {
    return;
  }
  IPAddress remoteIP_, localIP_;
  uint16_t remotePort_, localPort_;
  for (int i = 0; i < 4; i++) {
    remoteIP_[i] = LoRa.read();
  }
  remotePort_ = (LoRa.read() << 8) | LoRa.read();
  for (int i = 0; i < 4; i++) {
    LoRa.read();
  }
  //localIP_ = WiFi.localIP();
  localPort_ = (LoRa.read() << 8) | LoRa.read();
  uint16_t length_ = (LoRa.read() << 8) | LoRa.read();
  uint8_t * data_ = (uint8_t*)malloc(sizeof(uint8_t) * length_);
  for (int i = 0; i < length_; i++) {
    data_[i] = LoRa.read();
  }
  for (int i = 0; i < (last_port - first_port); i++) {
    if (clients[i] == localPort_) {
      udps[i].writeTo(data_, length_, remoteIP_, remotePort_);
      M5.Lcd.print("LoRa -> Wi-Fi: ");
      M5.Lcd.print(remoteIP_);
      M5.Lcd.print(": ");
      M5.Lcd.print(remotePort_);
      break;
    }
  }
  delay(5);
}

void a3() {
  for (int i = 0; i < (last_port - first_port); i++) {
    if (clients[i]) {
      if (udps[i].connected()) {
        udps[i].onPacket([](AsyncUDPPacket packet) {
          while (LoRa.beginPacket() == 0) {
            delay(100);
          }
          LoRa.beginPacket();
          LoRa.write(0xA3);
          for (int j = 0; j < sizeof(gateway_name) / sizeof(char); j++) {
            LoRa.write(gateway_name[j]);
          }
        
          for (int j = 0; j < 4; j++) {
            LoRa.write(packet.remoteIP()[j]);
          }
          LoRa.write((packet.remotePort() >> 8) & 0xFF);
          LoRa.write(packet.remotePort() & 0xFF);
          for (int j = 0; j < 4; j++) {
            LoRa.write(packet.localIP()[j]);
          }
          LoRa.write((packet.localPort() >> 8) & 0xFF);
          LoRa.write(packet.localPort() & 0xFF);
          LoRa.write((packet.length() >> 8) & 0xFF);
          LoRa.write(packet.length() & 0xFF);
          for (int j = 0; j < packet.length(); j++) {
            LoRa.write(packet.data()[j]);
          }
          LoRa.endPacket(true);
          M5.Lcd.print("@");
        });
      }
      delay(5);
    }
  }
}

void gateway() {
  if (LoRa.parsePacket()) {
    if (LoRa.available()) {
      switch (LoRa.read()) {
        case 0xB1:
          a2();
          break;
        case 0xB2:
          udp_out();
          break;
      }
    }
  }
  a1();
  a3();
}

void setup() {
  M5.begin();
  wifi();
  lora();
  preferences();
}

void loop() {
  gateway();
}
