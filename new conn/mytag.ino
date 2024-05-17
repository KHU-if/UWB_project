/*
 * Copyright (c) 2015 by Thomas Trojer <thomas@trojer.net>
 * Decawave DW1000 library for arduino.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file RangingTag.ino
 * Use this to test two-way ranging functionality with two DW1000. This is
 * the tag component's code which polls for range computation. Addressing and
 * frame filtering is currently done in a custom way, as no MAC features are
 * implemented yet.
 *
 * Complements the "RangingAnchor" example sketch.
 *
 * @todo
 *  - use enum instead of define
 *  - move strings to flash (less RAM consumption)
 */

#include <SPI.h>
#include <DW1000.h>

#include <WiFi.h>

// SPI 통신에 사용할 핀들을 정의합니다.
const uint8_t PIN_SCK = 18;  
const uint8_t PIN_MOSI = 23; 
const uint8_t PIN_MISO = 19;  
const uint8_t PIN_SS = 2;  
const uint8_t PIN_RST = 15;  
const uint8_t PIN_IRQ = 5;

#define levels 1
uint8_t level = 1;

// messages used in the ranging protocol
// TODO replace by enum
#define POLL 0
#define POLL_ACK 1

// message sent/received state
volatile boolean sentAck = false;
volatile boolean receivedAck = false;
// timestamps to remember
DW1000Time timePollSent;
DW1000Time timePollReceived1;
DW1000Time timePollReceived2;
DW1000Time timePollReceived3;
DW1000Time timePollAckSent1;
DW1000Time timePollAckSent2;
DW1000Time timePollAckSent3;
DW1000Time timePollAckReceived1;
DW1000Time timePollAckReceived2;
DW1000Time timePollAckReceived3;
// data buffer
#define LEN_DATA 16
byte data[LEN_DATA];
// watchdog and reset period
uint32_t lastActivity;
uint32_t resetPeriod = 2000;
// reply times (same on both sides for symm. ranging)
uint16_t replyDelayTimeUS = 3000;

// Wifi stuffs
const char* ssid = "HAM23";        /*Enter Your SSID*/
const char* password = "hamhamham";  /*Enter Your Password*/

WiFiServer server(80); /* Instance of WiFiServer with port number 80 */
String request;

WiFiClient client;

TaskHandle_t wifiTask;

float data1 = 0;
float data2 = 0;
float data3 = 0;

void SendData() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.print(data1);
  client.print("/");
  client.print(data2);
  client.print("/");
  client.print(data3);
  client.print("\0");
}

void ListenWifi(void * params) {
  while(true) {
  client = server.available();
  if(!client)
  {
    continue;
  }

  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      request += c;

      if (c == '\n')
      {
        SendData();
        break;
      }
    }
  }

  delay(1);
  request="";
  client.stop();
  }
}

void SetWifi() {
  Serial.print("Connecting to: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(100);
  }
  Serial.print("\n");
  Serial.print("Connected to Wi-Fi ");
  Serial.println(WiFi.SSID());
  delay(1000);
  server.begin(); /* Start the HTTP web Server */
  Serial.print("Connect to IP Address: ");
  Serial.print("http://");
  Serial.println(WiFi.localIP());

  xTaskCreatePinnedToCore(
  ListenWifi,          // 태스크 함수
  "WifiTask",           // 테스크 이름
  10000,             // 스택 크기(워드단위)
  NULL,              // 태스크 파라미터
  1,                 // 태스크 우선순위
  &wifiTask,            // 태스크 핸들
  1);                // 실행될 코어
}

void setup() {
    // DEBUG monitoring
    Serial.begin(115200);

    // wifi server set
    SetWifi();

    Serial.println(F("### DW1000-arduino-ranging-tag ###"));
    // initialize the driver
    DW1000.begin(PIN_IRQ, PIN_RST);
    DW1000.select(PIN_SS);
    Serial.println("DW1000 initialized ...");
    // general configuration
    DW1000.newConfiguration();
    DW1000.setDefaults();
    DW1000.setDeviceAddress(2);
    DW1000.setNetworkId(10);
    DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
    DW1000.commitConfiguration();
    Serial.println(F("Committed configuration ..."));
    // DEBUG chip info and registers pretty printed
    char msg[128];
    DW1000.getPrintableDeviceIdentifier(msg);
    Serial.print("Device ID: "); Serial.println(msg);
    DW1000.getPrintableExtendedUniqueIdentifier(msg);
    Serial.print("Unique ID: "); Serial.println(msg);
    DW1000.getPrintableNetworkIdAndShortAddress(msg);
    Serial.print("Network ID & Device Address: "); Serial.println(msg);
    DW1000.getPrintableDeviceMode(msg);
    Serial.print("Device mode: "); Serial.println(msg);
    // attach callback for (successfully) sent and received messages
    DW1000.attachSentHandler(handleSent);
    DW1000.attachReceivedHandler(handleReceived);
    // anchor starts by transmitting a POLL message
    receiver();
    noteActivity();
}

void noteActivity() {
    // update activity timestamp, so that we do not reach "resetPeriod"
    lastActivity = millis();
}

void resetInactive() {
    // tag sends POLL and listens for POLL_ACK
    Serial.println("reset inactive");
    transmitPoll();
    noteActivity();
}

void handleSent() {
    // status change on sent success
    sentAck = true;
}

void handleReceived() {
    // status change on received success
    receivedAck = true;
}

void transmitPoll() {
    Serial.println("Transmit poll " );
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = POLL << 2;
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void receiver() {
    DW1000.newReceive();
    DW1000.setDefaults();
    // so we don't need to restart the receiver manually
    DW1000.receivePermanently(true);
    DW1000.startReceive();
}

bool doing = false;
bool checked = false;
bool done1 = true;

void loop() {
    if (!sentAck && !receivedAck) {
        // check if inactive'
        if (millis() - lastActivity > resetPeriod) {
            resetInactive();
            return;
        }
    }
    // continue on any success confirmation
    if (!doing) {
      Serial.println("do");
      sentAck = false;
      checked = false;
      doing = true;
      done1 = false;  
      transmitPoll();
    }
    
    if (sentAck && !checked) {
        DW1000.getTransmitTimestamp(timePollSent);
        checked = true;
    }
    if (receivedAck) {
      Serial.println("recv");
      receivedAck = false;
      // get message and parse
      DW1000.getData(data, LEN_DATA);
      byte msgId = data[0] >> 2;
      byte id = data[0] & 3;
      if (id == 1) {
        if (msgId == POLL_ACK) {
            DW1000.getReceiveTimestamp(timePollAckReceived1);
            memcpy(data + 1, &timePollReceived1, 4);
            memcpy(data + 5, &timePollAckSent1, 4);
            noteActivity();

            Serial.print("id ");
            Serial.print(id);
            Serial.print(" ");
            Serial.print(timePollSent);
            Serial.print(" ");
            Serial.print(timePollReceived1);
            Serial.print(" ");
            Serial.print(timePollAckSent1);
            Serial.print(" ");
            Serial.println(timePollAckReceived1);

            done1 = true;
        }
      }

      if (done1) {
        doing = false;
        delay(1000);
      }
    }
}
