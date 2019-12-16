#include <M5Stack.h>
#include <LoRa.h>
#include "WiFi.h"
#include "AsyncUDP.h"

#define LORA_DEFAULT_SS_PIN    5
#define LORA_DEFAULT_RESET_PIN 36
#define LORA_DEFAULT_DIO0_PIN  26

uint16_t first_port = 48654;
uint16_t last_port = 48718;
unsigned long interval = 5000;
char gateway_name[4];
uint16_t * clients;
unsigned long last = 0;
AsyncUDP * udps;

typedef void (*pFunc2UInt8_t)(uint8_t, uint8_t);

void wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin("", "");
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wi-Fi Failed");
    while (1);
  }
  Serial.println("Wi-Fi OK");
}

void lora() {
  LoRa.setPins(LORA_DEFAULT_SS_PIN, LORA_DEFAULT_RESET_PIN, LORA_DEFAULT_DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Failed");
    while (1);
  }
  LoRa.enableCrc();
  Serial.println("LoRa OK");
}

void preferences() {
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    gateway_name[i] = (char)random(0, 256);
  }
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

void a1(pFunc2UInt8_t callcack) {
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
    (*callcack)(0xA1, 0);
  }
}

void a2(pFunc2UInt8_t callcack) {
  if (forign()) {
    return;
  }
  uint16_t port = (LoRa.read() << 8) | LoRa.read();
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
      (*callcack)(0xA2, 0x00);
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
      (*callcack)(0xA2, 0xFF);
      return;
    }
  }
}

void c1(pFunc2UInt8_t callcack) {
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
  localPort_ = (LoRa.read() << 8) | LoRa.read();
  uint16_t length_ = (LoRa.read() << 8) | LoRa.read();
  uint8_t * data_ = (uint8_t*)malloc(sizeof(uint8_t) * length_);
  for (int i = 0; i < length_; i++) {
    data_[i] = LoRa.read();
  }
  for (int i = 0; i < (last_port - first_port); i++) {
    if (clients[i] == localPort_) {
      udps[i].writeTo(data_, length_, remoteIP_, remotePort_);
      (*callcack)(0xC1, 0);
      break;
    }
  }
  delay(5);
}

void a3(pFunc2UInt8_t callcack) {
  for (int i = 0; i < (last_port - first_port); i++) {
    if (clients[i]) {
      if (udps[i].connected()) {
        udps[i].onPacket([callcack](AsyncUDPPacket packet) {
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
          (*callcack)(0xA3, 0);
        });
      }
      delay(5);
    }
  }
}

void gateway(pFunc2UInt8_t callcack) {
  if (LoRa.parsePacket()) {
    if (LoRa.available()) {
      switch (LoRa.read()) {
        case 0xB1:
          a2(callcack);
          break;
        case 0xB2:
          c1(callcack);
          break;
      }
    }
  }
  a1(callcack);
  a3(callcack);
}

void callback(uint8_t key, uint8_t value) {
  switch (key) {
    case 0xA1:
      M5.Lcd.print("*");
      break;
    case 0xA2:
      {
        if (value) {
          M5.Lcd.print("(V)");
        } else {
          M5.Lcd.print("(X)");  
        }
      }
      break;
    case 0xA3:
      M5.Lcd.print("(C<-G)");
      break;
    case 0xC1:
      M5.Lcd.print("(N<-G)");
      break;
  }
}

void setup() {
  M5.begin();
  wifi();
  lora();
  preferences();
}

void loop() {
  gateway(callback);
}
