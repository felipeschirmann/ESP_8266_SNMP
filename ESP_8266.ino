/*
    This sketch establishes a TCP connection to a "quote of the day" service.
    It sends a "hello" message, and then prints received data.
*/

int DEBUG=0;

#include <NTPClient.h>
// change next line to use with another board/shield
#include <ESP8266WiFi.h>
//#include <WiFi.h> // for WiFi shield
//#include <WiFi101.h> // for WiFi 101 shield or MKR1000
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "Oi 05AC"
#define STAPSK  "kNTIu3Y2GS"
#endif

//SNMP Client
#include <WiFiUdp.h>
#include <Arduino_SNMP_Manager.h>


// Adafruit_SH1106G
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
//#define i2c_Address 0x3d //initialize with the I2C addr 0x3D Typically Adafruit OLED's

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


//************************************
//* SNMP Device Info                 *
//************************************
IPAddress router(192, 168, 1, 254);
const char *community = "public";
const int snmpVersion = 1; // SNMP Version 1 = 0, SNMP Version 2 = 1
// OIDs
const char *oidIfSpeedGuage =    ".1.3.6.1.2.1.2.2.1.5.2";       // Guage Regular ethernet interface ifSpeed.2

const char *oidInOctetsCount32 = ".1.3.6.1.2.1.2.2.1.10.2";      // Counter32 ifInOctets.2
//const char *oidIn64Counter =     ".1.3.6.1.2.1.31.1.1.1.6.2";  // /Counter64 64-bit ifInOctets.2

const char *oidOutOctetsCount32 = ".1.3.6.1.2.1.2.2.1.16.2";     // Counter32 ifOutOctets.2
//const char *oidOut64Counter =     ".1.3.6.1.2.1.31.1.1.1.10.2"; // C/ounter64 64-bit ifOutOctets.2

const char *oidServiceCountInt = ".1.3.6.1.2.1.1.7.0";           // Integer sysServices
const char *oidSysName =         ".1.3.6.1.2.1.1.5.0";           // OctetString SysName
const char *oidUptime =          ".1.3.6.1.2.1.1.3.0";           // TimeTicks uptime (hundredths of seconds)
//************************************

//************************************
//* Settings                         *
//************************************
int pollInterval = 5000; // delay in milliseconds
//************************************

//************************************
//* Initialise                       *
//************************************
// Variables
unsigned int ifSpeedResponse = 0;

unsigned int inOctetsResponse = 0;
unsigned int outOctetsResponse = 0;

unsigned int velocitIn = 0;
unsigned int velocitOut = 0;

int servicesResponse = 0;
char sysName[50];
char *sysNameResponse = sysName;
//long long unsigned int hcCounter = 0;
int uptime = 0;
int lastUptime = 0;

unsigned long pollStart = 0;
unsigned long intervalBetweenPolls = 0;

float bandwidthInUtilPct = 0;
float bandwidthOutUtilPct = 0;

unsigned int lastInOctets = 0;
unsigned int lastOutOctets = 0;
// SNMP Objects
WiFiUDP udp;                                           // UDP object used to send and receive packets
SNMPManager snmp = SNMPManager(community);             // Starts an SMMPManager to listen to replies to get-requests
SNMPGet snmpRequest = SNMPGet(community, snmpVersion); // Starts an SMMPGet instance to send requests

// Blank callback pointer for each OID
ValueCallback *callbackIfSpeed;

ValueCallback *callbackInOctets;
ValueCallback *callbackOutOctets;

ValueCallback *callbackServices;
ValueCallback *callbackSysName;
//ValueCallback *callback64Count/er;
ValueCallback *callbackUptime;
//************************************

//************************************
//* Function declarations            *
//************************************
void getSNMP();
void doSNMPCalculations();
void printVariableValues();
//************************************



//NTP Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char* ssid     = STASSID;
const char* password = STAPSK;

const char* host = "djxmmx.net";
const uint16_t port = 17;

void setup() {
  Serial.begin(115200);

  // We start by connecting to a WiFi network

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  /* Explicitly set the ESP8266 to be a WiFi-client, otherwise, it by default,
     would try to act as both a client and an access-point and could cause
     network-issues with your other WiFi-devices on your WiFi-network. */
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  if ( DEBUG == 1 ) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

  timeClient.begin();
  timeClient.setTimeOffset(-3 * 3600);

  snmp.setUDP(&udp); // give snmp a pointer to the UDP object
  snmp.begin();      // start the SNMP Manager

   setupVarCalbacksSNMP();

  delay(250); // wait for the OLED to power up
  display.begin(i2c_Address, true); // Address 0x3C default
 //display.setContrast (0); // dim display
 
  display.display();
  delay(2000);

  // Clear the buffer.
  display.clearDisplay();

  // draw a single pixel
  display.drawPixel(10, 10, SH110X_WHITE);
  // Show the display buffer on the hardware.
  // NOTE: You _must_ call display after making any drawing commands
  // to make them visible on the display hardware!
  display.display();
  delay(2000);
  display.clearDisplay();

}

void loop() {

  delay(1000);

  timeClient.update();
  if ( DEBUG == 1 ) {
    Serial.println(timeClient.getFormattedTime());
  }
  
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SH110X_WHITE); //  white text
  display.setCursor(0, 0);     // Start at top-left corner
  
  display.print("Time: "); 
  display.println(timeClient.getFormattedTime());
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.println();
  display.print("ROUTER: ");
  display.println(router);
  display.print("In:  ");
  display.print(bandwidthInUtilPct);
  display.println("%");
  display.print("Out: ");
  display.print(bandwidthOutUtilPct);
  display.println("%");
  display.print("In:  ");
  display.print(velocitIn);
  display.println(" Mb");
  display.print("Out: ");
  display.print(velocitOut);
  display.println(" Mb");
  
  snmp.loop();
  intervalBetweenPolls = millis() - pollStart;
  if (intervalBetweenPolls >= pollInterval)
  {
    pollStart += pollInterval; // this prevents drift in the delays
    getSNMP();
    doSNMPCalculations(); // Do something with the data collected
    printVariableValues(); // Print the values to the serial console and Display
  }
  display.display();
}
