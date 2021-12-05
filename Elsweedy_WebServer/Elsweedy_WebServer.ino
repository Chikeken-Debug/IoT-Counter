#include "config.h"

Button button1 = {SensorPin, false};

WiFiServer server(80);

void setup(void)
{
  Setup();
  EEPROM_Setup();
  connectWifi();
}

void loop(void)
{
  interpt();
  button_Click();
  handleSeverClient();
  checkConnectivity();
}

/********************************************** ISR Functions *********************************************/
void IRAM_ATTR isr1()
{
  Serial.println("ISR Called");
  button1.pressed = true;
  prev_millis = millis();
}

/********************************************** Setup Functions *********************************************/
void Setup(void)
{
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(button1.PIN, INPUT_PULLUP);

  attachInterrupt(button1.PIN, isr1, HIGH);
  // Serial.println("Done Setup Func");

  digitalWrite(LED , LOW);
}

void EEPROM_Setup(void)
{
  EEPROM.begin(EEPROM_SIZE);
  ssid = readStringFromFlash(0); // Read SSID stored at address 0
  //Serial.print("SSID = ");
  //Serial.println(ssid);
  password = readStringFromFlash(40); // Read Password stored at address 40
  //Serial.print("passwords = ");
  //Serial.println(password);
  SensorValue = readStringFromFlash(120).toInt();   // Read the initial sensor value at address 150
  //Serial.println("initial sensor value " + String(SensorValue)  );

  // Read IP from address 150
  int ip1 = readintFromFlash(150);
  int ip2 = readintFromFlash(151);
  int ip3 = readintFromFlash(152);
  int ip4 = readintFromFlash(153);
  IPAddress local_IP_temp(ip1, ip2, ip3, ip4);
  local_IP = local_IP_temp;
  // Read gatway IP from address 154
  int gat1 = readintFromFlash(154);
  int gat2 = readintFromFlash(155);
  int gat3 = readintFromFlash(156);
  int gat4 = readintFromFlash(157);
  IPAddress gateway_temp(gat1, gat2, gat3, gat4);
  gateway = gateway_temp;
  // Read subnet IP from address 154
  int sub1 = readintFromFlash(158);
  int sub2 = readintFromFlash(159);
  int sub3 = readintFromFlash(160);
  int sub4 = readintFromFlash(161);
  IPAddress subnet_temp(sub1, sub2, sub3, sub4);
  subnet = gateway_temp;

  //  Serial.println("initial IP " + local_IP_temp.toString()  );
  //  Serial.println("initial gatway " + gateway_temp.toString()  );
  //  Serial.println("initial subnet " + gateway_temp.toString()  );
  //
  //  Serial.println("Done EEPROM Setup Func");
}

void connectWifi()
{
  if (readStringFromFlash(100) == "1")
  {
    Gen_access_point();
  }
  button_Click();
  //Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  Serial.println("");
  int tryCount = 0 ;
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    button_Click();
    tryCount ++ ;
    Serial.print(".");
    delay(500);

    if (tryCount > 30) {
      ESP.restart();
    }
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    digitalWrite(LED , HIGH);
    Serial.println(WiFi.localIP());
    server.begin();
    // Serial.println("HTTP server started");
  }
}

/********************************************** Loop Functions *********************************************/
void short_interpt()
{
  now_millis = millis();
  if ((button1.pressed == true) && (now_millis > prev_millis) && (digitalRead(button1.PIN) == 1))
  {
    button1.pressed = false;
    SensorValue++;
    Serial.println(SensorValue);
  }
}

void interpt()
{
  now_millis = millis();
  if ((button1.pressed == true) && (now_millis > prev_millis) && (digitalRead(button1.PIN) == 1))
  {
    button1.pressed = false;
    //Serial.println("Interrupt Release");
    SensorValue++;
    Serial.printf("Button 1 has been pressed %u times\n", SensorValue);
  }
  //  Serial.print(button1.pressed);
  //  Serial.print("  ");
  //  Serial.print(digitalRead(button1.PIN));
  //  Serial.print("  ");
  //  Serial.println(SensorValue);
  delay(200);
}

void button_Click()
{
  //Serial.println("Inside Button_Click");
  if ( !digitalRead(AccessPointPin) ) {
    //Serial.println("Inside Button_Clicked");
    delay(2000);
    if ( !digitalRead(AccessPointPin) ) {
      digitalWrite(LED , LOW);
      //Serial.println("Inside Button_Click_Access_loop");
      Serial.println("AccessPoint clicked");
      // write sensor value to flash
      //Serial.println("Writing value < " + String(SensorValue) + " > to EEPROM" );
      writeStringToFlash(String(SensorValue).c_str() , 120);
      writeStringToFlash("1", 100);
      ESP.restart();
    }
  }
}

void handleSeverClient()
{
  //Serial.println("check Client.");          // print a message out in the serial port

  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {        // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    int clientCount = 0 ;
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) { // loop while the client's connected
      short_interpt();
      delay(1);
      //Serial.println("Help iam stuck !!!!!");
      if (client.available()) {             // if there's bytes to read from the client,
        //Serial.println("client available");
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            if (header.indexOf("GET /reset") >= 0) {
              //ResetFlag = 1;
              resetCounter();
            }
            client.println(returnHtml(String(SensorValue) , false ));
            //        Serial.println("put the data");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
      clientCount ++ ;
      if (clientCount > 1500) { // timout 1.5 sec for the client
        //  Serial.println("hero action");
        break ;
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    //Serial.println("");
  }
}

void checkConnectivity() {
  // check for wifi for recoonect function
  //Serial.println("WIFI CHECK Time ");
  if (WiFi.status() == WL_CONNECTED) return ;
  digitalWrite(LED , LOW );
  int tryCounts = 0;

  WiFi.disconnect();
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println("try reConnecting ... ");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    button_Click();
    short_interpt();
    Serial.print(".");
    delay(1000);
    if (tryCounts ++ >= 10) break ; // try for 10 seconds only
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("");
    Serial.println("Connected again sucess ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED , HIGH);
  }
}

/********************************************** Helpful Functions *********************************************/
void resetCounter()
{
  //Serial.println("Reset Button Clicked");
  sendData("value=" + String(SensorValue));
  SensorValue = 0;
  //Serial.println("Writing value 0 to EEPROM" );
  writeStringToFlash("0" , 120);
  //Serial.println("new value posted");
  updateViaOta();
}

void Gen_access_point()
{
  //Serial.println("Inside Gen_accesss_point");
  WiFi.softAP("Boards Counter", "88888888");
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  server.begin();

  while (true) {
    client_handle();
    if (exit_but) {
      exit_but = false;
      break;
    }
  }

  writeStringToFlash("0", 100);
  ESP.restart();
}

void sendData(String params)
{
  HTTPClient http;

  String url = "https://script.google.com/macros/s/" + GOOGLE_SCRIPT_ID + "/exec?" + params;
  Serial.println(url);
  Serial.println("Making a request");

#ifdef ESP8266
  //Serial.println("REQUESTting ESP8266");
  WiFiClientSecure client;
  client.setInsecure(); //the magic line, use with caution
  client.connect("https://script.google.com", 443);
  http.begin(client , url );
#endif
#ifdef ESP32
  //Serial.println("REQUESTting ESP32");
  http.begin(url );
#endif

  delay(2000);
  int httpCode = http.GET();
  http.end();
  if (httpCode == HTTP_CODE_OK) {
    Serial.println(":OK ");
  }
  Serial.println(": done " + String(httpCode));
}

void writeStringToFlash(const char* toStore, int startAddr)
{
  int i = 0;
  for (; i < LENGTH(toStore); i++) {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}

void writeintToFlash(int toStore, int startAddr)
{
  EEPROM.write(startAddr, toStore);
  EEPROM.commit();
}

String readStringFromFlash(int startAddr)
{
  char in[128]; // char array of size 128 for reading the stored data
  int i = 0;
  for (; i < 128; i++) {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}

int readintFromFlash(int startAddr)
{
  return EEPROM.read(startAddr);
}

void client_handle()
{
  WiFiClient client = server.available();
  ssid = "";
  password = "";

  if (client) {
    Serial.println("New Client");

    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            client.println(wifiCorrect ? returnHtml(local_IP.toString() , true) : htmlConfigurationString);
            
            if ( header.indexOf("GET /?ssid") >= 0 && !wifiCorrect) {
              String url = header;
              ssid = "";
              password = "";
              String p2 = "";

              url = url.substring(4);
              url = url.substring(url.indexOf('='), url.indexOf(' '));
              //Serial.println("URL  :  " + url);
              ssid = url;
              ssid = ssid.substring(url.indexOf('=') + 1, url.indexOf('&'));
              ssid = urlParse(ssid);
              //Serial.println("ssid  :  " + ssid);

              p2 = url.substring(url.indexOf('&') + 1);
              password = p2;
              password = password.substring(p2.indexOf('=') + 1, p2.indexOf('&'));
              password = urlParse(password);
              //Serial.println("password  :  " + password);
              writeStringToFlash(ssid.c_str(), 0);
              writeStringToFlash(password.c_str(), 40);

              count = 0;
              //Serial.println("before");
              //Serial.println(ssid);
              //Serial.println(password);

              ssid.replace("%20", " ");
              password.replace("%20", " ");
              //Serial.println("after");
              Serial.println(ssid);
              Serial.println(password);
              WiFi.begin(ssid.c_str(), password.c_str());
              delay(500);
              Serial.println("Connecting");
              while (WiFi.status() != WL_CONNECTED) {
                delay(700);
                count++;
                if (count > 20) {
                  Serial.println("Wrong Pass");
                  digitalWrite(LED , HIGH);
                  delay(250);
                  digitalWrite(LED , LOW);
                  delay(250);
                  digitalWrite(LED , HIGH);
                  delay(250);
                  digitalWrite(LED , LOW);
                  break;
                }
              }
              if (count < 20) {
                wifiCorrect = true ;
                digitalWrite(LED , HIGH);
                //Serial.println("Wifi Correct");
                //Serial.println(WiFi.localIP().toString());

                IPAddress ipv4 = get_sweet_ip(WiFi.localIP(), WiFi.subnetMask());
                local_IP = ipv4 ;
                IPAddress subv4 = WiFi.subnetMask();
                IPAddress gatev4 = WiFi.gatewayIP();
                 // Write IP to eeprom
                writeintToFlash( ipv4[0] , 150);
                writeintToFlash( ipv4[1] , 151);
                writeintToFlash( ipv4[2] , 152);
                writeintToFlash( ipv4[3] , 153);
                // Write sunbnet to eeprom
                writeintToFlash( subv4[0] , 154);
                writeintToFlash( subv4[1] , 155);
                writeintToFlash( subv4[2] , 156);
                writeintToFlash( subv4[3] , 157);
                // Write gatway to eeprom
                writeintToFlash( gatev4[0] , 158);
                writeintToFlash( gatev4[1] , 159);
                writeintToFlash( gatev4[2] , 160);
                writeintToFlash( gatev4[3] , 161);

                client.println(returnHtml(local_IP.toString() , true));
              }
            } else if (header.indexOf("GET /reset") >= 0) {
              //Serial.println("ok button Clicked");
              exit_but = true;
            }
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();

  }
}

String returnHtml(String  value , bool ipAdreesPage)
{
  String refreshSting = ipAdreesPage ?  " "  : "<meta http-equiv=\"refresh\" content=\"5;/\" />" ;
  String buttonString = ipAdreesPage ? "ok" : "Reset" ;
  String itemString = ipAdreesPage ? "" : "Item" ;
  String bottomString = ipAdreesPage ? "Server IP" : "Is Checked" ;
  String stringSize = ipAdreesPage ? "50" : "100";

  return "<DOCTYPE html>"\
         "<html lang=\"en\">"\
         "<head>"\
         "<meta charset=\"UTF-8\">"\
         "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">" + refreshSting + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"\
         "<title>Boards Counter</title>"\
         "<style>"\
         "* {"\
         "padding: 0;"\
         "margin: 0;"\
         "font-weight: bold;"\
         "font-family: 'Calibri'"\
         "}"\
         "body {"\
         "background-color: rgb(34, 45, 57);"\
         "}"\
         ".NavBar {"\
         "background-color: #0AD666;"\
         "padding: 20px 10px;"\
         "}"\
         ".NavBar p1 {"\
         "color: rgb(34, 45, 57);"\
         "font-size: 25px;"\
         "}"\
         "#reset {"\
         "height: 50px;"\
         "display: inline;"\
         "position: absolute;"\
         "margin-right: 10px;"\
         "right: 0px;"\
         "top: 12px;"\
         "}"\
         ".NavBar button {"\
         "background-color: rgb(34, 45, 57);"\
         "color: #0AD666;"\
         "border: 0;"\
         "border-radius: 25px;"\
         "padding: 15px;"\
         "font-size: 18px;"\
         "}"\
         ".NavBar button:hover {"\
         "background-color: gray;"\
         "}"\
         ".mainScreen {"\
         "position: absolute;"\
         "top: 50%;"\
         "left: 50%;"\
         "transform: translateX(-50%) translateY(-50%);"\
         "text-align: center;"\
         "border-radius: 25px;"\
         "padding: 10px;"\
         "}"\
         ".mainScreen p1 {"\
         "color: white;"\
         "font-size: 60px;"\
         "}"\
         ".mainScreen p {"\
         "color: white;"\
         "font-size: 30px;"\
         "}"\
         "#value {"\
         "color: #0AD666;"\
         "font-size: " + stringSize + "px;"\
         "}"\
         "#footer {"\
         "width: 100%;"\
         "position: fixed;"\
         "bottom: 0;"\
         "padding: 10px;"\
         "background-color: white;"\
         "color: rgb(34, 45, 57);"\
         "text-align: center;}"\
         ".config_form {"\
         "     display: none;"\
         "}"\
         "</style>"\
         "</head>"\
         "<body>"\
         "<div class=\"NavBar\">"\
         "<p1>Elswedey Electric</p1>"\
         "<div id=\"reset\"><a href=\"/reset\"><button class=\"button\">" + buttonString + "</button></a></div>"\
         "</div>"\
         "<div class=\"mainScreen\">"\
         "<p><span id=\"value\">" + value + "</span>" + itemString + "</p>"\
         "<p1>" + bottomString + "</p1>"\
         "</div>"\
         "<section id=\"footer\">"\
         "Powered By EME IH"\
         "</section>"\
         "</body>"\
         "</html>" ;
}

String urlParse(String url)
{
  String encodedString = "";
  char c , code0, code1 ;

  for (int i = 0; i < url.length(); i++) { // loop throw the String
    c = url.charAt(i); // get the charahter
    if (c == '+') { // if + replace with space
      c = ' ';
    } else if (c == '%') { // if % so start of special charachter
      i++;
      code0 = url.charAt(i); // the first charachter after %
      i++;
      code1 = url.charAt(i); // the second charachter after %
      c = (h2int(code0) << 4) | h2int(code1); // convert the hex to int to get the asci
    }
    encodedString += c;
  }
  return encodedString;
}

unsigned char h2int(char c)
{
  // function to convert hex to int to get the asci code
  if (c >= '0' && c <= '9') {
    return ((unsigned char)c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return ((unsigned char)c - 'a' + 10);
  }
  if (c >= 'A' && c <= 'F') {
    return ((unsigned char)c - 'A' + 10);
  }
  return (0);
}

void updateViaOta()
{
  if (WiFi.status() != WL_CONNECTED) return ; // no wifi connection

  Serial.println("updating..");
  String url = "http://otadrive.com/deviceapi/update?";
  url += "k=d80e6d32-28f0-4093-83e4-dbe21d7ab493";
  url += "&v=" + CodeVersion;
  url += "&s=" + String(chipID) ;

  Serial.println("url : " + url);
  WiFiClient client;

#ifdef ESP8266
  // Serial.println("updating ESP8266");
  t_httpUpdate_return ret = ESPhttpUpdate.update(client , url, CodeVersion);
#endif
#ifdef ESP32
  //Serial.println("updating ESP32");
  t_httpUpdate_return ret = httpUpdate.update(client , url, CodeVersion);
#endif

  switch (ret)
  {
    case HTTP_UPDATE_FAILED:
      Serial.println("Update faild!");
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("No new update available");
      break;
    case HTTP_UPDATE_OK:
      // We can't see this, because of reset chip after update OK
      Serial.println("Update OK");
      break;
    default:
      break;
  }
  Serial.println("Done!..");
}

IPAddress get_sweet_ip(IPAddress given , IPAddress subnet) {
  String given_str = given.toString();
  String subnet_str = subnet.toString();
  subnet_str = subnet[3];

  if (subnet_str == "0") {
    Serial.println("done");
    IPAddress returned_ip(given[0], given[1], given[2], 200);
    return returned_ip;
  }
  else {
    return given;
  }
}
/********************************************** Congratulation *********************************************/
