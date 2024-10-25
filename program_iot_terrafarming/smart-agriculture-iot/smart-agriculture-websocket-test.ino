#include <WiFi.h>               
#include <WiFiClientSecure.h>    
#include <PubSubClient.h>        
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "certificates.h"

// Definindo os pinos dos sensores
#define DHTPIN 18               // Pino do sensor DHT para umidade e temperatura do ar
#define DHTTYPE DHT22           // Tipo de sensor DHT
#define SOIL_MOISTURE_PIN 36    // Pino do sensor de umidade do solo
#define LUMINOSITY_PIN 39       // Pino do sensor de luminosidade
#define DS18B20_PIN 4           // Pino do sensor de temperatura DS18B20 (ajuste conforme necessário)

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(DS18B20_PIN);           
DallasTemperature soilTempSensor(&oneWire); 

// Variáveis de WiFi e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

WebSocketsClient webSocket;
String logsUrl; 

// Variáveis BLE
BLEServer *pServer = nullptr;
BLEAdvertising *pAdvertising = nullptr;

// Declaração de funções
void setupBLE();
void listWiFiNetworks();
void connectWiFi();
void reconnectMQTT();
void sendToMQTT(String topic, String payload);
float calibrateSoilMoisture(int rawValue);
void sendLog(String message);

// Definindo UUIDs do serviço e característica BLE
#define SERVICE_UUID "4fafc201-1fb5-459e-8d40-b6b0e6e9b6b2"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

String ssid, password;
String crops[2];
String uuid;

// Funções BLE
class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("Cliente BLE conectado");
    }

    void onDisconnect(BLEServer* pServer) {
        Serial.println("Cliente BLE desconectado");
        pAdvertising->start(); // Reinicia a publicidade ao desconectar
        Serial.println("Publicidade BLE reiniciada.");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) {
        String value = pCharacteristic->getValue();
        StaticJsonDocument<200> doc;
        DeserializationError error = deserializeJson(doc, value);

        if (error) {
            Serial.print("Falha ao parsear JSON: ");
            Serial.println(error.f_str());
            return;
        }

        // Verifica se a chave "crops" existe e tem pelo menos dois elementos
        if (doc.containsKey("crops") && doc["crops"].is<JsonArray>() && doc["crops"].size() >= 2) {
            crops[0] = doc["crops"][0].as<String>();
            crops[1] = doc["crops"][1].as<String>();
            Serial.println("Culturas recebidas: " + crops[0] + ", " + crops[1]);
        } else {
            Serial.println("Erro: Dados de 'crops' incompletos ou ausentes");
            crops[0] = "";  // Define valores padrão caso os dados estejam incompletos
            crops[1] = "";
        }

        // Verifica se a chave "uuid" existe
        if (doc.containsKey("uuid")) {
            uuid = doc["uuid"].as<String>();
        } else {
            Serial.println("Erro: UUID ausente");
            uuid = "";  // Define valor padrão caso o UUID esteja ausente
        }

        // Exibe os valores recebidos para verificação
        Serial.println("Dados recebidos via BLE:");
        Serial.print("Cultura 1: "); Serial.println(crops[0].isEmpty() ? "Não recebida" : crops[0]);
        Serial.print("Cultura 2: "); Serial.println(crops[1].isEmpty() ? "Não recebida" : crops[1]);
        Serial.print("UUID: "); Serial.println(uuid.isEmpty() ? "Não recebido" : uuid);
    }
};

void connectWebSocket() {
    logsUrl = String(WiFi.localIP().toString()) + "/logs"; // Montando a URL
    Serial.println(logsUrl);

    // Separar o IP e a porta
    String host = WiFi.localIP().toString(); // Obtendo o IP
    uint16_t port = 80; // Defina a porta

    Serial.print("Conectando ao WebSocket em: ");
    Serial.print(host);
    Serial.print(":");
    Serial.println(port);

    // Conectando ao WebSocket
    webSocket.begin(host, port, "/logs"); // Chamada da função sem if

    // Agora, precisamos verificar se a conexão foi estabelecida
    Serial.println("Aguardando conexão WebSocket...");
    int attempts = 0;
    const int maxAttempts = 10; // Número máximo de tentativas de conexão

    while (attempts < maxAttempts) {
        webSocket.loop(); // Processa a comunicação do WebSocket

        if (webSocket.isConnected()) { // Verificando se a conexão foi estabelecida
            Serial.println("Conexão WebSocket estabelecida.");
            return; // Se conectado, saia da função
        }

        delay(1000); // Aguarde 1 segundo antes de tentar novamente
        attempts++;
    }

    Serial.println("Falha ao conectar ao WebSocket após várias tentativas.");
}

// Definição da função sendLog
void sendLog(String message) {
    if (webSocket.isConnected()) {
        webSocket.sendTXT(message); // Envia a mensagem através do WebSocket como texto
    } else {
        Serial.println("WebSocket não está conectado. Não foi possível enviar o log.");
    }
}

void setup() {
  Serial.begin(115200);
  delay(1000); 

  dht.begin();  
  soilTempSensor.begin();
  listWiFiNetworks();
  connectWiFi();

  espClient.setCACert(rootCA);
  espClient.setCertificate(certificate);
  espClient.setPrivateKey(privateKey);
  client.setServer(mqttEndpoint, mqttPort); 

  reconnectMQTT();

  setupBLE();
  connectWebSocket(); // Conectar ao WebSocket
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }

  // Aguarda a recepção dos dados de cultura e UUID via BLE antes de iniciar as medições
  if (crops[0].isEmpty() || crops[1].isEmpty() || uuid.isEmpty()) {
    Serial.println("Aguardando dados via BLE (cultura e UUID)...");
    sendLog("Aguardando dados via BLE (cultura e UUID)...");
    while (crops[0].isEmpty() || crops[1].isEmpty() || uuid.isEmpty()) {
      client.loop();
      delay(1000);
    }
    Serial.println("Dados recebidos via Bluetooth. Iniciando coletas...");
    sendLog("Dados recebidos via Bluetooth. Iniciando coletas...");
  }
  
  while (true) {
    float soilMoistureAvg = 0, soilTempAvg = 0, airMoistureAvg = 0, airTempAvg = 0, brightnessAvg = 0;

    for (int i = 0; i < 10; i++) {
      float soilMoisture = calibrateSoilMoisture(analogRead(SOIL_MOISTURE_PIN));
      float airHumidity = dht.readHumidity();
      float airTemp = dht.readTemperature();
      float luminosity = map(analogRead(LUMINOSITY_PIN), 0, 4095, 0, 100000);

      soilTempSensor.requestTemperatures();
      float soilTemp = soilTempSensor.getTempCByIndex(0);

      // Substituir por zero se não houver leitura válida
      soilMoisture = isnan(soilMoisture) ? 0 : soilMoisture;
      soilTemp = (soilTemp == DEVICE_DISCONNECTED_C || isnan(soilTemp)) ? 0 : soilTemp;
      airHumidity = isnan(airHumidity) ? 0 : airHumidity;
      airTemp = isnan(airTemp) ? 0 : airTemp;
      luminosity = isnan(luminosity) ? 0 : luminosity;

      String logMessage = "Leitura " + String(i + 1) + ": Umidade do Solo: " + String(soilMoisture) + "% | " +
                          "Temperatura do Solo: " + String(soilTemp) + "°C | " +
                          "Luminosidade: " + String(luminosity) + " Lux | " +
                          "Umidade do Ar: " + String(airHumidity) + "% | " +
                          "Temperatura do Ar: " + String(airTemp) + "°C | " +
                          "Culturas: " + crops[0] + ", " + crops[1];

      Serial.println(logMessage);
      sendLog(logMessage); // Enviando log pelo WebSocket
      Serial.println("------------------------------");

      soilMoistureAvg += soilMoisture;
      soilTempAvg += soilTemp;
      airMoistureAvg += airHumidity;
      airTempAvg += airTemp;
      brightnessAvg += luminosity;

      delay(2000);
    }

    // Cálculo das médias
    soilMoistureAvg /= 10;
    soilTempAvg /= 10;
    airMoistureAvg /= 10;
    airTempAvg /= 10;
    brightnessAvg /= 10;

    // Envio das médias para o MQTT
    sendToMQTT("agriculture/soil/moisture", "{\"moisture\": " + String(soilMoistureAvg) + ", \"status\": \"Normal\", \"crops\": [\"" + crops[0] + "\", \"" + crops[1] + "\"]}");
    sendToMQTT("agriculture/soil/temperature", "{\"temperature\": " + String(soilTempAvg) + ", \"status\": \"Normal\", \"crops\": [\"" + crops[0] + "\", \"" + crops[1] + "\"]}");
    sendToMQTT("agriculture/brightness", "{\"brightness\": " + String(brightnessAvg) + ", \"status\": \"Suficiente\", \"crops\": [\"" + crops[0] + "\", \"" + crops[1] + "\"]}");
    sendToMQTT("agriculture/air/moisture", "{\"moisture\": " + String(airMoistureAvg) + ", \"status\": \"Alta\", \"crops\": [\"" + crops[0] + "\", \"" + crops[1] + "\"]}");
    sendToMQTT("agriculture/air/temperature", "{\"temperature\": " + String(airTempAvg) + ", \"status\": \"Alta\", \"crops\": [\"" + crops[0] + "\", \"" + crops[1] + "\"]}");

    client.loop();

    // Log das médias finais
    String finalLogs = "Médias finais: Umidade do Solo: " + String(soilMoistureAvg) + "% | " +
                       "Temperatura do Solo: " + String(soilTempAvg) + "°C | " +
                       "Luminosidade: " + String(brightnessAvg) + " Lux | " +
                       "Umidade do Ar: " + String(airMoistureAvg) + "% | " +
                       "Temperatura do Ar: " + String(airTempAvg) + "°C | " +
                       "Culturas: " + crops[0] + ", " + crops[1];

    Serial.println(finalLogs);
    sendLog(finalLogs); // Enviando médias finais pelo WebSocket
    Serial.println("------------------------------");

    Serial.println("Deseja realizar uma nova medição agora ou esperar 2 horas?");
    Serial.println("Digite 'N' para nova medição ou 'E' para esperar 2 horas:");

    while (Serial.available() == 0) {}
    String resposta = Serial.readString();
    resposta.trim();

    if (resposta == "E" || resposta == "e") {
      Serial.println("Esperando por 2 horas...");
      sendLog("Esperando por 2 horas...");
      delay(7200000);
    } else if (resposta == "N" || resposta == "n") {
      Serial.println("Nova medição será feita agora.");
      sendLog("Nova medição será feita agora.");
    } else {
      Serial.println("Comando não reconhecido. Nova medição será feita.");
      sendLog("Comando não reconhecido. Nova medição será feita.");
    }
  }
}

void listWiFiNetworks() {
  Serial.println("Procurando redes WiFi disponíveis...");
  int numNetworks = WiFi.scanNetworks();

  if (numNetworks == 0) {
    Serial.println("Nenhuma rede encontrada.");
    return;
  }

  Serial.println("Redes WiFi disponíveis:");
  for (int i = 0; i < numNetworks; ++i) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.println(" dBm)");
    delay(10);
  }

  Serial.println("Digite o nome (SSID) da rede que você deseja conectar:");
  while (Serial.available() == 0) {}
  ssid = Serial.readString();
  ssid.trim();

  Serial.println("Digite a senha da rede WiFi:");
  while (Serial.available() == 0) {}
  password = Serial.readString();
  password.trim();
}

void connectWiFi() {
  Serial.println("Conectando à rede WiFi...");
  WiFi.begin(ssid.c_str(), password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado com sucesso!");
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT.");
    } else {
      Serial.print("Falha na conexão, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos.");
      delay(5000);
    }
  }
}

void sendToMQTT(String topic, String payload) {
  client.publish(topic.c_str(), payload.c_str());
}

void setupBLE() {
    BLEDevice::init("ESP32_WiFi_Credentials");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY |
        BLECharacteristic::PROPERTY_INDICATE
    );

    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    
    pAdvertising = pServer->getAdvertising();
    pAdvertising->setMinPreferred(0x06); // Aumentar a visibilidade
    pAdvertising->start();

    Serial.println("Bluetooth configurado.");
}

float calibrateSoilMoisture(int rawValue) {
  int dry = 700;
  int wet = 3200;
  return map(rawValue, dry, wet, 0, 100);
}