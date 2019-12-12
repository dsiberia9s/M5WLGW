#include <M5Stack.h>
#include <LoRa.h>

#define LORA_DEFAULT_SS_PIN    5
#define LORA_DEFAULT_RESET_PIN 36
#define LORA_DEFAULT_DIO0_PIN  26

bool registred = false;
uint16_t port;
char gateway_name[4];

void lora() {
  LoRa.setPins(LORA_DEFAULT_SS_PIN, LORA_DEFAULT_RESET_PIN, LORA_DEFAULT_DIO0_PIN);
  if (!LoRa.begin(433E6)) {
    M5.Lcd.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.enableCrc();
  M5.Lcd.println("LoRa init OK");
}

bool forign() {
  for (int i = 0; i < sizeof(gateway_name) / sizeof(char); i++) {
    if (gateway_name[i] != (char)LoRa.read()) {
      return true;
    }
  }
  return false;
}

void b1() {
  if (registred) {
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
}

void reg() {
  if (forign() || registred) {
    return;
  }
  uint16_t port_ = (LoRa.read() << 8) | LoRa.read();
  if (port == port_) {
    if (LoRa.read() == 0xFF) {
      registred = true;
      M5.Lcd.print("Assigned: ");
      M5.Lcd.println(port_);
    } else {
      port = 0;
    }
  }
}

void b2(IPAddress remoteIP_, uint16_t remotePort_, uint8_t * data_, uint16_t length_) {
  if (!registred) {
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
}

void udp_in() {
  if (forign() || !registred) {
    return;
  }
  IPAddress remoteIP_, localIP_;
  uint16_t remotePort_, localPort_;
  for (int i = 0; i < 4; i++) {
    remoteIP_[i] = LoRa.read();
  }
  remotePort_ = (LoRa.read() << 8) | LoRa.read();
  for (int i = 0; i < 4; i++) {
    localIP_[i] = LoRa.read();
  }
  localPort_ = (LoRa.read() << 8) | LoRa.read();
  uint16_t length_ = (LoRa.read() << 8) | LoRa.read();
  uint8_t * data_ = (uint8_t*)malloc(sizeof(uint8_t) * length_);
  for (int i = 0; i < length_; i++) {
    data_[i] = LoRa.read();
  }
  // forign data. don't touch it
  if (localPort_ != port) {
    remoteIP_ = {0, 0, 0, 0};
    localIP_ = {0, 0, 0, 0};
    remotePort_ = 0;
    localPort_ = 0;
    length_ = 0;
    data_ = {0};
  }
  M5.Lcd.print(remoteIP_);
  M5.Lcd.print(": ");
  M5.Lcd.println(remotePort_);
  M5.Lcd.print(localIP_);
  M5.Lcd.print(": ");
  M5.Lcd.println(localPort_);
  for (int i = 0; i < length_; i++) {
    M5.Lcd.print((char)data_[i]);
  }
  uint8_t datax_[] = {0, 1};
  b2(remoteIP_, remotePort_, datax_, 2);
}

void gateway_client() {
  if (LoRa.parsePacket()) {
    if (LoRa.available()) {
      int x = LoRa.read();
      Serial.print(x, HEX);
      switch (x) {
        case 0xA1:
          b1();
          break;
         case 0xA2:
          reg();
          break;
         case 0xA3:
          udp_in();
          break;
       }
     }
  }
}

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(3);
  lora();
}

void loop() {
  gateway_client();
}
