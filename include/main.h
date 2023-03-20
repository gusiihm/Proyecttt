// Librerias
#include "Arduino.h"
#include <Wire.h>
#include <SPI.h> //Librería para comunicación SPI
#include <Adafruit_PN532.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include "FS.h"
#include "SPIFFS.h"
#include "SPIFF_fun.h"
#include "FreeRTOSConfig.h"
#include <PN532.h>
#include <PN532_HSU.h>

#define FORMAT_SPIFFS_IF_FAILED true

// Variables PN532
uint8_t DatoRecibido[4];     // Para almacenar los datos
PN532_HSU pn532hsu(Serial2); // Declara objeto de comunicação utilizando Serial2
PN532 nfc(pn532hsu);
uint8_t data1[16] = {0x48, 0x4f, 0x4c, 0x41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // HOLA en hex

// variables para la red
static String SSID = "";
static String PASSWORD = "";
static String IP = "";
AsyncWebServer server(80);
static String answer = "";
static String answerNoModif = "";

// variables del Kit
static String ZONA = "";
static String PUERTA = "";

// Declaracion de funciones/tareas
void TaskLeerNFC(void *pvParameters);
void InicializarVariables();
void procSSID(AsyncWebServerRequest *request);
void procLocation(AsyncWebServerRequest *request);
void procCreateNFC(AsyncWebServerRequest *request);
void procCreateNFCZone(AsyncWebServerRequest *request);
void initServer();
//void modificarVar(String ssid, String pswd);

// para redirigir
static String noModif = "<!DOCTYPE html>\
<meta http-equiv='refresh' content='1; url=" + IP + "/' />\
<html>\
<body>\
    <h2>REDIRIGIENDO...</h2>\
</body>\
</html>";

//PONER PARA QUE EL VALOR DE LA PUERTA Y DE LA ZONA SE MUESTRE EL QUE ES
static String pagina = "<!DOCTYPE html >\
<meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />\
<html>\
<body>\
    <h2>CONFIGURACIÓN KIT CERRADURAS NFC</h2>\
    <form action='/changeSSID' method='post'>\
        <ul>\
            <li>\
                <label> SSID:</label>\
                <input type='text' name='ssid'>\
            </li>\
            <li>\
                <label>Nueva contraseña: </label>\
                <input type='password' name='pass'>\
            </li>\
            <li>\
                <button type='submit'> Enviar nueva configuración </button>\
            </li>\
        </ul>\
    </form>\
    <br>\
    <form action='/location' method='post'>\
        <h3> UBICACIÓN </h3>\
        <ul>\
            <li>\
                <label>Escoge una zona:</label>\
                <input type='number' name='zone' value='1' min='0' max='99' />\
            </li>\
            <li>\
                <label>Escoge una puerta:</label>\
                <input type='number' name='door' value='1' min='0' max='99' />\
                </select>\
            </li>\
            <li>\
                <button type='submit'> ELEGIR </button>\
            </li>\
        </ul>\
    </form>\
    <br>\
	<h3> CREAR TARJETA NFC </h3>\
    <div>\
        <ul>\
            <li>\
                <form action='/createNFC' method='post'>\
                    <button type='submit'> PARA PUERTA </button>\
                </form>\
            </li>\
            <li>\
                <form action='/createNFCZona' method='post'>\
                    <button type='submit'> PARA ZONA </button>\
                </form>\
            </li>\
        </ul>\
    </div>\
</body>\
</html>";