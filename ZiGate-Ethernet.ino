#include <WiFi.h>
#ifdef BONJOUR_SUPPORT
#include <ESPmDNS.h>
#endif

#include <WiFiClient.h>
#include <WebServer.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "config.h"
#include "web.h"
#include "log.h"


#include <driver/uart.h>
#include <lwip/ip_addr.h>

#include <ETH.h>
#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_POWER_PIN   -1

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR       1

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18

// application config
unsigned long timeLog;
ConfigSettingsStruct ConfigSettings;
ZiGateInfosStruct ZiGateInfos;
bool configOK=false;
String modeWiFi="STA";

#define BAUD_RATE 115200
#define TCP_LISTEN_PORT 9999

// if the bonjour support is turned on, then use the following as the name
#define DEVICE_NAME "ser2net"

// serial end ethernet buffer size
#define BUFFER_SIZE 128



#define VERSION 1.0

#define WL_MAC_ADDR_LENGTH 6

#ifdef BONJOUR_SUPPORT
// multicast DNS responder
MDNSResponder mdns;
#endif

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      DEBUG_PRINTLN(F("ETH Started"));
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      DEBUG_PRINTLN(F("ETH Connected"));
      ConfigSettings.connectedEther=true;
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      DEBUG_PRINTLN(F("ETH MAC: "));
      DEBUG_PRINT(ETH.macAddress());
      DEBUG_PRINT(F(", IPv4: "));
      DEBUG_PRINT(ETH.localIP());
      if (ETH.fullDuplex()) {
        DEBUG_PRINT(F(", FULL_DUPLEX"));
      }
      DEBUG_PRINT(F(", "));
      DEBUG_PRINT(ETH.linkSpeed());
      DEBUG_PRINTLN(F("Mbps"));
      ConfigSettings.connectedEther=true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      DEBUG_PRINTLN(F("ETH Disconnected"));
      ConfigSettings.connectedEther=false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      DEBUG_PRINTLN(F("ETH Stopped"));
      ConfigSettings.connectedEther=false;
      break;
    default:
      break;
  }
}

WiFiServer server(TCP_LISTEN_PORT);

IPAddress parse_ip_address(const char *str) {
    IPAddress result;    
    int index = 0;

    result[0] = 0;
    while (*str) {
        if (isdigit((unsigned char)*str)) {
            result[index] *= 10;
            result[index] += *str - '0';
        } else {
            index++;
            if(index<4) {
              result[index] = 0;
            }
        }
        str++;
    }
    
    return result;
}

bool loadConfigWifi() {
  const char * path = "/config.json";
  
  File configFile = SPIFFS.open(path, FILE_READ);
  if (!configFile) {
    DEBUG_PRINTLN(F("failed open"));
    return false;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc,configFile);

  // affectation des valeurs , si existe pas on place une valeur par defaut
  ConfigSettings.enableWiFi = (int)doc["enableWiFi"];
  strlcpy(ConfigSettings.ssid, doc["ssid"] | "", sizeof(ConfigSettings.ssid));
  strlcpy(ConfigSettings.password, doc["pass"] | "", sizeof(ConfigSettings.password));
  strlcpy(ConfigSettings.ipAddressWiFi, doc["ip"] | "", sizeof(ConfigSettings.ipAddressWiFi));
  strlcpy(ConfigSettings.ipMaskWiFi, doc["mask"] | "", sizeof(ConfigSettings.ipMaskWiFi));
  strlcpy(ConfigSettings.ipGWWiFi, doc["gw"] | "", sizeof(ConfigSettings.ipGWWiFi));
  ConfigSettings.tcpListenPort = TCP_LISTEN_PORT;

  configFile.close();
  return true;
}

bool loadConfigEther() {
  const char * path = "/configEther.json";
  
  File configFile = SPIFFS.open(path, FILE_READ);
  if (!configFile) {
    DEBUG_PRINTLN(F("failed open"));
    return false;
  }

  DynamicJsonDocument doc(1024);
  deserializeJson(doc,configFile);

  // affectation des valeurs , si existe pas on place une valeur par defaut
  ConfigSettings.dhcp = (int)doc["dhcp"];
  strlcpy(ConfigSettings.ipAddress, doc["ip"] | "", sizeof(ConfigSettings.ipAddress));
  strlcpy(ConfigSettings.ipMask, doc["mask"] | "", sizeof(ConfigSettings.ipMask));
  strlcpy(ConfigSettings.ipGW, doc["gw"] | "", sizeof(ConfigSettings.ipGW));
  configFile.close();
  return true;
}


void setupWifiAP()
{
  WiFi.mode(WIFI_AP);
  WiFi.disconnect();
  
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.softAPmacAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String AP_NameString = "ZIGATE-" + macID;

  char AP_NameChar[AP_NameString.length() + 1];
  memset(AP_NameChar, 0, AP_NameString.length() + 1);

  for (int i=0; i<AP_NameString.length(); i++)
    AP_NameChar[i] = AP_NameString.charAt(i);

  String WIFIPASSSTR = "admin"+macID;
  char WIFIPASS[WIFIPASSSTR.length()+1];
  memset(WIFIPASS,0,WIFIPASSSTR.length()+1);
  for (int i=0; i<WIFIPASSSTR.length(); i++)
    WIFIPASS[i] = WIFIPASSSTR.charAt(i);

  WiFi.softAP(AP_NameChar,WIFIPASS );
  WiFi.setSleep(false);
}

bool setupSTAWifi() {
  
  WiFi.mode(WIFI_STA);
  DEBUG_PRINTLN(F("WiFi.mode(WIFI_STA)"));
  WiFi.disconnect();
  DEBUG_PRINTLN(F("disconnect"));
  delay(100);

  WiFi.begin(ConfigSettings.ssid, ConfigSettings.password);
  WiFi.setSleep(false);
  DEBUG_PRINTLN(F("WiFi.begin"));

  IPAddress ip_address = parse_ip_address(ConfigSettings.ipAddressWiFi);
  IPAddress gateway_address = parse_ip_address(ConfigSettings.ipGWWiFi);
  IPAddress netmask = parse_ip_address(ConfigSettings.ipMaskWiFi);
  
  WiFi.config(ip_address, gateway_address, netmask);
  DEBUG_PRINTLN(F("WiFi.config"));

  int countDelay=10;
  while (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINT(F("."));
    countDelay--;
    if (countDelay==0)
    {
      return false;
    }
    delay(250);
  }
  return true;
}


void setup(void)
{  
 
  Serial.begin(115200);
  DEBUG_PRINTLN(F("Start"));
  if (digitalRead(FLASH_ZIGATE)!=0)
  {
    Serial2.begin(115200, SERIAL_8N1, RXD2, TXD2);
    DEBUG_PRINTLN(F("Mode Prod"));
  }else{
     DEBUG_PRINTLN(F("Mode Flash"));
  }
  WiFi.onEvent(WiFiEvent);
 
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  
  
  if (!SPIFFS.begin()) {
    DEBUG_PRINTLN(F("Erreur SPIFFS"));
    return;
  }

  DEBUG_PRINTLN(F("SPIFFS OK"));
  if ((!loadConfigWifi()) || (!loadConfigEther())) {
      DEBUG_PRINTLN(F("Erreur Loadconfig SPIFFS"));   
  } else {
    configOK=true;
    DEBUG_PRINTLN(F("Conf ok SPIFFS"));
  }

  if (ConfigSettings.enableWiFi)
  {
    if (configOK)
    {
      DEBUG_PRINTLN(F("configOK"));  
      if (!setupSTAWifi())
      {
         DEBUG_PRINTLN(F("AP"));
        setupWifiAP();
        modeWiFi="AP";
      }
       DEBUG_PRINTLN(F("setupSTAWifi"));   
    }else{
      
      setupWifiAP();
      modeWiFi="AP";
      DEBUG_PRINTLN(F("AP"));
    }
  }
  
  if (!ConfigSettings.dhcp)
  {
   // ETH.config(parse_ip_address(ConfigSettings.ipAddress),parse_ip_address(ConfigSettings.ipGW),parse_ip_address(ConfigSettings.ipMask),parse_ip_address(ConfigSettings.ipGW),parse_ip_address(ConfigSettings.ipGW));
    ETH.config(parse_ip_address(ConfigSettings.ipAddress), parse_ip_address(ConfigSettings.ipGW),parse_ip_address(ConfigSettings.ipMask));
  }
  //Serial.println("after 2 sprintf");
  //Config PiZiGate
  pinMode(RESET_ZIGATE, OUTPUT);
  pinMode(FLASH_ZIGATE, OUTPUT);
  digitalWrite(RESET_ZIGATE, 1);
  digitalWrite(FLASH_ZIGATE, 1);
  
  initWebServer();
  server.begin();

//GetVersion
  uint8_t cmdVersion[10]={0x01,0x02,0x10,0x10,0x02,0x10,0x02,0x10,0x10,0x03};
  Serial2.write(cmdVersion,10);
}

String hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);
    nextInt = constrain(nextInt, 0, 15);
    
    decValue = (decValue * 16) + nextInt;
  }
  
  return String(decValue);
}

WiFiClient client;

void loop(void)
{
  size_t bytes_read;
  uint8_t net_buf[BUFFER_SIZE];
  uint8_t serial_buf[BUFFER_SIZE];

  webServerHandleClient();
  
    /*if (modeWiFi=="STA")
    {
      if(WiFi.status() != WL_CONNECTED) {
        // we've lost the connection, so we need to reconnect
        if(client) {
          client.stop();
        }
        setupSTAWifi();
      }
   } */
    
    // Check if a client has connected
    if (!client) {
      // eat any bytes in the swSer buffer as there is nothing to see them
      while(Serial2.available()) {
        Serial2.read();
      }
        
      client = server.available();
      /*if(!client) {      
        return;
      }*/
  
    }
  #define min(a,b) ((a)<(b)?(a):(b))
    char output_sprintf[2];
    if(client.connected()) 
    {
      
      int count = client.available();
  
      if(count > 0) 
      {      
        bytes_read = client.read(net_buf, min(count, BUFFER_SIZE));
        Serial2.write(net_buf, bytes_read);         
        
        if (bytes_read>0)
        {
          String tmpTime; 
          String buff="";
          timeLog = millis();
          tmpTime = String(timeLog,DEC);
          logPush('[');
          for (int j =0;j<tmpTime.length();j++)
          {
            logPush(tmpTime[j]);
          }
          logPush(']');
          logPush('-');
          logPush('>');
  
          for (int i=0;i<bytes_read;i++)
          {
            sprintf(output_sprintf,"%02x",net_buf[i]);
            logPush(' ');
            logPush(output_sprintf[0]);
            logPush(output_sprintf[1]);
          }
          logPush('\n');
          //Serial.write(net_buf, bytes_read);
        }
        Serial2.flush();
      }
      
    } else {
       client.stop();
    }
      // now check the swSer for any bytes to send to the network
      bytes_read = 0;
      bool buffOK=false;
      
      while(Serial2.available() && bytes_read < BUFFER_SIZE) {
        buffOK=true;
        serial_buf[bytes_read] = Serial2.read();
        bytes_read++;
        
      }
      if (buffOK)
      {
        uint8_t tmp[128];
        for (int i=0;i<bytes_read;i++)
        {
          sprintf(output_sprintf,"%02x",serial_buf[i]);
          if(serial_buf[i]==0x01)
          {
            
            String tmpTime; 
            String buff="";
            timeLog = millis();
            tmpTime = String(timeLog,DEC);
            logPush('[');
            for (int j =0;j<tmpTime.length();j++)
            {
              logPush(tmpTime[j]);
            }
            logPush(']');
            logPush('<');
            logPush('-');
          }
          logPush(' ');
          
          logPush(output_sprintf[0]);
          logPush(output_sprintf[1]);
          if (serial_buf[i]==0x03)
          {
            logPush('\n');
          }
        }
        
        buffOK=false;
      }
      
      if(bytes_read > 0) {  
        client.write((const uint8_t*)serial_buf, bytes_read);
        client.flush();
      }
  
  
}