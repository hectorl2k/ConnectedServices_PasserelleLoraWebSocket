#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <mySD.h>


/*            SD            */
ext::File myFile;
const int chipSelect = 13; // Pin CS pour la carte SD
const char* fichierSD="save.txt";
const int pinMOSI = 15;    // Broche MOSI
const int pinMISO = 2;    // Broche MISO
const int pinSCK = 14;     // Broche SCK
bool isSDConnected = false;
bool isdataInBufferSD =false;



/*          WIFI            */
const char* ssid = "WIFI_DIGITAL";		
const char* password =  "AMP_Digital";


/*        WEB Socket            */
const char* websocket_server = "10.0.0.53";
const uint16_t websocket_port = 8003;
WebSocketsClient webSocket;
bool isWebSocketConnected = false;


/*            LORA             */
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
  isSDConnected=setupSD();


}

void loop()
{
  LoraLoop();                     // Ecoute Lora et envoie en WebSocket si message ou ecris sur carte SD
  webSocket.loop();               // Ecoute Message WebSocket
  SD_CleanBuffer();               // Check si info dans la carte SD, si oui on vide

  sendSerialToLora();             // Ecoute port série et envoie en Lora
   
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
    Serial.println(res);
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


bool setupSD()
{
  return (SD.begin(13, 15, 2, 14));
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

  if(isWebSocketConnected)
    {
      Serial.println("WebSocket connected.");
    }
}

void sendWebSocketMessage(JsonDocument msg)
{
  String jsonString;
  serializeJson(msg, jsonString);
  Serial.println(jsonString);
  if(isWebSocketConnected)            // Si Connecté --> Envoi WebSockset
  {
    webSocket.sendTXT(jsonString);
  }else if(isSDConnected){            // Sinon on verifie si on peux sauvegardé SD
    writeSD(jsonString);
    isdataInBufferSD=true;
  }else{                      
    isSDConnected=setupSD();
    if(isSDConnected)                // Reesaye de se reconnecter a SD
    {
      writeSD(jsonString);
      isdataInBufferSD=true;

    }else{
                              //ATTENTION IL FAUT ECRIRE LE CODE POUR RENVOYER EN LOrA LA PERtE dE DONNee
    }

  }
  
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
    Serial.println(msg);
    
    String jsonString="";
    DeserializationError error = deserializeJson(jsonSend, msg);

    if (!error && startsWithId(jsonSend.as<JsonObject>(), "id_dest", "bt_")) {
      LoRa.beginPacket();
      LoRa.print(msg);
      LoRa.endPacket();
    }
  }



  bool startsWithId(const JsonObject& jsonObject, const char* attributeName, const char* prefix) {
    if (jsonObject.containsKey(attributeName) && jsonObject[attributeName].is<const char*>()) {
        const char* value = jsonObject[attributeName].as<const char*>();
        return strncmp(value, prefix, strlen(prefix)) == 0;
    }
    return false;
}

 

void sendSerialToLora() {
  String message = ""; // Variable pour stocker le message reçu
  
  while (Serial.available()) {
    char c = Serial.read(); // Lire le prochain caractère du port série
    
    if (c != '\n') {
      message += c; // Ajouter le caractère au message
    } else {
      sendBoitier(message);
      message = "";
    }
  }
}


void SD_CleanBuffer()         // A finir
{
  if (isdataInBufferSD && isWebSocketConnected)
  {
    do{

    }while()
  }



}



bool writeSD(const char* filename, const String msg) {
  
  myFile = SD.open(filename, FILE_WRITE); // F_APPEND
  if (myFile) {
    myFile.println(msg);
    myFile.close();
    Serial.println("Message écrit sur la carte SD");
    return true;
  } 
  return false;
}


String readSD(const char* filename, int readRow, bool readAndDelete) {
  // Ouverture du fichier en mode lecture
  myFile = SD.open(filename);
  String result = "";
  String elseCara="";

  // Vérification si le fichier est ouvert avec succès
  if (myFile) {
    // Si readRow vaut 0 ou -1, lire tout le fichier
    if (readRow == 0 || readRow == -1) {
      while (myFile.available()) {
        result += myFile.readStringUntil('\n');
        result += "\n";
      }
      result.remove(result.length() - 1);
      myFile.close();
  }else {
    int i=0;
    while (myFile.available())
    {  
      i++;
      if(i==readRow)
      {
        result += myFile.readStringUntil('\n');  
      }else{
        elseCara += myFile.readStringUntil('\n'); 
        elseCara += "\n";
      }
    }
    if (elseCara.length() > 0 && elseCara[elseCara.length() - 1] == '\n') {
        elseCara.remove(elseCara.length() - 1);
    }
  }
  myFile.close();
  } else {
    return "";
  }

  if(readAndDelete)
  {
    if( elseCara.length() > 0)
    {
      SD.remove(const_cast<char*>(filename));
      ext::File tempFile = SD.open(filename, FILE_WRITE);
      if (tempFile) {
       tempFile.println(elseCara);
       tempFile.close();
      }
    }else{
      SD.remove(const_cast<char*>(filename));
    }
  }
  return result;
}
