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

  // descomentar para actualizar página web
  //deleteFile(SPIFFS, "/NoModif.html");
  //deleteFile(SPIFFS, "/WebServer.html");

  InicializarVariables();
  // String wifi_string = readFile(SPIFFS, "/wifi.txt");

  // BORRAR ESTO CUANDO TERMINE TODAS LAS PRUEBAS
  if (!SPIFFS.exists("/wifi.txt"))
  {
    String comando = "1";
    writeFile(SPIFFS, "/wifi.txt", (char *)comando.c_str());
  }

  initServer();

  /*xTaskCreate(
      TaskEscribirNFC, "TaskRedWifi",
      8192,
      NULL, 1,
      NULL);*/

  xTaskCreate(
      TaskLeerNFC, "TaskLeerNFC",
      4096,
      NULL, 1,
      NULL);
}

void loop() {}

// TAREAS
void TaskLeerNFC(void *pvParameters)
{
  nfc.begin();                                     // Inicializa o modulo
  uint32_t versiondata = nfc.getFirmwareVersion(); // Obtém informações sobre o modulo
  if (!versiondata)
  { // Verifica se a placa foi encontrada
    Serial.print("Placa PN53x no encontrada");
    while (1)
      ; // Loop infinito
  }
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  nfc.SAMConfig(); // Configura el módulo RFID para la lectura
  Serial.println("Esperando tarjeta ISO14443A ...");
  for (;;)
  {
    uint8_t success;                       // Controle de sucesso
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer UID
    uint8_t uidLength;                     // Tamanho da UID (4 ou 7 bytes depende do cartão)

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength); // Informacion do NFC

    if (success)
    {
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc.PrintHex(uid, uidLength); // Imprime el UID
      Serial.println("");

      if (uidLength == 4)
      { // Verifica que la tarjeta es de 4 bytes
        Serial.println("Tarjeta de 4 bytes");
        uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};                    // Clave A
        uint8_t keyb[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};                    // Clave B
        success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 6, 1, keyb); // Verifica claves

        if (success)
        {
          Serial.println("Sector 1 (Blocks 4..7) Autentificado");
          uint8_t data[16];                                   // Declaracion de un array de tamaño 16
          success = nfc.mifareclassic_ReadDataBlock(6, data); // Lectura bloque 6
          vTaskDelay(200);
          if (success)
          {
            Serial.println("Lectura realizada del bloque 4:");
            nfc.PrintHexChar(data, 16); // Imprime lo que está escrito en el bloque 6
            vTaskDelay(300);
            Serial.println("");

            Serial.println("Lectura de sector 1" + data[0]);
            vTaskDelay(1000);
          }
          else
            Serial.println("Ooops ... No es posible realizar la lectura, ¿la clave es correcta?");
        }
        else
          Serial.println("Ooops ... Fallo en la autenticación, ¿la clave es correcta?");
      }
      Serial.println("Separe la tarjeta porfavor. Puede volver a acercarla en 3 segundos");
      vTaskDelay(3000); // Espera 1s
    }
  }
}

void EscribirNFC(void *pvParameters)
{
  nfc.begin();                                     // Inicializa o modulo
  uint32_t versiondata = nfc.getFirmwareVersion(); // Obtém informações sobre o modulo
  if (!versiondata)
  { // Verifica se a placa foi encontrada
    Serial.print("Placa PN53x no encontrada");
    while (1)
      ; // Loop infinito
  }
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  nfc.SAMConfig(); // Configura el módulo RFID para la lectura
  Serial.println("Esperando tarjeta ISO14443A ...");
  for (;;)
  {
    uint8_t success;                       // Controle de sucesso
    uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer UID
    uint8_t uidLength;                     // Tamanho da UID (4 ou 7 bytes depende do cartão)

    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength); // Informacion do NFC

    if (success)
    {
      Serial.println("Found an ISO14443A card");
      Serial.print("  UID Length: ");
      Serial.print(uidLength, DEC);
      Serial.println(" bytes");
      Serial.print("  UID Value: ");
      nfc.PrintHex(uid, uidLength); // Imprime el UID
      Serial.println("");

      if (uidLength == 4)
      { // Verifica que la tarjeta es de 4 bytes
        Serial.println("Tarjeta de 4 bytes");
        uint8_t keya[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};                    // Clave A
        uint8_t keyb[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};                    // Clave B
        success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 6, 1, keyb); // Verifica claves

        if (success)
        {
          Serial.println("Sector 1 (Blocks 4..7) Autentificado");
          uint8_t data[16];                                   // Declaracion de un array de tamaño 16
          success = nfc.mifareclassic_ReadDataBlock(6, data); // Lectura bloque 6
          vTaskDelay(200);
          if (success)
          {
            Serial.println("Lectura realizada del bloque 4:");
            nfc.PrintHexChar(data, 16); // Imprime lo que está escrito en el bloque 6
            vTaskDelay(300);
            Serial.println("");

            Serial.println("Lectura de sector 1" + data[0]);
            vTaskDelay(1000);
          }
          else
            Serial.println("Ooops ... No es posible realizar la lectura, ¿la clave es correcta?");
        }
        else
          Serial.println("Ooops ... Fallo en la autenticación, ¿la clave es correcta?");
      }
      Serial.println("Separe la tarjeta porfavor. Puede volver a acercarla en 3 segundos");
      vTaskDelay(3000); // Espera 1s
    }
  }
}

// FUNCIONES DE AYUDA
void initServer()
{

  Serial.println("Configuring access point...");

  WiFi.softAP(SSID.c_str(), PASSWORD.c_str());
  IPAddress myIP = WiFi.softAPIP();

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("WebServer.html");
  // server.on("/", handleConnectionRoot);
  server.on("/changeSSID", HTTP_POST, procSSID);
  server.on("/location", HTTP_POST, procLocation);
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(400, "text/plain", "Not found"); });

  IP = myIP.toString();
  server.begin();
  Serial.print("SSID: ");
  Serial.println(SSID);
  Serial.print("IP address: ");
  Serial.println(IP);
}

void InicializarVariables()
{

  if (!SPIFFS.exists("/SSID.txt"))
  {
    String comando = "ConfigNFC";
    writeFile(SPIFFS, "/SSID.txt", (char *)comando.c_str());
  }
  if (!SPIFFS.exists("/PASS.txt"))
  {
    String comando = "ConfigNFC";
    writeFile(SPIFFS, "/PASS.txt", (char *)comando.c_str());
  }
  if (!SPIFFS.exists("/WebServer.html"))
  {
    writeFile(SPIFFS, "/WebServer.html", (char *)answer.c_str());
  }

  if (!SPIFFS.exists("/NoModif.html"))
  {
    writeFile(SPIFFS, "/NoModif.html", (char *)noModif.c_str());
  }
  if (!SPIFFS.exists("/ZONE.txt"))
  {
    writeFile(SPIFFS, "/ZONE.txt", "1");
  }
  if (!SPIFFS.exists("/DOOR.txt"))
  {
    writeFile(SPIFFS, "/DOOR.txt", "1");
  }
  answerNoModif = readFile(SPIFFS, "/NoModif.html");
  answer = readFile(SPIFFS, "/WebServer.html");
  SSID = readFile(SPIFFS, "/SSID.txt");
  PASSWORD = readFile(SPIFFS, "/PASS.txt");
  ZONA = readFile(SPIFFS, "/ZONE.txt");
  PUERTA = readFile(SPIFFS, "/DOOR.txt");
}

void procSSID(AsyncWebServerRequest *request)
{
  String SSID1 = request->arg("ssid"); // server.arg("ssid");
  bool t = false;

  if (!SSID1.isEmpty())
  {
    writeFile(SPIFFS, "/SSID.txt", (char *)SSID1.c_str());
    Serial.println("SSID nuevo: " + SSID1);
    t = true;
  }
  else
    Serial.println("SSID no fue modificado");

  String PASS = request->arg("pass"); // server.arg("pass");
  if (PASS.isEmpty())
    Serial.println("La contraseña no fue modificada");
  else if (PASS.length() < 8)
    Serial.println("La contraseña es menor de 8 caracteres");
  else
  {
    writeFile(SPIFFS, "/PASS.txt", (char *)PASS.c_str());
    Serial.println("La contraseña fue cambiada correctamente");
    t = true;
  }

  if (t)
  {
    request->send(200, "text/plain", "RESTARTING IN 5 SECONDS");
    Serial.println("Restarting in 5 seconds");
    vTaskDelay(5000);

    ESP.restart();
  }
  else
    request->send(200, "text/html", answerNoModif);
}

void procLocation(AsyncWebServerRequest *request)
{
  // Obtenemos valores de la web
  String ZONA1 = request->arg("zone");

  if (!ZONA1.isEmpty() && ZONA != ZONA1)
  {
    writeFile(SPIFFS, "/ZONE.txt", (char *)ZONA1.c_str());
    ZONA = ZONA1;
    Serial.println("ZONA NUEVA " + ZONA1);
  }
  else
    Serial.println("La zona no fue modificada: " + ZONA);

  String PUERTA1 = request->arg("door");
  if (!ZONA1.isEmpty() && ZONA != ZONA1)
  {
    writeFile(SPIFFS, "/DOOR.txt", (char *)PUERTA1.c_str());
    PUERTA = PUERTA1;
    Serial.println("PUERTA NUEVA " + PUERTA1);
  }
  else
    Serial.println("La puerta no fue modificada: " + PUERTA);

  request->send(200, "text/html", answerNoModif);
}
