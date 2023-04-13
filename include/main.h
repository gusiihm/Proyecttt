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
uint8_t CONTRASENA[16] = {0x03, 0x35, 0xd1, 0x01, 0x31, 0x55, 0x00, 0x45, 0x73, 0x74, 0x6f, 0x45, 0x73, 0x20, 0x70, 0x61};

// Declaracion de funciones/tareas
void TaskLeerNFC(void *pvParameters);
void TaskWriteNFC(void *pvParameters);
xTaskHandle xLeerNFC;
xTaskHandle xWriteNFC;
void InicializarVariables();
void procSSID(AsyncWebServerRequest *request);
void procLocation(AsyncWebServerRequest *request);
void procCreateNFC(AsyncWebServerRequest *request);
void initServer();
void handleConnectionRoot(AsyncWebServerRequest *request);
int VerificaPass(PN532 nfc, uint8_t uid[7], uint8_t uidLength, uint8_t key[6]);

// para redirigir
static String paginaNoModif = "<!DOCTYPE html>\
<meta http-equiv='refresh' content='1; url=" +
                              IP + "/' />\
<html>\
<body>\
    <h2>REDIRIGIENDO...</h2>\
</body>\
</html>";

static String pagTjPass = "<!DOCTYPE html>\
<meta http-equiv='refresh' content='10; url=" +
                              IP + "/' />\
<html>\
<body>\
    <h2>Esperando tarjeta NFC. Acerque la tarjeta...</h2>\
</body>\
</html>";

// PONER PARA QUE EL VALOR DE LA PUERTA Y DE LA ZONA SE MUESTRE EL QUE ES
static String pagina = "<!DOCTYPE html >\
<html>\
<head>\
    <meta http-equiv='Content-Type' content='text/html; charset=UTF-8' />\
    <style>\
		body {\
			font-family: Arial, sans-serif;\
			background-color: #f2f2f2;\
			margin: 0;\
			padding: 0;\
		}\
		h1 {\
			font-size: 32px;\
			font-weight: bold;\
			text-align: center;\
			margin-top: 50px;\
			margin-bottom: 20px;\
		}\
		form {\
			background-color: #ffffff;\
			padding: 20px;\
			border-radius: 10px;\
			box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);\
			max-width: 500px;\
			margin: 0 auto;\
		}\
		form ul {\
			list-style-type: none;\
			padding: 0;\
			margin: 0;\
		}\
		form li {\
			margin-bottom: 10px;\
		}\
		form label {\
			display: inline-block;\
			width: 150px;\
			font-weight: bold;\
			margin-right: 10px;\
		}\
		form input[type='text'],\
		form input[type='password'],\
		form input[type='number'] {\
			display: block;\
			width: 100%;\
			padding: 10px;\
			border: 1px solid #ccc;\
			border-radius: 5px;\
			box-sizing: border-box;\
			font-size: 16px;\
			margin-top: 5px;\
		}\
		form button[type='submit'] {\
			background-color: #007bff;\
			color: #ffffff;\
			padding: 10px 20px;\
			border: none;\
			border-radius: 5px;\
			font-size: 16px;\
			cursor: pointer;\
			margin-top: 10px;\
		}\
		form button[type='submit']:hover {\
			background-color: #0062cc;\
		}\
		h3 {\
			font-size: 20px;\
			font-weight: bold;\
			margin-top: 30px;\
			margin-bottom: 10px;\
			text-align: center;\
		}\
		div {\
			display: flex;\
			flex-wrap: wrap;\
			justify-content: space-between;\
			max-width: 500px;\
			margin: 0 auto;\
			margin-top: 20px;\
		}\
		div form {\
			flex-basis: 48%;\
			background-color: #ffffff;\
			padding: 20px;\
			border-radius: 10px;\
			box-shadow: 0 0 10px rgba(0, 0, 0, 0.2);\
			margin-bottom: 20px;\
		}\
		div form button[type='submit'] {\
			width: 100%;\
		}\
	</style>\
<head/>\
<body>\
    <h1>CONFIGURACIÓN KIT CERRADURAS NFC</h1>\
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
                <button type='submit'> Enviar nueva configuración </button>\
        </ul>\
    </form>\
    <br>\
    <form action='/location' method='post'>\
        <h3> UBICACIÓN </h3>\
        <ul>\
            <li>\
                <label>Escoge una zona:</label>\
                <input type='number' name='zone' value='1' min='1' max='99' required/>\
            </li>\
            <li>\
                <label>Escoge una puerta:</label>\
                <input type='number' name='door' value='1' min='1' max='99' required/>\
            </li>\
                <button type='submit'> ELEGIR </button>\
        </ul>\
    </form>\
    <br>\
	<h3> CREAR TARJETA NFC </h3>\
    <form action='/createNFC' method='post'>\
        <ul>\
            <input type='hidden' name='type' value='2' />\
            <li>\
                <label>Configurar tarjeta con contraseña</label>\
                <input type='text' name='pass' minlength='8' maxlength='16' required />\
            </li>\
            <button type='submit'> AÑADIR </button>\
        </ul>\
    </form>\
    <div>\
        <form action='/createNFC' method='post'>\
            <input type='hidden' name='type' value='0'/>\
            <button type='submit'> PARA PUERTA </button>\
        </form>\
        <form action='/createNFC' method='post'>\
            <input type='hidden' name='type' value='1'/>\
            <button type='submit'> PARA ZONA </button>\
        </form>\
    </div>\
</body>\
</html>";