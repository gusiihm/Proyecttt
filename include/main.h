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
#include <PN532.h>
#include <PN532_HSU.h>

#define FORMAT_SPIFFS_IF_FAILED true

//Variables PN532
uint8_t DatoRecibido[4]; // Para almacenar los datos
PN532_HSU pn532hsu(Serial2); // Declara objeto de comunicação utilizando Serial2
PN532 nfc(pn532hsu);
uint8_t data1[16] = {0x48, 0x4f, 0x4c, 0x41, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // HOLA en hex


//variables para la red
static String SSID = "";
static String PASSWORD = "";
static String IP = "";
AsyncWebServer server(80);
static String answer = "";
static String answerNoModif = "";


//Declaracion de funciones/tareas
void TaskLeerIdNFC(void *pvParameters);
void TaskRedWifi(void *pvParameters);
void InicializarVariables();
void procSSID(AsyncWebServerRequest *request);
void modificarVar(String ssid, String pswd);
void initServer();

//para redirigir
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