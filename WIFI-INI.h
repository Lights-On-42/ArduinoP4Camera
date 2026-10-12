#include <WiFi.h>
#include <WebServer.h>


WebServer server(80);
WiFiClient htmlclient;
bool streamStart=false;


void loginInspecifictWLan(char* WLan_Name, char* WLan_Password) 
{
  if (WiFi.status() == WL_CONNECTED) 
  {
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.hostname("c1");
  Serial.println(WLan_Name);
  WiFi.begin(WLan_Name, WLan_Password);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    counter++;
    if (counter == 30) 
    {
      Serial.println("WiFi not connected!!!");
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    //wir geben die IP adresse die uns das Wlan gegeben hat aus
    Serial.println(WiFi.localIP());

    Serial.print("Hostname: ");
    Serial.println(WiFi.getHostname());
  }

  Serial.println(WiFi.RSSI());
}

void handleRoot() 
{ 
  String combinedString =  String(htmlPage);

  server.send(200, "text/html", combinedString);
}

void handleStream() 
{
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  server.sendContent(response);

  Serial.println("Stream start");
  htmlclient = server.client();  
  htmlclient.print("--frame\r\n");
  htmlclient.print("Content-Type: image/jpg\r\n\r\n");
  
  Serial.println("Stream startet");

  streamStart=true;
}

void INIT_WIFI()
{


  server.on("/", handleRoot);
  //server.on("/a", handleRoot);
  server.on("/stream", handleStream);
  //server.on("/buttonstopstream", handleButtonStopStream);
  //server.onNotFound(handleNotFound);

  server.begin();
}

void WIFI_Tick()
{
  server.handleClient();
}