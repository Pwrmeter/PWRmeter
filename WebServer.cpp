#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "WebServer.hpp"
#include "html_templates.h"
#include "trianglify.min.js-gz.h"

WebServer::WebServer() : server(80), pzemInstance(nullptr), appDataInstance(nullptr) { // Definição AQUI (CORRETO)
    initialized = false;
}

WebServer::WebServer(int port) : server(port), pzemInstance(nullptr), appDataInstance(nullptr) {
    initialized = false;
}

void WebServer::setPzem(PZEM004Tv30* pzem) {
    pzemInstance = pzem;
}

void WebServer::setAppData(AppData* appData) {
    appDataInstance = appData;
}

void WebServer::setup() {
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        AsyncResponseStream* response = request->beginResponseStream("text/html");
        if (this->pzemInstance != nullptr && this->appDataInstance != nullptr) { // Verifique se ambos são válidos
            float voltage = this->pzemInstance->voltage();
            float current = this->pzemInstance->current();
            float power = this->pzemInstance->power();
            float energy = this->pzemInstance->energy();
             float frequency = this->pzemInstance->frequency();
            float pf = this->pzemInstance->pf();
            response->printf(TEMPLATE_HEADER, this->appDataInstance->getFWVersion().c_str(), // Use this->appDataInstance
                              this->appDataInstance->getDevIP().c_str(), this->appDataInstance->getGWIP().c_str(),
                              this->appDataInstance->getDevMAC().c_str(), this->appDataInstance->getSSID().c_str(),
                              this->appDataInstance->getRSSI().c_str(), this->appDataInstance->getLogServerIPInfo().c_str(),
                              this->appDataInstance->getHeap().c_str(), String(voltage).c_str(),
                              String(current).c_str(), String(power).c_str(),
                              String(energy).c_str(),String(frequency).c_str(), String(pf).c_str(), this->appDataInstance->getSamplesOK().c_str(),
                              this->appDataInstance->getSamplesNOK().c_str(), this->appDataInstance->getPZEMState().c_str());
        } else {
            response->printf(TEMPLATE_HEADER, this->appDataInstance->getFWVersion().c_str(), // Use this->appDataInstance
                              this->appDataInstance->getDevIP().c_str(), this->appDataInstance->getGWIP().c_str(),
                              this->appDataInstance->getDevMAC().c_str(), this->appDataInstance->getSSID().c_str(),
                              this->appDataInstance->getRSSI().c_str(), this->appDataInstance->getLogServerIPInfo().c_str(),
                              this->appDataInstance->getHeap().c_str(), "N/A", "N/A", "N/A", "N/A","N/A", "N/A",
                              this->appDataInstance->getSamplesOK().c_str(), this->appDataInstance->getSamplesNOK().c_str(), this->appDataInstance->getPZEMState().c_str());
        }

        response->print(TEMPLATE_FOOTER);
        request->send(response);
    });

    server.onNotFound(Web_PageNotFound);

    server.on("/trianglify.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript",
                                                                  trianglify_min_js_gz, sizeof(trianglify_min_js_gz));
        response->addHeader("Content-Encoding", "gzip");
        request->send(response);
    });

    server.begin();
    initialized = true;
}

void WebServer::handle() {
    if (!initialized) {
        setup();
    }
}

