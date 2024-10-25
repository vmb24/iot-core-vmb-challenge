#include <WiFi.h>               
#include <WiFiClientSecure.h>    
#include <PubSubClient.h>        
#include <DHT.h>
#include <OneWire.h>
#include <DallasTemperature.h>
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
String ssid, password;

// Declaração de funções
void listWiFiNetworks();
void connectWiFi();
void reconnectMQTT();
void sendToMQTT(String topic, String payload);
float calibrateSoilMoisture(int rawValue);

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
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
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

      // Impressão das leituras em colunas separadas por '|'
      Serial.print("Leitura ");
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print("Umidade do Solo: " + String(soilMoisture) + "% | ");
      Serial.print("Temperatura do Solo: " + String(soilTemp) + "°C | ");
      Serial.print("Luminosidade: " + String(luminosity) + " Lux | ");
      Serial.print("Umidade do Ar: " + String(airHumidity) + "% | ");
      Serial.println("Temperatura do Ar: " + String(airTemp) + "°C");
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
    sendToMQTT("agriculture/soil/moisture", "{\"moisture\": " + String(soilMoistureAvg) + ", \"status\": \"Normal\", \"crops\": [\"Milho\", \"Soja\"]}");
    sendToMQTT("agriculture/soil/temperature", "{\"temperature\": " + String(soilTempAvg) + ", \"status\": \"Normal\", \"crops\": [\"Milho\", \"Arroz\"]}");
    sendToMQTT("agriculture/brightness", "{\"brightness\": " + String(brightnessAvg) + ", \"status\": \"Suficiente\", \"crops\": [\"Milho\", \"Girassol\"]}");
    sendToMQTT("agriculture/air/moisture", "{\"moisture\": " + String(airMoistureAvg) + ", \"status\": \"Alta\", \"crops\": [\"Café\", \"Tomate\"]}");
    sendToMQTT("agriculture/air/temperature", "{\"temperature\": " + String(airTempAvg) + ", \"status\": \"Alta\", \"crops\": [\"Soja\", \"Batata\"]}");

    client.loop();

    // Impressão das médias finais
    Serial.println("Médias finais:");
    Serial.print("Umidade do Solo: " + String(soilMoistureAvg) + "% | ");
    Serial.print("Temperatura do Solo: " + String(soilTempAvg) + "°C | ");
    Serial.print("Luminosidade: " + String(brightnessAvg) + " Lux | ");
    Serial.print("Umidade do Ar: " + String(airMoistureAvg) + "% | ");
    Serial.println("Temperatura do Ar: " + String(airTempAvg) + "°C");
    Serial.println("------------------------------");

    Serial.println("Deseja realizar uma nova medição agora ou esperar 2 horas?");
    Serial.println("Digite 'N' para nova medição ou 'E' para esperar 2 horas:");

    while (Serial.available() == 0) {} 
    String resposta = Serial.readString();
    resposta.trim();

    if (resposta == "E" || resposta == "e") {
      Serial.println("Esperando por 2 horas...");
      delay(7200000);
    } else if (resposta == "N" || resposta == "n") {
      Serial.println("Nova medição será feita agora.");
    } else {
      Serial.println("Comando não reconhecido. Nova medição será feita.");
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

float calibrateSoilMoisture(int rawValue) {
  int dry = 700;
  int wet = 3200;
  return map(rawValue, dry, wet, 0, 100);
}
