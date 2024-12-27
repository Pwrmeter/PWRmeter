#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PZEM004Tv30.h>
#include "AppData.hpp" // Inclua AppData.hpp

class WebServer {
private:
    AsyncWebServer server;
    bool initialized = false;
    PZEM004Tv30* pzemInstance = nullptr;
    AppData* appDataInstance = nullptr; // Adicione um ponteiro para AppData

public:
    WebServer(int port = 80);
    void setup();
    void handle();
    void setPzem(PZEM004Tv30* pzem);
    void setAppData(AppData* appData); // Método para definir o AppData
};

#endif