//Librerias
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

//definiciones de PN532
#define PN532_SCK (18)
#define PN532_MOSI (23)
#define PN532_SS (5)
#define PN532_MISO (19)
#define FORMAT_SPIFFS_IF_FAILED true

//Variables PN532
uint8_t DatoRecibido[4]; // Para almacenar los datos
Adafruit_PN532 nfc(PN532_SS); // Línea enfocada para la comunicación por SPI

//variables para la red
static String SSID = "";
static String PASSWORD = "";
static String IP = "";
AsyncWebServer server(80);
static String answer = "";


//Declaracion de funciones/tareas
void TaskLeerIdNFC(void *pvParameters);
void TaskRedWifi(void *pvParameters);
void handleConnectionRoot();
void InicializarVariables();
void procSSID(AsyncWebServerRequest *request);
void modificarVar(String ssid, String pswd);
void initServer();

//poner para que se redirija a la página principal con un contador y un botón si no se redirije automáticamente

static String noModif = "<!DOCTYPE html>\
<meta http-equiv='refresh' content='5; url=" + IP + "/' />\
<html>\
    <body>\
        <h1>Hola desde ESP32 - Modo Punto de Acceso(AP)</h1>\
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
                <li class='button'>\
                <button type='submit'> Enviar nueva configuración </button>\
                </li>\
            </ul>\
        </form>\
    </body>\
</html>";

static String pagina = "<!DOCTYPE html>\
<meta http-equiv='Content-Type' content='text/html; charset=UTF-8'/>\
<html>\
    <body>\
        <h1>Hola desde ESP32 - Modo Punto de Acceso(AP)</h1>\
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
                <li class='button'>\
                <button type='submit'> Enviar nueva configuración </button>\
                </li>\
            </ul>\
        </form>\
    </body>\
</html>";