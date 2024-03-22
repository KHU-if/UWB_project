/*
  ESP32 WiFi STA Mode
  http:://www.electronicwings.com
*/

#include <WiFi.h>

const char* ssid = "*Your SSID*";         /*Enter Your SSID*/
const char* password = "*Your Password*"; /*Enter Your Password*/

WiFiServer server(80); /* Instance of WiFiServer with port number 80 */
String request;

WiFiClient client;

TaskHandle_t wifiTask;

int data = 0;

void SendData() {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  clinet.println();

  client.println(data);
}

void ListenWifi() {
  while(true) {
  client = server.available();
  if(!client)
  {
    return;
  }

  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      request += c;

      if (c == '\n')
      {
        sendData();
        break;
      }
    }
  }

  delay(1);
  request="";
  client.stop();
  }
}

void setup() 
{
  Serial.begin(115200);
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

void loop() {
  delay(100);
  data++;
  if (data == 100) {
    data = 0;
  }
}