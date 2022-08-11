#include "main.h"

// TARJETA NFC =  2:1,2,3;3:4 abriendo la puerta 1, 2 y 3 de la zona 2 y la puerta 4 de la zona 3

void setup()
{
  Serial.begin(115200);

  SPIFFS.begin(true);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
  {
#ifdef MY_DEBUG
    Serial.println("SPIFFS Mount Failed");
#endif
    esp_restart();
  }
  listDir(SPIFFS, "/", 0);

  InicializarVariables();
  // String wifi_string = readFile(SPIFFS, "/wifi.txt");

  // BORRAR ESTO CUANDO TERMINE TODAS LAS PRUEBAS
  if (!SPIFFS.exists("/wifi.txt"))
  {
    String comando = "1";
    writeFile(SPIFFS, "/wifi.txt", (char *)comando.c_str());
  }

 

  initServer();

  xTaskCreate(
      TaskRedWifi, "TaskRedWifi",
      4096,
      NULL, 2,
      NULL);

  xTaskCreate(
      TaskLeerIdNFC, "TaskLeerIdNFC",
      1024,
      NULL, 3,
      NULL);
}

void loop() {}

//TAREAS
void TaskLeerIdNFC(void *pvParameters)
{

  nfc.begin(); // Comienza la comunicación del PN532

  uint32_t versiondata = nfc.getFirmwareVersion(); // Obtiene información de la placa

  Serial.print("Chip encontrado PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX); // Imprime en el serial que version de Chip es el lector

  // Establezca el número máximo de reintentos para leer de una tarjeta.
  // Esto evita que tengamos que esperar eternamente por una tarjeta,
  // que es el comportamiento predeterminado del PN532.
  nfc.setPassiveActivationRetries(0xFF);

  nfc.SAMConfig(); // Configura la placa para realizar la lectura

  Serial.println("Esperando una tarjeta ISO14443A ...");

  boolean LeeTarjeta;                    // Variable para almacenar la detección de una tarjeta
  uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Búfer para almacenar el UID devuelto
  uint8_t LongitudUID;                   // Variable para almacenar la longitud del UID de la tarjeta

  for (;;)
  {
    // Recepción y detección de los datos de la tarjeta y almacenamiento en la variable "LeeTarjeta"
    LeeTarjeta = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &LongitudUID);

    // Se detecto un tarjeta RFID
    if (LeeTarjeta)
    {
      Serial.println("Tarjeta encontrada!");
      Serial.print("Longitud del UID: ");
      Serial.print(LongitudUID, DEC); // Imprime la longitud de los datos de la tarjeta en decimal
      Serial.println(" bytes");
      Serial.print("Valor del UID: ");
      // Imprime los datos almacenados en la tarjeta en Hexadecimal
      for (uint8_t i = 0; i < LongitudUID; i++)
      {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX);
      }
      Serial.println("");
      vTaskDelay(1000); // Espera de 1 segundo
    }
  }
}

void TaskRedWifi(void *pvParameters)
{
  for (;;)
    //server.handleClient();
  vTaskDelay(200);
}

//FUNCIONES DE AYUDA
void initServer()
{

  Serial.println("Configuring access point...");
  
  WiFi.softAP(SSID.c_str(), PASSWORD.c_str());
  IPAddress myIP = WiFi.softAPIP();
  

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("WebServer.html");
  //server.on("/", handleConnectionRoot);
  server.on("/changeSSID", HTTP_POST, procSSID);
  server.onNotFound([](AsyncWebServerRequest *request) {
      request->send(400, "text/plain", "Not found");
   });
  server.begin();

  Serial.print("SSID: ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(myIP);
  
}
/*
void handleConnectionRoot()
{
  server.send(200, "text/html", answer);
}*/

void InicializarVariables()
{

  if (!SPIFFS.exists("/SSID.txt"))
  {
    String comando = "ConfigNFC";
    writeFile(SPIFFS, "/SSID.txt", (char *)comando.c_str());
  }
  if (!SPIFFS.exists("/PASS.txt"))
  {
    String comando = "hola1234";
    writeFile(SPIFFS, "/PASS.txt", (char *)comando.c_str());
  }

  SSID = readFile(SPIFFS, "/SSID.txt");
  PASSWORD = readFile(SPIFFS, "/PASS.txt");
}

void procSSID(AsyncWebServerRequest *request)
{
  String SSID1 = request->arg("ssid"); //server.arg("ssid");
  bool t = false;

  if (!SSID1.isEmpty())
  {
    writeFile(SPIFFS, "/SSID.txt", (char *)SSID1.c_str());
    Serial.println("SSID nuevo: " + SSID1);
    t = true;
  }
  else
    Serial.println("SSID no fue modificado");
  String PASS = request->arg("pass");//server.arg("pass");
  if (!PASS.isEmpty())
  {
    writeFile(SPIFFS, "/PASS.txt", (char *)PASS.c_str());
    Serial.println("La contraseña fue cambiada correctamente");
    t = true;
  }
  else
    Serial.println("La contraseña no fue modificada");

  if (t)
  {
    Serial.println("Restarting in 5 seconds");
    request->redirect("/");
    vTaskDelay(5000);
    
    ESP.restart();
  }
}
