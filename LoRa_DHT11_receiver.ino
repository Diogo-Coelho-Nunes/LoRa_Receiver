#include <SPIFFS.h>

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Import WiFI library
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//LoRa frequency for Europe
#define BAND 866E6

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

const char* ssid     = "Nunes_hotspot";
const char* password = "nunescoelho";

//WiFiUDP ntpUDP;
//NTPClient timeClient(ntpUDP);

String LoRaData;

String formattedDate;
String day;
String hour;
String timestamp;
String temperature;
String humidity;
String pressure;
String Soil;
String readingID;


//Server connection on port 80
AsyncWebServer server(80);


String processor(const String& var){
  //Serial.println(var);
  if(var == "TEMPERATURE"){
    return temperature;
  }
  else if(var == "HUMIDITY"){
    return humidity;
  }
  else if(var == "PRESSURE"){
    return Soil;
  }
  else if(var == "TIMESTAMP"){
    return timestamp;
  }
  return String();
}

//Arranjar isto
String contaTemp(string Temp)
{
  time_t start, ending;
  String diferenca;

  if(Temp > 22)
  {
    time(&start);
  }
  else
  {
    time(&ending);
    return difftime(ending, start); 
  }
  return 0;

}

//Wifi Connection function
void connectWiFi(){
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  display.setCursor(0,20);
  display.print("Access web server at: ");
  display.setCursor(0,30);
  display.print(WiFi.localIP());
  display.display();
}

void getLoRaData() {
  Serial.print("Lora packet received: ");
  // Read packet
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    // LoRaData format: readingID/temperature&soilMoisture#batterylevel
    // String example: 1/27.43&654#95.34
    Serial.print(LoRaData); 
    
    // Get readingID, temperature and soil moisture
    int pos1 = LoRaData.indexOf('/');
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    temperature = LoRaData.substring(0, pos1);
    humidity = LoRaData.substring(pos1 +1, pos2);
    Soil = LoRaData.substring(pos2+1, pos3);

   while (LoRa.available()) {
      LoRaData = LoRa.readString();
      Serial.print(LoRaData);
    }

    //print RSSI of packet
    int rssi = LoRa.packetRssi();
    Serial.print(" with RSSI ");    
    Serial.println(rssi);

    display.clearDisplay();
    display.setCursor(0,0);
    display.print("LORA RECEIVER");
    display.setCursor(0,20);
    display.print("Temperature:");
    display.print(temperature);
    display.setCursor(0,30);
    display.print("AirHumidity:");
    display.print(humidity);
    display.setCursor(0,40);
    display.print("SoilMoisture:");
    display.print(Soil);
    
    display.display();
    }
  
}

void setup() { 
  //initialize Serial Monitor
  Serial.begin(115200);
  
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
   if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
   }
 
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", temperature.c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", humidity.c_str());
  });
  server.on("/soil", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", Soil.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", timestamp.c_str());
  });
 
  server.on("/winter", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/winter.jpg", "image/jpg");
  }); 
 
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER ");
  display.display();
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();
  
  connectWiFi();
  server.begin();
  
}

void loop() {  
//try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    //received a packet
    Serial.print("Received packet ");
    getLoRaData();

    //read packet
  }
}
