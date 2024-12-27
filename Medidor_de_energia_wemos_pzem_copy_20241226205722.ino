
#include <WiFi.h>
#include <MQTTClient.h>
#include <ArduinoOTA.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <PZEM004Tv30.h>
#include <SimpleTimer.h>
#include <Log.h>
#include <ESPmDNS.h>
//#include "secrets.h" // Importante: secrets.h FORA do diretório do sketch
#include "LogClient.hpp"
#include "TimeProvider.hpp"
#include "WebServer.hpp"
#include <Arduino.h>
#include "AppData.hpp"

#define FW_Version "1.0.5"

// Configuração do PZEM004T (UART TTL)
#define PZEM_RX 16
#define PZEM_TX 17

// Instancia o PZEM usando Serial2
PZEM004Tv30 pzem(Serial2, PZEM_RX, PZEM_TX);
AppData appData;

// Variáveis de controle
unsigned long pzemDataOK = 0;
unsigned long pzemDataNOK = 0;
int mState = 0;
unsigned long SLEEP_TIME = 60 * 1000;
const int MONITOR_LED = 2;
int monitor_led_state = LOW;
unsigned long ledBlink = 500;


// Variáveis WiFi e MQTT (CREDENCIAIS DIRETAMENTE NO CÓDIGO - NÃO RECOMENDADO)
const char* WIFI_SSID = "Menezes";          // Substitua pelo seu SSID
const char* WIFI_PASSWORD = "n@t132560"; // Substitua pela sua senha do WiFi
const char* MQTT_SERVER = "mqtt.eclipseprojects.io"; // Substitua pelo endereço do seu broker MQTT
const int MQTT_PORT = 1883;                 // Substitua pela porta do seu broker MQTT (geralmente 1883)
const char* MQTT_USER = "ESP8266_PowerMeter";     // Substitua pelo seu usuário MQTT
const char* MQTT_PASSWORD = "pass123"; // Substitua pela sua senha MQTT
const char* MQTT_CLIENT_ID = "ESP8266_PowerMeter"; // Substitua por um ID único para o seu cliente
const char* UDPLOG_SERVER = "192.168.1.68"; // Substitua pelo IP do seu servidor de log UDP
const int UDPLOG_PORT = 5014; // Substitua pela porta do seu servidor de log UDP


// Variáveis WiFi e MQTT
WiFiClient WIFIClient;
IPAddress udpServerAddress; // Agora no escopo global
char hostname[32];

SimpleTimer timer;
MQTTClient MQTT_client(512);
char MQTT_AttributesTopic[256];
char MQTT_TelemetryTopic[256];
char SensorAttributes[512];
char SensorTelemetry[512];

unsigned long pingMqtt = 5 * 60 * 1000;

LogClient Log; // Instancia o objeto LogClient

// Servidor Web Assíncrono
WebServer webServer(80);  // Instancia o WebServer

// Funções auxiliares

void setHostname() {
  sprintf_P(hostname, PSTR("ESP-PWRMETER-%04X"), ESP.getChipModel());
}

void calcAttributesTopic() {
  String s = "iot/device/" + String(MQTT_CLIENT_ID) + "/attributes";
  s.toCharArray(MQTT_AttributesTopic, 256);
}

void calcTelemetryTopic() {
  String s = "iot/device/" + String(MQTT_CLIENT_ID) + "/telemetry";
  s.toCharArray(MQTT_TelemetryTopic, 256);
}

void IOT_setAttributes() {
  String s = "[{\"type\":\"ESP32\"}," \
            "{\"ipaddr\":\"" + WiFi.localIP().toString() + "\"}," \
            "{\"macaddr\":\"" + WiFi.macAddress() + "\"}," \
            "{\"ssid\":\"" + WiFi.SSID() + "\"}," \
            "{\"rssi\":\"" + String(WiFi.RSSI()) + "\"}," \
            "{\"web\":\"http://" + WiFi.localIP().toString() + "\"}," \
            "{\"dataok\":" + String(pzemDataOK) + "}," \
            "{\"datanok\":" + String(pzemDataNOK) + "}" \
            "]";

  s.toCharArray(SensorAttributes, 512);
  Serial.println("PowerMeter Attributes:");
  Serial.println(SensorAttributes);
  MQTT_client.publish(MQTT_AttributesTopic, SensorAttributes);
}

void MQTT_callback(String &topic, String &payload) {
  Serial.println("Mensagem recebida no tópico: " + topic);
  Serial.println("Mensagem: " + payload);
}

bool MQTT_Connect() {
    Log.I("Conectando ao MQTT Broker..."); // Use o LogClient para mensagens

    if (MQTT_client.connected()) {
        Log.I("Já conectado ao MQTT.");
        return true;
    }

    MQTT_client.begin(MQTT_SERVER, MQTT_PORT, WIFIClient);

    if (MQTT_client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
        Log.I("Conectado ao MQTT!");
        // ... (outras configurações MQTT)
        return true;
    } else {
        Log.E("Falha na conexão MQTT, código: ");
        Log.E(String(MQTT_client.lastError())); // Ou outra forma de obter o erro, se disponível
        return false;
    }
}

void display_WIFIInfo() {
  Serial.println("WIFI conectado: " + WiFi.SSID());
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void OTA_Setup() {
  ArduinoOTA
    .onStart([]() {
      Serial.println("Iniciando OTA");
      digitalWrite(MONITOR_LED, HIGH);
    })
    .onEnd([]() {
      Serial.println("\nFim do OTA");
      digitalWrite(MONITOR_LED, LOW);
      ESP.restart();
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progresso OTA: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Erro OTA[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Autenticação Falhou");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Início Falhou");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Conexão Falhou");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Recebimento Falhou");
      else if (error == OTA_END_ERROR) Serial.println("Fim Falhou");
    });
  ArduinoOTA.setHostname(hostname);
  ArduinoOTA.begin();
}


void WIFI_Setup() {
  const char* ssid = WIFI_SSID;
  const char* pwd = WIFI_PASSWORD;
  WIFI_Setup(); // Chama a função WIFI_Setup para conectar ao Wi-Fi
  Log.begin(Serial2);

  Log.I("Conectando ao WiFi...");
  WiFi.disconnect();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, pwd);

  if (!WiFi.isConnected()) { // Verifica se o Wi-Fi está conectado
        Log.E("Falha na conexão Wi-Fi!");
        return; // Sai da função se a conexão falhar
  }

  if (!MQTT_Connect()) {
        Log.E("Falha ao conectar ao MQTT. Reiniciando em 10 segundos.");
        delay(10000);
        ESP.restart();
  }

  Log.I("");
  Log.I("WiFi conectado");
  Log.I("Endereço IP: ");
  Log.I(WiFi.localIP().toString());
  display_WIFIInfo();
}

void PWRMeter_Connect() {
  Serial.println("Conectando ao PZEM004T...");
    Serial2.begin(9600); // Inicializa a Serial2 com a velocidade correta (9600 bps)
  float v = pzem.voltage();
  if (isnan(v)) {
    Serial.println("Falha na conexão com o PZEM004T!");
    mState = 0;
        Serial2.end();
  } else {
    Serial.println("PZEM004T conectado!");
    mState = 2;
  }
}

void PWRMeter_getData() {
    if(!Serial2.available()){ //verifica se há dados disponíveis antes de ler
        Serial.println("Sem dados na Serial2");
        return;
    }
  float v = pzem.voltage();
  float i = pzem.current();
  float p = pzem.power();
  float e = pzem.energy();

  if (isnan(v)) {
    Serial.println("Erro ao ler dados do PZEM");
    pzemDataNOK++;
  } else {
    Serial.print("Tensão: "); Serial.println(v);
    Serial.print("Corrente: "); Serial.println(i);
    Serial.print("Potência: "); Serial.println(p);
    Serial.print("Energia: "); Serial.println(e);
    pzemDataOK++;

    String telemetry = "{\"voltage\":" + String(v) + ",\"current\":" + String(i) + ",\"power\":" + String(p) + ",\"energy\":" + String(e) + "}";
    telemetry.toCharArray(SensorTelemetry, 512);
    MQTT_client.publish(MQTT_TelemetryTopic, SensorTelemetry);
  }
}

void Blink_MonitorLed() {
  digitalWrite(MONITOR_LED, monitor_led_state);
  monitor_led_state = !monitor_led_state;
  timer.setTimeout(ledBlink, Blink_MonitorLed);
}

void printTime() {
  Serial.println(timeProvider.getFullTime()); // Implementação da função printTime
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600);
  delay(1000);
  webServer.setPzem(&pzem); // Passa o endereço do PZEM para o WebServer
  Log.begin(Serial); // Passa a Serial JÁ inicializada para o LogClient
  setHostname();
  webServer.setup(); // Inicializa o servidor web

  Log.setSerial(true);
  udpServerAddress.fromString(UDPLOG_SERVER);
  Log.setServer(udpServerAddress, UDPLOG_PORT);
  Log.setTagName("PWM01");

  pinMode(MONITOR_LED, OUTPUT);
  digitalWrite(MONITOR_LED, LOW);

  if (!pzem.begin()) {
        Serial.println("Erro ao inicializar o PZEM!");
        while (1);
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); // Conecta ao WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  display_WIFIInfo();

  Log.I("Habilitando o log UDP...");
  Log.setUdp(true);
  Log.W("------------------------------------------------> Reboot do sistema");

  OTA_Setup();

  timeProvider.setup();
  timeProvider.logTime();

  MQTT_Connect();
  IOT_setAttributes();

  Log.I("Configurando o servidor web...");
  webServer.setPzem(&pzem);
  webServer.setAppData(&appData); // Passa o endereço de appData para o WebServer
  webServer.setup();
  Log.I("Servidor web disponível na porta 80.");

  display_WIFIInfo();

  timer.setTimeout(ledBlink, Blink_MonitorLed);
  timer.setInterval(pingMqtt, IOT_setAttributes);
  timer.setInterval(3 * 60 * 1000, printTime);

  MDNS.begin(hostname);
  MDNS.addService("http", "tcp", 80);
}

void loop() {
  
  webServer.handle(); // Chama o método handle para processar as requisições
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado. Reconectando...");
    WiFi.reconnect();
    delay(5000); // Aguarda um pouco antes de tentar novamente
  }

  if (!MQTT_client.connected()) {
    Serial.println("MQTT desconectado. Reconectando...");
    if (!MQTT_Connect()) {
      Serial.println("Falha ao reconectar ao MQTT. Reiniciando em 10 segundos.");
      delay(10000);
      ESP.restart();
    } else {
      IOT_setAttributes(); // Reenvia os atributos após reconectar
    }
  }

  switch (mState) {
    case 0: // Conectar ao PZEM
      timer.setTimeout(3000, PWRMeter_Connect);
      ledBlink = 200; // Pisca rápido enquanto tenta conectar
      mState = 1;
      break;
    case 1: // Conectando (aguardando o timer)
      break;
    case 2: // Conectado, obter dados
      PWRMeter_getData();
      timer.setTimeout(SLEEP_TIME, []() { mState = 2; }); // Reinicia o timer e mantém o estado 2
      ledBlink = 500; // Pisca devagar quando conectado
      mState = 3; // Muda para o estado de "aguardando"
      break;
    case 3: // Aguardando o próximo ciclo (timer)
        break;
    default:
        break;
  }

  timer.run();
  MQTT_client.loop();
  ArduinoOTA.handle();
}