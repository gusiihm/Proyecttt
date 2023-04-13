#include "main.h"

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

  if (SPIFFS.exists("/data.txt"))
    deleteFile(SPIFFS, "/data.txt");

  deleteFile(SPIFFS, "/WebServer.html");
  deleteFile(SPIFFS, "/NoModif.html");
  deleteFile(SPIFFS, "/PASS.txt");

  //-- Se cargan en la parte de inicializar variables

  InicializarVariables();
  initServer();

  xTaskCreate(
      TaskLeerNFC, "TaskLeerNFC",
      4096,
      NULL, 1,
      &xLeerNFC);
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
    uint8_t uidLength;                     // Tamanho da UID (4 ou 7 bytes depende de tarjeta)

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
        uint8_t fini = 0;
        if (VerificaPass(nfc, uid, uidLength, key))
        {
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

                    if (z == ZONA.toInt() && (p == PUERTA.toInt() || p == 0x00))
                    {
                      // LLAMAR AQUÍ A LA FUNCION DE ABRIR LA PUERTA
                      Serial.println("LECTURA CORRECTA. ABRIENDO PUERTA...");
                      fini = 1;
                      break; // cambiar indices para salir del bucle
                    }
                  }
                }
                else
                {
                  Serial.print("No es posible realizar la lectura del bloque ");
                  Serial.println(i * 4 + x, DEC);
                }
                if (fini)
                  break;
              }
            }
            else
            {
              Serial.print("La clave no es correcta para el sector ");
              Serial.println(i, DEC);
            }
            if (fini)
              break;
          }
        }
        Serial.println("SEPARE LA TARJETA. En 3 segundos se podrá leer la tarjeta");
        vTaskDelay(3000);
        Serial.println("------- \n\n Esperando tarjeta ISO14443A ...");
      }
    }
  }
}

void TaskWriteNFC(void *pvParameters)
{
  Serial.println("Array paras: llega a antes de paras");

  // Arreglar esto
  String parametro = *((String *)pvParameters);
  String tipo = parametro.substring(0, 1);
  String contra = "";

  if (tipo == "2")
    contra = parametro.substring(2);

  Serial.println("Array paras: tipo-> " + tipo + " Contraseña->" + contra);

  // PONER PARA QUE SI ESTA CONFIGURADA LA PUERTA, SE CAMBIE A ZONA SI SE HA ELEGIDO ZONA

  if (tipo == "0")
  {
    unsigned long t0 = millis();
    Serial.println("CREAR TARJETA NFC CON UBICACIÓN");
    Serial.println("Esperando tarjeta NFC. Acerque la tarjeta. Se cancelará en 10s");
    while ((millis() - t0) < 10000)
    {
      uint8_t success;                       // Control de success
      uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer UID
      uint8_t uidLength;                     // Tamaño de UID (4 o 7 bytes depende de tarjeta)
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

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

          if (VerificaPass(nfc, uid, uidLength, key))
          {
            uint8_t fini = 0;
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

                      if (z == ZONA.toInt() && (p == PUERTA.toInt() || p == 0x00)) // Tarjeta ya tiene registrado la puerta
                      {
                        Serial.println("TARJETA YA CONFIGURADA PARA ABRIR ESTA PUERTA");
                        fini = 1;
                        break;
                      }
                      if (z == 0x00 && p == 0x00) // primer sitio de tarjeta que esté vacío
                      {
                        data[y] = ZONA.toInt();
                        data[y + 1] = PUERTA.toInt();

                        success = nfc.mifareclassic_WriteDataBlock(i * 4 + x, data); // Escribe en la tarjeta NFC

                        if (success)
                          Serial.println("Registrando");
                        else
                          Serial.println("No es posible registrar");

                        fini = 1;
                        break;
                      }
                    }
                  }
                  else
                  {
                    Serial.print("No es posible realizar la lectura del bloque ");
                    Serial.println(i * 4 + x, DEC);
                  }
                  if (fini)
                    break;
                }
              }
              else
              {
                Serial.print("La clave no es correcta para el sector ");
                Serial.println(i, DEC);
              }
              if (fini)
                break;
            }
          }
          else
            Serial.println("Tarjeta no configurada con contraseña");
        }
        else
          Serial.println("Tarjeta no válida");

        break; // Salimos del bucle
      }
    }
  }
  if (tipo == "1")
  {
    unsigned long t0 = millis();
    Serial.println("CREAR TARJETA NFC CON ZONA");
    Serial.println("Esperando tarjeta NFC. Acerque la tarjeta. Se cancelará en 10s");
    while ((millis() - t0) < 10000)
    {
      uint8_t success;                       // Control de success
      uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer UID
      uint8_t uidLength;                     // Tamaño de UID (4 o 7 bytes depende de tarjeta)
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
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

          if (VerificaPass(nfc, uid, uidLength, key))
          {
            uint8_t fini = 0;
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

                    for (int y = 0; y <= 15; y = y + 2)
                    {
                      uint8_t z = data[y];
                      uint8_t p = data[y + 1];

                      if (z == ZONA.toInt()) // Tarjeta ya tiene registrado la puerta
                      {
                        if (p == PUERTA.toInt())
                        {
                          Serial.println("TARJETA YA CONFIGURADA PARA ABRIR ESTA PUERTA");
                          data[y + 1] = 0x00;
                          success = nfc.mifareclassic_WriteDataBlock(i * 4 + x, data); // Escribe en la tarjeta NFC

                          if (success)
                            Serial.println("ZONA AÑADIDA CON ÉXITO");
                          else
                            Serial.println("No es posible registrar");
                        }
                        if (p == 0x00)
                          Serial.println("TARJETA YA CONFIGURADA PARA ABRIR ESTA ZONA");

                        fini = 1;
                        break;
                      }
                      if (z == 0x00 && p == 0x00) // primer sitio de tarjeta que esté vacío
                      {
                        data[y] = ZONA.toInt();
                        data[y + 1] = 0x00;

                        success = nfc.mifareclassic_WriteDataBlock(i * 4 + x, data); // Escribe en la tarjeta NFC

                        if (success)
                          Serial.println("ZONA AÑADIDA CON ÉXITO");
                        else
                          Serial.println("No es posible registrar");

                        fini = 1;
                        break;
                      }
                    }
                  }
                  else
                  {
                    Serial.print("No es posible realizar la lectura del bloque ");
                    Serial.println(i * 4 + x, DEC);
                  }
                  if (fini)
                    break;
                }
              }
              else
              {
                Serial.print("La clave no es correcta para el sector ");
                Serial.println(i, DEC);
              }
              if (fini)
                break;
            }
          }
          else
            Serial.println("Tarjeta no configurada con contraseña");
        }
        else
          Serial.println("Tarjeta no válida");
        break;
      }
    }
  }
  if (tipo == "2")
  {
    unsigned long t0 = millis();
    Serial.println("CONFIGURAR TARJETA CON CONTRASEÑA");
    Serial.println("Esperando tarjeta NFC. Acerque la tarjeta. Se cancelará en 10s");
    while ((millis() - t0) < 10000)
    {
      uint8_t success;                       // Control de success
      uint8_t uid[] = {0, 0, 0, 0, 0, 0, 0}; // Buffer UID
      uint8_t uidLength;                     // Tamaño de UID (4 o 7 bytes depende de tarjeta)
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
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

          success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, key);
          if (success)
          {
            success = nfc.mifareclassic_WriteDataBlock(4, CONTRASENA); // Escribe en la tarjeta NFC

            if (success)
              Serial.println("Registrando");
            else
              Serial.println("No es posible registrar");
          }
          else
            Serial.println("Fallo en la autenticación de bloque");
        }
        else
          Serial.println("Tarjeta no válida");
        break;
      }
    }
  }

  Serial.println("SEPARE LA TARJETA. En 3 segundos se podrá leer la tarjeta");
  vTaskDelay(3000);
  Serial.println("------- \n\n Esperando tarjeta ISO14443A ...");
  vTaskResume(xLeerNFC);
  vTaskDelete(xWriteNFC);
}

// FUNCIONES DE AYUDA
void initServer()
{

  Serial.println("Configuring access point...");

  WiFi.softAP(SSID.c_str(), PASSWORD.c_str());
  IPAddress myIP = WiFi.softAPIP();
  String h = IP + "/";
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("WebServer.html");
  server.on("/changeSSID", HTTP_POST, procSSID);
  server.on("/location", HTTP_POST, procLocation);
  server.on("/createNFC", HTTP_POST, procCreateNFC);
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
    writeFile(SPIFFS, "/NoModif.html", (char *)paginaNoModif.c_str());
  }
  if (!SPIFFS.exists("/ZONE.txt"))
  {
    writeFile(SPIFFS, "/ZONE.txt", "1");
  }
  if (!SPIFFS.exists("/DOOR.txt"))
  {
    writeFile(SPIFFS, "/DOOR.txt", "1");
  }
  if (!SPIFFS.exists("/CONTRASENA.txt"))
  {
    writeFile(SPIFFS, "/CONTRASENA.txt", "predeterminado");
  }
  
  answerNoModif = readFile(SPIFFS, "/NoModif.html");
  answer = readFile(SPIFFS, "/WebServer.html");
  SSID = readFile(SPIFFS, "/SSID.txt");
  PASSWORD = readFile(SPIFFS, "/PASS.txt");
  ZONA = readFile(SPIFFS, "/ZONE.txt");
  PUERTA = readFile(SPIFFS, "/DOOR.txt");

  //Se borraría en producto final
  Serial.print("PASS: ");
  Serial.println(PASSWORD);
  Serial.print("ZONA ACTUAL: ");
  Serial.println(ZONA);
  Serial.print("PUERTA ACTUAL: ");
  Serial.println(PUERTA);
  Serial.print("CONTRA: ");
  nfc.PrintHex(CONTRASENA, 4);

}

int VerificaPass(PN532 nfc, uint8_t uid[7], uint8_t uidLength, uint8_t key[6])
{
  Serial.println("\n\n VERIFICANDO....");

  uint8_t success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, key);
  if (success)
  {
    uint8_t data[16];
    success = nfc.mifareclassic_ReadDataBlock(4, data);
    if (success)
    {
      // arreglar esto porque no va bien xd
      for (int i = 0; i < 16; i++)
      {
        success = data[i] == CONTRASENA[i];
        if (!success)
          break;
      }

      if (success)
      {
        Serial.println("TARJETA CONFIGURADA CORRECTAMENTE \n\n");
        return 1;
      }
      else
      {
        Serial.println("Tarjeta NO configurada \n\n");

        return 0;
      }
    }
    else
    {
      Serial.println("Fallo en lectura de bloque verificación");
      return 0;
    }
  }
  else
  {
    Serial.println("Fallo en autenticacion de bloque verificación");
    return 0;
  }
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
  vTaskSuspend(xLeerNFC); // Suspendemos tarea de lectura

  String para = request->arg("type") + "," + request->arg("pass");

  xTaskCreate(
      TaskWriteNFC, "TaskWriteNFC", // Creamos tarea de escritura (hay que copiar lo de abajo y que se automate la tarea)
      4096,
      &para, 1,
      &xWriteNFC);

  request->send(200, "text/html", pagTjPass);
}
