#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>


const char* ssid = "WIFI_DIGITAL";		
const char* password =  "AMP_Digital";


const char* websocket_server = "10.0.0.53";
const uint16_t websocket_port = 8003;
WebSocketsClient webSocket;
bool isWebSocketConnected = false;

String identifiant_passerelle = "PasserelleLora_1";


 


#define LORA_BAND    868
#define SCK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     23 //14   // GPIO14 -- SX1278's RESET
#define DI0     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)




void setup()
{
  Serial.begin(115200);
  while (!Serial);

  setupWifi();
  setupLora();
  setupWebSocket();


}

void loop()
{
  LoraLoop();
  webSocket.loop();



    // try to parse packet
  
}


void LoraLoop()
{
  if (LoRa.parsePacket()) {
    JsonDocument receiveLoraJson;

      Serial.print("Received packet :");
      String res ="";
      while (LoRa.available()) {
          res +=(char)LoRa.read();
      }
    
    DeserializationError error = deserializeJson(receiveLoraJson, res);
    if (!error) {
      if(receiveLoraJson["id_dest"] =="serveur")
        {
          receiveLoraJson["id_passerelle"]=identifiant_passerelle;
          sendWebSocketMessage(receiveLoraJson);        
        }
    }else{Serial.println("Erreur Lors de la deserialisation");} 
  }
}



void setupWifi()
{

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {										// verification de la connection WIFI
  delay(500);
  Serial.print(".");
  }
  Serial.print("\nConnected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}


void setupLora()
{
  Serial.println("LoRa Receiver");
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI0);
  if (!LoRa.begin(LORA_BAND * 1E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}




void setupWebSocket()
{
  webSocket.setAuthorization("sanofi", "sanofi");
  webSocket.begin(websocket_server, websocket_port, "/ws/chat/bouton_connecte/");
  webSocket.onEvent(onWebSocketEvent);
  webSocket.setReconnectInterval(5000);

  while (!isWebSocketConnected) {
    webSocket.loop();
  }
  Serial.println("WebSocket connected.");
}

void sendWebSocketMessage(JsonDocument msg)
{
  String jsonString;
  serializeJson(msg, jsonString);
   Serial.println(jsonString);
  webSocket.sendTXT(jsonString);
}

void onWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("Disconnected from WebSocket.");
      delay(5000);
      isWebSocketConnected = false; // Mettre à false en cas de déconnexion
      break;
    case WStype_CONNECTED:
      Serial.println("Connected to WebSocket.");
      isWebSocketConnected = true; // Mettre à true lors de la connexion
      break;
    case WStype_TEXT:
     
      Serial.printf("Received message: %s\n", payload);
      sendBoitier(String(reinterpret_cast<char*>(payload)));
      break;
    case WStype_ERROR:
      Serial.println("WebSocket Error.");
      isWebSocketConnected = false; // Mettre à false en cas d'erreur
      break;
  }}



  void sendBoitier(String msg)
  {
    JsonDocument jsonSend;
    
    String jsonString="";
    DeserializationError error = deserializeJson(jsonSend, msg);

    if (!error && startsWithId(jsonSend.as<JsonObject>(), "id_dest", "bt_")) {

      LoRa.beginPacket();
      LoRa.print(msg);
      LoRa.endPacket();
      Serial.println(msg);
    }
  }



  bool startsWithId(const JsonObject& jsonObject, const char* attributeName, const char* prefix) {
    if (jsonObject.containsKey(attributeName) && jsonObject[attributeName].is<const char*>()) {
        const char* value = jsonObject[attributeName].as<const char*>();
        return strncmp(value, prefix, strlen(prefix)) == 0;
    }
    return false;
}
