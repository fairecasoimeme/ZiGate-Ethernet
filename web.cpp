#include <Arduino.h>
#include <ArduinoJson.h>
#include <ETH.h>
#include "WiFi.h"
//#include "web.h"
//#include <WebServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include "LITTLEFS.h"
#include "config.h"
#include "log.h"

#include "esp_adc_cal.h"

extern struct ConfigSettingsStruct ConfigSettings;
extern unsigned long timeLog;


//WebServer serverWeb(80);
AsyncWebServer serverWeb(80);

void webServerHandleClient()
{
   // serverWeb.handleClient();
}



const char HTTP_HEADER[] PROGMEM = 
  "<head>"
  "<script type='text/javascript' src='web/js/jquery-min.js'></script>"
  "<script type='text/javascript' src='web/js/bootstrap.min.js'></script>"
  "<script type='text/javascript' src='web/js/functions.js'></script>"
  "<link href='web/css/bootstrap.min.css' rel='stylesheet' type='text/css' />"
  "<link href='web/css/style.css' rel='stylesheet' type='text/css' />"
  " </head>"
    "<body>"
  "<nav class='navbar navbar-expand-lg navbar-light bg-light rounded'><a class='navbar-brand' href='/'><img src='web/img/logo.png'/> <strong>Config </strong>"
   VERSION
  "</a>"
  "<button class='navbar-toggler' type='button' data-toggle='collapse' data-target='#navbarNavDropdown' aria-controls='navbarNavDropdown' aria-expanded='false' aria-label='Toggle navigation'>"
  "<span class='navbar-toggler-icon'></span>"
  "</button>"
  "<div id='navbarNavDropdown' class='collapse navbar-collapse justify-content-md-center'>"
  "<ul class='navbar-nav'>"
    "<li class='nav-item'>"
      "<a class='nav-link' href='/'>Status</a>"     
    "</li>"
    "<li class='nav-item dropdown'>"
      "<a class='nav-link dropdown-toggle' href='#' id='navbarDropdown' role='button' data-toggle='dropdown' aria-haspopup='true' aria-expanded='false'>Config</a>"
      "<div class='dropdown-menu' aria-labelledby='navbarDropdown'>"   
          "<a class='dropdown-item' href='/general'>General</a>"
          "<a class='dropdown-item' href='/serial'>Serial</a>"      
          "<a class='dropdown-item' href='/ethernet'>Ethernet</a>"
          "<a class='dropdown-item' href='/wifi'>WiFi</a>"
      "</div>"
    "</li>"
     "<li class='nav-item'>"
      "<a class='nav-link' href='/tools'>Tools</a>"
      
    "</li>"
    "<li class='nav-item'>"
      "<a class='nav-link' href='/help'>Help</a>"
    "</li>"
  "</ul></div>"
  "</nav>";


const char HTTP_WIFI[] PROGMEM = 
 "<h1>Config WiFi</h1>"
  "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveWifi'>"
  "<div class='form-check'>"
    
    "<input class='form-check-input' id='wifiEnable' type='checkbox' name='wifiEnable' {{checkedWiFi}}>"
    "<label class='form-check-label' for='wifiEnable'>Enable</label>"
  "</div>"
  "<div class='form-group'>"
    "<label for='ssid'>SSID</label>"
    "<input class='form-control' id='ssid' type='text' name='WIFISSID' value='{{ssid}}'> <a onclick='scanNetwork();' class='btn btn-primary mb-2'>Scan</a><div id='networks'></div>"
  "</div>"
   "<div class='form-group'>"
    "<label for='pass'>Password</label>"
    "<input class='form-control' id='pass' type='password' name='WIFIpassword' value=''>"
  "</div>"
   "<div class='form-group'>"
    "<label for='ip'>@IP</label>"
  "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ip}}'>"
  "</div>"
   "<div class='form-group'>"
    "<label for='mask'>@Mask</label>"
  "<input class='form-control' id='mask' type='text' name='ipMask' value='{{mask}}'>"
  "</div>"
   "<div class='form-group'>"
    "<label for='gateway'>@Gateway</label>"
  "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{gw}}'>"
  "</div>"
   "Server Port : <br>{{port}}<br><br>"
  "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
  "</form>";

const char HTTP_SERIAL[] PROGMEM = 
 "<h1>Config Serial</h1>"
  "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveSerial'>"
    
   "<div class='form-group'>"
    "<label for='baud'>Speed</label>"
  "<select class='form-control' id='baud' name='baud'>"
    "<option value='9600' {{selected9600}}>9600 bauds</option>"
    "<option value='19200' {{selected19200}}>19200 bauds</option>"
    "<option value='38400' {{selected38400}}>38400 bauds</option>"
    "<option value='57600' {{selected57600}}>57600 bauds</option>"
    "<option value='115200' {{selected115200}}>115200 bauds</option>"
  "</select>"
  "</div>"
   "<br><br>"
  "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
  "</form>";

const char HTTP_HELP[] PROGMEM = 
 "<h1>Help !</h1>"
 
    "<h3>Shop & description</h3>"
    "You can go to this url :</br>"
    "<a href=\"https://zigate.fr/boutique\" target='_blank'>Shop </a></br>"
    "<a href=\"https://zigate.fr/documentation/descriptif-de-la-zigate-ethernet/\" target='_blank'>Description</a></br>"
    "<h3>Firmware Source & Issues</h3>"
    "Please go here :</br>"
    "<a href=\"https://github.com/fairecasoimeme/ZiGate-Ethernet\" target='_blank'>Sources</a>";

  


const char HTTP_ETHERNET[] PROGMEM = 
 "<h1>Config Ethernet</h1>"
  "<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveEther'>"
    "<div class='form-check'>"
    
    "<input class='form-check-input' id='dhcp' type='checkbox' name='dhcp' {{modeEther}}>"
    "<label class='form-check-label' for='dhcp'>DHCP</label>"
  "</div>"
   "<div class='form-group'>"
    "<label for='ip'>@IP</label>"
  "<input class='form-control' id='ip' type='text' name='ipAddress' value='{{ipEther}}'>"
  "</div>"
   "<div class='form-group'>"
    "<label for='mask'>@Mask</label>"
  "<input class='form-control' id='mask' type='text' name='ipMask' value='{{maskEther}}'>"
  "</div>"
   "<div class='form-group'>"
    "<label for='gateway'>@Gateway</label>"
  "<input type='text' class='form-control' id='gateway' name='ipGW' value='{{GWEther}}'>"
  "</div>"
   "Server Port : <br>{{port}}<br><br>"
  "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
  "</form>";

const char HTTP_GENERAL[] PROGMEM=
"<h1>General</h1>"
"<div class='row justify-content-md-center' >"
    "<div class='col-sm-6'><form method='POST' action='saveGeneral'>"
    "<div class='form-check'>"
    "<input class='form-check-input' id='disableWeb' type='checkbox' name='disableWeb' {{disableWeb}}>"
    "<label class='form-check-label' for='disableWeb'>Disable web server when ZiGate is connected</label>"
    "<br>"  
    "<input class='form-check-input' id='enableHeartBeat' type='checkbox' name='enableHeartBeat' {{enableHeartBeat}}>"
    "<label class='form-check-label' for='enableHeartBeat'>Enable HeartBeat (send ping to TCP when no trafic)</label>"
    "<br>"  
    "<label for='refreshLogs'>Refresh console log</label>"
    "<input class='form-control' id='refreshLogs' type='text' name='refreshLogs' value='{{refreshLogs}}'>"
    "<br>"
  "</div>"
  "<button type='submit' class='btn btn-primary mb-2'name='save'>Save</button>"
  "</form></div>"
"</div>";

const char HTTP_ROOT[] PROGMEM = 
 "<h1>Status</h1>"
  "<div class='row'>"
      "<div class='col-sm-6'>"
        "<div class='card'>"
          "<div class='card-header'>Ethernet</div>"
          "<div class='card-body'>"
            "<div id='ethConfig'>"
              "<strong>Connected : </strong>{{connectedEther}}"
              "<br><strong>Mode : </strong>{{modeEther}}"
              "<br><strong>@IP : </strong>{{ipEther}}"
              "<br><strong>@Mask : </strong>{{maskEther}}"
              "<br><strong>@GW : </strong>{{GWEther}}"
            "</div>"
          "</div>"
        "</div>"
      "</div>"
  "</div>"
 "<div class='row'>"
      "<div class='col-sm-6'>"
        "<div class='card'>"
            "<div class='card-header'>Wifi</div>"
            "<div class='card-body'>"
              "<div id='wifiConfig'>"
                "<strong>Enable : </strong>{{enableWifi}}"
                "<br><strong>SSID : </strong>{{ssidWifi}}"
                "<br><strong>@IP : </strong>{{ipWifi}}"
                "<br><strong>@Mask : </strong>{{maskWifi}}"
                "<br><strong>@GW : </strong>{{GWWifi}}"
              "</div>"
            "</div>"
        "</div>"
     "</div>"
  "</div>"
  

 
 ;


void handleNotFound(AsyncWebServerRequest * request) {

  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += request->url();
  message += F("\nMethod: ");
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += request->args();
  message += F("\n");

  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }

  request->send(404, F("text/plain"), message);

}

void handleHelp(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_HELP);
  result += F("</html>");
   
  request->send(200,"text/html", result);
  
}

void handleGeneral(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_GENERAL);
  result += F("</html>");

  if (ConfigSettings.disableWeb)
  {
    result.replace("{{disableWeb}}","checked");
  }else{
    result.replace("{{disableWeb}}","");
  }
  if (ConfigSettings.enableHeartBeat)
  {
    result.replace("{{enableHeartBeat}}","checked");
  }else{
    result.replace("{{enableHeartBeat}}","");
  }

  result.replace("{{refreshLogs}}",(String)ConfigSettings.refreshLogs);

  
  request->send(200,"text/html", result);
  
}


void handleWifi(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_WIFI);
  result += F("</html>");
   
  DEBUG_PRINTLN(ConfigSettings.enableWiFi);
  if (ConfigSettings.enableWiFi)
  {
    
    result.replace("{{checkedWiFi}}","Checked");
  }else{
    result.replace("{{checkedWiFi}}","");
  } 
  result.replace("{{ssid}}",String(ConfigSettings.ssid));
  result.replace("{{ip}}",ConfigSettings.ipAddressWiFi);
  result.replace("{{mask}}",ConfigSettings.ipMaskWiFi);
  result.replace("{{gw}}",ConfigSettings.ipGWWiFi);
  result.replace("{{port}}",String(ConfigSettings.tcpListenPort));


  request->send(200,"text/html", result);
  
}

void handleSerial(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_SERIAL);
  result += F("</html>");
  if (ConfigSettings.serialSpeed == 9600)
  {
     result.replace("{{selected9600}}","Selected");
  }else if (ConfigSettings.serialSpeed == 19200){
     result.replace("{{selected19200}}","Selected");
  }else if (ConfigSettings.serialSpeed == 38400){
     result.replace("{{selected38400}}","Selected");
  }else if (ConfigSettings.serialSpeed == 57600){
     result.replace("{{selected57600}}","Selected");
  }else if (ConfigSettings.serialSpeed == 115200){
     result.replace("{{selected115200}}","Selected");
  }else{
     result.replace("{{selected115200}}","Selected");
  }
  
 request->send(200,"text/html", result);
  
}

void handleEther(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_ETHERNET);
  result += F("</html>");
   
  if (ConfigSettings.dhcp)
  {
     result.replace("{{modeEther}}","Checked");
  }else{
     result.replace("{{modeEther}}","");
  }
  result.replace("{{ipEther}}",ConfigSettings.ipAddress);
  result.replace("{{maskEther}}",ConfigSettings.ipMask);
  result.replace("{{GWEther}}",ConfigSettings.ipGW);
  result.replace("{{port}}",String(ConfigSettings.tcpListenPort));
  
  request->send(200,"text/html", result);
  
}

uint32_t readADC_Cal(int ADC_Raw)
{
  esp_adc_cal_characteristics_t adc_chars;
  
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  return(esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}

void handleRoot(AsyncWebServerRequest * request) {
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
  result += FPSTR(HTTP_ROOT);
  result += F("</html>");

  if (ConfigSettings.enableWiFi)
  {
    result.replace("{{enableWifi}}","<img src='/web/img/ok.png'>");
  }else{
    result.replace("{{enableWifi}}","<img src='/web/img/nok.png'>");
  } 
  result.replace("{{ssidWifi}}",String(ConfigSettings.ssid));
  result.replace("{{ipWifi}}",ConfigSettings.ipAddressWiFi);
  result.replace("{{maskWifi}}",ConfigSettings.ipMaskWiFi);
  result.replace("{{GWWifi}}",ConfigSettings.ipGWWiFi);

  if (ConfigSettings.dhcp)
  {
     result.replace("{{modeEther}}","DHCP");
     result.replace("{{ipEther}}",ETH.localIP().toString());
     result.replace("{{maskEther}}",ETH.subnetMask().toString());
     result.replace("{{GWEther}}",ETH.gatewayIP().toString());
  }else{
     result.replace("{{modeEther}}","STATIC");
     result.replace("{{ipEther}}",ConfigSettings.ipAddress);
     result.replace("{{maskEther}}",ConfigSettings.ipMask);
     result.replace("{{GWEther}}",ConfigSettings.ipGW);
  }
  if (ConfigSettings.connectedEther)
  {
    result.replace("{{connectedEther}}","<img src='/web/img/ok.png'>");
  }else{
    result.replace("{{connectedEther}}","<img src='/web/img/nok.png'>");
  }
  float val;
  float Voltage = 0.0;
  val = analogRead(35);
  Voltage = (val*5300)/4095;
  result.replace("{{Voltage}}",String(Voltage/1000));

  val = analogRead(34);
  Voltage = (val*5300)/4095;
  result.replace("{{VoltageModem}}",String(Voltage/1000));
  
  request->send(200,"text/html", result);
  
}



void handleSaveGeneral(AsyncWebServerRequest * request)
{
  String StringConfig;
  String disableWeb;
  String enableHeartBeat;
  String refreshLogs;
  
  if (request->arg("disableWeb")=="on")
  {
    disableWeb="1";
  }else{
    disableWeb="0";
  }

  if (request->arg("enableHeartBeat")=="on")
  {
    enableHeartBeat="1";
  }else{
    enableHeartBeat="0";
  }

  if (request->arg("refreshLogs").toDouble()<1000)
  {
    refreshLogs="1000";
  }else{
    refreshLogs=request->arg("refreshLogs");
  }
    
   const char * path = "/config/configGeneral.json";

   StringConfig = "{\"disableWeb\":"+disableWeb+",\"enableHeartBeat\":"+enableHeartBeat+",\"refreshLogs\":"+refreshLogs+"}";    
   StaticJsonDocument<512> jsonBuffer;
   DynamicJsonDocument doc(1024);
   deserializeJson(doc, StringConfig);
   
   File configFile = LITTLEFS.open(path, FILE_WRITE);
   if (!configFile) {
    DEBUG_PRINTLN(F("failed open"));
   }else{
     serializeJson(doc, configFile);
   }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
  
 
}


void handleSaveWifi(AsyncWebServerRequest * request)
{
  if(!request->hasArg("WIFISSID")) {request->send(500, "text/plain", "BAD ARGS"); return;}

   String StringConfig;
   String enableWiFi;
    if (request->arg("wifiEnable")=="on")
    {
      enableWiFi="1";
    }else{
      enableWiFi="0";
    }
    String ssid = request->arg("WIFISSID");
    String pass = request->arg("WIFIpassword");
    String ipAddress = request->arg("ipAddress");
    String ipMask = request->arg("ipMask");
    String ipGW = request->arg("ipGW");
    String tcpListenPort = request->arg("tcpListenPort");

    const char * path = "/config/config.json";

   StringConfig = "{\"enableWiFi\":"+enableWiFi+",\"ssid\":\""+ssid+"\",\"pass\":\""+pass+"\",\"ip\":\""+ipAddress+"\",\"mask\":\""+ipMask+"\",\"gw\":\""+ipGW+"\",\"tcpListenPort\":\""+tcpListenPort+"\"}";    
   StaticJsonDocument<512> jsonBuffer;
   DynamicJsonDocument doc(1024);
   deserializeJson(doc, StringConfig);
   
   File configFile = LITTLEFS.open(path, FILE_WRITE);
   if (!configFile) {
    DEBUG_PRINTLN(F("failed open"));
   }else{
     serializeJson(doc, configFile);
   }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
  
 
}

void handleSaveSerial(AsyncWebServerRequest * request)
{
  String StringConfig;
  String serialSpeed = request->arg("baud");
  const char * path="/config/configSerial.json";

  StringConfig = "{\"baud\":"+serialSpeed+"}";
  DEBUG_PRINTLN(StringConfig);
  StaticJsonDocument<512> jsonBuffer;
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, StringConfig);
  
  File configFile = LITTLEFS.open(path, FILE_WRITE);
  if (!configFile) {
  DEBUG_PRINTLN(F("failed open"));
  }else{
   serializeJson(doc, configFile);
  }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
}

void handleSaveEther(AsyncWebServerRequest * request)
{
  if(!request->hasArg("ipAddress")) {request->send(500, "text/plain", "BAD ARGS"); return;}

   String StringConfig;
   String dhcp;
    if (request->arg("dhcp")=="on")
    {
      dhcp="1";
    }else{
      dhcp="0";
    }
    String ipAddress = request->arg("ipAddress");
    String ipMask = request->arg("ipMask");
    String ipGW = request->arg("ipGW");

    const char * path = "/config/configEther.json";

    
   StringConfig = "{\"dhcp\":"+dhcp+",\"ip\":\""+ipAddress+"\",\"mask\":\""+ipMask+"\",\"gw\":\""+ipGW+"\"}";    
   DEBUG_PRINTLN(StringConfig);
   StaticJsonDocument<512> jsonBuffer;
   DynamicJsonDocument doc(1024);
   deserializeJson(doc, StringConfig);
   
   File configFile = LITTLEFS.open(path, FILE_WRITE);
   if (!configFile) {
    DEBUG_PRINTLN(F("failed open"));
   }else{
     serializeJson(doc, configFile);
   }
  request->send(200, "text/html", "Save config OK ! <br><form method='GET' action='reboot'><input type='submit' name='reboot' value='Reboot'></form>");
  
 
}

void handleLogs(AsyncWebServerRequest * request) {
  String result;
  
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
   result +=F("<h1>Console</h1>");
   result += F("<div class='row justify-content-md-center'>");
   result += F("<div class='col-sm-6'>");
    result += F("<button type='button' onclick='cmd(\"ClearConsole\");document.getElementById(\"console\").value=\"\";' class='btn btn-primary'>Clear Console</button> ");
    result += F("<button type='button' onclick='cmd(\"GetVersion\");' class='btn btn-primary'>Get Version</button> ");
    result += F("<button type='button' onclick='cmd(\"ErasePDM\");' class='btn btn-primary'>Erase PDM</button> ");
   result += F("</div></div>");
   result += F("<div class='row justify-content-md-center' >");
   result += F("<div class='col-sm-6'>");
   
   result += F("Raw datas : <textarea id='console' rows='16' cols='100'>");
   
   result += F("</textarea></div></div>");
  //result += F("</div>");
  result += F("</body>");
  result +=F("<script language='javascript'>");
  result +=F("logRefresh({{refreshLogs}});");
  result +=F("</script>");
  result +=F("</html>");

  result.replace("{{refreshLogs}}",(String)ConfigSettings.refreshLogs);

  request->send(200, F("text/html"), result);
}

void handleTools(AsyncWebServerRequest * request) {
  String result;
  
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
   result +=F("<h1>Tools</h1>");
   result += F("<div class='btn-group-vertical'>");
   result += F("<a href='/logs' class='btn btn-primary mb-2'>Console</button>");
   result += F("<a href='/fsbrowser' class='btn btn-primary mb-2'>FSbrowser</button>");
   //result += F("<a href='/update' class='btn btn-primary mb-2'>Update</button>");
   result += F("<a href='/reboot' class='btn btn-primary mb-2'>Reboot</button>");
  result += F("</div></body></html>");

  request->send(200, F("text/html"), result);
}

void handleReboot(AsyncWebServerRequest * request){
  String result;
  
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
   result +=F("<h1>Reboot ...</h1>");
  result = result + F("</body></html>");
  AsyncWebServerResponse *response = request->beginResponse(303);
  response->addHeader(F("Location"),F("/"));
  request->send(response); 

  ESP.restart();
}

void handleUpdate(AsyncWebServerRequest * request) {
  String result; 
  result += F("<html>");
  result += FPSTR(HTTP_HEADER);
   result +=F("<h1>Update ...</h1>");
   result += F("<div class='btn-group-vertical'>");
   result += F("<a href='/setchipid' class='btn btn-primary mb-2'>setChipId</button>");
   result += F("<a href='/setmodeprod' class='btn btn-primary mb-2'>setModeProd</button>");
  result += F("</div>");

  result = result + F("</body></html>");

  request->send(200, F("text/html"), result);
}

void handleFSbrowser(AsyncWebServerRequest * request)
{
  String result;
  result += F("<html>");
  result += FPSTR(HTTP_HEADER); 
   result +=F("<h1>FSBrowser</h1>");
   result += F("<nav id='navbar-custom' class='navbar navbar-default navbar-fixed-left'>");
      result += F("      <div class='navbar-header'>");
        result += F("        <!--<a class='navbar-brand' href='#'>Brand</a>-->");
      result += F("      </div>");
   result +=F("<ul class='nav navbar-nav'>");
        
    String str = "";
    File root = LITTLEFS.open("/config");
    File file = root.openNextFile();
    while (file) 
    {
      String tmp =  file.name();
        tmp = tmp.substring(8);    
        result += F("<li><a href='#' onClick=\"readfile('");
        result +=tmp;
        result+=F("');\">");
        result +=  tmp;
        result +=F(  " ( ");
        result +=  file.size();
        result +=  F(" o)</a></li>");
        file = root.openNextFile();
        
    }
    result += F("</ul></nav>");
    result +=F("<div class='container-fluid' >");
    result +=F("  <div class='app-main-content'>");
    result+= F("<form method='POST' action='saveFile'>");
    result += F("<div class='form-group'>");
      result +=F(" <label for='file'>File : <span id='title'></span></label>");
       result +=F("<input type='hidden' name='filename' id='filename' value=''>");
      result +=F(" <textarea class='form-control' id='file' name='file' rows='10'>");     
      result +=F("</textarea>");
    result += F("</div>");
    result += F("<button type='submit' class='btn btn-primary mb-2'>Save</button>");
     result +=F("</Form>");            
    result +=F("</div>");
    result +=F("</div>");
  result +=  F("</body></html>");

  request->send(200, F("text/html"), result);
}

void handleReadfile(AsyncWebServerRequest * request)
{
  String result;
  uint8_t i=0;
  String filename = "/config/"+request->arg(i);
  File file = LITTLEFS.open(filename, "r");
 
  if (!file) {
    return;
  }
 
  while (file.available()) {
    result += (char) file.read();
  }
  file.close();
  request->send(200, F("text/html"), result);
}

void handleSavefile(AsyncWebServerRequest * request)
{
  if (request->method() != HTTP_POST) {
    request->send(405, F("text/plain"), F("Method Not Allowed"));
    
  } else 
  {
      uint8_t i=0;
      String filename = "/config/"+request->arg(i);
      String content = request->arg(1);
      File file = LITTLEFS.open(filename, "w");
      if (!file) 
      {
        DEBUG_PRINT(F("Failed to open file for reading\r\n"));
        return;
      }
      
      int bytesWritten = file.print(content);
 
      if (bytesWritten > 0) 
      {
        DEBUG_PRINTLN(F("File was written"));
        DEBUG_PRINTLN(bytesWritten);
     
      } else {
        DEBUG_PRINTLN(F("File write failed"));
      }
     
      file.close();
      AsyncWebServerResponse *response = request->beginResponse(303);
      response->addHeader(F("Location"),F("/fsbrowser"));
      request->send(response); 
    }
}

void handleLogBuffer(AsyncWebServerRequest * request)
{
  String result;
  result = logPrint();
  request->send(200, F("text/html"), result);

}

void handleScanNetwork(AsyncWebServerRequest * request)
{
   String result="";
   int n = WiFi.scanNetworks();
   if (n == 0) {
      result = " <label for='ssid'>SSID</label>";
      result += "<input class='form-control' id='ssid' type='text' name='WIFISSID' value='{{ssid}}'> <a onclick='scanNetwork();' class='btn btn-primary mb-2'>Scan</a><div id='networks'></div>";
    } else {
      
       result = "<select name='WIFISSID' onChange='updateSSID(this.value);'>";
       result += "<OPTION value=''>--Choose SSID--</OPTION>";
       for (int i = 0; i < n; ++i) {
            result += "<OPTION value='";
            result +=WiFi.SSID(i);
            result +="'>";
            result +=WiFi.SSID(i)+" ("+WiFi.RSSI(i)+")";
            result+="</OPTION>";
        }
        result += "</select>";
    }  
    request->send(200, F("text/html"), result);
}
void handleClearConsole(AsyncWebServerRequest * request)
{
  logClear();
  
  request->send(200, F("text/html"), "");
}

void handleGetVersion(AsyncWebServerRequest * request)
{
  //\01\02\10\10\02\10\02\10\10\03
  char output_sprintf[2];
  uint8_t cmd[10];
  cmd[0]=0x01;
  cmd[1]=0x02;
  cmd[2]=0x10;
  cmd[3]=0x10;
  cmd[4]=0x02;
  cmd[5]=0x10;
  cmd[6]=0x02;
  cmd[7]=0x10;
  cmd[8]=0x10;
  cmd[9]=0x03;

  Serial2.write(cmd,10);
  Serial2.flush();
  
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

  for (int i=0;i<10;i++)
  {
    sprintf(output_sprintf,"%02x",cmd[i]);
    logPush(' ');
    logPush(output_sprintf[0]);
    logPush(output_sprintf[1]);
  }
  logPush('\n');
  request->send(200, F("text/html"), "");
}

void initWebServer()
{
  serverWeb.serveStatic("/web/js/jquery-min.js", LITTLEFS, "/web/js/jquery-min.js");
  serverWeb.serveStatic("/web/js/functions.js", LITTLEFS, "/web/js/functions.js");
  serverWeb.serveStatic("/web/js/bootstrap.min.js", LITTLEFS, "/web/js/bootstrap.min.js");
  serverWeb.serveStatic("/web/js/bootstrap.min.js.map", LITTLEFS, "/web/js/bootstrap.min.js.map");
  serverWeb.serveStatic("/web/css/bootstrap.min.css", LITTLEFS, "/web/css/bootstrap.min.css");
  serverWeb.serveStatic("/web/css/style.css", LITTLEFS, "/web/css/style.css");
  serverWeb.serveStatic("/web/img/logo.png", LITTLEFS, "/web/img/logo.png");
  serverWeb.serveStatic("/web/img/wait.gif", LITTLEFS, "/web/img/wait.gif");
  serverWeb.serveStatic("/web/img/nok.png", LITTLEFS, "/web/img/nok.png");
  serverWeb.serveStatic("/web/img/ok.png", LITTLEFS, "/web/img/ok.png");
  serverWeb.serveStatic("/web/img/", LITTLEFS, "/web/img/");

  serverWeb.on("/",HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleRoot(request);
  });
  serverWeb.on("/general", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleGeneral(request);
  });
  serverWeb.on("/wifi", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleWifi(request);
  });
  serverWeb.on("/ethernet",HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleEther(request);
  });
  serverWeb.on("/serial", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleSerial(request);
  });
  serverWeb.on("/saveGeneral", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    handleSaveGeneral(request);
  });
  serverWeb.on("/saveSerial", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    handleSaveSerial(request);
  });
  serverWeb.on("/saveWifi", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    handleSaveWifi(request);
  });
  serverWeb.on("/saveEther", HTTP_POST, [](AsyncWebServerRequest * request)
  {
    handleSaveEther(request);
  });
  serverWeb.on("/tools",  HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleTools(request);
  });
  serverWeb.on("/fsbrowser", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleFSbrowser(request);
  });
  serverWeb.on("/logs", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleLogs(request);
  });
  serverWeb.on("/reboot",HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleReboot(request);
  });
  serverWeb.on("/update", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleUpdate(request);
  });
  serverWeb.on("/readFile",  HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleReadfile(request);
  });
  serverWeb.on("/saveFile",  HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleSavefile(request);
  });
  serverWeb.on("/getLogBuffer", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleLogBuffer(request);
  });
  serverWeb.on("/scanNetwork",  HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleScanNetwork(request);
  });
  serverWeb.on("/cmdClearConsole", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleClearConsole(request);
  });
  serverWeb.on("/cmdGetVersion",HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleGetVersion(request);
  });
  serverWeb.on("/help", HTTP_GET, [](AsyncWebServerRequest * request)
  {
    handleHelp(request);
  });
  serverWeb.onNotFound(handleNotFound);
  serverWeb.begin();
}
