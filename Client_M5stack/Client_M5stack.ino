#include <M5Stack.h>
#include <LoRa.h>

#define LORA_DEFAULT_SS_PIN    5
#define LORA_DEFAULT_RESET_PIN 36
#define LORA_DEFAULT_DIO0_PIN  26

bool registred = false;
uint8_t * data_;
char gateway_name[4];
uint16_t port;
uint16_t remotePort_;
uint16_t localPort_;
uint16_t length_;
IPAddress remoteIP_;
IPAddress localIP_;

typedef void (*pFunc2UInt8_t)(uint8_t, uint8_t);

void lora() {
  LoRa.setPins(LORA_DEFAULT_SS_PIN, LORA_DEFAULT_RESET_PIN, LORA_DEFAULT_DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.enableCrc();
  Serial.println("LoRa OK");
}

bool forign() {
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    if (gateway_name[i] != (char)LoRa.read()) {
      return true;
    }
  }
  return false;
}

void b1(pFunc2UInt8_t callcack) {
  if (registred) {
    (*callback)(0xB1, 0x00);
    return;
  }
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    gateway_name[i] = LoRa.read();
  }
  uint16_t first_port = (LoRa.read() << 8) | LoRa.read();
  uint16_t last_port = (LoRa.read() << 8) | LoRa.read();
  port = random(first_port, last_port + 1);
  LoRa.beginPacket();
  LoRa.write(0xB1);
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    LoRa.write(gateway_name[i]);
  }
  LoRa.write((port >> 8) & 0xFF);
  LoRa.write(port & 0xFF);
  LoRa.endPacket();
  (*callback)(0xB1, 0xFF);
}

void a2(pFunc2UInt8_t callcack) {
  if (forign() || registred) {
    (*callback)(0xA2, 0x00);
    return;
  }
  uint16_t port_ = (LoRa.read() << 8) | LoRa.read();
  if (port == port_) {
    if (LoRa.read() == 0xFF) {
      registred = true;
      (*callback)(0xA2, 0xFF);
    } else {
      port = 0;
      (*callback)(0xA2, 0x0F);
    }
  }
}

void a3(pFunc2UInt8_t callcack) {
  if (forign() || !registred) {
    (*callback)(0xA3, 0x00);
    return;
  }
  for (int i = 0; i < 4; i++) {
    remoteIP_[i] = LoRa.read();
  }
  remotePort_ = (LoRa.read() << 8) | LoRa.read();
  for (int i = 0; i < 4; i++) {
    localIP_[i] = LoRa.read();
  }
  localPort_ = (LoRa.read() << 8) | LoRa.read();
  length_ = (LoRa.read() << 8) | LoRa.read();
  data_ = (uint8_t*)malloc(sizeof(uint8_t) * length_);
  for (int i = 0; i < length_; i++) {
    data_[i] = LoRa.read();
  }
  if (localPort_ != port) {
    remoteIP_ = {0, 0, 0, 0};
    localIP_ = {0, 0, 0, 0};
    remotePort_ = 0;
    localPort_ = 0;
    length_ = 0;
    data_ = {0};
    return;
  }
  (*callback)(0xA3, 0xFF);
}

void b2(IPAddress remoteIP_, uint16_t remotePort_, uint8_t * data_, uint16_t length_) {
  if (!registred) {
    (*callback)(0xB2, 0x00);
    return;
  }
  LoRa.beginPacket();
  LoRa.write(0xB2);
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    LoRa.write(gateway_name[i]);
  }
  for (int i = 0; i < 4; i++) {
    LoRa.write(remoteIP_[i]);
  }
  LoRa.write((remotePort_ >> 8) & 0xFF);
  LoRa.write(remotePort_ & 0xFF);
  for (int i = 0; i < 4; i++) {
    LoRa.write(0);
  }
  LoRa.write((port >> 8) & 0xFF);
  LoRa.write(port & 0xFF);
  LoRa.write((length_ >> 8) & 0xFF);
  LoRa.write(length_ & 0xFF);
  for (int i = 0; i < length_; i++) {
    LoRa.write(data_[i]);
  }
  LoRa.endPacket();
  (*callback)(0xB2, 0xFF);
}

void client(pFunc2UInt8_t callcack) {
  if (LoRa.parsePacket()) {
    if (LoRa.available()) {
      switch (LoRa.read()) {
        case 0xA1:
          b1(callback);
          break;
         case 0xA2:
          a2(callback);
          break;
         case 0xA3:
          a3(callback);
          break;
       }
     }
  }
}

void callback(uint8_t key, uint8_t value) {
  M5.Lcd.print("0x");
  M5.Lcd.print(key, HEX);
  M5.Lcd.print(":");
  switch (key) {
    case 0xB1:
      {
        switch (value) {
          case 0x00:
            M5.Lcd.print("X");
            break;
          case 0xFF:
            M5.Lcd.print("V");
            break;
          }
      }
      break;
    case 0xA2:
      {
        switch (value) {
          case 0x00:
            M5.Lcd.print("X");
            break;
          case 0x0F:
            M5.Lcd.print("O");
            break;
          case 0xFF:
            M5.Lcd.print(port);
            break;
        }
      }
      break;
    case 0xA3:
      {
        switch (value) {
          case 0x00:
            M5.Lcd.print("X");
            break;
          case 0xFF:
            for (int i = 0; i < length_; i++) {
              M5.Lcd.print((char)data_[i]);
            }
            M5.Lcd.print(" ");
            // ответ / response
            uint8_t datax_[] = {(uint8_t)'H', (uint8_t)'E', (uint8_t)'L', (uint8_t)'L', (uint8_t)'O'};
            b2(remoteIP_, remotePort_, datax_, 5);
            break;
          }
      }
      break;
    case 0xB2:
      {
        switch (value) {
          case 0x00:
            M5.Lcd.print("X");
            break;
          case 0xFF:
            M5.Lcd.print("V");
            break;
          }
      }
      break;
  }
  M5.Lcd.print(" ");
}

void setup() {
  M5.begin();
  lora();
}

void loop() {
  client(callback);
}
