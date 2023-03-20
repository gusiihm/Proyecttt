#include "main.h"

// TARJETA NFC =  2001/2003/3004 abriendo la puerta 1, y 3 de la zona 2 y la puerta 4 de la zona 3.

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
  //------ Borrar cuando se acabe de modificar la página web -----
  deleteFile(SPIFFS, "/NoModif.html");
  deleteFile(SPIFFS, "/WebServer.html");
  //-- Se cargan en la parte de inicializar variables
  
  InicializarVariables();
  initServer();

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
    Serial.print("Placa PN53x no encontrada lectura nfc");
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
        uint8_t key[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Clave

        for (int i = 1; i <= 15; i++)
        {
          // Imprime por consola sector y bloques
          {
            Serial.print("\n --------- \n Sector ");
            Serial.print(i, DEC);
            Serial.print("(Blocks ");
            Serial.print(i * 4, DEC);
            Serial.print("..");
            Serial.print(i * 4 + 3, DEC);
            Serial.println(") Autentificado");
          }

          success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, (i * 4), 1, key); // Verifica claves

          if (success)
          {
            for (int x = 0; x < 3; x++)
            {

              uint8_t data[16];                                           // Declaracion de un array de tamaño 16
              success = nfc.mifareclassic_ReadDataBlock(i * 4 + x, data); // Lectura bloque x del sector i
              // vTaskDelay(200);
              if (success)
              {
                Serial.print("Lectura realizada del bloque ");
                Serial.print(i * 4 + x, DEC);
                Serial.print(": ");
                nfc.PrintHexChar(data, 16); // Imprime lo que está escrito en el bloque x

                // Verificar si abre puerta o si vacio
                for (int y = 0; y <= 15; y = y + 2)
                {
                  uint8_t z = data[y];
                  uint8_t p = data[y + 1];

                  if( z == ZONA.toInt() && ( p == PUERTA.toInt() || p == 0x00 )){
                    //LLAMAR AQUÍ A LA FUNCION DE ABRIR LA PUERTA
                    Serial.println("LECTURA CORRECTA. ABRIENDO PUERTA...");
                    break; //cambiar indices para salir del bucle
                  }
                }
              }
              else
              {
                Serial.print("No es posible realizar la lectura del bloque ");
                Serial.println(i * 4 + x, DEC);
              }
            }
          }
          else
          {
            Serial.print("La clave no es correcta para el sector ");
            Serial.println(i, DEC);
          }
        }
        Serial.println("Lectura de la tarjeta. Separe la tarjeta porfavor. Puede volver a acercarla en 3 segundos");
        vTaskDelay(3000); // Espera s
      }
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
  server.on("/createNFC", HTTP_POST, procCreateNFC);
  server.on("/createNFCZona", HTTP_POST, procCreateNFCZone);
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
    writeFile(SPIFFS, "/WebServer.html", (char *)pagina.c_str());
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

// hay que añadirle que está el 0x en el caso de que se use
int HexToDec(char num[])
{
  int len = strlen(num);
  int base = 1;
  int temp = 0;
  for (int i = len - 1; i >= 0; i--)
  {
    if (num[i] >= '0' && num[i] <= '9')
    {
      temp += (num[i] - 48) * base;
      base = base * 16;
    }
    else if (num[i] >= 'A' && num[i] <= 'F')
    {
      temp += (num[i] - 55) * base;
      base = base * 16;
    }
  }
  return temp;
}

// PROCESAR API's

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
  if (!PUERTA1.isEmpty() && PUERTA1 != PUERTA)
  {
    writeFile(SPIFFS, "/DOOR.txt", (char *)PUERTA1.c_str());
    PUERTA = PUERTA1;
    Serial.println("PUERTA NUEVA " + PUERTA1);
  }
  else
    Serial.println("La puerta no fue modificada: " + PUERTA);

  request->send(200, "text/html", answerNoModif);
}

// Lee la tarjeta, si tiene la misma zona y puerta pues no se graba nada, si no, se añade
// Lo suyo es que tenga un buzzer, pite para "decir estoy preparado" y pite largo cuando se esté leyendo (con leds que se enciendan en fila)
void procCreateNFC(AsyncWebServerRequest *request)
{
  Serial.println("CREAR TARJETA NFC CON UBICACIÓN");
  Serial.println("Redirigiendo....");
  request->send(200, "text/html", answerNoModif);
}

void procCreateNFCZone(AsyncWebServerRequest *request)
{
  Serial.println("CREAR TARJETA NFC PARA ZONA");
  Serial.println("Redirigiendo....");
  request->send(200, "text/html", answerNoModif);
}




/*
uint32_t versiondata = nfc.getFirmwareVersion(); // Obteniendo informacion sobre la placa
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
}*/