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

#define levels 3
uint8_t level = 1;

// messages used in the ranging protocol
// TODO replace by enum
#define POLL 0
#define POLL_ACK 1
#define RANGE 2
#define RANGE_REPORT 3
#define RANGE_FAILED 4

// message flow state
volatile byte expectedMsgId = POLL_ACK;
// message sent/received state
volatile boolean sentAck = false;
volatile boolean receivedAck = false;
// timestamps to remember
DW1000Time timePollSent;
DW1000Time timePollAckReceived;
DW1000Time timeRangeSent;
// data buffer
#define LEN_DATA 16
byte data[LEN_DATA];
// watchdog and reset period
uint32_t lastActivity;
uint32_t resetPeriod = 10;
// reply times (same on both sides for symm. ranging)
uint16_t replyDelayTimeUS = 3000;

// Wifi stuffs
const char* ssid = "/*Enter Your SSID*/";        /*Enter Your SSID*/
const char* password = "/*Enter Your Password*/";  /*Enter Your Password*/

WiFiServer server(80); /* Instance of WiFiServer with port number 80 */
String request;

WiFiClient client;

TaskHandle_t wifiTask;

float data1 = 0;
float data2 = 0;
float data3 = 0;

#define M_DATA_COUNT 50
int indices[3];
float data_circle[M_DATA_COUNT][3];

void SendData() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.print(indices[0]);
  client.print("/");
  client.print(indices[0]);
  client.print("/");
  client.print(indices[0]);
  client.print("<br>\n");

  for (int i = 0; i < M_DATA_COUNT; i++) {
    client.print(i);
    client.print(":");
    client.print(data_circle[i][0]);
    client.print("/");
    client.print(data_circle[i][1]);
    client.print("/");
    client.print(data_circle[i][2]);
    client.print("<br>\n");
  }

  /*
  client.print(data1);
  client.print("/");
  client.print(data2);
  client.print("/");
  client.print(data3);
  client.print("\0");
  */
}

void ListenWifi(void * params) {
  Serial.print("wifi core ");
  Serial.println(xPortGetCoreID());

  while(true) {
  client = server.available();
  if(!client)
  {
    delay(1);
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
  delay(200);
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
  0);                // 실행될 코어
}

void setup() {
    // DEBUG monitoring
    Serial.begin(115200);

    // wifi server set
    SetWifi();

    // data set
    indices[0] = indices[1] = indices[2] = 0;

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
    transmitPoll();
    noteActivity();
}

void noteActivity() {
    // update activity timestamp, so that we do not reach "resetPeriod"
    lastActivity = millis();
}

void resetInactive() {
    // tag sends POLL and listens for POLL_ACK
    expectedMsgId = POLL_ACK;
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
    //Serial.print("Transmit poll " );
    //Serial.println(level);
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = POLL << 2;
    data[0] += level;
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
}

void transmitRange() {
    //Serial.print("Transmit range " );
    //Serial.println(level);
    DW1000.newTransmit();
    DW1000.setDefaults();
    data[0] = RANGE << 2;
    data[0] += level;
    // delay sending the message and remember expected future sent timestamp
    DW1000Time deltaTime = DW1000Time(replyDelayTimeUS, DW1000Time::MICROSECONDS);
    timeRangeSent = DW1000.setDelay(deltaTime);
    timePollSent.getTimestamp(data + 1);
    timePollAckReceived.getTimestamp(data + 6);
    timeRangeSent.getTimestamp(data + 11);
    DW1000.setData(data, LEN_DATA);
    DW1000.startTransmit();
    //Serial.print("Expect RANGE to be sent @ "); Serial.println(timeRangeSent.getAsFloat());
}

void receiver() {
    DW1000.newReceive();
    DW1000.setDefaults();
    // so we don't need to restart the receiver manually
    DW1000.receivePermanently(true);
    DW1000.startReceive();
}

void SwitchContext() {
    delay(10);
    if (level == levels) {
        level = 1;
    }
    else {
        level++;
    }
}

void loop() {
    //Serial.println(level);
    if (!sentAck && !receivedAck) {
        delay(1);
        // check if inactive
        if (millis() - lastActivity > resetPeriod) {
            // Serial.println("reset");
            resetInactive();
        }
        return;
    }
    // continue on any success confirmation
    if (sentAck) {
        sentAck = false;
        byte msgId = data[0] >> 2;
        if (msgId == POLL) {
            DW1000.getTransmitTimestamp(timePollSent);
            //Serial.print("Sent POLL @ "); Serial.println(timePollSent.getAsFloat());
        } else if (msgId == RANGE) {
            DW1000.getTransmitTimestamp(timeRangeSent);
            noteActivity();
        }
    }
    if (receivedAck) {
        receivedAck = false;
        // get message and parse
        DW1000.getData(data, LEN_DATA);
        byte msgId = data[0] >> 2;
        if (msgId != expectedMsgId) {
            // unexpected message, start over again
            //Serial.print("Received wrong message # "); Serial.println(msgId);
            expectedMsgId = POLL_ACK;
            transmitPoll();
            return;
        }
        if (msgId == POLL_ACK) {
            DW1000.getReceiveTimestamp(timePollAckReceived);
            expectedMsgId = RANGE_REPORT;
            transmitRange();
            noteActivity();
        } else if (msgId == RANGE_REPORT) {
            expectedMsgId = POLL_ACK;
            float curRange;
            memcpy(&curRange, data + 1, 4);
            //Serial.print("id: ");
            //Serial.print(level);
            //Serial.print(" dist: ");
            //Serial.print(curRange);
            //Serial.println(" m");

            /*
            Serial.print(data1);
            Serial.print("/");
            Serial.print(data2);
            Serial.print("/");
            Serial.print(data3);
            Serial.println();
            */

            data_circle[indices[level - 1]++][level - 1] = curRange;
            if (indices[level - 1] == M_DATA_COUNT) indices[level - 1] = 0;
            
            // send wifi test
            if (level == 1) {
                data1 = curRange;
            }
            else if (level == 2) {
                data2 = curRange;
            }
            else if (level == 3) {
                data3 = curRange;
            }

            SwitchContext();
            transmitPoll();
            noteActivity();
        } else if (msgId == RANGE_FAILED) {
            expectedMsgId = POLL_ACK;
            transmitPoll();
            noteActivity();
        }
    }
}
