#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

extern "C" {
#include<user_interface.h>
}

String WIFI_SSID;
String WIFI_PASS;

const char* ssid = "AP";
const char* password = "123456789";

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,0);
IPAddress subnet(255,255,255,0);
int buttonPin = D4;
int buttonState = 0;
int alarmState = 1;

WiFiUDP Udp;
WiFiUDP wifi_udp;
unsigned int localUdpPort = 5000; // local port to listen on
char incomingPacket[255]; // buffer for incoming packets
char wifiPacket[255];
char controlPacket[255] = "&";
int packetSize = 0;
int wifipacketSize = 0;
String strs[2];
int StringCount = 0;
WiFiClient wifiClient;

void setup()
{
Serial.begin(115200);
Serial.println();

Serial.print("Setting soft-AP configuration ... ");
Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

Serial.print("Setting soft-AP ... ");
Serial.println(WiFi.softAP(ssid,password) ? "Ready" : "Failed!");

Serial.print("Soft-AP IP address = ");
Serial.println(WiFi.softAPIP());

while (WiFi.softAPgetStationNum() == 0)
  {
  delay(100);
  Serial.println("Wait for client");
  }

  Serial.printf("Device MAC address = %s\n", WiFi.softAPmacAddress().c_str());
  Udp.begin(localUdpPort);
  Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.softAPIP().toString().c_str(), localUdpPort);
  
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(buttonPin, INPUT);  
}


void loop()
  {
  delay(100);

  int packetSize = Udp.parsePacket();
  if (packetSize) {
    command_received();
  }

  buttonState = digitalRead(buttonPin);
  if (buttonState == HIGH) {
    alarm_on();
  }
  else {
    alarm_off();
  }

  strs[2];
  StringCount = 0;
}

void command_received() {
    Serial.println();
    Serial.printf("Received from %s, port %d\n", Udp.remoteIP().toString().c_str(), Udp.remotePort());    
    int len = Udp.read(incomingPacket, 255);
    if (len > 0) {
    incomingPacket[len] = 0;
    }
    
    if (*incomingPacket == *controlPacket) {
      wifi_received();
    } else {
    Serial.println(incomingPacket);
    String str = incomingPacket;
    // Split the string into substrings
    while (str.length() > 0)
    {
      int index = str.indexOf(' ');
      if (index == -1) // No space found
      {
        strs[StringCount++] = str;
        break;
      }
      else
      {
        strs[StringCount++] = str.substring(0, index);
        str = str.substring(index+1);
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
      WIFI_SSID = strs[0];
      WIFI_PASS = strs[1];
      }
    
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(incomingPacket);
    Udp.endPacket();

    if (WiFi.status() == WL_CONNECTED) {
      String slug = incomingPacket;
      String link = "http://192.168.0.107:8000/" + slug + "/";
      HTTPClient http;
      http.begin(wifiClient, link);
      int httpCode = http.GET();
      if (httpCode > 0) {
        String payload = http.getString();
        Serial.println(payload);

        StaticJsonDocument<2000> doc; 
			  DeserializationError error = deserializeJson(doc, payload);
			  if (error) {
				Serial.print(F("deserializeJson() failed: "));
				Serial.println(error.c_str());
				return;
			  }
			  const char* detail = doc["detail"];
        const char* project_name = doc["project_name"];
			  const char* domain_name = doc["domain_name"];
        Serial.println(detail);
			  Serial.println(project_name);
			  Serial.println(domain_name);

        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(detail);
        Udp.endPacket();
      }
    http.end();
    }
}
}

void wifi_received() {
  Serial.println("WiFi connect ... ");  
  int connect_try = 3;
  while (connect_try > 0) {
     if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      delay(3000);
     }
    connect_try--;
    }

  if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected!");
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Connected!");
      Udp.endPacket();
      delay(1000);      
      WiFi.softAPdisconnect (true);
    } else {      
      Serial.println("Wrong login/password!");
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Wrong login/password!");
      Udp.endPacket();
    }
}

void alarm_on() {
  digitalWrite(BUILTIN_LED, HIGH);
  delay(500);
  digitalWrite(BUILTIN_LED, LOW);
  delay(500);
  if (alarmState == 1) {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Alarm!");
      Udp.endPacket();
      alarmState = 0;
  }
}

void alarm_off() {
  digitalWrite(BUILTIN_LED, LOW);
    if (alarmState == 0) {
      Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
      Udp.write("Success!");
      Udp.endPacket();
      alarmState = 1;
  }
}

