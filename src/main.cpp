/*
MQTT NeoPixel Status Multiple Improved
Written by: Alexander van der Sar

website: https://www.vdsar.net/build-status-light-for-devops/
Repository: https://github.com/arvdsar/MQTT_NeoPixel_Status_Multiple_Improved
3D Print design: https://www.thingiverse.com/thing:4665511

This is the AWS version of the MQTT_NeoPixel_Status_Multiple. WiFiManager library is replaced with IotWebConf library
Connecting to AWS IoT Core with MQTT requires some additional code.
This library keeps the configuration portal available so you don't have to reflash to change settings.
It depends on IoTWebConf Libary v3.0.0 (not compatible with 2.x)

IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
and minimize distance between Arduino and first pixel.  Avoid connecting
on a live circuit...if you must, connect GND first.

*/

#define VERSIONNUMBER "v0.1 - 24-12-2020"

#include <Arduino.h>
#include <Stream.h>
#include <time.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266WiFi.h>        //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <IotWebConf.h>         // https://github.com/prampec/IotWebConf
#include <IotWebConfUsing.h>    // This loads aliases for easier class names.
#ifdef ESP8266
# include <ESP8266HTTPUpdateServer.h>
#elif defined(ESP32)
# include <IotWebConfESP32HTTPUpdateServer.h>
#endif



//AWS
#include "sha256.h"
#include "Utils.h"


//WEBSockets
#include <Hash.h>
#include <WebSocketsClient.h>

//MQTT PUBSUBCLIENT LIB
#include <PubSubClient.h>

//AWS MQTT Websocket
#include "Client.h"
#include "AWSWebSocketClient.h"
#include "CircularByteBuffer.h"

extern "C" {
  #include "user_interface.h"
}

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

int port = 443;

/* uncomment the following line to use an alternate root CA if the first one does not work */
// #define ALTERNATE_ROOT_CA

#ifndef ALTERNATE_ROOT_CA

// AWS root certificate - expires 2037
const char ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIEkjCCA3qgAwIBAgITBn+USionzfP6wq4rAfkI7rnExjANBgkqhkiG9w0BAQsF
ADCBmDELMAkGA1UEBhMCVVMxEDAOBgNVBAgTB0FyaXpvbmExEzARBgNVBAcTClNj
b3R0c2RhbGUxJTAjBgNVBAoTHFN0YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4x
OzA5BgNVBAMTMlN0YXJmaWVsZCBTZXJ2aWNlcyBSb290IENlcnRpZmljYXRlIEF1
dGhvcml0eSAtIEcyMB4XDTE1MDUyNTEyMDAwMFoXDTM3MTIzMTAxMDAwMFowOTEL
MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv
b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj
ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM
9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw
IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6
VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L
93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm
jgSubJrIqg0CAwEAAaOCATEwggEtMA8GA1UdEwEB/wQFMAMBAf8wDgYDVR0PAQH/
BAQDAgGGMB0GA1UdDgQWBBSEGMyFNOy8DJSULghZnMeyEE4KCDAfBgNVHSMEGDAW
gBScXwDfqgHXMCs4iKK4bUqc8hGRgzB4BggrBgEFBQcBAQRsMGowLgYIKwYBBQUH
MAGGImh0dHA6Ly9vY3NwLnJvb3RnMi5hbWF6b250cnVzdC5jb20wOAYIKwYBBQUH
MAKGLGh0dHA6Ly9jcnQucm9vdGcyLmFtYXpvbnRydXN0LmNvbS9yb290ZzIuY2Vy
MD0GA1UdHwQ2MDQwMqAwoC6GLGh0dHA6Ly9jcmwucm9vdGcyLmFtYXpvbnRydXN0
LmNvbS9yb290ZzIuY3JsMBEGA1UdIAQKMAgwBgYEVR0gADANBgkqhkiG9w0BAQsF
AAOCAQEAYjdCXLwQtT6LLOkMm2xF4gcAevnFWAu5CIw+7bMlPLVvUOTNNWqnkzSW
MiGpSESrnO09tKpzbeR/FoCJbM8oAxiDR3mjEH4wW6w7sGDgd9QIpuEdfF7Au/ma
eyKdpwAJfqxGF4PcnCZXmTA5YpaP7dreqsXMGz7KQ2hsVxa81Q4gLv7/wmpdLqBK
bRRYh5TmOTFffHPLkIhqhBGWJ6bt2YFGpn6jcgAKUj6DiAdjd4lpFw85hdKrCEVN
0FE6/V1dN2RMfjCyVSRCnTawXZwXgWHxyvkQAiSr6w10kY17RSlQOYiypok1JR4U
akcjMS9cmvqtmg5iUaQqqcT5NJ0hGA==
-----END CERTIFICATE-----
)EOF";

#else

// AWS root certificate (Verisign) - expires 2036
const char ca[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIE0zCCA7ugAwIBAgIQGNrRniZ96LtKIVjNzGs7SjANBgkqhkiG9w0BAQUFADCB
yjELMAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQL
ExZWZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJp
U2lnbiwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxW
ZXJpU2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0
aG9yaXR5IC0gRzUwHhcNMDYxMTA4MDAwMDAwWhcNMzYwNzE2MjM1OTU5WjCByjEL
MAkGA1UEBhMCVVMxFzAVBgNVBAoTDlZlcmlTaWduLCBJbmMuMR8wHQYDVQQLExZW
ZXJpU2lnbiBUcnVzdCBOZXR3b3JrMTowOAYDVQQLEzEoYykgMjAwNiBWZXJpU2ln
biwgSW5jLiAtIEZvciBhdXRob3JpemVkIHVzZSBvbmx5MUUwQwYDVQQDEzxWZXJp
U2lnbiBDbGFzcyAzIFB1YmxpYyBQcmltYXJ5IENlcnRpZmljYXRpb24gQXV0aG9y
aXR5IC0gRzUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQCvJAgIKXo1
nmAMqudLO07cfLw8RRy7K+D+KQL5VwijZIUVJ/XxrcgxiV0i6CqqpkKzj/i5Vbex
t0uz/o9+B1fs70PbZmIVYc9gDaTY3vjgw2IIPVQT60nKWVSFJuUrjxuf6/WhkcIz
SdhDY2pSS9KP6HBRTdGJaXvHcPaz3BJ023tdS1bTlr8Vd6Gw9KIl8q8ckmcY5fQG
BO+QueQA5N06tRn/Arr0PO7gi+s3i+z016zy9vA9r911kTMZHRxAy3QkGSGT2RT+
rCpSx4/VBEnkjWNHiDxpg8v+R70rfk/Fla4OndTRQ8Bnc+MUCH7lP59zuDMKz10/
NIeWiu5T6CUVAgMBAAGjgbIwga8wDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8E
BAMCAQYwbQYIKwYBBQUHAQwEYTBfoV2gWzBZMFcwVRYJaW1hZ2UvZ2lmMCEwHzAH
BgUrDgMCGgQUj+XTGoasjY5rw8+AatRIGCx7GS4wJRYjaHR0cDovL2xvZ28udmVy
aXNpZ24uY29tL3ZzbG9nby5naWYwHQYDVR0OBBYEFH/TZafC3ey78DAJ80M5+gKv
MzEzMA0GCSqGSIb3DQEBBQUAA4IBAQCTJEowX2LP2BqYLz3q3JktvXf2pXkiOOzE
p6B4Eq1iDkVwZMXnl2YtmAl+X6/WzChl8gGqCBpH3vn5fJJaCGkgDdk+bW48DW7Y
5gaRQBi5+MHt39tBquCWIMnNZBU4gcmU7qKEKQsTb47bDN0lAtukixlE0kF6BWlK
WE9gyn6CagsCqiUXObXbf+eEZSqVir2G3l6BFoMtEMze/aiCKm0oHw0LxOXnGiYZ
4fQRbxC1lfznQgUy286dUV4otp6F01vvpX1FQHKOtw5rDgb7MzVIcbidJ4vEZV8N
hnacRHr2lVz2XTIIM6RUthg/aFzyQkqFOFSDX9HoLPKsEdao7WNq
-----END CERTIFICATE-----
)EOF";

#endif

//MQTT config from aws lib
const int maxMQTTpackageSize = 512;
const int maxMQTTMessageHandlers = 1;

//# of connections
long connection = 0;

/*//generate random mqtt clientID
char* generateClientID () {
  char* cID = new char[23]();
  for (int i=0; i<22; i+=1)
    cID[i]=(char)random(1, 256);
  return cID;
}*/

//count messages arrived
int arrivedcount = 0;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
// -- Keep it under 16 characters to handle MQTT compatiblity (thingName+ChipID = unique mqttClientID)
const char thingName[] = "NeoPxLight";

// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "password";

#define STRING_LEN 128
#define NUMBER_LEN 32
// -- Configuration specific key. The value should be modified if config structure was changed.
#define CONFIG_VERSION "apx2"

// -- When CONFIG_PIN is pulled to ground on startup, the Thing will use the initial
//      password to buld an AP. (E.g. in case of lost password)
#define CONFIG_PIN D3

// -- Status indicator pin.
//      First it will light up (kept LOW), on Wifi connection it will blink,
//      when connected to the Wifi it will turn off (kept HIGH).
#define STATUS_PIN LED_BUILTIN

// -- Callback method declarations.
void wifiConnected();
void configSaved();
boolean formValidator();
void handleRoot();
void showLedOffset();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void setClock();
void sendmessage();

DNSServer dnsServer;
WebServer server(80);
#ifdef ESP8266
ESP8266HTTPUpdateServer httpUpdater;
#elif defined(ESP32)
HTTPUpdateServer httpUpdater;
#endif

WiFiClient espClient;
//PubSubClient client(espClient); //MQTT

//ESP8266WiFiMulti WiFiMulti;

AWSWebSocketClient awsWSclient(1000);

PubSubClient client(awsWSclient);

char mqttServerValue[STRING_LEN];
char mqttUserNameValue[STRING_LEN];
char mqttUserPasswordValue[STRING_LEN];
char awsRegionValue[STRING_LEN];
char awsThingValue[STRING_LEN];
char mqttTopicValue[STRING_LEN];
char ledOffsetValue[NUMBER_LEN];
char ledBrightnessValue[NUMBER_LEN];

char aws_topic[STRING_LEN];

char mqttClientId[STRING_LEN]; //automatically created. not via config!

IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword, CONFIG_VERSION);
IotWebConfTextParameter mqttServerParam = IotWebConfTextParameter("AWS endpoint", "mqttServer", mqttServerValue, STRING_LEN);
IotWebConfTextParameter mqttUserNameParam = IotWebConfTextParameter("AWS Key", "mqttUser", mqttUserNameValue, STRING_LEN);
IotWebConfPasswordParameter mqttUserPasswordParam = IotWebConfPasswordParameter("AWS Secret", "mqttPass", mqttUserPasswordValue, STRING_LEN);
IotWebConfTextParameter awsRegionParam = IotWebConfTextParameter("AWS Region", "awsregion", awsRegionValue, STRING_LEN, "eu-west-1");
IotWebConfTextParameter awsThingParam = IotWebConfTextParameter("AWS Thing Name", "awsthingname", awsThingValue, STRING_LEN,NULL,"yourthing");
IotWebConfTextParameter mqttTopicParam = IotWebConfTextParameter("MQTT Topic", "mqttTopic", mqttTopicValue, STRING_LEN,NULL,"some/thing/#");
IotWebConfNumberParameter ledOffsetParam = IotWebConfNumberParameter("Led Offset", "ledOffset", ledOffsetValue, NUMBER_LEN, "0");

//LedBrightness: 255 is the max brightness. It will draw to much current if you turn on all leds on white color (12 leds x 20 milliAmps x 3 colors (to make white) = 720 mA. Wemos can handle 500 mA)
//White means all leds Red/Green/Blue on so 3 x 20 mA per pixel. Just to be sure limited the Max setting to 200 instead of 255. No exact science though.
IotWebConfNumberParameter ledBrightnessParam = IotWebConfNumberParameter("Led Brightness", "ledBrightness", ledBrightnessValue, NUMBER_LEN, "10","5..200", "min='5' max='200' step='5'"); //Limited to 200 (out of 255)


#define PIN 4 //Neo pixel data pin (GPIO4 / D2)
#define NUMBEROFLEDS 12 //the amount of Leds on the strip
#define blinktime 800 //milliseconds between ON/OFF while blinking


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBEROFLEDS, PIN, NEO_GRB + NEO_KHZ400);

// IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
// pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
// and minimize distance between Arduino and first pixel.  Avoid connecting
// on a live circuit...if you must, connect GND first.

int pixel = 0;      //Indicate which Pixel to light
int inConfig = 0;  //Indicator if you are on config portal or not (for blocking Led Pattern)
long lastMsg = 0;   //timestamp of last MQTT Publish

long previous_time = 0;
long current_time = 0;
int blink = 0;           //to keep track of blinking status (on or off)
bool needReset = false;

/*
Assume NUMBEROFLEDS is 12, so using a 12 pixel led ring (or strip)
ledStateArr[] stores the state (color/blinking) of each Led Pixel. Leds start at 1 and count up.
ledStateArr[1] contains the state of Led 1
ledStateArr[12] contains the state of Led 12 
ledStateArr[0] contains 'garbage'. If you would send a MQTT topic like: some/thing/13 which is a not existing led
                                   then that will be captured and stored in ledStateArr[0] (only if content is valid)
                                   The same applies if you would have some/thing/wrong. That would also go to [0] (only if content is valid)
So this is a bit different than usual where Array position 0 is the first position. 
From MQTT I want to drive Led 1 to 12. Not led 0 to 11.
*/
int ledStateArr[NUMBEROFLEDS+1]; //Store state of each led (where Led 1 = ledStateArr[1] and not ledStateArr[0])


//***************************** SETUP ***************************************************
void setup() {

  Serial.begin(115200);
  delay(2000);
  Serial.println();
  Serial.println("Starting up...");

  //**** TO DO IF TO PREVENT CLOCK WHEN NO CONNECTION
  
  setClock(); //needs to be here. Apparently wifi is already running at this point in time. do some IF to skip when no connection!



  iotWebConf.setStatusPin(STATUS_PIN);
  //iotWebConf.setConfigPin(CONFIG_PIN);
  iotWebConf.addSystemParameter(&mqttServerParam);
  iotWebConf.addSystemParameter(&mqttUserNameParam);
  iotWebConf.addSystemParameter(&mqttUserPasswordParam);
  iotWebConf.addSystemParameter(&mqttTopicParam);
  iotWebConf.addSystemParameter(&awsRegionParam);
  iotWebConf.addSystemParameter(&awsThingParam);
  iotWebConf.addSystemParameter(&ledOffsetParam);
  iotWebConf.addSystemParameter(&ledBrightnessParam);
  iotWebConf.setConfigSavedCallback(&configSaved);
  iotWebConf.setFormValidator(&formValidator);
  iotWebConf.getApTimeoutParameter()->visible = false; //set to true if you want to specify the timeout in portal

  iotWebConf.setWifiConnectionCallback(&wifiConnected);
  iotWebConf.setupUpdateServer(
    [](const char* updatePath) { httpUpdater.setup(&server, updatePath); },
    [](const char* userName, char* password) { httpUpdater.updateCredentials(userName, password); });

  // -- Initializing the configuration.
  boolean validConfig = iotWebConf.init();
  if (!validConfig)
  {
    mqttServerValue[0] = '\0';
    mqttUserNameValue[0] = '\0';
    mqttUserPasswordValue[0] = '\0';
    awsRegionValue[0] = '\0';
    awsThingValue[0]= '\0';
    mqttTopicValue[0] ='\0';
    ledOffsetValue[0] = '\0';
    ledBrightnessValue[0] = '\0';
  }
  
  
  //Setup Ledstrip
  strip.begin();
  strip.setBrightness(atoi(ledBrightnessValue));
  strip.show(); // Initialize all pixels to 'off'
  
  strip.setPixelColor(0,strip.Color(255 ,0, 0)); //Set the first led of the LedRing to Red; 
  for(int x=1; x<NUMBEROFLEDS;x++){
      strip.setPixelColor(x,strip.Color(0 ,0, 200)); //Set the remaining led to blue; 
  }
  strip.show(); 

  // -- Set up required URL handlers on the web server.
  server.on("/", handleRoot);
  server.on("/config", []{ iotWebConf.handleConfig(); });
  server.onNotFound([](){ iotWebConf.handleNotFound(); });

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
 // setClock(); // Required for X.509 certificate  validation

//fill AWS parameters
    awsWSclient.setAWSRegion(awsRegionValue);
    awsWSclient.setAWSDomain(mqttServerValue); //aka aws_endpoint
    awsWSclient.setAWSKeyID(mqttUserNameValue); //aka aws_key
    awsWSclient.setAWSSecretKey(mqttUserPasswordValue); //aka aws_secret
    awsWSclient.setUseSSL(true);
    awsWSclient.setCA(ca);
    //as we had to configurate ntp time to validate the certificate, we can use it to validate aws connection as well
    awsWSclient.setUseAmazonTimestamp(false);

  //Set MQTT Server and port 
  client.setServer(mqttServerValue, port);
  client.setCallback(mqttCallback);

  showLedOffset(); //Display real Led 1 and the Led 1 after offset
  delay(5000); // so you have time to check if the green led is at the right spot.

  //add random string to mqttClientId to make it Unique
   //mqttClientId += String(ESP.getChipId(), HEX); //ChipId seems to be part of Mac Address 

//Create UNIQUE MQTT ClientId - When not unique on the same MQTT server, you'll get strange behaviour
//It uses the unique chipID of the ESP.
sprintf(mqttClientId, "%s%u", thingName,ESP.getChipId()); 
Serial.print("mqttclientid: ");
Serial.println(mqttClientId);


//set AWS Publish Topic:

//const char* aws_topic  = "$aws/things/"+awsThingValue+"/shadow/update";
//const char* aws_topic  = "$aws/things/AlexBuildLight/shadow/update";

sprintf(aws_topic, "$aws/things/%s/shadow/update", awsThingValue);
Serial.print("awstopic: ");
Serial.println(aws_topic);
}
//************************ END OF SETUP ********************************************


/*
MQTT Callback function
Determine Topic number and store the payload in ledStateArr (Array)
*/
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");

  int LedId = 0;

  //you should subscribe to topics like topic/# or topic/subtopic/#
  //This will result in topics like: topic/subtopic/0, topic/subtopic/1 where the number corresponds with the LED
  //ledStateArr[LedId] will contain the led status (what color you want) per led.
  
  char *token = strtok(topic, "/"); //split on /
    // Keep printing tokens while one of the 
    // delimiters present in str[]. 
    while (token != NULL) 
    { 
        LedId = atoi(token); 
        token = strtok(NULL, "/"); //break the while. LedId contains the last token
    } 
  if(LedId > NUMBEROFLEDS) //Is it an invalid LedId outside the range of leds?
    LedId = 0;             //Send the value to index 0 which is not used (ledStateArr[0] is not used)

  //Serial.print("Token: ");
  //Serial.println(LedId);

  payload[length] = '\0';
  
  //Print payload to Serial for debugging
  //for (unsigned int i=0;i<length;i++) { 
  //  Serial.print((char)payload[i]);
  //}


  /*
  Define color codes and with or without blink:
  green - greenblink (1 / 2)
  red - redblink (3 / 4)
  yellow - yellowblink (5 / 6)
  purple - purpleblink (7 /8)
  blue - blueblink (9 / 10)
  orange - orangeblink (11 / 12)
  off (0)

  */
  //check for possible topics
  if(strcmp((char*)payload,"green") == 0){ 
      ledStateArr[LedId] = 1;
    }
  else if(strcmp((char*)payload,"greenblink") == 0){ 
      ledStateArr[LedId] = 2;
    }
  else if(strcmp((char*)payload,"red") == 0){
          ledStateArr[LedId] = 3;
    }
  else if(strcmp((char*)payload,"redblink") == 0){
          ledStateArr[LedId] = 4;
    }
  else if(strcmp((char*)payload,"yellow") == 0){
          ledStateArr[LedId] = 5; 
  }
    else if(strcmp((char*)payload,"yellowblink") == 0){
          ledStateArr[LedId] = 6; 
  }
  else if(strcmp((char*)payload,"purple") == 0){
        ledStateArr[LedId] = 7;
    }
  else if(strcmp((char*)payload,"purpleblink") == 0){
        ledStateArr[LedId] = 8;
    }
  else if(strcmp((char*)payload,"blue") == 0){
          ledStateArr[LedId] = 9;
          }
  else if(strcmp((char*)payload,"blueblink") == 0){
          ledStateArr[LedId] = 10;
          }
  else if(strcmp((char*)payload,"orange") == 0){
          ledStateArr[LedId] = 11; 
  }
    else if(strcmp((char*)payload,"orangeblink") == 0){
          ledStateArr[LedId] = 12; 
  }
    else if(strcmp((char*)payload,"off") == 0){
          ledStateArr[LedId] = 0; 
  }
}
//**************** END OF MQTT CALLBACK FUNCTION *********************************


void reconnect() {
  // Loop until we're reconnected
  while ((iotWebConf.getState() == IOTWEBCONF_STATE_ONLINE) && !client.connected()) { 
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect(mqttClientId, mqttUserNameValue, mqttUserPasswordValue)) { //mqtt_user, mqtt_pass
      Serial.println("connected");
      Serial.println(mqttTopicValue);
      client.subscribe(mqttTopicValue); //subscribe to topic
      sendmessage(); //send message to aws 
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

    
  }
}

void setClock() {
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

//send a message to a mqtt topic
void sendmessage() {
    //send a message
    char buf[100];
    strcpy(buf, "{\"state\":{\"reported\":{\"on\": false}, \"desired\":{\"on\": false}}}");
    client.publish("aws_topic", buf);
}


//******************** START OF LOOP () *****************************************************

void loop() {

  iotWebConf.doLoop();
  client.loop(); //make sure MQTT Keeps running (hopefully prevents watchdog from kicking in)
  delay(10);

 /*
  DRIVE THE LEDS
  green - greenblink (1 / 2)
  red - redblink (3 / 4)
  yellow - yellowblink (5 / 6)
  purple - purpleblink (7 /8)
  blue - blueblink (9 / 10)
  orange - orangeblink (11 / 12)
  off (0)
  */
 for (int x=1;x<NUMBEROFLEDS+1;x++){ //loop through all leds and set the required color (R,G,B)
      client.loop(); //make sure MQTT Keeps running (hopefully prevents watchdog from kicking in)

        //Handle led_offset
        pixel = (x-1) + atoi(ledOffsetValue);
        if(pixel > (NUMBEROFLEDS-1)){
            pixel = pixel - NUMBEROFLEDS;
        }
  
  //GREEN
    if(ledStateArr[x] == 1) //GREEN
      strip.setPixelColor(pixel,strip.Color(0,255, 0)); //led on

    else if(ledStateArr[x] == 2){ //GREEN BLINKING
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(0 ,255, 0)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
        }
  //RED
  else if(ledStateArr[x] == 3) //RED
        strip.setPixelColor(pixel,strip.Color(255,0, 0)); //led on
  else if(ledStateArr[x] == 4){ //RED BLINKING
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(255 ,0, 0)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
        }
  //YELLOW     
  else if(ledStateArr[x] == 5) //YELLOW
        strip.setPixelColor(pixel,strip.Color(128,128, 0)); //led on
  else if(ledStateArr[x] == 6){ //YELLOW BLINKING)
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(128,128, 0)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
        }
//PURPLE
    else if(ledStateArr[x] == 7) //PURPLE
        strip.setPixelColor(pixel,strip.Color(128,0, 128)); //led on
     else if(ledStateArr[x] == 8){ //PURPLE BLINKING)
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(128,0, 128)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
        }
  //BLUE
    else if(ledStateArr[x] == 9) //BLUE
        strip.setPixelColor(pixel,strip.Color(0,0, 255)); //led on
    else if(ledStateArr[x] == 10){ //BLUE BLINKING)
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(0,0, 255)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
        }
    //ORANGE
    else if(ledStateArr[x] == 11) //BLUE
        strip.setPixelColor(pixel,strip.Color(255,128, 0)); //led on
    else if(ledStateArr[x] == 12){ //ORANGE (needs blinking)
        if(blink == 1)
          strip.setPixelColor(pixel,strip.Color(255,128, 0)); //led on
        else if(blink == 0)
             strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off
    }
    //OFF
     else if(ledStateArr[x] == 0) //LED OFF
        strip.setPixelColor(pixel,strip.Color(0,0, 0)); //led off

  } //end for-loop

//Block updating the LEDs while in Configuration portal (inConfig)

if(inConfig == 0) 
  strip.show(); //set all pixels  
 
 //Handle blinking of leds by switching blink value every x-milliseconds (blinktime)
 current_time = millis();
 if(current_time > previous_time + blinktime){
    if (blink == 0)
      blink = 1;
    else 
      blink = 0;
    previous_time = millis(); //set current time
 }
   
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  delay(10);

  if (needReset)
  {
    Serial.println("Rebooting after 1 second.");
    iotWebConf.delay(1000);
    ESP.restart();
  }
/* 
// Publish 'ONLINE' Message to Topic. Uncomment if you want to use this.
  long now = millis();
  if (now - lastMsg > 10000) {
    lastMsg = now;
  client.publish("build/test", "ONLINE"); //publish 'ONLINE' message to topic.
  }
  */
}
//******************** END OF LOOP () *****************************************


/**
 * Handle web requests to "/" path.
 */
void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  inConfig = 1; //You are in the Config Portal
  showLedOffset(); //Show real LED1 and your Led 1 at offset

  String s = F("<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>");
  s += iotWebConf.getHtmlFormatProvider()->getStyle();
  s += "<title>MQTT NeoPixel Status Light</title></head><body>";
  s += "<H1>";
  s += iotWebConf.getThingName();
  s+= "</H1>";
  s += "<div>MQTT ClientId: ";
  s += mqttClientId;
  s += "</div>";
  s += "<div>MAC address: ";
  s += WiFi.macAddress();
  s += "</div>";
  s += "<div>MQTT Server: ";
  s += mqttServerValue;
  s += "</div>";
  s += "<div>MQTT Topic: ";
  s += mqttTopicValue;
  s += "</div>";
  s += "<div>LED Offset: ";
  s += ledOffsetValue;
  s += "</div>";
  s += "<div>LED Brightness: ";
  s += ledBrightnessValue;
  s += "</div>";
  s += "<button type='button' onclick=\"location.href='';\" >Refresh</button>";
  s += "<div>Go to <a href='config'>configure page</a> to change values.</div>";
  s +="<div><small>AWS NeoPixel Status Light - Version: ";
  s += VERSIONNUMBER;
  s += " - Get latest version on <a href='https://github.com/arvdsar/MQTT_NeoPixel_Status_Multiple_Improved' target='_blank'>Github</a>.";
  s += "</small></div>";

  s += "</body></html>\n";
  server.send(200, "text/html", s);
}

void wifiConnected()
{
  //needMqttConnect = true; //not using this.
}

void configSaved()
{
  Serial.println("Configuration was updated.");
  showLedOffset(); //Show real LED1 and your Led 1 at offset so you can check the offset
  delay(5000);
  inConfig = 0; // Enable Led Pattern again
  needReset = true; 
}

boolean formValidator()
{
  Serial.println("Validating form.");
  boolean valid = true;

  int l = server.arg(mqttServerParam.getId()).length();
  if (l < 3)
  {
    mqttServerParam.errorMessage = "Please provide at least 3 characters!";
    valid = false;
  }

  return valid;
}


/*
 Handle led_offset
 Set all leds to blue, next make the original first led Red and then set the 
 led with offset to green. The Green Led should be where YOU want to see LED 1.
*/
void showLedOffset(){

  for(pixel =0;pixel < NUMBEROFLEDS;pixel++)
      strip.setPixelColor(pixel,strip.Color(0 ,0, 255)); //Set all leds to Blue
  strip.setPixelColor(0,strip.Color(255 ,0, 0)); //Set the offical first led to Red.

  pixel = 0 + atoi(ledOffsetValue);
  if(pixel > (NUMBEROFLEDS-1)){
    pixel = pixel - NUMBEROFLEDS;
  }  
  strip.setPixelColor(pixel,strip.Color(0 ,255, 0)); //Set the first led with offset to Green. Ready to go.
  strip.show(); 

}





