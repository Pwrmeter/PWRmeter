#include <WiFi.h>
#include "LogClient.hpp"

LogClient Log;

void LogClient::begin(Stream& stream) {
    outputStream = &stream;
}

void LogClient::sendUdpMsg(const String& msg) {
    if (udpEnabled && udpServer != INADDR_NONE && udpPort != 0 && WiFi.status() == WL_CONNECTED) {
        String message = "[" + tagName + "] " + msg;
        int packetSize = message.length() + 1;
        char buffer[packetSize];
        message.toCharArray(buffer, packetSize);

        UDPConnection.beginPacket(udpServer, udpPort);
        UDPConnection.write(reinterpret_cast<const uint8_t*>(buffer), packetSize - 1);
        UDPConnection.endPacket();
    }
}

void LogClient::I(const String& msg) { // APENAS UMA definição de I()
    if (serialEnabled) {
        outputStream->print("[I] ");
        outputStream->println(msg);
    }
    sendUdpMsg("[I] " + msg); // Use a versão com const String& aqui também
}

void LogClient::W(const String& msg) { // APENAS UMA definição de W()
    if (serialEnabled) {
        outputStream->print("[W] ");
        outputStream->println(msg);
    }
    sendUdpMsg("[W] " + msg); // Use a versão com const String& aqui também
}

void LogClient::E(const String& msg) { // APENAS UMA definição de E()
    if (serialEnabled) {
        outputStream->print("[E] ");
        outputStream->println(msg);
    }
    sendUdpMsg("[E] " + msg); // Use a versão com const String& aqui também
}