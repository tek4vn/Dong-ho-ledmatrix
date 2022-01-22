
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <SPI.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Sensor.h>
#include <Max72xxPanel.h>
#include <ArduinoJson.h>
#define DHTPIN            12
#define DHTTYPE           DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);
// =======================================================================
// Connection data:
// =======================================================================
const char* ssid     = "__hh.g 2";                      // SSID
const char* password = "0978829485";                    // PASSWORD
String weatherKey = "openweathermap API";  // openweathermap API http://openweathermap.org/api
String weatherLang = "&lang=en";
String cityID = "1597591"; 
// =======================================================================


WiFiClient client;

String weatherMain = "";
String weatherDescription = "";
String weatherLocation = "";
String country;
int humidity;
int pressure;
//float temp;
//float tempMin, tempMax;
//int clouds;
float windSpeed;
String date;
String currencyRates;
String weatherString;

long period;
int offset = 1, refresh = 0;
int pinCS = 0; 
int numberOfHorizontalDisplays = 4; 
int numberOfVerticalDisplays = 1; 
String decodedMsg;
Max72xxPanel matrix = Max72xxPanel(pinCS, numberOfHorizontalDisplays, numberOfVerticalDisplays);
//matrix.cp437(true);

int wait = 60; 
int spacer = 4;
int width = 5 + spacer; 


void setup(void) {
  Serial.begin(115200);
  dht.begin();

  matrix.setIntensity(13); 

  // พิกัดเริ่มต้นของเมทริกซ์ 8 * 8
  matrix.setRotation(0, 1);        // 1
  matrix.setRotation(1, 1);        // 2
  matrix.setRotation(2, 1);        // 3
  matrix.setRotation(3, 1);        // 4


  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  } Serial.print("connected");
}
// =======================================================================
#define MAX_DIGITS 16
byte dig[MAX_DIGITS] = {0};
byte digold[MAX_DIGITS] = {0};
byte digtrans[MAX_DIGITS] = {0};
int updCnt = 0;
int dots = 0;
long dotTime = 0;
long clkTime = 0;
int dx = 0;
int dy = 0;
byte del = 0;
int h, m, s;

// =======================================================================
void loop(void) {
  if (updCnt <= 0) {
    updCnt = 10;
    Serial.println("Getting data ...");
    getWeatherData();
    getTime();
    Serial.println("Data loaded");
    clkTime = millis();
  }

  if (millis() - clkTime > 5000 && !del && dots) { //ทุก 15 วินาทีแสดงสภาพอากาศ
    ScrollText(weatherString);
    updCnt--;
    clkTime = millis();
  }
  DisplayTime();
  if (millis() - dotTime > 2000) {
    dotTime = millis();
    dots = !dots;
  }
}
// =======================================================================
void DisplayTime() {
  updateTime();
  matrix.fillScreen(LOW);
  int y = (matrix.height() - 8) / 2;
  if (s & 1) {
    matrix.drawChar(14, y, (String(":"))[0], HIGH, LOW, 1);
  }
  else {
    matrix.drawChar(14, y, (String(" "))[0], HIGH, LOW, 1);
  }

  String hour1 = String (h / 10);
  String hour2 = String (h % 10);
  String min1 = String (m / 10);
  String min2 = String (m % 10);
  String sec1 = String (s / 10);
  String sec2 = String (s % 10);
  int xh = 2;
  int xm = 19;
  //    int xs = 28;

  matrix.drawChar(xh, y, hour1[0], HIGH, LOW, 1);
  matrix.drawChar(xh + 6, y, hour2[0], HIGH, LOW, 1);
  matrix.drawChar(xm, y, min1[0], HIGH, LOW, 1);
  matrix.drawChar(xm + 6, y, min2[0], HIGH, LOW, 1);
  //    matrix.drawChar(xs, y, sec1[0], HIGH, LOW, 1);
  //    matrix.drawChar(xs+6, y, sec2[0], HIGH, LOW, 1);



  matrix.write();
}

// =======================================================================
void DisplayText(String text) {
  matrix.fillScreen(LOW);
  for (int i = 0; i < text.length(); i++) {

    int letter = (matrix.width()) - i * (width - 1);
    int x = (matrix.width() + 1) - letter;
    int y = (matrix.height() - 8) / 2;
    matrix.drawChar(x, y, text[i], HIGH, LOW, 1);
    matrix.write();

  }

}
// =======================================================================
void ScrollText (String text) {
  for ( int i = 0 ; i < width * text.length() + matrix.width() - 1 - spacer; i++ ) {
    if (refresh == 1) i = 0;
    refresh = 0;
    matrix.fillScreen(LOW);
    int letter = i / width;
    int x = (matrix.width() - 1) - i % width;
    int y = (matrix.height() - 8) / 2;

    while ( x + width - spacer >= 0 && letter >= 0 ) {
      if ( letter < text.length() ) {
        matrix.drawChar(x, y, text[letter], HIGH, LOW, 1);
      }
      letter--;
      x -= width;
    }
    matrix.write();
    delay(wait);
  }
}



const char *weatherHost = "api.openweathermap.org";

void getWeatherData()
{
  sensors_event_t event;
  Serial.print("connecting to "); Serial.println(weatherHost);
  if (client.connect(weatherHost, 80)) {
    client.println(String("GET /data/2.5/weather?id=") + cityID + "&units=metric&appid=" + weatherKey + weatherLang + "\r\n" +
                   "Host: " + weatherHost + "\r\nUser-Agent: ArduinoWiFi/1.1\r\n" +
                   "Connection: close\r\n\r\n");
  } else {
    Serial.println("connection failed");
  //  return;
  }
  String line;
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    Serial.println("w.");
    repeatCounter++;
  }
  while (client.connected() && client.available()) {
    char c = client.read();
    if (c == '[' || c == ']') c = ' ';
    line += c;
  }

  client.stop();

  DynamicJsonBuffer jsonBuf;
  JsonObject &root = jsonBuf.parseObject(line);
  if (!root.success())
  {
    Serial.println("parseObject() failed");
 //   return;
  }
  //weatherMain = root["weather"]["main"].as<String>();
  weatherDescription = root["weather"]["description"].as<String>();
  weatherDescription.toLowerCase();

  String deg = String(char('~' + 25));
  dht.temperature().getEvent(&event);
  weatherString = "T:" + String((int)event.temperature) + "C";

  //weatherString += weatherDescription;
  dht.humidity().getEvent(&event);
  weatherString += " H:" + String((int)event.relative_humidity) + "%";

  weatherString += " ";
 // weatherString += "I love you";
  

}

float utcOffset = +7; 
long localEpoc = 0;
long localMillisAtUpdate = 0;

void getTime()
{
  WiFiClient client;
  if (!client.connect("www.google.com", 80)) {
    Serial.println("connection to google failed");
    return;
  }

  client.print(String("GET / HTTP/1.1\r\n") +
               String("Host: www.google.com\r\n") +
               String("Connection: close\r\n\r\n"));
  int repeatCounter = 0;
  while (!client.available() && repeatCounter < 10) {
    delay(500);
    //Serial.println(".");
    repeatCounter++;
  }

  String line;
  client.setNoDelay(false);
  while (client.connected() && client.available()) {
    line = client.readStringUntil('\n');
    line.toUpperCase();
    if (line.startsWith("DATE: ")) {
      date = "     " + line.substring(6, 22);
      h = line.substring(23, 25).toInt();
      m = line.substring(26, 28).toInt();
      s = line.substring(29, 31).toInt();
      localMillisAtUpdate = millis();
      localEpoc = (h * 60 * 60 + m * 60 + s);
    }
  }
  client.stop();
}

// =======================================================================r

void updateTime()
{
  long curEpoch = localEpoc + ((millis() - localMillisAtUpdate) / 1000);
  long epoch = round(curEpoch + 3600 * utcOffset + 86400L) + 86400L;
  h = ((epoch  % 86400L) / 3600) % 24;
  m = (epoch % 3600) / 60;
  s = epoch % 60;
}
