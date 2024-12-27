#ifndef LOG_CLIENT_H
#define LOG_CLIENT_H

#include <Arduino.h>
#include <WiFiUdp.h>

class LogClient {
private:
    bool serialEnabled = true;
    bool udpEnabled = false;
    WiFiUDP UDPConnection;
    IPAddress udpServer;
    uint16_t udpPort;
    String tagName;
    Stream* outputStream; // REMOVIDA a inicialização aqui

    void sendUdpMsg(const String& msg);

public:
    void begin(Stream& stream = Serial); // Valor padrão para Serial
    void setSerial(bool enable);
    void setUdp(bool enable);
    void setServer(const IPAddress& server, uint16_t port);
    void setTagName(const String& tag);

    void I(const String& msg);
    void W(const String& msg);
    void E(const String& msg);
};

extern LogClient Log;

#endif